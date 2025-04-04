/*

For sysman API information, see:

    https://oneapi-src.github.io/level-zero-spec/level-zero/latest/sysman/PROG.html#sysman-programming-guide

and

    https://github.com/intel/compute-runtime/tree/master/programmers-guide

*/
#include "args.h"    // for arg_search_t, arg_enum, process_devi...
#include "device.h"  // for ze_error_to_str, engine_type_to_str
#include "engine.h"  // for ze_error_to_str, engine_type_to_str
#include "helpers.h" // for ze_error_to_str, engine_type_to_str
#include "power_domain.h"
#include "process.h"     // for ze_error_to_str, engine_type_to_str
#include "temperature.h" // for ze_error_to_str, engine_type_to_str
#include "ze-ncurses.h"
#include <algorithm>            // for min
#include <exception>            // for exception
#include <fstream>              // for basic_ostream, operator<<, endl, bas...
#include <iomanip>              // for operator<<, setfill, setw
#include <iostream>             // for cerr, cout
#include <level_zero/ze_api.h>  // for _ze_result_t, ze_result_t, ZE_MAX_DE...
#include <level_zero/zes_api.h> // for zes_device_handle_t, _zes_structure_...
#include <memory>               // for unique_ptr, allocator, make_unique
#include <ncurses.h>            // for move, wprintw, stdscr, noecho, cbreak
#include <sstream>              // for basic_ostringstream
#include <stdexcept>            // for runtime_error
#include <stdint.h>             // for uint32_t, uint64_t
#include <stdio.h>              // for printf, fprintf, stderr, size_t, NULL
#include <stdlib.h>             // for free, malloc, exit
#include <string>               // for char_traits, basic_string, operator<<
#include <sys/types.h>          // for pid_t
#include <variant>              // for get
#include <vector>               // for vector

void draw_utilization_bar(int width, double utilization, const std::string &label)
{
    int utilization_width = (width * 0.75) - 2; // 75% for the progress bar with []
    int label_width = width * 0.25 - 1;         // 25% for the label with one space

    // Draw the label (left-aligned, ellipsis if necessary)
    std::string truncated_label = fit_label(label, label_width, justify_right);

    printw("%-.*s", label_width, truncated_label.c_str());

    // Draw the progress bar (75% of the terminal width)
    printw(" [");
    int pos = utilization_width * utilization / 100;
    for (int i = 0; i < utilization_width; ++i)
    {
        if (i == 1)
        {
            printw(" %3d%% ", (uint32_t)utilization);
        }
        else if (i > 1 && i <= 6)
        {
            /* do nothing in % label */
        }
        else if (i < pos)
        {
            printw("#");
        }
        else
        {
            printw(" ");
        }
    }
    printw("]");
}

void show_device_memory(Device *device)
{
    zes_mem_state_t memState = device->getMemoryState();
    printf(" Memory: %lu\n", memState.size);
}

void show_device_properties(const Device *device)
{
    const char *type = nullptr;

    const zes_device_properties_t *properties = device->getDeviceProperties();
    const zes_device_ext_properties_t *ext_properties = device->getDeviceExtProperties();
    const zes_pci_properties_t *pci_properties = device->getDevicePciProperties();

    std::string uuid_str = uuid_to_string(&ext_properties->uuid);
    std::ostringstream output;
    printf("Device: %04X:%04X (%s)\n",
           device->getDeviceProperties()->core.vendorId,
           device->getDeviceProperties()->core.deviceId,
           device->getDeviceProperties()->modelName);

    output << " UUID: " << uuid_str << std::endl;

    output << " BDF: "
           << std::hex << std::uppercase
           << std::setw(4) << std::setfill('0') << pci_properties->address.domain << ":"
           << std::setw(4) << std::setfill('0') << pci_properties->address.bus << ":"
           << std::setw(4) << std::setfill('0') << pci_properties->address.device << ":"
           << std::setw(4) << std::setfill('0') << pci_properties->address.function << std::endl;

    output << " PCI ID: "
           << std::setw(4) << std::setfill('0') << properties->core.vendorId << ":"
           << std::setw(4) << std::setfill('0') << properties->core.deviceId << std::endl;

    output << " Subdevices: " << properties->numSubdevices << std::endl;
    output << " Serial Number: " << properties->serialNumber << std::endl;
    output << " Board Number: " << properties->boardNumber << std::endl;
    output << " Brand Name: " << properties->brandName << std::endl;
    output << " Model Name: " << properties->modelName << std::endl;
    output << " Vendor Name: " << properties->vendorName << std::endl;
    output << " Driver Version: " << properties->driverVersion << std::endl;

    switch (ext_properties->type)
    {
    case ZES_DEVICE_TYPE_GPU:
        type = "GPU";
        break;
    case ZES_DEVICE_TYPE_CPU:
        type = "CPU";
        break;
    case ZES_DEVICE_TYPE_FPGA:
        type = "FPGA";
        break;
    case ZES_DEVICE_TYPE_MCA:
        type = "MCA";
        break;
    case ZES_DEVICE_TYPE_VPU:
        type = "VPU";
        break;
    default:
        type = "UNKNOWN";
        break;
    }

    output << " Type: " << type << std::endl;

    output << " Is integrated with host: " << ((ext_properties->flags & ZES_DEVICE_PROPERTY_FLAG_INTEGRATED) ? "Yes" : "No") << std::endl;
    output << " Is a sub-device: " << ((ext_properties->flags & ZES_DEVICE_PROPERTY_FLAG_SUBDEVICE) ? "Yes" : "No") << std::endl;
    output << " Supports error correcting memory: " << ((ext_properties->flags & ZES_DEVICE_PROPERTY_FLAG_ECC) ? "Yes" : "No") << std::endl;
    output << " Supports on-demand page-faulting: " << ((ext_properties->flags & ZES_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) ? "Yes" : "No") << std::endl;

    printf("%s", output.str().c_str());
}

void show_engine_group_properties(const Engine *engine)
{
    printf("   %s", engine_type_to_str(engine->getEngineProperties()->type));
    if (engine->getEngineProperties()->onSubdevice)
    {
        printf(" (Sub-device ID: %04X)", engine->getEngineProperties()->subdeviceId);
    }
    printf("\n");
}

void show_engine_groups(const Device *device)
{
    uint32_t enginesCount = device->getEngineCount();
    std::ostringstream output;

    printf(" Engines: %d\n", enginesCount);
    if (enginesCount == 0)
    {
        return;
    }

    for (uint32_t i = 0; i < enginesCount; ++i)
    {
        const Engine *engine = device->getEngine(i);
        printf("  Engine %d: %s", i + 1, engine_type_to_str(engine->getEngineProperties()->type));
        if (engine->getEngineProperties()->onSubdevice)
        {
            printf(" (Sub-device ID: %04X)", engine->getEngineProperties()->subdeviceId);
        }
        printf("\n");
    }
}

void show_power_domains(const Device *device)
{
    uint32_t powerDomainsCount = device->getPowerDomainCount();
    std::ostringstream output;

    printf(" Power Domains: %d\n", powerDomainsCount);
    if (powerDomainsCount == 0)
    {
        return;
    }

    for (uint32_t i = 0; i < powerDomainsCount; ++i)
    {
        const PowerDomain *powerDomain = device->getPowerDomain(i);
        const zes_power_properties_t *properties = powerDomain->getPowerDomainProperties();
        if (properties->onSubdevice)
        {
            printf("  Power Domain %d: Sub Device: %04X, Can Control: %s, Threshold supported: %s\n",
                   i + 1,
                   properties->subdeviceId,
                   properties->canControl ? "Yes" : "No", properties->isEnergyThresholdSupported ? "Yes" : "No");
        }
        else
        {
            printf("  Power Domain %d: Can Control: %s, Threshold supported: %s\n",
                   i + 1, properties->canControl ? "Yes" : "No", properties->isEnergyThresholdSupported ? "Yes" : "No");
        }
    }
}

void show_psus(const Device *device)
{
    uint32_t psuCount = device->getPSUCount();
    std::ostringstream output;

    printf(" Power Supply Units: %d\n", psuCount);
    if (psuCount == 0)
    {
        return;
    }

    for (uint32_t i = 0; i < psuCount; ++i)
    {
        const PSU *psu = device->getPSU(i);
        const zes_psu_properties_t *properties = psu->getPSUProperties();
        if (properties->onSubdevice)
        {
            printf("  Power Supply Unit %d: Sub Device: %04X, Fan: %s, Amp Limit: %d\n",
                   i + 1,
                   properties->subdeviceId,
                   properties->haveFan ? "Yes" : "No", properties->ampLimit);
        }
        else
        {
            printf("  Power Supply Unit %d: Fan: %s, Amp Limit: %d\n",
                   i + 1, properties->haveFan ? "Yes" : "No", properties->ampLimit);
        }
        printf("\n");
    }
}

ze_result_t list_devices(std::vector<std::unique_ptr<Device>> &devices)
{
    // Walk through each device and display properties
    for (uint32_t j = 0; j < devices.size(); ++j)
    {
        const Device *device = devices[j].get();

        printf("Device %d: %04X:%04X (%s)\n",
               j + 1,
               device->getDeviceProperties()->core.vendorId,
               device->getDeviceProperties()->core.deviceId,
               device->getDeviceProperties()->modelName);
    }

    return ZE_RESULT_SUCCESS;
}

std::vector<std::unique_ptr<Device>> get_devices()
{
    std::vector<std::unique_ptr<Device>> devices;

    // Discover all the drivers
    uint32_t driversCount = 0;
    zesDriverGet(&driversCount, nullptr);

    if (driversCount == 0)
    {
        fprintf(stderr, "No ze sysman drivers found.\n");
        return devices;
    }

    std::unique_ptr<zes_driver_handle_t[]> drivers = std::make_unique<zes_driver_handle_t[]>(driversCount);
    if (!drivers)
    {
        fprintf(stderr, "Memory allocation failed for drivers\n");
        return devices;
    }

    zesDriverGet(&driversCount, drivers.get());

    for (uint32_t driver = 0; driver < driversCount; ++driver)
    {
        // Discover devices in a driver
        uint32_t deviceCount = 0;
        zesDeviceGet(drivers[driver], &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            printf("Driver %i:\n  No devices found\n", driver);
            continue;
        }

        std::unique_ptr<zes_device_handle_t[]> deviceHandles = std::make_unique<zes_device_handle_t[]>(deviceCount);
        if (!deviceHandles)
        {
            printf("Memory allocation failed for devices\n");
            return devices;
        }

        zesDeviceGet(drivers[driver], &deviceCount, deviceHandles.get());

        // Walk through each device and get properties
        for (uint32_t device = 0; device < deviceCount; ++device)
        {
            devices.emplace_back(std::make_unique<Device>(deviceHandles[device]));
        }
    }

    return devices;
}

int32_t find_device_index(arg_search_t search, std::vector<std::unique_ptr<Device>> &devices)
{
    pciid_t pciid;
    zes_uuid_t uuid;
    bdf_t bdf;

    for (uint32_t i = 0; i < devices.size(); ++i)
    {
        const Device *device = devices[i].get();
        bool match = false;
        switch (search.type)
        {
        case INDEX:
            match = i + 1 == std::get<uint32_t>(search.data);
            break;
        case PCIID:
            pciid = std::get<pciid_t>(search.data);
            match = (pciid.vendor == device->getDeviceProperties()->core.vendorId &&
                     pciid.device == device->getDeviceProperties()->core.deviceId);
            break;
        case BDF:
            bdf = std::get<bdf_t>(search.data);
            match = (bdf.domain == device->getDevicePciProperties()->address.domain &&
                     bdf.bus == device->getDevicePciProperties()->address.bus &&
                     bdf.device == device->getDevicePciProperties()->address.device &&
                     bdf.function == device->getDevicePciProperties()->address.function);
            break;
        case UUID:
            uuid = std::get<zes_uuid_t>(search.data);
            uint32_t k;
            for (k = 0; k < ZE_MAX_DEVICE_UUID_SIZE; ++k)
            {
                if (uuid.id[k] != device->getDeviceExtProperties()->uuid.id[k])
                {
                    break;
                }
            }
            match = k == ZE_MAX_DEVICE_UUID_SIZE;
            break;
        default:
            match = false;
            break;
        }

        if (match)
        {
            return i;
        }
    }

    return -1;
}

uint32_t show_process_info(uint32_t row, uint32_t width, const ProcessInfo *processInfo)
{
    const zes_process_state_t *state = processInfo->getProcessState();
    const uint32_t pid_size = 6;
    wprintw(stdscr, "%*d %-*s",
            pid_size,
            state->processId,
            width - pid_size,
            fit_label(processInfo->getCommandLine(), width - pid_size, justify_middle).c_str());
    move(++row, 0);
    wprintw(stdscr, "%-*s MEM: %-12lu SHR: %-12lu FLAGS: %s",
            pid_size,
            "",
            state->memSize, state->sharedSize,
            engine_flags_to_str(state->engines).c_str());
    move(++row, 0);
    return row;
}

class XeNcurses
{
public:
    XeNcurses()
    {
        // Initialize ncurses
        if (initscr() == NULL)
        {
            throw std::runtime_error("Error initializing ncurses.");
        }
        cbreak();
        noecho();
        curs_set(0);
        keypad(stdscr, TRUE);
    }

    ~XeNcurses()
    {
        // Save screen content before exiting
        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        std::vector<std::string> screen_content(rows);
        for (int i = 0; i < rows; ++i)
        {
            std::vector<char> line(cols + 1, '\0');
            move(i, 0);
            innstr(line.data(), cols); // Capture line from ncurses screen
            screen_content[i] = std::string(line.data());
        }

        // Restore terminal state
        endwin();

        // Restore screen contents
        for (const auto &line : screen_content)
        {
            std::cout << line << std::endl;
        }
    }

    void run(Device *device, uint32_t interval, bool oneShot)
    {
        std::ostringstream output;

        timeout(interval); // Non-blocking input

        /* Enter loop to gather rum-time statistics */
        while (true)
        {
            uint32_t height, width;

            getmaxyx(stdscr, height, width);

            try
            {
                uint32_t headings = 0;
                move(0, 0);
                wprintw(stdscr, "Device: %04X:%04X (%s)",
                        device->getDeviceProperties()->core.vendorId,
                        device->getDeviceProperties()->core.deviceId,
                        device->getDeviceProperties()->modelName);
                move(++headings, 0);

                zes_mem_state_t memState = device->getMemoryState();

                uint32_t row = headings;

                wprintw(stdscr, "Total Memory: %12lu",
                        memState.size);
                move(++row, 0);

                if (memState.size != 0)
                {
                    double utilization = 100 * memState.free / memState.size;
                    draw_utilization_bar(width, utilization, "Free memory: ");
                    move(++row, 0);
                }

                for (uint32_t i = 0; i < device->getEngineCount(); ++i)
                {
                    Engine *engine = device->getEngine(i);
                    const zes_engine_properties_t *engineProperties = engine->getEngineProperties();
                    if (engineProperties != nullptr)
                    {
                        double utilization = engine->getEngineUtilization();
                        draw_utilization_bar(width, utilization, engine_type_to_str(engine->getEngineProperties()->type));
                        move(++row, 0);
                    }
                }

                for (uint32_t i = 0; i < device->getTemperatureCount(); ++i)
                {
                    wprintw(stdscr, "Sensor %d: %.1fC", i, device->getTemperature(i));
                    move(++row, 0);
                }

                double total_energy = 0;
                for (uint32_t i = 0; i < device->getPowerDomainCount(); ++i)
                {
                    PowerDomain *powerDomain = device->getPowerDomain(i);
                    double energy = powerDomain->getPowerDomainEnergy();
                    total_energy += energy;
                }
                if (total_energy != 0)
                {
                    wprintw(stdscr, "Power usage: %.01fW", total_energy);
                    move(++row, 0);
                }

                for (uint32_t i = 0; i < device->getPSUCount(); ++i)
                {
                    const PSU *psu = device->getPSU(i);
                    const zes_psu_state_t *state = psu->getPSUState();
                    wprintw(stdscr, "Power Supply Unit %d: %s", i, voltage_status_to_str(state->voltStatus));
                    move(++row, 0);
                }

                ze_result_t ret = device->updateProcesses();
                if (ret == ZE_RESULT_SUCCESS && device->getProcessCount() > 0)
                {
                    uint32_t pid_size = 6;
                    //
                    // Header begins
                    for (uint32_t i = 0; i < width; i++)
                    {
                        waddch(stdscr, '-');
                    }
                    move(++row, 0);
                    wprintw(stdscr, "%*s COMMAND-LINE", pid_size, "PID");
                    move(++row, 0);
                    wprintw(stdscr, "%-*s %-17s %-17s %s",
                            pid_size, "", "USED MEMORY", "SHARED MEMORY", "ENGINE FLAGS");
                    move(++row, 0);
                    for (uint32_t i = 0; i < width; i++)
                    {
                        waddch(stdscr, '-');
                    }
                    move(++row, 0);
                    // Header ends
                    //
                    for (uint32_t i = 0; i < device->getProcessCount(); ++i)
                    {
                        row = show_process_info(row, width, device->getProcessInfo(i));
                    }
                }

                while (row < height)
                {
                    clrtoeol();
                    move(++row, 0);
                }

                refresh(); // Refresh the screen to show the message

                if (oneShot)
                {
                    return;
                }

                switch (getch())
                {
                case ERR:
                    break;
                case KEY_UP:
                    printw("Up arrow key pressed\n");
                    break;
                case 'q':
                    wprintw(stdscr, "Exiting.");
                    return;
                default:
                    break;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error: " << e.what() << "\n";
            }
        }
    }
};

void show_temperatures(Device *device)
{
    device->updateTemperatures();

    for (uint32_t i = 0; i < device->getTemperatureCount(); ++i)
    {
        printf("  Sensor %d: %.1fC", i, device->getTemperature(i));
    }
}

void copyright()
{
    printf("ze-monitor: A small Level Zero Sysman GPU monitor utility\n");
    printf("Copyright (C) 2025 James Ketrenos\n");
}

#ifndef APP_VERSION
#define APP_VERSION "unknown"
#endif
void version()
{
    printf("Version: %s\n", APP_VERSION);
}

void usage()
{
    const uint32_t indent = 2;
    const uint32_t option_len = 12;
    const char *options[][2] = {
        {"device ID", "Device ID to query. Can accept #, BDF, PCI-ID, /dev/dri/*."},
        {"help", "This text."},
        {"info", "Show additional details about device."},
        {"interval ms", "Time interval for polling. Default 1000."},
        {"one-shot", "Gather statistics, output, then exit."},
        {"version", "Version info."},
        {nullptr, nullptr}};
    printf("\n");
    printf("usage: ze-monitor [OPTIONS]\n");
    printf("\n");

    for (uint32_t i = 0; options[i][0] != nullptr; ++i)
    {
        printf("%*s --%-*s %s\n", indent, "", option_len, options[i][0], options[i][1]);
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    bool showInfo = false;
    bool listDevices = true;
    bool oneShot = false;
    arg_search_t argSearch;
    uint32_t interval = 1000;

    // Process command-line arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        // Look for --device argument
        if (arg == "--device" && i + 1 < argc)
        {
            std::string device_arg = argv[i + 1];
            argSearch = process_device_argument(device_arg);
            if (argSearch.type == INVALID)
            {
                std::cerr << "Invalid argument: " << arg << std::endl;
                return -1;
            }
            i++; // Skip the device argument
            listDevices = false;
        }
        else if (arg == "--interval" && i + 1 < argc)
        {
            interval = strtoul(argv[i + 1], nullptr, 10);
            i++; // Skip the device argument
        }
        else if (arg == "--info")
        {
            showInfo = true;
        }
        else if (arg == "--one-shot")
        {
            oneShot = true;
        }
        else if (arg == "--list")
        {
            listDevices = true;
        }
        else if (arg == "--version")
        {
            copyright();
            version();
            return 0;
        }
        else if (arg == "--help")
        {
            copyright();
            usage();
            return 0;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return 1;
        }
    }

    if (zesInit(0) != ZE_RESULT_SUCCESS)
    {
        printf("Can't initialize the API\n");
        return -1;
    }

    std::vector<std::unique_ptr<Device>> devices = get_devices();

    Device *device = nullptr;
    int32_t index = -1;
    if (argSearch.type != INVALID)
    {
        index = find_device_index(argSearch, devices);
        if (index != -1)
        {
            device = devices[index].get();
        }
        else
        {
            printf("--device %s not found.\n", argSearch.match.c_str());
            list_devices(devices);
            exit(-1);
        }
    }

    if (device == nullptr && !showInfo)
    {
        listDevices = true;
    }

    if (listDevices)
    {
        list_devices(devices);
        return 0;
    }

    // If --info was requested, either show the single --device or if no device
    // was provided, list all devices.
    if (showInfo)
    {
        if (device != nullptr)
        {
            show_device_properties(device);
            show_device_memory(device);
            show_engine_groups(device);
            show_temperatures(device);
            show_power_domains(device);
            show_psus(device);
        }
        else
        {
            for (uint32_t i = 0; i < devices.size(); ++i)
            {
                device = devices[i].get();

                printf("Device %d: %04X:%04X (%s)\n",
                       i + 1,
                       device->getDeviceProperties()->core.vendorId,
                       device->getDeviceProperties()->core.deviceId,
                       device->getDeviceProperties()->modelName);

                show_device_properties(device);
                show_device_memory(device);
                show_engine_groups(device);
                show_temperatures(device);
                show_power_domains(device);
                show_psus(device);
            }
        }
        return 0;
    }

    // WIP
    // Playing around with ncurses a bit more...
    //    ze_ncurses_main(device, interval, oneShot);
    //    exit(0);
    try
    {
        XeNcurses app;
        app.run(device, interval, oneShot);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}

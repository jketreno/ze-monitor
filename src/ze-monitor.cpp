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
#include "process.h"            // for ze_error_to_str, engine_type_to_str
#include "temperature.h"        // for ze_error_to_str, engine_type_to_str
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
            printf("  Power Domain %d: Can Control: %s, Threshold supported: %s\n",
                   i + 1, properties->canControl ? "Yes" : "No", properties->isEnergyThresholdSupported ? "Yes" : "No");
        }
        else
        {
            printf("  Power Domain %d: Sub Device: %04X, Can Control: %s, Threshold supported: %s\n",
                   properties->subdeviceId,
                   i + 1, properties->canControl ? "Yes" : "No", properties->isEnergyThresholdSupported ? "Yes" : "No");
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
        // Cleanup ncurses
        nocbreak(); // Don't clear the screen
        noecho();
        endwin();
    }

    void run(uint32_t interval, const Device *device)
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
                TemperatureMonitor tempMonitor(device->getHandle());

                uint32_t headings = 0;
                move(0, 0);
                wprintw(stdscr, "Device: %04X:%04X (%s)",
                        device->getDeviceProperties()->core.vendorId,
                        device->getDeviceProperties()->core.deviceId,
                        device->getDeviceProperties()->modelName);
                move(++headings, 0);
                wprintw(stdscr, "Engines: %d", device->getEngineCount());
                move(++headings, 0);
                wprintw(stdscr, "Temperature Sensors: %d", tempMonitor.getSensorCount());
                move(++headings, 0);
                wprintw(stdscr, "Power Domains: %d", device->getPowerDomainCount());
                move(++headings, 0);

                uint32_t row = headings;

                ProcessMonitor processMonitor(device->getHandle());

                tempMonitor.updateTemperatures();

                wprintw(stdscr, "Processes: %d", processMonitor.getProcessCount());
                move(++row, 0);

                for (uint32_t i = 0; i < device->getEngineCount(); ++i)
                {
                    const Engine *engine = device->getEngine(i);
                    const zes_engine_properties_t *engineProperties = engine->getEngineProperties();
                    if (engineProperties != nullptr)
                    {
                        double utilization = engine->getEngineUtilization();
                        draw_utilization_bar(width, utilization, engine_type_to_str(engine->getEngineProperties()->type));
                        move(++row, 0);
                    }
                }

                for (uint32_t i = 0; i < tempMonitor.getSensorCount(); ++i)
                {
                    tempMonitor.displayTemperatures(i);
                    move(++row, 0);
                }

                for (uint32_t i = 0; i < device->getPowerDomainCount(); ++i)
                {
                    const PowerDomain *powerDomain = device->getPowerDomain(i);
                    double energy = powerDomain->getPowerDomainEnergy();
                    wprintw(stdscr, "Power Domain %d: %.02fW", i, energy);
                    move(++row, 0);
                }

                for (uint32_t i = 0; i < processMonitor.getProcessCount(); ++i)
                {
                    processMonitor.displayProcesses(i);
                    move(++row, 0);
                }

                while (row < height)
                {
                    clrtoeol();
                    move(++row, 0);
                }

                refresh(); // Refresh the screen to show the message

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

void show_temperatures(const Device *device)
{
    TemperatureMonitor tempMonitor(device->getHandle());
    printf(" Temperature Sensors: %d\n", tempMonitor.getSensorCount());
    tempMonitor.updateTemperatures();

    for (uint32_t i = 0; i < tempMonitor.getSensorCount(); ++i)
    {
        printf("  Sensor %d: %.1fC\n", i + 1, tempMonitor.getTemperature(i));
    }
}

int main(int argc, char *argv[])
{
    bool showInfo = false;
    arg_search_t argSearch;

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
        }
        else if (arg == "--info")
        {
            showInfo = true;
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

    const Device *device = nullptr;
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
            printf("BDF --device %s not found.\n", argSearch.match.c_str());
            list_devices(devices);
            exit(-1);
        }
    }

    if (device == nullptr)
    {
        if (!showInfo)
        {
            list_devices(devices);
            exit(0);
        }
    }

    if (showInfo)
    {
        if (device != nullptr)
        {
            show_device_properties(device);
            show_engine_groups(device);
            show_temperatures(device);
            show_power_domains(device);
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
                show_engine_groups(device);
                show_temperatures(device);
                show_power_domains(device);
            }
        }
        return 0;
    }

    try
    {
        XeNcurses app;
        app.run(1000, device);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}

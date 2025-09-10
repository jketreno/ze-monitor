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
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/component/component.hpp>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
using namespace ftxui;

// Helper to format bytes
std::string format_bytes(uint64_t bytes) {
    const char* suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double count = bytes;
    while (count >= 1024 && i < 4) {
        count /= 1024;
        ++i;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f %s", count, suffixes[i]);
    return buf;
}

// Helper to get color based on percentage
Color get_percentage_color(double percentage) {
    if (percentage < 30) return Color::Green;
    if (percentage < 60) return Color::Yellow;
    if (percentage < 80) return Color::RGB(255, 215, 0);
    return Color::Red;
}

// Helper to get temperature color
Color get_temp_color(double temp) {
    if (temp < 50) return Color::Cyan;
    if (temp < 70) return Color::Green;
    if (temp < 80) return Color::Yellow;
    if (temp < 90) return Color::RGB(255, 215, 0);
    return Color::Red;
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
            listDevices = false;
        }
        else if (arg == "--info")
        {
            showInfo = true;
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

    // FTXUI main UI loop
    auto screen = ScreenInteractive::Fullscreen();

    enum class ViewMode {
        OVERVIEW,
        ENGINES,
        PROCESSES,
        POWER,
        THERMAL
    };

    struct UIState {
        ViewMode view_mode = ViewMode::OVERVIEW;
        int process_offset = 0;
        int engine_offset = 0;
        int thermal_offset = 0;
        int power_offset = 0;
        bool show_help = false;
        std::chrono::steady_clock::time_point refresh_time = std::chrono::steady_clock::now();
    };

    UIState state;

    // Auto-refresh timer
    auto refresh_timer = [&] {
        auto now = std::chrono::steady_clock::now();
        if (now - state.refresh_time > std::chrono::seconds(1)) {
            state.refresh_time = now;
            return true;
        }
        return false;
    };

    auto component = Renderer([&]() -> Element {
        // Auto refresh every second
        if (refresh_timer()) {
            device->updateTemperatures();
            device->updateProcesses();
        }

        Elements main_content;

        // Header with device info
        auto mem = device->getMemoryState();
        double mem_usage_pct = 0.0;
        if (mem.size > 0) {
            mem_usage_pct = (1.0 - (double)mem.free / mem.size) * 100;
        }
        auto avg_temp = 0.0;
        if (device->getTemperatureCount() > 0) {
            for (uint32_t i = 0; i < device->getTemperatureCount(); ++i) {
                avg_temp += device->getTemperature(i);
            }
            avg_temp /= device->getTemperatureCount();
        }

        auto header = vbox({
            hbox({
                text("🚀 ") | color(Color::Cyan),
                text("ZE-MONITOR") | bold | color(Color::White),
                text(" | ") | color(Color::GrayDark),
                text(device->getDeviceProperties()->modelName) | color(Color::Cyan),
                text(" | ") | color(Color::GrayDark),
                text("View: ") | color(Color::GrayDark),
                text([&]() {
                    switch (state.view_mode) {
                        case ViewMode::OVERVIEW: return "Overview";
                        case ViewMode::ENGINES: return "Engines";
                        case ViewMode::PROCESSES: return "Processes";
                        case ViewMode::POWER: return "Power";
                        case ViewMode::THERMAL: return "Thermal";
                    }
                    return "Unknown";
                }()) | bold | color(Color::Yellow)
            }),
            hbox({
                text("Memory: ") | color(Color::White),
                mem.size > 0 ? gauge(mem_usage_pct / 100.0) | size(WIDTH, EQUAL, 20) | color(get_percentage_color(mem_usage_pct)) : text("N/A") | size(WIDTH, EQUAL, 20),
                mem.size > 0 ? text(" " + std::to_string((int)mem_usage_pct) + "% ") | color(get_percentage_color(mem_usage_pct)) : text(""),
                mem.size > 0 ? text("(" + format_bytes(mem.size - mem.free) + "/" + format_bytes(mem.size) + ")") | color(Color::GrayDark) : text(""),
                separator(),
                text(" Temp: ") | color(Color::White),
                text(std::to_string((int)avg_temp) + "°C") | color(get_temp_color(avg_temp))
            })
        }) | border | color(Color::Cyan);

        main_content.push_back(header);

        // Content based on view mode
        switch (state.view_mode) {
            case ViewMode::OVERVIEW: {
                // Engine utilization overview
                Elements engine_rows;
                engine_rows.push_back(
                    hbox({
                        text("ENGINE") | bold | size(WIDTH, EQUAL, 20),
                        text("UTILIZATION") | bold | size(WIDTH, EQUAL, 20),
                        text("SUB-DEV") | bold | size(WIDTH, EQUAL, 10)
                    }) | color(Color::White)
                );
                
                for (uint32_t i = 0; i < device->getEngineCount(); ++i) {
                    auto engine = device->getEngine(i);
                    double util = engine->getEngineUtilization();
                    auto subdev = engine->getEngineProperties()->onSubdevice ? 
                        std::to_string(engine->getEngineProperties()->subdeviceId) : "N/A";
                    
                    engine_rows.push_back(
                        hbox({
                            text(engine_type_to_str(engine->getEngineProperties()->type)) | 
                                size(WIDTH, EQUAL, 20) | color(Color::Cyan),
                            hbox({
                                gauge(util / 100.0) | size(WIDTH, EQUAL, 15) | color(get_percentage_color(util)),
                                text(" " + std::to_string((int)util) + "%") | color(get_percentage_color(util))
                            }) | size(WIDTH, EQUAL, 20),
                            text(subdev) | size(WIDTH, EQUAL, 10) | color(Color::GrayDark)
                        })
                    );
                }
                
                main_content.push_back(
                    vbox({
                        text("🔧 Engine Utilization") | bold | color(Color::Green),
                        vbox(std::move(engine_rows)) | size(WIDTH, EQUAL, 12)
                    }) | border
                );

                // Top processes
                Elements proc_rows;
                proc_rows.push_back(
                    hbox({
                        text("PID") | bold | size(WIDTH, EQUAL, 8),
                        text("COMMAND") | bold | size(WIDTH, EQUAL, 25),
                        text("MEMORY") | bold | size(WIDTH, EQUAL, 12),
                        text("SHARED") | bold | size(WIDTH, EQUAL, 12)
                    }) | color(Color::White)
                );
                
                int proc_limit = std::min(8, (int)device->getProcessCount());
                for (int i = 0; i < proc_limit; ++i) {
                    auto proc = device->getProcessInfo(i);
                    auto mem_pct = mem.size > 0 ? (double)proc->getProcessState()->memSize / mem.size * 100 : 0.0;
                    
                    proc_rows.push_back(
                        hbox({
                            text(std::to_string(proc->getProcessState()->processId)) | 
                                size(WIDTH, EQUAL, 8) | color(Color::Yellow),
                            text(proc->getCommandLine().substr(0, 20) + (proc->getCommandLine().length() > 20 ? "..." : "")) | 
                                size(WIDTH, EQUAL, 25) | color(Color::White),
                            text(format_bytes(proc->getProcessState()->memSize)) | 
                                size(WIDTH, EQUAL, 12) | color(get_percentage_color(mem_pct)),
                            text(format_bytes(proc->getProcessState()->sharedSize)) | 
                                size(WIDTH, EQUAL, 12) | color(Color::GrayDark)
                        })
                    );
                }
                
                main_content.push_back(
                    vbox({
                        text("📊 Top Processes") | bold | color(Color::Green),
                        vbox(std::move(proc_rows)) | size(WIDTH, EQUAL, 12)
                    }) | border
                );
                break;
            }

            case ViewMode::ENGINES: {
                Elements engine_detail;
                engine_detail.push_back(
                    hbox({
                        text("ENGINE") | bold | size(WIDTH, EQUAL, 25),
                        text("UTILIZATION") | bold | size(WIDTH, EQUAL, 25),
                        text("SUB-DEVICE") | bold | size(WIDTH, EQUAL, 15),
                        text("STATUS") | bold | size(WIDTH, EQUAL, 15)
                    }) | color(Color::White)
                );

                int visible_engines = 15;
                int start = state.engine_offset;
                int end = std::min(start + visible_engines, (int)device->getEngineCount());
                
                for (int i = start; i < end; ++i) {
                    auto engine = device->getEngine(i);
                    double util = engine->getEngineUtilization();
                    auto status = util > 0 ? "ACTIVE" : "IDLE";
                    auto status_color = util > 0 ? Color::Green : Color::GrayDark;
                    
                    engine_detail.push_back(
                        hbox({
                            text(engine_type_to_str(engine->getEngineProperties()->type)) | 
                                size(WIDTH, EQUAL, 25) | color(Color::Cyan),
                            hbox({
                                gauge(util / 100.0) | size(WIDTH, EQUAL, 18) | color(get_percentage_color(util)),
                                text(" " + std::to_string((int)util) + "%") | color(get_percentage_color(util))
                            }) | size(WIDTH, EQUAL, 25),
                            text(engine->getEngineProperties()->onSubdevice ? 
                                std::to_string(engine->getEngineProperties()->subdeviceId) : "N/A") | 
                                size(WIDTH, EQUAL, 15) | color(Color::GrayDark),
                            text(status) | size(WIDTH, EQUAL, 15) | color(status_color)
                        })
                    );
                }
                
                main_content.push_back(
                    vbox({
                        text("🔧 Engine Details") | bold | color(Color::Green),
                        vbox(std::move(engine_detail)) | size(WIDTH, EQUAL, 18)
                    }) | border
                );
                break;
            }

            case ViewMode::PROCESSES: {
                Elements process_detail;
                process_detail.push_back(
                    hbox({
                        text("PID") | bold | size(WIDTH, EQUAL, 8),
                        text("COMMAND") | bold | size(WIDTH, EQUAL, 30),
                        text("MEMORY") | bold | size(WIDTH, EQUAL, 12),
                        text("SHARED") | bold | size(WIDTH, EQUAL, 12),
                        text("ENGINES") | bold | size(WIDTH, EQUAL, 15)
                    }) | color(Color::White)
                );

                int visible_processes = 15;
                int start = state.process_offset;
                int end = std::min(start + visible_processes, (int)device->getProcessCount());
                
                for (int i = start; i < end; ++i) {
                    auto proc = device->getProcessInfo(i);
                    auto mem_pct = mem.size > 0 ? (double)proc->getProcessState()->memSize / mem.size * 100 : 0.0;
                    
                    process_detail.push_back(
                        hbox({
                            text(std::to_string(proc->getProcessState()->processId)) | 
                                size(WIDTH, EQUAL, 8) | color(Color::Yellow),
                            text(proc->getCommandLine().length() > 25 ? 
                                proc->getCommandLine().substr(0, 25) + "..." : proc->getCommandLine()) | 
                                size(WIDTH, EQUAL, 30) | color(Color::White),
                            text(format_bytes(proc->getProcessState()->memSize)) | 
                                size(WIDTH, EQUAL, 12) | color(get_percentage_color(mem_pct)),
                            text(format_bytes(proc->getProcessState()->sharedSize)) | 
                                size(WIDTH, EQUAL, 12) | color(Color::GrayDark),
                            text(engine_flags_to_str(proc->getProcessState()->engines)) | 
                                size(WIDTH, EQUAL, 15) | color(Color::Cyan)
                        })
                    );
                }
                
                main_content.push_back(
                    vbox({
                        text("📊 Process Details") | bold | color(Color::Green),
                        vbox(std::move(process_detail)) | size(WIDTH, EQUAL, 18)
                    }) | border
                );
                break;
            }

            case ViewMode::THERMAL: {
                Elements thermal_detail;
                thermal_detail.push_back(
                    hbox({
                        text("SENSOR") | bold | size(WIDTH, EQUAL, 15),
                        text("TEMPERATURE") | bold | size(WIDTH, EQUAL, 20),
                        text("STATUS") | bold | size(WIDTH, EQUAL, 15),
                        text("GRAPH") | bold | size(WIDTH, EQUAL, 30)
                    }) | color(Color::White)
                );

                for (uint32_t i = 0; i < device->getTemperatureCount(); ++i) {
                    auto temp = device->getTemperature(i);
                    auto status = temp < 80 ? "NORMAL" : temp < 90 ? "WARM" : "HOT";
                    auto status_color = temp < 80 ? Color::Green : temp < 90 ? Color::Yellow : Color::Red;
                    
                    // Simple temperature bar (0-100°C scale)
                    double temp_ratio = std::min(temp / 100.0, 1.0);
                    
                    thermal_detail.push_back(
                        hbox({
                            text("Sensor " + std::to_string(i + 1)) | 
                                size(WIDTH, EQUAL, 15) | color(Color::Cyan),
                            text(std::to_string((int)temp) + "°C") | 
                                size(WIDTH, EQUAL, 20) | color(get_temp_color(temp)),
                            text(status) | size(WIDTH, EQUAL, 15) | color(status_color),
                            gauge(temp_ratio) | size(WIDTH, EQUAL, 25) | color(get_temp_color(temp))
                        })
                    );
                }
                
                main_content.push_back(
                    vbox({
                        text("🌡️  Thermal Monitoring") | bold | color(Color::Green),
                        vbox(std::move(thermal_detail)) | size(WIDTH, EQUAL, 10)
                    }) | border
                );
                break;
            }

            case ViewMode::POWER: {
                Elements power_detail;
                power_detail.push_back(
                    hbox({
                        text("DOMAIN") | bold | size(WIDTH, EQUAL, 15),
                        text("POWER") | bold | size(WIDTH, EQUAL, 15),
                        text("ENERGY") | bold | size(WIDTH, EQUAL, 15),
                        text("CONTROL") | bold | size(WIDTH, EQUAL, 10),
                        text("SUB-DEV") | bold | size(WIDTH, EQUAL, 10)
                    }) | color(Color::White)
                );

                for (uint32_t i = 0; i < device->getPowerDomainCount(); ++i) {
                    auto power_domain = device->getPowerDomain(i);
                    auto properties = power_domain->getPowerDomainProperties();
                    auto energy = power_domain->getPowerDomainEnergy();
                    
                    power_detail.push_back(
                        hbox({
                            text("Domain " + std::to_string(i + 1)) | 
                                size(WIDTH, EQUAL, 15) | color(Color::Cyan),
                            text(std::to_string((int)energy) + "W") | 
                                size(WIDTH, EQUAL, 15) | color(Color::Yellow),
                            text("N/A") | 
                                size(WIDTH, EQUAL, 15) | color(Color::GrayDark),
                            text(properties->canControl ? "YES" : "NO") | 
                                size(WIDTH, EQUAL, 10) | color(properties->canControl ? Color::Green : Color::Red),
                            text(properties->onSubdevice ? std::to_string(properties->subdeviceId) : "N/A") | 
                                size(WIDTH, EQUAL, 10) | color(Color::GrayDark)
                        })
                    );
                }

                // PSU information
                if (device->getPSUCount() > 0) {
                    power_detail.push_back(separator());
                    power_detail.push_back(text("Power Supply Units:") | bold | color(Color::White));
                    
                    for (uint32_t i = 0; i < device->getPSUCount(); ++i) {
                        auto psu = device->getPSU(i);
                        auto properties = psu->getPSUProperties();
                        
                        power_detail.push_back(
                            hbox({
                                text("PSU " + std::to_string(i + 1)) | 
                                    size(WIDTH, EQUAL, 15) | color(Color::Cyan),
                                text("Fan: " + std::string(properties->haveFan ? "YES" : "NO")) | 
                                    size(WIDTH, EQUAL, 15) | color(properties->haveFan ? Color::Green : Color::Red),
                                text("Limit: " + std::to_string(properties->ampLimit) + "A") | 
                                    size(WIDTH, EQUAL, 15) | color(Color::Yellow),
                                text("") | size(WIDTH, EQUAL, 10),
                                text(properties->onSubdevice ? std::to_string(properties->subdeviceId) : "N/A") | 
                                    size(WIDTH, EQUAL, 10) | color(Color::GrayDark)
                            })
                        );
                    }
                }
                
                main_content.push_back(
                    vbox({
                        text("⚡ Power Management") | bold | color(Color::Green),
                        vbox(std::move(power_detail)) | size(WIDTH, EQUAL, 15)
                    }) | border
                );
                break;
            }
        }

        // Key hints
        Elements key_hints;
        if (state.show_help) {
            key_hints = {
                text("📋 Key Bindings:") | bold | color(Color::White),
                hbox({
                    text("1-5") | color(Color::Yellow), text(": Switch views  ") | color(Color::GrayDark),
                    text("↑↓") | color(Color::Yellow), text(": Scroll  ") | color(Color::GrayDark),
                    text("h") | color(Color::Yellow), text(": Toggle help  ") | color(Color::GrayDark),
                    text("q/ESC") | color(Color::Yellow), text(": Quit") | color(Color::GrayDark)
                }),
                text("Views: 1=Overview 2=Engines 3=Processes 4=Power 5=Thermal") | color(Color::GrayDark)
            };
        } else {
            key_hints = {
                hbox({
                    text("Views: ") | color(Color::GrayDark),
                    text("1") | color(Color::Yellow), text("=Overview ") | color(Color::GrayDark),
                    text("2") | color(Color::Yellow), text("=Engines ") | color(Color::GrayDark),
                    text("3") | color(Color::Yellow), text("=Processes ") | color(Color::GrayDark),
                    text("4") | color(Color::Yellow), text("=Power ") | color(Color::GrayDark),
                    text("5") | color(Color::Yellow), text("=Thermal ") | color(Color::GrayDark),
                    text("| ") | color(Color::GrayDark),
                    text("↑↓") | color(Color::Yellow), text("=Scroll ") | color(Color::GrayDark),
                    text("h") | color(Color::Yellow), text("=Help ") | color(Color::GrayDark),
                    text("q") | color(Color::Yellow), text("=Quit") | color(Color::GrayDark)
                })
            };
        }

        main_content.push_back(
            vbox(std::move(key_hints)) | border | color(Color::Blue)
        );

        return vbox(std::move(main_content));

    }) | CatchEvent([&](Event event) {
        // View switching
        if (event == Event::Character('1')) {
            state.view_mode = ViewMode::OVERVIEW;
            return true;
        } else if (event == Event::Character('2')) {
            state.view_mode = ViewMode::ENGINES;
            return true;
        } else if (event == Event::Character('3')) {
            state.view_mode = ViewMode::PROCESSES;
            return true;
        } else if (event == Event::Character('4')) {
            state.view_mode = ViewMode::POWER;
            return true;
        } else if (event == Event::Character('5')) {
            state.view_mode = ViewMode::THERMAL;
            return true;
        }
        
        // Scrolling
        else if (event == Event::ArrowUp) {
            switch (state.view_mode) {
                case ViewMode::PROCESSES:
                    state.process_offset = std::max(0, state.process_offset - 1);
                    break;
                case ViewMode::ENGINES:
                    state.engine_offset = std::max(0, state.engine_offset - 1);
                    break;
                case ViewMode::THERMAL:
                    state.thermal_offset = std::max(0, state.thermal_offset - 1);
                    break;
                case ViewMode::POWER:
                    state.power_offset = std::max(0, state.power_offset - 1);
                    break;
                default:
                    break;
            }
            return true;
        } else if (event == Event::ArrowDown) {
            switch (state.view_mode) {
                case ViewMode::PROCESSES: {
                    int max_offset = std::max(0, (int)device->getProcessCount() - 15);
                    state.process_offset = std::min(max_offset, state.process_offset + 1);
                    break;
                }
                case ViewMode::ENGINES: {
                    int max_offset = std::max(0, (int)device->getEngineCount() - 15);
                    state.engine_offset = std::min(max_offset, state.engine_offset + 1);
                    break;
                }
                case ViewMode::THERMAL: {
                    int max_offset = std::max(0, (int)device->getTemperatureCount() - 15);
                    state.thermal_offset = std::min(max_offset, state.thermal_offset + 1);
                    break;
                }
                case ViewMode::POWER: {
                    int max_offset = std::max(0, (int)device->getPowerDomainCount() - 15);
                    state.power_offset = std::min(max_offset, state.power_offset + 1);
                    break;
                }
                default:
                    break;
            }
            return true;
        }
        
        // Help toggle
        else if (event == Event::Character('h') || event == Event::Character('H')) {
            state.show_help = !state.show_help;
            return true;
        }
        
        // Quit
        else if (event == Event::Escape || event == Event::Character('q') || event == Event::Character('Q')) {
            screen.Exit();
            return true;
        }

        return false;
    });

    // Auto-refresh component
    std::thread refresh_thread([&]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            screen.PostEvent(Event::Custom);
        }
    });
    refresh_thread.detach();

    // Main event loop
    screen.Loop(component);

    return 0;
}
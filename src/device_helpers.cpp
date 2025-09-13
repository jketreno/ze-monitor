#include "device_helpers.h"
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <regex>
#include <iostream>
#include "helpers.h"

std::vector<std::unique_ptr<Device>> get_devices() {
    std::vector<std::unique_ptr<Device>> devices;
    uint32_t driversCount = 0;
    zesDriverGet(&driversCount, nullptr);
    if (driversCount == 0) {
        fprintf(stderr, "No ze sysman drivers found.\n");
        return devices;
    }
    std::unique_ptr<zes_driver_handle_t[]> drivers = std::make_unique<zes_driver_handle_t[]>(driversCount);
    if (!drivers) {
        fprintf(stderr, "Memory allocation failed for drivers\n");
        return devices;
    }
    zesDriverGet(&driversCount, drivers.get());
    for (uint32_t driver = 0; driver < driversCount; ++driver) {
        uint32_t deviceCount = 0;
        zesDeviceGet(drivers[driver], &deviceCount, nullptr);
        if (deviceCount == 0) {
            printf("Driver %i:\n  No devices found\n", driver);
            continue;
        }
        std::unique_ptr<zes_device_handle_t[]> deviceHandles = std::make_unique<zes_device_handle_t[]>(deviceCount);
        if (!deviceHandles) {
            printf("Memory allocation failed for devices\n");
            return devices;
        }
        zesDeviceGet(drivers[driver], &deviceCount, deviceHandles.get());
        for (uint32_t device = 0; device < deviceCount; ++device) {
            devices.emplace_back(std::make_unique<Device>(deviceHandles[device]));
        }
    }
    return devices;
}

ze_result_t list_devices(std::vector<std::unique_ptr<Device>> &devices) {
    for (uint32_t j = 0; j < devices.size(); ++j) {
        const Device *device = devices[j].get();
        printf("Device %d: %04X:%04X (%s)\n", j + 1,
               device->getDeviceProperties()->core.vendorId,
               device->getDeviceProperties()->core.deviceId,
               device->getDeviceProperties()->modelName);
    }
    return ZE_RESULT_SUCCESS;
}

int32_t find_device_index(arg_search_t search, std::vector<std::unique_ptr<Device>> &devices) {
    pciid_t pciid;
    zes_uuid_t uuid;
    bdf_t bdf;
    for (uint32_t i = 0; i < devices.size(); ++i) {
        const Device *device = devices[i].get();
        bool match = false;
        switch (search.type) {
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
            for (k = 0; k < ZE_MAX_DEVICE_UUID_SIZE; ++k) {
                if (uuid.id[k] != device->getDeviceExtProperties()->uuid.id[k]) {
                    break;
                }
            }
            match = k == ZE_MAX_DEVICE_UUID_SIZE;
            break;
        default:
            match = false;
            break;
        }
        if (match) {
            return i;
        }
    }
    return -1;
}

void show_device_memory(Device *device) {
    zes_mem_state_t memState = device->getMemoryState();
    printf(" Memory: %lu\n", memState.size);
}

void show_device_properties(const Device *device) {
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
    output << " BDF: " << std::hex << std::uppercase << std::setw(4)
           << std::setfill('0') << pci_properties->address.domain << ":"
           << std::setw(4) << std::setfill('0') << pci_properties->address.bus
           << ":" << std::setw(4) << std::setfill('0') << pci_properties->address.device << ":" << std::setw(4)
           << std::setfill('0') << pci_properties->address.function << std::endl;
    output << " PCI ID: " << std::setw(4) << std::setfill('0')
           << properties->core.vendorId << ":" << std::setw(4)
           << std::setfill('0') << properties->core.deviceId << std::endl;
    output << " Subdevices: " << properties->numSubdevices << std::endl;
    output << " Serial Number: " << properties->serialNumber << std::endl;
    output << " Board Number: " << properties->boardNumber << std::endl;
    output << " Brand Name: " << properties->brandName << std::endl;
    output << " Model Name: " << properties->modelName << std::endl;
    output << " Vendor Name: " << properties->vendorName << std::endl;
    output << " Driver Version: " << properties->driverVersion << std::endl;
    switch (ext_properties->type) {
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
    output << " Is integrated with host: "
           << ((ext_properties->flags & ZES_DEVICE_PROPERTY_FLAG_INTEGRATED)
                   ? "Yes"
                   : "No")
           << std::endl;
    output << " Is a sub-device: "
           << ((ext_properties->flags & ZES_DEVICE_PROPERTY_FLAG_SUBDEVICE)
                   ? "Yes"
                   : "No")
           << std::endl;
    output << " Supports error correcting memory: "
           << ((ext_properties->flags & ZES_DEVICE_PROPERTY_FLAG_ECC) ? "Yes"
                                                                      : "No")
           << std::endl;
    output << " Supports on-demand page-faulting: "
           << ((ext_properties->flags & ZES_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING)
                   ? "Yes"
                   : "No")
           << std::endl;
    printf("%s", output.str().c_str());
}

void show_engine_group_properties(const Engine *engine) {
    printf("   %s", engine_type_to_str(engine->getEngineProperties()->type));
    if (engine->getEngineProperties()->onSubdevice) {
        printf(" (Sub-device ID: %04X)", engine->getEngineProperties()->subdeviceId);
    }
    printf("\n");
}

void show_engine_groups(const Device *device) {
    uint32_t enginesCount = device->getEngineCount();
    printf(" Engines: %d\n", enginesCount);
    if (enginesCount == 0) {
        return;
    }
    for (uint32_t i = 0; i < enginesCount; ++i) {
        const Engine *engine = device->getEngine(i);
        printf("  Engine %d: %s", i + 1, engine_type_to_str(engine->getEngineProperties()->type));
        if (engine->getEngineProperties()->onSubdevice) {
            printf(" (Sub-device ID: %04X)", engine->getEngineProperties()->subdeviceId);
        }
        printf("\n");
    }
}

void show_power_domains(const Device *device) {
    uint32_t powerDomainsCount = device->getPowerDomainCount();
    printf(" Power Domains: %d\n", powerDomainsCount);
    if (powerDomainsCount == 0) {
        return;
    }
    for (uint32_t i = 0; i < powerDomainsCount; ++i) {
        const PowerDomain *powerDomain = device->getPowerDomain(i);
        const zes_power_properties_t *properties = powerDomain->getPowerDomainProperties();
        if (properties->onSubdevice) {
            printf("  Power Domain %d: Sub Device: %04X, Can Control: %s, Threshold supported: %s\n",
                   i + 1, properties->subdeviceId, properties->canControl ? "Yes" : "No",
                   properties->isEnergyThresholdSupported ? "Yes" : "No");
        } else {
            printf("  Power Domain %d: Can Control: %s, Threshold supported: %s\n",
                   i + 1, properties->canControl ? "Yes" : "No",
                   properties->isEnergyThresholdSupported ? "Yes" : "No");
        }
    }
}

void show_psus(const Device *device) {
    uint32_t psuCount = device->getPSUCount();
    printf(" Power Supply Units: %d\n", psuCount);
    if (psuCount == 0) {
        return;
    }
    for (uint32_t i = 0; i < psuCount; ++i) {
        const PSU *psu = device->getPSU(i);
        const zes_psu_properties_t *properties = psu->getPSUProperties();
        if (properties->onSubdevice) {
            printf("  Power Supply Unit %d: Sub Device: %04X, Fan: %s, Amp Limit: %d\n",
                   i + 1, properties->subdeviceId, properties->haveFan ? "Yes" : "No",
                   properties->ampLimit);
        } else {
            printf("  Power Supply Unit %d: Fan: %s, Amp Limit: %d\n", i + 1,
                   properties->haveFan ? "Yes" : "No", properties->ampLimit);
        }
        printf("\n");
    }
}

void show_temperatures(Device *device) {
    device->updateTemperatures();
    for (uint32_t i = 0; i < device->getTemperatureCount(); ++i) {
        printf("  Sensor %d: %.1fC", i, device->getTemperature(i));
    }
}

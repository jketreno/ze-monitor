#pragma once

#include "engine.h"
#include "power_domain.h"

#include <level_zero/ze_api.h>  // for _ze_result_t, ze_result_t, ZE_MAX_DE...
#include <level_zero/zes_api.h> // for zes_device_handle_t, _zes_structure_...
#include <stdexcept>            // for runtime_error
#include <memory>               // for unique_ptr, allocator, make_unique
#include <vector>               // for vector

class Device {
public:
    Device(zes_device_handle_t handle) : device(handle)
    {
        std::memset(&deviceExtProperties, 0, sizeof(deviceExtProperties));
        deviceExtProperties.stype = ZES_STRUCTURE_TYPE_DEVICE_EXT_PROPERTIES;
        std::memset(&deviceProperties, 0, sizeof(deviceProperties));
        deviceProperties.stype = ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES;
        deviceProperties.pNext = &deviceExtProperties;
        std::memset(&pciProperties, 0, sizeof(pciProperties));
        pciProperties.stype = ZES_STRUCTURE_TYPE_PCI_PROPERTIES;

        if (!initializeDevice())
        {
            throw std::runtime_error("Failed to initialize engine.");
        }
    }

    zes_device_handle_t getHandle() const { return device; }
    const zes_device_properties_t *getDeviceProperties() const { return &deviceProperties; }
    const zes_device_ext_properties_t *getDeviceExtProperties() const { return &deviceExtProperties; }
    const zes_pci_properties_t *getDevicePciProperties() const { return &pciProperties; }
    uint32_t getEngineCount() const { return engines.size(); }
    const Engine *getEngine(uint32_t index) const { return engines[index].get(); }
    uint32_t getPowerDomainCount() const { return powerDomains.size(); }
    const PowerDomain *getPowerDomain(uint32_t index) const { return powerDomains[index].get(); }

private:
    zes_device_handle_t device;
    zes_device_ext_properties_t deviceExtProperties;
    zes_device_properties_t deviceProperties;
    zes_pci_properties_t pciProperties;

    std::vector<std::unique_ptr<Engine>> engines;
    std::vector<std::unique_ptr<PowerDomain>> powerDomains;

    bool initializeDevice();
};


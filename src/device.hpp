#ifndef __device_hpp__
#define __device_hpp__

#include "engine.hpp"

#include <level_zero/ze_api.h>  // for _ze_result_t, ze_result_t, ZE_MAX_DE...
#include <level_zero/zes_api.h> // for zes_device_handle_t, _zes_structure_...
#include <stdexcept>            // for runtime_error
#include <memory>               // for unique_ptr, allocator, make_unique
#include <vector>               // for vector

class Device {
public:
    Device(zes_device_handle_t handle) : device(handle)
    {
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

private:
    zes_device_handle_t device;
    zes_device_ext_properties_t deviceExtProperties;
    zes_device_properties_t deviceProperties;
    zes_pci_properties_t pciProperties;

    std::vector<std::unique_ptr<Engine>> engines;

    bool initializeDevice();
};

#endif
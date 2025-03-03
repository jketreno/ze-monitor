#pragma once

#include <level_zero/ze_api.h>  // for _ze_result_t, ze_result_t, ZE_MAX_DE...
#include <level_zero/zes_api.h> // for zes_device_handle_t, _zes_structure_...
#include <cstring>               // for unique_ptr, allocator, make_unique
#include <memory>               // for unique_ptr, allocator, make_unique
#include <stdexcept>            // for runtime_error
#include <vector>               // for vector

class Engine {
public:
    Engine(zes_engine_handle_t handle) : engine(handle)
    {
        std::memset(&properties, 0, sizeof(properties));
        properties.stype = ZES_STRUCTURE_TYPE_ENGINE_PROPERTIES;
        std::memset(&stats, 0, sizeof(stats));

        if (!initializeEngine())
        {
            throw std::runtime_error("Failed to initialize engine.");
        }
    }

    zes_engine_handle_t getHandle() const { return engine; }
    double getEngineUtilization() const { return utilization; }
    const zes_engine_properties_t *getEngineProperties() const { return &properties; }

private:    
    zes_engine_handle_t engine;
    zes_engine_stats_t stats;
    zes_engine_properties_t properties;
    double utilization;

    bool initializeEngine();
    ze_result_t updateStats();
};


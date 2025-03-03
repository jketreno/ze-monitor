#pragma once

#include <level_zero/ze_api.h>  // for _ze_result_t, ze_result_t, ZE_MAX_DE...
#include <level_zero/zes_api.h> // for zes_device_handle_t, _zes_structure_...
#include <cstring>               // for unique_ptr, allocator, make_unique
#include <memory>               // for unique_ptr, allocator, make_unique
#include <stdexcept>            // for runtime_error
#include <vector>               // for vector

class PSU {
public:
    PSU(zes_psu_handle_t handle) : psu(handle)
    {
        std::memset(&properties, 0, sizeof(properties));
        properties.stype = ZES_STRUCTURE_TYPE_PSU_PROPERTIES;
        std::memset(&state, 0, sizeof(state));
        state.stype = ZES_STRUCTURE_TYPE_PSU_STATE;

        if (!initializePSU())
        {
            throw std::runtime_error("Failed to initialize power supply unit.");
        }
    }

    zes_psu_handle_t getHandle() const { return psu; }
    const zes_psu_state_t *getPSUState() const { return &state; }
    const zes_psu_properties_t *getPSUProperties() const { return &properties; }

private:    
    zes_psu_handle_t psu;
    zes_psu_properties_t properties;
    zes_psu_state_t state;
    bool initializePSU();
    ze_result_t updateStats();
};


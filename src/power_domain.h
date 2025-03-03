#pragma once

#include <level_zero/ze_api.h>  // for _ze_result_t, ze_result_t, ZE_MAX_DE...
#include <level_zero/zes_api.h> // for zes_device_handle_t, _zes_structure_...
#include <cstring>               // for unique_ptr, allocator, make_unique
#include <memory>               // for unique_ptr, allocator, make_unique
#include <stdexcept>            // for runtime_error
#include <vector>               // for vector

class PowerDomain {
public:
    PowerDomain(zes_pwr_handle_t handle) : power(handle)
    {
        std::memset(&properties, 0, sizeof(properties));
        properties.stype = ZES_STRUCTURE_TYPE_POWER_PROPERTIES;
        std::memset(&counter, 0, sizeof(counter));

        if (!initializePowerDomain())
        {
            throw std::runtime_error("Failed to initialize power.");
        }
    }

    zes_pwr_handle_t getHandle() const { return power; }
    double getPowerDomainEnergy() const { return energy; }
    const zes_power_properties_t *getPowerDomainProperties() const { return &properties; }

private:    
    zes_pwr_handle_t power;
    zes_power_properties_t properties;
    zes_power_energy_counter_t counter;
    double energy;
    bool initializePowerDomain();
    ze_result_t updateStats();
};


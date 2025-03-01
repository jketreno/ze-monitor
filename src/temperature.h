#pragma once

#include <level_zero/ze_api.h>  // for _ze_result_t, ze_result_t, ZE_MAX_DE...
#include <level_zero/zes_api.h> // for zes_device_handle_t, _zes_structure_...
#include <fstream>              // for basic_ostream, operator<<, endl, bas...
#include <stdexcept>            // for runtime_error
#include <iostream>             // for cerr, cout
#include <memory>               // for unique_ptr, allocator, make_unique
#include <sstream>              // for basic_ostringstream
#include <vector>               // for vector

class TemperatureMonitor
{
public:
    TemperatureMonitor(zes_device_handle_t device) : device(device)
    {
        if (!initializeSensors())
        {
            throw std::runtime_error("Failed to initialize temperature monitoring.");
        }
    }

    void updateTemperatures();
    void displayTemperatures(uint32_t index);
    double getTemperature(uint32_t index) const { return temperatures[index]; };
    uint32_t getSensorCount() const { return sensors.size(); }

private:
    zes_device_handle_t device;
    std::vector<zes_temp_handle_t> sensors;
    std::vector<double> temperatures;

    bool initializeSensors();
};


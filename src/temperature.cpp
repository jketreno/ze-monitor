#include "temperature.h"

bool TemperatureMonitor::initializeSensors()
{
    uint32_t count = 0;
    ze_result_t result = zesDeviceEnumTemperatureSensors(device, &count, nullptr);
    if (result != ZE_RESULT_SUCCESS)
    {
        std::cerr << "Failed to enumerate temperature sensors: " << result << "\n";
        return false;
    }

    if (count > 0)
    {
        sensors.resize(count);
        temperatures.resize(count);
        result = zesDeviceEnumTemperatureSensors(device, &count, sensors.data());
        if (result != ZE_RESULT_SUCCESS)
        {
            std::cerr << "Failed to retrieve temperature sensors: " << result << "\n";
            sensors.clear();
            return false;
        }
    }
    return true;
}

ze_result_t TemperatureMonitor::updateTemperatures()
{
    ze_result_t ret;
    for (size_t i = 0; i < sensors.size(); ++i)
    {
        ret = zesTemperatureGetState(sensors[i], &temperatures[i]);
        if (ret != ZE_RESULT_SUCCESS)
        {
            std::cerr << "Failed to get temperature for sensor " << i << "\n";
            return ret;
        }
    }
    return ZE_RESULT_SUCCESS;
}

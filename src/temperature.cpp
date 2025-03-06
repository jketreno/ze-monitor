#include "temperature.h"
#include "helpers.h"

bool TemperatureMonitor::initializeSensors()
{
    uint32_t count = 0;
    ze_result_t ret = zesDeviceEnumTemperatureSensors(device, &count, nullptr);
    if (ret != ZE_RESULT_SUCCESS)
    {
        std::cerr << "Failed to enumerate temperature sensors: " << std::hex << ret << " (" << ze_error_to_str(ret) << ")" << std::endl;
        return false;
    }

    if (count > 0)
    {
        sensors.resize(count);
        temperatures.resize(count);
        ret = zesDeviceEnumTemperatureSensors(device, &count, sensors.data());
        if (ret != ZE_RESULT_SUCCESS)
        {
            std::cerr << "Failed to retrieve temperature sensors: " << std::hex << ret << " (" << ze_error_to_str(ret) << ")" << std::endl;
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
            std::cerr << "Failed to get temperature sensor " << i << ": " << std::hex << ret << " (" << ze_error_to_str(ret) << ")" << std::endl;
            return ret;
        }
    }
    return ZE_RESULT_SUCCESS;
}

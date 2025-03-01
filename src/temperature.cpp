#include "temperature.hpp"
#include <ncurses.h>            // for move, wprintw, stdscr, noecho, cbreak

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

void TemperatureMonitor::updateTemperatures()
{
    for (size_t i = 0; i < sensors.size(); ++i)
    {
        if (zesTemperatureGetState(sensors[i], &temperatures[i]) != ZE_RESULT_SUCCESS)
        {
            std::cerr << "Failed to get temperature for sensor " << i << "\n";
        }
    }
}

void TemperatureMonitor::displayTemperatures(uint32_t index)
{
    if (index >= temperatures.size())
        return;
    std::ostringstream output;
    output << "Sensor " << index << ": " << temperatures[index] << "C";
    wprintw(stdscr, "%s", output.str().c_str());
}


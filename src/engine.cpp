#include "engine.h"
#include <iostream>             // for cerr, cout

bool Engine::initializeEngine()
{
    ze_result_t ret;

    ret = zesEngineGetProperties(engine, &properties);
    if (ret != ZE_RESULT_SUCCESS)
    {
        return false;
    }

    ret = updateStats();
    if (ret != ZE_RESULT_SUCCESS)
    {
        std::cerr << "Failed to get activity for engine." << std::endl;
        return false;
    }

    return true;
}

ze_result_t Engine::updateStats() {
    uint64_t lastActivetime = stats.activeTime;
    uint64_t lastTimestamp = stats.timestamp;
    ze_result_t ret;

    ret = zesEngineGetActivity(engine, &stats);
    if (ret != ZE_RESULT_SUCCESS)
    {
        std::cerr << "Failed to get activity for engine." << std::endl;
        return ret;
    }

    if ((stats.timestamp - lastTimestamp) > 0)
    {
        utilization = 100 * (stats.activeTime - lastActivetime) /
                     (stats.timestamp - lastTimestamp);
    }
    else
    {
        utilization = 0;
    }

    if (utilization > 100) {
        fprintf(stderr, "stats error: %lu %lu %lu %lu\n", stats.activeTime, lastActivetime, stats.timestamp, lastTimestamp);
    }

    return ZE_RESULT_SUCCESS;
}

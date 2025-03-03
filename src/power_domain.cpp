#include "power_domain.h"
#include <iostream>             // for cerr, cout

bool PowerDomain::initializePowerDomain() {
    ze_result_t ret;

    ret = zesPowerGetProperties(power, &properties);
    if (ret != ZE_RESULT_SUCCESS)
    {
        return false;
    }

    ret = updateStats();
    if (ret != ZE_RESULT_SUCCESS)
    {
        std::cerr << "Failed to get activity for power." << std::endl;
        return false;
    }

    return true;
}

ze_result_t PowerDomain::updateStats() {
    uint64_t lastEnergy = counter.energy;
    uint64_t lastTimestamp = counter.timestamp;
    ze_result_t ret;

    ret = zesPowerGetEnergyCounter(power, &counter);
    if (ret != ZE_RESULT_SUCCESS)
    {
        std::cerr << "Failed to get energy counter." << std::endl;
        return ret;
    }

    if ((counter.timestamp - lastTimestamp) > 0)
    {
        energy = (counter.energy - lastEnergy) /
                (counter.timestamp - lastTimestamp);
    }
    else
    {
        energy = 0;
    }

    return ZE_RESULT_SUCCESS;
}

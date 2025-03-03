#include "psu.h"
#include <iostream>             // for cerr, cout

bool PSU::initializePSU() {
    ze_result_t ret;

    ret = zesPsuGetProperties(psu, &properties);
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

ze_result_t PSU::updateStats() {
    ze_result_t ret;

    ret = zesPsuGetState(psu, &state);
    if (ret != ZE_RESULT_SUCCESS)
    {
        std::cerr << "Failed to get PSU stats." << std::endl;
        return ret;
    }
    
    return ZE_RESULT_SUCCESS;
}

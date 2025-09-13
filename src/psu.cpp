
#include "psu.h"

bool PSU::initializePSU() {
  ze_result_t ret = zesPsuGetProperties(psu, &properties);
  if (ret != ZE_RESULT_SUCCESS)
    return false;
  ret = updateStats();
  return ret == ZE_RESULT_SUCCESS;
}

ze_result_t PSU::updateStats() {
  ze_result_t ret;

  ret = zesPsuGetState(psu, &state);
  if (ret != ZE_RESULT_SUCCESS)
    return ret;
  return ZE_RESULT_SUCCESS;
}

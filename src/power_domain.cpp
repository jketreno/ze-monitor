
#include "power_domain.h"
#include <cmath>

bool PowerDomain::initializePowerDomain() {
  ze_result_t ret = zesPowerGetProperties(power, &properties);
  if (ret != ZE_RESULT_SUCCESS)
    return false;
  ret = updateStats();
  return ret == ZE_RESULT_SUCCESS;
}

ze_result_t PowerDomain::updateStats() {
  uint64_t lastEnergy = counter.energy;
  uint64_t lastTimestamp = counter.timestamp;
  ze_result_t ret;

  ret = zesPowerGetEnergyCounter(power, &counter);
  if (ret != ZE_RESULT_SUCCESS)
    return ret;

  if ((counter.timestamp - lastTimestamp) > 0) {
    double raw = static_cast<double>(counter.energy - lastEnergy) /
                 static_cast<double>(counter.timestamp - lastTimestamp);
    // Clamp negative, NaN, or Inf values to zero
    if (raw < 0.0 || !std::isfinite(raw)) {
      energy = 0.0;
    } else {
      energy = raw;
    }
  } else {
    energy = 0.0;
  }

  return ZE_RESULT_SUCCESS;
}

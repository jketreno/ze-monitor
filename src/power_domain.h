#pragma once

#include "initializable.h"
#include <cstring>
#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>

class PowerDomain : public Initializable {
public:
  explicit PowerDomain(zes_pwr_handle_t handle) : power(handle), energy(0.0) {
    memset(&properties, 0, sizeof(properties));
    properties.stype = ZES_STRUCTURE_TYPE_POWER_PROPERTIES;
    memset(&counter, 0, sizeof(counter));
    require_init(initializePowerDomain(), "power domain");
  }

  zes_pwr_handle_t getHandle() const { return power; }
  double getPowerDomainEnergy() {
    ze_result_t ret = updateStats();
    if (ret != ZE_RESULT_SUCCESS) {
      return 0;
    }
    return energy;
  }
  const zes_power_properties_t *getPowerDomainProperties() const {
    return &properties;
  }

private:
  zes_pwr_handle_t power;
  zes_power_properties_t properties;
  zes_power_energy_counter_t counter;
  double energy;
  bool initializePowerDomain();
  ze_result_t updateStats();
};

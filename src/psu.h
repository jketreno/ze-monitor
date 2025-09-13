#pragma once

#include "initializable.h"
#include <cstring>
#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>

class PSU : public Initializable {
public:
  explicit PSU(zes_psu_handle_t handle) : psu(handle) {
    memset(&properties, 0, sizeof(properties));
    properties.stype = ZES_STRUCTURE_TYPE_PSU_PROPERTIES;
    memset(&state, 0, sizeof(state));
    state.stype = ZES_STRUCTURE_TYPE_PSU_STATE;
    require_init(initializePSU(), "power supply unit");
  }

  zes_psu_handle_t getHandle() const { return psu; }
  const zes_psu_state_t *getPSUState() const { return &state; }
  const zes_psu_properties_t *getPSUProperties() const { return &properties; }

private:
  zes_psu_handle_t psu;
  zes_psu_properties_t properties;
  zes_psu_state_t state;
  bool initializePSU();
  ze_result_t updateStats();
};

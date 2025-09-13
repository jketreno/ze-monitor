#pragma once

#include "initializable.h"
#include <cstring>
#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>

class Engine : public Initializable {
public:
  explicit Engine(zes_engine_handle_t handle)
      : engine(handle), utilization(0.0) {
    memset(&properties, 0, sizeof(properties));
    properties.stype = ZES_STRUCTURE_TYPE_ENGINE_PROPERTIES;
    memset(&stats, 0, sizeof(stats));
    require_init(initializeEngine(), "engine");
  }

  zes_engine_handle_t getHandle() const { return engine; }
  double getEngineUtilization() {
    ze_result_t ret = updateStats();
    if (ret != ZE_RESULT_SUCCESS) {
      return 0;
    }
    return utilization;
  }
  const zes_engine_properties_t *getEngineProperties() const {
    return &properties;
  }

private:
  zes_engine_handle_t engine;
  zes_engine_stats_t stats;
  zes_engine_properties_t properties;
  double utilization;

  bool initializeEngine();
  ze_result_t updateStats();
};

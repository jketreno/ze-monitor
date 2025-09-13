#pragma once

#include "initializable.h"
#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>
#include <vector>

class TemperatureMonitor : public Initializable {
public:
  explicit TemperatureMonitor(zes_device_handle_t device) : device(device) {
    require_init(initializeSensors(), "temperature monitoring");
  }

  ze_result_t updateTemperatures();
  double getTemperature(uint32_t index) const { return temperatures[index]; }
  uint32_t getSensorCount() const { return sensors.size(); }

private:
  zes_device_handle_t device;
  std::vector<zes_temp_handle_t> sensors;
  std::vector<double> temperatures;

  bool initializeSensors();
};

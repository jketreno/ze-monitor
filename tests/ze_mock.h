#pragma once

#include <level_zero/ze_api.h>  // for _ze_result_t, ze_result_t, ZE_MAX_DE...
#include <level_zero/zes_api.h> // for zes_device_handle_t, _zes_structure_...
#include <vector>

extern ze_result_t g_enumSensorsResult;
extern uint32_t g_sensorCount;
extern ze_result_t g_getTempResult;
extern std::vector<double> g_mockTemperatures;
extern bool g_enumSensorsCalledOnce;
extern void resetMocks();

#include "ze_mock.h"
#include <level_zero/ze_api.h>  // for _ze_result_t, ze_result_t, ZE_MAX_DE...
#include <level_zero/zes_api.h> // for zes_device_handle_t, _zes_structure_...
#include <vector>

ze_result_t g_enumSensorsResult = ZE_RESULT_SUCCESS;
uint32_t g_sensorCount = 3;
ze_result_t g_getTempResult = ZE_RESULT_SUCCESS;
std::vector<double> g_mockTemperatures = {45.5, 52.8, 39.2};
bool g_enumSensorsCalledOnce = false;

// Mock Implementation of `zesDeviceGetProperties`
ze_result_t zesDeviceGetProperties(zes_device_handle_t device, zes_device_properties_t* pProperties) {
    if (!pProperties) return ZE_RESULT_ERROR_INVALID_ARGUMENT;
    return ZE_RESULT_SUCCESS;
}

// Mock Implementation of `zesDevicePciGetProperties`
ze_result_t zesDevicePciGetProperties(zes_device_handle_t device, zes_pci_properties_t* pProperties) {
    if (!pProperties) return ZE_RESULT_ERROR_INVALID_ARGUMENT;
    pProperties->address.domain = 0;
    pProperties->address.bus = 1;
    pProperties->address.device = 2;
    pProperties->address.function = 0;
    return ZE_RESULT_SUCCESS;
}

// Mock Implementation of `zesDeviceEnumEngineGroups`
ze_result_t zesDeviceEnumEngineGroups(zes_device_handle_t device, uint32_t* pCount, zes_engine_handle_t* phEngines) {
    if (!pCount) return ZE_RESULT_ERROR_INVALID_ARGUMENT;

    if (!phEngines) {
        *pCount = g_sensorCount; // Mock count
        return ZE_RESULT_SUCCESS;
    }

    for (uint32_t i = 0; i < *pCount; i++) {
        phEngines[i] = reinterpret_cast<zes_engine_handle_t>(i + 1);
    }
    return ZE_RESULT_SUCCESS;
}

// Mock Implementation of `zesDeviceEnumPowerDomains`
ze_result_t zesDeviceEnumPowerDomains(zes_device_handle_t device, uint32_t* pCount, zes_pwr_handle_t* phPower) {
    if (!pCount) return ZE_RESULT_ERROR_INVALID_ARGUMENT;

    if (!phPower) {
        *pCount = 2; // Mock 2 power domains
        return ZE_RESULT_SUCCESS;
    }

    for (uint32_t i = 0; i < *pCount; i++) {
        phPower[i] = reinterpret_cast<zes_pwr_handle_t>(i + 1);
    }
    return ZE_RESULT_SUCCESS;
}

// Mock Implementation of `zesDeviceEnumPsus`
ze_result_t zesDeviceEnumPsus(zes_device_handle_t device, uint32_t* pCount, zes_psu_handle_t* phPsu) {
    if (!pCount) return ZE_RESULT_ERROR_INVALID_ARGUMENT;

    if (!phPsu) {
        *pCount = 1; // Assume 1 PSU available
        return ZE_RESULT_SUCCESS;
    }

    for (uint32_t i = 0; i < *pCount; i++) {
        phPsu[i] = reinterpret_cast<zes_psu_handle_t>(i + 1);
    }
    return ZE_RESULT_SUCCESS;
}

// Mock Implementation of `zesDeviceEnumMemoryModules`
ze_result_t zesDeviceEnumMemoryModules(zes_device_handle_t device, uint32_t* pCount, zes_mem_handle_t* phMemory) {
    if (!pCount) return ZE_RESULT_ERROR_INVALID_ARGUMENT;

    if (!phMemory) {
        *pCount = 2; // Assume 2 memory modules
        return ZE_RESULT_SUCCESS;
    }

    for (uint32_t i = 0; i < *pCount; i++) {
        phMemory[i] = reinterpret_cast<zes_mem_handle_t>(i + 1);
    }
    return ZE_RESULT_SUCCESS;
}

// Mock Implementation of `zesMemoryGetState`
ze_result_t zesMemoryGetState(zes_mem_handle_t hMemory, zes_mem_state_t* pState) {
    if (!pState) return ZE_RESULT_ERROR_INVALID_ARGUMENT;

    pState->free = 512 * 1024 * 1024; // 512MB free
    pState->size = 1024 * 1024 * 1024; // 1GB total size
    return ZE_RESULT_SUCCESS;
}

ze_result_t zesDeviceEnumTemperatureSensors(zes_device_handle_t device, uint32_t* pCount, zes_temp_handle_t* pSensors) {
    if (g_enumSensorsResult != ZE_RESULT_SUCCESS) {
        return g_enumSensorsResult;
    }

    if (pSensors == nullptr) {
        *pCount = g_sensorCount;
        g_enumSensorsCalledOnce = true;
        return ZE_RESULT_SUCCESS;
    } else {
        if (!g_enumSensorsCalledOnce) {
            return ZE_RESULT_ERROR_INVALID_ARGUMENT;
        }
        
        // Fill sensor handles with dummy values
        for (uint32_t i = 0; i < g_sensorCount; i++) {
            // Cast an integer to a sensor handle for testing
            pSensors[i] = reinterpret_cast<zes_temp_handle_t>(static_cast<uintptr_t>(i + 1));
        }
        return ZE_RESULT_SUCCESS;
    }
}

ze_result_t zesTemperatureGetState(zes_temp_handle_t hTemperature, double* pTemperature) {
    if (g_getTempResult != ZE_RESULT_SUCCESS) {
        return g_getTempResult;
    }

    // Convert the handle back to an index
    uintptr_t index = reinterpret_cast<uintptr_t>(hTemperature) - 1;
    
    if (index < g_mockTemperatures.size()) {
        *pTemperature = g_mockTemperatures[index];
        return ZE_RESULT_SUCCESS;
    }
    
    return ZE_RESULT_ERROR_INVALID_ARGUMENT;
}

// Helper to reset mocks between tests
void resetMocks() {
    g_enumSensorsResult = ZE_RESULT_SUCCESS;
    g_sensorCount = 3;
    g_getTempResult = ZE_RESULT_SUCCESS;
    g_mockTemperatures = {45.5, 52.8, 39.2};
    g_enumSensorsCalledOnce = false;
}

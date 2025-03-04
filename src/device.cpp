#include "device.h"
#include "helpers.h"
#include <iostream>             // for cerr, cout
#include <stdexcept>            // for runtime_error

bool Device::initializeDevice()
{
    ze_result_t ret;

    ret = zesDeviceGetProperties(device, &deviceProperties);
    if (ret != ZE_RESULT_SUCCESS)
    {
        std::cerr << "zesDeviceGetProperties failed " << std::hex << ret << " " << ze_error_to_str(ret) << std::endl;
        return false;
    }

    ret = zesDevicePciGetProperties(device, &pciProperties);
    if (ret != ZE_RESULT_SUCCESS)
    {
        std::cerr << "zesDevicePciGetProperties failed " << std::hex << ret << " " << ze_error_to_str(ret) << std::endl;
        return false;
    }

    uint32_t count = 0;
    ze_result_t result;

    result = zesDeviceEnumEngineGroups(device, &count, nullptr);
    if (result != ZE_RESULT_SUCCESS)
    {
        std::cerr << "Failed to enumerate engine groups: " << result << "\n";
        return false;
    }

    if (count > 0)
    {
        std::unique_ptr<zes_engine_handle_t[]> engineHandles = std::make_unique<zes_engine_handle_t[]>(count);

        result = zesDeviceEnumEngineGroups(device, &count, engineHandles.get());
        if (result != ZE_RESULT_SUCCESS)
        {
            std::cerr << "Failed to retrieve engine groups: " << result << "\n";
            return false;
        }

        for (size_t i = 0; i < count; ++i) 
        {
            engines.emplace_back(std::make_unique<Engine>(engineHandles[i]));
        }
    }

    count = 0;
    result = zesDeviceEnumPowerDomains(device, &count, nullptr);
    if (result != ZE_RESULT_SUCCESS)
    {
        std::cerr << "Failed to enumerate power domains: " << result << "\n";
        return false;
    }

    if (count > 0)
    {
        std::unique_ptr<zes_pwr_handle_t[]> powerHandles = std::make_unique<zes_pwr_handle_t[]>(count);

        result = zesDeviceEnumPowerDomains(device, &count, powerHandles.get());
        if (result != ZE_RESULT_SUCCESS)
        {
            std::cerr << "Failed to retrieve power domains: " << result << "\n";
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            powerDomains.emplace_back(std::make_unique<PowerDomain>(powerHandles[i]));
        }
    }

    count = 0;
    result = zesDeviceEnumPsus(device, &count, nullptr);
    if (result != ZE_RESULT_SUCCESS)
    {
        // Not all hardware supports PSUs
        if (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE)
        {
            count = 0;
        }
        else
        {
            std::cerr << "Failed to enumerate power supply units: " << std::hex << result << " (" << ze_error_to_str(result) << ")" << std::endl;
            return false;
        }
    }

    if (count > 0)
    {
        std::unique_ptr<zes_psu_handle_t[]> psuHandles = std::make_unique<zes_psu_handle_t[]>(count);

        result = zesDeviceEnumPsus(device, &count, psuHandles.get());
        if (result != ZE_RESULT_SUCCESS)
        {
            std::cerr << "Failed to enumerate power supply units: " << std::hex << result << " (" << ze_error_to_str(result) << ")" << std::endl;
            return false;
        }

        for (size_t i = 0; i < count; ++i)
        {
            psus.emplace_back(std::make_unique<PSU>(psuHandles[i]));
        }
    }

    count = 0;
    result = zesDeviceEnumMemoryModules(device, &count, nullptr);
    if (result != ZE_RESULT_SUCCESS)
    {
        // Not all hardware supports PSUs
        if (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE)
        {
            count = 0;
        }
        else
        {
            std::cerr << "Failed to enumerate memory modules: " << std::hex << result << " (" << ze_error_to_str(result) << ")" << std::endl;
            return false;
        }
    }

    if (count > 0)
    {
        memoryHandles.resize(count);

        result = zesDeviceEnumMemoryModules(device, &count, memoryHandles.data());
        if (result != ZE_RESULT_SUCCESS)
        {
            std::cerr << "Failed to enumerate power memory modules: " << std::hex << result << " (" << ze_error_to_str(result) << ")" << std::endl;
            return false;
        }
    }

    return true;
}

const zes_mem_state_t Device::getMemoryState()
{
    zes_mem_state_t ret;
    ret.free = 0;
    ret.size = 0;

    for (uint32_t i = 0; i < memoryHandles.size(); ++i)
    {
        zes_mem_state_t memState;

        zesMemoryGetState(memoryHandles[i], &memState);
        ret.free += memState.free;
        ret.size += memState.size;
    }

    return ret;
}
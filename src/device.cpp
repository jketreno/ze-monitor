#include "device.h"
#include "helpers.h"
#include <iostream>             // for cerr, cout

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

    return true;
}

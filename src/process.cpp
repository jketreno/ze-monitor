#include "process.h"
#include "helpers.h"

ze_result_t ProcessMonitor::updateProcessStats()
{
    zes_process_state_t processes[_MAX_PROCESS];
    uint32_t count = _MAX_PROCESS;

    ze_result_t ret = zesDeviceProcessesGetState(device, &count, processes);
    if (ret != ZE_RESULT_SUCCESS && ret != ZE_RESULT_ERROR_INVALID_SIZE)
    {
        std::cerr << "Unable to get process information (ret " << std::hex << ret << "): " << ze_error_to_str(ret) << std::endl;
        return ZE_RESULT_ERROR_UNSUPPORTED_FEATURE;
    }

    if (ret == ZE_RESULT_ERROR_INVALID_SIZE)
    {
        count = std::min(count, _MAX_PROCESS);
        ret = zesDeviceProcessesGetState(device, &count, processes);
        if (ret != ZE_RESULT_SUCCESS)
        {
            std::cerr << "Retry failed to get process info (ret " << std::hex << ret << "): " << ze_error_to_str(ret) << std::endl;
            return ZE_RESULT_ERROR_UNSUPPORTED_FEATURE;
        }
    }

    // TODO: Walk looking for matching pids in new array and only update the
    // zes_process_state_t fields; don't re-look up the process info
    processInfo.clear();
    for (size_t i = 0; i < count; ++i)
    {
        processInfo.emplace_back(std::make_unique<ProcessInfo>(processes[i]));
    }

    return ZE_RESULT_SUCCESS;
}
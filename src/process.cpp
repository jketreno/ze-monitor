#include "process.hpp"
#include "helpers.hpp"

bool ProcessMonitor::initializeProcesses()
{
    actualSize = _MAX_PROCESS;

    ze_result_t ret = zesDeviceProcessesGetState(device, &actualSize, processes.data());
    if (ret != ZE_RESULT_SUCCESS && ret != ZE_RESULT_ERROR_INVALID_SIZE)
    {
        actualSize = 0;
        std::cerr << "Unable to get process information (ret " << std::hex << ret << "): " << ze_error_to_str(ret) << std::endl;
        return true;
    }

    if (ret == ZE_RESULT_ERROR_INVALID_SIZE)
    {
        actualSize = std::min(actualSize, _MAX_PROCESS);
        ret = zesDeviceProcessesGetState(device, &actualSize, processes.data());
        if (ret != ZE_RESULT_SUCCESS)
        {
            std::cerr << "Retry failed to get process info (ret " << std::hex << ret << "): " << ze_error_to_str(ret) << std::endl;
            return false;
        }
    }

    return true;
}

void ProcessMonitor::displayProcesses(uint32_t index) const
{
    if (index >= actualSize)
        return;
    const auto &process = processes[index];
    try
    {
        ProcessInfo info(process.processId);
        std::ostringstream output;
        output << process.processId << " "
               << info.getCommandLine() << " "
               << "MEM: " << process.memSize << " "
               << "SHR: " << process.sharedSize << " "
               << "FLAGS: " << engine_flags_to_str(process.engines)
               << std::endl;
        wprintw(stdscr, "%s", output.str().c_str());
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error retrieving process info: " << e.what() << std::endl;
    }
}


#pragma once

#include <level_zero/ze_api.h>  // for _ze_result_t, ze_result_t, ZE_MAX_DE...
#include <level_zero/zes_api.h> // for zes_device_handle_t, _zes_structure_...
#include <fstream>              // for basic_ostream, operator<<, endl, bas...
#include <stdexcept>            // for runtime_error
#include <iostream>             // for cerr, cout
#include <memory>               // for unique_ptr, allocator, make_unique
#include <sstream>              // for basic_ostringstream
#include <vector>               // for vector

class ProcessInfo
{
public:
    explicit ProcessInfo(zes_process_state_t state) : state(state) {}

    const zes_process_state_t *getProcessState() const { return &state; }
    std::string getProcessName() const { return readFile("/proc/" + std::to_string(state.processId) + "/comm"); }
    std::string getCommandLine() const
    {
        std::string cmdline = readFile("/proc/" + std::to_string(state.processId) + "/cmdline");
        for (char &c : cmdline)
        {
            if (c == '\0')
                c = ' '; // Replace null terminators with spaces
        }
        return cmdline;
    }

private:
    zes_process_state_t state;

    static std::string readFile(const std::string &path)
    {
        std::ifstream file(path);
        if (!file)
            return "N/A";
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

class ProcessMonitor
{
public:
    explicit ProcessMonitor(zes_device_handle_t handle) : device(handle)
    {
    }

    ze_result_t updateProcessStats();
    uint32_t getProcessCount() const { return processInfo.size(); }
    const ProcessInfo *getProcessInfo(uint32_t index) const { return processInfo[index].get(); }

private:
    zes_device_handle_t device;
    std::vector<std::unique_ptr<ProcessInfo>> processInfo;
    static const uint32_t _MAX_PROCESS = 2048;
};

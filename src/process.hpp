#ifndef __process_hpp__
#define __process_hpp__

#include <level_zero/ze_api.h>  // for _ze_result_t, ze_result_t, ZE_MAX_DE...
#include <level_zero/zes_api.h> // for zes_device_handle_t, _zes_structure_...
#include <fstream>              // for basic_ostream, operator<<, endl, bas...
#include <stdexcept>            // for runtime_error
#include <iostream>             // for cerr, cout
#include <memory>               // for unique_ptr, allocator, make_unique
#include <sstream>              // for basic_ostringstream
#include <vector>               // for vector

class ProcessMonitor
{
public:
    explicit ProcessMonitor(zes_device_handle_t device) : device(device)
    {
        processes.resize(_MAX_PROCESS);

        if (!initializeProcesses())
        {
            std::cerr << "Failed to initialize process monitoring." << std::endl;
        }
    }

    void displayProcesses(uint32_t index) const;
    uint32_t getProcessCount() const { return actualSize; }

private:
    zes_device_handle_t device;
    std::vector<zes_process_state_t> processes;
    static const uint32_t _MAX_PROCESS = 2048;
    uint32_t actualSize;
    bool initializeProcesses();
};

class ProcessInfo
{
public:
    explicit ProcessInfo(pid_t pid) : pid_(pid) {}

    std::string getProcessName() const { return readFile("/proc/" + std::to_string(pid_) + "/comm"); }
    std::string getCommandLine() const
    {
        std::string cmdline = readFile("/proc/" + std::to_string(pid_) + "/cmdline");
        for (char &c : cmdline)
        {
            if (c == '\0')
                c = ' '; // Replace null terminators with spaces
        }
        return cmdline;
    }

private:
    pid_t pid_;

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

#endif
#pragma once
#include <memory>
#include <vector>
#include <string>
#include "device.h"
#include "args.h"
#include "ui.h"

// Holds state for the monitoring loop
struct MonitorConfig {
    Device* device;
    UIState* state;
    int wait_ms;
    bool no_ansi;
    std::string start_mode;
};

// Run the main monitoring loop (TTY or non-TTY)
int run_monitor(const MonitorConfig& config);

#include "monitor.h"
#include <thread>
#include <chrono>
#include <clocale>
#include <cstdio>
#include <regex>
#include <sstream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include "ui.h"

using namespace ftxui;

int run_monitor(const MonitorConfig& config) {
    Device* device = config.device;
    UIState& state = *config.state;
    int wait_ms = config.wait_ms;
    bool no_ansi = config.no_ansi;
    std::string start_mode = config.start_mode;

    // If the user passed --mode, set the starting view
    if (!start_mode.empty()) {
        if (start_mode == "overview" || start_mode == "OVERVIEW" || start_mode == "0" || start_mode == "1") {
            state.view_mode = ViewMode::OVERVIEW;
        } else if (start_mode == "engines" || start_mode == "ENGINES" || start_mode == "2") {
            state.view_mode = ViewMode::ENGINES;
        } else if (start_mode == "processes" || start_mode == "PROCESSES" || start_mode == "3") {
            state.view_mode = ViewMode::PROCESSES;
        } else if (start_mode == "power" || start_mode == "POWER" || start_mode == "4") {
            state.view_mode = ViewMode::POWER;
        } else if (start_mode == "thermal" || start_mode == "THERMAL" || start_mode == "5") {
            state.view_mode = ViewMode::THERMAL;
        } else {
            fprintf(stderr, "Unknown mode: %s; valid: overview|engines|processes|power|thermal\n", start_mode.c_str());
            return 1;
        }
    }

    if (!isatty(fileno(stdout)) || no_ansi) {
        setlocale(LC_ALL, "");
        device->updateTemperatures();
        device->updateProcesses();
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
        device->updateTemperatures();
        device->updateProcesses();
        const int width = 80;
        const int max_height = 2000;
        Element root = build_ui(device, state, width, max_height, no_ansi);
        try {
            ftxui::Screen screen(width, max_height);
            Render(screen, root);
            std::string out = screen.ToString();
            std::vector<std::string> lines;
            size_t pos = 0;
            while (pos < out.size()) {
                size_t nl = out.find('\n', pos);
                if (nl == std::string::npos) {
                    lines.push_back(out.substr(pos));
                    break;
                }
                lines.push_back(out.substr(pos, nl - pos));
                pos = nl + 1;
            }
            while (!lines.empty()) {
                const std::string &last = lines.back();
                bool all_space = true;
                for (char c : last) {
                    if (c != ' ' && c != '\t' && c != '\r') {
                        all_space = false;
                        break;
                    }
                }
                if (all_space)
                    lines.pop_back();
                else
                    break;
            }
            std::string trimmed;
            for (size_t i = 0; i < lines.size(); ++i) {
                trimmed += lines[i];
                trimmed.push_back('\n');
            }
            if (!trimmed.empty()) {
                if (no_ansi) {
                    static const std::regex ansi_re("\x1B\\[[0-9;]*[A-Za-z]");
                    std::string stripped = std::regex_replace(trimmed, ansi_re, "");
                    fwrite(stripped.c_str(), 1, stripped.size(), stdout);
                } else {
                    fwrite(trimmed.c_str(), 1, trimmed.size(), stdout);
                }
                fflush(stdout);
            }
        } catch (...) {
            // Fallback to simple textual output if rendering fails
            // (optional: could call show_device_properties, etc.)
        }
        return 0;
    }

    // FTXUI main UI loop
    auto screen = ScreenInteractive::Fullscreen();
    std::string final_frame_buffer;
    Element last_rendered_element;
    auto refresh_timer = [&] {
        auto now = std::chrono::steady_clock::now();
        if (now - state.refresh_time > std::chrono::seconds(1)) {
            state.refresh_time = now;
            return true;
        }
        return false;
    };
    // bool one_shot_rendered = false;
    auto component = Renderer([&]() -> Element {
        auto terminal = ftxui::Terminal::Size();
        int screen_width = terminal.dimx;
        int screen_height = terminal.dimy;
        (void)screen_width;
        (void)screen_height;
        if (refresh_timer()) {
            device->updateTemperatures();
            device->updateProcesses();
        }
        Element root = build_ui(device, state, screen_width, screen_height, no_ansi);
        last_rendered_element = root;
        return root;
    });
    std::thread refresh_thread([&]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            screen.PostEvent(Event::Custom);
        }
    });
    refresh_thread.detach();
    if (auto active = ScreenInteractive::Active()) {
        active->Loop(component);
        if (!final_frame_buffer.empty()) {
            auto closure = active->WithRestoredIO([&]() {
                printf("%s\n", final_frame_buffer.c_str());
            });
            closure();
        }
    } else {
        screen.Loop(component);
    }
    return 0;
}

#pragma once
#include <chrono>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <string>
#include <vector>

using ftxui::Element;
using ftxui::Color;
#include <chrono>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <string>
#include <vector>

// Enum for view modes
enum class ViewMode { OVERVIEW, ENGINES, PROCESSES, POWER, THERMAL };

// UI state struct
struct UIState {
  ViewMode view_mode = ViewMode::OVERVIEW;
  int process_offset = 0;
  int engine_offset = 0;
  int thermal_offset = 0;
  int power_offset = 0;
  bool show_help = false;
  std::chrono::steady_clock::time_point refresh_time =
      std::chrono::steady_clock::now();
};

// UI helper function declarations
std::string format_bytes(uint64_t bytes);
Color get_percentage_color(double percentage);
Color get_temp_color(double temp);
std::string ellipses(std::string str, int width, bool from_end = false);
std::string longest_common_prefix(const std::vector<std::string> &v);

// Forward declaration for Device

class Device;

Element build_engine_details(Device *device, UIState &state, int terminal_width,
                     int visible_engines = 15,
                     const std::string &filter_pattern = std::string(),
                     const std::string &title = "🔧 Engine Details");
Element build_process_details(Device *device, UIState &state,
                                     int terminal_width,
                                     int visible_processes = 15,
                                     bool summary = false, int top_n = 3);
Element build_ui(Device *device, UIState &state, int screen_width,
                        int screen_height, bool no_ansi = false);

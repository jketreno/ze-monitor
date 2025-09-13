#include "ui.h"

ftxui::Element build_ui(Device *device, UIState &state, int screen_width, int screen_height, bool no_ansi) {
  (void)device; (void)state; (void)screen_width; (void)screen_height; (void)no_ansi;
  return ftxui::text("UI not yet implemented");
}
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <regex>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

// Format bytes as human-readable string
std::string format_bytes(uint64_t bytes) {
  static const char *sizes[] = {"B", "KB", "MB", "GB", "TB", "PB"};
  int order = 0;
  double num = static_cast<double>(bytes);
  while (num >= 1024 && order < 5) {
    order++;
    num /= 1024;
  }
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(order == 0 ? 0 : 1) << num << sizes[order];
  return oss.str();
}

// Color for percentage (green/yellow/red)

ftxui::Color get_percentage_color(double percentage) {
  (void)percentage;
  return ftxui::Color{};
}

// Color for temperature (green/yellow/red)
ftxui::Color get_temp_color(double temp) {
  (void)temp;
  return ftxui::Color{};
}

// Longest common prefix in a vector of strings
std::string longest_common_prefix(const std::vector<std::string> &v) {
  if (v.empty()) return "";
  std::string prefix = v[0];
  for (size_t i = 1; i < v.size(); ++i) {
    size_t j = 0;
    while (j < prefix.size() && j < v[i].size() && prefix[j] == v[i][j]) ++j;
    prefix = prefix.substr(0, j);
    if (prefix.empty()) break;
  }
  return prefix;
}

// --- UI rendering implementations moved from ze-monitor.cpp ---

#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <sstream>
#include <cstdio>
#include "ui.h"
#include "device.h"
#include "engine.h"
#include "helpers.h"

ftxui::Element build_engine_details(Device *device, UIState &state, int terminal_width,
                     int visible_engines,
                     const std::string &filter_pattern,
                     const std::string &title) {
  ftxui::Elements engine_detail;
  // First gather engine names and strip longest common prefix for display.
  std::vector<std::string> engine_names;
  engine_names.reserve(device->getEngineCount());
  for (uint32_t i = 0; i < device->getEngineCount(); ++i) {
    auto e = device->getEngine(i);
    engine_names.push_back(engine_type_to_str(e->getEngineProperties()->type));
  }
  std::string lcp = longest_common_prefix(engine_names);
  std::vector<std::string> display_names;
  display_names.reserve(engine_names.size());
  size_t max_name_len = 0;
  for (const auto &n : engine_names) {
    std::string name = n;
    if (!lcp.empty() && name.rfind(lcp, 0) == 0) {
      name = name.substr(lcp.size());
    }
    display_names.push_back(name);
    if (name.length() > max_name_len)
      max_name_len = name.length();
  }

  // Fixed widths for sub-device and status columns (retain previous defaults)
  const int subdev_w = 15;
  const int status_w = 15;
  const int percent_w = 5;  // the " 0%" field
  const int separators = 3; // number of separator characters between columns

  int engine_col = (int)max_name_len;
  // Compute available width for utilization column
  int util_w = terminal_width - engine_col - subdev_w - status_w - percent_w -
               separators;
  // If utilization would be too narrow, cap engine column and ellipsize names
  if (util_w < 10) {
  engine_col = std::min(10, engine_col);
    // recompute util width after capping engine column
    util_w = terminal_width - engine_col - subdev_w - status_w - percent_w -
             separators;
  }

  // Build header using computed column widths
  engine_detail.push_back(
  ftxui::hbox({ftxui::text("ENGINE") | ftxui::bold | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, engine_col), ftxui::separator(),
    ftxui::text("UTILIZATION") | ftxui::bold | ftxui::flex, ftxui::separator(),
    ftxui::text("SUB-DEVICE") | ftxui::bold | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, subdev_w), ftxui::separator(),
    ftxui::text("STATUS") | ftxui::bold | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, status_w)}) |
  ftxui::color(ftxui::Color::White));

  // Compile regex filter if provided
  bool use_filter = !filter_pattern.empty();
  std::regex filter_re;
  if (use_filter) {
    try {
      filter_re = std::regex(filter_pattern);
    } catch (...) {
      use_filter = false;
    }
  }

  int start = state.engine_offset;
  int end = std::min(start + visible_engines, (int)device->getEngineCount());

  for (int i = start; i < end; ++i) {
    auto engine = device->getEngine(i);
    double util = engine->getEngineUtilization();
    auto status = util > 0 ? "ACTIVE" : "IDLE";
    auto status_color = util > 0 ? Color::Green : Color::GrayDark;

    std::string name = display_names[i];
    // Apply filter if requested (match against the display name)
    if (use_filter && !std::regex_search(name, filter_re))
      continue;
    // If we capped engine_col to 10 due to narrow util column, ellipsize name
    if ((int)name.length() > engine_col) {
      name = ellipses(name, engine_col);
    }

    engine_detail.push_back(ftxui::hbox({
  ftxui::notflex(ftxui::text(name) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, engine_col) |
     ftxui::color(ftxui::Color::Cyan)),
   ftxui::separator(),
   ftxui::xflex_grow(ftxui::gauge(util / 100.0) | ftxui::color(get_percentage_color(util))),
   ftxui::notflex(ftxui::text(" " + std::to_string((int)util) + "%") |
     ftxui::size(ftxui::WIDTH, ftxui::EQUAL, percent_w) |
     ftxui::color(get_percentage_color(util))),
   ftxui::separator(),
   ftxui::notflex(ftxui::text(engine->getEngineProperties()->onSubdevice
        ? std::to_string(
        engine->getEngineProperties()->subdeviceId)
        : "N/A") |
     ftxui::size(ftxui::WIDTH, ftxui::EQUAL, subdev_w) | ftxui::color(ftxui::Color::GrayDark)),
   ftxui::separator(),
   ftxui::notflex(ftxui::text(status) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, status_w) |
     ftxui::color(status_color))}));
  }

  return ftxui::vbox({ftxui::text(title) | ftxui::bold | ftxui::color(ftxui::Color::Green),
       ftxui::vbox(std::move(engine_detail))}) |
     ftxui::border;
}

ftxui::Element build_process_details(Device *device, UIState &state,
                                     int terminal_width,
                                     int visible_processes,
                                     bool summary, int top_n) {
  (void)terminal_width;
  struct PInfo {
    uint32_t pid;
    uint64_t memSize;
    uint64_t sharedSize;
    uint64_t engines_mask;
    int engines_count;
    std::string command;
  };

  std::vector<PInfo> procs;
  procs.reserve(device->getProcessCount());
  uint64_t total_mem = 0;

  for (uint32_t i = 0; i < device->getProcessCount(); ++i) {
    auto proc = device->getProcessInfo(i);
    auto ps = proc->getProcessState();
    PInfo p;
    p.pid = ps->processId;
    p.memSize = ps->memSize;
    p.sharedSize = ps->sharedSize;
    p.engines_mask = ps->engines;
    p.engines_count = static_cast<int>(__builtin_popcountll(p.engines_mask));
    p.command = proc->getCommandLine();
    procs.push_back(std::move(p));
    total_mem += ps->memSize;
  }

  // Sort by engines_count desc, then PID asc
  std::sort(procs.begin(), procs.end(), [](const PInfo &a, const PInfo &b) {
    if (a.engines_count != b.engines_count)
      return a.engines_count > b.engines_count;
    return a.pid < b.pid;
  });

  if (summary) {
    // Build a table-style summary with the same columns as the full details
  ftxui::Elements process_detail;
  process_detail.push_back(
    ftxui::hbox({ftxui::notflex(ftxui::text("PID") | ftxui::bold | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 8)), ftxui::separator(),
        ftxui::notflex(ftxui::text("COMMAND") | ftxui::bold | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 30)),
        ftxui::separator(),
        ftxui::notflex(ftxui::text("MEMORY") | ftxui::bold | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12)),
        ftxui::separator(),
        ftxui::notflex(ftxui::text("SHARED") | ftxui::bold | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12)),
        ftxui::separator(),
        ftxui::notflex(ftxui::text("ENGINES") | ftxui::bold | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 15))}));

    int shown = 0;
    for (size_t i = 0; i < procs.size() && shown < top_n; ++i) {
      const auto &p = procs[i];
      if (p.engines_count == 0)
        continue; // only show active processes in the summary

  auto mem = device->getMemoryState();
  (void)mem;

      process_detail.push_back(
    ftxui::hbox({ftxui::notflex(ftxui::text(std::to_string(p.pid)) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 8)),
    ftxui::separator(),
    ftxui::notflex(ftxui::text(ellipses(p.command, 30)) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 30)),
    ftxui::separator(),
    ftxui::notflex(ftxui::text(format_bytes(p.memSize)) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12)),
    ftxui::separator(),
    ftxui::notflex(ftxui::text(format_bytes(p.sharedSize)) |
      ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12)),
    ftxui::separator(),
    ftxui::notflex(ftxui::text(engine_flags_to_str(p.engines_mask)) |
      ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 15))}));

      ++shown;
    }

  return ftxui::vbox({ftxui::hbox(ftxui::text("📊 Process Summary") | ftxui::bold,
            ftxui::separator(), ftxui::text("Processes: "),
            ftxui::text(std::to_string((int)procs.size())),
            ftxui::separator(), ftxui::text("Total mem:"),
            ftxui::text(format_bytes(total_mem))),
         ftxui::vbox(std::move(process_detail))}) |
       ftxui::border;
  }

  // Full details view: render header then the requested slice of the sorted
  // process list using state.process_offset for paging.
  ftxui::Elements process_detail;
  process_detail.push_back(
    ftxui::hbox({
      ftxui::notflex(ftxui::text("PID") | ftxui::bold | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 8)), ftxui::separator(),
      ftxui::notflex(ftxui::text("COMMAND") | ftxui::bold | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 30)),
      ftxui::separator(), ftxui::notflex(ftxui::text("MEMORY") | ftxui::bold | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12)),
      ftxui::separator(), ftxui::notflex(ftxui::text("SHARED") | ftxui::bold | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12)),
      ftxui::separator(),
      ftxui::notflex(ftxui::text("ENGINES") | ftxui::bold | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 15))
    }));

  int start = state.process_offset;
  // clamp start to available range
  if (start < 0)
    start = 0;
  int end = std::min(start + visible_processes, (int)procs.size());

  for (int i = start; i < end; ++i) {
    const auto &p = procs[i];
  auto mem = device->getMemoryState();
  (void)mem;

    process_detail.push_back(
  ftxui::hbox({
      ftxui::notflex(ftxui::text(std::to_string(p.pid)) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 8)),
      ftxui::separator(),
      ftxui::notflex(ftxui::text(p.command) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 30)),
      ftxui::separator(),
      ftxui::notflex(ftxui::text(format_bytes(p.memSize)) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12)),
      ftxui::separator(),
      ftxui::notflex(ftxui::text(format_bytes(p.sharedSize)) |
        ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12)),
      ftxui::separator(),
      ftxui::notflex(ftxui::text(engine_flags_to_str(p.engines_mask)) |
        ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 15))
  }));
  }

  return ftxui::vbox({ftxui::text("📊 Process Details") | ftxui::bold,
       ftxui::vbox(std::move(process_detail))}) |
     ftxui::border;
}




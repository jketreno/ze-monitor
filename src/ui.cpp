
#include "ui.h"
#include "device.h"

ftxui::Element build_ui(Device *device, UIState &state, int screen_width, int screen_height, bool no_ansi) {
  (void)screen_height;
  // Auto refresh: update temperatures and processes if a second passed.
  auto now = std::chrono::steady_clock::now();
  if (now - state.refresh_time > std::chrono::seconds(1)) {
    state.refresh_time = now;
    device->updateTemperatures();
    device->updateProcesses();
  }

  ftxui::Elements main_content;

  // Header with device info
  auto mem = device->getMemoryState();
  double mem_usage_pct = 0.0;
  if (mem.size > 0) {
    mem_usage_pct = (1.0 - (double)mem.free / mem.size) * 100;
  }
  auto avg_temp = 0.0;
  if (device->getTemperatureCount() > 0) {
    for (uint32_t i = 0; i < device->getTemperatureCount(); ++i) {
      avg_temp += device->getTemperature(i);
    }
    avg_temp /= device->getTemperatureCount();
  }

  auto header =
      ftxui::vbox(
          {ftxui::hbox(
               {ftxui::text("🚀 ") | ftxui::color(ftxui::Color::Cyan),
                ftxui::text("ZE-MONITOR") | ftxui::bold |
                    ftxui::color(ftxui::Color::White),
                ftxui::text(" | ") | ftxui::color(ftxui::Color::GrayDark),
                ftxui::text(device->getDeviceProperties()->modelName) |
                    ftxui::color(ftxui::Color::Cyan),
                ftxui::text(" | ") | ftxui::color(ftxui::Color::GrayDark),
                ftxui::text("View: ") | ftxui::color(ftxui::Color::GrayDark),
                ftxui::text([&]() {
                  switch (state.view_mode) {
                  case ViewMode::OVERVIEW:
                    return "Overview";
                  case ViewMode::ENGINES:
                    return "Engines";
                  case ViewMode::PROCESSES:
                    return "Processes";
                  case ViewMode::POWER:
                    return "Power";
                  case ViewMode::THERMAL:
                    return "Thermal";
                  }
                  return "Unknown";
                }()) |
                    ftxui::bold | ftxui::color(ftxui::Color::Yellow)}),
           ftxui::hbox(
               {ftxui::notflex(ftxui::text("Memory: ") |
                               ftxui::color(ftxui::Color::White)),
                mem.size > 0
                    ? ftxui::xflex_grow(
                          ftxui::gauge(mem_usage_pct / 100.0) |
                          ftxui::color(get_percentage_color(mem_usage_pct)))
                    : ftxui::text("N/A"),
                mem.size > 0
                    ? ftxui::notflex(
                          ftxui::text(" " + std::to_string((int)mem_usage_pct) +
                                      "%") |
                          ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 5) |
                          ftxui::color(get_percentage_color(mem_usage_pct)))
                    : ftxui::text(""),
                mem.size > 0
                    ? ftxui::notflex(
                          ftxui::text("(" + format_bytes(mem.size - mem.free) +
                                      "/" + format_bytes(mem.size) + ")") |
                          ftxui::color(ftxui::Color::GrayDark))
                    : ftxui::text(""),
                ftxui::separator(),
                ftxui::text(" Temp: ") | ftxui::color(ftxui::Color::White),
                ftxui::text(std::to_string((int)avg_temp) + "°C") |
                    ftxui::color(get_temp_color(avg_temp))})}) |
      ftxui::size(ftxui::HEIGHT, ftxui::GREATER_THAN, 2) | ftxui::border |
      ftxui::color(ftxui::Color::Cyan) | ftxui::notflex;

  main_content.push_back(header | ftxui::notflex);

  switch (state.view_mode) {
  case ViewMode::OVERVIEW: {
    main_content.push_back(build_engine_details(
        device, state, screen_width, 15, "ALL$", "🔧 Engine Utilization"));
    main_content.push_back(
        build_process_details(device, state, screen_width, 15, true, 3));
    break;
  }
  case ViewMode::ENGINES: {
    main_content.push_back(
        build_engine_details(device, state, screen_width, 15));
    break;
  }
  case ViewMode::PROCESSES: {
    main_content.push_back(
        build_process_details(device, state, screen_width, 15));
    break;
  }
  case ViewMode::THERMAL: {
    ftxui::Elements thermal_detail;
    thermal_detail.push_back(
        ftxui::hbox({ftxui::text("SENSOR") | ftxui::bold |
                         ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 15),
                     ftxui::text(" "),
                     ftxui::text("TEMPERATURE") | ftxui::bold |
                         ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 20),
                     ftxui::text(" "),
                     ftxui::text("STATUS") | ftxui::bold |
                         ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 15),
                     ftxui::text(" "),
                     ftxui::text("GRAPH") | ftxui::bold |
                         ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 30)}) |
        ftxui::color(ftxui::Color::White));
    for (uint32_t i = 0; i < device->getTemperatureCount(); ++i) {
      auto temp = device->getTemperature(i);
      auto status = temp < 80 ? "NORMAL" : temp < 90 ? "WARM" : "HOT";
      auto status_color = temp < 80   ? ftxui::Color::Green
                          : temp < 90 ? ftxui::Color::Yellow
                                      : ftxui::Color::Red;
      double temp_ratio = std::min(temp / 100.0, 1.0);
      thermal_detail.push_back(
          ftxui::hbox({ftxui::text("Sensor " + std::to_string(i + 1)) |
                           ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 15) |
                           ftxui::color(ftxui::Color::Cyan),
                       ftxui::separator(),
                       ftxui::text(std::to_string((int)temp) + "°C") |
                           ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 20) |
                           ftxui::color(get_temp_color(temp)),
                       ftxui::separator(),
                       ftxui::text(status) |
                           ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 15) |
                           ftxui::color(status_color),
                       ftxui::separator(),
                       ftxui::xflex_grow(ftxui::gauge(temp_ratio) |
                                         ftxui::color(get_temp_color(temp)))}));
    }
    main_content.push_back(
        ftxui::vbox({ftxui::text("🌡️  Thermal Monitoring") | ftxui::bold |
                         ftxui::color(ftxui::Color::Green),
                     ftxui::vbox(std::move(thermal_detail))}) |
        ftxui::border);
    break;
  }
  case ViewMode::POWER: {
    ftxui::Elements power_detail;
    power_detail.push_back(
        ftxui::hbox({ftxui::text("DOMAIN") | ftxui::bold |
                         ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 9),
                     ftxui::separator(),
                     ftxui::text("POWER") | ftxui::bold |
                         ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 5),
                     ftxui::separator(),
                     ftxui::text("ENERGY") | ftxui::bold |
                         ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 6),
                     ftxui::separator(),
                     ftxui::text("CONTROL") | ftxui::bold |
                         ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 7),
                     ftxui::separator(),
                     ftxui::text("SUB-DEV") | ftxui::bold |
                         ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 10)}) |
        ftxui::color(ftxui::Color::White));
    for (uint32_t i = 0; i < device->getPowerDomainCount(); ++i) {
      auto power_domain = device->getPowerDomain(i);
      auto properties = power_domain->getPowerDomainProperties();
      auto energy = power_domain->getPowerDomainEnergy();
      power_detail.push_back(ftxui::hbox(
          {ftxui::text("Domain " + std::to_string(i + 1)) |
               ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 9) |
               ftxui::color(ftxui::Color::Cyan),
           ftxui::separator(),
           ftxui::text(energy >= 0 ? (std::to_string((int)energy) + "W")
                                   : "N/A") |
               ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 5) |
               ftxui::color(ftxui::Color::Yellow),
           ftxui::separator(),
           ftxui::text("N/A") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 6) |
               ftxui::color(ftxui::Color::GrayDark),
           ftxui::separator(),
           ftxui::text(properties->canControl ? "YES" : "NO") |
               ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 7) |
               ftxui::color(properties->canControl ? ftxui::Color::Green
                                                   : ftxui::Color::Red),
           ftxui::separator(),
           ftxui::text(properties->onSubdevice
                           ? std::to_string(properties->subdeviceId)
                           : "N/A") |
               ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 10) |
               ftxui::color(ftxui::Color::GrayDark)}));
    }
    if (device->getPSUCount() > 0) {
      power_detail.push_back(ftxui::separator());
      power_detail.push_back(ftxui::text("Power Supply Units:") | ftxui::bold |
                             ftxui::color(ftxui::Color::White));
      for (uint32_t i = 0; i < device->getPSUCount(); ++i) {
        auto psu = device->getPSU(i);
        auto properties = psu->getPSUProperties();
        power_detail.push_back(ftxui::hbox(
            {ftxui::text("PSU " + std::to_string(i + 1)) |
                 ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 15) |
                 ftxui::color(ftxui::Color::Cyan),
             ftxui::text("Fan: " +
                         std::string(properties->haveFan ? "YES" : "NO")) |
                 ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 15) |
                 ftxui::color(properties->haveFan ? ftxui::Color::Green
                                                  : ftxui::Color::Red),
             ftxui::text("Limit: " + std::to_string(properties->ampLimit) +
                         "A") |
                 ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 15) |
                 ftxui::color(ftxui::Color::Yellow),
             ftxui::text("") | ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 10),
             ftxui::text(properties->onSubdevice
                             ? std::to_string(properties->subdeviceId)
                             : "N/A") |
                 ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 10) |
                 ftxui::color(ftxui::Color::GrayDark)}));
      }
    }
    main_content.push_back(
        ftxui::vbox({ftxui::text("⚡ Power Management") | ftxui::bold |
                         ftxui::color(ftxui::Color::Green),
                     ftxui::vbox(std::move(power_detail))}) |
        ftxui::border);
    break;
  }
  }
  // Key hints: only include when stdout is a terminal and ANSI is enabled
  if (isatty(fileno(stdout)) && !no_ansi) {
    ftxui::Elements key_hints;
    if (state.show_help) {
      key_hints = {
          ftxui::text("📋 Key Bindings:") | ftxui::bold |
              ftxui::color(ftxui::Color::White),
          ftxui::hbox(
              {ftxui::text("1-5") | ftxui::color(ftxui::Color::Yellow),
               ftxui::text(": Switch views  ") |
                   ftxui::color(ftxui::Color::GrayDark),
               ftxui::text("↑↓") | ftxui::color(ftxui::Color::Yellow),
               ftxui::text(": Scroll  ") | ftxui::color(ftxui::Color::GrayDark),
               ftxui::text("h") | ftxui::color(ftxui::Color::Yellow),
               ftxui::text(": Toggle help  ") |
                   ftxui::color(ftxui::Color::GrayDark),
               ftxui::text("q/ESC") | ftxui::color(ftxui::Color::Yellow),
               ftxui::text(": Quit") | ftxui::color(ftxui::Color::GrayDark)}),
          ftxui::text(
              "Views: 1=Overview 2=Engines 3=Processes 4=Power 5=Thermal") |
              ftxui::color(ftxui::Color::GrayDark)};
    } else {
      key_hints = {ftxui::hbox(
          {ftxui::text("Views: ") | ftxui::color(ftxui::Color::GrayDark),
           ftxui::text("1") | ftxui::color(ftxui::Color::Yellow),
           ftxui::text("=Overview ") | ftxui::color(ftxui::Color::GrayDark),
           ftxui::text("2") | ftxui::color(ftxui::Color::Yellow),
           ftxui::text("=Engines ") | ftxui::color(ftxui::Color::GrayDark),
           ftxui::text("3") | ftxui::color(ftxui::Color::Yellow),
           ftxui::text("=Processes ") | ftxui::color(ftxui::Color::GrayDark),
           ftxui::text("4") | ftxui::color(ftxui::Color::Yellow),
           ftxui::text("=Power ") | ftxui::color(ftxui::Color::GrayDark),
           ftxui::text("5") | ftxui::color(ftxui::Color::Yellow),
           ftxui::text("=Thermal ") | ftxui::color(ftxui::Color::GrayDark),
           ftxui::text("| ") | ftxui::color(ftxui::Color::GrayDark),
           ftxui::text("↑↓") | ftxui::color(ftxui::Color::Yellow),
           ftxui::text("=Scroll ") | ftxui::color(ftxui::Color::GrayDark),
           ftxui::text("h") | ftxui::color(ftxui::Color::Yellow),
           ftxui::text("=Help ") | ftxui::color(ftxui::Color::GrayDark),
           ftxui::text("q") | ftxui::color(ftxui::Color::Yellow),
           ftxui::text("=Quit") | ftxui::color(ftxui::Color::GrayDark)})};
    }
    main_content.push_back(
        ftxui::notflex(ftxui::vbox(std::move(key_hints))) |
        ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, state.show_help ? 3 : 1) |
        ftxui::notflex | ftxui::border | ftxui::color(ftxui::Color::Blue));
  }
  ftxui::Element root = ftxui::vbox(std::move(main_content));
  return root;
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
  const char *suffixes[] = {"B", "KB", "MB", "GB", "TB"};
  int i = 0;
  double count = bytes;
  while (count >= 1024 && i < 4) {
    count /= 1024;
    ++i;
  }
  char buf[32];
  snprintf(buf, sizeof(buf), "%.1f %s", count, suffixes[i]);
  return buf;
}

// Color for percentage (green/yellow/red)
ftxui::Color get_percentage_color(double percentage) {
  if (percentage < 30)
    return ftxui::Color::Green;
  if (percentage < 60)
    return ftxui::Color::Yellow;
  if (percentage < 80)
    return ftxui::Color::RGB(255, 215, 0);
  return ftxui::Color::Red;
}

// Color for temperature (green/yellow/red)
ftxui::Color get_temp_color(double temp) {
  if (temp < 50)
    return ftxui::Color::Cyan;
  if (temp < 70)
    return ftxui::Color::Green;
  if (temp < 80)
    return ftxui::Color::Yellow;
  if (temp < 90)
    return ftxui::Color::RGB(255, 215, 0);
  return ftxui::Color::Red;
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




/*

For sysman API information, see:

https://oneapi-src.github.io/level-zero-spec/level-zero/latest/sysman/PROG.html#sysman-programming-guide

and

https://github.com/intel/compute-runtime/tree/master/programmers-guide

*/

#include "args.h"
#include "device_helpers.h"
#include "helpers.h"
#include "monitor.h"

void copyright() {
  printf("ze-monitor: A small Level Zero Sysman GPU monitor utility\n");
  printf("Copyright (C) 2025 James Ketrenos\n");
}

#ifndef APP_VERSION
#define APP_VERSION "unknown"
#endif
void version() { printf("Version: %s\n", APP_VERSION); }

void usage() {
  const uint32_t indent = 2;
  const uint32_t option_len = 12;
  const char *options[][2] = {
      {"device ID",
       "Device ID to query. Can accept #, BDF, PCI-ID, /dev/dri/*."},
      {"help", "This text."},
      {"mode <view>",
       "Set initial view mode: overview|engines|processes|power|thermal."},
      {"no-ansi", "Disable ANSI/colors and force unicode box output."},
      {"wait-time <ms>",
       "Wait time in milliseconds for sensor polling (default 1100)."},
      {"info", "Show additional details about device."},
      {"version", "Version info."},
      {nullptr, nullptr}};
  printf("\n");
  printf("usage: ze-monitor [OPTIONS]\n");
  printf("\n");
  for (uint32_t i = 0; options[i][0] != nullptr; ++i) {
    printf("%*s --%-*s %s\n", indent, "", option_len, options[i][0],
           options[i][1]);
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
  ParsedArgs parsed = parse_args(argc, argv);
  bool showInfo = parsed.showInfo;
  bool listDevices = parsed.listDevices;
  // bool one_shot = parsed.one_shot;
  bool no_ansi = parsed.no_ansi;
  std::string start_mode = parsed.start_mode;
  int wait_ms = parsed.wait_ms;
  arg_search_t argSearch = parsed.argSearch;

  if (zesInit(0) != ZE_RESULT_SUCCESS) {
    printf("Can't initialize the API\n");
    return -1;
  }

  std::vector<std::unique_ptr<Device>> devices = get_devices();

  Device *device = nullptr;
  int32_t index = -1;
  if (argSearch.type != INVALID) {
    index = find_device_index(argSearch, devices);
    if (index != -1) {
      device = devices[index].get();
    } else {
      printf("--device %s not found.\n", argSearch.match.c_str());
      list_devices(devices);
      exit(-1);
    }
  }

  if (device == nullptr && !showInfo) {
    listDevices = true;
  }

  if (listDevices) {
    list_devices(devices);
    return 0;
  }

  // If --info was requested, either show the single --device or if no device
  // was provided, list all devices.
  if (showInfo) {
    if (device != nullptr) {
      show_device_properties(device);
      show_device_memory(device);
      show_engine_groups(device);
      show_temperatures(device);
      show_power_domains(device);
      show_psus(device);
    } else {
      for (uint32_t i = 0; i < devices.size(); ++i) {
        device = devices[i].get();

        printf("Device %d: %04X:%04X (%s)\n", i + 1,
               device->getDeviceProperties()->core.vendorId,
               device->getDeviceProperties()->core.deviceId,
               device->getDeviceProperties()->modelName);

        show_device_properties(device);
        show_device_memory(device);
        show_engine_groups(device);
        show_temperatures(device);
        show_power_domains(device);
        show_psus(device);
      }
    }
    return 0;
  }

  // Monitoring loop and UI
  UIState state;
  MonitorConfig config{device,  &state,     wait_ms,
                       no_ansi, start_mode, parsed.one_shot};
  return run_monitor(config);
}
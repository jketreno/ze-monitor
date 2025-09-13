
#include "args.h"
#include <cstdlib>
#include <iostream>

ParsedArgs parse_args(int argc, char *argv[]) {
  ParsedArgs parsed;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "--device" && i + 1 < argc) {
      std::string device_arg = argv[i + 1];
      parsed.argSearch = process_device_argument(device_arg);
      if (parsed.argSearch.type == INVALID) {
        std::cerr << "Invalid argument: " << arg << std::endl;
        exit(-1);
      }
      i++;
      parsed.listDevices = false;
    } else if (arg == "--info") {
      parsed.showInfo = true;
    } else if (arg == "--one-shot") {
      parsed.one_shot = true;
    } else if (arg == "--mode" && i + 1 < argc) {
      parsed.start_mode = argv[i + 1];
      i++;
    } else if (arg == "--no-ansi") {
      parsed.no_ansi = true;
    } else if (arg == "--wait-time" && i + 1 < argc) {
      parsed.wait_ms = atoi(argv[i + 1]);
      i++;
    } else if (arg == "--list") {
      parsed.listDevices = true;
    } else if (arg == "--version") {
      extern void copyright();
      extern void version();
      copyright();
      version();
      exit(0);
    } else if (arg == "--help") {
      extern void copyright();
      extern void usage();
      copyright();
      usage();
      exit(0);
    } else {
      std::cerr << "Unknown argument: " << arg << std::endl;
      exit(1);
    }
  }
  return parsed;
}
#include "args.h"

arg_search_t process_device_argument(const std::string &device_arg) {
  // Define the two possible regex patterns
  std::regex uuid_device_regex(
      "^([0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-"
      "f]{12})$"); // Matches uuid format
  std::regex pciid_device_regex(
      "^([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})$"); // Matches pci id (vendor:device)
                                              // format
  std::regex bdf_regex(
      "^([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})$");
  std::regex index_regex("^[0-9]+$");
  std::smatch matches;

  arg_search_t search;

  if (std::regex_match(device_arg, matches, index_regex)) {
    search.type = INDEX;
    search.data = (uint32_t)std::stoul(matches[0], nullptr, 10);
    search.match = matches[0];
    return search;
  }

  if (std::regex_match(device_arg, matches, bdf_regex)) {
    search.type = BDF;
    bdf_t v;
    v.domain = std::stoul(matches[1], nullptr, 16);
    v.bus = std::stoul(matches[2], nullptr, 16);
    v.device = std::stoul(matches[3], nullptr, 16);
    v.function = std::stoul(matches[4], nullptr, 16);
    search.data = v;
    search.match = matches[0];
    return search;
  }

  if (std::regex_match(device_arg, matches, pciid_device_regex)) {
    search.type = PCIID;
    pciid_t v;
    v.vendor = std::stoul(matches[1], nullptr, 16);
    v.device = std::stoul(matches[2], nullptr, 16);
    search.data = v;
    search.match = matches[0];
    return search;
  }

  if (std::regex_match(device_arg, matches, uuid_device_regex)) {
    search.type = UUID;
    search.data = uuid_from_string(matches[1]);
    search.match = matches[0];
    return search;
  }

  if (device_arg.length() != 0) {
    pciid_t pciid = get_pci_id_for_render_node(device_arg);
    if (pciid.device != 0 && pciid.vendor != 0) {
      search.type = PCIID;
      search.data = pciid;
      search.match = matches[0];
      return search;
    }
  }

  search.type = INVALID;
  return search;
}

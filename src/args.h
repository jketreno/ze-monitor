
#pragma once

#include "helpers.h"
#include <string>
#include <variant>

typedef enum arg_enum { PCIID, UUID, RENDERID, BDF, INDEX, INVALID } arg_type_t;

class arg_search_t {
public:
  arg_search_t() : type(INVALID) {}

  std::variant<zes_uuid_t, pciid_t, std::string, bdf_t, uint32_t> data;
  std::string match;
  arg_type_t type;
};

arg_search_t process_device_argument(const std::string &device_arg);

struct ParsedArgs {
  bool showInfo = false;
  bool listDevices = true;
  bool one_shot = false;
  bool no_ansi = false;
  std::string start_mode;
  int wait_ms = 1100;
  arg_search_t argSearch;
};

ParsedArgs parse_args(int argc, char *argv[]);

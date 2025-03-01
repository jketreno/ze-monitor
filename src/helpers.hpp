#ifndef __helpers_cpp__
#define __helpers_cpp__
#include <fmt/core.h>
#include <level_zero/zes_api.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <array> // Optional: If using std::array for UUID representation
#include <bitset>
#include <fstream>
#include <iomanip> // For std::setw and std::setfill
#include <iostream>
#include <regex>
#include <sstream> // For std::ostringstream
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

typedef enum
{
    justify_left,
    justify_right
} justify_dir_t;

std::string fit_label(const std::string &label, uint32_t max_width, justify_dir_t justify);
void draw_utilization_bar(int width, double utilization, const std::string &label);

typedef struct
{
    uint32_t vendor;
    uint32_t device;
} pciid_t;

template <typename T> std::string to_binary_string(T value);
std::string uuid_to_string(const zes_uuid_t *uuid);
std::string pciid_to_string(const pciid_t *pciid);
zes_uuid_t uuid_from_string(const std::string &str);
const char *ze_error_to_str(ze_result_t ret);
const char *engine_type_to_str(zes_engine_group_t type);
std::string engine_flags_to_str(zes_engine_type_flags_t flags);
#endif
#pragma once

#include <fmt/core.h>
#include <level_zero/zes_api.h>
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
    justify_middle,
    justify_right
} justify_dir_t;

std::string fit_label(const std::string &label, uint32_t max_width, justify_dir_t justify);

typedef struct
{
    uint32_t vendor;
    uint32_t device;
} pciid_t;

typedef struct
{
    uint32_t domain;
    uint32_t bus;
    uint32_t device;
    uint32_t function;
} bdf_t;

template <typename T> std::string to_binary_string(T value);
std::string uuid_to_string(const zes_uuid_t *uuid);
std::string pciid_to_string(const pciid_t *pciid);
zes_uuid_t uuid_from_string(const std::string &str);
const char *ze_error_to_str(ze_result_t ret);
const char *engine_type_to_str(zes_engine_group_t type);
const char *voltage_status_to_str(zes_psu_voltage_status_t type);
std::string engine_flags_to_str(zes_engine_type_flags_t flags);
pciid_t get_pci_id_for_render_node(const std::string &render_path);

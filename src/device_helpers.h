#pragma once
#include <vector>
#include <memory>
#include "device.h"
#include "engine.h"
#include "args.h"

// Returns a vector of all discovered devices
std::vector<std::unique_ptr<Device>> get_devices();

// Prints a summary of all devices
ze_result_t list_devices(std::vector<std::unique_ptr<Device>> &devices);

// Finds a device index by search criteria
int32_t find_device_index(arg_search_t search, std::vector<std::unique_ptr<Device>> &devices);

// Device info helpers
void show_device_properties(const Device *device);
void show_device_memory(Device *device);
void show_engine_group_properties(const Engine *engine);
void show_engine_groups(const Device *device);
void show_power_domains(const Device *device);
void show_psus(const Device *device);
void show_temperatures(Device *device);

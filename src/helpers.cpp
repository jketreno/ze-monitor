#include <fmt/core.h>
#include <level_zero/zes_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "helpers.hpp"

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

std::string fit_label(const std::string &label, uint32_t max_width, justify_dir_t justify)
{
    if (label.length() > max_width)
    {
        switch (justify)
        {
        case justify_left:
            return label.substr(0, max_width - 3) + "...";
        case justify_right:
            return "..." + label.substr(label.length() - (max_width - 3), max_width - 3);
        }
    }
    return label;
}

void draw_utilization_bar(int width, double utilization, const std::string &label)
{
    int utilization_width = (width * 0.75) - 2; // 75% for the progress bar with []
    int label_width = width * 0.25 - 1;         // 25% for the label with one space

    // Draw the label (left-aligned, ellipsis if necessary)
    std::string truncated_label = fit_label(label, label_width, justify_right);

    printw("%-.*s", label_width, truncated_label.c_str());

    // Draw the progress bar (75% of the terminal width)
    printw(" [");
    int pos = utilization_width * utilization / 100;
    for (int i = 0; i < utilization_width; ++i)
    {
        if (i == 1)
        {
            printw(" %3d%% ", (uint32_t)utilization);
        }
        else if (i > 1 && i <= 6)
        {
            /* do nothing in % label */
        }
        else if (i < pos)
        {
            printw("#");
        }
        else
        {
            printw(" ");
        }
    }
    printw("]");
}

template <typename T> std::string to_binary_string(T value)
{
    return std::bitset<sizeof(T) * 8>(value).to_string();
}

std::string uuid_to_string(const zes_uuid_t *uuid)
{
    std::ostringstream oss;

    for (uint32_t i = 0; i < ZES_MAX_UUID_SIZE; ++i)
    {
        // Append each byte as two hexadecimal digits
        oss << std::setw(2) << std::setfill('0') << std::hex << (uint32_t)uuid->id[i];

        // Append dashes at appropriate positions
        switch (i)
        {
        case 3:
        case 5:
        case 7:
        case 9:
            oss << "-";
            break;
        default:
            break;
        }
    }

    std::string uuid_str = oss.str();

    // Convert the resulting string to uppercase
    for (auto &c : uuid_str)
    {
        c = std::toupper(static_cast<unsigned char>(c));
    }

    return uuid_str;
}

std::string pciid_to_string(const pciid_t *pciid)
{
    char buffer[10]; // Enough space for 4 digits vendor, 1 colon, and 4 digits device (total 9 characters)
    std::snprintf(buffer, sizeof(buffer), "%04X:%04X", pciid->vendor, pciid->device);
    return std::string(buffer);
}

zes_uuid_t uuid_from_string(const std::string &str)
{
    zes_uuid_t v;
    std::istringstream iss(str);
    int i;
    char c;

    // Read the UUID string into the byte array
    for (i = 0; i < 16; ++i)
    {
        std::string byte_str;

        // Read exactly 2 characters (one byte)
        iss >> std::setw(2) >> byte_str;

        // Convert the byte from string to integer (hexadecimal to decimal)
        unsigned int byte = std::stoi(byte_str, nullptr, 16);

        // Store the byte into the array
        v.id[i] = static_cast<uint8_t>(byte);

        // Skip the dash if present
        if (iss.peek() == '-')
        {
            iss >> c;
        }
    }

    return v;
}

const char *ze_error_to_str(ze_result_t ret)
{
// Define a macro to prevent having to type each code twice...
#define type_to_str(X) \
    case X:            \
        return #X;     \
        break

    switch (ret)
    {
        type_to_str(ZE_RESULT_SUCCESS);
        type_to_str(ZE_RESULT_ERROR_UNINITIALIZED);
        type_to_str(ZE_RESULT_ERROR_DEVICE_LOST);
        type_to_str(ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY);
        type_to_str(ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY);
        type_to_str(ZE_RESULT_ERROR_INVALID_NULL_HANDLE);
        type_to_str(ZE_RESULT_ERROR_INVALID_NULL_POINTER);
        type_to_str(ZE_RESULT_ERROR_INVALID_SIZE);
        type_to_str(ZE_RESULT_ERROR_MODULE_BUILD_FAILURE);
        type_to_str(ZE_RESULT_ERROR_MODULE_LINK_FAILURE);
        type_to_str(ZE_RESULT_ERROR_DEVICE_REQUIRES_RESET);
        type_to_str(ZE_RESULT_ERROR_DEVICE_IN_LOW_POWER_STATE);
        type_to_str(ZE_RESULT_EXP_ERROR_DEVICE_IS_NOT_VERTEX);
        type_to_str(ZE_RESULT_EXP_ERROR_VERTEX_IS_NOT_DEVICE);
        type_to_str(ZE_RESULT_EXP_ERROR_REMOTE_DEVICE);
        type_to_str(ZE_RESULT_EXP_ERROR_OPERANDS_INCOMPATIBLE);
        type_to_str(ZE_RESULT_EXP_RTAS_BUILD_RETRY);
        type_to_str(ZE_RESULT_EXP_RTAS_BUILD_DEFERRED);
        type_to_str(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS);
        type_to_str(ZE_RESULT_ERROR_NOT_AVAILABLE);
        type_to_str(ZE_RESULT_ERROR_DEPENDENCY_UNAVAILABLE);
        type_to_str(ZE_RESULT_WARNING_DROPPED_DATA);
        type_to_str(ZE_RESULT_ERROR_UNSUPPORTED_VERSION);
        type_to_str(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
        type_to_str(ZE_RESULT_ERROR_INVALID_ARGUMENT);
        type_to_str(ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE);
        type_to_str(ZE_RESULT_ERROR_UNSUPPORTED_SIZE);
        type_to_str(ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT);
        type_to_str(ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT);
        type_to_str(ZE_RESULT_ERROR_INVALID_ENUMERATION);
        type_to_str(ZE_RESULT_ERROR_UNSUPPORTED_ENUMERATION);
        type_to_str(ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT);
        type_to_str(ZE_RESULT_ERROR_INVALID_NATIVE_BINARY);
        type_to_str(ZE_RESULT_ERROR_INVALID_GLOBAL_NAME);
        type_to_str(ZE_RESULT_ERROR_INVALID_KERNEL_NAME);
        type_to_str(ZE_RESULT_ERROR_INVALID_FUNCTION_NAME);
        type_to_str(ZE_RESULT_ERROR_INVALID_GROUP_SIZE_DIMENSION);
        type_to_str(ZE_RESULT_ERROR_INVALID_GLOBAL_WIDTH_DIMENSION);
        type_to_str(ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX);
        type_to_str(ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE);
        type_to_str(ZE_RESULT_ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE);
        type_to_str(ZE_RESULT_ERROR_INVALID_MODULE_UNLINKED);
        type_to_str(ZE_RESULT_ERROR_INVALID_COMMAND_LIST_TYPE);
        type_to_str(ZE_RESULT_ERROR_OVERLAPPING_REGIONS);
        type_to_str(ZE_RESULT_WARNING_ACTION_REQUIRED);
        type_to_str(ZE_RESULT_ERROR_INVALID_KERNEL_HANDLE);
        type_to_str(ZE_RESULT_ERROR_UNKNOWN);
        type_to_str(ZE_RESULT_FORCE_UINT32);
    default:
        return "UNKNOWN";
        break;
    }
#undef type_to_str
}

const char *engine_type_to_str(zes_engine_group_t type)
{
// Define a macro to prevent having to type each code twice...
#define type_to_str(X) \
    case X:            \
        return #X;     \
        break

    switch (type)
    {
        type_to_str(ZES_ENGINE_GROUP_ALL);
        type_to_str(ZES_ENGINE_GROUP_COMPUTE_ALL);
        type_to_str(ZES_ENGINE_GROUP_MEDIA_ALL);
        type_to_str(ZES_ENGINE_GROUP_COPY_ALL);
        type_to_str(ZES_ENGINE_GROUP_COMPUTE_SINGLE);
        type_to_str(ZES_ENGINE_GROUP_RENDER_SINGLE);
        type_to_str(ZES_ENGINE_GROUP_MEDIA_DECODE_SINGLE);
        type_to_str(ZES_ENGINE_GROUP_MEDIA_ENCODE_SINGLE);
        type_to_str(ZES_ENGINE_GROUP_COPY_SINGLE);
        type_to_str(ZES_ENGINE_GROUP_MEDIA_ENHANCEMENT_SINGLE);
        type_to_str(ZES_ENGINE_GROUP_3D_SINGLE);
        type_to_str(ZES_ENGINE_GROUP_3D_RENDER_COMPUTE_ALL);
        type_to_str(ZES_ENGINE_GROUP_RENDER_ALL);
        type_to_str(ZES_ENGINE_GROUP_3D_ALL);
        type_to_str(ZES_ENGINE_GROUP_MEDIA_CODEC_SINGLE);
    default:
        return "UNKNOWN";
        break;
    }
#undef type_to_str
}


std::string engine_flags_to_str(zes_engine_type_flags_t flags)
{
    std::ostringstream oss;
    // Map of flag to string representation
    std::unordered_map<zes_engine_type_flags_t, const char *> flag_map = {
        {ZES_ENGINE_TYPE_FLAG_OTHER, "OTHER"},
        {ZES_ENGINE_TYPE_FLAG_COMPUTE, "COMPUTE"},
        {ZES_ENGINE_TYPE_FLAG_3D, "3D"},
        {ZES_ENGINE_TYPE_FLAG_MEDIA, "MEDIA"},
        {ZES_ENGINE_TYPE_FLAG_DMA, "DMA"},
        {ZES_ENGINE_TYPE_FLAG_RENDER, "RENDER"}};

    bool first = true;
    for (const auto &[flag, name] : flag_map)
    {
        if ((flags & flag) == flag)
        {
            if (!first)
            {
                oss << " ";
            }
            oss << name;
            first = false;
        }
    }

    return oss.str();
}


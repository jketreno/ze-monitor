#pragma once

#include "helpers.h"

typedef enum arg_enum
{
    PCIID,
    UUID,
    RENDERID,
    BDF,
    INDEX,
    INVALID
} arg_type_t;

class arg_search_t
{
public:
    arg_search_t() : type(INVALID) {}

    // Define a variant to store one of the types
    std::variant<zes_uuid_t, pciid_t, std::string, bdf_t, uint32_t> data;
    std::string match;
    arg_type_t type;
};

arg_search_t process_device_argument(const std::string &device_arg);


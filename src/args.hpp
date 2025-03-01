#ifndef __args_hpp__
#define __args_hpp__

#include "helpers.hpp"

typedef enum arg_enum
{
    PCIID,
    UUID,
    RENDERID,
    INVALID
} arg_type_t;

class arg_search_t
{
public:
    arg_search_t() : type(INVALID) {}

    // Define a variant to store one of the types
    std::variant<zes_uuid_t, pciid_t, std::string> data;

    arg_type_t type;
};

arg_search_t process_device_argument(const std::string &device_arg);

#endif
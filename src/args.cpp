#include "args.h"

arg_search_t process_device_argument(const std::string &device_arg)
{
    // Define the two possible regex patterns
    std::regex uuid_device_regex("^([0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12})$"); // Matches uuid format
    std::regex pciid_device_regex("^([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})$");                                            // Matches pci id (vendor:device) format
    std::regex render_device_regex("^/dev/dri/renderD([0-9]+)$");                                                    // Matches /dev/dri/renderD<digits> format
    std::regex index_regex("^[0-9]+$");
    std::smatch matches;

    arg_search_t search;

    if (std::regex_match(device_arg, matches, index_regex))
    {
        search.type = INDEX;
        search.data = (uint32_t)std::stoul(matches[0], nullptr, 10);
        return search;
    }
    else if (std::regex_match(device_arg, matches, pciid_device_regex))
    {
        search.type = PCIID;
        pciid_t v;
        v.vendor = std::stoul(matches[1], nullptr, 16);
        v.device = std::stoul(matches[2], nullptr, 16);
        search.data = v;
        return search;
    }
    else if (std::regex_match(device_arg, matches, render_device_regex))
    {
        search.type = RENDERID;
        search.data = device_arg;
        return search;
    }
    if (std::regex_match(device_arg, matches, uuid_device_regex))
    {
        search.type = UUID;
        search.data = uuid_from_string(matches[1]);
        return search;
    }
    else
    {
        search.type = INVALID;
        return search;
    }
}


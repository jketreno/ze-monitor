#include "args.h"

arg_search_t process_device_argument(const std::string &device_arg)
{
    // Define the two possible regex patterns
    std::regex uuid_device_regex("^([0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12})$"); // Matches uuid format
    std::regex pciid_device_regex("^([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})$");                                            // Matches pci id (vendor:device) format
    std::regex bdf_regex("^([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})$");
    std::regex index_regex("^[0-9]+$");
    std::smatch matches;

    arg_search_t search;

    if (std::regex_match(device_arg, matches, index_regex))
    {
        search.type = INDEX;
        search.data = (uint32_t)std::stoul(matches[0], nullptr, 10);
        search.match = matches[0];
        return search;
    }

    if (std::regex_match(device_arg, matches, bdf_regex))
    {
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

    if (std::regex_match(device_arg, matches, pciid_device_regex))
    {
        search.type = PCIID;
        pciid_t v;
        v.vendor = std::stoul(matches[1], nullptr, 16);
        v.device = std::stoul(matches[2], nullptr, 16);
        search.data = v;
        search.match = matches[0];
        return search;
    }

    if (std::regex_match(device_arg, matches, uuid_device_regex))
    {
        search.type = UUID;
        search.data = uuid_from_string(matches[1]);
        search.match = matches[0];
        return search;
    }

    if (device_arg.length() != 0)
    {
        pciid_t pciid = get_pci_id_for_render_node(device_arg);
        if (pciid.device != 0 && pciid.vendor != 0)
        {
            search.type = PCIID;
            search.data = pciid;
            search.match = matches[0];
            return search;
        }
    }

    search.type = INVALID;
    return search;
}


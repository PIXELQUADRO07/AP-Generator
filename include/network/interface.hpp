#pragma once

#include <string>

namespace apm::network {

class InterfaceManager {
public:
    static bool set_up(const std::string& if_name);
    static bool set_down(const std::string& if_name);
    static bool assign_ip(const std::string& if_name, const std::string& ip_with_prefix);
    static bool flush_ip(const std::string& if_name);
    static bool enable_ip_forwarding();
    static bool disable_ip_forwarding();
    static std::string get_ip(const std::string& if_name);
    static std::string get_mac(const std::string& if_name);
};

} // namespace apm::network

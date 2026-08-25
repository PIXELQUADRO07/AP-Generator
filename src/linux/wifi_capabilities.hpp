#pragma once

#include <string>
#include "apmanager/core/types.hpp"

namespace apm::linux_backend {

class WifiCapabilities {
public:
    static bool inspect_interface(const std::string& if_name, WifiInterface& iface);
    static std::string detect_driver(const std::string& if_name);
    static std::string detect_phy(const std::string& if_name);
    static std::string detect_ip_address(const std::string& if_name);
    static bool detect_ap_support(const std::string& phy_name, const std::string& driver);
    static void detect_bands_and_channels(const std::string& phy_name, WifiInterface& iface);
};

} // namespace apm::linux_backend

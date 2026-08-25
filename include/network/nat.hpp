#pragma once

#include <string>

namespace apm::network {

struct NatConfig {
    std::string ap_interface;
    std::string upstream_interface;
    bool enable_portal_redirect = false;
    int portal_port = 8080;
};

class NatManager {
public:
    static bool enable_nat(const NatConfig& config);
    static bool disable_nat(const NatConfig& config);
};

} // namespace apm::network

#pragma once

#include <string>

namespace apm::linux_backend {

class Firewall {
public:
    static bool apply_masquerade(const std::string& upstream_iface, const std::string& ap_iface);
    static bool remove_masquerade(const std::string& upstream_iface, const std::string& ap_iface);

    static bool apply_portal_redirect(const std::string& ap_iface, int portal_port = 8080);
    static bool remove_portal_redirect(const std::string& ap_iface, int portal_port = 8080);

    static bool authorize_client(const std::string& client_ip);
    static bool deauthorize_client(const std::string& client_ip);

    static bool flush_ap_rules(const std::string& ap_iface, const std::string& upstream_iface);
};

} // namespace apm::linux_backend

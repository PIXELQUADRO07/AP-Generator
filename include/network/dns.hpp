#pragma once

#include <string>
#include <vector>

namespace apm::network {

struct DnsConfig {
    std::string interface;
    std::string listen_ip = "192.168.50.1";
    bool captive_portal_dns = false;
    std::string captive_redirect_ip = "192.168.50.1";
    std::vector<std::string> upstream_servers = {"8.8.8.8", "1.1.1.1"};
};

class DnsServer {
public:
    DnsServer() = default;
    ~DnsServer() = default;

    static std::string generate_dns_config(const DnsConfig& config);
};

} // namespace apm::network

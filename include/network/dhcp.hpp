#pragma once

#include <string>
#include <vector>
#include "apmanager/core/types.hpp"

namespace apm::network {

struct DhcpLease {
    std::string mac;
    std::string ip;
    std::string hostname;
    std::string expiry;
};

struct DhcpConfig {
    std::string interface;
    std::string gateway_ip = "192.168.50.1";
    std::string netmask = "255.255.255.0";
    std::string range_start = "192.168.50.10";
    std::string range_end = "192.168.50.250";
    std::string lease_time = "12h";
};

class DhcpServer {
public:
    DhcpServer() = default;
    ~DhcpServer();

    bool start(const DhcpConfig& config, const std::string& lease_file = "config/run/dnsmasq.leases");
    bool stop();
    bool is_running() const;

    std::vector<DhcpLease> get_leases(const std::string& lease_file = "config/run/dnsmasq.leases") const;

private:
    int pid_ = -1;
    std::string config_path_;
};

} // namespace apm::network

#pragma once

#include <string>
#include <vector>
#include "apmanager/core/types.hpp"
#include "network/dhcp.hpp"

namespace apm::linux_backend {

class LinuxNetworkService {
public:
    LinuxNetworkService() = default;
    ~LinuxNetworkService();

    bool setup_ap_network(const AccessPointConfig& config, std::string* error_msg = nullptr);
    bool teardown_ap_network(const AccessPointConfig& config, std::string* error_msg = nullptr);

    std::vector<ClientInfo> get_clients() const;

private:
    network::DhcpServer dhcp_server_;
    AccessPointConfig last_config_;
};

} // namespace apm::linux_backend

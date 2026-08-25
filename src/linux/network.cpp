#include "network.hpp"
#include "firewall.hpp"
#include "client_monitor.hpp"
#include "network/interface.hpp"
#include "network/nat.hpp"

#include <iostream>

namespace apm::linux_backend {

LinuxNetworkService::~LinuxNetworkService() {
    // Ensure cleanup
    if (!last_config_.interface.empty()) {
        teardown_ap_network(last_config_);
    }
}

bool LinuxNetworkService::setup_ap_network(const AccessPointConfig& config, std::string* /*error_msg*/) {
    last_config_ = config;

    // 1. Bring interface UP and assign gateway IP
    network::InterfaceManager::set_up(config.interface);
    std::string cidr = config.gateway_ip + "/24";
    network::InterfaceManager::assign_ip(config.interface, cidr);

    // 2. Start DHCP & DNS services via dnsmasq
    network::DhcpConfig dhcp_cfg;
    dhcp_cfg.interface = config.interface;
    dhcp_cfg.gateway_ip = config.gateway_ip;
    dhcp_cfg.netmask = config.netmask;
    dhcp_cfg.range_start = config.dhcp_range_start;
    dhcp_cfg.range_end = config.dhcp_range_end;
    dhcp_cfg.enable_captive_portal = config.captive_portal;
    dhcp_cfg.portal_port = 8080;
    dhcp_server_.start(dhcp_cfg);

    // 3. Configure NAT if internet sharing is enabled
    if (config.internet_sharing && !config.upstream_interface.empty()) {
        network::NatConfig nat_cfg;
        nat_cfg.ap_interface = config.interface;
        nat_cfg.upstream_interface = config.upstream_interface;
        nat_cfg.enable_portal_redirect = config.captive_portal;
        nat_cfg.portal_port = 8080;
        network::NatManager::enable_nat(nat_cfg);
    } else if (config.captive_portal) {
        Firewall::apply_portal_redirect(config.interface, 8080);
    }

    return true;
}

bool LinuxNetworkService::teardown_ap_network(const AccessPointConfig& config, std::string* /*error_msg*/) {
    dhcp_server_.stop();

    if (config.internet_sharing && !config.upstream_interface.empty()) {
        network::NatConfig nat_cfg;
        nat_cfg.ap_interface = config.interface;
        nat_cfg.upstream_interface = config.upstream_interface;
        nat_cfg.enable_portal_redirect = config.captive_portal;
        nat_cfg.portal_port = 8080;
        network::NatManager::disable_nat(nat_cfg);
    }

    Firewall::flush_ap_rules(config.interface, config.upstream_interface);
    network::InterfaceManager::flush_ip(config.interface);

    return true;
}

std::vector<ClientInfo> LinuxNetworkService::get_clients() const {
    ClientMonitor monitor(last_config_.interface);
    return monitor.get_clients();
}

} // namespace apm::linux_backend

#include "firewall.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace apm::linux_backend {

namespace {

int exec_cmd(const std::string& cmd) {
    return std::system((cmd + " >/dev/null 2>&1").c_str());
}

} // namespace

bool Firewall::apply_masquerade(const std::string& upstream_iface, const std::string& ap_iface) {
    if (upstream_iface.empty() || ap_iface.empty()) return false;

    // Check if already present, if not add
    exec_cmd("iptables -t nat -C POSTROUTING -o " + upstream_iface + " -j MASQUERADE 2>/dev/null || iptables -t nat -A POSTROUTING -o " + upstream_iface + " -j MASQUERADE");
    exec_cmd("iptables -C FORWARD -i " + ap_iface + " -o " + upstream_iface + " -j ACCEPT 2>/dev/null || iptables -A FORWARD -i " + ap_iface + " -o " + upstream_iface + " -j ACCEPT");
    exec_cmd("iptables -C FORWARD -i " + upstream_iface + " -o " + ap_iface + " -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null || iptables -A FORWARD -i " + upstream_iface + " -o " + ap_iface + " -m state --state RELATED,ESTABLISHED -j ACCEPT");

    return true;
}

bool Firewall::remove_masquerade(const std::string& upstream_iface, const std::string& ap_iface) {
    if (upstream_iface.empty() || ap_iface.empty()) return false;

    exec_cmd("iptables -t nat -D POSTROUTING -o " + upstream_iface + " -j MASQUERADE");
    exec_cmd("iptables -D FORWARD -i " + ap_iface + " -o " + upstream_iface + " -j ACCEPT");
    exec_cmd("iptables -D FORWARD -i " + upstream_iface + " -o " + ap_iface + " -m state --state RELATED,ESTABLISHED");

    return true;
}

bool Firewall::apply_portal_redirect(const std::string& ap_iface, int portal_port) {
    if (ap_iface.empty()) return false;
    std::string port_str = std::to_string(portal_port);

    exec_cmd("iptables -t nat -A PREROUTING -i " + ap_iface + " -p tcp --dport 80 -j REDIRECT --to-ports " + port_str);
    return true;
}

bool Firewall::remove_portal_redirect(const std::string& ap_iface, int portal_port) {
    if (ap_iface.empty()) return false;
    std::string port_str = std::to_string(portal_port);

    exec_cmd("iptables -t nat -D PREROUTING -i " + ap_iface + " -p tcp --dport 80 -j REDIRECT --to-ports " + port_str);
    return true;
}

bool Firewall::authorize_client(const std::string& client_ip) {
    if (client_ip.empty()) return false;
    exec_cmd("iptables -t nat -I PREROUTING 1 -s " + client_ip + " -j ACCEPT");
    return true;
}

bool Firewall::deauthorize_client(const std::string& client_ip) {
    if (client_ip.empty()) return false;
    exec_cmd("iptables -t nat -D PREROUTING -s " + client_ip + " -j ACCEPT");
    return true;
}

bool Firewall::flush_ap_rules(const std::string& ap_iface, const std::string& upstream_iface) {
    remove_masquerade(upstream_iface, ap_iface);
    remove_portal_redirect(ap_iface, 8080);
    remove_portal_redirect(ap_iface, 80);
    return true;
}

} // namespace apm::linux_backend

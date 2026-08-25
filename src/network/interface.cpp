#include "network/interface.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <array>
#include <unistd.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

namespace apm::network {

namespace {

int run_system_cmd(const std::string& cmd) {
    return std::system((cmd + " >/dev/null 2>&1").c_str());
}

} // namespace

bool InterfaceManager::set_up(const std::string& if_name) {
    if (if_name.empty()) return false;
    return run_system_cmd("ip link set dev " + if_name + " up") == 0;
}

bool InterfaceManager::set_down(const std::string& if_name) {
    if (if_name.empty()) return false;
    return run_system_cmd("ip link set dev " + if_name + " down") == 0;
}

bool InterfaceManager::assign_ip(const std::string& if_name, const std::string& ip_with_prefix) {
    if (if_name.empty() || ip_with_prefix.empty()) return false;
    // Flush old IP first
    run_system_cmd("ip addr flush dev " + if_name);
    return run_system_cmd("ip addr add " + ip_with_prefix + " dev " + if_name) == 0;
}

bool InterfaceManager::flush_ip(const std::string& if_name) {
    if (if_name.empty()) return false;
    return run_system_cmd("ip addr flush dev " + if_name) == 0;
}

bool InterfaceManager::enable_ip_forwarding() {
    std::ofstream out("/proc/sys/net/ipv4/ip_forward");
    if (out.is_open()) {
        out << "1\n";
        out.close();
        return true;
    }
    return run_system_cmd("sysctl -w net.ipv4.ip_forward=1") == 0;
}

bool InterfaceManager::disable_ip_forwarding() {
    std::ofstream out("/proc/sys/net/ipv4/ip_forward");
    if (out.is_open()) {
        out << "0\n";
        out.close();
        return true;
    }
    return run_system_cmd("sysctl -w net.ipv4.ip_forward=0") == 0;
}

std::string InterfaceManager::get_ip(const std::string& if_name) {
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1 || ifaddr == nullptr) {
        return "";
    }

    std::string ip_str;
    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;

        if (ifa->ifa_name == if_name && ifa->ifa_addr->sa_family == AF_INET) {
            auto* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            char host[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &(sa->sin_addr), host, INET_ADDRSTRLEN)) {
                ip_str = host;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return ip_str;
}

std::string InterfaceManager::get_mac(const std::string& if_name) {
    std::ifstream file("/sys/class/net/" + if_name + "/address");
    if (!file.is_open()) return "";
    std::string mac;
    std::getline(file, mac);
    return mac;
}

} // namespace apm::network

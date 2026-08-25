#pragma once

#include <string>
#include <vector>
#include <set>
#include "apmanager/core/types.hpp"

namespace apm::linux_backend {

class ClientMonitor {
public:
    explicit ClientMonitor(const std::string& ap_interface = "");

    void set_interface(const std::string& iface) { interface_ = iface; }

    // Live client info from iw station dump + DHCP leases
    std::vector<ClientInfo> get_clients() const;

    // De-authenticate a specific client by MAC
    bool kick_client(const std::string& mac, std::string* error_msg = nullptr);

    // MAC blacklist management
    bool blacklist_add(const std::string& mac);
    bool blacklist_remove(const std::string& mac);
    bool is_blacklisted(const std::string& mac) const;
    std::vector<std::string> get_blacklist() const;
    void load_blacklist(const std::string& path = "config/mac_blacklist.txt");
    void save_blacklist(const std::string& path = "config/mac_blacklist.txt") const;

private:
    std::string interface_;
    std::set<std::string> blacklist_;

    // Parse iw station dump output for a given interface
    std::vector<ClientInfo> parse_station_dump() const;
    // Merge DHCP lease data (IP, hostname) into station data
    void merge_dhcp_leases(std::vector<ClientInfo>& clients) const;
};

} // namespace apm::linux_backend

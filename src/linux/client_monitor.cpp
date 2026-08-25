#include "client_monitor.hpp"
#include "network/dhcp.hpp"

#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <array>
#include <algorithm>
#include <regex>

namespace fs = std::filesystem;

namespace apm::linux_backend {

namespace {

std::string run_cmd(const std::string& cmd) {
    std::string result;
    std::array<char, 256> buffer;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    return result;
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

} // namespace

ClientMonitor::ClientMonitor(const std::string& ap_interface)
    : interface_(ap_interface) {
    load_blacklist();
}

std::vector<ClientInfo> ClientMonitor::parse_station_dump() const {
    std::vector<ClientInfo> clients;
    if (interface_.empty()) return clients;

    std::string output = run_cmd("iw dev " + interface_ + " station dump 2>/dev/null");
    if (output.empty()) return clients;

    std::istringstream iss(output);
    std::string line;
    ClientInfo current_client;
    bool in_station = false;

    while (std::getline(iss, line)) {
        // Line format: Station aa:bb:cc:dd:ee:ff (on wlan0)
        if (line.rfind("Station ", 0) == 0) {
            if (in_station && !current_client.mac.empty()) {
                clients.push_back(current_client);
            }
            current_client = ClientInfo();
            in_station = true;

            std::istringstream l_iss(line);
            std::string dummy, mac;
            if (l_iss >> dummy >> mac) {
                current_client.mac = mac;
            }
            continue;
        }

        if (!in_station) continue;

        // Parse metrics
        if (line.find("rx bytes:") != std::string::npos) {
            auto pos = line.find("rx bytes:");
            try { current_client.rx_bytes = std::stoll(line.substr(pos + 9)); } catch (...) {}
        } else if (line.find("tx bytes:") != std::string::npos) {
            auto pos = line.find("tx bytes:");
            try { current_client.tx_bytes = std::stoll(line.substr(pos + 9)); } catch (...) {}
        } else if (line.find("signal:") != std::string::npos) {
            auto pos = line.find("signal:");
            try { current_client.signal_dbm = std::stoi(line.substr(pos + 7)); } catch (...) {}
        } else if (line.find("inactive time:") != std::string::npos) {
            auto pos = line.find("inactive time:");
            current_client.connected_since = line.substr(pos + 14);
            // Trim whitespace
            auto f = current_client.connected_since.find_first_not_of(" \t");
            if (f != std::string::npos) current_client.connected_since = current_client.connected_since.substr(f);
        }
    }

    if (in_station && !current_client.mac.empty()) {
        clients.push_back(current_client);
    }

    return clients;
}

void ClientMonitor::merge_dhcp_leases(std::vector<ClientInfo>& clients) const {
    network::DhcpServer dhcp_srv;
    auto leases = dhcp_srv.get_leases("config/run/dnsmasq.leases");

    for (auto& cli : clients) {
        for (const auto& lease : leases) {
            if (to_lower(cli.mac) == to_lower(lease.mac)) {
                cli.ip = lease.ip;
                cli.hostname = lease.hostname;
                break;
            }
        }
    }

    // Add DHCP lease entries that might not be in station dump yet
    for (const auto& lease : leases) {
        bool found = false;
        for (const auto& cli : clients) {
            if (to_lower(cli.mac) == to_lower(lease.mac)) {
                found = true;
                break;
            }
        }
        if (!found) {
            ClientInfo new_cli;
            new_cli.mac = lease.mac;
            new_cli.ip = lease.ip;
            new_cli.hostname = lease.hostname;
            new_cli.connected_since = lease.expiry;
            clients.push_back(new_cli);
        }
    }
}

std::vector<ClientInfo> ClientMonitor::get_clients() const {
    auto clients = parse_station_dump();
    merge_dhcp_leases(clients);
    return clients;
}

bool ClientMonitor::kick_client(const std::string& mac, std::string* error_msg) {
    if (mac.empty()) {
        if (error_msg) *error_msg = "MAC address cannot be empty.";
        return false;
    }

    int res = -1;
    if (!interface_.empty()) {
        std::string cmd1 = "hostapd_cli -i " + interface_ + " deauthenticate " + mac + " >/dev/null 2>&1";
        res = std::system(cmd1.c_str());
        std::string cmd2 = "iw dev " + interface_ + " station del " + mac + " >/dev/null 2>&1";
        (void)std::system(cmd2.c_str());
    } else {
        std::string cmd = "hostapd_cli deauthenticate " + mac + " >/dev/null 2>&1";
        res = std::system(cmd.c_str());
    }

    if (res != 0) {
        if (error_msg) *error_msg = "Failed to deauthenticate client " + mac + " (hostapd_cli return: " + std::to_string(res) + ")";
        return false;
    }
    return true;
}

bool ClientMonitor::blacklist_add(const std::string& mac) {
    if (mac.empty()) return false;
    blacklist_.insert(to_lower(mac));
    save_blacklist();
    kick_client(mac);
    return true;
}

bool ClientMonitor::blacklist_remove(const std::string& mac) {
    if (mac.empty()) return false;
    auto it = blacklist_.find(to_lower(mac));
    if (it != blacklist_.end()) {
        blacklist_.erase(it);
        save_blacklist();
        return true;
    }
    return false;
}

bool ClientMonitor::is_blacklisted(const std::string& mac) const {
    return blacklist_.find(to_lower(mac)) != blacklist_.end();
}

std::vector<std::string> ClientMonitor::get_blacklist() const {
    return std::vector<std::string>(blacklist_.begin(), blacklist_.end());
}

void ClientMonitor::load_blacklist(const std::string& path) {
    blacklist_.clear();
    std::ifstream in(path);
    if (!in.is_open()) return;

    std::string line;
    while (std::getline(in, line)) {
        // Trim whitespace
        auto f = line.find_first_not_of(" \t\r\n");
        if (f == std::string::npos) continue;
        if (line[f] == '#') continue; // comment
        auto e = line.find_last_not_of(" \t\r\n");
        std::string mac = line.substr(f, e - f + 1);
        if (!mac.empty()) {
            blacklist_.insert(to_lower(mac));
        }
    }
}

void ClientMonitor::save_blacklist(const std::string& path) const {
    std::error_code ec;
    fs::create_directories("config", ec);

    std::ofstream out(path);
    if (!out.is_open()) return;

    out << "# AP-Generator MAC Blacklist\n";
    for (const auto& mac : blacklist_) {
        out << mac << "\n";
    }
}

} // namespace apm::linux_backend

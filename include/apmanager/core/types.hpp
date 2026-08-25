#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace apm {

enum class SecurityMode {
    Open,
    WPA2,
    WPA2WPA3,
    WPA3
};

inline std::string security_mode_to_string(SecurityMode mode) {
    switch (mode) {
        case SecurityMode::Open:     return "open";
        case SecurityMode::WPA2:     return "wpa2";
        case SecurityMode::WPA2WPA3: return "wpa2/wpa3";
        case SecurityMode::WPA3:     return "wpa3";
    }
    return "unknown";
}

inline SecurityMode security_mode_from_string(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    if (str == "wpa2") return SecurityMode::WPA2;
    if (str == "wpa3") return SecurityMode::WPA3;
    if (str == "wpa2/wpa3" || str == "wpa2wpa3" || str == "mixed") return SecurityMode::WPA2WPA3;
    return SecurityMode::Open;
}

struct WifiInterface {
    std::string name;
    std::string mac;
    std::string driver;
    std::string phy_name;
    std::string ip_address;

    bool up = false;
    bool connected = false;

    bool supports_ap = false;
    bool supports_wpa2 = true;
    bool supports_wpa3 = false;
    bool supports_concurrent_ap_sta = false;

    std::vector<std::string> bands;
    std::vector<int> channels_2ghz;
    std::vector<int> channels_5ghz;
    std::vector<int> channels; // All supported channels
};

struct AccessPointConfig {
    std::string name;             // Preset name (if applicable)
    std::string interface;        // AP wireless interface (e.g. wlan0, wlan1)
    std::string ssid;             // Network SSID
    std::string bssid;            // Optional custom BSSID (MAC)
    int channel = 6;              // Default channel (e.g. 1, 6, 11, 36, etc.)

    SecurityMode security = SecurityMode::WPA2;
    std::string password;         // Passphrase for WPA2/WPA3

    bool internet_sharing = false;
    std::string upstream_interface; // Upstream interface (e.g. eth0, wlan0)

    bool captive_portal = false;
    std::string portal_path;      // Path to HTML or folder

    std::string gateway_ip = "192.168.50.1";
    std::string netmask = "255.255.255.0";
    std::string dhcp_range_start = "192.168.50.10";
    std::string dhcp_range_end = "192.168.50.250";
};

struct ValidationResult {
    bool valid = true;
    std::string error;

    explicit operator bool() const noexcept { return valid; }
    static ValidationResult ok() { return {true, ""}; }
    static ValidationResult fail(const std::string& err) { return {false, err}; }
};

struct ClientInfo {
    std::string mac;
    std::string ip;
    std::string hostname;
    std::string connected_since;
    int64_t rx_bytes = 0;
    int64_t tx_bytes = 0;
    int signal_dbm = 0;
};

struct ApStatus {
    bool running = false;
    AccessPointConfig config;
    std::string hostapd_pid;
    std::string dnsmasq_pid;
    std::string uptime;
    std::vector<ClientInfo> connected_clients;
};

} // namespace apm
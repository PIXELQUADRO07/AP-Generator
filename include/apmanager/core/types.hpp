#pragma once

#include <string>
#include <vector>

namespace apm {

enum class SecurityMode {
    Open,
    WPA2,
    WPA2WPA3,
    WPA3
};

struct WifiInterface {
    std::string name;
    std::string mac;

    bool up = false;
    bool connected = false;

    bool supports_ap = false;
    bool supports_wpa2 = false;
    bool supports_wpa3 = false;
    bool supports_concurrent_ap_sta = false;

    std::vector<int> channels;
};

struct AccessPointConfig {
    std::string interface;
    std::string ssid;
    std::string bssid;

    int channel = 0;

    SecurityMode security = SecurityMode::Open;
    std::string password;

    bool internet_sharing = false;
    bool captive_portal = false;
};

}
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

        bool connected = false;
        bool suports_ap = false;
        bool supports_wpa3 = false;
    };

    struct AccesPointConfig {
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
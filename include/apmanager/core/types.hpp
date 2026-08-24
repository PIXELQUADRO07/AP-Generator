#pragma once 

#include <string>
#include <vector>
#include <iostream>

using namespace std;

namespace apm {

    enum class SecurityMode {
        Open,
        WPA2,
        WPA2WPA3,
        WPA3,
    };


    //discovered interface on system

    struct WifiInterface {
        string name;
        string mac;

        bool up = false;
        bool connected = false;

        bool supports_ap = false;
        bool supports_wpa3 = false;
    };

    //Acces point configuration to start /create
    
    struct AccesPointConfig {
        string interface;
        string ssid;
        string bssid; //empty = auto.

        int channel = 0;
        
        SecurityMode security = SecurityMode::Open;
        string password;

        bool Internet_sharing = false;
        bool captive_portal = false;
    };
}
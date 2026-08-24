#include "apmanager/core/wifi_backend.hpp"

#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/types.h>

namespace apm {

namespace {


    std::string read_sysfs_line(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return "";
        }

        std::string line;
        std::getline(file, line);
        return line;
    }


    bool is_wireless_interface(const std::string& if_name) {
        const std::string base = "/sys/class/net/" + if_name;

        DIR* dir = opendir((base + "/wireless").c_str());
        if (dir != nullptr) {
            closedir(dir);
            return true;
        }

        dir = opendir((base + "/phy80211").c_str());
        if (dir != nullptr) {
            closedir(dir);
            return true;
        }

        return false;
    }

}

std::vector<WifiInterface> LinuxWifiBackend::discover_interfaces() {
    std::vector<WifiInterface> interfaces;

    DIR* net_dir = opendir("/sys/class/net");
    if (net_dir == nullptr) {
        return interfaces;
    }

    struct dirent* entry;
    while ((entry = readdir(net_dir)) != nullptr) {
        std::string name = entry->d_name;

        if (name == "." || name == "..") {
            continue;
        }

        if (!is_wireless_interface(name)) {
            continue;
        }

        WifiInterface iface;
        iface.name = name;
        iface.mac = read_sysfs_line("/sys/class/net/" + name + "/address");

        const std::string operstate =
            read_sysfs_line("/sys/class/net/" + name + "/operstate");
        iface.up = (operstate == "up");


        iface.connected = iface.up;


        iface.supports_ap = false;
        iface.supports_wpa3 = false;

        interfaces.push_back(iface);
    }

    closedir(net_dir);
    return interfaces;
}

bool LinuxWifiBackend::create_ap(const AccessPointConfig& /*config*/) {

    return false;
}

bool LinuxWifiBackend::stop_ap() {

    return false;
}

}
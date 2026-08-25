#include "wifi_capabilities.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <unistd.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace fs = std::filesystem;

namespace apm::linux_backend {

namespace {

std::string run_command_output(const std::string& cmd) {
    std::string result;
    std::array<char, 256> buffer;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    return result;
}

std::string read_sysfs_first_line(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::string line;
    std::getline(file, line);
    return line;
}

} // namespace

std::string WifiCapabilities::detect_driver(const std::string& if_name) {
    std::string driver_link = "/sys/class/net/" + if_name + "/device/driver";
    std::error_code ec;
    if (fs::exists(driver_link, ec)) {
        fs::path resolved = fs::canonical(driver_link, ec);
        if (!ec) {
            return resolved.filename().string();
        }
    }
    return "";
}

std::string WifiCapabilities::detect_phy(const std::string& if_name) {
    std::string phy_link = "/sys/class/net/" + if_name + "/phy80211";
    std::error_code ec;
    if (fs::exists(phy_link, ec)) {
        fs::path resolved = fs::canonical(phy_link, ec);
        if (!ec) {
            return resolved.filename().string();
        }
    }
    return "";
}

std::string WifiCapabilities::detect_ip_address(const std::string& if_name) {
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1 || ifaddr == nullptr) {
        return "";
    }

    std::string ip_str = "";
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

bool WifiCapabilities::detect_ap_support(const std::string& phy_name, const std::string& driver) {
    if (!phy_name.empty()) {
        std::string iw_out = run_command_output("iw phy " + phy_name + " info 2>/dev/null");
        if (!iw_out.empty()) {
            if (iw_out.find("* AP\n") != std::string::npos ||
                iw_out.find("* AP\r\n") != std::string::npos ||
                iw_out.find("* AP ") != std::string::npos ||
                iw_out.find(" AP\n") != std::string::npos) {
                return true;
            }
        }
    }

    // Fallback: Check if the device uses mac80211 / known AP-capable drivers
    if (!driver.empty()) {
        static const std::vector<std::string> ap_drivers = {
            "iwlwifi", "ath9k", "ath10k", "ath11k", "ath12k", "mt76",
            "mt7921e", "mt7921u", "mt7922", "mt7915e", "rtw88", "rtw89",
            "rtl8192cu", "rtl8812au", "rtl8821cu", "brcmfmac", "mac80211_hwsim"
        };
        for (const auto& d : ap_drivers) {
            if (driver.find(d) != std::string::npos) {
                return true;
            }
        }
    }

    return !phy_name.empty();
}

void WifiCapabilities::detect_bands_and_channels(const std::string& phy_name, WifiInterface& iface) {
    if (!phy_name.empty()) {
        std::string iw_out = run_command_output("iw phy " + phy_name + " info 2>/dev/null");
        if (!iw_out.empty()) {
            // Check for SAE / WPA3
            if (iw_out.find("SAE") != std::string::npos || iw_out.find("WPA3") != std::string::npos) {
                iface.supports_wpa3 = true;
            }

            // Check for concurrent AP + STA in combinations
            if (iw_out.find("#{ managed } <= 1, #{ AP } <= 1") != std::string::npos ||
                iw_out.find("#{ AP, mesh point } <= 1, #{ managed } <= 1") != std::string::npos ||
                iw_out.find("#{ managed, AP }") != std::string::npos) {
                iface.supports_concurrent_ap_sta = true;
            }

            // Parse frequencies / channels: e.g. "* 2412.0 MHz [1] (20.0 dBm)" or "* 5180.0 MHz [36]"
            std::regex chan_regex(R"(\*\s+(\d+(?:\.\d+)?)\s+MHz\s+\[(\d+)\])");
            auto words_begin = std::sregex_iterator(iw_out.begin(), iw_out.end(), chan_regex);
            auto words_end = std::sregex_iterator();

            for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                std::smatch match = *i;
                double freq = std::stod(match[1].str());
                int ch = std::stoi(match[2].str());

                iface.channels.push_back(ch);
                if (freq >= 2400.0 && freq < 2500.0) {
                    iface.channels_2ghz.push_back(ch);
                } else if (freq >= 5000.0 && freq < 6000.0) {
                    iface.channels_5ghz.push_back(ch);
                }
            }
        }
    }

    // Fallback if no channels parsed via iw
    if (iface.channels_2ghz.empty()) {
        for (int ch = 1; ch <= 13; ++ch) {
            iface.channels_2ghz.push_back(ch);
            iface.channels.push_back(ch);
        }
    }

    // If driver supports 5 GHz
    if (iface.channels_5ghz.empty() && (iface.driver.find("iwl") != std::string::npos ||
                                        iface.driver.find("ath10") != std::string::npos ||
                                        iface.driver.find("mt79") != std::string::npos ||
                                        iface.driver.find("rtw88") != std::string::npos ||
                                        iface.driver.find("rtw89") != std::string::npos)) {
        static const std::vector<int> std_5g = {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 149, 153, 157, 161, 165};
        iface.channels_5ghz = std_5g;
        for (int ch : std_5g) {
            iface.channels.push_back(ch);
        }
    }

    if (!iface.channels_2ghz.empty()) {
        iface.bands.push_back("2.4 GHz");
    }
    if (!iface.channels_5ghz.empty()) {
        iface.bands.push_back("5 GHz");
    }
}

bool WifiCapabilities::inspect_interface(const std::string& if_name, WifiInterface& iface) {
    iface.name = if_name;
    iface.mac = read_sysfs_first_line("/sys/class/net/" + if_name + "/address");

    std::string operstate = read_sysfs_first_line("/sys/class/net/" + if_name + "/operstate");
    iface.up = (operstate == "up");

    // Carrier check for connected
    std::string carrier = read_sysfs_first_line("/sys/class/net/" + if_name + "/carrier");
    iface.connected = (carrier == "1" || iface.up);

    iface.driver = detect_driver(if_name);
    iface.phy_name = detect_phy(if_name);
    iface.ip_address = detect_ip_address(if_name);

    iface.supports_ap = detect_ap_support(iface.phy_name, iface.driver);
    iface.supports_wpa2 = true;
    iface.supports_wpa3 = (iface.driver.find("iwl") != std::string::npos ||
                           iface.driver.find("mt79") != std::string::npos ||
                           iface.driver.find("ath1") != std::string::npos);

    detect_bands_and_channels(iface.phy_name, iface);
    return true;
}

} // namespace apm::linux_backend

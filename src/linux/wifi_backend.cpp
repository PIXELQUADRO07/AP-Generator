#include "apmanager/core/wifi_backend.hpp"
#include "apmanager/core/preset_manager.hpp"
#include "wifi_capabilities.hpp"

#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

namespace apm {

struct LinuxWifiBackend::Impl {
    std::string state_file = "config/run/ap-state.json";
};

namespace {

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

} // namespace

LinuxWifiBackend::LinuxWifiBackend() : impl_(std::make_unique<Impl>()) {
    std::error_code ec;
    fs::create_directories("config/run", ec);
}

LinuxWifiBackend::~LinuxWifiBackend() = default;

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
        linux_backend::WifiCapabilities::inspect_interface(name, iface);
        interfaces.push_back(iface);
    }

    closedir(net_dir);
    return interfaces;
}

WifiInterface LinuxWifiBackend::get_interface_capabilities(const std::string& if_name) {
    WifiInterface iface;
    linux_backend::WifiCapabilities::inspect_interface(if_name, iface);
    return iface;
}

bool LinuxWifiBackend::start_ap(const AccessPointConfig& config) {
    std::error_code ec;
    fs::create_directories("config/run", ec);

    std::ofstream out(impl_->state_file);
    if (!out.is_open()) {
        return false;
    }

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    out << "{\n";
    out << "  \"timestamp\": " << timestamp << ",\n";
    out << "  \"pid\": " << getpid() << ",\n";
    out << "  \"config\": " << PresetManager::serialize_config_json(config) << "\n";
    out << "}\n";
    out.close();

    return true;
}

bool LinuxWifiBackend::stop_ap() {
    if (!is_ap_running()) {
        return false;
    }

    std::error_code ec;
    fs::remove(impl_->state_file, ec);
    return true;
}

bool LinuxWifiBackend::is_ap_running() const {
    std::error_code ec;
    return fs::exists(impl_->state_file, ec);
}

ApStatus LinuxWifiBackend::get_status() const {
    ApStatus status;
    if (!is_ap_running()) {
        status.running = false;
        return status;
    }

    std::ifstream in(impl_->state_file);
    if (!in.is_open()) {
        status.running = false;
        return status;
    }

    std::stringstream buf;
    buf << in.rdbuf();
    std::string content = buf.str();

    status.running = true;

    // Parse timestamp
    auto time_pos = content.find("\"timestamp\":");
    if (time_pos != std::string::npos) {
        try {
            int64_t ts = std::stoll(content.substr(time_pos + 12));
            auto now = std::chrono::system_clock::now();
            auto now_ts = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            int64_t uptime_sec = std::max<int64_t>(0, now_ts - ts);
            status.uptime = std::to_string(uptime_sec) + "s";
        } catch (...) {
            status.uptime = "0s";
        }
    }

    PresetManager::deserialize_config_json(content, status.config);
    return status;
}

} // namespace apm
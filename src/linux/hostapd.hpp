#pragma once

#include <string>
#include <vector>
#include "apmanager/core/types.hpp"

namespace apm::linux_backend {

class HostapdManager {
public:
    HostapdManager() = default;
    ~HostapdManager();

    bool start(const AccessPointConfig& config, std::string* error_msg = nullptr);
    bool stop(std::string* error_msg = nullptr);
    bool is_running() const;

    static std::string generate_config_string(const AccessPointConfig& config);
    std::vector<std::string> get_associated_stations() const;

private:
    int pid_ = -1;
    std::string config_path_ = "config/run/hostapd.conf";
    std::string pid_file_ = "config/run/hostapd.pid";
};

} // namespace apm::linux_backend

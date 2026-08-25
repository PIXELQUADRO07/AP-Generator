#pragma once

#include <vector>
#include <string>
#include <memory>

#include "apmanager/core/types.hpp"

namespace apm {

class WifiBackend {
public:
    virtual ~WifiBackend() = default;

    virtual std::vector<WifiInterface> discover_interfaces() = 0;
    virtual WifiInterface get_interface_capabilities(const std::string& if_name) = 0;
    virtual bool start_ap(const AccessPointConfig& config) = 0;
    virtual bool stop_ap() = 0;
    virtual bool is_ap_running() const = 0;
    virtual ApStatus get_status() const = 0;
};

class LinuxWifiBackend : public WifiBackend {
public:
    LinuxWifiBackend();
    ~LinuxWifiBackend() override;

    std::vector<WifiInterface> discover_interfaces() override;
    WifiInterface get_interface_capabilities(const std::string& if_name) override;
    bool start_ap(const AccessPointConfig& config) override;
    bool stop_ap() override;
    bool is_ap_running() const override;
    ApStatus get_status() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace apm
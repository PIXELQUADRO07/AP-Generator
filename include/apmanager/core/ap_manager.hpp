#pragma once

#include <memory>
#include <string>
#include <vector>

#include "apmanager/core/types.hpp"
#include "apmanager/core/wifi_backend.hpp"

namespace apm {

class ApManager {
public:
    explicit ApManager(std::unique_ptr<WifiBackend> backend);
    ~ApManager() = default;

    // Interface discovery and capability inspection
    std::vector<WifiInterface> list_interfaces();
    WifiInterface get_interface_capabilities(const std::string& if_name);

    // Configuration validation
    ValidationResult validate(const AccessPointConfig& config) const;

    // AP lifecycle
    bool start(const AccessPointConfig& config, std::string* error_msg = nullptr);
    bool stop(std::string* error_msg = nullptr);
    bool is_running() const;
    ApStatus get_status() const;

    // Backend access
    WifiBackend* backend() { return backend_.get(); }
    const WifiBackend* backend() const { return backend_.get(); }

private:
    std::unique_ptr<WifiBackend> backend_;
};

} // namespace apm

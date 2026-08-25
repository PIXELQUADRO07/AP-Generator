#include "apmanager/core/ap_manager.hpp"

#include <iostream>
#include <regex>

namespace apm {

ApManager::ApManager(std::unique_ptr<WifiBackend> backend)
    : backend_(std::move(backend)) {}

std::vector<WifiInterface> ApManager::list_interfaces() {
    if (!backend_) {
        return {};
    }
    return backend_->discover_interfaces();
}

WifiInterface ApManager::get_interface_capabilities(const std::string& if_name) {
    if (!backend_) {
        return {};
    }
    return backend_->get_interface_capabilities(if_name);
}

ValidationResult ApManager::validate(const AccessPointConfig& config) const {
    if (config.interface.empty()) {
        return ValidationResult::fail("No wireless interface specified.");
    }

    if (config.ssid.empty()) {
        return ValidationResult::fail("SSID cannot be empty.");
    }

    if (config.ssid.size() > 32) {
        return ValidationResult::fail("SSID cannot exceed 32 characters (IEEE 802.11 limit).");
    }

    if (config.channel < 1 || (config.channel > 14 && config.channel < 36) || config.channel > 165) {
        return ValidationResult::fail("Channel " + std::to_string(config.channel) + " is not a valid 2.4 GHz or 5 GHz Wi-Fi channel.");
    }

    if (config.security != SecurityMode::Open) {
        if (config.password.empty()) {
            return ValidationResult::fail("Password is required for WPA2/WPA3 security mode.");
        }
        if (config.password.size() < 8 || config.password.size() > 63) {
            return ValidationResult::fail("WPA passphrase must be between 8 and 63 ASCII characters.");
        }
    }

    if (config.internet_sharing) {
        if (config.upstream_interface.empty()) {
            return ValidationResult::fail("Internet sharing is enabled but no upstream interface was specified.");
        }
        if (config.upstream_interface == config.interface) {
            return ValidationResult::fail("Upstream interface cannot be the same as the AP interface (" + config.interface + ").");
        }
    }

    if (!config.bssid.empty()) {
        // Validate MAC format XX:XX:XX:XX:XX:XX
        std::regex mac_regex("^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$");
        if (!std::regex_match(config.bssid, mac_regex)) {
            return ValidationResult::fail("Invalid BSSID format: " + config.bssid + " (expected format XX:XX:XX:XX:XX:XX).");
        }
    }

    return ValidationResult::ok();
}

bool ApManager::start(const AccessPointConfig& config, std::string* error_msg) {
    ValidationResult val = validate(config);
    if (!val) {
        if (error_msg) *error_msg = val.error;
        return false;
    }

    if (!backend_) {
        if (error_msg) *error_msg = "No Wi-Fi backend initialized.";
        return false;
    }

    if (backend_->is_ap_running()) {
        if (error_msg) *error_msg = "An Access Point is already running. Stop it first.";
        return false;
    }

    bool ok = backend_->start_ap(config);
    if (!ok && error_msg && error_msg->empty()) {
        *error_msg = "Failed to start Access Point on interface " + config.interface + ".";
    }
    return ok;
}

bool ApManager::stop(std::string* error_msg) {
    if (!backend_) {
        if (error_msg) *error_msg = "No Wi-Fi backend initialized.";
        return false;
    }

    if (!backend_->is_ap_running()) {
        if (error_msg) *error_msg = "No Access Point is currently running.";
        return false;
    }

    return backend_->stop_ap();
}

bool ApManager::is_running() const {
    return backend_ && backend_->is_ap_running();
}

ApStatus ApManager::get_status() const {
    if (!backend_) {
        return {};
    }
    return backend_->get_status();
}

} // namespace apm
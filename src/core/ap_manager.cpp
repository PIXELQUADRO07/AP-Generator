#include "apmanager/core/ap_manager.hpp"

namespace apm {

    ApManager::ApManager(std::unique_ptr<WifiBackend> backend)
     : backend_(std::move(backend)) {}

     std::string ApManager::validate(const AccesPointConfig& config) const {
        if (config.interface.empty()) {
            return "No interface specified.";
        }

        if (config.ssid.empty()) {
            return "The SSID cannot be empty.";
        }

        if (config.ssid.size() > 32) {
            return "The SSID cannot be longer than 32 characters";
        }

        if(config.security != SecurityMode:Open) {



            if(config.password.size() < 8 || config.password.size() > 63) {
                return "The password cannot be shorter than 8 or longer than 63 characters.";
            }
        }
     }
}
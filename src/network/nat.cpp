#include "network/nat.hpp"
#include "network/interface.hpp"

#if defined(__linux__)
#include "../linux/firewall.hpp"
#endif

namespace apm::network {

bool NatManager::enable_nat(const NatConfig& config) {
    if (config.ap_interface.empty() || config.upstream_interface.empty()) {
        return false;
    }

    // Enable kernel IP forwarding
    InterfaceManager::enable_ip_forwarding();

#if defined(__linux__)
    bool ok = linux_backend::Firewall::apply_masquerade(config.upstream_interface, config.ap_interface);
    if (config.enable_portal_redirect) {
        linux_backend::Firewall::apply_portal_redirect(config.ap_interface, config.portal_port);
    }
    return ok;
#else
    return false;
#endif
}

bool NatManager::disable_nat(const NatConfig& config) {
#if defined(__linux__)
    if (config.enable_portal_redirect) {
        linux_backend::Firewall::remove_portal_redirect(config.ap_interface, config.portal_port);
    }
    return linux_backend::Firewall::remove_masquerade(config.upstream_interface, config.ap_interface);
#else
    return false;
#endif
}

} // namespace apm::network

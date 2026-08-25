#include "portal/captive_portal.hpp"

#if defined(__linux__)
#include "../linux/firewall.hpp"
#endif

#include <filesystem>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

namespace apm::portal {

CaptivePortal::CaptivePortal(const std::string& presets_dir) {
    if (!presets_dir.empty() && fs::exists(presets_dir)) {
        presets_dir_ = presets_dir;
    } else if (fs::exists("portal/presets")) {
        presets_dir_ = "portal/presets";
    } else {
        presets_dir_ = "portal/presets";
    }
}

CaptivePortal::~CaptivePortal() {
    stop();
}

std::vector<std::string> CaptivePortal::list_available_templates() const {
    std::vector<std::string> templates;
    std::error_code ec;
    if (!fs::exists(presets_dir_, ec)) {
        return templates;
    }

    for (const auto& entry : fs::directory_iterator(presets_dir_, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".html") {
            templates.push_back(entry.path().stem().string());
        }
    }

    std::sort(templates.begin(), templates.end());
    return templates;
}

std::string CaptivePortal::resolve_template_path(const std::string& template_name) const {
    if (template_name.empty()) {
        return "portal/default/index.html";
    }

    // Direct existing path
    if (fs::exists(template_name)) {
        return template_name;
    }

    // With .html extension in presets_dir
    std::string candidate1 = presets_dir_ + "/" + template_name + ".html";
    if (fs::exists(candidate1)) {
        return candidate1;
    }

    // Direct filename in presets_dir
    std::string candidate2 = presets_dir_ + "/" + template_name;
    if (fs::exists(candidate2)) {
        return candidate2;
    }

    return "portal/default/index.html";
}

bool CaptivePortal::start(const std::string& portal_template_or_path, int port) {
    HttpServerConfig cfg;
    cfg.port = port;
    cfg.listen_ip = "0.0.0.0";
    cfg.document_root = "portal/default";

    if (!portal_template_or_path.empty()) {
        cfg.single_html_path = resolve_template_path(portal_template_or_path);
    } else {
        cfg.single_html_path = resolve_template_path("");
    }

    server_.set_auth_callback([this](const std::string& client_ip, const std::string& form_data) {
        (void)form_data;
        std::cout << "[CaptivePortal] Client autenticato con successo: " << client_ip << "\n";
        this->authorize_client(client_ip);
    });

    return server_.start(cfg);
}

bool CaptivePortal::stop() {
    return server_.stop();
}

bool CaptivePortal::is_running() const {
    return server_.is_running();
}

void CaptivePortal::authorize_client(const std::string& client_ip) {
    if (client_ip.empty()) return;
    authorized_ips_.insert(client_ip);

#if defined(__linux__)
    linux_backend::Firewall::authorize_client(client_ip);
#endif
}

bool CaptivePortal::is_client_authorized(const std::string& client_ip) const {
    return authorized_ips_.find(client_ip) != authorized_ips_.end();
}

std::vector<std::string> CaptivePortal::get_authorized_clients() const {
    return std::vector<std::string>(authorized_ips_.begin(), authorized_ips_.end());
}

} // namespace apm::portal

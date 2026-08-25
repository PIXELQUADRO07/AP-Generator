#pragma once

#include <string>
#include <vector>
#include <memory>
#include <set>
#include "portal/http_server.hpp"

namespace apm::portal {

class CaptivePortal {
public:
    explicit CaptivePortal(const std::string& presets_dir = "portal/presets");
    ~CaptivePortal();

    bool start(const std::string& portal_template_or_path, int port = 8080);
    bool stop();
    bool is_running() const;

    std::vector<std::string> list_available_templates() const;
    std::string resolve_template_path(const std::string& template_name) const;

    void authorize_client(const std::string& client_ip);
    bool is_client_authorized(const std::string& client_ip) const;
    std::vector<std::string> get_authorized_clients() const;

private:
    std::string presets_dir_;
    HttpServer server_;
    std::set<std::string> authorized_ips_;
};

} // namespace apm::portal

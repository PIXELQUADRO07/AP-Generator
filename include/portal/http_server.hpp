#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>

namespace apm::portal {

using AuthCallback = std::function<void(const std::string& client_ip, const std::string& form_data)>;

struct HttpServerConfig {
    int port = 8080;
    std::string listen_ip = "0.0.0.0";
    std::string document_root = "portal/default";
    std::string single_html_path = "";
    std::string portal_title = "Wi-Fi Hotspot Login";
};

class HttpServer {
public:
    HttpServer();
    ~HttpServer();

    bool start(const HttpServerConfig& config);
    bool stop();
    bool is_running() const;

    void set_auth_callback(AuthCallback callback) { auth_callback_ = std::move(callback); }

private:
    HttpServerConfig config_;
    std::atomic<bool> running_{false};
    int server_fd_ = -1;
    int shutdown_pipe_[2] = {-1, -1};
    std::unique_ptr<std::thread> worker_thread_;
    AuthCallback auth_callback_;

    void run_loop();
    void handle_client(int client_fd);
    std::string get_mime_type(const std::string& path) const;
    std::string load_portal_html() const;
};

} // namespace apm::portal

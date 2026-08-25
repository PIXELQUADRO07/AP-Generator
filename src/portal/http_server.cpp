#include "portal/http_server.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include <filesystem>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>

namespace fs = std::filesystem;

namespace apm::portal {

namespace {

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

} // namespace

HttpServer::HttpServer() = default;

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start(const HttpServerConfig& config) {
    if (running_) {
        return true;
    }

    config_ = config;

    // Create shutdown pipe
    if (pipe(shutdown_pipe_) != 0) {
        return false;
    }

    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        close(shutdown_pipe_[0]);
        close(shutdown_pipe_[1]);
        return false;
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config_.port);
    if (config_.listen_ip.empty() || config_.listen_ip == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, config_.listen_ip.c_str(), &addr.sin_addr);
    }

    if (bind(server_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(server_fd_);
        close(shutdown_pipe_[0]);
        close(shutdown_pipe_[1]);
        server_fd_ = -1;
        return false;
    }

    if (listen(server_fd_, 64) < 0) {
        close(server_fd_);
        close(shutdown_pipe_[0]);
        close(shutdown_pipe_[1]);
        server_fd_ = -1;
        return false;
    }

    set_nonblocking(server_fd_);
    running_ = true;
    worker_thread_ = std::make_unique<std::thread>(&HttpServer::run_loop, this);
    return true;
}

bool HttpServer::stop() {
    if (!running_) {
        return true;
    }

    running_ = false;

    // Signal shutdown pipe
    if (shutdown_pipe_[1] != -1) {
        char dummy = 'q';
        (void)write(shutdown_pipe_[1], &dummy, 1);
    }

    if (worker_thread_ && worker_thread_->joinable()) {
        worker_thread_->join();
    }

    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
    }

    if (shutdown_pipe_[0] != -1) {
        close(shutdown_pipe_[0]);
        close(shutdown_pipe_[1]);
        shutdown_pipe_[0] = -1;
        shutdown_pipe_[1] = -1;
    }

    return true;
}

bool HttpServer::is_running() const {
    return running_;
}

void HttpServer::run_loop() {
    struct pollfd fds[2];
    fds[0].fd = server_fd_;
    fds[0].events = POLLIN;
    fds[1].fd = shutdown_pipe_[0];
    fds[1].events = POLLIN;

    while (running_) {
        int ret = poll(fds, 2, 500);
        if (ret <= 0) continue;

        if (fds[1].revents & POLLIN) {
            break;
        }

        if (fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd_, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
            if (client_fd >= 0) {
                handle_client(client_fd);
                close(client_fd);
            }
        }
    }
}

std::string HttpServer::get_mime_type(const std::string& path) const {
    std::string ext = fs::path(path).extension().string();
    if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".json") return "application/json";
    if (ext == ".ico") return "image/x-icon";
    return "text/plain; charset=utf-8";
}

std::string HttpServer::load_portal_html() const {
    if (!config_.single_html_path.empty() && fs::exists(config_.single_html_path)) {
        std::ifstream in(config_.single_html_path);
        if (in.is_open()) {
            std::stringstream buffer;
            buffer << in.rdbuf();
            return buffer.str();
        }
    }

    std::string default_path = config_.document_root + "/index.html";
    if (fs::exists(default_path)) {
        std::ifstream in(default_path);
        if (in.is_open()) {
            std::stringstream buffer;
            buffer << in.rdbuf();
            return buffer.str();
        }
    }

    // Built-in fallback HTML
    return R"(<!DOCTYPE html>
<html lang="it">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Access Point Captive Portal</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #0f172a; color: #f8fafc; display: flex; align-items: center; justify-content: center; height: 100vh; margin: 0; }
        .card { background: #1e293b; padding: 2.5rem; border-radius: 1rem; box-shadow: 0 10px 25px rgba(0,0,0,0.5); text-align: center; max-width: 400px; width: 90%; border: 1px solid #334155; }
        h1 { color: #38bdf8; font-size: 1.6rem; margin-bottom: 0.5rem; }
        p { color: #94a3b8; font-size: 0.95rem; margin-bottom: 1.5rem; line-height: 1.4; }
        .btn { display: inline-block; background: #0284c7; color: white; border: none; padding: 0.8rem 2rem; font-size: 1rem; font-weight: bold; border-radius: 0.5rem; cursor: pointer; text-decoration: none; transition: background 0.2s; width: 100%; box-sizing: border-box; }
        .btn:hover { background: #0369a1; }
    </style>
</head>
<body>
    <div class="card">
        <h1>Benvenuto sulla Rete Wi-Fi</h1>
        <p>Per iniziare a navigare liberamente su Internet, premi il pulsante sottostante per connetterti.</p>
        <form action="/login" method="POST">
            <button type="submit" class="btn">Connetti / Accedi</button>
        </form>
    </div>
</body>
</html>)";
}

void HttpServer::handle_client(int client_fd) {
    char buffer[4096];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) return;

    buffer[bytes_read] = '\0';
    std::string request(buffer, bytes_read);

    std::istringstream iss(request);
    std::string method, path, proto;
    iss >> method >> path >> proto;

    // Get client IP
    struct sockaddr_in peer{};
    socklen_t peer_len = sizeof(peer);
    std::string client_ip = "127.0.0.1";
    if (getpeername(client_fd, reinterpret_cast<struct sockaddr*>(&peer), &peer_len) == 0) {
        char ip_str[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &peer.sin_addr, ip_str, sizeof(ip_str))) {
            client_ip = ip_str;
        }
    }

    // Captive Portal Detection Endpoints (redirect or respond with portal)
    if (path == "/generate_204" || path == "/gen_204" ||
        path == "/hotspot-detect.html" || path == "/canonical.html" ||
        path == "/ncsi.txt" || path == "/connecttest.txt" ||
        path == "/redirect") {
        
        std::string html = load_portal_html();
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/html; charset=utf-8\r\n";
        response << "Content-Length: " << html.size() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << html;
        std::string resp_str = response.str();
        send(client_fd, resp_str.data(), resp_str.size(), 0);
        return;
    }

    // Handle Login / Authentication Form Submission
    if (path == "/login" || path == "/auth" || path == "/authorize") {
        std::string body;
        auto body_pos = request.find("\r\n\r\n");
        if (body_pos != std::string::npos) {
            body = request.substr(body_pos + 4);
        }

        if (auth_callback_) {
            auth_callback_(client_ip, body);
        }

        std::string success_page = R"(<!DOCTYPE html>
<html lang="it">
<head>
    <meta charset="UTF-8"><title>Connessione Riuscita</title>
    <style>
        body { font-family: sans-serif; background: #0f172a; color: #f8fafc; display: flex; align-items: center; justify-content: center; height: 100vh; margin: 0; }
        .box { background: #1e293b; padding: 2rem; border-radius: 1rem; text-align: center; border: 1px solid #334155; }
        h1 { color: #22c55e; }
    </style>
</head>
<body>
    <div class="box">
        <h1>Connesso con successo!</h1>
        <p>Ora puoi navigare liberamente su Internet.</p>
    </div>
</body>
</html>)";

        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/html; charset=utf-8\r\n";
        response << "Content-Length: " << success_page.size() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << success_page;
        std::string resp_str = response.str();
        send(client_fd, resp_str.data(), resp_str.size(), 0);
        return;
    }

    // Check if static file requested (e.g. /style.css, /script.js, /logo.png)
    if (path != "/" && path != "/index.html") {
        std::string sanitized_path = path;
        if (!sanitized_path.empty() && sanitized_path[0] == '/') {
            sanitized_path = sanitized_path.substr(1);
        }
        std::string file_path = config_.document_root + "/" + sanitized_path;

        if (fs::exists(file_path) && fs::is_regular_file(file_path)) {
            std::ifstream file(file_path, std::ios::binary);
            if (file.is_open()) {
                std::stringstream file_buf;
                file_buf << file.rdbuf();
                std::string file_content = file_buf.str();

                std::ostringstream response;
                response << "HTTP/1.1 200 OK\r\n";
                response << "Content-Type: " << get_mime_type(file_path) << "\r\n";
                response << "Content-Length: " << file_content.size() << "\r\n";
                response << "Connection: close\r\n\r\n";
                response << file_content;
                std::string resp_str = response.str();
                send(client_fd, resp_str.data(), resp_str.size(), 0);
                return;
            }
        }
    }

    // Default: Return Captive Portal Index Page
    std::string html = load_portal_html();
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html; charset=utf-8\r\n";
    response << "Content-Length: " << html.size() << "\r\n";
    response << "Connection: close\r\n\r\n";
    response << html;
    std::string resp_str = response.str();
    send(client_fd, resp_str.data(), resp_str.size(), 0);
}

} // namespace apm::portal

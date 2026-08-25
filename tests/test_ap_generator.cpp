#include <iostream>
#include <sstream>
#include <cassert>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include "apmanager/core/types.hpp"
#include "apmanager/core/ap_manager.hpp"
#include "apmanager/core/preset_manager.hpp"
#include "portal/http_server.hpp"
#include "portal/captive_portal.hpp"
#include "hostapd.hpp"
#include "firewall.hpp"
#include "network/interface.hpp"
#include "network/dhcp.hpp"
#include "network/dns.hpp"
#include "network/nat.hpp"

namespace {

std::string send_http_request(int port, const std::string& method, const std::string& path, const std::string& body = "") {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return "";

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(sock);
        return "";
    }

    std::ostringstream req;
    req << method << " " << path << " HTTP/1.1\r\n";
    req << "Host: 127.0.0.1:" << port << "\r\n";
    req << "User-Agent: AP-Generator-Test\r\n";
    if (!body.empty()) {
        req << "Content-Type: application/x-www-form-urlencoded\r\n";
        req << "Content-Length: " << body.size() << "\r\n";
    }
    req << "Connection: close\r\n\r\n";
    req << body;

    std::string req_str = req.str();
    send(sock, req_str.data(), req_str.size(), 0);

    std::string response;
    char buf[1024];
    ssize_t n;
    while ((n = recv(sock, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        response += buf;
    }

    close(sock);
    return response;
}

} // namespace

int main() {
    std::cout << "[TEST] Starting AP-Generator Integration Test Suite...\n";

    // 1. Test Preset Serialization / Deserialization
    {
        std::cout << "[TEST 1] Testing Preset serialization & deserialization... ";
        apm::AccessPointConfig cfg;
        cfg.name = "UnitTestAP";
        cfg.ssid = "TestSSID";
        cfg.interface = "wlan0";
        cfg.channel = 6;
        cfg.security = apm::SecurityMode::WPA2;
        cfg.password = "UnitPass123";
        cfg.internet_sharing = true;
        cfg.upstream_interface = "eth0";
        cfg.captive_portal = true;
        cfg.portal_path = "Google_Modern";

        std::string json = apm::PresetManager::serialize_config_json(cfg);
        assert(json.find("\"ssid\": \"TestSSID\"") != std::string::npos);

        apm::AccessPointConfig loaded_cfg;
        std::string err;
        bool ok = apm::PresetManager::deserialize_config_json(json, loaded_cfg, &err);
        assert(ok);
        assert(loaded_cfg.ssid == "TestSSID");
        assert(loaded_cfg.password == "UnitPass123");
        assert(loaded_cfg.security == apm::SecurityMode::WPA2);
        assert(loaded_cfg.internet_sharing == true);
        assert(loaded_cfg.captive_portal == true);
        std::cout << "PASSED\n";
    }

    // 2. Test Hostapd Config Generation
    {
        std::cout << "[TEST 2] Testing hostapd.conf generation... ";
        apm::AccessPointConfig cfg;
        cfg.interface = "wlan0";
        cfg.ssid = "SecureOffice";
        cfg.channel = 36;
        cfg.security = apm::SecurityMode::WPA3;
        cfg.password = "SuperWpa3Key!";

        std::string hostapd_conf = apm::linux_backend::HostapdManager::generate_config_string(cfg);
        assert(hostapd_conf.find("interface=wlan0") != std::string::npos);
        assert(hostapd_conf.find("ssid=SecureOffice") != std::string::npos);
        assert(hostapd_conf.find("hw_mode=a") != std::string::npos);
        assert(hostapd_conf.find("channel=36") != std::string::npos);
        assert(hostapd_conf.find("wpa_key_mgmt=SAE") != std::string::npos);
        assert(hostapd_conf.find("sae_password=SuperWpa3Key!") != std::string::npos);
        assert(hostapd_conf.find("ieee80211w=2") != std::string::npos);
        std::cout << "PASSED\n";
    }

    // 3. Test DNS Config Generation
    {
        std::cout << "[TEST 3] Testing DNS config generation... ";
        apm::network::DnsConfig dns_cfg;
        dns_cfg.interface = "wlan0";
        dns_cfg.listen_ip = "192.168.50.1";
        dns_cfg.captive_portal_dns = true;

        std::string dns_conf = apm::network::DnsServer::generate_dns_config(dns_cfg);
        assert(dns_conf.find("address=/#/192.168.50.1") != std::string::npos);
        std::cout << "PASSED\n";
    }

    // 4. Test Native C++ Captive Portal HTTP Server
    {
        std::cout << "[TEST 4] Testing Captive Portal HTTP Server... ";
        apm::portal::CaptivePortal portal;
        bool started = portal.start("Google_Modern", 9099);
        assert(started);
        assert(portal.is_running());

        // Wait a tiny bit for server loop
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Test GET /
        std::string root_resp = send_http_request(9099, "GET", "/");
        assert(root_resp.find("HTTP/1.1 200 OK") != std::string::npos);
        assert(root_resp.find("Google") != std::string::npos || root_resp.find("html") != std::string::npos);

        // Test GET /generate_204 (Android captive detection)
        std::string android_resp = send_http_request(9099, "GET", "/generate_204");
        assert(android_resp.find("HTTP/1.1 200 OK") != std::string::npos);

        // Test GET /ncsi.txt (Windows captive detection)
        std::string win_resp = send_http_request(9099, "GET", "/ncsi.txt");
        assert(win_resp.find("HTTP/1.1 200 OK") != std::string::npos);

        // Test POST /login (Form submission)
        std::string login_resp = send_http_request(9099, "POST", "/login", "user=guest&pass=123");
        assert(login_resp.find("HTTP/1.1 200 OK") != std::string::npos);
        assert(portal.is_client_authorized("127.0.0.1"));

        portal.stop();
        assert(!portal.is_running());
        std::cout << "PASSED\n";
    }

    std::cout << "======================================================\n";
    std::cout << "   ALL AP-GENERATOR INTEGRATION TESTS PASSED (4/4)    \n";
    std::cout << "======================================================\n";
    return 0;
}

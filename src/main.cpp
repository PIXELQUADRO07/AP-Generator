#include <iostream>
#include <memory>
#include <string>

#include "apmanager/core/wifi_backend.hpp"

namespace {

    void print_interfaces(const std::vector<apm::WifiInterface>& interfaces) {
        std::cout << "Wi-Fi Interfaces\n";
        std::cout << "----------------------------------------\n\n";

        if (interfaces.empty()) {
            std::cout << "Nessuna interfaccia wireless trovata.\n";
            return;
        }

        for (const auto& iface : interfaces) {
            std::cout << iface.name << "\n";
            std::cout << "  MAC:       " << iface.mac << "\n";
            std::cout << "  State:     " << (iface.up ? "UP" : "DOWN") << "\n";
            std::cout << "  Connected: " << (iface.connected ? "YES" : "NO") << "\n";
            std::cout << "  AP mode:   " << (iface.supports_ap ? "YES" : "?") << "\n";
            std::cout << "  WPA3:      " << (iface.supports_wpa3 ? "YES" : "?") << "\n";
            std::cout << "\n";
        }
    }

    void print_usage() {
        std::cout << "Uso: ap-generator <comando>\n\n";
        std::cout << "Comandi disponibili:\n";
        std::cout << "  interfaces   Elenca le interfacce Wi-Fi rilevate\n";
    }

}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const std::string command = argv[1];


    std::unique_ptr<apm::WifiBackend> backend =
        std::make_unique<apm::LinuxWifiBackend>();

    if (command == "interfaces") {
        print_interfaces(backend->discover_interfaces());
        return 0;
    }

    std::cout << "Comando sconosciuto: " << command << "\n\n";
    print_usage();
    return 1;
}
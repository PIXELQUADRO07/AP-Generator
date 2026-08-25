#include "apgen/ui/banner.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <iomanip>
#include <csignal>
#include <thread>
#include <chrono>

#include "apmanager/core/types.hpp"
#include "apmanager/core/wifi_backend.hpp"
#include "apmanager/core/ap_manager.hpp"
#include "apmanager/core/preset_manager.hpp"
#include "portal/captive_portal.hpp"

namespace {

void print_interfaces(const std::vector<apm::WifiInterface>& interfaces) {
    std::cout << "========================================\n";
    std::cout << "        Wi-Fi Interfaces Detected       \n";
    std::cout << "========================================\n\n";

    if (interfaces.empty()) {
        std::cout << "Nessuna interfaccia wireless rilevata nel sistema.\n";
        return;
    }

    for (const auto& iface : interfaces) {
        std::cout << "Interface: " << iface.name << "\n";
        std::cout << "  MAC Address:  " << iface.mac << "\n";
        std::cout << "  Driver:       " << (iface.driver.empty() ? "N/A" : iface.driver) << "\n";
        std::cout << "  PHY:          " << (iface.phy_name.empty() ? "N/A" : iface.phy_name) << "\n";
        std::cout << "  State:        " << (iface.up ? "UP" : "DOWN") << "\n";
        std::cout << "  Connected:    " << (iface.connected ? "YES" : "NO") << "\n";
        if (!iface.ip_address.empty()) {
            std::cout << "  IP Address:   " << iface.ip_address << "\n";
        }
        std::cout << "  AP Mode:      " << (iface.supports_ap ? "YES" : "NO") << "\n";
        std::cout << "  WPA3 (SAE):   " << (iface.supports_wpa3 ? "YES" : "NO") << "\n";
        std::cout << "  Bands:        ";
        if (iface.bands.empty()) {
            std::cout << "Unknown";
        } else {
            for (size_t i = 0; i < iface.bands.size(); ++i) {
                std::cout << iface.bands[i] << (i + 1 < iface.bands.size() ? ", " : "");
            }
        }
        std::cout << "\n\n";
    }
}

void print_capabilities(const apm::WifiInterface& iface) {
    if (iface.name.empty()) {
        std::cerr << "Errore: Interfaccia specificata non trovata.\n";
        return;
    }

    std::cout << "========================================\n";
    std::cout << "  Capabilities for: " << iface.name << "\n";
    std::cout << "========================================\n\n";

    std::cout << "Hardware & Network Details:\n";
    std::cout << "  MAC Address:          " << iface.mac << "\n";
    std::cout << "  Driver:               " << (iface.driver.empty() ? "Unknown" : iface.driver) << "\n";
    std::cout << "  PHY Device:           " << (iface.phy_name.empty() ? "Unknown" : iface.phy_name) << "\n";
    std::cout << "  Link State:           " << (iface.up ? "UP" : "DOWN") << "\n";
    std::cout << "  Current IP:           " << (iface.ip_address.empty() ? "None" : iface.ip_address) << "\n\n";

    std::cout << "Wireless Features:\n";
    std::cout << "  Access Point (AP):    " << (iface.supports_ap ? "SUPPORTED" : "UNSUPPORTED") << "\n";
    std::cout << "  WPA2-Personal:        " << (iface.supports_wpa2 ? "SUPPORTED" : "UNSUPPORTED") << "\n";
    std::cout << "  WPA3-Personal (SAE):  " << (iface.supports_wpa3 ? "SUPPORTED" : "UNSUPPORTED") << "\n";
    std::cout << "  AP + STA Concurrency: " << (iface.supports_concurrent_ap_sta ? "SUPPORTED" : "NO / UNKNOWN") << "\n\n";

    std::cout << "Supported Bands & Channels:\n";
    if (!iface.channels_2ghz.empty()) {
        std::cout << "  2.4 GHz Channels (" << iface.channels_2ghz.size() << "): ";
        for (size_t i = 0; i < iface.channels_2ghz.size(); ++i) {
            std::cout << iface.channels_2ghz[i] << (i + 1 < iface.channels_2ghz.size() ? ", " : "");
        }
        std::cout << "\n";
    }
    if (!iface.channels_5ghz.empty()) {
        std::cout << "  5 GHz Channels (" << iface.channels_5ghz.size() << "): ";
        for (size_t i = 0; i < iface.channels_5ghz.size(); ++i) {
            std::cout << iface.channels_5ghz[i] << (i + 1 < iface.channels_5ghz.size() ? ", " : "");
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

void print_usage() {
    std::cout << "======================================================\n";
    std::cout << "               AP-Generator CLI                       \n";
    std::cout << "  Wi-Fi Access Point and Network Management Tool      \n";
    std::cout << "======================================================\n\n";
    std::cout << "Uso: ap-generator <comando> [opzioni]\n\n";
    std::cout << "Comandi Interfaccia:\n";
    std::cout << "  interfaces                     Elenca tutte le interfacce Wi-Fi rilevate\n";
    std::cout << "  capabilities <interfaccia>     Mostra le capacità dettagliate della scheda\n\n";
    std::cout << "Comandi Configurazione & Preset:\n";
    std::cout << "  preset list                    Elenca tutti i preset salvati\n";
    std::cout << "  preset show <nome>             Visualizza la configurazione di un preset\n";
    std::cout << "  preset save <nome> [opzioni]   Crea o salva un preset di configurazione\n";
    std::cout << "  preset delete <nome>           Elimina un preset esistente\n";
    std::cout << "  validate <nome|file.json>      Verifica la validità di una configurazione\n\n";
    std::cout << "Comandi Captive Portal:\n";
    std::cout << "  portal list                    Elenca i template HTML disponibili\n";
    std::cout << "  portal test <template> [--port <p>]  Avvia il server HTTP per testare un template\n\n";
    std::cout << "Comandi Access Point:\n";
    std::cout << "  start [nome_preset]            Avvia l'Access Point (usando un preset)\n";
    std::cout << "  stop                           Arresta l'Access Point attivo\n";
    std::cout << "  status                         Mostra lo stato dell'Access Point e i client\n";
    std::cout << "  clients                        Elenca i client connessi con DHCP leases\n\n";
    std::cout << "Opzioni per 'preset save':\n";
    std::cout << "  --ssid <nome>                  Nome della rete Wi-Fi (SSID)\n";
    std::cout << "  --interface <iface>            Interfaccia wireless (es. wlan0)\n";
    std::cout << "  --channel <1-165>              Canale radio Wi-Fi (default: 6)\n";
    std::cout << "  --security <open|wpa2|wpa3>    Modalità di sicurezza (default: wpa2)\n";
    std::cout << "  --password <password>          Password WPA2/WPA3 (8-63 caratteri)\n";
    std::cout << "  --sharing                      Abilita la condivisione internet (NAT)\n";
    std::cout << "  --upstream <iface>             Interfaccia con connessione Internet (es. eth0)\n";
    std::cout << "  --portal                       Abilita il Captive Portal\n";
    std::cout << "  --portal-path <path>           Nome template o percorso captive portal\n\n";
    std::cout << "Esempio:\n";
    std::cout << "  ap-generator preset save GuestHotspot --interface wlan0 --ssid FreeGuest --security open --portal --portal-path Google_Modern\n";
    std::cout << "  ap-generator start GuestHotspot\n";
}

} // namespace

int main(int argc, char** argv) {
    apgen::ui::print_banner();
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const std::string command = argv[1];

    if (command == "help" || command == "--help" || command == "-h") {
        print_usage();
        return 0;
    }

    auto backend = std::make_unique<apm::LinuxWifiBackend>();
    apm::ApManager ap_mgr(std::move(backend));
    apm::PresetManager preset_mgr;
    apm::portal::CaptivePortal portal_mgr;

    if (command == "interfaces") {
        print_interfaces(ap_mgr.list_interfaces());
        return 0;
    }

    if (command == "capabilities") {
        if (argc < 3) {
            std::cerr << "Errore: specificare il nome dell'interfaccia. Es: ap-generator capabilities wlan0\n";
            return 1;
        }
        std::string iface_name = argv[2];
        print_capabilities(ap_mgr.get_interface_capabilities(iface_name));
        return 0;
    }

    if (command == "portal") {
        if (argc < 3) {
            std::cerr << "Uso: ap-generator portal <list|test <template>>\n";
            return 1;
        }
        std::string subcmd = argv[2];

        if (subcmd == "list") {
            auto templates = portal_mgr.list_available_templates();
            std::cout << "========================================\n";
            std::cout << "   Captive Portal Templates (" << templates.size() << ")\n";
            std::cout << "========================================\n\n";
            for (const auto& t : templates) {
                std::cout << "  * " << t << "\n";
            }
            std::cout << "\nPuoi visualizzarne un'anteprima con: ap-generator portal test <nome_template>\n";
            return 0;
        }

        if (subcmd == "test") {
            if (argc < 4) {
                std::cerr << "Errore: specificare il template da testare. Es: ap-generator portal test Google_Modern\n";
                return 1;
            }
            std::string tpl = argv[3];
            int port = 8080;
            for (int i = 4; i < argc; ++i) {
                if (std::string(argv[i]) == "--port" && i + 1 < argc) {
                    port = std::stoi(argv[++i]);
                }
            }

            std::string resolved = portal_mgr.resolve_template_path(tpl);
            std::cout << "======================================================\n";
            std::cout << "       Avvio Captive Portal Server di Test            \n";
            std::cout << "======================================================\n";
            std::cout << "Template: " << tpl << " (" << resolved << ")\n";
            std::cout << "URL:      http://127.0.0.1:" << port << "/\n";
            std::cout << "Premi CTRL+C per arrestare il server...\n\n";

            if (!portal_mgr.start(tpl, port)) {
                std::cerr << "Errore avvio server HTTP su porta " << port << ".\n";
                return 1;
            }

            // Loop until signal or input
            while (portal_mgr.is_running()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            return 0;
        }

        std::cerr << "Sottocomando portal sconosciuto: " << subcmd << "\n";
        return 1;
    }

    if (command == "preset") {
        if (argc < 3) {
            std::cerr << "Uso: ap-generator preset <list|show|save|delete> [opzioni]\n";
            return 1;
        }
        std::string subcmd = argv[2];

        if (subcmd == "list") {
            auto list = preset_mgr.list_presets();
            std::cout << "Preset disponibili (" << list.size() << ") in " << preset_mgr.get_presets_directory() << ":\n";
            if (list.empty()) {
                std::cout << "  (Nessun preset salvato)\n";
            } else {
                for (const auto& p : list) {
                    std::cout << "  * " << p << "\n";
                }
            }
            return 0;
        }

        if (subcmd == "show") {
            if (argc < 4) {
                std::cerr << "Errore: specificare il nome del preset. Es: ap-generator preset show <nome>\n";
                return 1;
            }
            std::string name = argv[3];
            apm::AccessPointConfig cfg;
            std::string err;
            if (!preset_mgr.load_preset(name, cfg, &err)) {
                std::cerr << "Errore: " << err << "\n";
                return 1;
            }
            std::cout << preset_mgr.serialize_config_json(cfg) << "\n";
            return 0;
        }

        if (subcmd == "delete") {
            if (argc < 4) {
                std::cerr << "Errore: specificare il nome del preset da eliminare.\n";
                return 1;
            }
            std::string name = argv[3];
            std::string err;
            if (!preset_mgr.delete_preset(name, &err)) {
                std::cerr << "Errore: " << err << "\n";
                return 1;
            }
            std::cout << "Preset '" << name << "' eliminato con successo.\n";
            return 0;
        }

        if (subcmd == "save") {
            if (argc < 4) {
                std::cerr << "Errore: specificare il nome del preset. Es: ap-generator preset save <nome> [opzioni]\n";
                return 1;
            }
            std::string name = argv[3];
            apm::AccessPointConfig cfg;
            cfg.name = name;

            for (int i = 4; i < argc; ++i) {
                std::string arg = argv[i];
                if (arg == "--ssid" && i + 1 < argc) cfg.ssid = argv[++i];
                else if (arg == "--interface" && i + 1 < argc) cfg.interface = argv[++i];
                else if (arg == "--channel" && i + 1 < argc) cfg.channel = std::stoi(argv[++i]);
                else if (arg == "--security" && i + 1 < argc) cfg.security = apm::security_mode_from_string(argv[++i]);
                else if (arg == "--password" && i + 1 < argc) cfg.password = argv[++i];
                else if (arg == "--sharing") cfg.internet_sharing = true;
                else if (arg == "--upstream" && i + 1 < argc) cfg.upstream_interface = argv[++i];
                else if (arg == "--portal") cfg.captive_portal = true;
                else if (arg == "--portal-path" && i + 1 < argc) cfg.portal_path = argv[++i];
                else if (arg == "--gateway" && i + 1 < argc) cfg.gateway_ip = argv[++i];
            }

            if (cfg.ssid.empty()) {
                cfg.ssid = name;
            }

            std::string err;
            if (!preset_mgr.save_preset(name, cfg, &err)) {
                std::cerr << "Errore salvataggio preset: " << err << "\n";
                return 1;
            }
            std::cout << "Preset '" << name << "' salvato con successo in " << preset_mgr.get_presets_directory() << "/" << name << ".json\n";
            return 0;
        }

        std::cerr << "Sottocomando preset sconosciuto: " << subcmd << "\n";
        return 1;
    }

    if (command == "validate") {
        if (argc < 3) {
            std::cerr << "Errore: specificare il nome del preset o il file json da validare.\n";
            return 1;
        }
        std::string target = argv[2];
        apm::AccessPointConfig cfg;
        std::string err;
        if (!preset_mgr.load_preset(target, cfg, &err)) {
            std::cerr << "Errore caricamento configurazione: " << err << "\n";
            return 1;
        }

        apm::ValidationResult res = ap_mgr.validate(cfg);
        if (res) {
            std::cout << "Configurazione '" << target << "' VALIDA!\n";
            std::cout << "  Interface: " << cfg.interface << "\n";
            std::cout << "  SSID:      " << cfg.ssid << "\n";
            std::cout << "  Channel:   " << cfg.channel << "\n";
            std::cout << "  Security:  " << apm::security_mode_to_string(cfg.security) << "\n";
            std::cout << "  Sharing:   " << (cfg.internet_sharing ? "YES (" + cfg.upstream_interface + ")" : "NO") << "\n";
            std::cout << "  Portal:    " << (cfg.captive_portal ? "YES (" + cfg.portal_path + ")" : "NO") << "\n";
            return 0;
        } else {
            std::cerr << "Configurazione NON VALIDA:\n  " << res.error << "\n";
            return 1;
        }
    }

    if (command == "status") {
        apm::ApStatus status = ap_mgr.get_status();
        std::cout << "========================================\n";
        std::cout << "        AP-Generator Status             \n";
        std::cout << "========================================\n";
        std::cout << "Status:    " << (status.running ? "RUNNING" : "STOPPED") << "\n";
        if (status.running) {
            std::cout << "SSID:      " << status.config.ssid << "\n";
            std::cout << "Interface: " << status.config.interface << "\n";
            std::cout << "Channel:   " << status.config.channel << "\n";
            std::cout << "Security:  " << apm::security_mode_to_string(status.config.security) << "\n";
            std::cout << "Gateway:   " << status.config.gateway_ip << "\n";
            std::cout << "Sharing:   " << (status.config.internet_sharing ? "YES (" + status.config.upstream_interface + ")" : "NO") << "\n";
            std::cout << "Portal:    " << (status.config.captive_portal ? "YES (" + status.config.portal_path + ")" : "NO") << "\n";
            std::cout << "Uptime:    " << status.uptime << "\n";
            std::cout << "Clients:   " << status.connected_clients.size() << "\n";
        }
        return 0;
    }

    if (command == "clients") {
        apm::ApStatus status = ap_mgr.get_status();
        std::cout << "========================================\n";
        std::cout << "        Connected Clients List          \n";
        std::cout << "========================================\n";
        if (!status.running) {
            std::cout << "Nessun Access Point attivo.\n";
            return 0;
        }
        if (status.connected_clients.empty()) {
            std::cout << "Nessun client connesso o lease DHCP attivo.\n";
            return 0;
        }
        std::cout << std::left << std::setw(20) << "MAC Address"
                  << std::setw(18) << "IP Address"
                  << std::setw(20) << "Hostname"
                  << "Expiry\n";
        std::cout << "----------------------------------------------------------------\n";
        for (const auto& c : status.connected_clients) {
            std::cout << std::left << std::setw(20) << c.mac
                      << std::setw(18) << c.ip
                      << std::setw(20) << c.hostname
                      << c.connected_since << "\n";
        }
        return 0;
    }

    if (command == "start") {
        apm::AccessPointConfig cfg;
        if (argc >= 3) {
            std::string preset_name = argv[2];
            std::string err;
            if (!preset_mgr.load_preset(preset_name, cfg, &err)) {
                std::cerr << "Errore caricamento preset '" << preset_name << "': " << err << "\n";
                return 1;
            }
        } else {
            std::cerr << "Specificare il nome di un preset da avviare. Es: ap-generator start MyPreset\n";
            return 1;
        }

        std::string err;
        if (!ap_mgr.start(cfg, &err)) {
            std::cerr << "Errore avvio AP: " << err << "\n";
            return 1;
        }
        std::cout << "Access Point avviato con successo su " << cfg.interface << " (SSID: " << cfg.ssid << ").\n";
        return 0;
    }

    if (command == "stop") {
        std::string err;
        if (!ap_mgr.stop(&err)) {
            std::cerr << "Errore arresto AP: " << err << "\n";
            return 1;
        }
        std::cout << "Access Point arrestato.\n";
        return 0;
    }

    std::cout << "Comando sconosciuto: " << command << "\n\n";
    print_usage();
    return 1;
}
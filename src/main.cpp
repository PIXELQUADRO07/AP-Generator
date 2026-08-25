#include "apgen/ui/banner.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <iomanip>
#include <csignal>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>

#include "apmanager/core/types.hpp"
#include "apmanager/core/wifi_backend.hpp"
#include "apmanager/core/ap_manager.hpp"
#include "apmanager/core/preset_manager.hpp"
#include "apmanager/core/i18n.hpp"
#include "portal/captive_portal.hpp"

#if defined(__linux__)
#include "linux/client_monitor.hpp"
#endif

namespace {

using apm::I18n;

std::vector<std::string> split_command_args(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;
    char quote_char = '\0';

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if ((c == '"' || c == '\'') && !in_quotes) {
            in_quotes = true;
            quote_char = c;
        } else if (in_quotes && c == quote_char) {
            in_quotes = false;
            quote_char = '\0';
        } else if (std::isspace(static_cast<unsigned char>(c)) && !in_quotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

std::string format_bytes(int64_t bytes) {
    if (bytes <= 0) return "0 B";
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    double b = static_cast<double>(bytes);
    while (b >= 1024.0 && u < 4) {
        b /= 1024.0;
        u++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << b << " " << units[u];
    return oss.str();
}

void print_interfaces(const std::vector<apm::WifiInterface>& interfaces) {
    std::cout << "========================================\n";
    std::cout << "  " << I18n::tr("iface_header") << " (" << interfaces.size() << ")\n";
    std::cout << "========================================\n\n";

    if (interfaces.empty()) {
        std::cout << "  " << I18n::tr("iface_none") << "\n\n";
        return;
    }

    for (const auto& iface : interfaces) {
        std::cout << "Interface: " << iface.name << "\n";
        std::cout << "  " << std::left << std::setw(22) << I18n::tr("iface_mac") << iface.mac << "\n";
        std::cout << "  " << std::left << std::setw(22) << I18n::tr("iface_driver") << (iface.driver.empty() ? "N/A" : iface.driver) << "\n";
        std::cout << "  " << std::left << std::setw(22) << I18n::tr("iface_phy") << (iface.phy_name.empty() ? "N/A" : iface.phy_name) << "\n";
        std::cout << "  " << std::left << std::setw(22) << I18n::tr("iface_state") << (iface.up ? "UP" : "DOWN") << "\n";
        std::cout << "  " << std::left << std::setw(22) << I18n::tr("iface_connected") << (iface.connected ? "YES" : "NO") << "\n";
        if (!iface.ip_address.empty()) {
            std::cout << "  " << std::left << std::setw(22) << I18n::tr("iface_ip") << iface.ip_address << "\n";
        }
        std::cout << "  " << std::left << std::setw(22) << I18n::tr("iface_ap_mode") << (iface.supports_ap ? "YES" : "NO") << "\n";
        std::cout << "  " << std::left << std::setw(22) << I18n::tr("iface_wpa3") << (iface.supports_wpa3 ? "YES" : "NO") << "\n";
        std::cout << "  " << std::left << std::setw(22) << I18n::tr("iface_bands");
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
        std::cerr << I18n::tr("cap_not_found") << "\n";
        return;
    }

    std::cout << "========================================\n";
    std::cout << "  " << I18n::tr("cap_header") << " " << iface.name << "\n";
    std::cout << "========================================\n\n";

    std::cout << "Hardware & Network Details:\n";
    std::cout << "  " << std::left << std::setw(24) << I18n::tr("iface_mac") << iface.mac << "\n";
    std::cout << "  " << std::left << std::setw(24) << I18n::tr("iface_driver") << (iface.driver.empty() ? "Unknown" : iface.driver) << "\n";
    std::cout << "  " << std::left << std::setw(24) << I18n::tr("iface_phy") << (iface.phy_name.empty() ? "Unknown" : iface.phy_name) << "\n";
    std::cout << "  " << std::left << std::setw(24) << I18n::tr("iface_state") << (iface.up ? "UP" : "DOWN") << "\n";
    std::cout << "  " << std::left << std::setw(24) << I18n::tr("iface_ip") << (iface.ip_address.empty() ? "None" : iface.ip_address) << "\n\n";

    std::cout << "Wireless Features:\n";
    std::cout << "  " << std::left << std::setw(24) << I18n::tr("iface_ap_mode") << (iface.supports_ap ? "SUPPORTED" : "UNSUPPORTED") << "\n";
    std::cout << "  " << std::left << std::setw(24) << "WPA2-Personal:" << (iface.supports_wpa2 ? "SUPPORTED" : "UNSUPPORTED") << "\n";
    std::cout << "  " << std::left << std::setw(24) << I18n::tr("iface_wpa3") << (iface.supports_wpa3 ? "SUPPORTED" : "UNSUPPORTED") << "\n";
    std::cout << "  " << std::left << std::setw(24) << I18n::tr("iface_concurrency") << (iface.supports_concurrent_ap_sta ? "SUPPORTED" : "NO / UNKNOWN") << "\n\n";

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

void print_working_config(const apm::AccessPointConfig& cfg) {
    std::cout << "======================================================\n";
    std::cout << "        Configurazione Attiva in Memoria (Buffer)     \n";
    std::cout << "======================================================\n";
    std::cout << "  Interface:          " << (cfg.interface.empty() ? "(non impostata)" : cfg.interface) << "\n";
    std::cout << "  SSID:               " << (cfg.ssid.empty() ? "(non impostato)" : cfg.ssid) << "\n";
    std::cout << "  Channel:            " << cfg.channel << "\n";
    std::cout << "  Security:           " << apm::security_mode_to_string(cfg.security) << "\n";
    if (cfg.security != apm::SecurityMode::Open) {
        std::cout << "  Password:           " << (cfg.password.empty() ? "(non impostata)" : cfg.password) << "\n";
    }
    std::cout << "  Internet Sharing:   " << (cfg.internet_sharing ? ("SI (Upstream: " + (cfg.upstream_interface.empty() ? "auto" : cfg.upstream_interface) + ")") : "NO") << "\n";
    std::cout << "  Captive Portal:     " << (cfg.captive_portal ? ("SI (Template: " + (cfg.portal_path.empty() ? "default" : cfg.portal_path) + ")") : "NO") << "\n";
    std::cout << "  Gateway IP:         " << cfg.gateway_ip << " (" << cfg.netmask << ")\n";
    std::cout << "  DHCP Range:         " << cfg.dhcp_range_start << " - " << cfg.dhcp_range_end << "\n";
    if (!cfg.bssid.empty()) {
        std::cout << "  BSSID:              " << cfg.bssid << "\n";
    }
    std::cout << "======================================================\n";
    std::cout << "Modifica i parametri con: set <chiave> <valore>\n";
    std::cout << "Avvia subito con:        start\n\n";
}

void print_help() {
    std::cout << "=========================================================================\n";
    std::cout << "                  " << I18n::tr("help_title") << "\n";
    std::cout << "=========================================================================\n\n";

    std::cout << I18n::tr("help_cat_network") << "\n";
    std::cout << "  " << I18n::tr("help_interfaces") << "\n";
    std::cout << "  " << I18n::tr("help_capabilities") << "\n\n";

    std::cout << I18n::tr("help_cat_presets") << "\n";
    std::cout << "  " << I18n::tr("help_preset_list") << "\n";
    std::cout << "  " << I18n::tr("help_preset_show") << "\n";
    std::cout << "  " << I18n::tr("help_preset_save") << "\n";
    std::cout << "  " << I18n::tr("help_preset_delete") << "\n";
    std::cout << "  " << I18n::tr("help_validate") << "\n\n";

    std::cout << I18n::tr("help_cat_ap") << "\n";
    std::cout << "  " << I18n::tr("help_wizard") << "\n";
    std::cout << "  " << I18n::tr("help_set") << "\n";
    std::cout << "  " << I18n::tr("help_show") << "\n";
    std::cout << "  " << I18n::tr("help_start") << "\n";
    std::cout << "  " << I18n::tr("help_stop") << "\n";
    std::cout << "  " << I18n::tr("help_status") << "\n";
    std::cout << "  " << I18n::tr("help_clients") << "\n";
    std::cout << "  " << I18n::tr("help_kick") << "\n";
    std::cout << "  " << I18n::tr("help_blacklist") << "\n\n";

    std::cout << I18n::tr("help_cat_portal") << "\n";
    std::cout << "  " << I18n::tr("help_portal_list") << "\n";
    std::cout << "  " << I18n::tr("help_portal_test") << "\n\n";

    std::cout << I18n::tr("help_cat_system") << "\n";
    std::cout << "  " << I18n::tr("help_language") << "\n";
    std::cout << "  " << I18n::tr("help_clear") << "\n";
    std::cout << "  " << I18n::tr("help_help") << "\n";
    std::cout << "  " << I18n::tr("help_exit") << "\n\n";
}

void run_wizard(apm::ApManager& ap_mgr, apm::PresetManager& preset_mgr, apm::AccessPointConfig& working_cfg) {
    std::cout << "\n" << I18n::tr("wizard_title") << "\n";
    std::cout << I18n::tr("wizard_welcome") << "\n\n";

    auto interfaces = ap_mgr.list_interfaces();
    if (interfaces.empty()) {
        std::cerr << I18n::tr("iface_none") << "\n";
        return;
    }

    std::cout << I18n::tr("wizard_step_iface") << ":\n";
    for (size_t i = 0; i < interfaces.size(); ++i) {
        std::cout << "  [" << (i + 1) << "] " << interfaces[i].name 
                  << " (MAC: " << interfaces[i].mac 
                  << ", AP Support: " << (interfaces[i].supports_ap ? "YES" : "NO") << ")\n";
    }

    std::cout << "Scelta [1-" << interfaces.size() << "] (default 1): ";
    std::string iface_choice;
    std::getline(std::cin, iface_choice);
    int iface_idx = 0;
    if (!iface_choice.empty()) {
        try { iface_idx = std::stoi(iface_choice) - 1; } catch (...) {}
    }
    if (iface_idx < 0 || iface_idx >= static_cast<int>(interfaces.size())) {
        iface_idx = 0;
    }
    std::string selected_iface = interfaces[iface_idx].name;
    std::cout << "-> Selezionata interfaccia: " << selected_iface << "\n\n";

    // SSID
    std::cout << I18n::tr("wizard_step_ssid") << " (default: AP-Hotspot): ";
    std::string ssid;
    std::getline(std::cin, ssid);
    if (ssid.empty()) ssid = "AP-Hotspot";

    // Channel
    std::cout << "Canale Wi-Fi [1-14, 36-165] (default: 6): ";
    std::string ch_str;
    std::getline(std::cin, ch_str);
    int channel = 6;
    if (!ch_str.empty()) {
        try { channel = std::stoi(ch_str); } catch (...) {}
    }

    // Security Mode
    std::cout << "\n" << I18n::tr("wizard_step_sec") << " (default 2): ";
    std::string sec_choice;
    std::getline(std::cin, sec_choice);
    apm::SecurityMode sec_mode = apm::SecurityMode::WPA2;
    if (sec_choice == "1") sec_mode = apm::SecurityMode::Open;
    else if (sec_choice == "3") sec_mode = apm::SecurityMode::WPA3;

    std::string password;
    if (sec_mode != apm::SecurityMode::Open) {
        while (true) {
            std::cout << I18n::tr("wizard_step_pass") << " ";
            std::getline(std::cin, password);
            if (password.size() >= 8 && password.size() <= 63) {
                break;
            }
            std::cout << "Password non valida. Deve avere tra 8 e 63 caratteri.\n";
        }
    }

    // Internet Sharing
    std::cout << "\n" << I18n::tr("wizard_step_sharing") << " (default: n): ";
    std::string share_choice;
    std::getline(std::cin, share_choice);
    bool internet_sharing = (share_choice == "s" || share_choice == "S" || share_choice == "y" || share_choice == "Y");
    std::string upstream_iface;

    if (internet_sharing) {
        std::cout << I18n::tr("wizard_step_upstream") << " (es. eth0, wlan0): ";
        std::getline(std::cin, upstream_iface);
    }

    // Captive Portal
    std::cout << "\n" << I18n::tr("wizard_step_portal") << " (default: n): ";
    std::string portal_choice;
    std::getline(std::cin, portal_choice);
    bool captive_portal = (portal_choice == "s" || portal_choice == "S" || portal_choice == "y" || portal_choice == "Y");
    std::string portal_template;

    if (captive_portal) {
        apm::portal::CaptivePortal pmgr;
        auto templates = pmgr.list_available_templates();
        std::cout << I18n::tr("wizard_step_tpl") << ":\n";
        for (size_t i = 0; i < templates.size(); ++i) {
            std::cout << "  [" << (i + 1) << "] " << templates[i] << "\n";
        }
        std::cout << "Scelta template [1-" << templates.size() << "] (default 1): ";
        std::string tpl_choice;
        std::getline(std::cin, tpl_choice);
        int tpl_idx = 0;
        if (!tpl_choice.empty()) {
            try { tpl_idx = std::stoi(tpl_choice) - 1; } catch (...) {}
        }
        if (tpl_idx >= 0 && tpl_idx < static_cast<int>(templates.size())) {
            portal_template = templates[tpl_idx];
        } else if (!templates.empty()) {
            portal_template = templates[0];
        }
    }

    // Assemble Config
    apm::AccessPointConfig cfg;
    cfg.interface = selected_iface;
    cfg.ssid = ssid;
    cfg.channel = channel;
    cfg.security = sec_mode;
    cfg.password = password;
    cfg.internet_sharing = internet_sharing;
    cfg.upstream_interface = upstream_iface;
    cfg.captive_portal = captive_portal;
    cfg.portal_path = portal_template;

    // Validate
    auto val = ap_mgr.validate(cfg);
    if (!val) {
        std::cerr << "\n" << I18n::tr("preset_invalid") << " " << val.error << "\n";
        return;
    }

    working_cfg = cfg;

    // Save as preset?
    std::cout << "\n" << I18n::tr("wizard_step_save") << " ";
    std::string preset_name;
    std::getline(std::cin, preset_name);
    if (!preset_name.empty()) {
        cfg.name = preset_name;
        std::string err;
        if (preset_mgr.save_preset(preset_name, cfg, &err)) {
            std::cout << I18n::tr("preset_saved") << " " << preset_name << "\n";
        }
    }

    // Start now?
    std::cout << "\n" << I18n::tr("wizard_start_now") << " ";
    std::string start_choice;
    std::getline(std::cin, start_choice);
    if (start_choice == "s" || start_choice == "S" || start_choice == "y" || start_choice == "Y") {
        std::string err;
        if (ap_mgr.start(cfg, &err)) {
            std::cout << "\n" << I18n::tr("ap_start_success") << " " << cfg.interface << " (SSID: " << cfg.ssid << ")\n";
        } else {
            std::cerr << "\n" << I18n::tr("ap_start_fail") << " " << err << "\n";
        }
    }

    std::cout << I18n::tr("wizard_done") << "\n\n";
}

bool execute_command_tokens(const std::vector<std::string>& tokens,
                            apm::ApManager& ap_mgr,
                            apm::PresetManager& preset_mgr,
                            apm::portal::CaptivePortal& portal_mgr,
                            apm::AccessPointConfig& working_cfg) {
    if (tokens.empty()) return true;

    const std::string& command = tokens[0];

    if (command == "exit" || command == "quit" || command == "q") {
        std::cout << I18n::tr("goodbye") << "\n";
        return false;
    }

    if (command == "clear" || command == "cls") {
        std::cout << "\033[3J\033[H\033[2J";
        std::cout.flush();
        (void)std::system("clear");
        apgen::ui::print_banner();
        return true;
    }

    if (command == "help" || command == "?" || command == "--help" || command == "-h") {
        print_help();
        return true;
    }

    if (command == "language" || command == "lang") {
        if (tokens.size() < 2) {
            std::cout << I18n::tr("lang_current") << "\n";
            std::cout << "Uso: language <it|en>\n";
            return true;
        }
        I18n::instance().set_language_by_code(tokens[1]);
        std::cout << I18n::tr("lang_switched") << "\n";
        return true;
    }

    if (command == "interfaces" || command == "ifaces") {
        print_interfaces(ap_mgr.list_interfaces());
        return true;
    }

    if (command == "capabilities") {
        if (tokens.size() < 2) {
            std::cerr << "Errore: specificare il nome dell'interfaccia. Es: capabilities wlan0\n";
            return true;
        }
        print_capabilities(ap_mgr.get_interface_capabilities(tokens[1]));
        return true;
    }

    if (command == "wizard") {
        run_wizard(ap_mgr, preset_mgr, working_cfg);
        return true;
    }

    // SET command for manually modifying configuration variables
    if (command == "set" || command == "show") {
        if (command == "show" || tokens.size() == 1 || (tokens.size() == 2 && tokens[1] == "show")) {
            print_working_config(working_cfg);
            return true;
        }

        if (tokens.size() >= 2 && tokens[1] == "reset") {
            working_cfg = apm::AccessPointConfig();
            working_cfg.ssid = "AP-Hotspot";
            working_cfg.channel = 6;
            std::cout << "Configurazione reimpostata ai valori predefiniti.\n\n";
            return true;
        }

        if (tokens.size() < 3) {
            std::cout << "Uso del comando 'set':\n";
            std::cout << "  set interface <wlan0|wlp...>      - Imposta l'interfaccia wireless\n";
            std::cout << "  set ssid <Nome_Rete>              - Imposta il nome della rete\n";
            std::cout << "  set channel <1-165>               - Imposta il canale Wi-Fi\n";
            std::cout << "  set security <open|wpa2|wpa3>     - Imposta la sicurezza\n";
            std::cout << "  set password <passphrase>         - Imposta la password Wi-Fi\n";
            std::cout << "  set sharing <on|off>              - Abilita/disabilita condivisione Internet\n";
            std::cout << "  set upstream <eth0|wlan0>         - Imposta interfaccia con connessione Internet\n";
            std::cout << "  set portal <on|off>               - Abilita/disabilita Captive Portal\n";
            std::cout << "  set template <Google_Modern|...>  - Imposta il template Captive Portal\n";
            std::cout << "  set gateway <192.168.50.1>        - Imposta IP del Gateway\n";
            std::cout << "  set netmask <255.255.255.0>       - Imposta la maschera di rete\n";
            std::cout << "  set dhcp-start <192.168.50.10>    - Inizio range DHCP\n";
            std::cout << "  set dhcp-end <192.168.50.250>     - Fine range DHCP\n";
            std::cout << "  set bssid <XX:XX:XX:XX:XX:XX>     - Imposta BSSID (MAC custom)\n";
            std::cout << "  set show                          - Mostra la configurazione corrente\n";
            std::cout << "  set reset                         - Ripristina valori default\n";
            return true;
        }

        std::string key = tokens[1];
        std::string val = tokens[2];
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        if (key == "interface" || key == "iface" || key == "dev") {
            working_cfg.interface = val;
            std::cout << "✓ Interface impostata su: " << val << "\n";
        } else if (key == "ssid" || key == "name") {
            working_cfg.ssid = val;
            std::cout << "✓ SSID impostato su: " << val << "\n";
        } else if (key == "channel" || key == "chan" || key == "ch") {
            try {
                working_cfg.channel = std::stoi(val);
                std::cout << "✓ Canale impostato su: " << working_cfg.channel << "\n";
            } catch (...) {
                std::cerr << "Errore: numero di canale non valido.\n";
            }
        } else if (key == "security" || key == "sec" || key == "mode") {
            working_cfg.security = apm::security_mode_from_string(val);
            std::cout << "✓ Sicurezza impostata su: " << apm::security_mode_to_string(working_cfg.security) << "\n";
        } else if (key == "password" || key == "pass" || key == "key") {
            working_cfg.password = val;
            std::cout << "✓ Password impostata.\n";
        } else if (key == "sharing" || key == "nat" || key == "share") {
            std::string lval = val;
            std::transform(lval.begin(), lval.end(), lval.begin(), ::tolower);
            working_cfg.internet_sharing = (lval == "1" || lval == "true" || lval == "on" || lval == "si" || lval == "yes" || lval == "y");
            std::cout << "✓ Condivisione Internet: " << (working_cfg.internet_sharing ? "ABILITATA" : "DISABILITATA") << "\n";
        } else if (key == "upstream" || key == "wan" || key == "src") {
            working_cfg.upstream_interface = val;
            std::cout << "✓ Interfaccia upstream impostata su: " << val << "\n";
        } else if (key == "portal" || key == "captive") {
            std::string lval = val;
            std::transform(lval.begin(), lval.end(), lval.begin(), ::tolower);
            working_cfg.captive_portal = (lval == "1" || lval == "true" || lval == "on" || lval == "si" || lval == "yes" || lval == "y");
            std::cout << "✓ Captive Portal: " << (working_cfg.captive_portal ? "ABILITATO" : "DISABILITATO") << "\n";
        } else if (key == "template" || key == "portal-path" || key == "portal_path" || key == "tpl") {
            working_cfg.portal_path = val;
            std::cout << "✓ Template portal impostato su: " << val << "\n";
        } else if (key == "gateway" || key == "gw" || key == "ip") {
            working_cfg.gateway_ip = val;
            std::cout << "✓ Gateway IP impostato su: " << val << "\n";
        } else if (key == "netmask" || key == "mask") {
            working_cfg.netmask = val;
            std::cout << "✓ Netmask impostata su: " << val << "\n";
        } else if (key == "dhcp-start" || key == "dhcp_start" || key == "start_ip") {
            working_cfg.dhcp_range_start = val;
            std::cout << "✓ DHCP Range Start impostato su: " << val << "\n";
        } else if (key == "dhcp-end" || key == "dhcp_end" || key == "end_ip") {
            working_cfg.dhcp_range_end = val;
            std::cout << "✓ DHCP Range End impostato su: " << val << "\n";
        } else if (key == "bssid" || key == "mac") {
            working_cfg.bssid = val;
            std::cout << "✓ BSSID impostato su: " << val << "\n";
        } else {
            std::cerr << "Chiave sconosciuta: '" << key << "'. Digita 'set' per la lista dei parametri.\n";
        }
        return true;
    }

    if (command == "portal") {
        if (tokens.size() < 2) {
            std::cerr << "Uso: portal <list|test <template>>\n";
            return true;
        }
        const std::string& subcmd = tokens[1];

        if (subcmd == "list") {
            auto templates = portal_mgr.list_available_templates();
            std::cout << "========================================\n";
            std::cout << "  " << I18n::tr("portal_templates_title") << " (" << templates.size() << ")\n";
            std::cout << "========================================\n\n";
            for (const auto& t : templates) {
                std::cout << "  * " << t << "\n";
            }
            std::cout << "\nAnteprima: portal test <nome_template>\n";
            return true;
        }

        if (subcmd == "test") {
            if (tokens.size() < 3) {
                std::cerr << "Specificare il template da testare. Es: portal test Google_Modern\n";
                return true;
            }
            std::string tpl = tokens[2];
            int port = 8080;
            for (size_t i = 3; i < tokens.size(); ++i) {
                if (tokens[i] == "--port" && i + 1 < tokens.size()) {
                    port = std::stoi(tokens[++i]);
                }
            }

            std::string resolved = portal_mgr.resolve_template_path(tpl);
            std::cout << "\n======================================================\n";
            std::cout << "       Captive Portal Test Server (" << I18n::instance().get_language_name() << ")\n";
            std::cout << "======================================================\n";
            std::cout << "Template: " << tpl << " (" << resolved << ")\n";
            std::cout << "URL:      http://127.0.0.1:" << port << "/\n";
            std::cout << I18n::tr("portal_test_stop_hint") << "\n\n";

            if (!portal_mgr.start(tpl, port)) {
                std::cerr << "Errore avvio server HTTP su porta " << port << ".\n";
                return true;
            }

            // Wait for user input or interrupt
            std::string dummy;
            std::getline(std::cin, dummy);
            portal_mgr.stop();
            std::cout << "Server di test arrestato.\n";
            return true;
        }

        std::cerr << "Sottocomando portal sconosciuto: " << subcmd << "\n";
        return true;
    }

    if (command == "preset") {
        if (tokens.size() < 2) {
            std::cerr << "Uso: preset <list|show|save|delete> [opzioni]\n";
            return true;
        }
        const std::string& subcmd = tokens[1];

        if (subcmd == "list") {
            auto list = preset_mgr.list_presets();
            std::cout << "========================================\n";
            std::cout << "  " << I18n::tr("preset_list_title") << " (" << list.size() << ")\n";
            std::cout << "========================================\n";
            if (list.empty()) {
                std::cout << "  " << I18n::tr("preset_none") << "\n";
            } else {
                for (const auto& p : list) {
                    std::cout << "  * " << p << "\n";
                }
            }
            std::cout << "\n";
            return true;
        }

        if (subcmd == "show") {
            if (tokens.size() < 3) {
                std::cerr << "Specificare il nome del preset: preset show <nome>\n";
                return true;
            }
            apm::AccessPointConfig cfg;
            std::string err;
            if (!preset_mgr.load_preset(tokens[2], cfg, &err)) {
                std::cerr << I18n::tr("preset_load_fail") << " " << err << "\n";
                return true;
            }
            std::cout << preset_mgr.serialize_config_json(cfg) << "\n";
            return true;
        }

        if (subcmd == "delete") {
            if (tokens.size() < 3) {
                std::cerr << "Specificare il nome del preset da eliminare.\n";
                return true;
            }
            std::string err;
            if (!preset_mgr.delete_preset(tokens[2], &err)) {
                std::cerr << I18n::tr("preset_delete_fail") << " " << err << "\n";
                return true;
            }
            std::cout << I18n::tr("preset_deleted") << " '" << tokens[2] << "'\n";
            return true;
        }

        if (subcmd == "save") {
            std::string name = (tokens.size() >= 3) ? tokens[2] : (working_cfg.name.empty() ? working_cfg.ssid : working_cfg.name);
            if (name.empty()) {
                std::cerr << "Specificare il nome del preset: preset save <nome> [opzioni]\n";
                return true;
            }
            apm::AccessPointConfig cfg = working_cfg;
            cfg.name = name;

            for (size_t i = 3; i < tokens.size(); ++i) {
                const std::string& arg = tokens[i];
                if (arg == "--ssid" && i + 1 < tokens.size()) cfg.ssid = tokens[++i];
                else if (arg == "--interface" && i + 1 < tokens.size()) cfg.interface = tokens[++i];
                else if (arg == "--channel" && i + 1 < tokens.size()) cfg.channel = std::stoi(tokens[++i]);
                else if (arg == "--security" && i + 1 < tokens.size()) cfg.security = apm::security_mode_from_string(tokens[++i]);
                else if (arg == "--password" && i + 1 < tokens.size()) cfg.password = tokens[++i];
                else if (arg == "--sharing") cfg.internet_sharing = true;
                else if (arg == "--upstream" && i + 1 < tokens.size()) cfg.upstream_interface = tokens[++i];
                else if (arg == "--portal") cfg.captive_portal = true;
                else if (arg == "--portal-path" && i + 1 < tokens.size()) cfg.portal_path = tokens[++i];
                else if (arg == "--gateway" && i + 1 < tokens.size()) cfg.gateway_ip = tokens[++i];
            }

            if (cfg.ssid.empty()) cfg.ssid = name;

            std::string err;
            if (!preset_mgr.save_preset(name, cfg, &err)) {
                std::cerr << "Errore salvataggio: " << err << "\n";
                return true;
            }
            working_cfg = cfg;
            std::cout << I18n::tr("preset_saved") << " " << name << " (" << preset_mgr.get_presets_directory() << "/" << name << ".json)\n";
            return true;
        }

        std::cerr << "Sottocomando preset sconosciuto: " << subcmd << "\n";
        return true;
    }

    if (command == "validate") {
        apm::AccessPointConfig cfg = working_cfg;
        if (tokens.size() >= 2) {
            std::string err;
            if (!preset_mgr.load_preset(tokens[1], cfg, &err)) {
                std::cerr << I18n::tr("preset_load_fail") << " " << err << "\n";
                return true;
            }
        }
        auto res = ap_mgr.validate(cfg);
        if (res) {
            std::cout << I18n::tr("preset_valid") << " (" << cfg.ssid << " on " << cfg.interface << ")\n";
        } else {
            std::cerr << I18n::tr("preset_invalid") << " " << res.error << "\n";
        }
        return true;
    }

    if (command == "status") {
        auto status = ap_mgr.get_status();
        std::cout << "========================================\n";
        std::cout << "  " << I18n::tr("status_header") << "\n";
        std::cout << "========================================\n";
        std::cout << "Stato:     " << (status.running ? I18n::tr("running_status_active") : I18n::tr("running_status_stopped")) << "\n";
        if (status.running) {
            std::cout << "SSID:      " << status.config.ssid << "\n";
            std::cout << "Interface: " << status.config.interface << "\n";
            std::cout << "Channel:   " << status.config.channel << "\n";
            std::cout << "Security:  " << apm::security_mode_to_string(status.config.security) << "\n";
            std::cout << "Gateway:   " << status.config.gateway_ip << "\n";
            std::cout << "Sharing:   " << (status.config.internet_sharing ? "YES (" + status.config.upstream_interface + ")" : "NO") << "\n";
            std::cout << "Portal:    " << (status.config.captive_portal ? "YES (" + status.config.portal_path + ")" : "NO") << "\n";
            std::cout << I18n::tr("status_uptime") << " " << status.uptime << "\n";
            std::cout << I18n::tr("status_clients_count") << " " << status.connected_clients.size() << "\n";
        }
        std::cout << "\n";
        return true;
    }

    if (command == "clients") {
        auto status = ap_mgr.get_status();
        std::cout << "========================================================================================\n";
        std::cout << "  " << I18n::tr("clients_header") << "\n";
        std::cout << "========================================================================================\n";
        if (!status.running) {
            std::cout << "  " << I18n::tr("ap_not_running") << "\n\n";
            return true;
        }
        if (status.connected_clients.empty()) {
            std::cout << "  " << I18n::tr("clients_none") << "\n\n";
            return true;
        }

        std::cout << std::left << std::setw(19) << "MAC Address"
                  << std::setw(16) << "IP Address"
                  << std::setw(18) << "Hostname"
                  << std::setw(12) << "Signal"
                  << std::setw(12) << "RX Bytes"
                  << std::setw(12) << "TX Bytes"
                  << "\n";
        std::cout << "----------------------------------------------------------------------------------------\n";
        for (const auto& c : status.connected_clients) {
            std::string sig = (c.signal_dbm != 0) ? (std::to_string(c.signal_dbm) + " dBm") : "N/A";
            std::cout << std::left << std::setw(19) << c.mac
                      << std::setw(16) << (c.ip.empty() ? "Pending DHCP" : c.ip)
                      << std::setw(18) << (c.hostname.empty() ? "Unknown" : c.hostname)
                      << std::setw(12) << sig
                      << std::setw(12) << format_bytes(c.rx_bytes)
                      << std::setw(12) << format_bytes(c.tx_bytes)
                      << "\n";
        }
        std::cout << "\n";
        return true;
    }

    if (command == "kick") {
        if (tokens.size() < 2) {
            std::cerr << "Specificare il MAC address del client da disconnettere: kick <mac>\n";
            return true;
        }
#if defined(__linux__)
        auto status = ap_mgr.get_status();
        std::string iface = status.running ? status.config.interface : working_cfg.interface;
        apm::linux_backend::ClientMonitor cm(iface);
        std::string err;
        if (cm.kick_client(tokens[1], &err)) {
            std::cout << I18n::tr("kick_success") << " " << tokens[1] << "\n";
        } else {
            std::cerr << I18n::tr("kick_fail") << " " << err << "\n";
        }
#else
        std::cerr << "Kick command non supportato su questa piattaforma.\n";
#endif
        return true;
    }

    if (command == "blacklist") {
#if defined(__linux__)
        apm::linux_backend::ClientMonitor cm;
        if (tokens.size() < 2 || tokens[1] == "list") {
            auto list = cm.get_blacklist();
            std::cout << "========================================\n";
            std::cout << "  " << I18n::tr("blacklist_list") << " (" << list.size() << ")\n";
            std::cout << "========================================\n";
            if (list.empty()) {
                std::cout << "  " << I18n::tr("blacklist_empty") << "\n\n";
            } else {
                for (const auto& mac : list) {
                    std::cout << "  * " << mac << "\n";
                }
                std::cout << "\n";
            }
            return true;
        }

        std::string sub = tokens[1];
        if (sub == "add" && tokens.size() >= 3) {
            cm.blacklist_add(tokens[2]);
            std::cout << I18n::tr("blacklist_added") << " " << tokens[2] << "\n";
            return true;
        }
        if ((sub == "del" || sub == "remove") && tokens.size() >= 3) {
            if (cm.blacklist_remove(tokens[2])) {
                std::cout << I18n::tr("blacklist_removed") << " " << tokens[2] << "\n";
            } else {
                std::cerr << "MAC address non trovato nella blacklist.\n";
            }
            return true;
        }

        std::cerr << "Uso: blacklist <list|add <mac>|remove <mac>>\n";
#else
        std::cerr << "Blacklist non supportata su questa piattaforma.\n";
#endif
        return true;
    }

    if (command == "start") {
        apm::AccessPointConfig cfg;
        if (tokens.size() >= 2) {
            std::string preset_name = tokens[1];
            std::string err;
            if (!preset_mgr.load_preset(preset_name, cfg, &err)) {
                std::cerr << I18n::tr("preset_load_fail") << " " << err << "\n";
                return true;
            }
            working_cfg = cfg;
        } else {
            // Start using active in-memory working config
            cfg = working_cfg;
            if (cfg.interface.empty()) {
                auto ifaces = ap_mgr.list_interfaces();
                if (!ifaces.empty()) {
                    cfg.interface = ifaces[0].name;
                    working_cfg.interface = cfg.interface;
                }
            }
            if (cfg.ssid.empty()) {
                cfg.ssid = "AP-Hotspot";
                working_cfg.ssid = cfg.ssid;
            }
        }

        std::string err;
        if (!ap_mgr.start(cfg, &err)) {
            std::cerr << I18n::tr("ap_start_fail") << " " << err << "\n";
            return true;
        }
        std::cout << I18n::tr("ap_start_success") << " " << cfg.interface << " (SSID: " << cfg.ssid << ")\n";
        return true;
    }

    if (command == "stop") {
        std::string err;
        if (!ap_mgr.stop(&err)) {
            std::cerr << I18n::tr("ap_stop_fail") << " " << err << "\n";
            return true;
        }
        std::cout << I18n::tr("ap_stop_success") << "\n";
        return true;
    }

    std::cout << I18n::tr("unknown_command") << "\n";
    return true;
}

void start_interactive_shell(apm::ApManager& ap_mgr, apm::PresetManager& preset_mgr, apm::portal::CaptivePortal& portal_mgr) {
    std::cout << "\n=========================================================================\n";
    std::cout << "  " << I18n::tr("app_title") << "\n";
    std::cout << "  " << I18n::tr("lang_current") << " | Digita 'help' per i comandi | 'exit' per uscire\n";
    std::cout << "=========================================================================\n\n";

    apm::AccessPointConfig working_cfg;
    working_cfg.ssid = "AP-Hotspot";
    working_cfg.channel = 6;
    working_cfg.security = apm::SecurityMode::Open;

    // Default to first detected interface if available
    auto detected = ap_mgr.list_interfaces();
    if (!detected.empty()) {
        working_cfg.interface = detected[0].name;
    }

    std::string line;
    while (true) {
        std::cout << "ap-generator: ";
        std::cout.flush();

        if (!std::getline(std::cin, line)) {
            std::cout << "\n" << I18n::tr("goodbye") << "\n";
            break;
        }

        auto tokens = split_command_args(line);
        if (tokens.empty()) continue;

        bool keep_running = execute_command_tokens(tokens, ap_mgr, preset_mgr, portal_mgr, working_cfg);
        if (!keep_running) {
            break;
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    // Print ASCII Banner
    apgen::ui::print_banner();

    // Check optional leading language switch parameter (e.g. --lang en)
    int arg_start = 1;
    if (argc >= 3 && (std::string(argv[1]) == "--lang" || std::string(argv[1]) == "-l")) {
        I18n::instance().set_language_by_code(argv[2]);
        arg_start = 3;
    }

    auto backend = std::make_unique<apm::LinuxWifiBackend>();
    apm::ApManager ap_mgr(std::move(backend));
    apm::PresetManager preset_mgr;
    apm::portal::CaptivePortal portal_mgr;

    // No arguments -> Start Interactive Shell (REPL)
    if (arg_start >= argc) {
        start_interactive_shell(ap_mgr, preset_mgr, portal_mgr);
        return 0;
    }

    std::vector<std::string> tokens;
    for (int i = arg_start; i < argc; ++i) {
        tokens.push_back(argv[i]);
    }

    if (tokens[0] == "shell" || tokens[0] == "interactive" || tokens[0] == "repl") {
        start_interactive_shell(ap_mgr, preset_mgr, portal_mgr);
        return 0;
    }

    apm::AccessPointConfig cli_working_cfg;
    auto detected = ap_mgr.list_interfaces();
    if (!detected.empty()) {
        cli_working_cfg.interface = detected[0].name;
    }
    cli_working_cfg.ssid = "AP-Hotspot";

    // Execute one-shot CLI command
    execute_command_tokens(tokens, ap_mgr, preset_mgr, portal_mgr, cli_working_cfg);
    return 0;
}
#include "apmanager/core/i18n.hpp"

#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace apm {

I18n::I18n() {
    load_translations();
    load_language_preference();
}

void I18n::detect_system_language() {
    const char* lang_env = std::getenv("LANG");
    if (!lang_env) lang_env = std::getenv("LC_ALL");
    if (!lang_env) lang_env = std::getenv("LC_MESSAGES");

    if (lang_env) {
        std::string s(lang_env);
        if (s.rfind("it", 0) == 0 || s.rfind("IT", 0) == 0) {
            current_lang_ = Language::IT;
            return;
        }
    }
    // Default to English if not Italian
    current_lang_ = Language::EN;
}

void I18n::load_language_preference() {
    std::string config_path = "config/settings.json";
    if (fs::exists(config_path)) {
        std::ifstream in(config_path);
        if (in.is_open()) {
            std::string line;
            while (std::getline(in, line)) {
                if (line.find("\"language\"") != std::string::npos) {
                    if (line.find("\"it\"") != std::string::npos || line.find("\"IT\"") != std::string::npos) {
                        current_lang_ = Language::IT;
                        return;
                    } else if (line.find("\"en\"") != std::string::npos || line.find("\"EN\"") != std::string::npos) {
                        current_lang_ = Language::EN;
                        return;
                    }
                }
            }
        }
    }
    detect_system_language();
}

void I18n::save_language_preference() const {
    std::error_code ec;
    fs::create_directories("config", ec);
    std::ofstream out("config/settings.json");
    if (out.is_open()) {
        out << "{\n";
        out << "  \"language\": \"" << get_language_code() << "\"\n";
        out << "}\n";
    }
}

void I18n::set_language(Language lang) {
    current_lang_ = lang;
    save_language_preference();
}

void I18n::set_language_by_code(const std::string& code) {
    std::string lower = code;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "it" || lower == "ita" || lower == "italian" || lower == "italiano") {
        set_language(Language::IT);
    } else {
        set_language(Language::EN);
    }
}

std::string I18n::get_language_code() const {
    return (current_lang_ == Language::IT) ? "it" : "en";
}

std::string I18n::get_language_name() const {
    return (current_lang_ == Language::IT) ? "Italiano" : "English";
}

std::string I18n::get(const std::string& key) const {
    const auto& dict = (current_lang_ == Language::IT) ? dict_it_ : dict_en_;
    auto it = dict.find(key);
    if (it != dict.end()) {
        return it->second;
    }
    // Fallback to English dict if missing in Italian
    auto it_en = dict_en_.find(key);
    if (it_en != dict_en_.end()) {
        return it_en->second;
    }
    return key;
}

void I18n::load_translations() {
    // ==========================================
    // ITALIAN DICTIONARY
    // ==========================================
    dict_it_ = {
        // App General
        {"app_title", "AP-Generator - Strumento Avanzato di Gestione Access Point Wi-Fi"},
        {"prompt_label", "ap-generator"},
        {"lang_switched", "Lingua impostata su: Italiano"},
        {"lang_current", "Lingua attuale: Italiano"},
        {"goodbye", "Arrivederci! Chiusura di AP-Generator."},
        {"unknown_command", "Comando sconosciuto. Digita 'help' o '?' per visualizzare i comandi disponibili."},
        {"running_status_active", "ATTIVO (RUNNING)"},
        {"running_status_stopped", "ARRESTATO (STOPPED)"},

        // Help Menu
        {"help_title", "COMANDI DISPONIBILI NELLA SHELL INTERATTIVA"},
        {"help_cat_network", "--- Rete & Hardware ---"},
        {"help_interfaces", "interfaces              - Mostra tutte le interfacce Wi-Fi rilevate"},
        {"help_capabilities", "capabilities <iface>    - Mostra le capacità dettagliate della scheda"},
        {"help_cat_presets", "--- Gestione Preset ---"},
        {"help_preset_list", "preset list             - Elenca i preset salvati"},
        {"help_preset_show", "preset show <nome>      - Mostra la configurazione di un preset"},
        {"help_preset_save", "preset save <nome> ...  - Salva un nuovo preset di configurazione"},
        {"help_preset_delete", "preset delete <nome>    - Elimina un preset"},
        {"help_validate", "validate <nome|file>    - Valida i parametri di una configurazione"},
        {"help_cat_ap", "--- Gestione Access Point ---"},
        {"help_start", "start <preset>          - Avvia l'Access Point specificato"},
        {"help_stop", "stop                    - Arresta l'Access Point attivo"},
        {"help_status", "status                  - Mostra lo stato dell'Access Point e i client"},
        {"help_clients", "clients                 - Mostra i client connessi e i dati di rete"},
        {"help_kick", "kick <mac>              - Disconnette forzatamente un client connesso"},
        {"help_blacklist", "blacklist <add|del|list>- Gestisce la lista nera dei MAC address"},
        {"help_wizard", "wizard                  - Procedura guidata interattiva passo-passo"},
        {"help_set", "set [chiave] [valore]   - Modifica o visualizza le variabili di configurazione"},
        {"help_show", "show                    - Mostra la configurazione corrente in memoria"},
        {"help_cat_portal", "--- Captive Portal ---"},
        {"help_portal_list", "portal list             - Elenca i template HTML disponibili"},
        {"help_portal_test", "portal test <tpl>       - Avvia server HTTP di prova per il template"},
        {"help_cat_system", "--- Sistema & Utilità ---"},
        {"help_language", "language <it|en>        - Cambia lingua dell'applicazione"},
        {"help_clear", "clear                   - Pulisce lo schermo del terminale"},
        {"help_help", "help, ?                 - Mostra questa guida ai comandi"},
        {"help_exit", "exit, quit, q           - Esci dall'applicazione"},

        // Interfaces & Capabilities
        {"iface_header", "Interfacce Wi-Fi Rilevate"},
        {"iface_none", "Nessuna interfaccia wireless rilevata nel sistema."},
        {"iface_mac", "Indirizzo MAC:"},
        {"iface_driver", "Driver:"},
        {"iface_phy", "Dispositivo PHY:"},
        {"iface_state", "Stato:"},
        {"iface_connected", "Connesso:"},
        {"iface_ip", "Indirizzo IP:"},
        {"iface_ap_mode", "Supporto AP Mode:"},
        {"iface_wpa3", "Supporto WPA3 (SAE):"},
        {"iface_bands", "Bande di frequenza:"},
        {"iface_concurrency", "Concorrenza AP+STA:"},
        {"cap_header", "Capacità Wireless per:"},
        {"cap_not_found", "Errore: Interfaccia specificata non trovata."},

        // AP Control
        {"ap_start_success", "Access Point avviato con successo su interfaccia"},
        {"ap_start_fail", "Errore durante l'avvio dell'Access Point:"},
        {"ap_stop_success", "Access Point arrestato e configurazione di rete ripristinata."},
        {"ap_stop_fail", "Errore durante l'arresto dell'Access Point:"},
        {"ap_already_running", "Un Access Point è già attivo. Arrestalo prima di avviarne un altro."},
        {"ap_not_running", "Nessun Access Point attualmente in esecuzione."},
        {"ap_specify_preset", "Specificare il nome di un preset da avviare. Es: start LabHotspot"},

        // Status & Clients
        {"status_header", "Stato Access Point"},
        {"status_uptime", "Tempo di attività (Uptime):"},
        {"status_clients_count", "Client Connessi:"},
        {"clients_header", "Client Connessi e Lease DHCP"},
        {"clients_none", "Nessun client connesso o lease attivo al momento."},
        {"kick_success", "Client disconnesso con successo:"},
        {"kick_fail", "Impossibile disconnettere il client:"},
        {"blacklist_added", "MAC aggiunto alla lista nera:"},
        {"blacklist_removed", "MAC rimosso dalla lista nera:"},
        {"blacklist_list", "Lista Nera MAC Address (Blacklist):"},
        {"blacklist_empty", "Nessun indirizzo MAC nella lista nera."},

        // Presets
        {"preset_list_title", "Preset di Configurazione Disponibili"},
        {"preset_none", "(Nessun preset salvato)"},
        {"preset_saved", "Preset salvato con successo:"},
        {"preset_deleted", "Preset eliminato con successo:"},
        {"preset_delete_fail", "Errore durante l'eliminazione del preset:"},
        {"preset_load_fail", "Errore durante il caricamento del preset:"},
        {"preset_valid", "Configurazione VALIDA!"},
        {"preset_invalid", "Configurazione NON VALIDA:"},

        // Portal
        {"portal_templates_title", "Template Captive Portal Disponibili"},
        {"portal_test_start", "Avvio del server Captive Portal di anteprima su:"},
        {"portal_test_stop_hint", "Premi INVIO o CTRL+C per arrestare il server di test."},

        // Wizard
        {"wizard_title", "=== PROCEDURA GUIDATA CONFIGURAZIONE ACCESS POINT ==="},
        {"wizard_welcome", "Questa procedura ti guiderà nella creazione e avvio rapido di un Access Point."},
        {"wizard_step_iface", "Passo 1/6: Seleziona l'interfaccia wireless da usare per l'AP"},
        {"wizard_step_ssid", "Passo 2/6: Inserisci il nome della rete Wi-Fi (SSID)"},
        {"wizard_step_sec", "Passo 3/6: Scegli la sicurezza [1] Open, [2] WPA2-Personal, [3] WPA3-Personal"},
        {"wizard_step_pass", "Inserisci la password Wi-Fi (minimo 8 caratteri):"},
        {"wizard_step_sharing", "Passo 4/6: Vuoi abilitare la condivisione Internet (NAT)? [s/n]:"},
        {"wizard_step_upstream", "Seleziona l'interfaccia con connessione Internet (upstream)"},
        {"wizard_step_portal", "Passo 5/6: Vuoi abilitare il Captive Portal? [s/n]:"},
        {"wizard_step_tpl", "Seleziona il template per il Captive Portal"},
        {"wizard_step_save", "Passo 6/6: Inserisci il nome per salvare questo Preset (lascia vuoto per non salvare):"},
        {"wizard_start_now", "Vuoi avviare l'Access Point adesso? [s/n]:"},
        {"wizard_done", "Procedura guidata completata con successo!"},
        {"wizard_cancel", "Procedura guidata annullata."}
    };

    // ==========================================
    // ENGLISH DICTIONARY
    // ==========================================
    dict_en_ = {
        // App General
        {"app_title", "AP-Generator - Advanced Wi-Fi Access Point Management Tool"},
        {"prompt_label", "ap-generator"},
        {"lang_switched", "Language set to: English"},
        {"lang_current", "Current language: English"},
        {"goodbye", "Goodbye! Exiting AP-Generator."},
        {"unknown_command", "Unknown command. Type 'help' or '?' to view available commands."},
        {"running_status_active", "ACTIVE (RUNNING)"},
        {"running_status_stopped", "STOPPED"},

        // Help Menu
        {"help_title", "AVAILABLE COMMANDS IN INTERACTIVE SHELL"},
        {"help_cat_network", "--- Network & Hardware ---"},
        {"help_interfaces", "interfaces              - List all detected Wi-Fi interfaces"},
        {"help_capabilities", "capabilities <iface>    - Display detailed card capabilities"},
        {"help_cat_presets", "--- Preset Management ---"},
        {"help_preset_list", "preset list             - List all saved presets"},
        {"help_preset_show", "preset show <name>      - Display configuration of a preset"},
        {"help_preset_save", "preset save <name> ...  - Create/save a configuration preset"},
        {"help_preset_delete", "preset delete <name>    - Delete an existing preset"},
        {"help_validate", "validate <name|file>    - Validate configuration parameters"},
        {"help_cat_ap", "--- Access Point Management ---"},
        {"help_start", "start <preset>          - Start the specified Access Point"},
        {"help_stop", "stop                    - Stop active Access Point"},
        {"help_status", "status                  - Show Access Point status & clients"},
        {"help_clients", "clients                 - Display connected clients & network data"},
        {"help_kick", "kick <mac>              - Force de-authenticate a connected client"},
        {"help_blacklist", "blacklist <add|del|list>- Manage MAC address blacklist"},
        {"help_wizard", "wizard                  - Interactive step-by-step setup wizard"},
        {"help_set", "set [key] [val]         - Modify or view active configuration variables"},
        {"help_show", "show                    - Show active configuration in memory"},
        {"help_cat_portal", "--- Captive Portal ---"},
        {"help_portal_list", "portal list             - List available HTML templates"},
        {"help_portal_test", "portal test <tpl>       - Start preview HTTP server for template"},
        {"help_cat_system", "--- System & Utilities ---"},
        {"help_language", "language <it|en>        - Switch application language"},
        {"help_clear", "clear                   - Clear terminal screen"},
        {"help_help", "help, ?                 - Display this command reference"},
        {"help_exit", "exit, quit, q           - Exit application"},

        // Interfaces & Capabilities
        {"iface_header", "Detected Wi-Fi Interfaces"},
        {"iface_none", "No wireless interfaces detected in the system."},
        {"iface_mac", "MAC Address:"},
        {"iface_driver", "Driver:"},
        {"iface_phy", "PHY Device:"},
        {"iface_state", "State:"},
        {"iface_connected", "Connected:"},
        {"iface_ip", "IP Address:"},
        {"iface_ap_mode", "AP Mode Support:"},
        {"iface_wpa3", "WPA3 (SAE) Support:"},
        {"iface_bands", "Supported Bands:"},
        {"iface_concurrency", "AP+STA Concurrency:"},
        {"cap_header", "Wireless Capabilities for:"},
        {"cap_not_found", "Error: Specified interface not found."},

        // AP Control
        {"ap_start_success", "Access Point successfully started on interface"},
        {"ap_start_fail", "Error starting Access Point:"},
        {"ap_stop_success", "Access Point stopped and network configuration restored."},
        {"ap_stop_fail", "Error stopping Access Point:"},
        {"ap_already_running", "An Access Point is already running. Stop it first."},
        {"ap_not_running", "No Access Point is currently running."},
        {"ap_specify_preset", "Specify the name of a preset to start. E.g.: start LabHotspot"},

        // Status & Clients
        {"status_header", "Access Point Status"},
        {"status_uptime", "Uptime:"},
        {"status_clients_count", "Connected Clients:"},
        {"clients_header", "Connected Clients & DHCP Leases"},
        {"clients_none", "No connected clients or active leases at the moment."},
        {"kick_success", "Client successfully kicked/deauthenticated:"},
        {"kick_fail", "Failed to kick client:"},
        {"blacklist_added", "MAC added to blacklist:"},
        {"blacklist_removed", "MAC removed from blacklist:"},
        {"blacklist_list", "MAC Address Blacklist:"},
        {"blacklist_empty", "Blacklist is currently empty."},

        // Presets
        {"preset_list_title", "Available Configuration Presets"},
        {"preset_none", "(No presets saved)"},
        {"preset_saved", "Preset saved successfully:"},
        {"preset_deleted", "Preset deleted successfully:"},
        {"preset_delete_fail", "Error deleting preset:"},
        {"preset_load_fail", "Error loading preset:"},
        {"preset_valid", "Configuration is VALID!"},
        {"preset_invalid", "INVALID configuration:"},

        // Portal
        {"portal_templates_title", "Available Captive Portal Templates"},
        {"portal_test_start", "Starting preview Captive Portal HTTP server at:"},
        {"portal_test_stop_hint", "Press ENTER or CTRL+C to stop the test server."},

        // Wizard
        {"wizard_title", "=== ACCESS POINT SETUP WIZARD ==="},
        {"wizard_welcome", "This wizard will guide you through creating and starting an Access Point."},
        {"wizard_step_iface", "Step 1/6: Select the wireless interface for the AP"},
        {"wizard_step_ssid", "Step 2/6: Enter the Wi-Fi Network Name (SSID)"},
        {"wizard_step_sec", "Step 3/6: Choose Security Mode [1] Open, [2] WPA2-Personal, [3] WPA3-Personal"},
        {"wizard_step_pass", "Enter Wi-Fi Password (min 8 characters):"},
        {"wizard_step_sharing", "Step 4/6: Enable Internet Sharing (NAT)? [y/n]:"},
        {"wizard_step_upstream", "Select the upstream Internet interface"},
        {"wizard_step_portal", "Step 5/6: Enable Captive Portal? [y/n]:"},
        {"wizard_step_tpl", "Select the Captive Portal template"},
        {"wizard_step_save", "Step 6/6: Enter a Preset name to save (leave empty to skip):"},
        {"wizard_start_now", "Do you want to start the Access Point now? [y/n]:"},
        {"wizard_done", "Setup wizard completed successfully!"},
        {"wizard_cancel", "Setup wizard cancelled."}
    };
}

} // namespace apm

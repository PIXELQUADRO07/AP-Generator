#include "apmanager/core/preset_manager.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace fs = std::filesystem;

namespace apm {

namespace {

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n\"");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n\"");
    return s.substr(start, end - start + 1);
}

std::string escape_json_string(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        if (c == '"') oss << "\\\"";
        else if (c == '\\') oss << "\\\\";
        else if (c == '\b') oss << "\\b";
        else if (c == '\f') oss << "\\f";
        else if (c == '\n') oss << "\\n";
        else if (c == '\r') oss << "\\r";
        else if (c == '\t') oss << "\\t";
        else oss << c;
    }
    return oss.str();
}

} // namespace

PresetManager::PresetManager(const std::string& custom_presets_dir) {
    if (!custom_presets_dir.empty()) {
        presets_dir_ = custom_presets_dir;
    } else {
        const char* env_dir = std::getenv("AP_CONFIG_DIR");
        if (env_dir) {
            presets_dir_ = env_dir;
        } else if (fs::exists("config/presets") || fs::exists("config")) {
            presets_dir_ = "config/presets";
        } else {
            const char* home = std::getenv("HOME");
            if (home) {
                presets_dir_ = std::string(home) + "/.config/ap-generator/presets";
            } else {
                presets_dir_ = "config/presets";
            }
        }
    }
    ensure_directory_exists();
}

void PresetManager::ensure_directory_exists() const {
    std::error_code ec;
    fs::create_directories(presets_dir_, ec);
}

std::string PresetManager::get_preset_filepath(const std::string& name) const {
    std::string clean_name = name;
    if (clean_name.size() > 5 && clean_name.substr(clean_name.size() - 5) == ".json") {
        return presets_dir_ + "/" + clean_name;
    }
    return presets_dir_ + "/" + clean_name + ".json";
}

bool PresetManager::preset_exists(const std::string& name) const {
    std::string path = get_preset_filepath(name);
    return fs::exists(path);
}

std::string PresetManager::serialize_config_json(const AccessPointConfig& config) {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"name\": \"" << escape_json_string(config.name) << "\",\n";
    oss << "  \"ssid\": \"" << escape_json_string(config.ssid) << "\",\n";
    oss << "  \"interface\": \"" << escape_json_string(config.interface) << "\",\n";
    oss << "  \"bssid\": \"" << escape_json_string(config.bssid) << "\",\n";
    oss << "  \"channel\": " << config.channel << ",\n";
    oss << "  \"security\": \"" << security_mode_to_string(config.security) << "\",\n";
    oss << "  \"password\": \"" << escape_json_string(config.password) << "\",\n";
    oss << "  \"internet_sharing\": " << (config.internet_sharing ? "true" : "false") << ",\n";
    oss << "  \"upstream_interface\": \"" << escape_json_string(config.upstream_interface) << "\",\n";
    oss << "  \"captive_portal\": " << (config.captive_portal ? "true" : "false") << ",\n";
    oss << "  \"portal_path\": \"" << escape_json_string(config.portal_path) << "\",\n";
    oss << "  \"gateway_ip\": \"" << escape_json_string(config.gateway_ip) << "\",\n";
    oss << "  \"netmask\": \"" << escape_json_string(config.netmask) << "\",\n";
    oss << "  \"dhcp_range_start\": \"" << escape_json_string(config.dhcp_range_start) << "\",\n";
    oss << "  \"dhcp_range_end\": \"" << escape_json_string(config.dhcp_range_end) << "\"\n";
    oss << "}\n";
    return oss.str();
}

bool PresetManager::deserialize_config_json(const std::string& json_str, AccessPointConfig& config, std::string* error_msg) {
    std::istringstream iss(json_str);
    std::string line;

    bool found_any = false;

    while (std::getline(iss, line)) {
        auto colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string key_part = line.substr(0, colon);
        std::string val_part = line.substr(colon + 1);

        // Remove trailing commas and whitespace
        auto comma = val_part.rfind(',');
        if (comma != std::string::npos) {
            val_part = val_part.substr(0, comma);
        }

        std::string key = trim(key_part);
        std::string val = trim(val_part);

        if (key.empty()) continue;
        found_any = true;

        if (key == "name") config.name = val;
        else if (key == "ssid") config.ssid = val;
        else if (key == "interface") config.interface = val;
        else if (key == "bssid") config.bssid = val;
        else if (key == "channel") {
            try { config.channel = std::stoi(val); } catch (...) {}
        }
        else if (key == "security") config.security = security_mode_from_string(val);
        else if (key == "password") config.password = val;
        else if (key == "internet_sharing") config.internet_sharing = (val == "true" || val == "1");
        else if (key == "upstream_interface") config.upstream_interface = val;
        else if (key == "captive_portal") config.captive_portal = (val == "true" || val == "1");
        else if (key == "portal_path") config.portal_path = val;
        else if (key == "gateway_ip") config.gateway_ip = val;
        else if (key == "netmask") config.netmask = val;
        else if (key == "dhcp_range_start") config.dhcp_range_start = val;
        else if (key == "dhcp_range_end") config.dhcp_range_end = val;
    }

    if (!found_any) {
        if (error_msg) *error_msg = "Invalid or empty JSON config content.";
        return false;
    }

    return true;
}

bool PresetManager::save_preset(const std::string& name, const AccessPointConfig& config, std::string* error_msg) {
    if (name.empty()) {
        if (error_msg) *error_msg = "Preset name cannot be empty.";
        return false;
    }

    ensure_directory_exists();
    std::string filepath = get_preset_filepath(name);

    AccessPointConfig save_cfg = config;
    if (save_cfg.name.empty()) {
        save_cfg.name = name;
    }

    std::ofstream out(filepath);
    if (!out.is_open()) {
        if (error_msg) *error_msg = "Failed to open " + filepath + " for writing.";
        return false;
    }

    out << serialize_config_json(save_cfg);
    out.close();
    return true;
}

bool PresetManager::load_preset(const std::string& name, AccessPointConfig& config, std::string* error_msg) {
    std::string filepath = get_preset_filepath(name);
    if (!fs::exists(filepath)) {
        // Also check if user passed a direct path to a file
        if (fs::exists(name)) {
            filepath = name;
        } else {
            if (error_msg) *error_msg = "Preset '" + name + "' not found in " + presets_dir_ + ".";
            return false;
        }
    }

    std::ifstream in(filepath);
    if (!in.is_open()) {
        if (error_msg) *error_msg = "Failed to open preset file " + filepath + ".";
        return false;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    return deserialize_config_json(buffer.str(), config, error_msg);
}

bool PresetManager::delete_preset(const std::string& name, std::string* error_msg) {
    std::string filepath = get_preset_filepath(name);
    std::error_code ec;
    if (!fs::exists(filepath, ec)) {
        if (error_msg) *error_msg = "Preset '" + name + "' does not exist.";
        return false;
    }

    if (!fs::remove(filepath, ec)) {
        if (error_msg) *error_msg = "Failed to remove preset file: " + ec.message();
        return false;
    }

    return true;
}

std::vector<std::string> PresetManager::list_presets() {
    ensure_directory_exists();
    std::vector<std::string> presets;

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(presets_dir_, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            presets.push_back(entry.path().stem().string());
        }
    }

    std::sort(presets.begin(), presets.end());
    return presets;
}

bool PresetManager::export_preset(const std::string& name, const std::string& target_path, std::string* error_msg) {
    AccessPointConfig cfg;
    if (!load_preset(name, cfg, error_msg)) {
        return false;
    }

    std::ofstream out(target_path);
    if (!out.is_open()) {
        if (error_msg) *error_msg = "Failed to open export destination: " + target_path;
        return false;
    }

    out << serialize_config_json(cfg);
    return true;
}

bool PresetManager::import_preset(const std::string& source_path, const std::string& name, std::string* error_msg) {
    std::ifstream in(source_path);
    if (!in.is_open()) {
        if (error_msg) *error_msg = "Failed to open import file: " + source_path;
        return false;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();

    AccessPointConfig cfg;
    if (!deserialize_config_json(buffer.str(), cfg, error_msg)) {
        return false;
    }

    std::string target_name = name;
    if (target_name.empty()) {
        target_name = fs::path(source_path).stem().string();
    }

    return save_preset(target_name, cfg, error_msg);
}

} // namespace apm

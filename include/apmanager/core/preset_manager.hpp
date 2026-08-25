#pragma once

#include <string>
#include <vector>
#include "apmanager/core/types.hpp"

namespace apm {

class PresetManager {
public:
    explicit PresetManager(const std::string& custom_presets_dir = "");
    ~PresetManager() = default;

    bool save_preset(const std::string& name, const AccessPointConfig& config, std::string* error_msg = nullptr);
    bool load_preset(const std::string& name, AccessPointConfig& config, std::string* error_msg = nullptr);
    bool delete_preset(const std::string& name, std::string* error_msg = nullptr);
    std::vector<std::string> list_presets();
    bool preset_exists(const std::string& name) const;

    bool export_preset(const std::string& name, const std::string& target_path, std::string* error_msg = nullptr);
    bool import_preset(const std::string& source_path, const std::string& name, std::string* error_msg = nullptr);

    std::string get_presets_directory() const { return presets_dir_; }

    static std::string serialize_config_json(const AccessPointConfig& config);
    static bool deserialize_config_json(const std::string& json_str, AccessPointConfig& config, std::string* error_msg = nullptr);

private:
    std::string presets_dir_;
    void ensure_directory_exists() const;
    std::string get_preset_filepath(const std::string& name) const;
};

} // namespace apm

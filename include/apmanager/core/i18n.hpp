#pragma once

#include <string>
#include <unordered_map>
#include <memory>

namespace apm {

enum class Language {
    IT, // Italiano
    EN  // English
};

class I18n {
public:
    static I18n& instance() {
        static I18n inst;
        return inst;
    }

    void set_language(Language lang);
    void set_language_by_code(const std::string& code);
    Language get_language() const { return current_lang_; }
    std::string get_language_code() const;
    std::string get_language_name() const;

    // Get localized text by string key
    std::string get(const std::string& key) const;

    // Helper shorthand
    static std::string tr(const std::string& key) {
        return instance().get(key);
    }

private:
    I18n();
    void detect_system_language();
    void load_translations();
    void save_language_preference() const;
    void load_language_preference();

    Language current_lang_ = Language::IT;
    std::unordered_map<std::string, std::string> dict_it_;
    std::unordered_map<std::string, std::string> dict_en_;
};

} // namespace apm

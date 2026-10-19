#pragma once

#include <string>
#include <unordered_map>

namespace OpenCraft {

class LocalizationManager {
public:
    static LocalizationManager& global();

    LocalizationManager();

    void setLanguage(const std::string& langCode);
    const std::string& getLanguage() const { return m_currentLanguage; }

    std::string get(const std::string& key) const;
    bool has(const std::string& key) const;

    // Métodos de cálculo de accesibilidad visual
    static int calculateAutoUIScale(int screenWidth, int screenHeight);
    static float getUIScaleFactor(int uiScaleSetting, int screenWidth, int screenHeight);

private:
    std::string m_currentLanguage = "es_ES";
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_tables;

    void initDictionary();
};

} // namespace OpenCraft

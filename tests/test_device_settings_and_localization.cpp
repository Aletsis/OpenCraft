#include <cassert>
#include <iostream>
#include <filesystem>
#include "platform/DeviceSettings.hpp"
#include "platform/PlatformPaths.hpp"
#include "ui/LocalizationManager.hpp"

using namespace OpenCraft;

void testDefaultSettingsAndPresets() {
    DeviceSettings s;
    assert(s.renderDistance == 8);
    assert(s.simulationDistance == 8);
    assert(s.masterVolume == 100);
    assert(s.subtitlesEnabled == true);
    assert(s.language == "es_ES");

    // Probar Preset Bajo (Móvil / Ahorro)
    s.applyQualityPreset(QualityPresetLevel::Low);
    assert(s.renderDistance == 4);
    assert(s.simulationDistance == 4);
    assert(s.smoothLighting == false);
    assert(s.maxParticles == 256);

    // Probar Preset Ultra
    s.applyQualityPreset(QualityPresetLevel::Ultra);
    assert(s.renderDistance == 16);
    assert(s.simulationDistance == 12);
    assert(s.maxParticles == 4096);

    // Resetear a valores por defecto
    s.resetToDefaults();
    assert(s.renderDistance == 8);
    assert(s.maxParticles == 1024);
    std::cout << "[PASS] Configuración por defecto y perfiles de calidad verificados." << std::endl;
}

void testSettingsSerializationAndFileIO() {
    std::filesystem::path testDir = std::filesystem::current_path() / "test_scratch_settings";
    std::filesystem::create_directories(testDir);
    std::filesystem::path cfgFile = testDir / "test_settings.cfg";

    DeviceSettings original;
    original.renderDistance = 14;
    original.masterVolume = 85;
    original.musicVolume = 50;
    original.subtitlesEnabled = true;
    original.highContrast = true;
    original.language = "en_US";
    original.fov = 85;
    original.mouseSensitivity = 0.25f;

    bool saved = original.saveToFile(cfgFile);
    assert(saved);
    assert(std::filesystem::exists(cfgFile));

    DeviceSettings loaded;
    bool loadedOk = loaded.loadFromFile(cfgFile);
    assert(loadedOk);

    assert(loaded.renderDistance == 14);
    assert(loaded.masterVolume == 85);
    assert(loaded.musicVolume == 50);
    assert(loaded.subtitlesEnabled == true);
    assert(loaded.highContrast == true);
    assert(loaded.language == "en_US");
    assert(loaded.fov == 85);
    assert(std::abs(loaded.mouseSensitivity - 0.25f) < 0.001f);

    // Cálculo de volumen efectivo
    float effVol = loaded.getEffectiveVolume(loaded.musicVolume);
    // 0.85 * 0.50 = 0.425
    assert(std::abs(effVol - 0.425f) < 0.001f);

    std::filesystem::remove_all(testDir);
    std::cout << "[PASS] Serialización, idempotencia y cálculo de volumen efectivo verificados." << std::endl;
}

void testLocalizationDictionaryAndScaling() {
    auto& loc = LocalizationManager::global();

    // Idioma Español
    loc.setLanguage("es_ES");
    assert(loc.get("menu.play") == "Jugar");
    assert(loc.get("options.title") == "Configuración");
    assert(loc.get("subtitles.break") == "Bloque roto");
    assert(loc.get("subtitles.step") == "Pisadas");

    // Idioma Inglés
    loc.setLanguage("en_US");
    assert(loc.get("menu.play") == "Play Game");
    assert(loc.get("options.title") == "Settings");
    assert(loc.get("subtitles.break") == "Block broken");
    assert(loc.get("subtitles.step") == "Footsteps");

    // Clave inexistente retorna la clave misma
    assert(loc.get("unknown.dummy.key") == "unknown.dummy.key");

    // Cálculo automático de escala de UI según resolución
    assert(LocalizationManager::calculateAutoUIScale(640, 480) == 1);
    assert(LocalizationManager::calculateAutoUIScale(1280, 720) == 2);
    assert(LocalizationManager::calculateAutoUIScale(1920, 1080) == 3);
    assert(LocalizationManager::calculateAutoUIScale(3840, 2160) == 4);

    // Factor de escala
    assert(LocalizationManager::getUIScaleFactor(2, 1920, 1080) == 2.0f);
    assert(LocalizationManager::getUIScaleFactor(0, 1920, 1080) == 3.0f); // Auto

    std::cout << "[PASS] Diccionario bilingüe (ES/EN) y escalado adaptativo de UI verificados." << std::endl;
}

int main() {
    std::cout << "=== Ejecutando Suite de Pruebas: DeviceSettingsAndLocalizationTest ===" << std::endl;
    testDefaultSettingsAndPresets();
    testSettingsSerializationAndFileIO();
    testLocalizationDictionaryAndScaling();
    std::cout << "=== Todas las pruebas de Configuración y Localización SUPERADAS ===" << std::endl;
    return 0;
}

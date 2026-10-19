#pragma once

#include <string>
#include <filesystem>
#include <cstdint>

namespace OpenCraft {

enum class QualityPresetLevel {
    Custom = 0,
    Low = 1,    // Móvil / Ahorro de batería
    Medium = 2, // Estándar equilibrado
    High = 3,   // Escritorio alto
    Ultra = 4   // Gráficos máximos
};

struct DeviceSettings {
    // Versión de esquema del archivo de configuración
    uint32_t schemaVersion = 1;

    // --- Vídeo y Gráficos ---
    int renderDistance = 8;
    int simulationDistance = 8;
    bool vsync = true;
    int fov = 70;
    int brightness = 50; // 0 = sombrío, 100 = brillante
    bool smoothLighting = true;
    int maxParticles = 1024;
    QualityPresetLevel qualityPreset = QualityPresetLevel::Medium;

    // --- Audio y Volúmenes (0 a 100) ---
    int masterVolume = 100;
    int musicVolume = 80;
    int weatherVolume = 100;
    int blocksVolume = 100;
    int hostileVolume = 100;
    int friendlyVolume = 100;
    int playersVolume = 100;
    int ambientVolume = 80;
    bool subtitlesEnabled = true;

    // --- Controles y Cámara ---
    float mouseSensitivity = 0.15f;
    float touchSensitivity = 150.0f;
    bool invertY = false;
    bool autoJump = false;
    bool viewBobbing = true;
    bool touchControlsEnabled = true;
    float touchButtonScale = 1.0f;

    // --- Accesibilidad e Idioma ---
    int uiScale = 0; // 0 = Auto, 1 = 1x, 2 = 2x, 3 = 3x, 4 = 4x
    bool highContrast = false;
    std::string language = "es_ES"; // "es_ES" o "en_US"

    // Métodos de gestión
    void resetToDefaults();
    void applyQualityPreset(QualityPresetLevel preset);

    bool loadFromFile(const std::filesystem::path& filePath);
    bool saveToFile(const std::filesystem::path& filePath) const;

    // Cargar desde ruta por defecto de plataforma
    bool loadDefault();
    bool saveDefault() const;

    // Métodos de utilidad de audio: normalizar a rango [0.0f, 1.0f]
    float getEffectiveVolume(int categoryVolume) const {
        return (static_cast<float>(masterVolume) / 100.0f) * (static_cast<float>(categoryVolume) / 100.0f);
    }
};

} // namespace OpenCraft

#pragma once

#include <string>
#include <cstdint>
#include "platform/DeviceSettings.hpp"
#include "platform/PlatformPaths.hpp"

namespace OpenCraft {

struct QualityProfile {
    std::string name;
    QualityPresetLevel level = QualityPresetLevel::Medium;
    int renderDistance = 8;
    int simulationDistance = 8;
    bool smoothLighting = true;
    int maxParticles = 1024;
    int targetFps = 60;
    bool vsync = true;
    size_t estimatedMemoryCapMB = 120;
};

class QualityProfiles {
public:
    static QualityProfile get(QualityPresetLevel level) {
        switch (level) {
            case QualityPresetLevel::Low:
                return QualityProfile{.name = "Bajo (Ahorro de Batería / Móvil ARM)",
                                      .level = QualityPresetLevel::Low,
                                      .renderDistance = 4,
                                      .simulationDistance = 4,
                                      .smoothLighting = false,
                                      .maxParticles = 256,
                                      .targetFps = 60,
                                      .vsync = true,
                                      .estimatedMemoryCapMB = 80};
            case QualityPresetLevel::Medium:
                return QualityProfile{.name = "Medio (Estándar Equilibrado)",
                                      .level = QualityPresetLevel::Medium,
                                      .renderDistance = 8,
                                      .simulationDistance = 8,
                                      .smoothLighting = true,
                                      .maxParticles = 1024,
                                      .targetFps = 60,
                                      .vsync = true,
                                      .estimatedMemoryCapMB = 150};
            case QualityPresetLevel::High:
                return QualityProfile{.name = "Alto (Escritorio / GPU Dedicada)",
                                      .level = QualityPresetLevel::High,
                                      .renderDistance = 12,
                                      .simulationDistance = 10,
                                      .smoothLighting = true,
                                      .maxParticles = 2048,
                                      .targetFps = 60,
                                      .vsync = true,
                                      .estimatedMemoryCapMB = 250};
            case QualityPresetLevel::Ultra:
                return QualityProfile{.name = "Ultra (Máxima Distancia y Partículas)",
                                      .level = QualityPresetLevel::Ultra,
                                      .renderDistance = 16,
                                      .simulationDistance = 12,
                                      .smoothLighting = true,
                                      .maxParticles = 4096,
                                      .targetFps = 60,
                                      .vsync = false,
                                      .estimatedMemoryCapMB = 450};
            default: return get(QualityPresetLevel::Medium);
        }
    }

    static QualityPresetLevel detectRecommended() {
        PlatformType p = PlatformPaths::getPlatformType();
        if (p == PlatformType::LinuxMobile || p == PlatformType::Android || p == PlatformType::iOS) {
            return QualityPresetLevel::Low;
        }
        return QualityPresetLevel::Medium;
    }
};

} // namespace OpenCraft

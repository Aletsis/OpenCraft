#include "platform/DeviceSettings.hpp"
#include "platform/PlatformPaths.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace OpenCraft {

void DeviceSettings::resetToDefaults() { *this = DeviceSettings{}; }

void DeviceSettings::applyQualityPreset(QualityPresetLevel preset) {
    qualityPreset = preset;
    switch (preset) {
        case QualityPresetLevel::Low:
            renderDistance = 4;
            simulationDistance = 4;
            vsync = true;
            smoothLighting = false;
            maxParticles = 256;
            break;
        case QualityPresetLevel::Medium:
            renderDistance = 8;
            simulationDistance = 8;
            vsync = true;
            smoothLighting = true;
            maxParticles = 1024;
            break;
        case QualityPresetLevel::High:
            renderDistance = 12;
            simulationDistance = 10;
            vsync = true;
            smoothLighting = true;
            maxParticles = 2048;
            break;
        case QualityPresetLevel::Ultra:
            renderDistance = 16;
            simulationDistance = 12;
            vsync = false;
            smoothLighting = true;
            maxParticles = 4096;
            break;
        case QualityPresetLevel::Custom: break;
    }
}

static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool DeviceSettings::loadFromFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed.starts_with("#") || trimmed.starts_with("//")) {
            continue;
        }

        size_t eqPos = trimmed.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = trim(trimmed.substr(0, eqPos));
        std::string val = trim(trimmed.substr(eqPos + 1));

        // Un valor corrupto no debe impedir arrancar: se conserva el valor por defecto de esa clave
        try {
            if (key == "schemaVersion")
                schemaVersion = static_cast<uint32_t>(std::stoul(val));
            else if (key == "renderDistance")
                renderDistance = std::clamp(std::stoi(val), 2, 32);
            else if (key == "simulationDistance")
                simulationDistance = std::clamp(std::stoi(val), 2, 16);
            else if (key == "vsync")
                vsync = (val == "true" || val == "1");
            else if (key == "fov")
                fov = std::clamp(std::stoi(val), 30, 110);
            else if (key == "smoothLighting")
                smoothLighting = (val == "true" || val == "1");
            else if (key == "maxParticles")
                maxParticles = std::clamp(std::stoi(val), 64, 8192);
            else if (key == "qualityPreset")
                qualityPreset = static_cast<QualityPresetLevel>(std::stoi(val));

            else if (key == "masterVolume")
                masterVolume = std::clamp(std::stoi(val), 0, 100);
            else if (key == "musicVolume")
                musicVolume = std::clamp(std::stoi(val), 0, 100);
            else if (key == "weatherVolume")
                weatherVolume = std::clamp(std::stoi(val), 0, 100);
            else if (key == "blocksVolume")
                blocksVolume = std::clamp(std::stoi(val), 0, 100);
            else if (key == "hostileVolume")
                hostileVolume = std::clamp(std::stoi(val), 0, 100);
            else if (key == "friendlyVolume")
                friendlyVolume = std::clamp(std::stoi(val), 0, 100);
            else if (key == "playersVolume")
                playersVolume = std::clamp(std::stoi(val), 0, 100);
            else if (key == "ambientVolume")
                ambientVolume = std::clamp(std::stoi(val), 0, 100);
            else if (key == "subtitlesEnabled")
                subtitlesEnabled = (val == "true" || val == "1");

            else if (key == "mouseSensitivity")
                mouseSensitivity = std::stof(val);
            else if (key == "touchSensitivity")
                touchSensitivity = std::stof(val);
            else if (key == "invertY")
                invertY = (val == "true" || val == "1");
            else if (key == "viewBobbing")
                viewBobbing = (val == "true" || val == "1");
            else if (key == "touchControlsEnabled")
                touchControlsEnabled = (val == "true" || val == "1");
            else if (key == "touchButtonScale")
                touchButtonScale = std::clamp(std::stof(val), 0.5f, 2.5f);

            else if (key == "uiScale")
                uiScale = std::clamp(std::stoi(val), 0, 4);
            else if (key == "highContrast")
                highContrast = (val == "true" || val == "1");
            else if (key == "language")
                language = val;
            else if (key == "brightness")
                brightness = std::clamp(std::stoi(val), 0, 100);
            else if (key == "autoJump")
                autoJump = (val == "true" || val == "1");
            else if (key == "guiScale")
                uiScale = std::clamp(std::stoi(val), 0, 4);
        } catch (const std::exception&) {
            continue;
        }
    }

    return true;
}

bool DeviceSettings::saveToFile(const std::filesystem::path& filePath) const {
    std::filesystem::create_directories(filePath.parent_path());
    std::ofstream file(filePath, std::ios::trunc);
    if (!file.is_open()) return false;

    file << "# OpenCraft Device Configuration (Settings)\n";
    file << "schemaVersion=" << schemaVersion << "\n\n";

    file << "# Graphics & Video\n";
    file << "renderDistance=" << renderDistance << "\n";
    file << "simulationDistance=" << simulationDistance << "\n";
    file << "vsync=" << (vsync ? "true" : "false") << "\n";
    file << "fov=" << fov << "\n";
    file << "brightness=" << brightness << "\n";
    file << "smoothLighting=" << (smoothLighting ? "true" : "false") << "\n";
    file << "maxParticles=" << maxParticles << "\n";
    file << "qualityPreset=" << static_cast<int>(qualityPreset) << "\n\n";

    file << "# Audio\n";
    file << "masterVolume=" << masterVolume << "\n";
    file << "musicVolume=" << musicVolume << "\n";
    file << "weatherVolume=" << weatherVolume << "\n";
    file << "blocksVolume=" << blocksVolume << "\n";
    file << "hostileVolume=" << hostileVolume << "\n";
    file << "friendlyVolume=" << friendlyVolume << "\n";
    file << "playersVolume=" << playersVolume << "\n";
    file << "ambientVolume=" << ambientVolume << "\n";
    file << "subtitlesEnabled=" << (subtitlesEnabled ? "true" : "false") << "\n\n";

    file << "# Controls & Input\n";
    file << "mouseSensitivity=" << mouseSensitivity << "\n";
    file << "touchSensitivity=" << touchSensitivity << "\n";
    file << "invertY=" << (invertY ? "true" : "false") << "\n";
    file << "autoJump=" << (autoJump ? "true" : "false") << "\n";
    file << "viewBobbing=" << (viewBobbing ? "true" : "false") << "\n";
    file << "touchControlsEnabled=" << (touchControlsEnabled ? "true" : "false") << "\n";
    file << "touchButtonScale=" << touchButtonScale << "\n\n";

    file << "# Accessibility & Localization\n";
    file << "uiScale=" << uiScale << "\n";
    file << "highContrast=" << (highContrast ? "true" : "false") << "\n";
    file << "language=" << language << "\n";

    return true;
}

bool DeviceSettings::loadDefault() {
    std::filesystem::path configPath = PlatformPaths::getUserConfigDir() / "settings.cfg";
    if (std::filesystem::exists(configPath)) {
        return loadFromFile(configPath);
    }
    return false;
}

bool DeviceSettings::saveDefault() const {
    std::filesystem::path configPath = PlatformPaths::getUserConfigDir() / "settings.cfg";
    return saveToFile(configPath);
}

} // namespace OpenCraft

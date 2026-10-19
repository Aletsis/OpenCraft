#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace OpenCraft {

enum class PlatformType {
    LinuxDesktop,
    LinuxMobile,
    Android,
    iOS,
    Generic
};

class PlatformPaths {
public:
    static PlatformType getPlatformType();
    static std::string getPlatformName();

    // Rutas para datos de usuario y partidas
    static std::filesystem::path getUserDataDir();
    static std::filesystem::path getUserConfigDir();
    static std::filesystem::path getSavesDir();
    static std::filesystem::path getBackupsDir();
    static std::filesystem::path getExecutableDir();

    // Resolución de activos (shaders, texturas, sonidos, packs)
    static std::filesystem::path resolveAsset(const std::string& relativePath);
    static std::vector<std::filesystem::path> getAssetSearchPaths();

    // Configuración para pruebas y entornos aislados
    static void setOverrideBaseDir(const std::filesystem::path& dir);
    static void clearOverrideBaseDir();

private:
    static std::filesystem::path s_overrideBaseDir;
};

} // namespace OpenCraft

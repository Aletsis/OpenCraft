#include "platform/PlatformPaths.hpp"
#include <cstdlib>
#include <iostream>
#include <iterator>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#include <mach-o/dyld.h>
#elif defined(__linux__) && !defined(__ANDROID__)
#include <unistd.h>
#endif

namespace {

// Carpeta base de datos por usuario en Windows (%APPDATA%) y macOS (Application Support)
std::filesystem::path platformDataBase() {
#if defined(_WIN32)
    if (const wchar_t* appData = _wgetenv(L"APPDATA"); appData && *appData) {
        return std::filesystem::path(appData) / "OpenCraft";
    }
#elif defined(__APPLE__) && !TARGET_OS_IPHONE
    if (const char* home = std::getenv("HOME"); home && *home) {
        return std::filesystem::path(home) / "Library" / "Application Support" / "OpenCraft";
    }
#endif
    return {};
}

} // namespace

namespace OpenCraft {

std::filesystem::path PlatformPaths::s_overrideBaseDir = "";

PlatformType PlatformPaths::getPlatformType() {
#if defined(__ANDROID__)
    return PlatformType::Android;
#elif defined(_WIN32)
    return PlatformType::Windows;
#elif defined(__APPLE__)
#if TARGET_OS_IPHONE
    return PlatformType::iOS;
#else
    return PlatformType::MacOS;
#endif
#elif defined(__linux__)
    // Detectar entorno móvil (Phosh, Plasma Mobile, pmOS) mediante variables de entorno
    const char* phosh = std::getenv("PHOSH_VERSION");
    const char* pmos = std::getenv("PMOS_VERSION");
    if (phosh || pmos) {
        return PlatformType::LinuxMobile;
    }
    return PlatformType::LinuxDesktop;
#else
    return PlatformType::Generic;
#endif
}

std::string PlatformPaths::getPlatformName() {
    switch (getPlatformType()) {
        case PlatformType::LinuxDesktop: return "Linux Desktop (X11/Wayland)";
        case PlatformType::LinuxMobile: return "Linux Mobile (Phosh/pmOS/Wayland)";
        case PlatformType::Android: return "Android (NDK/GLES3)";
        case PlatformType::iOS: return "iOS (Metal/GLES)";
        case PlatformType::Windows: return "Windows";
        case PlatformType::MacOS: return "macOS";
        default: return "Generic Platform";
    }
}

std::filesystem::path PlatformPaths::getUserDataDir() {
    if (!s_overrideBaseDir.empty()) {
        return s_overrideBaseDir / "data";
    }

#if defined(__linux__) && !defined(__ANDROID__)
    const char* xdgData = std::getenv("XDG_DATA_HOME");
    if (xdgData && *xdgData) {
        return std::filesystem::path(xdgData) / "opencraft";
    }
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return std::filesystem::path(home) / ".local" / "share" / "opencraft";
    }
#endif
    if (auto base = platformDataBase(); !base.empty()) return base;

    return std::filesystem::path("opencraft_data");
}

std::filesystem::path PlatformPaths::getUserConfigDir() {
    if (!s_overrideBaseDir.empty()) {
        return s_overrideBaseDir / "config";
    }

#if defined(__linux__) && !defined(__ANDROID__)
    const char* xdgConfig = std::getenv("XDG_CONFIG_HOME");
    if (xdgConfig && *xdgConfig) {
        return std::filesystem::path(xdgConfig) / "opencraft";
    }
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return std::filesystem::path(home) / ".config" / "opencraft";
    }
#endif
    if (auto base = platformDataBase(); !base.empty()) return base / "config";

    return std::filesystem::path(".");
}

std::filesystem::path PlatformPaths::getSavesDir() { return getUserDataDir() / "worlds"; }

std::vector<std::filesystem::path> PlatformPaths::getAssetSearchPaths() {
    std::vector<std::filesystem::path> paths;

    if (!s_overrideBaseDir.empty()) {
        paths.push_back(s_overrideBaseDir / "assets");
        paths.push_back(s_overrideBaseDir);
    }

    // 1. Variable de entorno explícita (instalaciones personalizadas y empaquetado)
    if (const char* envDir = std::getenv("OPENCRAFT_ASSETS_DIR"); envDir && *envDir) {
        paths.push_back(std::filesystem::path(envDir));
    }

    // 2. Relativo al ejecutable: build/ (assets/ junto al binario), paquete RC (bin/../assets)
    //    e instalación FHS (bin/../share/opencraft/assets)
    std::filesystem::path exeDir = getExecutableDir();
    if (!exeDir.empty()) {
        paths.push_back(exeDir / "assets");
        paths.push_back(exeDir.parent_path() / "assets");
        paths.push_back(exeDir.parent_path() / "share" / "opencraft" / "assets");
    }

    // 3. Directorio de trabajo local
    paths.push_back(std::filesystem::current_path() / "assets");
    paths.push_back(std::filesystem::current_path());

#if defined(__linux__) && !defined(__ANDROID__)
    // 4. Rutas XDG y sistema estándar
    const char* home = std::getenv("HOME");
    if (home && *home) {
        paths.push_back(std::filesystem::path(home) / ".local" / "share" / "opencraft" / "assets");
    }
    paths.push_back(std::filesystem::path("/usr/local/share/opencraft/assets"));
    paths.push_back(std::filesystem::path("/usr/share/opencraft/assets"));
#endif

    return paths;
}

std::filesystem::path PlatformPaths::getExecutableDir() {
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH * 4];
    DWORD len = GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(std::size(buffer)));
    if (len > 0 && len < std::size(buffer)) {
        return std::filesystem::path(std::wstring(buffer, len)).parent_path();
    }
#elif defined(__APPLE__)
    char buffer[4096];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
        std::error_code ec;
        auto exe = std::filesystem::weakly_canonical(std::filesystem::path(buffer), ec);
        return (ec ? std::filesystem::path(buffer) : exe).parent_path();
    }
#elif defined(__linux__) && !defined(__ANDROID__)
    std::error_code ec;
    std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec && !exe.empty()) {
        return exe.parent_path();
    }
#endif
    return {};
}

std::filesystem::path PlatformPaths::getBackupsDir() { return getUserDataDir() / "backups"; }

std::filesystem::path PlatformPaths::resolveAsset(const std::string& relativePath) {
    if (relativePath.empty()) return {};

    std::filesystem::path direct(relativePath);
    if (direct.is_absolute()) {
        return direct;
    }

    // Buscar en todas las ubicaciones candidatas
    for (const auto& basePath : getAssetSearchPaths()) {
        std::filesystem::path candidate = basePath / relativePath;
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
        // Probar quitando prefijo "assets/" si la ruta base ya lo incluye
        if (relativePath.starts_with("assets/")) {
            std::filesystem::path stripped = basePath / relativePath.substr(7);
            if (std::filesystem::exists(stripped)) {
                return stripped;
            }
        }
    }

    // Fallback: retornar ruta original
    return direct;
}

void PlatformPaths::setOverrideBaseDir(const std::filesystem::path& dir) { s_overrideBaseDir = dir; }

void PlatformPaths::clearOverrideBaseDir() { s_overrideBaseDir.clear(); }

} // namespace OpenCraft

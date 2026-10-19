#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace OpenCraft {

class WorldSession;

struct ContentPackManifest {
    std::string id;
    std::string name;
    std::string version = "1.0.0";
    std::string description;
    uint32_t packFormat = 1;
    std::vector<std::string> dependencies;
};

struct ContentPackRecipe {
    std::string resultItem;
    uint16_t resultCount = 1;
    std::vector<std::string> ingredients;
    bool isShaped = false;
    uint8_t width = 0;
    uint8_t height = 0;
};

struct ContentPack {
    ContentPackManifest manifest;
    bool isValid = false;
    std::string validationError;
    std::vector<ContentPackRecipe> recipes;
    std::vector<std::string> advancements;
    std::unordered_map<std::string, std::vector<std::string>> functions;
};

class ContentPackManager {
public:
    ContentPackManager() = default;
    ~ContentPackManager() = default;

    // Carga directa en memoria (para tests, configuración interna y fixtures)
    bool registerPack(ContentPack pack);

    // Valida dependencias e identificadores contra los registros del juego
    bool validatePack(ContentPack& pack) const;

    // Activación y desactivación
    bool enablePack(const std::string& id);
    bool disablePack(const std::string& id);
    bool isPackEnabled(const std::string& id) const;

    // Hash de sincronización determinista para red
    std::string computeNetworkHash() const;
    bool verifyNetworkHash(const std::string& remoteHash) const;

    // Aplicación de contenido activo a la sesión
    void applyToSession(WorldSession& session);

    const std::unordered_map<std::string, ContentPack>& getAllPacks() const noexcept { return m_packs; }

    const std::vector<std::string>& getEnabledPackIds() const noexcept { return m_enabledOrder; }

    void clear() {
        m_packs.clear();
        m_enabledOrder.clear();
    }

private:
    std::unordered_map<std::string, ContentPack> m_packs;
    std::vector<std::string> m_enabledOrder;
};

} // namespace OpenCraft

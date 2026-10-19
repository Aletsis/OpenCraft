#pragma once

#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include "storage/WorldStorage.hpp"

namespace OpenCraft {

struct WorldSummary {
    std::string id;
    std::string name;
    uint64_t seed = 0;
    uint32_t schemaVersion = 1;
    uint32_t generatorVersion = 1;
    int64_t lastPlayed = 0;
    uint64_t sizeOnDisk = 0;
    uint32_t gameMode = 0;
    bool hardcore = false;
    bool locked = false;
};

class WorldManager {
public:
    explicit WorldManager(std::filesystem::path savesDir = "saves", std::filesystem::path backupsDir = "backups");
    ~WorldManager();

    const std::filesystem::path& getSavesDir() const noexcept { return m_savesDir; }
    const std::filesystem::path& getBackupsDir() const noexcept { return m_backupsDir; }

    // Listar mundos existentes en el directorio saves/
    std::vector<WorldSummary> listWorlds() const;

    // Crear un nuevo mundo
    std::string createWorld(const std::string& name, uint64_t seed, uint32_t gameMode = 0,
                            uint32_t schemaVersion = CURRENT_SCHEMA_VERSION, bool hardcore = false);

    // Abrir un mundo existente (con verificación y adquisición de session.lock)
    std::unique_ptr<WorldStorage> openWorld(const std::string& worldId, StorageError& outError);

    // Cerrar sesión y liberar session.lock
    void closeWorld(const std::string& worldId);

    // Copia de seguridad completa en backups/
    std::filesystem::path backupWorld(const std::string& worldId);
    // Copia recursiva portable (crea el destino si no existe y sobrescribe archivos)
    static bool copyDirectoryTree(const std::filesystem::path& src, const std::filesystem::path& dst,
                                  std::error_code& ec);

    // Migración de esquema (conservando respaldo previo e idempotente)
    StorageError migrateWorld(const std::string& worldId, uint32_t targetSchema = CURRENT_SCHEMA_VERSION);

    // Eliminar mundo
    bool deleteWorld(const std::string& worldId);

    // Comprobar si un mundo está bloqueado por otro proceso activo
    bool isWorldLocked(const std::string& worldId) const;

private:
    std::filesystem::path m_savesDir;
    std::filesystem::path m_backupsDir;
    std::vector<std::string> m_openWorldLocks;

    bool acquireLock(const std::filesystem::path& worldDir);
    void releaseLock(const std::filesystem::path& worldDir);
    static std::string sanitizeWorldId(const std::string& name);
};

} // namespace OpenCraft

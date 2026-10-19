#include <iostream>
#include <cassert>
#include <filesystem>
#include "storage/WorldManager.hpp"
#include "storage/WorldFormat.hpp"
#include "sim/WorldSession.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F02 Test 4] Migración de esquemas, copias de seguridad e idempotencia ---" << std::endl;

    std::filesystem::path testSaves = "build/test_saves_migration";
    std::filesystem::path testBackups = "build/test_backups_migration";
    std::error_code ec;
    std::filesystem::remove_all(testSaves, ec);
    std::filesystem::remove_all(testBackups, ec);

    WorldManager manager(testSaves, testBackups);

    // 1. Crear un mundo con Esquema v1
    std::string worldId = manager.createWorld("Mundo Esquema 1", 7777, 0, SCHEMA_VERSION_1);
    assert(!worldId.empty());

    // Añadir modificaciones en el chunk (0, 0) y jugador
    {
        StorageError openErr = StorageError::None;
        auto storage = manager.openWorld(worldId, openErr);
        assert(storage != nullptr);

        Chunk chunk(0, 0);
        chunk.generateTerrain(7777);
        chunk.setBlock(4, 30, 4, BlockID::Cobblestone);

        storage->beginTransaction();
        storage->stageChunk(chunk);
        assert(storage->commitTransaction() == StorageError::None);

        manager.closeWorld(worldId);
    }

    // Verificar que el esquema inicial es SCHEMA_VERSION_1 (1)
    {
        WorldStorage storage(testSaves / worldId);
        WorldMetadata meta;
        assert(storage.loadMetadata(meta) == StorageError::None);
        assert(meta.schemaVersion == SCHEMA_VERSION_1);
    }

    // 2. Ejecutar migración hacia Esquema v2
    StorageError migErr = manager.migrateWorld(worldId, SCHEMA_VERSION_2);
    assert(migErr == StorageError::None && "La migración a Esquema v2 debe tener éxito");
    (void)migErr;

    // 3. Comprobar que se generó una copia de respaldo en backups/
    bool backupFound = false;
    for (const auto& entry : std::filesystem::directory_iterator(testBackups, ec)) {
        if (entry.is_directory() && entry.path().filename().string().find(worldId + "_backup_") == 0) {
            backupFound = true;
            // Comprobar que el respaldo conservó el esquema original v1
            WorldStorage backupStorage(entry.path());
            WorldMetadata bMeta;
            assert(backupStorage.loadMetadata(bMeta) == StorageError::None);
            assert(bMeta.schemaVersion == SCHEMA_VERSION_1 && "El respaldo debe conservar el esquema original v1");
            break;
        }
    }
    assert(backupFound && "Se debe haber creado una copia de seguridad en el directorio backups/");
    (void)backupFound;

    // 4. Comprobar que el mundo activo ahora está en SCHEMA_VERSION_2
    {
        WorldStorage storage(testSaves / worldId);
        WorldMetadata meta;
        assert(storage.loadMetadata(meta) == StorageError::None);
        assert(meta.schemaVersion == SCHEMA_VERSION_2 && "El esquema en el mundo activo debe haberse actualizado a v2");

        PlayerSaveData player;
        assert(storage.loadPlayer(player) == StorageError::None);
        assert(player.schemaVersion == SCHEMA_VERSION_2);

        Chunk chunk(0, 0);
        assert(storage.loadChunk(0, 0, chunk) == StorageError::None);
        assert(chunk.getBlock(4, 30, 4) == BlockID::Cobblestone &&
               "Los datos de bloques deben permanecer íntegros tras migrar");
    }

    // 5. Comprobar idempotencia: Migrar por segunda vez hacia SCHEMA_VERSION_2
    StorageError migErr2 = manager.migrateWorld(worldId, SCHEMA_VERSION_2);
    assert(migErr2 == StorageError::None && "Una segunda migración debe ser idempotente y exitosa sin errores");
    (void)migErr2;

    {
        WorldStorage storage(testSaves / worldId);
        WorldMetadata meta;
        assert(storage.loadMetadata(meta) == StorageError::None);
        assert(meta.schemaVersion == SCHEMA_VERSION_2);
    }

    std::filesystem::remove_all(testSaves, ec);
    std::filesystem::remove_all(testBackups, ec);
    std::cout << "EXITO: Migración v1 -> v2, copia de seguridad automática e idempotencia verificadas." << std::endl;
    return 0;
}

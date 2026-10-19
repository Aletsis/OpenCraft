#pragma once

#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <system_error>
#include "storage/WorldFormat.hpp"
#include "world/Dimension.hpp"

namespace OpenCraft {

enum class StorageError {
    None,
    FileNotFound,
    IoError,
    CorruptedData,
    ChecksumMismatch,
    DiskFull,
    InvalidMagic,
    UnsupportedVersion,
    TransactionAborted,
    Locked
};

const char* storageErrorToString(StorageError err) noexcept;

struct StagedFile {
    std::filesystem::path finalPath;
    std::filesystem::path tmpPath;
    std::vector<uint8_t> data;
};

class WorldStorage {
public:
    explicit WorldStorage(std::filesystem::path worldDir);
    ~WorldStorage();

    const std::filesystem::path& getWorldDir() const noexcept { return m_worldDir; }

    // Inicialización y recuperación ante fallos
    StorageError init();
    StorageError open([[maybe_unused]] bool createIfMissing = false) { return init(); }
    StorageError initializeWorld(const std::string& name, uint64_t seed, uint32_t generatorVersion = 1);
    StorageError recoverJournalIfNeeded();

    // Comprobación de existencia
    // Comprobación de existencia
    bool exists() const;
    bool hasChunk(int cx, int cz, DimensionType dim = DimensionType::Overworld) const;

    // Operaciones de lectura directa
    StorageError loadMetadata(WorldMetadata& outMeta);
    StorageError loadPlayer(PlayerSaveData& outPlayer);
    StorageError loadPlayer(PlayerSaveData& outPlayer, uint32_t entityId);
    StorageError loadPlayer(PlayerSaveData& outPlayer, const std::string& identifier);
    StorageError loadChunk(int cx, int cz, Chunk& outChunk, BlockRegistry& registry = BlockRegistry::global(),
                           std::vector<MobSaveData>* outMobs = nullptr, DimensionType dim = DimensionType::Overworld);
    StorageError loadEntities(std::vector<ItemEntitySaveData>& outEntities);

    // Transacción atómica de guardado
    StorageError beginTransaction();
    StorageError stageMetadata(const WorldMetadata& meta);
    StorageError stagePlayer(const PlayerSaveData& player);
    StorageError stagePlayer(const PlayerSaveData& player, uint32_t entityId);
    StorageError stagePlayer(const PlayerSaveData& player, const std::string& identifier);
    StorageError stageChunk(const Chunk& chunk, const std::vector<MobSaveData>& mobs = {},
                            DimensionType dim = DimensionType::Overworld);
    StorageError stageEntities(const std::vector<ItemEntitySaveData>& entities);
    StorageError commitTransaction();
    void rollbackTransaction();

    bool isInTransaction() const noexcept { return m_inTransaction; }

    std::vector<uint32_t> getSavedPlayerIds() const;
    std::vector<std::string> getSavedPlayerIdentifiers() const;

    // Rutas de archivos auxiliares
    std::filesystem::path getMetadataPath() const { return m_worldDir / "world.meta"; }
    std::filesystem::path getPlayerPath() const { return m_worldDir / "players" / "player.dat"; }
    std::filesystem::path getPlayerPath(uint32_t entityId) const {
        if (entityId == 0 || entityId == 1) return getPlayerPath();
        return m_worldDir / "players" / ("player_" + std::to_string(entityId) + ".dat");
    }
    std::filesystem::path getPlayerPath(const std::string& identifier) const {
        if (identifier.empty() || identifier == "player") return getPlayerPath();
        if (identifier.rfind("player_", 0) == 0) return m_worldDir / "players" / (identifier + ".dat");
        return m_worldDir / "players" / ("player_" + identifier + ".dat");
    }
    std::filesystem::path getEntitiesPath() const { return m_worldDir / "entities" / "items.dat"; }
    std::filesystem::path getChunkPath(int cx, int cz, DimensionType dim = DimensionType::Overworld) const {
        std::string dimFolder = "chunks";
        if (dim == DimensionType::Abyss)
            dimFolder = "DIM-1";
        else if (dim == DimensionType::Rim)
            dimFolder = "DIM1";
        return m_worldDir / dimFolder / ("c." + std::to_string(cx) + "." + std::to_string(cz) + ".dat");
    }
    std::filesystem::path getJournalPath() const { return m_worldDir / "journal.dat"; }

private:
    std::filesystem::path m_worldDir;
    bool m_inTransaction = false;
    std::vector<StagedFile> m_stagedFiles;

    StorageError writeStagedFile(const StagedFile& staged);
    StorageError readBinaryFile(const std::filesystem::path& path, std::vector<uint8_t>& outBuffer);
};

} // namespace OpenCraft

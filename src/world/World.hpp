#pragma once

#include <unordered_map>
#include <memory>
#include <cstdint>
#include <vector>
#include "world/Chunk.hpp"
#include "world/Block.hpp"
#include "world/BlockRegistry.hpp"
#include "world/ChestBlockEntity.hpp"
#include "world/FurnaceBlockEntity.hpp"
#include "world/CommandBlockEntity.hpp"
#include "world/LightEngine.hpp"

namespace OpenCraft {

struct ChunkCoord {
    int x = 0;
    int z = 0;

    bool operator==(const ChunkCoord& o) const noexcept { return x == o.x && z == o.z; }
};

struct ChunkCoordHash {
    size_t operator()(const ChunkCoord& c) const noexcept {
        return (static_cast<size_t>(c.x) * 73856093) ^ (static_cast<size_t>(c.z) * 19349663);
    }
};

class World {
public:
    World();
    ~World() = default;
    World(World&&) = default;
    World& operator=(World&&) = default;

    BlockID getBlock(int worldX, int worldY, int worldZ) const noexcept;
    void setBlock(int worldX, int worldY, int worldZ, BlockID id);

    BlockStateId getBlockState(int worldX, int worldY, int worldZ) const noexcept;
    void setBlockState(int worldX, int worldY, int worldZ, BlockStateId state);

    // Canales de Luz (rango [0, 15])
    uint8_t getSkyLight(int worldX, int worldY, int worldZ) const noexcept;
    void setSkyLight(int worldX, int worldY, int worldZ, uint8_t level);

    uint8_t getBlockLight(int worldX, int worldY, int worldZ) const noexcept;
    void setBlockLight(int worldX, int worldY, int worldZ, uint8_t level);

    void init(int renderDistance = 3, uint64_t seed = 0, uint32_t generatorVersion = 2);
    // Por defecto escribir en una zona sin chunk crea uno vacío (útil en pruebas y herramientas);
    // el streaming lo desactiva para no tapar terreno que aún no se ha cargado o generado
    void setAutoCreateChunks(bool enabled) noexcept { m_autoCreateChunks = enabled; }
    bool getAutoCreateChunks() const noexcept { return m_autoCreateChunks; }

    Chunk* getChunk(int chunkX, int chunkZ) noexcept;
    const Chunk* getChunk(int chunkX, int chunkZ) const noexcept;
    bool isChunkLoaded(int chunkX, int chunkZ) const noexcept;

    void setChunk(int chunkX, int chunkZ, std::unique_ptr<Chunk> chunk);
    std::unique_ptr<Chunk> removeChunk(int chunkX, int chunkZ);

    const std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash>& getChunks() const noexcept {
        return m_chunks;
    }

    int getRenderDistance() const noexcept { return m_renderDistance; }
    void setRenderDistance(int rd) noexcept { m_renderDistance = rd; }

    int getSimulationDistance() const noexcept { return m_simulationDistance; }
    void setSimulationDistance(int sd) noexcept { m_simulationDistance = sd; }

    uint64_t getSeed() const noexcept { return m_seed; }
    void setSeed(uint64_t seed) noexcept { m_seed = seed; }

    uint32_t getGeneratorVersion() const noexcept { return m_generatorVersion; }
    void setGeneratorVersion(uint32_t version) noexcept { m_generatorVersion = version; }

    // Bloques-Entidad
    void setBlockEntity(int worldX, int worldY, int worldZ, std::shared_ptr<BlockEntity> entity);
    std::shared_ptr<BlockEntity> getBlockEntity(int worldX, int worldY, int worldZ) const;
    std::shared_ptr<BlockEntity> removeBlockEntity(int worldX, int worldY, int worldZ);
    std::shared_ptr<ChestBlockEntity> getChestBlockEntity(int worldX, int worldY, int worldZ) const;
    std::shared_ptr<FurnaceBlockEntity> getFurnaceBlockEntity(int worldX, int worldY, int worldZ) const;
    std::shared_ptr<CommandBlockEntity> getCommandBlockEntity(int worldX, int worldY, int worldZ) const;

    // Calcula un hash 64-bit determinista de todos los bloques del mundo en orden ordenado
    uint64_t computeStateHash() const noexcept;

    // Conteo total de chunks en memoria y estimación de bytes
    size_t getLoadedChunkCount() const noexcept { return m_chunks.size(); }
    size_t getEstimatedMemoryUsage() const noexcept;

    // Motor de Iluminación
    LightEngine& getLightEngine() noexcept { return m_lightEngine; }
    const LightEngine& getLightEngine() const noexcept { return m_lightEngine; }

private:
    bool m_autoCreateChunks = true;
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> m_chunks;
    int m_renderDistance = 4;
    int m_simulationDistance = 3;
    uint64_t m_seed = 0;
    uint32_t m_generatorVersion = 2;
    LightEngine m_lightEngine;
};

} // namespace OpenCraft

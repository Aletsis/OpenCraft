#pragma once

#include <vector>
#include <array>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include "world/Block.hpp"
#include "world/BlockRegistry.hpp"
#include "world/ChunkSection.hpp"
#include "world/BlockEntity.hpp"
#include "math/Math.hpp"

namespace OpenCraft {

enum class ChunkState {
    Unloaded,
    Requested,
    Generating,
    Ready,
    Meshing,
    Visible,
    Unloading
};

const char* chunkStateToString(ChunkState state) noexcept;

class Chunk {
public:
    static constexpr int CHUNK_WIDTH = 16;
    static constexpr int CHUNK_DEPTH = 16;
    static constexpr int SECTION_SIZE = 16;
    static constexpr int MIN_SECTION_Y = -4;
    static constexpr int MAX_SECTION_Y = 19;
    static constexpr int SECTION_COUNT = 24;                         // De -4 a 19 inclusive = 24 secciones
    static constexpr int MIN_Y = MIN_SECTION_Y * SECTION_SIZE;       // -64
    static constexpr int MAX_Y = (MAX_SECTION_Y + 1) * SECTION_SIZE; // 320
    static constexpr int CHUNK_HEIGHT = MAX_Y - MIN_Y;               // 384
    static constexpr int TOTAL_HEIGHT = CHUNK_HEIGHT;

    // Para compatibilidad con estructuras heredadas v1
    static constexpr int LEGACY_HEIGHT = 64;
    static constexpr int LEGACY_BLOCK_COUNT = CHUNK_WIDTH * LEGACY_HEIGHT * CHUNK_DEPTH;

    Chunk(int chunkX, int chunkZ);
    ~Chunk() = default;

    int getChunkX() const noexcept { return m_chunkX; }
    int getChunkZ() const noexcept { return m_chunkZ; }

    ChunkState getState() const noexcept { return m_state; }
    void setState(ChunkState state) noexcept { m_state = state; }

    uint32_t getMeshVersion() const noexcept { return m_meshVersion; }
    void incrementMeshVersion() noexcept { ++m_meshVersion; }

    bool isDirty() const noexcept { return m_isDirty; }
    void markDirty() noexcept {
        m_isDirty = true;
        incrementMeshVersion();
    }
    void clearDirty() noexcept { m_isDirty = false; }

    // Rango vertical de bloques: [-64, 319]
    BlockStateId getBlockState(int x, int y, int z) const noexcept;
    void setBlockState(int x, int y, int z, BlockStateId state) noexcept;

    BlockID getBlock(int x, int y, int z) const noexcept;
    void setBlock(int x, int y, int z, BlockID id) noexcept;

    // Canales de Luz (Cielo y Emisión de Bloque en rango [0, 15])
    uint8_t getSkyLight(int x, int y, int z) const noexcept;
    void setSkyLight(int x, int y, int z, uint8_t level) noexcept;

    uint8_t getBlockLight(int x, int y, int z) const noexcept;
    void setBlockLight(int x, int y, int z, uint8_t level) noexcept;

    // Acceso a secciones verticales
    static constexpr int sectionYToIndex(int sectionY) noexcept { return sectionY - MIN_SECTION_Y; }

    static constexpr int indexToSectionY(int index) noexcept { return index + MIN_SECTION_Y; }

    ChunkSection* getSection(int sectionY) noexcept;
    const ChunkSection* getSection(int sectionY) const noexcept;

    ChunkSection* getSectionByIndex(size_t index) noexcept;
    const ChunkSection* getSectionByIndex(size_t index) const noexcept;

    ChunkSection& getOrCreateSection(int sectionY);
    bool isSectionEmpty(int sectionY) const noexcept;

    // Luz inicial calculada (ChunkLighting); no se persiste, se recalcula al cargar
    bool isLightComputed() const noexcept { return m_lightComputed; }
    void setLightComputed(bool computed) noexcept { m_lightComputed = computed; }

    // Generación determinista de terreno
    void generateTerrain(uint64_t seed = 0, uint32_t generatorVersion = 2);

    // Conteo de memoria aproximada en bytes
    size_t getEstimatedMemoryUsage() const noexcept;

    // Gestión de Bloques-Entidad (Cofres, etc.)
    void setBlockEntity(const BlockPos& pos, std::shared_ptr<BlockEntity> entity);
    std::shared_ptr<BlockEntity> getBlockEntity(const BlockPos& pos) const;
    std::shared_ptr<BlockEntity> removeBlockEntity(const BlockPos& pos);
    const std::unordered_map<BlockPos, std::shared_ptr<BlockEntity>, BlockPosHash>& getBlockEntities() const noexcept {
        return m_blockEntities;
    }
    std::unordered_map<BlockPos, std::shared_ptr<BlockEntity>, BlockPosHash>& getBlockEntities() noexcept {
        return m_blockEntities;
    }

private:
    int m_chunkX = 0;
    int m_chunkZ = 0;
    ChunkState m_state = ChunkState::Ready;
    uint32_t m_meshVersion = 1;
    bool m_isDirty = true;
    bool m_lightComputed = false;

    static std::unique_ptr<ChunkSection> makeSection(int sectionY);

    // Almacenamiento disperso de 24 secciones verticales
    std::array<std::unique_ptr<ChunkSection>, SECTION_COUNT> m_sections{};
    std::unordered_map<BlockPos, std::shared_ptr<BlockEntity>, BlockPosHash> m_blockEntities;
};

} // namespace OpenCraft

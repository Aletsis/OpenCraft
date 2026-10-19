#include "world/Chunk.hpp"
#include "world/WorldGenerator.hpp"
#include <cmath>

namespace OpenCraft {

const char* chunkStateToString(ChunkState state) noexcept {
    switch (state) {
        case ChunkState::Unloaded: return "Unloaded";
        case ChunkState::Requested: return "Requested";
        case ChunkState::Generating: return "Generating";
        case ChunkState::Ready: return "Ready";
        case ChunkState::Meshing: return "Meshing";
        case ChunkState::Visible: return "Visible";
        case ChunkState::Unloading: return "Unloading";
    }
    return "Unknown";
}

static inline int floorDiv16(int a) noexcept {
    int res = a / 16;
    int rem = a % 16;
    if (rem != 0 && (a < 0)) {
        res--;
    }
    return res;
}

static inline int mod16(int a) noexcept { return ((a % 16) + 16) % 16; }

Chunk::Chunk(int chunkX, int chunkZ) : m_chunkX(chunkX), m_chunkZ(chunkZ) {}

BlockStateId Chunk::getBlockState(int x, int y, int z) const noexcept {
    if (x < 0 || x >= CHUNK_WIDTH || z < 0 || z >= CHUNK_DEPTH || y < MIN_Y || y >= MAX_Y) {
        return AIR_BLOCK_STATE;
    }

    int secY = floorDiv16(y);
    int idx = sectionYToIndex(secY);
    if (idx < 0 || idx >= SECTION_COUNT) {
        return AIR_BLOCK_STATE;
    }

    const auto& sec = m_sections[idx];
    if (!sec || sec->isEmpty()) {
        return AIR_BLOCK_STATE;
    }

    return sec->getBlockState(x, mod16(y), z);
}

void Chunk::setBlockState(int x, int y, int z, BlockStateId state) noexcept {
    if (x < 0 || x >= CHUNK_WIDTH || z < 0 || z >= CHUNK_DEPTH || y < MIN_Y || y >= MAX_Y) {
        return;
    }

    int secY = floorDiv16(y);
    int idx = sectionYToIndex(secY);
    if (idx < 0 || idx >= SECTION_COUNT) {
        return;
    }

    if (!m_sections[idx]) {
        if (state == AIR_BLOCK_STATE) {
            return;
        }
        m_sections[idx] = makeSection(secY);
    }

    m_sections[idx]->setBlockState(x, mod16(y), z, state);
    m_isDirty = true;
    incrementMeshVersion();
}

BlockID Chunk::getBlock(int x, int y, int z) const noexcept {
    BlockStateId state = getBlockState(x, y, z);
    return BlockRegistry::global().toLegacyId(state);
}

void Chunk::setBlock(int x, int y, int z, BlockID id) noexcept {
    BlockStateId state = BlockRegistry::global().fromLegacyId(id);
    setBlockState(x, y, z, state);
}

uint8_t Chunk::getSkyLight(int x, int y, int z) const noexcept {
    if (x < 0 || x >= CHUNK_WIDTH || z < 0 || z >= CHUNK_DEPTH || y < MIN_Y || y >= MAX_Y) {
        return 0;
    }
    int secY = floorDiv16(y);
    int idx = sectionYToIndex(secY);
    if (idx < 0 || idx >= SECTION_COUNT || !m_sections[idx]) {
        return 15; // Sección inexistente: solo aire, expuesta al cielo
    }
    return m_sections[idx]->getSkyLight(x, mod16(y), z);
}

void Chunk::setSkyLight(int x, int y, int z, uint8_t level) noexcept {
    if (x < 0 || x >= CHUNK_WIDTH || z < 0 || z >= CHUNK_DEPTH || y < MIN_Y || y >= MAX_Y) {
        return;
    }
    int secY = floorDiv16(y);
    int idx = sectionYToIndex(secY);
    if (idx < 0 || idx >= SECTION_COUNT) {
        return;
    }
    if (!m_sections[idx]) {
        if (level == 15) return;
        m_sections[idx] = makeSection(secY);
    }
    m_sections[idx]->setSkyLight(x, mod16(y), z, level);
    m_isDirty = true;
    incrementMeshVersion();
}

uint8_t Chunk::getBlockLight(int x, int y, int z) const noexcept {
    if (x < 0 || x >= CHUNK_WIDTH || z < 0 || z >= CHUNK_DEPTH || y < MIN_Y || y >= MAX_Y) {
        return 0;
    }
    int secY = floorDiv16(y);
    int idx = sectionYToIndex(secY);
    if (idx < 0 || idx >= SECTION_COUNT || !m_sections[idx]) {
        return 0;
    }
    return m_sections[idx]->getBlockLight(x, mod16(y), z);
}

void Chunk::setBlockLight(int x, int y, int z, uint8_t level) noexcept {
    if (x < 0 || x >= CHUNK_WIDTH || z < 0 || z >= CHUNK_DEPTH || y < MIN_Y || y >= MAX_Y) {
        return;
    }
    int secY = floorDiv16(y);
    int idx = sectionYToIndex(secY);
    if (idx < 0 || idx >= SECTION_COUNT) {
        return;
    }
    if (!m_sections[idx]) {
        if (level == 0) return;
        m_sections[idx] = makeSection(secY);
    }
    m_sections[idx]->setBlockLight(x, mod16(y), z, level);
    m_isDirty = true;
    incrementMeshVersion();
}

ChunkSection* Chunk::getSection(int sectionY) noexcept {
    int idx = sectionYToIndex(sectionY);
    if (idx >= 0 && idx < SECTION_COUNT) {
        return m_sections[idx].get();
    }
    return nullptr;
}

const ChunkSection* Chunk::getSection(int sectionY) const noexcept {
    int idx = sectionYToIndex(sectionY);
    if (idx >= 0 && idx < SECTION_COUNT) {
        return m_sections[idx].get();
    }
    return nullptr;
}

ChunkSection* Chunk::getSectionByIndex(size_t index) noexcept {
    if (index < SECTION_COUNT) {
        return m_sections[index].get();
    }
    return nullptr;
}

const ChunkSection* Chunk::getSectionByIndex(size_t index) const noexcept {
    if (index < SECTION_COUNT) {
        return m_sections[index].get();
    }
    return nullptr;
}

std::unique_ptr<ChunkSection> Chunk::makeSection(int sectionY) {
    // Las secciones nuevas nacen como aire a cielo abierto (luz de cielo 15, luz de bloque 0)
    auto section = std::make_unique<ChunkSection>(sectionY);
    std::array<uint8_t, ChunkSection::BLOCKS_PER_SECTION> light;
    light.fill(0xF0);
    section->setRawLight(light);
    return section;
}

ChunkSection& Chunk::getOrCreateSection(int sectionY) {
    int idx = sectionYToIndex(sectionY);
    if (idx < 0 || idx >= SECTION_COUNT) {
        throw std::out_of_range("Sección Y fuera de rango en Chunk");
    }
    if (!m_sections[idx]) {
        m_sections[idx] = makeSection(sectionY);
    }
    return *m_sections[idx];
}

bool Chunk::isSectionEmpty(int sectionY) const noexcept {
    int idx = sectionYToIndex(sectionY);
    if (idx < 0 || idx >= SECTION_COUNT || !m_sections[idx]) {
        return true;
    }
    return m_sections[idx]->isEmpty();
}

void Chunk::generateTerrain(uint64_t seed, uint32_t generatorVersion) {
    WorldGenerator::generateChunk(m_chunkX, m_chunkZ, *this, seed, generatorVersion);
}

size_t Chunk::getEstimatedMemoryUsage() const noexcept {
    size_t total = sizeof(*this);
    for (const auto& sec : m_sections) {
        if (sec) {
            total += sizeof(ChunkSection);
        }
    }
    return total;
}

void Chunk::setBlockEntity(const BlockPos& pos, std::shared_ptr<BlockEntity> entity) {
    if (entity) {
        m_blockEntities[pos] = std::move(entity);
    } else {
        m_blockEntities.erase(pos);
    }
    markDirty();
}

std::shared_ptr<BlockEntity> Chunk::getBlockEntity(const BlockPos& pos) const {
    auto it = m_blockEntities.find(pos);
    if (it != m_blockEntities.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<BlockEntity> Chunk::removeBlockEntity(const BlockPos& pos) {
    auto it = m_blockEntities.find(pos);
    if (it != m_blockEntities.end()) {
        auto entity = std::move(it->second);
        m_blockEntities.erase(it);
        markDirty();
        return entity;
    }
    return nullptr;
}

} // namespace OpenCraft

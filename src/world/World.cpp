#include "world/World.hpp"
#include "world/ChunkLighting.hpp"
#include <algorithm>
#include <cmath>

namespace OpenCraft {

World::World() = default;

static inline int floorDiv16(int a) noexcept {
    int res = a / 16;
    int rem = a % 16;
    if (rem != 0 && (a < 0)) {
        res--;
    }
    return res;
}

static inline int mod16(int a) noexcept { return ((a % 16) + 16) % 16; }

Chunk* World::getChunk(int chunkX, int chunkZ) noexcept {
    ChunkCoord coord{chunkX, chunkZ};
    auto it = m_chunks.find(coord);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

const Chunk* World::getChunk(int chunkX, int chunkZ) const noexcept {
    ChunkCoord coord{chunkX, chunkZ};
    auto it = m_chunks.find(coord);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool World::isChunkLoaded(int chunkX, int chunkZ) const noexcept {
    return m_chunks.contains(ChunkCoord{chunkX, chunkZ});
}

BlockStateId World::getBlockState(int worldX, int worldY, int worldZ) const noexcept {
    if (worldY < Chunk::MIN_Y || worldY >= Chunk::MAX_Y) {
        return AIR_BLOCK_STATE;
    }

    int cx = floorDiv16(worldX);
    int cz = floorDiv16(worldZ);

    const Chunk* chunk = getChunk(cx, cz);
    if (chunk) {
        int lx = mod16(worldX);
        int lz = mod16(worldZ);
        return chunk->getBlockState(lx, worldY, lz);
    }
    return AIR_BLOCK_STATE;
}

void World::setBlockState(int worldX, int worldY, int worldZ, BlockStateId state) {
    if (worldY < Chunk::MIN_Y || worldY >= Chunk::MAX_Y) {
        return;
    }

    int cx = floorDiv16(worldX);
    int cz = floorDiv16(worldZ);

    int lx = mod16(worldX);
    int lz = mod16(worldZ);

    Chunk* chunk = getChunk(cx, cz);
    if (!chunk) {
        // Con streaming activo no se inventan chunks vacíos: ocuparían el lugar del terreno real
        if (!m_autoCreateChunks) return;
        auto newChunk = std::make_unique<Chunk>(cx, cz);
        chunk = newChunk.get();
        setChunk(cx, cz, std::move(newChunk));
    }
    if (chunk) {
        BlockStateId oldState = chunk->getBlockState(lx, worldY, lz);

        chunk->setBlockState(lx, worldY, lz, state);

        if (oldState != state) {
            BlockID oldBlock = BlockRegistry::global().toLegacyId(oldState);
            BlockID newBlock = BlockRegistry::global().toLegacyId(state);

            // Notificar al motor de iluminación
            if (newBlock == BlockID::Air) {
                m_lightEngine.onBlockBroken(*this, {worldX, worldY, worldZ}, oldBlock);
            } else {
                m_lightEngine.onBlockPlaced(*this, {worldX, worldY, worldZ}, newBlock);
            }
        }

        // Si el bloque está en el borde del chunk, marcar sucio el chunk vecino para remallado
        if (lx == 0) {
            Chunk* neighbor = getChunk(cx - 1, cz);
            if (neighbor) neighbor->markDirty();
        } else if (lx == Chunk::CHUNK_WIDTH - 1) {
            Chunk* neighbor = getChunk(cx + 1, cz);
            if (neighbor) neighbor->markDirty();
        }

        if (lz == 0) {
            Chunk* neighbor = getChunk(cx, cz - 1);
            if (neighbor) neighbor->markDirty();
        } else if (lz == Chunk::CHUNK_DEPTH - 1) {
            Chunk* neighbor = getChunk(cx, cz + 1);
            if (neighbor) neighbor->markDirty();
        }
    }
}

uint8_t World::getSkyLight(int worldX, int worldY, int worldZ) const noexcept {
    if (worldY < Chunk::MIN_Y || worldY >= Chunk::MAX_Y) {
        return (worldY >= Chunk::MAX_Y) ? 15 : 0;
    }
    int cx = floorDiv16(worldX);
    int cz = floorDiv16(worldZ);
    const Chunk* chunk = getChunk(cx, cz);
    if (!chunk) return 0; // Chunks no cargados devuelven 0 (no luz plena)
    return chunk->getSkyLight(mod16(worldX), worldY, mod16(worldZ));
}

void World::setSkyLight(int worldX, int worldY, int worldZ, uint8_t level) {
    if (worldY < Chunk::MIN_Y || worldY >= Chunk::MAX_Y) return;
    int cx = floorDiv16(worldX);
    int cz = floorDiv16(worldZ);
    Chunk* chunk = getChunk(cx, cz);
    if (!chunk) return;
    chunk->setSkyLight(mod16(worldX), worldY, mod16(worldZ), level);
}

uint8_t World::getBlockLight(int worldX, int worldY, int worldZ) const noexcept {
    if (worldY < Chunk::MIN_Y || worldY >= Chunk::MAX_Y) return 0;
    int cx = floorDiv16(worldX);
    int cz = floorDiv16(worldZ);
    const Chunk* chunk = getChunk(cx, cz);
    if (!chunk) return 0; // Chunks no cargados devuelven 0
    return chunk->getBlockLight(mod16(worldX), worldY, mod16(worldZ));
}

void World::setBlockLight(int worldX, int worldY, int worldZ, uint8_t level) {
    if (worldY < Chunk::MIN_Y || worldY >= Chunk::MAX_Y) return;
    int cx = floorDiv16(worldX);
    int cz = floorDiv16(worldZ);
    Chunk* chunk = getChunk(cx, cz);
    if (!chunk) return;
    chunk->setBlockLight(mod16(worldX), worldY, mod16(worldZ), level);
}

BlockID World::getBlock(int worldX, int worldY, int worldZ) const noexcept {
    BlockStateId state = getBlockState(worldX, worldY, worldZ);
    return BlockRegistry::global().toLegacyId(state);
}

void World::setBlock(int worldX, int worldY, int worldZ, BlockID id) {
    BlockStateId state = BlockRegistry::global().fromLegacyId(id);
    setBlockState(worldX, worldY, worldZ, state);
}

void World::setChunk(int chunkX, int chunkZ, std::unique_ptr<Chunk> chunk) {
    ChunkCoord coord{chunkX, chunkZ};
    // Todo chunk que entra al mundo (generado, cargado o creado por portales) llega con luz válida
    if (chunk && !chunk->isLightComputed()) {
        ChunkLighting::computeChunkLight(*chunk);
    }
    m_chunks[coord] = std::move(chunk);
}

std::unique_ptr<Chunk> World::removeChunk(int chunkX, int chunkZ) {
    ChunkCoord coord{chunkX, chunkZ};
    auto it = m_chunks.find(coord);
    if (it != m_chunks.end()) {
        std::unique_ptr<Chunk> chunk = std::move(it->second);
        m_chunks.erase(it);
        return chunk;
    }
    return nullptr;
}

void World::init(int renderDistance, uint64_t seed, uint32_t generatorVersion) {
    m_renderDistance = renderDistance;
    m_seed = seed;
    m_generatorVersion = generatorVersion;
    m_chunks.clear();

    // Generar chunks iniciales en el radio de render
    for (int cx = -m_renderDistance; cx <= m_renderDistance; ++cx) {
        for (int cz = -m_renderDistance; cz <= m_renderDistance; ++cz) {
            ChunkCoord coord{cx, cz};
            auto chunk = std::make_unique<Chunk>(cx, cz);
            chunk->generateTerrain(m_seed, m_generatorVersion);
            m_chunks[coord] = std::move(chunk);
        }
    }

    // Inicializar iluminación para los chunks generados
    for (auto& [coord, chunk] : m_chunks) {
        if (chunk) {
            m_lightEngine.initializeChunkLighting(*this, *chunk);
        }
    }
}

uint64_t World::computeStateHash() const noexcept {
    // 64-bit FNV-1a hash sobre las coordenadas ordenadas y bloques de todos los chunks
    uint64_t hash = 14695981039346656037ULL;
    constexpr uint64_t fnvPrime = 1099511628211ULL;

    std::vector<ChunkCoord> coords;
    coords.reserve(m_chunks.size());
    for (const auto& [coord, chunkPtr] : m_chunks) {
        if (chunkPtr) {
            coords.push_back(coord);
        }
    }

    std::sort(coords.begin(), coords.end(), [](const ChunkCoord& a, const ChunkCoord& b) {
        if (a.x != b.x) return a.x < b.x;
        return a.z < b.z;
    });

    for (const auto& coord : coords) {
        const Chunk* chunk = getChunk(coord.x, coord.z);
        if (!chunk) continue;

        // Hashear coordenadas del chunk
        hash ^= static_cast<uint64_t>(coord.x * 73856093);
        hash *= fnvPrime;
        hash ^= static_cast<uint64_t>(coord.z * 19349663);
        hash *= fnvPrime;

        for (int secIdx = 0; secIdx < Chunk::SECTION_COUNT; ++secIdx) {
            const ChunkSection* sec = chunk->getSectionByIndex(secIdx);
            if (!sec || sec->isEmpty()) {
                hash ^= 0xAAULL;
                hash *= fnvPrime;
            } else {
                const auto& blocks = sec->getRawBlocks();
                for (BlockStateId b : blocks) {
                    hash ^= static_cast<uint64_t>(b & 0xFF);
                    hash *= fnvPrime;
                    hash ^= static_cast<uint64_t>((b >> 8) & 0xFF);
                    hash *= fnvPrime;
                }
            }
        }
    }

    return hash;
}

size_t World::getEstimatedMemoryUsage() const noexcept {
    size_t total = sizeof(*this);
    for (const auto& [coord, chunkPtr] : m_chunks) {
        (void)coord;
        if (chunkPtr) {
            total += chunkPtr->getEstimatedMemoryUsage();
        }
    }
    return total;
}

void World::setBlockEntity(int worldX, int worldY, int worldZ, std::shared_ptr<BlockEntity> entity) {
    int cx = (worldX >= 0) ? (worldX / Chunk::CHUNK_WIDTH) : ((worldX - (Chunk::CHUNK_WIDTH - 1)) / Chunk::CHUNK_WIDTH);
    int cz = (worldZ >= 0) ? (worldZ / Chunk::CHUNK_DEPTH) : ((worldZ - (Chunk::CHUNK_DEPTH - 1)) / Chunk::CHUNK_DEPTH);

    Chunk* chunk = getChunk(cx, cz);
    if (!chunk) {
        if (!m_autoCreateChunks) return;
        auto newChunk = std::make_unique<Chunk>(cx, cz);
        chunk = newChunk.get();
        setChunk(cx, cz, std::move(newChunk));
    }
    if (chunk) {
        chunk->setBlockEntity(BlockPos{worldX, worldY, worldZ}, std::move(entity));
    }
}

std::shared_ptr<BlockEntity> World::getBlockEntity(int worldX, int worldY, int worldZ) const {
    int cx = (worldX >= 0) ? (worldX / Chunk::CHUNK_WIDTH) : ((worldX - (Chunk::CHUNK_WIDTH - 1)) / Chunk::CHUNK_WIDTH);
    int cz = (worldZ >= 0) ? (worldZ / Chunk::CHUNK_DEPTH) : ((worldZ - (Chunk::CHUNK_DEPTH - 1)) / Chunk::CHUNK_DEPTH);

    const Chunk* chunk = getChunk(cx, cz);
    if (chunk) {
        return chunk->getBlockEntity(BlockPos{worldX, worldY, worldZ});
    }
    return nullptr;
}

std::shared_ptr<BlockEntity> World::removeBlockEntity(int worldX, int worldY, int worldZ) {
    int cx = (worldX >= 0) ? (worldX / Chunk::CHUNK_WIDTH) : ((worldX - (Chunk::CHUNK_WIDTH - 1)) / Chunk::CHUNK_WIDTH);
    int cz = (worldZ >= 0) ? (worldZ / Chunk::CHUNK_DEPTH) : ((worldZ - (Chunk::CHUNK_DEPTH - 1)) / Chunk::CHUNK_DEPTH);

    Chunk* chunk = getChunk(cx, cz);
    if (chunk) {
        return chunk->removeBlockEntity(BlockPos{worldX, worldY, worldZ});
    }
    return nullptr;
}

std::shared_ptr<ChestBlockEntity> World::getChestBlockEntity(int worldX, int worldY, int worldZ) const {
    auto entity = getBlockEntity(worldX, worldY, worldZ);
    if (entity && entity->getType() == BlockEntityType::Chest) {
        return std::static_pointer_cast<ChestBlockEntity>(entity);
    }
    return nullptr;
}

std::shared_ptr<FurnaceBlockEntity> World::getFurnaceBlockEntity(int worldX, int worldY, int worldZ) const {
    auto entity = getBlockEntity(worldX, worldY, worldZ);
    if (entity && entity->getType() == BlockEntityType::Furnace) {
        return std::static_pointer_cast<FurnaceBlockEntity>(entity);
    }
    return nullptr;
}

std::shared_ptr<CommandBlockEntity> World::getCommandBlockEntity(int worldX, int worldY, int worldZ) const {
    auto entity = getBlockEntity(worldX, worldY, worldZ);
    if (entity && entity->getType() == BlockEntityType::CommandBlock) {
        return std::static_pointer_cast<CommandBlockEntity>(entity);
    }
    return nullptr;
}

} // namespace OpenCraft

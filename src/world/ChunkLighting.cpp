#include "world/ChunkLighting.hpp"
#include "world/Chunk.hpp"
#include "world/BlockRegistry.hpp"
#include <algorithm>
#include <vector>

namespace OpenCraft::ChunkLighting {

namespace {

struct LightNode {
    int x, y, z;
    uint8_t level;
};

// Atenuación al entrar en un bloque: 15 = opaco, el aire y el cristal restan 1 por paso
uint8_t attenuation(const BlockProperties& p) {
    if (p.isOpaque) return 15;
    return std::max<uint8_t>(1, p.lightFilter);
}

template <typename Get, typename Set>
void flood(Chunk& chunk, std::vector<LightNode>& queue, int minY, int maxY, Get get, Set set) {
    const auto& reg = BlockRegistry::global();
    static constexpr int DIRS[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    size_t head = 0;
    while (head < queue.size()) {
        LightNode n = queue[head++];
        if (n.level <= 1) continue;
        for (const auto& d : DIRS) {
            int nx = n.x + d[0], ny = n.y + d[1], nz = n.z + d[2];
            if (nx < 0 || nx >= Chunk::CHUNK_WIDTH || nz < 0 || nz >= Chunk::CHUNK_DEPTH || ny < minY || ny >= maxY) {
                continue;
            }
            uint8_t att = attenuation(reg.get(chunk.getBlockState(nx, ny, nz)));
            if (att >= n.level) continue;
            uint8_t next = static_cast<uint8_t>(n.level - att);
            // Solo se propaga por celdas almacenables (las secciones inexistentes no guardan luz)
            if (get(nx, ny, nz) < next && set(nx, ny, nz, next)) {
                queue.push_back({nx, ny, nz, next});
            }
        }
    }
    queue.clear();
}

} // namespace

void computeChunkLight(Chunk& chunk) {
    const auto& reg = BlockRegistry::global();

    // Rango vertical ocupado: por encima de la sección más alta todo es cielo abierto (implícito)
    int topIdx = -1;
    for (int i = Chunk::SECTION_COUNT - 1; i >= 0; --i) {
        if (chunk.getSectionByIndex(static_cast<size_t>(i))) {
            topIdx = i;
            break;
        }
    }
    if (topIdx < 0) {
        chunk.setLightComputed(true);
        return;
    }
    const int minY = Chunk::MIN_Y;
    const int maxY = (Chunk::indexToSectionY(topIdx) + 1) * Chunk::SECTION_SIZE;

    // Reiniciar luz de las secciones existentes
    for (int i = 0; i <= topIdx; ++i) {
        if (ChunkSection* sec = chunk.getSectionByIndex(static_cast<size_t>(i))) {
            std::array<uint8_t, ChunkSection::BLOCKS_PER_SECTION> zero{};
            sec->setRawLight(zero);
        }
    }

    auto getSky = [&](int x, int y, int z) -> uint8_t {
        const ChunkSection* s = chunk.getSection(y >= 0 ? y / 16 : (y - 15) / 16);
        return s ? s->getSkyLight(x, ((y % 16) + 16) % 16, z) : 15;
    };
    auto setSky = [&](int x, int y, int z, uint8_t v) -> bool {
        ChunkSection* s = chunk.getSection(y >= 0 ? y / 16 : (y - 15) / 16);
        if (!s) return false;
        s->setSkyLight(x, ((y % 16) + 16) % 16, z, v);
        return true;
    };
    auto getBlk = [&](int x, int y, int z) -> uint8_t {
        const ChunkSection* s = chunk.getSection(y >= 0 ? y / 16 : (y - 15) / 16);
        return s ? s->getBlockLight(x, ((y % 16) + 16) % 16, z) : 0;
    };
    auto setBlk = [&](int x, int y, int z, uint8_t v) -> bool {
        ChunkSection* s = chunk.getSection(y >= 0 ? y / 16 : (y - 15) / 16);
        if (!s) return false;
        s->setBlockLight(x, ((y % 16) + 16) % 16, z, v);
        return true;
    };

    // 1. Luz de cielo descendente por columnas. El aire no atenúa en vertical.
    std::vector<LightNode> queue;
    queue.reserve(4096);
    std::array<int, 256> skyBottom{}; // primera Y (exclusiva) con luz plena por columna
    for (int z = 0; z < Chunk::CHUNK_DEPTH; ++z) {
        for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x) {
            uint8_t level = 15;
            int y = maxY - 1;
            for (; y >= minY && level > 0; --y) {
                const auto& p = reg.get(chunk.getBlockState(x, y, z));
                if (p.isOpaque) {
                    level = 0;
                    break;
                }
                if (p.lightFilter > 1) {
                    level = static_cast<uint8_t>(std::max(0, level - p.lightFilter));
                }
                setSky(x, y, z, level);
            }
            skyBottom[static_cast<size_t>(z * 16 + x)] = y + 1;
        }
    }

    // 2. Inundación horizontal desde las celdas iluminadas que bordean zonas en sombra
    for (int z = 0; z < Chunk::CHUNK_DEPTH; ++z) {
        for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x) {
            int bottom = skyBottom[static_cast<size_t>(z * 16 + x)];
            int deepestNeighbor = bottom;
            if (x > 0) deepestNeighbor = std::max(deepestNeighbor, skyBottom[static_cast<size_t>(z * 16 + x - 1)]);
            if (x < 15) deepestNeighbor = std::max(deepestNeighbor, skyBottom[static_cast<size_t>(z * 16 + x + 1)]);
            if (z > 0) deepestNeighbor = std::max(deepestNeighbor, skyBottom[static_cast<size_t>((z - 1) * 16 + x)]);
            if (z < 15) deepestNeighbor = std::max(deepestNeighbor, skyBottom[static_cast<size_t>((z + 1) * 16 + x)]);
            // Solo las celdas iluminadas junto a vecinos más altos (en sombra) pueden propagar
            for (int y = bottom; y < deepestNeighbor && y < maxY; ++y) {
                uint8_t l = getSky(x, y, z);
                if (l > 1) queue.push_back({x, y, z, l});
            }
            if (bottom < maxY && bottom > minY) {
                uint8_t l = getSky(x, bottom, z);
                if (l > 1) queue.push_back({x, bottom, z, l});
            }
        }
    }
    flood(chunk, queue, minY, maxY, getSky, setSky);

    // 3. Luz de bloques emisores (lava, antorchas, piedra luminosa...)
    for (int i = 0; i <= topIdx; ++i) {
        const ChunkSection* sec = chunk.getSectionByIndex(static_cast<size_t>(i));
        if (!sec || sec->isEmpty()) continue;
        int baseY = Chunk::indexToSectionY(i) * Chunk::SECTION_SIZE;
        const auto& blocks = sec->getRawBlocks();
        for (int ly = 0; ly < 16; ++ly) {
            for (int lz = 0; lz < 16; ++lz) {
                for (int lx = 0; lx < 16; ++lx) {
                    BlockStateId st = blocks[ChunkSection::getIndex(lx, ly, lz)];
                    if (st == AIR_BLOCK_STATE) continue;
                    uint8_t e = reg.get(st).lightEmission;
                    if (e > 0) {
                        setBlk(lx, baseY + ly, lz, e);
                        queue.push_back({lx, baseY + ly, lz, e});
                    }
                }
            }
        }
    }
    flood(chunk, queue, minY, maxY, getBlk, setBlk);

    chunk.setLightComputed(true);
}

} // namespace OpenCraft::ChunkLighting

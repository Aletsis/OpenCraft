#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include "world/Chunk.hpp"
#include "world/ChunkMesh.hpp"

namespace OpenCraft {

class World;

// Geometría de una sección de 16^3 separada por pase de render
struct SectionMeshData {
    int sectionY = 0;
    std::array<std::vector<VoxelVertex>, 3> vertices;
    std::array<std::vector<uint32_t>, 3> indices;

    bool empty() const noexcept { return indices[0].empty() && indices[1].empty() && indices[2].empty(); }
};

struct ChunkMeshBuildResult {
    int chunkX = 0;
    int chunkZ = 0;
    uint32_t meshVersion = 0;
    std::vector<SectionMeshData> sections;
};

// Copia autocontenida (bloques y luz) de un chunk más un borde de un bloque de sus vecinos.
// Se captura en el hilo principal y se malla en cualquier hilo sin tocar el mundo.
struct ChunkMeshInput {
    static constexpr int PAD = 18; // 16 + borde a cada lado

    int chunkX = 0;
    int chunkZ = 0;
    uint32_t meshVersion = 0;
    int minY = 0; // rango capturado [minY, maxY)
    int maxY = 0;
    std::array<bool, Chunk::SECTION_COUNT> sectionHasBlocks{};
    std::vector<BlockStateId> blocks;
    std::vector<uint8_t> light; // luz de cielo en el nibble alto, de bloque en el bajo

    size_t index(int lx, int y, int lz) const noexcept {
        return (static_cast<size_t>(y - minY) * PAD + static_cast<size_t>(lz + 1)) * PAD + static_cast<size_t>(lx + 1);
    }
    BlockStateId blockAt(int lx, int y, int lz) const noexcept;
    uint8_t skyAt(int lx, int y, int lz) const noexcept;
    uint8_t blockLightAt(int lx, int y, int lz) const noexcept;
};

namespace ChunkMesher {

// Bloque usado para los vecinos aún no cargados: oculta caras que se reconstruirán cuando llegue
constexpr BlockStateId UNLOADED_NEIGHBOR = 0xFFFF;

ChunkMeshInput capture(const World& world, const Chunk& chunk);
void build(const ChunkMeshInput& input, ChunkMeshBuildResult& result);

} // namespace ChunkMesher

} // namespace OpenCraft

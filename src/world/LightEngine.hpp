#pragma once

#include <queue>
#include <vector>
#include <cstdint>
#include <array>
#include "math/Math.hpp"
#include "world/Block.hpp"
#include "world/BlockEntity.hpp"

namespace OpenCraft {

class World;
class Chunk;

struct LightNode {
    BlockPos pos;
    uint8_t level = 0;
};

struct LightRemovalNode {
    BlockPos pos;
    uint8_t level = 0;
};

class LightEngine {
public:
    LightEngine() = default;
    ~LightEngine() = default;

    // Propagación y retirada de luz de bloques (emisión: antorchas, lava, fuego)
    void propagateBlockLight(World& world);
    void removeBlockLight(World& world);

    // Propagación y retirada de luz de cielo (sol)
    void propagateSkyLight(World& world);
    void removeSkyLight(World& world);

    // Notificaciones de colocación y rotura de bloques
    void onBlockPlaced(World& world, const BlockPos& pos, BlockID newBlock);
    void onBlockBroken(World& world, const BlockPos& pos, BlockID oldBlock);

    // Inicialización de iluminación al generar o cargar un nuevo chunk
    void initializeChunkLighting(World& world, Chunk& chunk);

    // Encolar manualmente nodos para pruebas y fixtures
    void queueBlockLightSpread(const BlockPos& pos, uint8_t level);
    void queueSkyLightSpread(const BlockPos& pos, uint8_t level);

private:
    std::queue<LightNode> m_blockLightQueue;
    std::queue<LightRemovalNode> m_blockLightRemovalQueue;

    std::queue<LightNode> m_skyLightQueue;
    std::queue<LightRemovalNode> m_skyLightRemovalQueue;
};

} // namespace OpenCraft

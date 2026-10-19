#include "world/LightEngine.hpp"
#include "world/World.hpp"
#include "world/Chunk.hpp"
#include <algorithm>

namespace OpenCraft {

static inline int floorDiv16(int a) noexcept {
    int res = a / 16;
    int rem = a % 16;
    if (rem != 0 && (a < 0)) {
        res--;
    }
    return res;
}

static constexpr std::array<BlockPos, 6> s_offsets = {
    {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};

void LightEngine::queueBlockLightSpread(const BlockPos& pos, uint8_t level) { m_blockLightQueue.push({pos, level}); }

void LightEngine::queueSkyLightSpread(const BlockPos& pos, uint8_t level) { m_skyLightQueue.push({pos, level}); }

void LightEngine::propagateBlockLight(World& world) {
    while (!m_blockLightQueue.empty()) {
        LightNode node = m_blockLightQueue.front();
        m_blockLightQueue.pop();

        uint8_t currentLevel = world.getBlockLight(node.pos.x, node.pos.y, node.pos.z);
        if (currentLevel < node.level) {
            currentLevel = node.level;
            world.setBlockLight(node.pos.x, node.pos.y, node.pos.z, currentLevel);
        }

        if (currentLevel <= 1) continue;

        for (const auto& off : s_offsets) {
            BlockPos npos{node.pos.x + off.x, node.pos.y + off.y, node.pos.z + off.z};
            if (npos.y < Chunk::MIN_Y || npos.y >= Chunk::MAX_Y) continue;

            int nChunkX = floorDiv16(npos.x);
            int nChunkZ = floorDiv16(npos.z);
            if (!world.isChunkLoaded(nChunkX, nChunkZ)) continue;

            BlockID nBlock = world.getBlock(npos.x, npos.y, npos.z);
            const auto& nInfo = BlockDatabase::get(nBlock);
            uint8_t filter = nInfo.lightFilter;
            if (filter >= 15) continue; // Bloque opaco bloquea la luz completamente

            uint8_t targetLevel = (currentLevel > filter) ? static_cast<uint8_t>(currentLevel - filter) : 0;
            if (targetLevel > 0 && world.getBlockLight(npos.x, npos.y, npos.z) < targetLevel) {
                world.setBlockLight(npos.x, npos.y, npos.z, targetLevel);
                m_blockLightQueue.push({npos, targetLevel});
            }
        }
    }
}

void LightEngine::removeBlockLight(World& world) {
    while (!m_blockLightRemovalQueue.empty()) {
        LightRemovalNode remNode = m_blockLightRemovalQueue.front();
        m_blockLightRemovalQueue.pop();

        for (const auto& off : s_offsets) {
            BlockPos npos{remNode.pos.x + off.x, remNode.pos.y + off.y, remNode.pos.z + off.z};
            if (npos.y < Chunk::MIN_Y || npos.y >= Chunk::MAX_Y) continue;

            int nChunkX = floorDiv16(npos.x);
            int nChunkZ = floorDiv16(npos.z);
            if (!world.isChunkLoaded(nChunkX, nChunkZ)) continue;

            uint8_t nLevel = world.getBlockLight(npos.x, npos.y, npos.z);
            if (nLevel != 0 && nLevel < remNode.level) {
                world.setBlockLight(npos.x, npos.y, npos.z, 0);
                m_blockLightRemovalQueue.push({npos, nLevel});
            } else if (nLevel > remNode.level) {
                m_blockLightQueue.push({npos, nLevel});
            }
        }
    }

    // Re-propagar desde las fuentes restantes
    propagateBlockLight(world);
}

void LightEngine::propagateSkyLight(World& world) {
    while (!m_skyLightQueue.empty()) {
        LightNode node = m_skyLightQueue.front();
        m_skyLightQueue.pop();

        uint8_t currentLevel = world.getSkyLight(node.pos.x, node.pos.y, node.pos.z);
        if (currentLevel < node.level) {
            currentLevel = node.level;
            world.setSkyLight(node.pos.x, node.pos.y, node.pos.z, currentLevel);
        }

        if (currentLevel <= 1) continue;

        for (const auto& off : s_offsets) {
            BlockPos npos{node.pos.x + off.x, node.pos.y + off.y, node.pos.z + off.z};
            if (npos.y < Chunk::MIN_Y || npos.y >= Chunk::MAX_Y) continue;

            int nChunkX = floorDiv16(npos.x);
            int nChunkZ = floorDiv16(npos.z);
            if (!world.isChunkLoaded(nChunkX, nChunkZ)) continue;

            BlockID nBlock = world.getBlock(npos.x, npos.y, npos.z);
            const auto& nInfo = BlockDatabase::get(nBlock);
            uint8_t filter = nInfo.lightFilter;
            if (filter >= 15) continue;

            uint8_t targetLevel = 0;
            // Si la propagación es hacia abajo (-Y) y el origen tenía 15 sin filtro, mantiene 15
            if (off.y == -1 && currentLevel == 15 && filter <= 1) {
                targetLevel = 15;
            } else {
                targetLevel = (currentLevel > filter) ? static_cast<uint8_t>(currentLevel - filter) : 0;
            }

            if (targetLevel > 0 && world.getSkyLight(npos.x, npos.y, npos.z) < targetLevel) {
                world.setSkyLight(npos.x, npos.y, npos.z, targetLevel);
                m_skyLightQueue.push({npos, targetLevel});
            }
        }
    }
}

void LightEngine::removeSkyLight(World& world) {
    while (!m_skyLightRemovalQueue.empty()) {
        LightRemovalNode remNode = m_skyLightRemovalQueue.front();
        m_skyLightRemovalQueue.pop();

        for (const auto& off : s_offsets) {
            BlockPos npos{remNode.pos.x + off.x, remNode.pos.y + off.y, remNode.pos.z + off.z};
            if (npos.y < Chunk::MIN_Y || npos.y >= Chunk::MAX_Y) continue;

            int nChunkX = floorDiv16(npos.x);
            int nChunkZ = floorDiv16(npos.z);
            if (!world.isChunkLoaded(nChunkX, nChunkZ)) continue;

            uint8_t nLevel = world.getSkyLight(npos.x, npos.y, npos.z);
            if (nLevel != 0 && (nLevel < remNode.level || (off.y == -1 && remNode.level == 15))) {
                world.setSkyLight(npos.x, npos.y, npos.z, 0);
                m_skyLightRemovalQueue.push({npos, nLevel});
            } else if (nLevel > remNode.level) {
                m_skyLightQueue.push({npos, nLevel});
            }
        }
    }

    // Re-propagar desde los huecos de cielo restantes
    propagateSkyLight(world);
}

void LightEngine::onBlockPlaced(World& world, const BlockPos& pos, BlockID newBlock) {
    const auto& info = BlockDatabase::get(newBlock);

    // 1. Emisión de luz del nuevo bloque (ej. antorcha, fuego, lava)
    if (info.lightEmission > 0) {
        world.setBlockLight(pos.x, pos.y, pos.z, info.lightEmission);
        m_blockLightQueue.push({pos, info.lightEmission});
        propagateBlockLight(world);
    }

    // 2. Si es un bloque opaco o atenuador, retirar la luz que pasaba por allí
    if (info.lightFilter > 1) {
        uint8_t oldBlockLight = world.getBlockLight(pos.x, pos.y, pos.z);
        if (oldBlockLight > 0 && info.lightEmission == 0) {
            world.setBlockLight(pos.x, pos.y, pos.z, 0);
            m_blockLightRemovalQueue.push({pos, oldBlockLight});
            removeBlockLight(world);
        }

        uint8_t oldSkyLight = world.getSkyLight(pos.x, pos.y, pos.z);
        if (oldSkyLight > 0) {
            world.setSkyLight(pos.x, pos.y, pos.z, 0);
            m_skyLightRemovalQueue.push({pos, oldSkyLight});
            removeSkyLight(world);
        }
    }
}

void LightEngine::onBlockBroken(World& world, const BlockPos& pos, BlockID oldBlock) {
    const auto& oldInfo = BlockDatabase::get(oldBlock);

    // 1. Si era emisor de luz, retirar su brillo
    if (oldInfo.lightEmission > 0) {
        world.setBlockLight(pos.x, pos.y, pos.z, 0);
        m_blockLightRemovalQueue.push({pos, oldInfo.lightEmission});
        removeBlockLight(world);
    }

    // 2. Si era bloque opaco, permitir que la luz de vecinos penetre en este espacio
    if (oldInfo.lightFilter >= 15) {
        // Consultar vecinos para luz de bloque
        for (const auto& off : s_offsets) {
            BlockPos npos{pos.x + off.x, pos.y + off.y, pos.z + off.z};
            if (npos.y < Chunk::MIN_Y || npos.y >= Chunk::MAX_Y) continue;
            uint8_t nbLight = world.getBlockLight(npos.x, npos.y, npos.z);
            if (nbLight > 1) {
                m_blockLightQueue.push({npos, nbLight});
            }
            uint8_t nSkyLight = world.getSkyLight(npos.x, npos.y, npos.z);
            if (nSkyLight > 1) {
                m_skyLightQueue.push({npos, nSkyLight});
            }
        }

        // Si sobre este bloque hay cielo abierto (luz 15), raycast vertical hacia abajo
        if (pos.y + 1 < Chunk::MAX_Y && world.getSkyLight(pos.x, pos.y + 1, pos.z) == 15) {
            for (int y = pos.y; y >= Chunk::MIN_Y; --y) {
                BlockID b = world.getBlock(pos.x, y, pos.z);
                const auto& bInfo = BlockDatabase::get(b);
                if (bInfo.lightFilter >= 15) break;

                uint8_t lvl = (bInfo.lightFilter <= 1) ? 15 : static_cast<uint8_t>(15 - bInfo.lightFilter);
                world.setSkyLight(pos.x, y, pos.z, lvl);
                m_skyLightQueue.push({{pos.x, y, pos.z}, lvl});
                if (bInfo.lightFilter > 1) break;
            }
        }

        propagateBlockLight(world);
        propagateSkyLight(world);
    }
}

void LightEngine::initializeChunkLighting(World& world, Chunk& chunk) {
    int startX = chunk.getChunkX() * Chunk::CHUNK_WIDTH;
    int startZ = chunk.getChunkZ() * Chunk::CHUNK_DEPTH;

    // Raycast vertical de luz solar para cada columna (x, z)
    for (int lz = 0; lz < Chunk::CHUNK_DEPTH; ++lz) {
        int wz = startZ + lz;
        for (int lx = 0; lx < Chunk::CHUNK_WIDTH; ++lx) {
            int wx = startX + lx;
            bool hitObstacle = false;

            for (int wy = Chunk::MAX_Y - 1; wy >= Chunk::MIN_Y; --wy) {
                BlockID b = chunk.getBlock(lx, wy, lz);
                const auto& info = BlockDatabase::get(b);

                // Emisión de bloques (antorchas, lava, fuego)
                if (info.lightEmission > 0) {
                    chunk.setBlockLight(lx, wy, lz, info.lightEmission);
                    m_blockLightQueue.push(LightNode{BlockPos{wx, wy, wz}, info.lightEmission});
                }

                if (!hitObstacle) {
                    if (info.lightFilter <= 1) {
                        chunk.setSkyLight(lx, wy, lz, 15);
                    } else {
                        // Primer obstáculo (hojas, agua, tierra, etc.)
                        uint8_t lvl = (info.lightFilter >= 15) ? 0 : static_cast<uint8_t>(15 - info.lightFilter);
                        chunk.setSkyLight(lx, wy, lz, lvl);
                        if (lvl > 0) {
                            m_skyLightQueue.push(LightNode{BlockPos{wx, wy, wz}, lvl});
                        }
                        hitObstacle = true;
                    }
                } else {
                    chunk.setSkyLight(lx, wy, lz, 0);
                }
            }
        }
    }

    // Propagar luz acumulada a vecinos
    propagateSkyLight(world);
    propagateBlockLight(world);
}

} // namespace OpenCraft

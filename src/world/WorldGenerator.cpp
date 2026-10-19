#include "world/WorldGenerator.hpp"
#include "world/Chunk.hpp"
#include "world/BlockRegistry.hpp"
#include "world/Biome.hpp"
#include "world/StructureManager.hpp"
#include "world/ChestBlockEntity.hpp"
#include <cmath>
#include <algorithm>

namespace OpenCraft {

static int floorInt(float val) noexcept {
    int i = static_cast<int>(val);
    return (val < static_cast<float>(i)) ? (i - 1) : i;
}

uint64_t WorldGenerator::hashCoords(int64_t x, int64_t z, uint64_t seed) noexcept {
    uint64_t h =
        seed ^ (static_cast<uint64_t>(x) * 0x9E3779B97F4A7C15ULL) ^ (static_cast<uint64_t>(z) * 0xBF58476D1CE4E5B9ULL);
    h = (h ^ (h >> 30)) * 0xBF58476D1CE4E5B9ULL;
    h = (h ^ (h >> 27)) * 0x94D049BB133111EBULL;
    return h ^ (h >> 31);
}

uint64_t WorldGenerator::hash3D(int64_t x, int64_t y, int64_t z, uint64_t seed) noexcept {
    uint64_t h = seed ^ (static_cast<uint64_t>(x) * 0x9E3779B97F4A7C15ULL) ^
                 (static_cast<uint64_t>(y) * 0x517CC1B727220A95ULL) ^
                 (static_cast<uint64_t>(z) * 0xBF58476D1CE4E5B9ULL);
    h = (h ^ (h >> 30)) * 0xBF58476D1CE4E5B9ULL;
    h = (h ^ (h >> 27)) * 0x94D049BB133111EBULL;
    return h ^ (h >> 31);
}

float WorldGenerator::dotGradient(int ix, int iz, float x, float z, uint64_t seed) noexcept {
    uint64_t h = hashCoords(ix, iz, seed);
    // 8 direcciones de gradientes enteros normalizados
    static constexpr float GRADIENTS[8][2] = {{1.0f, 0.0f},
                                              {-1.0f, 0.0f},
                                              {0.0f, 1.0f},
                                              {0.0f, -1.0f},
                                              {0.70710678f, 0.70710678f},
                                              {-0.70710678f, 0.70710678f},
                                              {0.70710678f, -0.70710678f},
                                              {-0.70710678f, -0.70710678f}};
    const auto& g = GRADIENTS[h & 7];
    float dx = x - static_cast<float>(ix);
    float dz = z - static_cast<float>(iz);
    return dx * g[0] + dz * g[1];
}

float WorldGenerator::gradientNoise2D(float x, float z, uint64_t seed) noexcept {
    int x0 = floorInt(x);
    int x1 = x0 + 1;
    int z0 = floorInt(z);
    int z1 = z0 + 1;

    float sx = smoothstep(x - static_cast<float>(x0));
    float sz = smoothstep(z - static_cast<float>(z0));

    float n00 = dotGradient(x0, z0, x, z, seed);
    float n10 = dotGradient(x1, z0, x, z, seed);
    float n01 = dotGradient(x0, z1, x, z, seed);
    float n11 = dotGradient(x1, z1, x, z, seed);

    float ix0 = n00 + sx * (n10 - n00);
    float ix1 = n01 + sx * (n11 - n01);

    return ix0 + sz * (ix1 - ix0);
}

float WorldGenerator::octaveNoise2D(float x, float z, uint64_t seed, int octaves, float persistence,
                                    float lacunarity) noexcept {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        total += gradientNoise2D(x * frequency, z * frequency, seed + static_cast<uint64_t>(i * 31)) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return total / maxValue;
}

void WorldGenerator::generateChunk(int chunkX, int chunkZ, Chunk& chunk, uint64_t seed, uint32_t version) {
    auto& reg = BlockRegistry::global();
    BlockStateId bedrockState = reg.fromLegacyId(BlockID::Bedrock);
    BlockStateId stoneState = reg.fromLegacyId(BlockID::Stone);
    BlockStateId dirtState = reg.fromLegacyId(BlockID::Dirt);
    BlockStateId grassState = reg.fromLegacyId(BlockID::Grass);
    BlockStateId woodState = reg.fromLegacyId(BlockID::Wood);
    BlockStateId leavesState = reg.fromLegacyId(BlockID::Leaves);

    int startWorldX = chunkX * Chunk::CHUNK_WIDTH;
    int startWorldZ = chunkZ * Chunk::CHUNK_DEPTH;

    if (version == 1) {
        // Generador legacy para pruebas o compatibilidad v1
        for (int lx = 0; lx < Chunk::CHUNK_WIDTH; ++lx) {
            for (int lz = 0; lz < Chunk::CHUNK_DEPTH; ++lz) {
                int wx = startWorldX + lx;
                int wz = startWorldZ + lz;

                float n = gradientNoise2D(static_cast<float>(wx) * 0.05f, static_cast<float>(wz) * 0.05f, seed);
                int height = static_cast<int>(26.0f + n * 12.0f);
                if (height < 3) height = 3;
                if (height > 55) height = 55;

                for (int y = 0; y <= height; ++y) {
                    if (y == 0)
                        chunk.setBlockState(lx, y, lz, bedrockState);
                    else if (y < height - 3)
                        chunk.setBlockState(lx, y, lz, stoneState);
                    else if (y < height)
                        chunk.setBlockState(lx, y, lz, dirtState);
                    else
                        chunk.setBlockState(lx, y, lz, grassState);
                }
            }
        }
        chunk.clearDirty();
        return;
    }

    // =========================================================================
    // Generador v2: Rango vertical [-64, 319], determinista por semilla
    // =========================================================================
    if (version == 2) {
        struct TreeInfo {
            int x, y, z;
        };
        std::vector<TreeInfo> trees;

        for (int lx = 0; lx < Chunk::CHUNK_WIDTH; ++lx) {
            for (int lz = 0; lz < Chunk::CHUNK_DEPTH; ++lz) {
                int wx = startWorldX + lx;
                int wz = startWorldZ + lz;

                // Ruido continental y de relieve
                float noise = octaveNoise2D(static_cast<float>(wx) * 0.008f, static_cast<float>(wz) * 0.008f, seed, 4,
                                            0.5f, 2.0f);

                // Altura de superficie alrededor de y=64 (rango típico 40..110)
                int surfaceHeight = static_cast<int>(64.0f + noise * 40.0f);
                if (surfaceHeight < -40) surfaceHeight = -40;
                if (surfaceHeight > 280) surfaceHeight = 280;

                // 1. Capa de Lecho de roca (Bedrock) en el fondo [-64, -60]
                chunk.setBlockState(lx, Chunk::MIN_Y, lz, bedrockState);
                for (int y = Chunk::MIN_Y + 1; y <= Chunk::MIN_Y + 4; ++y) {
                    uint64_t bh = hash3D(wx, y, wz, seed ^ 0xBEBED0CCULL);
                    if ((bh % 100) < static_cast<uint64_t>((Chunk::MIN_Y + 5 - y) * 20)) {
                        chunk.setBlockState(lx, y, lz, bedrockState);
                    } else {
                        chunk.setBlockState(lx, y, lz, stoneState);
                    }
                }

                // 2. Columna de Piedra (Subsuelo)
                for (int y = Chunk::MIN_Y + 5; y < surfaceHeight - 3; ++y) {
                    chunk.setBlockState(lx, y, lz, stoneState);
                }

                // 3. Capas de Tierra
                for (int y = std::max(Chunk::MIN_Y + 5, surfaceHeight - 3); y < surfaceHeight; ++y) {
                    chunk.setBlockState(lx, y, lz, dirtState);
                }

                // 4. Bloque superficial (Hierba)
                if (surfaceHeight >= Chunk::MIN_Y) {
                    chunk.setBlockState(lx, surfaceHeight, lz, grassState);
                }

                // 5. Determinar colocación de árboles
                if (lx >= 2 && lx <= 13 && lz >= 2 && lz <= 13 && surfaceHeight >= 10 && surfaceHeight < 240) {
                    uint64_t th = hashCoords(wx, wz, seed ^ 0x72EE501DULL);
                    if ((th % 75) == 0) {
                        trees.push_back({lx, surfaceHeight + 1, lz});
                    }
                }
            }
        }

        // Generar árboles registrados
        for (const auto& tree : trees) {
            int tx = tree.x;
            int ty = tree.y;
            int tz = tree.z;
            int trunkHeight = 4;

            // Tronco
            for (int h = 0; h < trunkHeight; ++h) {
                chunk.setBlockState(tx, ty + h, tz, woodState);
            }

            // Corona de hojas
            int leafBase = ty + trunkHeight - 2;
            for (int ox = -2; ox <= 2; ++ox) {
                for (int oz = -2; oz <= 2; ++oz) {
                    for (int oy = 0; oy < 2; ++oy) {
                        if (std::abs(ox) == 2 && std::abs(oz) == 2 && oy == 1) continue;
                        int lx = tx + ox;
                        int ly = leafBase + oy;
                        int lz = tz + oz;
                        if (lx >= 0 && lx < Chunk::CHUNK_WIDTH && lz >= 0 && lz < Chunk::CHUNK_DEPTH) {
                            if (chunk.getBlockState(lx, ly, lz) == AIR_BLOCK_STATE) {
                                chunk.setBlockState(lx, ly, lz, leavesState);
                            }
                        }
                    }
                }
            }

            // Copa
            for (int ox = -1; ox <= 1; ++ox) {
                for (int oz = -1; oz <= 1; ++oz) {
                    if (std::abs(ox) == 1 && std::abs(oz) == 1) continue;
                    int lx = tx + ox;
                    int ly = ty + trunkHeight;
                    int lz = tz + oz;
                    if (lx >= 0 && lx < Chunk::CHUNK_WIDTH && lz >= 0 && lz < Chunk::CHUNK_DEPTH) {
                        if (chunk.getBlockState(lx, ly, lz) == AIR_BLOCK_STATE) {
                            chunk.setBlockState(lx, ly, lz, leavesState);
                        }
                    }
                }
            }
            if (chunk.getBlockState(tx, ty + trunkHeight + 1, tz) == AIR_BLOCK_STATE) {
                chunk.setBlockState(tx, ty + trunkHeight + 1, tz, leavesState);
            }
        }

        chunk.clearDirty();
        return;
    }

    // =========================================================================
    // Generador v3 (Fase 10): 6 Biomas, Densidad 3D (Cuevas/Acuíferos),
    // Distribución de Minerales y Estructuras Multi-Chunk Deterministas
    // =========================================================================
    BlockStateId airState = AIR_BLOCK_STATE;
    BlockStateId sandState = reg.fromLegacyId(BlockID::Sand);
    BlockStateId snowBlockState = reg.fromLegacyId(BlockID::SnowBlock);
    BlockStateId iceState = reg.fromLegacyId(BlockID::Ice);
    BlockStateId waterState = reg.fromLegacyId(BlockID::Water);

    BlockStateId coalOreState = reg.fromLegacyId(BlockID::CoalOre);
    BlockStateId ironOreState = reg.fromLegacyId(BlockID::IronOre);
    BlockStateId goldOreState = reg.fromLegacyId(BlockID::GoldOre);
    BlockStateId diamondOreState = reg.fromLegacyId(BlockID::DiamondOre);
    BlockStateId emeraldOreState = reg.fromLegacyId(BlockID::EmeraldOre);

    struct FeatureInfo {
        int x, y, z;
        BiomeType biome;
    };
    std::vector<FeatureInfo> features;

    // Pasada 1: Terreno y Biomas según clima (T, H, C, R)
    for (int lx = 0; lx < Chunk::CHUNK_WIDTH; ++lx) {
        for (int lz = 0; lz < Chunk::CHUNK_DEPTH; ++lz) {
            int wx = startWorldX + lx;
            int wz = startWorldZ + lz;

            float temp = octaveNoise2D(static_cast<float>(wx) * 0.003f, static_cast<float>(wz) * 0.003f, seed + 101, 3,
                                       0.5f, 2.0f);
            float hum = octaveNoise2D(static_cast<float>(wx) * 0.003f, static_cast<float>(wz) * 0.003f, seed + 202, 3,
                                      0.5f, 2.0f);
            float cont = octaveNoise2D(static_cast<float>(wx) * 0.005f, static_cast<float>(wz) * 0.005f, seed + 303, 4,
                                       0.5f, 2.0f);
            float rel = octaveNoise2D(static_cast<float>(wx) * 0.008f, static_cast<float>(wz) * 0.008f, seed + 404, 4,
                                      0.5f, 2.0f);

            int surfaceHeight = 64;
            if (cont < -0.22f) {
                // Bioma Océano
                surfaceHeight = static_cast<int>(38.0f + (cont + 0.6f) * 35.0f);
            } else if (rel > 0.40f) {
                // Bioma Montaña
                surfaceHeight = static_cast<int>(74.0f + rel * 65.0f);
            } else {
                // Tierras continentales: Llanura, Bosque, Desierto, Nieve
                surfaceHeight = static_cast<int>(63.0f + rel * 22.0f);
            }

            if (surfaceHeight < -40) surfaceHeight = -40;
            if (surfaceHeight > 260) surfaceHeight = 260;

            BiomeType biome = BiomeRegistry::determineBiome(temp, hum, cont, static_cast<float>(surfaceHeight));
            const auto& bProps = BiomeRegistry::getProperties(biome);

            BlockStateId surfState = reg.fromLegacyId(bProps.surfaceBlock);
            BlockStateId subState = reg.fromLegacyId(bProps.subSurfaceBlock);

            // Capa de Lecho de roca
            chunk.setBlockState(lx, Chunk::MIN_Y, lz, bedrockState);
            for (int y = Chunk::MIN_Y + 1; y <= Chunk::MIN_Y + 4; ++y) {
                uint64_t bh = hash3D(wx, y, wz, seed ^ 0xBEBED0CCULL);
                if ((bh % 100) < static_cast<uint64_t>((Chunk::MIN_Y + 5 - y) * 20)) {
                    chunk.setBlockState(lx, y, lz, bedrockState);
                } else {
                    chunk.setBlockState(lx, y, lz, stoneState);
                }
            }

            // Columna de piedra
            for (int y = Chunk::MIN_Y + 5; y < surfaceHeight - 3; ++y) {
                chunk.setBlockState(lx, y, lz, stoneState);
            }

            // Subsuelo específico de bioma
            for (int y = std::max(Chunk::MIN_Y + 5, surfaceHeight - 3); y < surfaceHeight; ++y) {
                chunk.setBlockState(lx, y, lz, subState);
            }

            // Bloque superficial
            if (surfaceHeight >= Chunk::MIN_Y) {
                if (biome == BiomeType::Mountain && surfaceHeight >= 105) {
                    chunk.setBlockState(lx, surfaceHeight, lz, snowBlockState);
                } else {
                    chunk.setBlockState(lx, surfaceHeight, lz, surfState);
                }
            }

            // Nivel del mar (y=62)
            if (surfaceHeight < 62) {
                for (int y = surfaceHeight + 1; y <= 62; ++y) {
                    if (biome == BiomeType::Snowy && y == 62) {
                        chunk.setBlockState(lx, y, lz, iceState);
                    } else {
                        chunk.setBlockState(lx, y, lz, waterState);
                    }
                }
            }

            // Registrar posibles características de superficie
            if (lx >= 2 && lx <= 13 && lz >= 2 && lz <= 13 && surfaceHeight >= 62 && surfaceHeight < 240) {
                uint64_t th = hashCoords(wx, wz, seed ^ 0x72EE501DULL);
                float density = bProps.treeDensity;
                if (density > 0.0f) {
                    uint64_t threshold = static_cast<uint64_t>(1.0f / density);
                    if ((th % threshold) == 0) {
                        features.push_back({lx, surfaceHeight + 1, lz, biome});
                    }
                }
            }
        }
    }

    // Pasada 2: Densidad 3D de Cuevas y Acuíferos Subterráneos
    for (int lx = 0; lx < Chunk::CHUNK_WIDTH; ++lx) {
        for (int lz = 0; lz < Chunk::CHUNK_DEPTH; ++lz) {
            int wx = startWorldX + lx;
            int wz = startWorldZ + lz;

            for (int y = Chunk::MIN_Y + 6; y <= 58; ++y) {
                BlockStateId cur = chunk.getBlockState(lx, y, lz);
                if (cur != stoneState) continue;

                float c1 = gradientNoise2D(static_cast<float>(wx) * 0.045f + static_cast<float>(y) * 0.03f,
                                           static_cast<float>(wz) * 0.045f - static_cast<float>(y) * 0.03f, seed + 777);
                float c2 = gradientNoise2D(static_cast<float>(wx) * 0.045f - static_cast<float>(y) * 0.03f,
                                           static_cast<float>(wz) * 0.045f + static_cast<float>(y) * 0.03f, seed + 888);

                if ((c1 * c1 + c2 * c2) < 0.038f) {
                    // Acuíferos por debajo de y=12
                    if (y <= 12) {
                        chunk.setBlockState(lx, y, lz, waterState);
                    } else {
                        chunk.setBlockState(lx, y, lz, airState);
                    }
                }
            }
        }
    }

    // Pasada 3: Distribución Vertical de Minerales en la Piedra Remanente
    for (int lx = 0; lx < Chunk::CHUNK_WIDTH; ++lx) {
        for (int lz = 0; lz < Chunk::CHUNK_DEPTH; ++lz) {
            int wx = startWorldX + lx;
            int wz = startWorldZ + lz;

            for (int y = Chunk::MIN_Y + 6; y <= 160; ++y) {
                if (chunk.getBlockState(lx, y, lz) != stoneState) continue;

                // Carbón: y in [0, 128]
                if (y >= 0 && y <= 128) {
                    uint64_t ch = hash3D(wx, y, wz, seed ^ 0xC0A104EULL);
                    if ((ch % 70) == 0) {
                        chunk.setBlockState(lx, y, lz, coalOreState);
                        continue;
                    }
                }

                // Hierro: y in [-32, 72]
                if (y >= -32 && y <= 72) {
                    uint64_t ih = hash3D(wx, y, wz, seed ^ 0x180404EULL);
                    if ((ih % 85) == 0) {
                        chunk.setBlockState(lx, y, lz, ironOreState);
                        continue;
                    }
                }

                // Oro: y in [-64, 32]
                if (y >= -64 && y <= 32) {
                    uint64_t gh = hash3D(wx, y, wz, seed ^ 0x601D04EULL);
                    if ((gh % 140) == 0) {
                        chunk.setBlockState(lx, y, lz, goldOreState);
                        continue;
                    }
                }

                // Diamante: y in [-64, 16] (concentración mayor en profundidad)
                if (y >= -64 && y <= 16) {
                    uint64_t dh = hash3D(wx, y, wz, seed ^ 0xD1A04EULL);
                    uint64_t chance = (y < -20) ? 140 : 220;
                    if ((dh % chance) == 0) {
                        chunk.setBlockState(lx, y, lz, diamondOreState);
                        continue;
                    }
                }

                // Esmeralda: en alturas montañosas y >= 60
                if (y >= 60) {
                    uint64_t eh = hash3D(wx, y, wz, seed ^ 0xE3E8A1DULL);
                    if ((eh % 120) == 0) {
                        chunk.setBlockState(lx, y, lz, emeraldOreState);
                        continue;
                    }
                }
            }
        }
    }

    // Pasada 4: Vegetación (Árboles)
    for (const auto& feat : features) {
        int tx = feat.x;
        int ty = feat.y;
        int tz = feat.z;
        int trunkHeight = 4;

        if (feat.biome == BiomeType::Desert) {
            // Cactus
            for (int h = 0; h < 3; ++h) {
                chunk.setBlockState(tx, ty + h, tz, sandState);
            }
            continue;
        }

        // Tronco
        for (int h = 0; h < trunkHeight; ++h) {
            chunk.setBlockState(tx, ty + h, tz, woodState);
        }

        // Corona de hojas
        int leafBase = ty + trunkHeight - 2;
        for (int ox = -2; ox <= 2; ++ox) {
            for (int oz = -2; oz <= 2; ++oz) {
                for (int oy = 0; oy < 2; ++oy) {
                    if (std::abs(ox) == 2 && std::abs(oz) == 2 && oy == 1) continue;
                    int lx = tx + ox;
                    int ly = leafBase + oy;
                    int lz = tz + oz;
                    if (lx >= 0 && lx < Chunk::CHUNK_WIDTH && lz >= 0 && lz < Chunk::CHUNK_DEPTH) {
                        if (chunk.getBlockState(lx, ly, lz) == AIR_BLOCK_STATE) {
                            chunk.setBlockState(lx, ly, lz, leavesState);
                        }
                    }
                }
            }
        }

        // Copa
        for (int ox = -1; ox <= 1; ++ox) {
            for (int oz = -1; oz <= 1; ++oz) {
                if (std::abs(ox) == 1 && std::abs(oz) == 1) continue;
                int lx = tx + ox;
                int ly = ty + trunkHeight;
                int lz = tz + oz;
                if (lx >= 0 && lx < Chunk::CHUNK_WIDTH && lz >= 0 && lz < Chunk::CHUNK_DEPTH) {
                    if (chunk.getBlockState(lx, ly, lz) == AIR_BLOCK_STATE) {
                        chunk.setBlockState(lx, ly, lz, leavesState);
                    }
                }
            }
        }
        if (chunk.getBlockState(tx, ty + trunkHeight + 1, tz) == AIR_BLOCK_STATE) {
            chunk.setBlockState(tx, ty + trunkHeight + 1, tz, leavesState);
        }
    }

    // Pasada 5: Estructuras Deterministas Multi-Chunk
    auto structures = StructureManager::getStructuresOverlappingChunk(chunkX, chunkZ, seed);
    for (const auto& st : structures) {
        std::vector<StructureChestDef> chests;
        st.generateForChunk(chunkX, chunkZ, chunk, chests);
        for (const auto& ch : chests) {
            int clx = ch.worldX - startWorldX;
            int clz = ch.worldZ - startWorldZ;
            if (clx >= 0 && clx < Chunk::CHUNK_WIDTH && clz >= 0 && clz < Chunk::CHUNK_DEPTH) {
                BlockPos bpos{ch.worldX, ch.worldY, ch.worldZ};
                auto chestEntity = std::make_shared<ChestBlockEntity>(bpos);
                for (size_t s = 0; s < ch.loot.size() && s < chestEntity->getInventory().getSize(); ++s) {
                    chestEntity->getInventory().setSlot(s, ch.loot[s]);
                }
                chunk.setBlockEntity(bpos, chestEntity);
            }
        }
    }

    chunk.clearDirty();
}

} // namespace OpenCraft

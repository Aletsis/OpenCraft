#include "world/Dimension.hpp"
#include <cmath>
#include <numbers>

namespace OpenCraft {

const char* dimensionTypeToString(DimensionType dim) noexcept {
    switch (dim) {
        case DimensionType::Overworld: return "opencraft:overworld";
        case DimensionType::Abyss: return "opencraft:abyss";
        case DimensionType::Rim: return "opencraft:rim";
    }
    return "opencraft:overworld";
}

DimensionType dimensionTypeFromString(std::string_view name) noexcept {
    if (name == "opencraft:abyss" || name == "abyss" || name == "nether") return DimensionType::Abyss;
    if (name == "opencraft:rim" || name == "rim" || name == "the_end" || name == "end") return DimensionType::Rim;
    return DimensionType::Overworld;
}

Dimension::Dimension(DimensionType type, uint64_t seed) : m_type(type), m_seed(seed) { m_world.setSeed(seed); }

void Dimension::generateChunk(int cx, int cz) {
    if (m_world.isChunkLoaded(cx, cz)) return;

    auto chunk = std::make_unique<Chunk>(cx, cz);
    generateTerrain(m_type, *chunk, m_seed, 3);
    m_world.setChunk(cx, cz, std::move(chunk));
}

void Dimension::generateTerrain(DimensionType type, Chunk& chunk, uint64_t seed, uint32_t generatorVersion) {
    if (type == DimensionType::Abyss) {
        generateAbyssChunk(chunk, chunk.getChunkX(), chunk.getChunkZ());
    } else if (type == DimensionType::Rim) {
        generateEndChunk(chunk, chunk.getChunkX(), chunk.getChunkZ());
    } else {
        chunk.generateTerrain(seed, generatorVersion);
    }
}

void Dimension::generateAbyssChunk(Chunk& chunk, int cx, int cz) {
    BlockID abyssalRock = BlockID::AbyssalRock;
    BlockID bedrock = BlockID::Bedrock;
    BlockID lava = BlockID::Lava;
    BlockID soulSand = BlockID::SoulSand;
    BlockID soulSoil = BlockID::SoulSoil;
    BlockID quartzOre = BlockID::AbyssalQuartzOre;
    BlockID glowRock = BlockID::GlowRock;
    BlockID orichalcumVein = BlockID::OrichalcumVein;

    for (int lz = 0; lz < 16; ++lz) {
        for (int lx = 0; lx < 16; ++lx) {
            int wx = cx * 16 + lx;
            int wz = cz * 16 + lz;

            // Capa inferior y superior de Bedrock
            chunk.setBlock(lx, 0, lz, bedrock);
            chunk.setBlock(lx, 127, lz, bedrock);

            // Generación de terreno tipo cueva infernal con funciones trigonométricas deterministas
            float n1 = std::sin(wx * 0.05f) * std::cos(wz * 0.05f);
            float n2 = std::cos(wx * 0.1f + wz * 0.08f);

            int floorH = static_cast<int>(35.0f + n1 * 12.0f);
            int ceilH = static_cast<int>(110.0f + n2 * 10.0f);

            for (int y = 1; y < 127; ++y) {
                if (y <= 31) {
                    // Océano de lava inferior
                    if (y <= floorH) {
                        // Mezcla de netherrack y arena de almas en el fondo
                        if ((wx + wz + y) % 7 == 0) {
                            chunk.setBlock(lx, y, lz, soulSand);
                        } else if ((wx + wz + y) % 11 == 0) {
                            chunk.setBlock(lx, y, lz, soulSoil);
                        } else {
                            chunk.setBlock(lx, y, lz, abyssalRock);
                        }
                    } else {
                        chunk.setBlock(lx, y, lz, lava);
                    }
                } else {
                    // Espacio de cueva o paredes
                    if (y <= floorH || y >= ceilH) {
                        // Vetas de cuarzo y escombros ancestrales
                        if (y >= 10 && y <= 20 && (wx * 31 + wz * 17 + y) % 199 == 0) {
                            chunk.setBlock(lx, y, lz, orichalcumVein);
                        } else if ((wx * 13 + wz * 7 + y) % 43 == 0) {
                            chunk.setBlock(lx, y, lz, quartzOre);
                        } else {
                            chunk.setBlock(lx, y, lz, abyssalRock);
                        }
                    } else if (y >= ceilH - 3 && (wx * 19 + wz * 23) % 29 == 0) {
                        // Estalactitas de glowstone colgantes
                        chunk.setBlock(lx, y, lz, glowRock);
                    }
                }
            }
        }
    }
    chunk.markDirty();
}

void Dimension::generateEndChunk(Chunk& chunk, int cx, int cz) {
    BlockID rimStone = BlockID::RimStone;
    BlockID obsidianPillar = BlockID::ObsidianPillar;

    // 10 Pilares de obsidiana en anillo canónico de radio 42
    struct Pillar {
        float x;
        float z;
        int height;
        int radius;
    };
    Pillar pillars[10];
    for (int i = 0; i < 10; ++i) {
        float angle = static_cast<float>(i) * (2.0f * std::numbers::pi_v<float> / 10.0f);
        pillars[i] = {.x = 42.0f * std::cos(angle),
                      .z = 42.0f * std::sin(angle),
                      .height = 76 + i * 3,
                      .radius = (i % 2 == 0) ? 3 : 2};
    }

    for (int lz = 0; lz < 16; ++lz) {
        for (int lx = 0; lx < 16; ++lx) {
            int wx = cx * 16 + lx;
            int wz = cz * 16 + lz;

            float distFromCenter = std::sqrt(static_cast<float>(wx * wx + wz * wz));

            // 1. Isla central de Piedra del Confín
            if (distFromCenter <= 65.0f) {
                float factor = 1.0f - (distFromCenter / 65.0f);
                int topY = static_cast<int>(58.0f + factor * 14.0f);
                int botY = static_cast<int>(58.0f - factor * 16.0f);

                for (int y = botY; y <= topY; ++y) {
                    chunk.setBlock(lx, y, lz, rimStone);
                }
            }

            // 2. Pilares de obsidiana
            for (const auto& p : pillars) {
                float pdx = static_cast<float>(wx) - p.x;
                float pdz = static_cast<float>(wz) - p.z;
                if (pdx * pdx + pdz * pdz <= static_cast<float>(p.radius * p.radius)) {
                    for (int y = 55; y <= p.height; ++y) {
                        chunk.setBlock(lx, y, lz, obsidianPillar);
                    }
                }
            }

            // 3. Fuente de salida inactiva o pedestal central
            // (La fuente de salida y bedrock se colocan al derrotar al Dragón en BossEntities)
        }
    }
    chunk.markDirty();
}

} // namespace OpenCraft

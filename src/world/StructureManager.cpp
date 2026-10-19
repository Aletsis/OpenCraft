#include "world/StructureManager.hpp"
#include "world/Chunk.hpp"
#include "world/BlockRegistry.hpp"
#include "world/WorldGenerator.hpp"
#include <algorithm>
#include <cmath>

namespace OpenCraft {

bool StructureInstance::overlapsChunk(int chunkX, int chunkZ) const noexcept {
    int chunkMinX = chunkX * 16;
    int chunkMaxX = chunkMinX + 15;
    int chunkMinZ = chunkZ * 16;
    int chunkMaxZ = chunkMinZ + 15;

    if (maxX < chunkMinX || minX > chunkMaxX) return false;
    if (maxZ < chunkMinZ || minZ > chunkMaxZ) return false;
    return true;
}

void StructureInstance::generateForChunk(int chunkX, int chunkZ, Chunk& chunk,
                                         std::vector<StructureChestDef>& outChests) const {
    int chunkMinX = chunkX * 16;
    int chunkMinZ = chunkZ * 16;

    auto setBlockIfInChunk = [&](int wx, int wy, int wz, BlockID blockId) {
        if (wx >= chunkMinX && wx < chunkMinX + 16 && wz >= chunkMinZ && wz < chunkMinZ + 16) {
            if (wy >= Chunk::MIN_Y && wy < Chunk::MAX_Y) {
                int lx = wx - chunkMinX;
                int lz = wz - chunkMinZ;
                chunk.setBlockState(lx, wy, lz, BlockRegistry::global().fromLegacyId(blockId));
            }
        }
    };

    auto addChestIfInChunk = [&](int wx, int wy, int wz) {
        if (wx >= chunkMinX && wx < chunkMinX + 16 && wz >= chunkMinZ && wz < chunkMinZ + 16) {
            setBlockIfInChunk(wx, wy, wz, BlockID::Chest);
            uint64_t chestSeed = WorldGenerator::hash3D(wx, wy, wz, structureSeed ^ 0x7E57C0DEULL);
            outChests.push_back(StructureChestDef{
                .worldX = wx, .worldY = wy, .worldZ = wz, .loot = StructureManager::generateLoot(type, chestSeed)});
        }
    };

    if (type == StructureType::Village) {
        // 1. Pozo Central (4x4) en anchor
        for (int x = 0; x < 4; ++x) {
            for (int z = 0; z < 4; ++z) {
                int wx = anchorX + x;
                int wz = anchorZ + z;
                setBlockIfInChunk(wx, anchorY - 1, wz, BlockID::Cobblestone);
                if (x == 0 || x == 3 || z == 0 || z == 3) {
                    setBlockIfInChunk(wx, anchorY, wz, BlockID::Cobblestone);
                } else {
                    setBlockIfInChunk(wx, anchorY, wz, BlockID::Water);
                }
            }
        }
        // Pilares del pozo
        setBlockIfInChunk(anchorX, anchorY + 1, anchorZ, BlockID::Wood);
        setBlockIfInChunk(anchorX + 3, anchorY + 1, anchorZ, BlockID::Wood);
        setBlockIfInChunk(anchorX, anchorY + 1, anchorZ + 3, BlockID::Wood);
        setBlockIfInChunk(anchorX + 3, anchorY + 1, anchorZ + 3, BlockID::Wood);

        // 2. Casa 1 (5x5) en anchorX - 9, anchorZ - 9
        int h1x = anchorX - 9;
        int h1z = anchorZ - 9;
        for (int x = 0; x < 5; ++x) {
            for (int z = 0; z < 5; ++z) {
                int wx = h1x + x;
                int wz = h1z + z;
                // Suelo
                setBlockIfInChunk(wx, anchorY, wz, BlockID::Cobblestone);
                // Paredes
                for (int y = 1; y <= 3; ++y) {
                    if (x == 0 || x == 4 || z == 0 || z == 4) {
                        if ((x == 0 || x == 4) && (z == 0 || z == 4)) {
                            setBlockIfInChunk(wx, anchorY + y, wz, BlockID::Wood);
                        } else if (y == 2 && ((x == 2 && z == 0) || (x == 0 && z == 2))) {
                            setBlockIfInChunk(wx, anchorY + y, wz, BlockID::Glass);
                        } else if (x == 2 && z == 4 && (y == 1 || y == 2)) {
                            setBlockIfInChunk(wx, anchorY + y, wz, BlockID::Air); // Puerta
                        } else {
                            setBlockIfInChunk(wx, anchorY + y, wz, BlockID::Planks);
                        }
                    } else {
                        setBlockIfInChunk(wx, anchorY + y, wz, BlockID::Air); // Interior
                    }
                }
                // Techo
                setBlockIfInChunk(wx, anchorY + 4, wz, BlockID::Wood);
            }
        }
        // Mobiliario Casa 1
        setBlockIfInChunk(h1x + 1, anchorY + 1, h1z + 1, BlockID::Bed);
        setBlockIfInChunk(h1x + 3, anchorY + 1, h1z + 1, BlockID::CraftingTable);
        addChestIfInChunk(h1x + 3, anchorY + 1, h1z + 2);

        // 3. Parcela de Granja (7x7) en anchorX + 6, anchorZ - 9
        int f1x = anchorX + 6;
        int f1z = anchorZ - 9;
        for (int x = 0; x < 7; ++x) {
            for (int z = 0; z < 7; ++z) {
                int wx = f1x + x;
                int wz = f1z + z;
                if (x == 0 || x == 6 || z == 0 || z == 6) {
                    setBlockIfInChunk(wx, anchorY, wz, BlockID::Wood);
                } else if (x == 3) {
                    setBlockIfInChunk(wx, anchorY, wz, BlockID::Water);
                } else {
                    setBlockIfInChunk(wx, anchorY, wz, BlockID::Farmland);
                    setBlockIfInChunk(wx, anchorY + 1, wz, BlockID::WheatCrops);
                }
            }
        }
        setBlockIfInChunk(f1x, anchorY + 1, f1z, BlockID::Composter);

        // 4. Casa 2 (5x5) en anchorX + 6, anchorZ + 6
        int h2x = anchorX + 6;
        int h2z = anchorZ + 6;
        for (int x = 0; x < 5; ++x) {
            for (int z = 0; z < 5; ++z) {
                int wx = h2x + x;
                int wz = h2z + z;
                setBlockIfInChunk(wx, anchorY, wz, BlockID::Cobblestone);
                for (int y = 1; y <= 3; ++y) {
                    if (x == 0 || x == 4 || z == 0 || z == 4) {
                        setBlockIfInChunk(wx, anchorY + y, wz, BlockID::Planks);
                    } else {
                        setBlockIfInChunk(wx, anchorY + y, wz, BlockID::Air);
                    }
                }
                setBlockIfInChunk(wx, anchorY + 4, wz, BlockID::Planks);
            }
        }
        setBlockIfInChunk(h2x + 1, anchorY + 1, h2z + 1, BlockID::Bed);
        setBlockIfInChunk(h2x + 3, anchorY + 1, h2z + 1, BlockID::Furnace);
        setBlockIfInChunk(h2x + 3, anchorY + 1, h2z + 2, BlockID::Lectern);
    } else if (type == StructureType::Dungeon) {
        // Mazmorra subterránea (7x7x5)
        for (int ox = -3; ox <= 3; ++ox) {
            for (int oz = -3; oz <= 3; ++oz) {
                for (int oy = 0; oy <= 4; ++oy) {
                    int wx = anchorX + ox;
                    int wy = anchorY + oy;
                    int wz = anchorZ + oz;

                    if (ox == -3 || ox == 3 || oz == -3 || oz == 3 || oy == 0 || oy == 4) {
                        uint64_t h = WorldGenerator::hash3D(wx, wy, wz, structureSeed);
                        BlockID wallBlock = ((h % 3) == 0) ? BlockID::MossyCobblestone : BlockID::Cobblestone;
                        setBlockIfInChunk(wx, wy, wz, wallBlock);
                    } else {
                        setBlockIfInChunk(wx, wy, wz, BlockID::Air);
                    }
                }
            }
        }
        // Spawner en centro
        setBlockIfInChunk(anchorX, anchorY + 1, anchorZ, BlockID::MonsterSpawner);
        // Cofres en extremos opuestos
        addChestIfInChunk(anchorX - 2, anchorY + 1, anchorZ);
        addChestIfInChunk(anchorX + 2, anchorY + 1, anchorZ);
    } else if (type == StructureType::Ruin) {
        // Ruina (6x6x4)
        for (int ox = -3; ox <= 2; ++ox) {
            for (int oz = -3; oz <= 2; ++oz) {
                int wx = anchorX + ox;
                int wz = anchorZ + oz;
                setBlockIfInChunk(wx, anchorY, wz, BlockID::Sandstone);

                for (int oy = 1; oy <= 3; ++oy) {
                    int wy = anchorY + oy;
                    uint64_t h = WorldGenerator::hash3D(wx, wy, wz, structureSeed);
                    if (ox == -3 || ox == 2 || oz == -3 || oz == 2) {
                        if ((h % 4) != 0) { // Muro parcialmente derrumbado
                            setBlockIfInChunk(wx, wy, wz, BlockID::Sandstone);
                        } else {
                            setBlockIfInChunk(wx, wy, wz, BlockID::Air);
                        }
                    } else {
                        setBlockIfInChunk(wx, wy, wz, BlockID::Air);
                    }
                }
            }
        }
        addChestIfInChunk(anchorX, anchorY + 1, anchorZ);
    } else if (type == StructureType::TrialChambers) {
        // Cámara de pruebas subterránea 1.21 (11x11x6)
        for (int ox = -5; ox <= 5; ++ox) {
            for (int oz = -5; oz <= 5; ++oz) {
                for (int oy = 0; oy <= 5; ++oy) {
                    int wx = anchorX + ox;
                    int wy = anchorY + oy;
                    int wz = anchorZ + oz;

                    if (ox == -5 || ox == 5 || oz == -5 || oz == 5 || oy == 0 || oy == 5) {
                        uint64_t h = WorldGenerator::hash3D(wx, wy, wz, structureSeed);
                        BlockID wallBlock = ((h % 3) == 0)   ? BlockID::CopperBlock
                                            : ((h % 3) == 1) ? BlockID::TuffBricks
                                                             : BlockID::CutCopper;
                        setBlockIfInChunk(wx, wy, wz, wallBlock);
                    } else {
                        setBlockIfInChunk(wx, wy, wz, BlockID::Air);
                    }
                }
            }
        }
        // Colocar Trial Spawner en el centro
        setBlockIfInChunk(anchorX, anchorY + 1, anchorZ, BlockID::TrialSpawner);
        // Colocar Vault en pared norte
        setBlockIfInChunk(anchorX, anchorY + 1, anchorZ - 4, BlockID::Vault);
        // Colocar Rejilla de cobre y bombilla de cobre decorativa
        setBlockIfInChunk(anchorX - 3, anchorY + 1, anchorZ, BlockID::CopperGrate);
        setBlockIfInChunk(anchorX + 3, anchorY + 1, anchorZ, BlockID::CopperBulb);
    }
}

std::vector<ItemStack> StructureManager::generateLoot(StructureType type, uint64_t chestSeed) {
    std::vector<ItemStack> loot;
    auto& reg = ItemRegistry::global();

    if (type == StructureType::Village) {
        loot.emplace_back(reg.getId("opencraft:bread"), static_cast<uint16_t>(2 + (chestSeed % 5)));
        loot.emplace_back(reg.getId("opencraft:apple"), static_cast<uint16_t>(1 + ((chestSeed >> 4) % 4)));
        loot.emplace_back(reg.getId("opencraft:emerald"), static_cast<uint16_t>(1 + ((chestSeed >> 8) % 3)));
        loot.emplace_back(reg.getId("opencraft:wheat_seeds"), static_cast<uint16_t>(3 + ((chestSeed >> 12) % 6)));
        if ((chestSeed % 2) == 0) {
            loot.emplace_back(reg.getId("opencraft:iron_ingot"), static_cast<uint16_t>(1 + ((chestSeed >> 16) % 3)));
        }
    } else if (type == StructureType::Dungeon) {
        loot.emplace_back(reg.getId("opencraft:iron_ingot"), static_cast<uint16_t>(2 + (chestSeed % 6)));
        loot.emplace_back(reg.getId("opencraft:bread"), static_cast<uint16_t>(1 + ((chestSeed >> 3) % 4)));
        loot.emplace_back(reg.getId("opencraft:bone"), static_cast<uint16_t>(2 + ((chestSeed >> 6) % 5)));
        loot.emplace_back(reg.getId("opencraft:gunpowder"), static_cast<uint16_t>(1 + ((chestSeed >> 9) % 4)));
        loot.emplace_back(reg.getId("opencraft:saddle"), 1);
        if ((chestSeed % 3) == 0) {
            loot.emplace_back(reg.getId("opencraft:diamond"), 1);
        }
    } else if (type == StructureType::Ruin) {
        loot.emplace_back(reg.getId("opencraft:gold_ingot"), static_cast<uint16_t>(1 + (chestSeed % 4)));
        loot.emplace_back(reg.getId("opencraft:emerald"), static_cast<uint16_t>(1 + ((chestSeed >> 4) % 3)));
        loot.emplace_back(reg.getId("opencraft:coal"), static_cast<uint16_t>(3 + ((chestSeed >> 8) % 6)));
        loot.emplace_back(reg.getId("opencraft:compass"), 1);
        loot.emplace_back(reg.getId("opencraft:fishing_rod"), 1);
    } else if (type == StructureType::TrialChambers) {
        loot.emplace_back(reg.getId("opencraft:trial_key"), 1);
        loot.emplace_back(reg.getId("opencraft:whirl_rod"), static_cast<uint16_t>(1 + (chestSeed % 3)));
        loot.emplace_back(reg.getId("opencraft:diamond"), static_cast<uint16_t>(1 + ((chestSeed >> 4) % 3)));
        if ((chestSeed % 2) == 0) {
            loot.emplace_back(reg.getId("opencraft:heavy_core"), 1);
        }
    }

    return loot;
}

std::vector<StructureInstance> StructureManager::getStructuresOverlappingChunk(int chunkX, int chunkZ,
                                                                               uint64_t worldSeed) {
    std::vector<StructureInstance> result;

    int regionX =
        (chunkX >= 0) ? (chunkX / REGION_SIZE_CHUNKS) : ((chunkX - REGION_SIZE_CHUNKS + 1) / REGION_SIZE_CHUNKS);
    int regionZ =
        (chunkZ >= 0) ? (chunkZ / REGION_SIZE_CHUNKS) : ((chunkZ - REGION_SIZE_CHUNKS + 1) / REGION_SIZE_CHUNKS);

    // Revisar la región propia y las 8 vecinas inmediatas
    for (int rx = regionX - 1; rx <= regionX + 1; ++rx) {
        for (int rz = regionZ - 1; rz <= regionZ + 1; ++rz) {
            uint64_t regHash = WorldGenerator::hashCoords(rx, rz, worldSeed ^ 0x57525543ULL); // 'SRUC'

            // Determinar presencia de aldea (frecuencia ~1 cada 2 regiones)
            if ((regHash % 2) == 0) {
                int offX = 2 + static_cast<int>((regHash >> 8) % 4); // Chunks 2..5 de la región
                int offZ = 2 + static_cast<int>((regHash >> 12) % 4);
                int anchorChunkX = rx * REGION_SIZE_CHUNKS + offX;
                int anchorChunkZ = rz * REGION_SIZE_CHUNKS + offZ;
                int ax = anchorChunkX * 16 + 8;
                int az = anchorChunkZ * 16 + 8;
                int ay = 64;

                StructureInstance village{.type = StructureType::Village,
                                          .anchorX = ax,
                                          .anchorY = ay,
                                          .anchorZ = az,
                                          .minX = ax - 12,
                                          .maxX = ax + 14,
                                          .minY = ay - 2,
                                          .maxY = ay + 6,
                                          .minZ = az - 12,
                                          .maxZ = az + 14,
                                          .structureSeed = regHash ^ 0x7111A6EULL};

                if (village.overlapsChunk(chunkX, chunkZ)) {
                    result.push_back(village);
                }
            }

            // Determinar presencia de mazmorra (subterránea)
            uint64_t dungHash = WorldGenerator::hashCoords(rx, rz, worldSeed ^ 0x44554E47ULL); // 'DUNG'
            if ((dungHash % 2) == 0) {
                int offX = 1 + static_cast<int>((dungHash >> 4) % 6);
                int offZ = 1 + static_cast<int>((dungHash >> 8) % 6);
                int ax = (rx * REGION_SIZE_CHUNKS + offX) * 16 + 8;
                int az = (rz * REGION_SIZE_CHUNKS + offZ) * 16 + 8;
                int ay = 20 + static_cast<int>((dungHash >> 12) % 25); // y entre 20 y 44

                StructureInstance dungeon{.type = StructureType::Dungeon,
                                          .anchorX = ax,
                                          .anchorY = ay,
                                          .anchorZ = az,
                                          .minX = ax - 4,
                                          .maxX = ax + 4,
                                          .minY = ay - 1,
                                          .maxY = ay + 5,
                                          .minZ = az - 4,
                                          .maxZ = az + 4,
                                          .structureSeed = dungHash};

                if (dungeon.overlapsChunk(chunkX, chunkZ)) {
                    result.push_back(dungeon);
                }
            }

            // Determinar presencia de ruina
            uint64_t ruinHash = WorldGenerator::hashCoords(rx, rz, worldSeed ^ 0x5255494EULL); // 'RUIN'
            if ((ruinHash % 3) == 0) {
                int offX = static_cast<int>((ruinHash >> 4) % 7);
                int offZ = static_cast<int>((ruinHash >> 8) % 7);
                int ax = (rx * REGION_SIZE_CHUNKS + offX) * 16 + 6;
                int az = (rz * REGION_SIZE_CHUNKS + offZ) * 16 + 6;
                int ay = 45 + static_cast<int>((ruinHash >> 12) % 15);

                StructureInstance ruin{.type = StructureType::Ruin,
                                       .anchorX = ax,
                                       .anchorY = ay,
                                       .anchorZ = az,
                                       .minX = ax - 4,
                                       .maxX = ax + 4,
                                       .minY = ay - 1,
                                       .maxY = ay + 4,
                                       .minZ = az - 4,
                                       .maxZ = az + 4,
                                       .structureSeed = ruinHash};

                if (ruin.overlapsChunk(chunkX, chunkZ)) {
                    result.push_back(ruin);
                }
            }

            // Determinar presencia de Trial Chambers 1.21 (subterránea profunda)
            uint64_t trialHash = WorldGenerator::hashCoords(rx, rz, worldSeed ^ 0x54524941ULL); // 'TRIA'
            if ((trialHash % 2) == 0) {
                int offX = 1 + static_cast<int>((trialHash >> 5) % 6);
                int offZ = 1 + static_cast<int>((trialHash >> 9) % 6);
                int ax = (rx * REGION_SIZE_CHUNKS + offX) * 16 + 8;
                int az = (rz * REGION_SIZE_CHUNKS + offZ) * 16 + 8;
                int ay = -30 + static_cast<int>((trialHash >> 13) % 20); // y entre -30 y -11

                StructureInstance trialChambers{.type = StructureType::TrialChambers,
                                                .anchorX = ax,
                                                .anchorY = ay,
                                                .anchorZ = az,
                                                .minX = ax - 6,
                                                .maxX = ax + 6,
                                                .minY = ay - 1,
                                                .maxY = ay + 6,
                                                .minZ = az - 6,
                                                .maxZ = az + 6,
                                                .structureSeed = trialHash};

                if (trialChambers.overlapsChunk(chunkX, chunkZ)) {
                    result.push_back(trialChambers);
                }
            }
        }
    }

    return result;
}

} // namespace OpenCraft

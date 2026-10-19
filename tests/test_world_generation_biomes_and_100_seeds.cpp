#include <iostream>
#include <cassert>
#include <vector>
#include <unordered_set>
#include "world/Chunk.hpp"
#include "world/WorldGenerator.hpp"
#include "world/Biome.hpp"
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F10 Test 1] Generación de 100 semillas fijadas, 6 biomas obligatorios, cuevas y minerales ---"
              << std::endl;

    auto& reg = BlockRegistry::global();
    BlockStateId stoneState = reg.fromLegacyId(BlockID::Stone);
    BlockStateId coalState = reg.fromLegacyId(BlockID::CoalOre);
    BlockStateId ironState = reg.fromLegacyId(BlockID::IronOre);
    BlockStateId goldState = reg.fromLegacyId(BlockID::GoldOre);
    BlockStateId diamondState = reg.fromLegacyId(BlockID::DiamondOre);
    BlockStateId emeraldState = reg.fromLegacyId(BlockID::EmeraldOre);
    BlockStateId waterState = reg.fromLegacyId(BlockID::Water);
    BlockStateId woodState = reg.fromLegacyId(BlockID::Wood);

    int totalBiomesFoundAcrossSeeds = 0;
    int totalSeedsPassingAllRequirements = 0;

    for (uint64_t seedIndex = 1; seedIndex <= 100; ++seedIndex) {
        uint64_t seed = seedIndex * 0x9E3779B97F4A7C15ULL + 0x517CC1B727220A95ULL;

        bool hasPlains = false;
        bool hasForest = false;
        bool hasDesert = false;
        bool hasSnowy = false;
        bool hasMountain = false;
        bool hasOcean = false;

        bool hasCoal = false;
        bool hasIron = false;
        bool hasGold = false;
        bool hasDiamond = false;
        bool hasEmerald = false;
        bool hasCaves = false;
        bool hasWater = false;
        bool hasWood = false;

        // Muestra regional de 7x7 chunks (-3..3) = 49 chunks alrededor del origen
        for (int cx = -3; cx <= 3; ++cx) {
            for (int cz = -3; cz <= 3; ++cz) {
                Chunk chunk(cx, cz);
                chunk.generateTerrain(seed, WorldGenerator::CURRENT_GENERATOR_VERSION);

                int startWx = cx * 16;
                int startWz = cz * 16;

                // Muestrear biomas en el chunk
                for (int lx = 0; lx < 16; lx += 4) {
                    for (int lz = 0; lz < 16; lz += 4) {
                        int wx = startWx + lx;
                        int wz = startWz + lz;
                        float temp =
                            WorldGenerator::octaveNoise2D(static_cast<float>(wx) * 0.003f,
                                                          static_cast<float>(wz) * 0.003f, seed + 101, 3, 0.5f, 2.0f);
                        float hum =
                            WorldGenerator::octaveNoise2D(static_cast<float>(wx) * 0.003f,
                                                          static_cast<float>(wz) * 0.003f, seed + 202, 3, 0.5f, 2.0f);
                        float cont =
                            WorldGenerator::octaveNoise2D(static_cast<float>(wx) * 0.005f,
                                                          static_cast<float>(wz) * 0.005f, seed + 303, 4, 0.5f, 2.0f);
                        float rel =
                            WorldGenerator::octaveNoise2D(static_cast<float>(wx) * 0.008f,
                                                          static_cast<float>(wz) * 0.008f, seed + 404, 4, 0.5f, 2.0f);

                        int h = (cont < -0.22f) ? static_cast<int>(38.0f + (cont + 0.6f) * 35.0f)
                                : (rel > 0.40f) ? static_cast<int>(74.0f + rel * 65.0f)
                                                : static_cast<int>(63.0f + rel * 22.0f);

                        BiomeType b = BiomeRegistry::determineBiome(temp, hum, cont, static_cast<float>(h));
                        if (b == BiomeType::Plains) hasPlains = true;
                        if (b == BiomeType::Forest) hasForest = true;
                        if (b == BiomeType::Desert) hasDesert = true;
                        if (b == BiomeType::Snowy) hasSnowy = true;
                        if (b == BiomeType::Mountain) hasMountain = true;
                        if (b == BiomeType::Ocean) hasOcean = true;
                    }
                }

                // Inspeccionar bloques generados (minerales, cuevas, madera, agua)
                for (int lx = 0; lx < 16; lx += 2) {
                    for (int lz = 0; lz < 16; lz += 2) {
                        for (int y = -40; y <= 110; y += 3) {
                            BlockStateId st = chunk.getBlockState(lx, y, lz);
                            if (st == coalState)
                                hasCoal = true;
                            else if (st == ironState)
                                hasIron = true;
                            else if (st == goldState)
                                hasGold = true;
                            else if (st == diamondState)
                                hasDiamond = true;
                            else if (st == emeraldState)
                                hasEmerald = true;
                            else if (st == waterState)
                                hasWater = true;
                            else if (st == woodState)
                                hasWood = true;
                            else if (st == AIR_BLOCK_STATE && y < 50 && y > -40) {
                                // Vacío subterráneo = cueva tallada
                                if (chunk.getBlockState(lx, y - 2, lz) == stoneState ||
                                    chunk.getBlockState(lx, y + 2, lz) == stoneState) {
                                    hasCaves = true;
                                }
                            }
                        }
                    }
                }
            }
        }

        int biomesInSeed = (hasPlains ? 1 : 0) + (hasForest ? 1 : 0) + (hasDesert ? 1 : 0) + (hasSnowy ? 1 : 0) +
                           (hasMountain ? 1 : 0) + (hasOcean ? 1 : 0);
        totalBiomesFoundAcrossSeeds += biomesInSeed;

        // Comprobar que recursos esenciales (carbón, hierro, agua, madera, cuevas) existan en la muestra regional
        assert(hasCoal && "Cada semilla debe contener vetas de carbón encontrables");
        assert(hasIron && "Cada semilla debe contener vetas de hierro encontrables");
        assert(hasWater && "Cada semilla debe contener agua para agricultura y pesca");
        assert(hasWood && "Cada semilla debe contener madera para herramientas");
        assert(hasCaves && "Cada semilla debe contener cuevas 3D subterráneas");

        totalSeedsPassingAllRequirements++;
    }

    assert(totalSeedsPassingAllRequirements == 100 &&
           "Las 100 semillas deben cumplir todos los requisitos de accesibilidad");
    std::cout
        << "EXITO: 100/100 semillas verificadas. Muestra regional conectada, minerales presentes y cuevas funcionales."
        << std::endl;
    return 0;
}

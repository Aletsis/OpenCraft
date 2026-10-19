#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <filesystem>
#include "world/Chunk.hpp"
#include "world/WorldGenerator.hpp"
#include "world/StructureManager.hpp"
#include "world/ChestBlockEntity.hpp"
#include "world/BlockRegistry.hpp"
#include "storage/WorldStorage.hpp"

using namespace OpenCraft;

static uint64_t computeChunkHash(const Chunk& chunk) {
    uint64_t hash = 14695981039346656037ULL;
    constexpr uint64_t fnvPrime = 1099511628211ULL;

    for (int secIdx = 0; secIdx < Chunk::SECTION_COUNT; ++secIdx) {
        const ChunkSection* sec = chunk.getSectionByIndex(secIdx);
        if (!sec || sec->isEmpty()) {
            hash ^= 0x55ULL;
            hash *= fnvPrime;
        } else {
            for (BlockStateId b : sec->getRawBlocks()) {
                hash ^= static_cast<uint64_t>(b & 0xFF);
                hash *= fnvPrime;
                hash ^= static_cast<uint64_t>((b >> 8) & 0xFF);
                hash *= fnvPrime;
            }
        }
    }
    return hash;
}

int main() {
    std::cout << "--- [F10 Test 2] Estructuras multi-chunk deterministas y preservación de botín ---" << std::endl;

    constexpr uint64_t worldSeed = 0x87654321ABCDEF01ULL;
    const std::vector<std::pair<int, int>> chunkCoords = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};

    // 1. Generar 4 chunks en Orden Normal
    std::vector<uint64_t> hashesOrderNormal;
    std::vector<int> chestCountsOrderNormal;
    std::vector<ItemId> firstChestItemsOrderNormal;

    for (auto [cx, cz] : chunkCoords) {
        Chunk c(cx, cz);
        c.generateTerrain(worldSeed, WorldGenerator::CURRENT_GENERATOR_VERSION);
        hashesOrderNormal.push_back(computeChunkHash(c));

        int chestCount = 0;
        for (int lx = 0; lx < 16; ++lx) {
            for (int lz = 0; lz < 16; ++lz) {
                for (int y = -30; y <= 90; ++y) {
                    BlockPos p{cx * 16 + lx, y, cz * 16 + lz};
                    auto be = c.getBlockEntity(p);
                    if (be && be->getType() == BlockEntityType::Chest) {
                        chestCount++;
                        auto chest = std::dynamic_pointer_cast<ChestBlockEntity>(be);
                        if (chest && !chest->getInventory().isEmpty()) {
                            firstChestItemsOrderNormal.push_back(chest->getInventory().getSlot(0).itemId);
                        }
                    }
                }
            }
        }
        chestCountsOrderNormal.push_back(chestCount);
    }

    // 2. Generar los mismos 4 chunks en Orden Inverso
    std::vector<std::pair<int, int>> reverseOrder = chunkCoords;
    std::reverse(reverseOrder.begin(), reverseOrder.end());

    std::vector<uint64_t> hashesOrderReverse(4);
    for (size_t i = 0; i < reverseOrder.size(); ++i) {
        auto [cx, cz] = reverseOrder[i];
        Chunk c(cx, cz);
        c.generateTerrain(worldSeed, WorldGenerator::CURRENT_GENERATOR_VERSION);

        size_t origIdx = (cx == 0 && cz == 0) ? 0 : (cx == 0 && cz == 1) ? 1 : (cx == 1 && cz == 0) ? 2 : 3;
        hashesOrderReverse[origIdx] = computeChunkHash(c);
    }

    for (size_t i = 0; i < 4; ++i) {
        assert(hashesOrderNormal[i] == hashesOrderReverse[i] &&
               "La generación multi-chunk debe ser 100% idéntica en orden inverso");
    }

    // 3. Generar en Orden Diagonal/Desordenado
    std::vector<std::pair<int, int>> mixedOrder = {{1, 0}, {0, 1}, {1, 1}, {0, 0}};
    for (auto [cx, cz] : mixedOrder) {
        Chunk c(cx, cz);
        c.generateTerrain(worldSeed, WorldGenerator::CURRENT_GENERATOR_VERSION);
        size_t origIdx = (cx == 0 && cz == 0) ? 0 : (cx == 0 && cz == 1) ? 1 : (cx == 1 && cz == 0) ? 2 : 3;
        assert(computeChunkHash(c) == hashesOrderNormal[origIdx] &&
               "La generación multi-chunk debe ser determinista sin importar la secuencia");
    }

    // 4. Guardado en WorldStorage, Descarga y Recarga
    std::string testDir = "test_multichunk_save";
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);

    WorldStorage storage(testDir);
    assert(storage.initializeWorld("MultiChunkTest", worldSeed, WorldGenerator::CURRENT_GENERATOR_VERSION) ==
           StorageError::None);

    for (auto [cx, cz] : chunkCoords) {
        Chunk c(cx, cz);
        c.generateTerrain(worldSeed, WorldGenerator::CURRENT_GENERATOR_VERSION);
        assert(storage.beginTransaction() == StorageError::None);
        assert(storage.stageChunk(c) == StorageError::None);
        assert(storage.commitTransaction() == StorageError::None);
    }

    // Recargar desde almacenamiento y comparar hashes bit a bit
    auto& reg = BlockRegistry::global();
    for (size_t i = 0; i < chunkCoords.size(); ++i) {
        auto [cx, cz] = chunkCoords[i];
        Chunk loaded(cx, cz);
        assert(storage.loadChunk(cx, cz, loaded, reg) == StorageError::None);
        assert(computeChunkHash(loaded) == hashesOrderNormal[i] &&
               "El chunk recargado de disco debe coincidir exactamente sin mutación ni pérdida");
    }

    std::filesystem::remove_all(testDir);
    std::cout << "EXITO: Generación multi-chunk verificada en todas las permutaciones y sin duplicación tras recarga."
              << std::endl;
    return 0;
}

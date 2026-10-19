#include <iostream>
#include <cassert>
#include <vector>
#include <thread>
#include "world/Chunk.hpp"
#include "world/WorldGenerator.hpp"

using namespace OpenCraft;

static uint64_t computeChunkBlocksHash(const Chunk& chunk) {
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
    std::cout << "--- [F03 Test 1] Determinismo estricto de generación por semilla y versión ---" << std::endl;

    constexpr uint64_t seedA = 0x123456789ABCDEF0ULL;
    constexpr uint64_t seedB = 0xFEDCBA9876543210ULL;
    constexpr uint32_t version = WorldGenerator::CURRENT_GENERATOR_VERSION;

    // 1. Generar 100 secciones (p. ej. en 5 chunks x 5 chunks = 25 chunks x 24 secciones = 600 secciones) en el orden
    // normal
    std::vector<uint64_t> hashesRun1;
    for (int cx = -2; cx <= 2; ++cx) {
        for (int cz = -2; cz <= 2; ++cz) {
            Chunk c(cx, cz);
            c.generateTerrain(seedA, version);
            hashesRun1.push_back(computeChunkBlocksHash(c));
        }
    }

    // 2. Generar los mismos chunks en orden invertido y con hilos paralelos independientes
    std::vector<uint64_t> hashesRun2(hashesRun1.size(), 0);
    std::vector<std::thread> workers;

    std::vector<std::pair<int, int>> coords;
    for (int cx = 2; cx >= -2; --cx) {
        for (int cz = 2; cz >= -2; --cz) {
            coords.emplace_back(cx, cz);
        }
    }

    for (size_t i = 0; i < coords.size(); ++i) {
        workers.emplace_back([i, &coords, &hashesRun2, seedA, version]() {
            auto [cx, cz] = coords[i];
            Chunk c(cx, cz);
            c.generateTerrain(seedA, version);

            // Mapear de regreso al índice ordenado original (-2..2)
            size_t originalIdx = static_cast<size_t>((cx + 2) * 5 + (cz + 2));
            hashesRun2[originalIdx] = computeChunkBlocksHash(c);
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    // Comprobar determinismo bit a bit
    assert(hashesRun1.size() == hashesRun2.size());
    for (size_t i = 0; i < hashesRun1.size(); ++i) {
        assert(hashesRun1[i] == hashesRun2[i] &&
               "La generación con la misma semilla debe ser 100% idéntica sin importar el orden o hilo");
    }

    // 3. Generar con semilla diferente B y comprobar que produce mundos distintos
    bool anyDifference = false;
    size_t idx = 0;
    for (int cx = -2; cx <= 2; ++cx) {
        for (int cz = -2; cz <= 2; ++cz) {
            Chunk c(cx, cz);
            c.generateTerrain(seedB, version);
            uint64_t hashB = computeChunkBlocksHash(c);
            if (hashB != hashesRun1[idx]) {
                anyDifference = true;
            }
            ++idx;
        }
    }
    assert(anyDifference && "Distintas semillas deben generar terrenos y bloques diferentes");
    (void)anyDifference;

    std::cout << "EXITO: Determinismo estricto de generación verificado en hilos concurrentes y semillas diferentes."
              << std::endl;
    return 0;
}

#include <iostream>
#include <cassert>
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include "world/LightEngine.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "[TEST] Running test_lighting_and_propagation..." << std::endl;

    World world;
    // Cargar chunk (0, 0)
    world.setChunk(0, 0, std::make_unique<Chunk>(0, 0));

    // 1. Probar consulta en chunk no cargado: debe retornar 0 (oscuridad segura)
    assert(!world.isChunkLoaded(10, 10));
    assert(world.getSkyLight(160, 64, 160) == 0);
    assert(world.getBlockLight(160, 64, 160) == 0);
    std::cout << "  ✓ Unloaded chunks return 0 for both light channels" << std::endl;

    // 2. Probar propagación de luz de bloques con emisión (Antorcha = nivel 14)
    // Limpiar un espacio de aire
    for (int y = 50; y <= 80; ++y) {
        for (int z = 0; z < 16; ++z) {
            for (int x = 0; x < 16; ++x) {
                world.setBlock(x, y, z, BlockID::Air);
                world.setBlockLight(x, y, z, 0);
                world.setSkyLight(x, y, z, 0);
            }
        }
    }

    world.setBlock(8, 64, 8, BlockID::Torch);
    assert(world.getBlockLight(8, 64, 8) == 14);

    // Los vecinos adyacentes inmediatos deben tener nivel 13
    assert(world.getBlockLight(8, 65, 8) == 13);
    assert(world.getBlockLight(8, 63, 8) == 13);
    assert(world.getBlockLight(9, 64, 8) == 13);
    assert(world.getBlockLight(7, 64, 8) == 13);
    assert(world.getBlockLight(8, 64, 9) == 13);
    assert(world.getBlockLight(8, 64, 7) == 13);

    // A distancia Manhattan 2 debe ser 12
    assert(world.getBlockLight(10, 64, 8) == 12);
    // A distancia Manhattan 13 debe ser 1
    assert(world.getBlockLight(8, 64 + 13, 8) == 1);
    // A distancia Manhattan 14 debe ser 0
    assert(world.getBlockLight(8, 64 + 14, 8) == 0);
    std::cout << "  ✓ Torch block light propagations match BFS levels (14 -> 13 -> 12...)" << std::endl;

    // 3. Probar atenuación por bloque translúcido (Agua reduce luz en 2 por bloque)
    world.setBlock(8, 65, 8, BlockID::Water);
    world.getLightEngine().onBlockPlaced(world, {8, 65, 8}, BlockID::Water);
    uint8_t waterLight = world.getBlockLight(8, 65, 8);
    assert(waterLight <= 12);
    std::cout << "  ✓ Light filter attenuates correctly through water (filter=2)" << std::endl;

    // 4. Probar remoción de luz de bloques: quitar la antorcha
    world.setBlock(8, 64, 8, BlockID::Air);
    world.getLightEngine().onBlockBroken(world, {8, 64, 8}, BlockID::Torch);
    assert(world.getBlockLight(8, 64, 8) == 0);
    assert(world.getBlockLight(9, 64, 8) == 0);
    assert(world.getBlockLight(10, 64, 8) == 0);
    std::cout << "  ✓ Block light removal BFS clears light properly" << std::endl;

    // 5. Probar luz solar directa vertical (columna completa con nivel 15)
    world.setBlock(8, 65, 8, BlockID::Air);
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            world.setBlock(x, 60, z, BlockID::Stone);
        }
    }
    Chunk* chunk = world.getChunk(0, 0);
    world.getLightEngine().initializeChunkLighting(world, *chunk);
    assert(world.getSkyLight(8, 70, 8) == 15);
    assert(world.getSkyLight(8, 61, 8) == 15);
    // Bajo la piedra debe ser 0
    assert(world.getSkyLight(8, 59, 8) == 0);
    std::cout << "  ✓ Direct skylight travels vertically without loss until obstacle" << std::endl;

    // 6. Colocar bloque opaco que tape la luz solar
    world.setBlock(8, 65, 8, BlockID::Stone);
    world.getLightEngine().onBlockPlaced(world, {8, 65, 8}, BlockID::Stone);
    assert(world.getSkyLight(8, 65, 8) == 0);
    assert(world.getSkyLight(8, 64, 8) < 15);
    std::cout << "  ✓ Opaque block placement shadows skylight column" << std::endl;

    std::cout << "[TEST PASSED] test_lighting_and_propagation passed successfully!" << std::endl;
    return 0;
}

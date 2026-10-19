#include <iostream>
#include <cassert>
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include "world/FluidSimulator.hpp"
#include "world/ScheduledUpdates.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "[TEST] Running test_fluid_flow_and_interaction..." << std::endl;

    World world;
    ScheduledUpdateQueue queue;
    FluidSimulator sim;

    // Cargar chunk (0, 0)
    world.setChunk(0, 0, std::make_unique<Chunk>(0, 0));

    // Preparar plataforma de piedra plana en y=60
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            world.setBlock(x, 60, z, BlockID::Stone);
            for (int y = 61; y <= 70; ++y) {
                world.setBlock(x, y, z, BlockID::Air);
            }
        }
    }

    // 1. Colocar fuente de agua en (8, 62, 8) y verificar caída vertical
    world.setBlock(8, 62, 8, BlockID::Water);
    sim.onFluidPlaced(world, queue, {8, 62, 8}, BlockID::Water, 0);

    assert(queue.size() > 0);
    std::vector<ScheduledBlockUpdate> ready;
    queue.pollReady(5, 100, ready);
    for (const auto& u : ready) {
        sim.processUpdate(world, queue, u, 5);
    }
    // Debe haber caído verticalmente hacia y=61
    assert(world.getBlock(8, 61, 8) == BlockID::FlowingWater);
    std::cout << "  ✓ Water flows downwards vertically with top priority" << std::endl;

    // 2. Extensión horizontal del agua en la plataforma (hasta 7 bloques)
    for (uint64_t tick = 10; tick <= 50; tick += 5) {
        queue.pollReady(tick, 100, ready);
        for (const auto& u : ready) {
            sim.processUpdate(world, queue, u, tick);
        }
    }
    // Vecinos adyacentes en y=61 deben tener agua fluyendo
    assert(world.getBlock(9, 61, 8) == BlockID::FlowingWater);
    assert(world.getBlock(7, 61, 8) == BlockID::FlowingWater);
    assert(world.getBlock(8, 61, 9) == BlockID::FlowingWater);
    assert(world.getBlock(8, 61, 7) == BlockID::FlowingWater);
    std::cout << "  ✓ Water spreads horizontally across solid floor" << std::endl;

    // 3. Creación de fuente infinita: 2 fuentes con un bloque de aire en medio rodeado
    world.setBlock(2, 61, 2, BlockID::Water);
    world.setBlock(4, 61, 2, BlockID::Water);
    world.setBlock(3, 61, 2, BlockID::FlowingWater);
    bool formed = sim.tryFormInfiniteWaterSource(world, {3, 61, 2});
    assert(formed);
    assert(world.getBlock(3, 61, 2) == BlockID::Water);
    std::cout << "  ✓ Infinite water source formed when 2 adjacent sources feed empty space" << std::endl;

    // 4. Interacción Agua + Lava -> Obsidiana (Agua toca fuente de Lava)
    world.setBlock(10, 61, 10, BlockID::Lava);         // Fuente de lava
    world.setBlock(11, 61, 10, BlockID::FlowingWater); // Agua fluyente adyacente
    bool converted = sim.checkFluidInteractions(world, {10, 61, 10});
    assert(converted);
    assert(world.getBlock(10, 61, 10) == BlockID::Obsidian);
    std::cout << "  ✓ Water hitting Lava source creates Obsidian" << std::endl;

    // 5. Interacción Agua + Lava fluyente -> Cobblestone (Agua toca lava que fluye)
    world.setBlock(12, 61, 12, BlockID::FlowingLava);
    world.setBlock(13, 61, 12, BlockID::FlowingWater);
    converted = sim.checkFluidInteractions(world, {12, 61, 12});
    assert(converted);
    assert(world.getBlock(12, 61, 12) == BlockID::Cobblestone);
    std::cout << "  ✓ Water hitting Flowing Lava creates Cobblestone" << std::endl;

    // 6. Lava cae verticalmente sobre agua -> Piedra (Stone)
    world.setBlock(5, 62, 5, BlockID::FlowingLava);
    world.setBlock(5, 61, 5, BlockID::Water);
    converted = sim.checkFluidInteractions(world, {5, 61, 5});
    assert(converted);
    assert(world.getBlock(5, 61, 5) == BlockID::Stone);
    std::cout << "  ✓ Lava falling vertically onto Water creates Stone" << std::endl;

    std::cout << "[TEST PASSED] test_fluid_flow_and_interaction passed successfully!" << std::endl;
    return 0;
}

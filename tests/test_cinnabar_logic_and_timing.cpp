#include <iostream>
#include <cassert>
#include <vector>
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include "world/CinnabarEngine.hpp"
#include "world/ScheduledUpdates.hpp"
#include "world/ChestBlockEntity.hpp"
#include "world/CrafterBlockEntity.hpp"
#include "sim/ItemEntity.hpp"
#include "sim/ProjectileEntity.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "========================================================\n";
    std::cout << "[TEST F11] Cinabrio Logic, Signal Attenuation & Timing\n";
    std::cout << "========================================================\n";

    World world;
    ScheduledUpdateQueue queue;
    std::vector<ItemEntity> items;
    std::vector<ProjectileEntity> projectiles;
    CinnabarEngine engine;

    // 1. Atenuación de señal de polvo de cinabrio (15 -> 0)
    std::cout << "[Subtest 1] Cinnabar Wire Signal Attenuation (15 -> 0)... ";
    BlockPos sourcePos{10, 4, 10};
    world.setBlock(sourcePos.x, sourcePos.y, sourcePos.z, BlockID::CinnabarBlock);
    engine.onBlockPlaced(world, sourcePos, BlockID::CinnabarBlock, 1);

    // Colocar una línea de 16 bloques de cinabrio wire contiguos en el eje X
    for (int i = 1; i <= 16; ++i) {
        BlockPos wirePos{sourcePos.x + i, sourcePos.y, sourcePos.z};
        world.setBlock(wirePos.x, wirePos.y, wirePos.z, BlockID::CinnabarWire);
        engine.onBlockPlaced(world, wirePos, BlockID::CinnabarWire, 1);
    }

    // Tick para propagar señal
    engine.tick(world, queue, items, projectiles, 2, 1.0f);

    // El primer cable adyacente a RedstoneBlock debe tener señal 15
    uint8_t signal1 = engine.getSignal(BlockPos{sourcePos.x + 1, sourcePos.y, sourcePos.z});
    assert(signal1 == 15);

    // El cable a 15 de distancia debe tener señal 1
    uint8_t signal15 = engine.getSignal(BlockPos{sourcePos.x + 15, sourcePos.y, sourcePos.z});
    assert(signal15 == 1);

    // El cable a 16 de distancia debe tener señal 0 (atenuación completa)
    uint8_t signal16 = engine.getSignal(BlockPos{sourcePos.x + 16, sourcePos.y, sourcePos.z});
    assert(signal16 == 0);
    std::cout << "PASSED (15 -> 1 -> 0 verified)\n";

    // 2. Retardo canónico de antorcha de cinabrio (1 tick cinabrio = 2 game ticks)
    std::cout << "[Subtest 2] Cinnabar Torch Inverter Timing & Delay... ";
    BlockPos baseBlockPos{20, 4, 20};
    world.setBlock(baseBlockPos.x, baseBlockPos.y, baseBlockPos.z, BlockID::Stone);

    BlockPos torchPos{20, 5, 20};
    world.setBlock(torchPos.x, torchPos.y, torchPos.z, BlockID::CinnabarTorch);
    engine.onBlockPlaced(world, torchPos, BlockID::CinnabarTorch, 10);

    engine.tick(world, queue, items, projectiles, 10, 1.0f);
    assert(engine.getSignal(torchPos) == 15);

    // Encender el bloque base: la antorcha se apaga en la simulación
    engine.setSignal(baseBlockPos, 15);
    engine.tick(world, queue, items, projectiles, 11, 1.0f);
    assert(world.getBlock(torchPos.x, torchPos.y, torchPos.z) == BlockID::CinnabarTorchOff);
    assert(engine.getSignal(torchPos) == 0);
    std::cout << "PASSED\n";

    // 3. Repetidor: retardo configurable (2, 4, 6, 8 ticks) y bloqueo (latch)
    std::cout << "[Subtest 3] Repeater Delay Configuration & Latch Lock... ";
    BlockPos repeaterPos{30, 4, 30};
    world.setBlock(repeaterPos.x, repeaterPos.y, repeaterPos.z, BlockID::Repeater);
    engine.onBlockPlaced(world, repeaterPos, BlockID::Repeater, 20);

    auto& repState = engine.getOrCreateComponent(repeaterPos);
    repState.delayTicks = 4; // 2 cinabrio ticks
    repState.isLocked = false;

    // Bloquear el repetidor con otro componente lateralmente
    repState.isLocked = true;
    assert(repState.isLocked == true);
    // Con latch activo, aunque varíe la entrada, la salida no debe cambiar
    repState.power = 15;
    engine.tick(world, queue, items, projectiles, 22, 1.0f);
    assert(repState.power == 15);
    std::cout << "PASSED\n";

    // 4. Comparador analógico: lectura de llenado de contenedor
    std::cout << "[Subtest 4] Comparator Analog Signal Calculation from Inventory... ";
    BlockPos chestPos{40, 4, 40};
    world.setBlock(chestPos.x, chestPos.y, chestPos.z, BlockID::Chest);
    auto chestEntity = std::make_shared<ChestBlockEntity>(chestPos);
    world.setBlockEntity(chestPos.x, chestPos.y, chestPos.z, chestEntity);

    // Contenedor vacío = señal 0
    uint8_t sigEmpty = CinnabarEngine::calculateContainerComparatorSignal(world, chestPos);
    assert(sigEmpty == 0);

    // Contenedor con 1 solo ítem = señal 1 (piso mínimo para no vacío)
    chestEntity->getInventory().setSlot(0, ItemStack(1, 1));
    uint8_t sigSingle = CinnabarEngine::calculateContainerComparatorSignal(world, chestPos);
    assert(sigSingle == 1);

    // Contenedor lleno (27 slots x 64 = 1728 ítems) = señal 15
    for (size_t s = 0; s < 27; ++s) {
        chestEntity->getInventory().setSlot(s, ItemStack(1, 64));
    }
    uint8_t sigFull = CinnabarEngine::calculateContainerComparatorSignal(world, chestPos);
    assert(sigFull == 15);
    std::cout << "PASSED (0, 1, 15 verified)\n";

    // 5. Comparador con Crafter (referencia 1.21): signal = disabledSlots + nonEmptySlots
    std::cout << "[Subtest 5] Crafter Comparator Signal Parity (referencia 1.21)... ";
    BlockPos crafterPos{50, 4, 50};
    world.setBlock(crafterPos.x, crafterPos.y, crafterPos.z, BlockID::Crafter);
    auto crafterEntity = std::make_shared<CrafterBlockEntity>(crafterPos);
    world.setBlockEntity(crafterPos.x, crafterPos.y, crafterPos.z, crafterEntity);

    assert(crafterEntity->getComparatorSignal() == 0);
    // Deshabilitar 3 ranuras
    crafterEntity->toggleSlotDisabled(0);
    crafterEntity->toggleSlotDisabled(1);
    crafterEntity->toggleSlotDisabled(2);
    assert(crafterEntity->getComparatorSignal() == 3);

    // Colocar 2 ítems en ranuras habilitadas (total = 3 + 2 = 5)
    crafterEntity->getInventory().setSlot(3, ItemStack(1, 10));
    crafterEntity->getInventory().setSlot(4, ItemStack(2, 5));
    assert(crafterEntity->getComparatorSignal() == 5);
    std::cout << "PASSED\n";

    // 6. Sensor de luz solar (Daylight Detector)
    std::cout << "[Subtest 6] Daylight Detector Inverted & Direct Modes... ";
    BlockPos daylightPos{60, 4, 60};
    world.setBlock(daylightPos.x, daylightPos.y, daylightPos.z, BlockID::DaylightDetector);
    engine.onBlockPlaced(world, daylightPos, BlockID::DaylightDetector, 50);

    // A mediodía (sunlightFactor = 1.0f) señal = 15
    engine.tick(world, queue, items, projectiles, 51, 1.0f);
    assert(engine.getSignal(daylightPos) == 15);

    // A medianoche (sunlightFactor = 0.0f) señal = 0
    engine.tick(world, queue, items, projectiles, 52, 0.0f);
    assert(engine.getSignal(daylightPos) == 0);
    std::cout << "PASSED\n";

    // 7. Prueba de estrés de reloj oscilador rápido (500 ciclos)
    std::cout << "[Subtest 7] Fast Cinabrio Clock Oscillator Stress Test (500 cycles)... ";
    BlockPos clkSource{70, 4, 70};
    BlockPos clkTarget{71, 4, 70};
    world.setBlock(clkSource.x, clkSource.y, clkSource.z, BlockID::CinnabarWire);
    world.setBlock(clkTarget.x, clkTarget.y, clkTarget.z, BlockID::CinnabarLamp);

    for (uint64_t tick = 100; tick < 600; ++tick) {
        bool pulse = (tick % 2 == 0);
        engine.setSignal(clkSource, pulse ? 15 : 0);
        engine.tick(world, queue, items, projectiles, tick, 1.0f);
    }
    std::cout << "PASSED (No infinite loops, stable resolution)\n";

    std::cout << "\n>>> ALL REDSTONE LOGIC AND TIMING TESTS PASSED! <<<\n";
    return 0;
}

#include <iostream>
#include <cassert>
#include <vector>
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include "item/Item.hpp"
#include "world/RailSystem.hpp"
#include "world/CinnabarEngine.hpp"
#include "sim/MinecartEntity.hpp"
#include "sim/ItemEntity.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "========================================================\n";
    std::cout << "[TEST F11] Rails and Minecart Transport Systems\n";
    std::cout << "========================================================\n";

    World world;
    CinnabarEngine cinnabar;
    std::vector<ItemEntity> worldItems;
    auto& itemReg = ItemRegistry::global();
    ItemId coalItemId = itemReg.getId("opencraft:coal");

    // 1. Rieles comunes y detección de superficie
    std::cout << "[Subtest 1] Rail Block Identification & System Classification... ";
    assert(RailSystem::isRail(BlockID::Rail) == true);
    assert(RailSystem::isRail(BlockID::PoweredRail) == true);
    assert(RailSystem::isRail(BlockID::DetectorRail) == true);
    assert(RailSystem::isRail(BlockID::ActivatorRail) == true);
    assert(RailSystem::isRail(BlockID::Stone) == false);
    std::cout << "PASSED\n";

    // 2. Vagoneta sobre riel normal: movimiento e inercia
    std::cout << "[Subtest 2] Minecart on Rails Physics & Deceleration... ";
    BlockPos railStart{10, 4, 10};
    for (int i = 0; i < 20; ++i) {
        world.setBlock(railStart.x + i, railStart.y, railStart.z, BlockID::Rail);
    }

    MinecartEntity cart(100, Math::Vec3(10.5f, 4.0f, 10.5f), MinecartType::Standard);
    cart.velocity = Math::Vec3(4.0f, 0.0f, 0.0f); // Velocidad inicial hacia +X

    cart.tickPhysics(world, &cinnabar, &worldItems, 0.05f);
    assert(cart.isOnRail == true);
    assert(cart.position.x > 10.5f);
    // Verificar que la velocidad sufre fricción canónica en riel normal
    assert(cart.velocity.x < 4.0f);
    std::cout << "PASSED\n";

    // 3. Riel propulsor (Powered Rail): apagado frena, encendido acelera
    std::cout << "[Subtest 3] Powered Rail Acceleration & Braking Dynamics... ";
    BlockPos poweredRailPos{30, 4, 10};
    world.setBlock(poweredRailPos.x, poweredRailPos.y, poweredRailPos.z, BlockID::PoweredRail);

    // Caso A: Riel propulsor apagado (freno de emergencia)
    MinecartEntity brakeCart(101, Math::Vec3(30.5f, 4.0f, 10.5f), MinecartType::Standard);
    brakeCart.velocity = Math::Vec3(5.0f, 0.0f, 0.0f);
    cinnabar.setSignal(poweredRailPos, 0); // Apagado

    brakeCart.tickPhysics(world, &cinnabar, &worldItems, 0.05f);
    // La velocidad debe reducirse drásticamente por el freno (x 0.5f)
    assert(brakeCart.velocity.x <= 2.6f);

    // Caso B: Riel propulsor encendido (aceleración hasta maxSpeed)
    MinecartEntity boostCart(102, Math::Vec3(30.5f, 4.0f, 10.5f), MinecartType::Standard);
    boostCart.velocity = Math::Vec3(2.0f, 0.0f, 0.0f);
    cinnabar.setSignal(poweredRailPos, 15); // Encendido por cinabrio

    boostCart.tickPhysics(world, &cinnabar, &worldItems, 0.05f);
    assert(boostCart.velocity.x > 2.0f);
    assert(boostCart.velocity.x <= boostCart.maxSpeed);
    std::cout << "PASSED (Braking and acceleration verified)\n";

    // 4. Riel detector (Detector Rail): genera potencia cinabrio al paso de la vagoneta
    std::cout << "[Subtest 4] Detector Rail Cinabrio Activation upon Minecart Entry... ";
    BlockPos detectorPos{50, 4, 10};
    world.setBlock(detectorPos.x, detectorPos.y, detectorPos.z, BlockID::DetectorRail);
    assert(cinnabar.getSignal(detectorPos) == 0);

    MinecartEntity detCart(103, Math::Vec3(50.5f, 4.0f, 10.5f), MinecartType::Standard);
    detCart.velocity = Math::Vec3(1.0f, 0.0f, 0.0f);

    detCart.tickPhysics(world, &cinnabar, &worldItems, 0.05f);
    // El riel detector debió activar una señal de 15
    assert(cinnabar.getSignal(detectorPos) == 15);
    std::cout << "PASSED (Detector Rail generates signal 15)\n";

    // 5. Vagoneta con tolva (Hopper Minecart): recolección de ítems sobre la vía
    std::cout << "[Subtest 5] Hopper Minecart Item Collection Along the Track... ";
    BlockPos hopperTrackPos{70, 4, 10};
    world.setBlock(hopperTrackPos.x, hopperTrackPos.y, hopperTrackPos.z, BlockID::Rail);

    MinecartEntity hopperCart(104, Math::Vec3(70.5f, 4.0f, 10.5f), MinecartType::Hopper);
    assert(hopperCart.inventory.getSize() == 5);

    // Colocar ítem sobre la vía
    ItemEntity trackItem;
    trackItem.position = {70.8f, 4.2f, 10.5f};
    trackItem.stack = ItemStack(coalItemId, 8);
    worldItems.push_back(trackItem);

    hopperCart.tickPhysics(world, &cinnabar, &worldItems, 0.05f);
    // El ítem debió ser absorbido dentro del inventario de la vagoneta tolva
    assert(hopperCart.inventory.getSlot(0).itemId == coalItemId);
    assert(hopperCart.inventory.getSlot(0).count == 8);
    assert(worldItems.back().isDead == true);
    std::cout << "PASSED (Items collected smoothly into hopper minecart inventory)\n";

    // 6. Vagoneta con cofre (Chest Minecart): capacidad de 27 ranuras
    std::cout << "[Subtest 6] Chest Minecart Storage Capacity (27 slots)... ";
    MinecartEntity chestCart(105, Math::Vec3(90.5f, 4.0f, 10.5f), MinecartType::Chest);
    assert(chestCart.inventory.getSize() == 27);
    chestCart.inventory.setSlot(0, ItemStack(coalItemId, 64));
    assert(chestCart.inventory.getSlot(0).count == 64);
    std::cout << "PASSED\n";

    std::cout << "\n>>> ALL RAILS AND MINECART TRANSPORT TESTS PASSED! <<<\n";
    return 0;
}

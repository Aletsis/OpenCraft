#include <iostream>
#include <cassert>
#include <vector>
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include "item/Item.hpp"
#include "world/HopperBlockEntity.hpp"
#include "world/DropperBlockEntity.hpp"
#include "world/DispenserBlockEntity.hpp"
#include "world/CrafterBlockEntity.hpp"
#include "world/AutomationBlockEntities.hpp"
#include "world/ChestBlockEntity.hpp"
#include "crafting/RecipeRegistry.hpp"
#include "sim/ItemEntity.hpp"
#include "sim/ProjectileEntity.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "========================================================\n";
    std::cout << "[TEST F11] Hoppers, Droppers, Dispensers & Crafter 1.21\n";
    std::cout << "========================================================\n";

    World world;
    std::vector<ItemEntity> worldItems;
    std::vector<ProjectileEntity> worldProjectiles;
    auto& itemReg = ItemRegistry::global();
    ItemId stoneItemId = itemReg.getId("opencraft:stone");
    ItemId arrowItemId = itemReg.getId("opencraft:arrow");
    ItemId bucketItemId = itemReg.getId("opencraft:bucket");
    ItemId waterBucketItemId = itemReg.getId("opencraft:water_bucket");
    ItemId planksItemId = itemReg.getId("opencraft:planks");
    ItemId stickItemId = itemReg.getId("opencraft:stick");

    // 1. Tolva: Succión de ítem superior en 3D
    std::cout << "[Subtest 1] Hopper 3D Item Entity Suction... ";
    BlockPos hopperPos{10, 4, 10};
    world.setBlock(hopperPos.x, hopperPos.y, hopperPos.z, BlockID::Hopper);
    auto hopperEntity = std::make_shared<HopperBlockEntity>(hopperPos, HopperFacing::Down);
    world.setBlockEntity(hopperPos.x, hopperPos.y, hopperPos.z, hopperEntity);

    // Colocar un ítem flotando sobre la tolva
    ItemEntity droppedItem;
    droppedItem.position = {10.5f, 5.2f, 10.5f};
    droppedItem.stack = ItemStack(stoneItemId, 16);
    droppedItem.pickupDelayTicks = 0;
    worldItems.push_back(droppedItem);

    bool transferOk = hopperEntity->stepTransfer(world, worldItems);
    assert(transferOk == true);
    // El ítem fue absorbido por la tolva
    assert(hopperEntity->getInventory().getSlot(0).count == 16);
    assert(hopperEntity->getInventory().getSlot(0).itemId == stoneItemId);
    assert(worldItems[0].isDead == true);
    // La tolva entra en enfriamiento de 8 ticks
    assert(hopperEntity->getCooldown() == 8);
    std::cout << "PASSED (8-tick cooldown activated)\n";

    // 2. Tolva: Bloqueo por cinabrio (isLocked)
    std::cout << "[Subtest 2] Hopper Cinabrio Locking (Inhibits transfer)... ";
    hopperEntity->setCooldown(0);
    hopperEntity->setLocked(true);

    ItemEntity droppedItem2;
    droppedItem2.position = {10.5f, 5.2f, 10.5f};
    droppedItem2.stack = ItemStack(stoneItemId, 5);
    worldItems.push_back(droppedItem2);

    bool lockedTransfer = hopperEntity->stepTransfer(world, worldItems);
    assert(lockedTransfer == false);
    assert(worldItems.back().isDead == false);
    std::cout << "PASSED\n";

    // 3. Cadena de Tolvas y Prueba de Conservación de Masa (1,000 ítems)
    std::cout << "[Subtest 3] 1,000 Items Mass Conservation Chain Transfer... ";
    worldItems.clear();
    hopperEntity->setLocked(false);
    hopperEntity->setCooldown(0);
    // Vaciar tolva inicial
    hopperEntity->getInventory().clear();

    BlockPos chestDestPos{10, 3, 10};
    world.setBlock(chestDestPos.x, chestDestPos.y, chestDestPos.z, BlockID::Chest);
    auto chestEntity = std::make_shared<ChestBlockEntity>(chestDestPos);
    world.setBlockEntity(chestDestPos.x, chestDestPos.y, chestDestPos.z, chestEntity);

    // Inyectar 1,000 unidades en lotes a través de la tolva
    uint32_t totalTransferred = 0;
    for (int batch = 0; batch < 1000; ++batch) {
        hopperEntity->setCooldown(0);
        hopperEntity->getInventory().setSlot(0, ItemStack(stoneItemId, 1));
        bool pushed = hopperEntity->stepTransfer(world, worldItems);
        if (pushed) {
            totalTransferred++;
        }
    }
    // Verificar que los ítems están todos dentro del cofre sin pérdidas ni duplicaciones
    uint32_t chestCount = 0;
    for (size_t s = 0; s < chestEntity->getInventory().getSize(); ++s) {
        chestCount += chestEntity->getInventory().getSlot(s).count;
    }
    assert(totalTransferred == 1000);
    assert(chestCount == 1000);
    std::cout << "PASSED (1,000 / 1,000 items preserved exactly)\n";

    // 4. Soltador (Dropper): expulsión de ítems hacia un cofre contiguo
    std::cout << "[Subtest 4] Dropper Injection into Adjacent Container... ";
    BlockPos dropperPos{20, 4, 20};
    world.setBlock(dropperPos.x, dropperPos.y, dropperPos.z, BlockID::Dropper);
    auto dropperEntity = std::make_shared<DropperBlockEntity>(dropperPos, BlockDirection::North);
    dropperEntity->getInventory().setSlot(0, ItemStack(stoneItemId, 5));
    world.setBlockEntity(dropperPos.x, dropperPos.y, dropperPos.z, dropperEntity);

    // Contenedor destino en North (z - 1)
    BlockPos targetChestPos{20, 4, 19};
    world.setBlock(targetChestPos.x, targetChestPos.y, targetChestPos.z, BlockID::Chest);
    auto targetChest = std::make_shared<ChestBlockEntity>(targetChestPos);
    world.setBlockEntity(targetChestPos.x, targetChestPos.y, targetChestPos.z, targetChest);

    bool dropSuccess = AutomationActions::triggerDropper(world, *dropperEntity, worldItems);
    assert(dropSuccess == true);
    assert(dropperEntity->getInventory().getSlot(0).count == 4);
    assert(targetChest->getInventory().getSlot(0).count == 1);
    std::cout << "PASSED (Item moved directly to target container)\n";

    // 5. Dispensador (Dispenser): flechas como proyectiles y cubetas de agua
    std::cout << "[Subtest 5] Dispenser Arrow Projectile & Fluid Placement... ";
    BlockPos dispenserPos{30, 4, 30};
    world.setBlock(dispenserPos.x, dispenserPos.y, dispenserPos.z, BlockID::Dispenser);
    auto dispenserEntity = std::make_shared<DispenserBlockEntity>(dispenserPos, BlockDirection::East);
    dispenserEntity->getInventory().setSlot(0, ItemStack(arrowItemId, 3));
    world.setBlockEntity(dispenserPos.x, dispenserPos.y, dispenserPos.z, dispenserEntity);

    // Disparar flecha
    bool dispArrow = AutomationActions::triggerDispenser(world, *dispenserEntity, worldItems, worldProjectiles);
    assert(dispArrow == true);
    assert(dispenserEntity->getInventory().getSlot(0).count == 2);
    assert(!worldProjectiles.empty());
    assert(worldProjectiles.back().type == ProjectileType::Arrow);

    // Cubeta de agua: debe colocar un bloque de agua y dejar cubeta vacía
    BlockPos fluidDispPos{40, 4, 40};
    world.setBlock(fluidDispPos.x, fluidDispPos.y, fluidDispPos.z, BlockID::Dispenser);
    auto fluidDispEntity = std::make_shared<DispenserBlockEntity>(fluidDispPos, BlockDirection::East);
    fluidDispEntity->getInventory().setSlot(0, ItemStack(waterBucketItemId, 1));
    world.setBlockEntity(fluidDispPos.x, fluidDispPos.y, fluidDispPos.z, fluidDispEntity);
    // Espacio libre delante (41, 4, 40)
    world.setBlock(41, 4, 40, BlockID::Air);

    bool dispWater = AutomationActions::triggerDispenser(world, *fluidDispEntity, worldItems, worldProjectiles);
    assert(dispWater == true);
    assert(world.getBlock(41, 4, 40) == BlockID::Water);
    assert(fluidDispEntity->getInventory().getSlot(0).itemId == bucketItemId);
    std::cout << "PASSED (Arrow spawned, water placed, bucket emptied)\n";

    // 6. Crafter Automático (referencia 1.21): Fabricación y ranuras deshabilitadas
    std::cout << "[Subtest 6] Crafter 1.21 Automated Recipe Execution & Disabled Slots... ";
    BlockPos crafterPos{50, 4, 50};
    world.setBlock(crafterPos.x, crafterPos.y, crafterPos.z, BlockID::Crafter);
    auto crafterEntity = std::make_shared<CrafterBlockEntity>(crafterPos, BlockDirection::East);
    world.setBlockEntity(crafterPos.x, crafterPos.y, crafterPos.z, crafterEntity);

    // Receta de palos (2 tablones verticales en 3x3):
    // slot 0 (col 0, fila 0) = planks
    // slot 3 (col 0, fila 1) = planks
    // deshabilitamos las ranuras restantes (1, 2, 4, 5, 6, 7, 8)
    crafterEntity->getInventory().setSlot(0, ItemStack(planksItemId, 2));
    crafterEntity->getInventory().setSlot(3, ItemStack(planksItemId, 2));
    for (size_t s = 0; s < 9; ++s) {
        if (s != 0 && s != 3) {
            crafterEntity->toggleSlotDisabled(s);
        }
    }

    worldItems.clear();
    bool craftOk = AutomationActions::triggerCrafter(world, *crafterEntity, worldItems);
    assert(craftOk == true);
    // Se consumió 1 tablón de cada ranura
    assert(crafterEntity->getInventory().getSlot(0).count == 1);
    assert(crafterEntity->getInventory().getSlot(3).count == 1);
    // Se expulsó 4 palos (resultado de la receta)
    assert(!worldItems.empty());
    assert(worldItems.back().stack.itemId == stickItemId);
    assert(worldItems.back().stack.count == 4);
    std::cout << "PASSED (Crafter 1.21 recipe matched and crafted autonomously!)\n";

    std::cout << "\n>>> ALL HOPPERS, DROPPERS, DISPENSERS AND CRAFTER TESTS PASSED! <<<\n";
    return 0;
}

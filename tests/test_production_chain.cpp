#include <iostream>
#include <cassert>
#include "sim/WorldSession.hpp"
#include "item/Item.hpp"
#include "world/BlockRegistry.hpp"
#include "crafting/RecipeRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F05 Prueba 1] Cadena de Produccion Completa (Madera -> Hierro) ---" << std::endl;

    WorldSession session;
    session.init(1, 12345ULL);

    auto& itemReg = ItemRegistry::global();
    auto& blockReg = BlockRegistry::global();

    ItemId woodLogId = itemReg.getId("opencraft:wood_log");
    ItemId planksId = itemReg.getId("opencraft:planks");
    ItemId tableId = itemReg.getId("opencraft:crafting_table");
    ItemId stickId = itemReg.getId("opencraft:stick");
    ItemId woodPickId = itemReg.getId("opencraft:wooden_pickaxe");
    ItemId cobblestoneId = itemReg.getId("opencraft:cobblestone");
    ItemId stonePickId = itemReg.getId("opencraft:stone_pickaxe");
    ItemId furnaceId = itemReg.getId("opencraft:furnace");
    ItemId coalId = itemReg.getId("opencraft:coal");
    ItemId rawIronId = itemReg.getId("opencraft:raw_iron");
    ItemId ironIngotId = itemReg.getId("opencraft:iron_ingot");
    ItemId ironPickId = itemReg.getId("opencraft:iron_pickaxe");

    assert(woodLogId != EMPTY_ITEM_ID);
    assert(planksId != EMPTY_ITEM_ID);
    assert(tableId != EMPTY_ITEM_ID);
    assert(stickId != EMPTY_ITEM_ID);
    assert(woodPickId != EMPTY_ITEM_ID);
    assert(cobblestoneId != EMPTY_ITEM_ID);
    assert(stonePickId != EMPTY_ITEM_ID);
    assert(furnaceId != EMPTY_ITEM_ID);
    assert(coalId != EMPTY_ITEM_ID);
    assert(rawIronId != EMPTY_ITEM_ID);
    assert(ironIngotId != EMPTY_ITEM_ID);
    assert(ironPickId != EMPTY_ITEM_ID);

    // Vaciar inventario
    session.getPlayer().inventory.clear();
    session.getPlayer().cursorItem.clear();
    session.getPlayer().syncHotbar();

    // 1. Minar 3 bloques de madera con la mano desarmada
    session.getPlayer().position = Math::Vec3(0.0f, 10.0f, 0.0f);
    session.getWorld().setBlockState(1, 10, 0, blockReg.getStateId("opencraft:wood_log"));
    session.getWorld().setBlockState(1, 11, 0, blockReg.getStateId("opencraft:wood_log"));
    session.getWorld().setBlockState(1, 12, 0, blockReg.getStateId("opencraft:wood_log"));

    session.queueCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 1, .y = 10, .z = 0});
    session.queueCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 1, .y = 11, .z = 0});
    session.queueCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 1, .y = 12, .z = 0});
    session.update(0.05f);

    for (int i = 0; i < 15; ++i) {
        session.update(0.05f);
    }

    assert(session.getPlayer().inventory.hasItem(woodLogId, 3) && "Debe haber recogido 3 troncos de madera");
    std::cout << "Paso 1: 3 troncos de madera recolectados." << std::endl;

    // 2. Fabricar tablones (planks) usando la cuadricula 2x2
    size_t woodSlot = 0;
    for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
        if (session.getPlayer().inventory.getSlot(i).itemId == woodLogId) {
            woodSlot = i;
            break;
        }
    }
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::PlayerInventory,
                                                     .fromSlot = woodSlot,
                                                     .toContainer = InventoryContainerType::CraftingGrid,
                                                     .toSlot = 0,
                                                     .amount = 1});
    session.update(0.05f);

    WorldSnapshot snap = session.getSnapshot();
    assert(!snap.craftingOutput.isEmpty() && snap.craftingOutput.itemId == planksId && snap.craftingOutput.count == 4);

    // Tomar los 4 tablones desde CraftingOutput al inventario
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::CraftingOutput,
                                                     .fromSlot = 0,
                                                     .toContainer = InventoryContainerType::PlayerInventory,
                                                     .toSlot = 1,
                                                     .amount = 4});
    session.update(0.05f);

    assert(session.getPlayer().inventory.hasItem(planksId, 4));
    assert(session.getPlayer().inventory.hasItem(woodLogId, 2));
    std::cout << "Paso 2: 4 tablones fabricados a partir de 1 tronco." << std::endl;

    // 3. Fabricar Mesa de Trabajo con 4 tablones en cuadricula 2x2
    for (size_t s = 0; s < 4; ++s) {
        session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                         .action = InventoryAction::Move,
                                                         .fromContainer = InventoryContainerType::PlayerInventory,
                                                         .fromSlot = 1,
                                                         .toContainer = InventoryContainerType::CraftingGrid,
                                                         .toSlot = s,
                                                         .amount = 1});
        session.update(0.05f);
    }

    snap = session.getSnapshot();
    assert(!snap.craftingOutput.isEmpty() && snap.craftingOutput.itemId == tableId && snap.craftingOutput.count == 1);

    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::CraftingOutput,
                                                     .fromSlot = 0,
                                                     .toContainer = InventoryContainerType::PlayerInventory,
                                                     .toSlot = 1,
                                                     .amount = 1});
    session.update(0.05f);

    assert(session.getPlayer().inventory.hasItem(tableId, 1));
    std::cout << "Paso 3: Mesa de crafteo fabricada." << std::endl;

    // Convertir otro tronco a 4 tablones para palos y pico
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::PlayerInventory,
                                                     .fromSlot = woodSlot,
                                                     .toContainer = InventoryContainerType::CraftingGrid,
                                                     .toSlot = 0,
                                                     .amount = 1});
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::CraftingOutput,
                                                     .fromSlot = 0,
                                                     .toContainer = InventoryContainerType::PlayerInventory,
                                                     .toSlot = 2,
                                                     .amount = 4});
    session.update(0.05f);
    assert(session.getPlayer().inventory.hasItem(planksId, 4));

    // Fabricar 4 palos usando 2 tablones verticales (ranura 0 y 2)
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::PlayerInventory,
                                                     .fromSlot = 2,
                                                     .toContainer = InventoryContainerType::CraftingGrid,
                                                     .toSlot = 0,
                                                     .amount = 1});
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::PlayerInventory,
                                                     .fromSlot = 2,
                                                     .toContainer = InventoryContainerType::CraftingGrid,
                                                     .toSlot = 2,
                                                     .amount = 1});
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::CraftingOutput,
                                                     .fromSlot = 0,
                                                     .toContainer = InventoryContainerType::PlayerInventory,
                                                     .toSlot = 3,
                                                     .amount = 4});
    session.update(0.05f);
    assert(session.getPlayer().inventory.hasItem(stickId, 4));
    std::cout << "Paso 4: 4 palos fabricados." << std::endl;

    // Colocar la mesa de trabajo en el mundo e interactuar para abrir la rejilla 3x3
    session.getWorld().setBlockState(1, 10, 0, blockReg.getStateId("opencraft:crafting_table"));
    session.queueCommand(InteractBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 1, .y = 10, .z = 0});
    session.update(0.05f);

    snap = session.getSnapshot();
    assert(snap.openContainerKind == OpenContainerKind::CraftingTable);
    assert(snap.craftingGrid.size() == 9);

    // Convertir el tercer tronco a tablones en la rejilla 3x3 slot 0
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::PlayerInventory,
                                                     .fromSlot = woodSlot,
                                                     .toContainer = InventoryContainerType::CraftingGrid,
                                                     .toSlot = 0,
                                                     .amount = 1});
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::CraftingOutput,
                                                     .fromSlot = 0,
                                                     .toContainer = InventoryContainerType::PlayerInventory,
                                                     .toSlot = 2,
                                                     .amount = 4});
    session.update(0.05f);

    // 5. Fabricar Pico de Madera (3 tablones en fila superior 0,1,2; 2 palos en 4,7)
    for (size_t s = 0; s < 3; ++s) {
        session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                         .action = InventoryAction::Move,
                                                         .fromContainer = InventoryContainerType::PlayerInventory,
                                                         .fromSlot = 2,
                                                         .toContainer = InventoryContainerType::CraftingGrid,
                                                         .toSlot = s,
                                                         .amount = 1});
    }
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::PlayerInventory,
                                                     .fromSlot = 3,
                                                     .toContainer = InventoryContainerType::CraftingGrid,
                                                     .toSlot = 4,
                                                     .amount = 1});
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::PlayerInventory,
                                                     .fromSlot = 3,
                                                     .toContainer = InventoryContainerType::CraftingGrid,
                                                     .toSlot = 7,
                                                     .amount = 1});
    session.update(0.05f);

    snap = session.getSnapshot();
    assert(!snap.craftingOutput.isEmpty() && snap.craftingOutput.itemId == woodPickId);

    // Extraer Pico de Madera al inventario (slot 0)
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::CraftingOutput,
                                                     .fromSlot = 0,
                                                     .toContainer = InventoryContainerType::PlayerInventory,
                                                     .toSlot = 0,
                                                     .amount = 1});
    session.queueCommand(CloseContainerCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
    session.update(0.05f);

    assert(session.getPlayer().inventory.hasItem(woodPickId, 1));
    std::cout << "Paso 5: Pico de madera fabricado y equipado." << std::endl;

    // Seleccionar slot con pico de madera
    size_t woodPickSlot = 0;
    for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
        if (session.getPlayer().inventory.getSlot(i).itemId == woodPickId) {
            woodPickSlot = i;
            break;
        }
    }
    session.queueCommand(SelectSlotCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .slot = woodPickSlot});
    session.update(0.05f);

    // 6. Minar 8 bloques de Stone con el pico de madera -> 8 Cobblestone
    session.getPlayer().position = Math::Vec3(3.5f, 10.0f, 0.5f);
    for (int b = 0; b < 8; ++b) {
        int bx = 2 + (b % 4);
        int bz = b / 4;
        session.getWorld().setBlockState(bx, 10, bz, blockReg.getStateId("opencraft:stone"));
        session.queueCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = bx, .y = 10, .z = bz});
        session.update(0.05f);
    }
    for (int i = 0; i < 20; ++i) session.update(0.05f);

    assert(session.getPlayer().inventory.hasItem(cobblestoneId, 8));
    std::cout << "Paso 6: 8 adoquines (cobblestone) minados con pico de madera." << std::endl;

    // 7. Fabricar Horno (8 adoquines en anillo en la mesa de crafteo)
    session.getPlayer().position = Math::Vec3(0.0f, 10.0f, 0.0f);
    session.queueCommand(InteractBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 1, .y = 10, .z = 0});
    session.update(0.05f);

    size_t cobbleSlot = 0;
    for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
        if (session.getPlayer().inventory.getSlot(i).itemId == cobblestoneId) {
            cobbleSlot = i;
            break;
        }
    }

    size_t ringSlots[8] = {0, 1, 2, 3, 5, 6, 7, 8};
    for (size_t s : ringSlots) {
        session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                         .action = InventoryAction::Move,
                                                         .fromContainer = InventoryContainerType::PlayerInventory,
                                                         .fromSlot = cobbleSlot,
                                                         .toContainer = InventoryContainerType::CraftingGrid,
                                                         .toSlot = s,
                                                         .amount = 1});
    }
    session.update(0.05f);

    snap = session.getSnapshot();
    assert(!snap.craftingOutput.isEmpty() && snap.craftingOutput.itemId == furnaceId);

    // Extraer horno al inventario
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::CraftingOutput,
                                                     .fromSlot = 0,
                                                     .toContainer = InventoryContainerType::PlayerInventory,
                                                     .toSlot = 4,
                                                     .amount = 1});
    session.queueCommand(CloseContainerCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
    session.update(0.05f);
    assert(session.getPlayer().inventory.hasItem(furnaceId, 1));
    std::cout << "Paso 7: Horno fabricado con 8 adoquines." << std::endl;

    // 8. Minar 3 adoquines mas y 1 mineral de carbon
    session.getPlayer().position = Math::Vec3(2.5f, 10.0f, 5.0f);
    for (int b = 0; b < 3; ++b) {
        int bx = 2 + b;
        session.getWorld().setBlockState(bx, 10, 5, blockReg.getStateId("opencraft:stone"));
        session.queueCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = bx, .y = 10, .z = 5});
        session.update(0.05f);
    }
    for (int i = 0; i < 20; ++i) session.update(0.05f);

    session.getWorld().setBlockState(2, 10, 6, blockReg.getStateId("opencraft:coal_ore"));
    session.queueCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 2, .y = 10, .z = 6});
    session.update(0.05f);
    for (int i = 0; i < 20; ++i) session.update(0.05f);
    assert(session.getPlayer().inventory.hasItem(coalId, 1));
    std::cout << "Paso 8: 1 mineral de carbon minado con exito." << std::endl;

    // Fabricar Pico de Piedra (3 adoquines + 2 palos)
    session.getPlayer().position = Math::Vec3(0.0f, 10.0f, 0.0f);
    session.queueCommand(InteractBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 1, .y = 10, .z = 0});
    session.update(0.05f);

    for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
        if (session.getPlayer().inventory.getSlot(i).itemId == cobblestoneId) cobbleSlot = i;
    }
    size_t stSlot = 0;
    for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
        if (session.getPlayer().inventory.getSlot(i).itemId == stickId) stSlot = i;
    }

    for (size_t s = 0; s < 3; ++s) {
        session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                         .action = InventoryAction::Move,
                                                         .fromContainer = InventoryContainerType::PlayerInventory,
                                                         .fromSlot = cobbleSlot,
                                                         .toContainer = InventoryContainerType::CraftingGrid,
                                                         .toSlot = s,
                                                         .amount = 1});
    }
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::PlayerInventory,
                                                     .fromSlot = stSlot,
                                                     .toContainer = InventoryContainerType::CraftingGrid,
                                                     .toSlot = 4,
                                                     .amount = 1});
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::PlayerInventory,
                                                     .fromSlot = stSlot,
                                                     .toContainer = InventoryContainerType::CraftingGrid,
                                                     .toSlot = 7,
                                                     .amount = 1});
    session.update(0.05f);

    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::CraftingOutput,
                                                     .fromSlot = 0,
                                                     .toContainer = InventoryContainerType::PlayerInventory,
                                                     .toSlot = 0,
                                                     .amount = 1});
    session.queueCommand(CloseContainerCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
    session.update(0.05f);
    assert(session.getPlayer().inventory.hasItem(stonePickId, 1));
    std::cout << "Paso 9: Pico de piedra fabricado y equipado." << std::endl;

    // 9. Minar 3 menas de hierro con el Pico de Piedra -> 3 Raw Iron
    size_t stonePickSlot = 0;
    for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
        if (session.getPlayer().inventory.getSlot(i).itemId == stonePickId) {
            stonePickSlot = i;
            break;
        }
    }
    session.queueCommand(SelectSlotCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .slot = stonePickSlot});
    session.update(0.05f);

    session.getPlayer().position = Math::Vec3(2.5f, 10.0f, 6.5f);
    for (int b = 0; b < 3; ++b) {
        int bx = 2 + b;
        session.getWorld().setBlockState(bx, 10, 7, blockReg.getStateId("opencraft:iron_ore"));
        session.queueCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = bx, .y = 10, .z = 7});
        session.update(0.05f);
    }
    for (int i = 0; i < 20; ++i) session.update(0.05f);
    assert(session.getPlayer().inventory.hasItem(rawIronId, 3));
    std::cout << "Paso 10: 3 minerales de hierro crudo (raw iron) obtenidos." << std::endl;

    // 10. Colocar el horno en (1, 10, 2), abrirlo y fundir hierro
    session.getPlayer().position = Math::Vec3(1.0f, 10.0f, 1.0f);
    session.getWorld().setBlockState(1, 10, 2, blockReg.getStateId("opencraft:furnace"));
    auto furnaceEntity = std::make_shared<FurnaceBlockEntity>(BlockPos{1, 10, 2});
    session.getWorld().setBlockEntity(1, 10, 2, furnaceEntity);

    session.queueCommand(InteractBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 1, .y = 10, .z = 2});
    session.update(0.05f);

    snap = session.getSnapshot();
    assert(snap.openContainerKind == OpenContainerKind::Furnace);

    size_t cSlot = 0, rSlot = 0;
    for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
        if (session.getPlayer().inventory.getSlot(i).itemId == coalId) cSlot = i;
        if (session.getPlayer().inventory.getSlot(i).itemId == rawIronId) rSlot = i;
    }

    // Colocar carbon en ranura 1 (fuel)
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::PlayerInventory,
                                                     .fromSlot = cSlot,
                                                     .toContainer = InventoryContainerType::FurnaceContainer,
                                                     .toSlot = 1,
                                                     .amount = 1});
    // Colocar 3 hierro crudo en ranura 0 (input)
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::PlayerInventory,
                                                     .fromSlot = rSlot,
                                                     .toContainer = InventoryContainerType::FurnaceContainer,
                                                     .toSlot = 0,
                                                     .amount = 3});
    session.update(0.05f);

    std::cout << "Fundiendo 3 lingotes de hierro en el horno (605 ticks = ~30s)..." << std::endl;
    for (int t = 0; t < 605; ++t) {
        session.update(0.05f);
    }

    snap = session.getSnapshot();
    assert(snap.containerSlots[2].itemId == ironIngotId && snap.containerSlots[2].count == 3);
    std::cout << "Paso 11: 3 lingotes de hierro fundidos exitosamente en el horno." << std::endl;

    // Extraer 3 lingotes al inventario
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::FurnaceContainer,
                                                     .fromSlot = 2,
                                                     .toContainer = InventoryContainerType::PlayerInventory,
                                                     .toSlot = 5,
                                                     .amount = 3});
    session.queueCommand(CloseContainerCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
    session.update(0.05f);
    assert(session.getPlayer().inventory.hasItem(ironIngotId, 3));

    // 11. Fabricar Pico de Hierro en la mesa de crafteo
    session.getPlayer().position = Math::Vec3(0.0f, 10.0f, 0.0f);
    session.queueCommand(InteractBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 1, .y = 10, .z = 0});
    session.update(0.05f);

    // Si no quedan palos suficientes, fabricar 4 palos más usando 2 tablones
    if (!session.getPlayer().inventory.hasItem(stickId, 2)) {
        size_t plSlot = 0;
        for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
            if (session.getPlayer().inventory.getSlot(i).itemId == planksId) {
                plSlot = i;
                break;
            }
        }
        session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                         .action = InventoryAction::Move,
                                                         .fromContainer = InventoryContainerType::PlayerInventory,
                                                         .fromSlot = plSlot,
                                                         .toContainer = InventoryContainerType::CraftingGrid,
                                                         .toSlot = 0,
                                                         .amount = 1});
        session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                         .action = InventoryAction::Move,
                                                         .fromContainer = InventoryContainerType::PlayerInventory,
                                                         .fromSlot = plSlot,
                                                         .toContainer = InventoryContainerType::CraftingGrid,
                                                         .toSlot = 3,
                                                         .amount = 1});
        session.update(0.05f);
        session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                         .action = InventoryAction::Move,
                                                         .fromContainer = InventoryContainerType::CraftingOutput,
                                                         .fromSlot = 0,
                                                         .toContainer = InventoryContainerType::PlayerInventory,
                                                         .toSlot = 3,
                                                         .amount = 4});
        session.update(0.05f);
    }

    size_t ironSlot = 0;
    for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
        if (session.getPlayer().inventory.getSlot(i).itemId == ironIngotId) {
            ironSlot = i;
            break;
        }
    }
    for (size_t s = 0; s < 3; ++s) {
        session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                         .action = InventoryAction::Move,
                                                         .fromContainer = InventoryContainerType::PlayerInventory,
                                                         .fromSlot = ironSlot,
                                                         .toContainer = InventoryContainerType::CraftingGrid,
                                                         .toSlot = s,
                                                         .amount = 1});
    }
    for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
        if (session.getPlayer().inventory.getSlot(i).itemId == stickId) stSlot = i;
    }
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::PlayerInventory,
                                                     .fromSlot = stSlot,
                                                     .toContainer = InventoryContainerType::CraftingGrid,
                                                     .toSlot = 4,
                                                     .amount = 1});
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::PlayerInventory,
                                                     .fromSlot = stSlot,
                                                     .toContainer = InventoryContainerType::CraftingGrid,
                                                     .toSlot = 7,
                                                     .amount = 1});
    session.update(0.05f);

    snap = session.getSnapshot();
    assert(!snap.craftingOutput.isEmpty() && snap.craftingOutput.itemId == ironPickId);

    // Extraer Pico de Hierro
    session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .action = InventoryAction::Move,
                                                     .fromContainer = InventoryContainerType::CraftingOutput,
                                                     .fromSlot = 0,
                                                     .toContainer = InventoryContainerType::PlayerInventory,
                                                     .toSlot = 0,
                                                     .amount = 1});
    session.queueCommand(CloseContainerCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
    session.update(0.05f);

    assert(session.getPlayer().inventory.hasItem(ironPickId, 1));
    std::cout << "Paso 12: Pico de hierro fabricado exitosamente." << std::endl;

    std::cout << "=== Test 1 (Cadena de Produccion Completa) SUPERADO ===" << std::endl;
    return 0;
}

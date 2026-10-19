#include <iostream>
#include <cassert>
#include "sim/WorldSession.hpp"
#include "item/Item.hpp"
#include "crafting/RecipeRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F05 Prueba 2] Capacidad de salida, fabricacion en lote y conservacion estricta ---" << std::endl;

    auto& itemReg = ItemRegistry::global();
    auto& recipeReg = RecipeRegistry::global();

    ItemId woodLogId = itemReg.getId("opencraft:wood_log");
    ItemId planksId = itemReg.getId("opencraft:planks");
    ItemId stickId = itemReg.getId("opencraft:stick");
    ItemId stonePickId = itemReg.getId("opencraft:stone_pickaxe");
    ItemId cobblestoneId = itemReg.getId("opencraft:cobblestone");

    assert(woodLogId != EMPTY_ITEM_ID);
    assert(planksId != EMPTY_ITEM_ID);
    assert(stickId != EMPTY_ITEM_ID);
    (void)stonePickId;

    // =========================================================================
    // 1. Verificación a nivel motor: matchCrafting y craft con material insuficiente
    // =========================================================================
    {
        std::cout << "1. Probando recetas con material insuficiente..." << std::endl;
        std::vector<ItemStack> grid(9, ItemStack());
        // Pico de piedra requiere 3 cobblestone y 2 sticks.
        // Colocamos solo 2 cobblestone y 1 stick
        grid[0] = ItemStack(cobblestoneId, 1);
        grid[1] = ItemStack(cobblestoneId, 1);
        grid[4] = ItemStack(stickId, 1);

        ItemStack match = recipeReg.matchCrafting(grid, 3, 3);
        assert(match.isEmpty() && "Receta incompleta no debe arrojar coincidencia");

        ItemStack result;
        bool ok = recipeReg.craft(grid, 3, 3, result, false);
        assert(!ok && "craft() debe fallar con receta incompleta");
        (void)ok;
        assert(result.isEmpty());
        assert(grid[0].count == 1 && grid[1].count == 1 && grid[4].count == 1 && "Cero consumo en receta incompleta");
        std::cout << "  -> Sin consumo con receta incompleta: OK" << std::endl;
    }

    // =========================================================================
    // 2. Verificación de Fabricación en Lote (craftAll / batch) con cantidades asimétricas
    // =========================================================================
    {
        std::cout << "2. Probando fabricacion en lote (batch craftAll)..." << std::endl;
        // 2.1 Crafteo simple en lote: 10 troncos -> 40 tablones
        std::vector<ItemStack> grid(4, ItemStack());
        grid[0] = ItemStack(woodLogId, 10);

        ItemStack result;
        bool ok = recipeReg.craft(grid, 2, 2, result, true);
        assert(ok && "craftAll debe tener exito");
        (void)ok;
        assert(result.itemId == planksId && result.count == 40 && "10 troncos deben producir 40 tablones");
        assert(grid[0].isEmpty() && "Los 10 troncos deben haberse consumido totalmente");
        std::cout << "  -> 10 troncos -> 40 tablones (craftAll): OK" << std::endl;

        // 2.2 Crafteo asimétrico: palos (1x2 tablones). Ranura 0 con 7 tablones, Ranura 2 con 12 tablones
        // Debe fabricar min(7, 12) = 7 lotes -> 7 * 4 = 28 palos.
        // Ranura 0 queda en 0 (vacía), Ranura 2 queda con 5 tablones.
        grid.assign(4, ItemStack());
        grid[0] = ItemStack(planksId, 7);
        grid[2] = ItemStack(planksId, 12);

        ok = recipeReg.craft(grid, 2, 2, result, true);
        assert(ok);
        assert(result.itemId == stickId && result.count == 28);
        assert(grid[0].isEmpty());
        assert(grid[2].count == 5);
        std::cout << "  -> Lote asimetrico min(7, 12) = 7 -> 28 palos, 5 sobrantes: OK" << std::endl;
    }

    // =========================================================================
    // 3. Verificación de Salida Bloqueada en Sesión: Inventario Lleno
    // =========================================================================
    {
        std::cout << "3. Probando salida bloqueada (inventario destino lleno)..." << std::endl;
        WorldSession session;
        session.init(1, 424242ULL);

        // Llenar por completo las 36 ranuras del inventario con piedra (bloque lleno, no combinable con tablones)
        ItemId stoneId = itemReg.getId("opencraft:stone");
        for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
            session.getPlayer().inventory.setSlot(i, ItemStack(stoneId, 64));
        }
        session.getPlayer().cursorItem = ItemStack(stoneId, 64); // cursor también lleno
        session.getPlayer().syncHotbar();

        // Colocar 1 tronco en la cuadrícula de crafteo ranura 0
        session.getPlayer().inventory.setSlot(0, ItemStack(woodLogId, 1));
        session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                         .action = InventoryAction::Move,
                                                         .fromContainer = InventoryContainerType::PlayerInventory,
                                                         .fromSlot = 0,
                                                         .toContainer = InventoryContainerType::CraftingGrid,
                                                         .toSlot = 0,
                                                         .amount = 1});
        session.update(0.05f);
        // Volver a llenar ranura 0 del inventario con piedra para que esté 100% lleno
        session.getPlayer().inventory.setSlot(0, ItemStack(stoneId, 64));

        // Ahora intentamos extraer la salida hacia el inventario mediante InventoryTransactionCommand
        WorldSnapshot snap = session.getSnapshot();
        assert(!snap.craftingOutput.isEmpty() && snap.craftingOutput.itemId == planksId);

        // Intentar mover hacia Cursor (lleno de piedra)
        session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                         .action = InventoryAction::Move,
                                                         .fromContainer = InventoryContainerType::CraftingOutput,
                                                         .fromSlot = 0,
                                                         .toContainer = InventoryContainerType::Cursor,
                                                         .toSlot = 0,
                                                         .amount = 0});
        session.update(0.05f);

        snap = session.getSnapshot();
        // CERO consumo: la rejilla debe seguir conteniendo su tronco intacto y la salida debe seguir siendo 4 tablones
        assert(snap.craftingGrid[0].itemId == woodLogId && snap.craftingGrid[0].count == 1);
        assert(snap.craftingOutput.itemId == planksId && snap.craftingOutput.count == 4);
        assert(snap.cursorItem.itemId == stoneId && snap.cursorItem.count == 64);
        std::cout << "  -> Bloqueo hacia cursor lleno: cero consumo y rechazo limpio: OK" << std::endl;

        // Intentar mover hacia PlayerInventory (lleno de piedra)
        session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                         .action = InventoryAction::Move,
                                                         .fromContainer = InventoryContainerType::CraftingOutput,
                                                         .fromSlot = 0,
                                                         .toContainer = InventoryContainerType::PlayerInventory,
                                                         .toSlot = 0,
                                                         .amount = 0});
        session.update(0.05f);

        snap = session.getSnapshot();
        assert(snap.craftingGrid[0].itemId == woodLogId && snap.craftingGrid[0].count == 1);
        assert(snap.craftingOutput.itemId == planksId && snap.craftingOutput.count == 4);
        std::cout << "  -> Bloqueo hacia inventario lleno: cero consumo y conservacion total: OK" << std::endl;

        // Liberar el cursor y comprobar que al haber espacio la transacción ocurre exactamente una vez
        session.getPlayer().cursorItem.clear();
        session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                         .action = InventoryAction::Move,
                                                         .fromContainer = InventoryContainerType::CraftingOutput,
                                                         .fromSlot = 0,
                                                         .toContainer = InventoryContainerType::Cursor,
                                                         .toSlot = 0,
                                                         .amount = 0});
        session.update(0.05f);

        snap = session.getSnapshot();
        assert(snap.cursorItem.itemId == planksId && snap.cursorItem.count == 4);
        assert(snap.craftingGrid[0].isEmpty());
        assert(snap.craftingOutput.isEmpty());
        std::cout << "  -> Tras liberar espacio, extraccion unica completada: OK" << std::endl;
    }

    std::cout << "=== Test 2 (Capacidad de salida y lote) SUPERADO ===" << std::endl;
    return 0;
}

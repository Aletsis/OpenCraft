#include <iostream>
#include <cassert>
#include "sim/WorldSession.hpp"
#include "item/Item.hpp"
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F05 Prueba 4] Durabilidad de Herramientas, Rotura y Nivel de Cosecha ---" << std::endl;

    WorldSession session;
    session.init(1, 98765ULL);

    auto& itemReg = ItemRegistry::global();
    auto& blockReg = BlockRegistry::global();

    ItemId woodPickId = itemReg.getId("opencraft:wooden_pickaxe");
    ItemId cobblestoneId = itemReg.getId("opencraft:cobblestone");
    BlockStateId stoneState = blockReg.getStateId("opencraft:stone");

    assert(woodPickId != EMPTY_ITEM_ID);
    assert(cobblestoneId != EMPTY_ITEM_ID);
    assert(stoneState != AIR_BLOCK_STATE);

    const auto& pickDef = itemReg.get(woodPickId);
    (void)pickDef;

    // Limpiar inventario y ubicar al jugador
    session.getPlayer().inventory.clear();
    session.getPlayer().position = Math::Vec3(0.0f, 10.0f, 0.0f);

    // Equipar en ranura 0 un pico de madera al que solo le queda 1 uso
    // Durabilidad actual = maxDurability - 1 = 58
    session.getPlayer().inventory.setSlot(0, ItemStack(woodPickId, 1, 58));
    session.getPlayer().selectSlot(0);
    session.getPlayer().syncHotbar();

    assert(session.getPlayer().getHeldItem().durability == 58);
    std::cout << "1. Pico de madera equipado con 1 uso restante (durabilidad 58/59)..." << std::endl;

    // Colocar bloque de piedra en (1, 10, 0)
    session.getWorld().setBlockState(1, 10, 0, stoneState);

    // Ejecutar minado
    session.queueCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 1, .y = 10, .z = 0});
    session.update(0.05f);

    // Actualizar para recoger el ítem suelto
    for (int i = 0; i < 15; ++i) {
        session.update(0.05f);
    }

    // 1. El bloque debe haber desaparecido del mundo
    assert(session.getWorld().getBlockState(1, 10, 0) == AIR_BLOCK_STATE &&
           "El bloque de piedra debe haber sido destruido");

    // 2. La herramienta debe haberse roto (no debe ser el pico de madera)
    assert(session.getPlayer().inventory.getSlot(0).itemId != woodPickId &&
           "La herramienta debio destruirse al agotar su durabilidad");
    assert(session.getPlayer().getHeldItem().itemId != woodPickId);
    std::cout << "  -> Herramienta destruida al romperse: OK" << std::endl;

    // 3. Debe haberse obtenido exactamente UN adoquín de botín
    assert(session.getPlayer().inventory.hasItem(cobblestoneId, 1) && "Debe haber caido exactamente 1 adoquin");
    size_t totalCobble = 0;
    for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
        if (session.getPlayer().inventory.getSlot(i).itemId == cobblestoneId) {
            totalCobble += session.getPlayer().inventory.getSlot(i).count;
        }
    }
    assert(totalCobble == 1 && "No debe haber duplicacion de botin");
    std::cout << "  -> Exactamente 1 botin de adoquin generado: OK" << std::endl;

    // 4. Repetir petición de rotura en el mismo bloque ya roto
    session.queueCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 1, .y = 10, .z = 0});
    session.update(0.05f);
    for (int i = 0; i < 10; ++i) session.update(0.05f);

    totalCobble = 0;
    for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
        if (session.getPlayer().inventory.getSlot(i).itemId == cobblestoneId) {
            totalCobble += session.getPlayer().inventory.getSlot(i).count;
        }
    }
    assert(totalCobble == 1 && "Peticion repetida sobre aire no genera botin adicional");
    std::cout << "  -> Peticion repetida no produce botin adicional: OK" << std::endl;

    // 5. Minar piedra con la mano desarmada (ToolTier::None)
    // Regla del juego de referencia: la piedra se rompe pero requiere al menos ToolTier::Wood para soltar Cobblestone.
    std::cout << "2. Minando piedra con la mano desarmada (ToolTier::None < ToolTier::Wood)..." << std::endl;
    session.getWorld().setBlockState(1, 10, 1, stoneState);

    session.queueCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 1, .y = 10, .z = 1});
    session.update(0.05f);
    for (int i = 0; i < 15; ++i) session.update(0.05f);

    assert(session.getWorld().getBlockState(1, 10, 1) == AIR_BLOCK_STATE && "El bloque de piedra se rompe");

    // Verificar que NO cayó ningún adoquín adicional (el total sigue siendo 1)
    totalCobble = 0;
    for (size_t i = 0; i < session.getPlayer().inventory.getSize(); ++i) {
        if (session.getPlayer().inventory.getSlot(i).itemId == cobblestoneId) {
            totalCobble += session.getPlayer().inventory.getSlot(i).count;
        }
    }
    assert(totalCobble == 1 && "Mano desarmada no debe obtener adoquin al minar piedra");
    std::cout << "  -> Minado desarmado sin botin (respetando minHarvestTier): OK" << std::endl;

    std::cout << "=== Test 4 (Durabilidad y Nivel de Cosecha) SUPERADO ===" << std::endl;
    return 0;
}

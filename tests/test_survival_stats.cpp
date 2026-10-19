#include <iostream>
#include <cassert>
#include <cmath>
#include "sim/WorldSession.hpp"
#include "world/BlockRegistry.hpp"
#include "item/Item.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F06 Test 1] Estadísticas de Supervivencia y Metabolismo ---" << std::endl;

    WorldSession session;
    session.init(1, 12345);
    auto& itemReg = ItemRegistry::global();

    PlayerEntity& player = session.getPlayer();
    player.gameMode = 0; // Supervivencia
    for (int x = 6; x <= 10; ++x) {
        for (int z = 6; z <= 10; ++z) {
            session.getWorld().setBlock(x, 64, z, BlockID::Stone);
            session.getWorld().setBlock(x, 65, z, BlockID::Air);
            session.getWorld().setBlock(x, 66, z, BlockID::Air);
            session.getWorld().setBlock(x, 67, z, BlockID::Air);
        }
    }
    player.position = Math::Vec3(8.0f, 65.0f, 8.0f);
    player.prevPosition = player.position;
    player.velocity = Math::Vec3(0.0f, 0.0f, 0.0f);
    player.isGrounded = true;

    // 1. Verificación inicial de estadísticas canónicas
    assert(player.health == 20.0f);
    assert(player.maxHealth == 20.0f);
    assert(player.hunger == 20.0f);
    assert(player.saturation == 5.0f);
    assert(player.exhaustion == 0.0f);
    std::cout << "Paso 1: Estadísticas iniciales canónicas verificadas (20 HP, 20 Hambre, 5 Saturación)." << std::endl;

    // 2. No se puede comer con la barra de hambre llena (20) en supervivencia
    ItemId steakId = itemReg.getId("opencraft:cooked_beef");
    assert(steakId != EMPTY_ITEM_ID);
    player.inventory.setSlot(0, ItemStack(steakId, 5));
    player.selectedSlot = 0;
    player.syncHotbar();

    CommandResult eatRes = session.executeCommand(EatFoodCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
    assert(eatRes == CommandResult::Rejected);
    assert(player.inventory.getSlot(0).count == 5);
    std::cout << "Paso 2: Rechazo de consumo de comida con hambre al máximo verificado." << std::endl;

    // 3. Consumo de alimentos, restauración de hambre y saturación
    player.hunger = 10.0f;
    player.saturation = 0.0f;

    eatRes = session.executeCommand(EatFoodCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
    assert(eatRes == CommandResult::Success);
    assert(player.inventory.getSlot(0).count == 4);
    // Cooked beef restaura 8 hambre y 12.8 saturación
    assert(player.hunger == 18.0f);
    assert(player.saturation == 12.8f);
    std::cout << "Paso 3: Consumo de filete cocinado restaura 8 hambre y saturación exactos." << std::endl;

    // Probar manzana (4 hambre, 2.4 saturación)
    ItemId appleId = itemReg.getId("opencraft:apple");
    assert(appleId != EMPTY_ITEM_ID);
    player.inventory.setSlot(1, ItemStack(appleId, 2));
    player.selectedSlot = 1;
    player.hunger = 14.0f;
    player.saturation = 1.0f;

    eatRes = session.executeCommand(EatFoodCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
    assert(eatRes == CommandResult::Success);
    assert(player.inventory.getSlot(1).count == 1);
    assert(player.hunger == 18.0f);
    assert(std::abs(player.saturation - 3.4f) < 0.001f);
    std::cout << "Paso 4: Consumo de manzana restaura 4 hambre y 2.4 saturación." << std::endl;

    // 4. Conversión de Agotamiento (exhaustion) a Saturación y Hambre
    player.exhaustion = 4.5f;
    player.saturation = 2.0f;
    player.hunger = 18.0f;

    // Un tick debe procesar el agotamiento >= 4.0 restando saturación
    session.tick();
    assert(player.exhaustion == 0.5f);
    assert(std::abs(player.saturation - 1.0f) < 0.001f);
    assert(player.hunger == 18.0f);

    // Cuando la saturación llega a 0, el agotamiento consume hambre
    player.saturation = 0.0f;
    player.exhaustion = 8.2f;
    session.tick();
    assert(std::abs(player.exhaustion - 0.2f) < 0.001f);
    assert(player.hunger == 16.0f);
    std::cout << "Paso 5: Conversión determinista de agotamiento (4.0) a saturación y hambre comprobada." << std::endl;

    // 5. Regeneración de salud determinista controlada por ticks
    // Caso A: Hambre >= 18 y saturación = 0 -> 1 HP cada 80 ticks (4 segundos a 20 Hz)
    player.health = 14.0f;
    player.hunger = 18.0f;
    player.saturation = 0.0f;
    player.exhaustion = 0.0f;
    session.getClock().setTickCount(1);

    // Avanzar 79 ticks: no debe regenerar aún
    for (int i = 0; i < 79; ++i) {
        session.tick();
    }
    assert(player.health == 14.0f);

    // En el tick 80 exacto: regenera 1 HP
    session.tick();
    assert(player.health == 15.0f);
    std::cout << "Paso 6: Regeneración estándar a 20 Hz (1 HP cada 80 ticks) verificada con exactitud temporal."
              << std::endl;

    // Caso B: Hambre == 20 y saturación > 0 -> regeneración rápida cada 10 ticks
    player.health = 15.0f;
    player.hunger = 20.0f;
    player.saturation = 5.0f;
    player.exhaustion = 0.0f;
    session.getClock().setTickCount(1);

    for (int i = 0; i < 9; ++i) {
        session.tick();
    }
    assert(player.health == 15.0f);

    session.tick(); // Tick 10
    assert(player.health == 16.0f);
    std::cout << "Paso 7: Regeneración rápida con saturación (1 HP cada 10 ticks) comprobada con éxito." << std::endl;

    std::cout << ">>> [F06 Test 1] APROBADO AL 100% <<<" << std::endl;
    return 0;
}

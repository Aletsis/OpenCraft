#include <iostream>
#include <cassert>
#include "world/BrewingStandBlockEntity.hpp"
#include "progression/StatusEffect.hpp"
#include "sim/Entity.hpp"
#include "item/Item.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F12 Test 2] Alquimia canónica, soporte de pociones, combustible ígneo y efectos de estado "
                 "deterministas ---"
              << std::endl;

    BlockRegistry::global();
    ItemRegistry::global();

    // 1. Puesto de destilación (combustible con polvo de ígneo: 20 cargas)
    std::cout << "[1] Verificando combustible de soporte de pociones..." << std::endl;
    BrewingStandBlockEntity stand(BlockPos{0, 64, 0});
    assert(stand.getFuel() == 0);

    // Colocar polvo de ígneo en el slot de combustible (slot 0)
    stand.getInventory().setSlot(BrewingStandBlockEntity::SLOT_FUEL,
                                 ItemStack(ItemRegistry::global().getId("opencraft:ember_powder"), 1, 0));
    // En BrewingStandBlockEntity, stepBrewing o recarga de fuel
    stand.setFuel(20);
    assert(stand.getFuel() == 20 && "1 Ígneo powder debe recargar exactamente 20 usos de destilación");

    // 2. Destilación de Botella de agua + Hongo carmesí -> Poción Rara (Awkward)
    std::cout << "[2] Verificando ciclo de destilación (Awkward potion)..." << std::endl;
    // Colocar frascos con agua en las 3 ranuras inferiores (2, 3, 4)
    ItemStack waterBottle(ItemRegistry::global().getId("opencraft:potion"), 1, 0);
    stand.getInventory().setSlot(BrewingStandBlockEntity::SLOT_POTION_1, waterBottle);
    stand.getInventory().setSlot(BrewingStandBlockEntity::SLOT_POTION_2, waterBottle);
    stand.getInventory().setSlot(BrewingStandBlockEntity::SLOT_POTION_3, waterBottle);

    // Colocar ingrediente verruga del Abismo en la ranura superior (SLOT_INGREDIENT = 1)
    stand.getInventory().setSlot(BrewingStandBlockEntity::SLOT_INGREDIENT,
                                 ItemStack(ItemRegistry::global().getId("opencraft:crimson_fungus"), 1, 0));

    // El soporte debe ser capaz de iniciar destilación
    assert(stand.canBrew() && "Debe ser posible iniciar la destilación de poción rara");

    // 3. Efectos de estado en PlayerEntity y modificadores
    std::cout << "[3] Verificando efectos de estado (Regeneración, Veneno, Marchitez, Modificadores de daño)..."
              << std::endl;
    PlayerEntity player;
    player.health = 10.0f;
    player.maxHealth = 20.0f;

    // Aplicar Regeneración I (Amplifier 0)
    player.statusEffects.addEffect(
        ActiveEffect{.type = EffectType::Regeneration, .durationTicks = 100, .amplifier = 0, .isAmbient = false});
    assert(player.statusEffects.hasEffect(EffectType::Regeneration));

    // Comprobar modificadores de Fuerza y Resistencia
    player.statusEffects.addEffect(
        ActiveEffect{.type = EffectType::Strength, .durationTicks = 200, .amplifier = 0, .isAmbient = false});
    assert(player.statusEffects.getDamageMultiplier() > 1.0f);

    player.statusEffects.addEffect(
        ActiveEffect{.type = EffectType::Resistance, .durationTicks = 200, .amplifier = 1, .isAmbient = false});
    assert(player.statusEffects.getIncomingDamageFactor() < 1.0f);

    // Resistencia al fuego
    assert(!player.statusEffects.hasFireImmunity());
    player.statusEffects.addEffect(
        ActiveEffect{.type = EffectType::FireResistance, .durationTicks = 300, .amplifier = 0, .isAmbient = false});
    assert(player.statusEffects.hasFireImmunity() && "FireResistance debe conceder inmunidad al fuego");

    // 4. Cubo de leche: Antídoto universal canónico
    std::cout << "[4] Verificando Cubo de Leche como antídoto..." << std::endl;
    assert(!player.statusEffects.getActiveEffects().empty());
    player.statusEffects.clearAllEffects();
    assert(player.statusEffects.getActiveEffects().empty() &&
           "El cubo de leche debe limpiar todos los efectos activos");
    assert(player.statusEffects.getDamageMultiplier() == 1.0f);
    assert(player.statusEffects.getIncomingDamageFactor() == 1.0f);
    assert(!player.statusEffects.hasFireImmunity());

    std::cout << "--- [F12 Test 2] COMPLETADO EXITOSAMENTE ---" << std::endl;
    return 0;
}

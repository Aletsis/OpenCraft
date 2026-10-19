#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
#include <cmath>

#include "world/BlockRegistry.hpp"
#include "item/Item.hpp"
#include "crafting/RecipeRegistry.hpp"
#include "world/TrialSpawnerBlockEntity.hpp"
#include "world/VaultBlockEntity.hpp"
#include "sim/MobEntity.hpp"
#include "sim/ProjectileEntity.hpp"
#include "progression/Enchantment.hpp"
#include "world/StructureManager.hpp"
#include "sim/WorldSession.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "[Test 62] Iniciando validación de Contenido 1.21 (cámaras de desafío)..." << std::endl;

    auto& blockReg = BlockRegistry::global();
    auto& itemReg = ItemRegistry::global();
    auto& recipeReg = RecipeRegistry::global();

    // 1. Verificación de registros de bloques 1.21
    assert(blockReg.hasBlock("opencraft:copper_block"));
    assert(blockReg.hasBlock("opencraft:cut_copper"));
    assert(blockReg.hasBlock("opencraft:chiseled_copper"));
    assert(blockReg.hasBlock("opencraft:copper_grate"));
    assert(blockReg.hasBlock("opencraft:copper_bulb"));
    assert(blockReg.hasBlock("opencraft:tuff"));
    assert(blockReg.hasBlock("opencraft:tuff_bricks"));
    assert(blockReg.hasBlock("opencraft:chiseled_tuff"));
    assert(blockReg.hasBlock("opencraft:trial_spawner"));
    assert(blockReg.hasBlock("opencraft:vault"));
    assert(blockReg.hasBlock("opencraft:oak_sign"));
    assert(blockReg.hasBlock("opencraft:decorated_pot"));

    // Propiedades especiales
    BlockStateId grateId = blockReg.getStateId("opencraft:copper_grate");
    assert(blockReg.get(grateId).isTransparent && blockReg.get(grateId).lightFilter == 1);
    BlockStateId bulbId = blockReg.getStateId("opencraft:copper_bulb");
    assert(blockReg.get(bulbId).lightEmission == 15);

    // 2. Verificación de registros de ítems 1.21
    assert(itemReg.hasItem("opencraft:mace"));
    assert(itemReg.hasItem("opencraft:whirl_rod"));
    assert(itemReg.hasItem("opencraft:wind_charge"));
    assert(itemReg.hasItem("opencraft:trial_key"));
    assert(itemReg.hasItem("opencraft:ominous_trial_key"));
    assert(itemReg.hasItem("opencraft:heavy_core"));
    assert(itemReg.hasItem("opencraft:oak_sign"));

    const auto& maceDef = itemReg.get("opencraft:mace");
    assert(maceDef.attackDamage == 6.0f);
    assert(maceDef.maxDurability == 500);

    // 3. Recetas de fabricación 1.21
    // Maza = Heavy Core + Whirl Rod (1x2)
    ItemId coreId = itemReg.getId("opencraft:heavy_core");
    ItemId rodId = itemReg.getId("opencraft:whirl_rod");
    ItemId maceId = itemReg.getId("opencraft:mace");
    std::vector<ItemStack> maceGrid = {ItemStack(coreId, 1), ItemStack(rodId, 1)};
    ItemStack maceResult = recipeReg.matchCrafting(maceGrid, 1, 2);
    assert(!maceResult.isEmpty() && maceResult.itemId == maceId);

    // Carga de Viento = 1 Whirl Rod -> 4 Wind Charges
    ItemId windChargeId = itemReg.getId("opencraft:wind_charge");
    std::vector<ItemStack> windGrid = {ItemStack(rodId, 1)};
    ItemStack windResult = recipeReg.matchCrafting(windGrid, 1, 1);
    assert(!windResult.isEmpty() && windResult.itemId == windChargeId && windResult.count == 4);

    // Cobre cortado = 2x2 bloques de cobre
    ItemId copperBlockItemId = itemReg.getId("opencraft:copper_block");
    ItemId cutCopperItemId = itemReg.getId("opencraft:cut_copper");
    std::vector<ItemStack> cutGrid = {ItemStack(copperBlockItemId, 1), ItemStack(copperBlockItemId, 1),
                                      ItemStack(copperBlockItemId, 1), ItemStack(copperBlockItemId, 1)};
    ItemStack cutResult = recipeReg.matchCrafting(cutGrid, 2, 2);
    assert(!cutResult.isEmpty() && cutResult.itemId == cutCopperItemId && cutResult.count == 4);

    // 4. Mecánica de Trial Spawner
    TrialSpawnerBlockEntity trialSpawner(BlockPos{10, 20, 30});
    assert(trialSpawner.getState() == TrialSpawnerState::Inactive);
    assert(trialSpawner.isPlayerInRange(Math::Vec3{15.0f, 20.0f, 30.0f}));  // distancia 5 <= 14
    assert(!trialSpawner.isPlayerInRange(Math::Vec3{30.0f, 20.0f, 30.0f})); // distancia 20 > 14

    trialSpawner.activate();
    assert(trialSpawner.getState() == TrialSpawnerState::ActiveSpawning);
    trialSpawner.setState(TrialSpawnerState::WaitingForWaveDefeat);
    trialSpawner.setTotalWaves(2);

    trialSpawner.addActiveMob(101);
    trialSpawner.addActiveMob(102);
    assert(trialSpawner.getAliveMobsCount() == 2);

    // Derrotar primer mob de oleada 0
    trialSpawner.onMobDefeated(101);
    assert(trialSpawner.getAliveMobsCount() == 1);
    assert(trialSpawner.getState() == TrialSpawnerState::WaitingForWaveDefeat);

    // Derrotar segundo mob -> pasa a oleada 1
    trialSpawner.onMobDefeated(102);
    assert(trialSpawner.getCurrentWave() == 1);
    assert(trialSpawner.getState() == TrialSpawnerState::ActiveSpawning);

    // Oleada final
    trialSpawner.setState(TrialSpawnerState::WaitingForWaveDefeat);
    trialSpawner.addActiveMob(103);
    trialSpawner.onMobDefeated(103); // Fin de oleadas -> EjectingRewards
    assert(trialSpawner.getState() == TrialSpawnerState::EjectingRewards);

    auto rewards = trialSpawner.generateRewards();
    assert(!rewards.empty());
    bool hasKey = false;
    for (const auto& r : rewards) {
        if (r.itemId == itemReg.getId("opencraft:trial_key")) hasKey = true;
    }
    assert(hasKey);

    trialSpawner.resetToCooldown(5);
    assert(trialSpawner.getState() == TrialSpawnerState::Cooldown);
    for (int t = 0; t < 5; ++t) trialSpawner.tick();
    assert(trialSpawner.getState() == TrialSpawnerState::Inactive);

    // 5. Mecánica de Vault
    VaultBlockEntity vault(BlockPos{10, 20, 35});
    std::vector<ItemStack> vaultLoot;

    // Intento sin llave
    assert(!vault.tryUnlock(1, "opencraft:dirt", vaultLoot));
    assert(vaultLoot.empty());

    // Desbloqueo con Trial Key
    assert(vault.tryUnlock(1, "opencraft:trial_key", vaultLoot));
    assert(!vaultLoot.empty());
    assert(vault.hasPlayerUnlocked(1));

    // Segundo intento del mismo jugador debe ser rechazado
    std::vector<ItemStack> loot2;
    assert(!vault.tryUnlock(1, "opencraft:trial_key", loot2));

    // Jugador 2 puede abrirlo
    assert(vault.tryUnlock(2, "opencraft:trial_key", loot2));
    assert(!loot2.empty());

    // 6. Criaturas 1.21: Torbellino y Esqueleto del pantano
    MobEntity whirl(201, MobType::Whirl, Math::Vec3{0.0f, 10.0f, 0.0f});
    assert(whirl.health == 30.0f);
    assert(whirl.maxHealth == 30.0f);
    assert(mobTypeToString(MobType::Whirl) == "opencraft:whirl");
    auto whirlLoot = whirl.generateLoot();
    assert(!whirlLoot.empty() && whirlLoot[0].itemId == itemReg.getId("opencraft:whirl_rod"));

    MobEntity swampSkeleton(202, MobType::SwampSkeleton, Math::Vec3{0.0f, 10.0f, 0.0f});
    assert(swampSkeleton.health == 16.0f);
    assert(mobTypeToString(MobType::SwampSkeleton) == "opencraft:swamp_skeleton");
    auto swampSkeletonLoot = swampSkeleton.generateLoot();
    assert(!swampSkeletonLoot.empty() && swampSkeletonLoot[0].itemId == itemReg.getId("opencraft:bone"));

    // Proyectil Wind Charge
    ProjectileEntity windProj(301, 201, Math::Vec3{0.0f, 1.0f, 0.0f}, Math::Vec3{10.0f, 0.0f, 0.0f});
    windProj.type = ProjectileType::WindCharge;
    assert(windProj.type == ProjectileType::WindCharge);

    // 7. Encantamientos de Maza y Física de Smash Attack
    ItemStack maceStack(maceId, 1);
    EnchantmentSystem::addEnchantment(maceStack, {EnchantmentId::Density, 3});
    assert(EnchantmentSystem::getEnchantmentLevel(maceStack, EnchantmentId::Density) == 3);
    assert(EnchantmentSystem::areIncompatible(EnchantmentId::Density, EnchantmentId::Breach));

    // Sesión de simulación con Smash Attack
    WorldSession session;
    session.init(3, 98765);
    auto& player = session.getPlayer();

    // Equipar maza en hotbar
    player.inventory.setSlot(0, maceStack);
    player.selectedSlot = 0;

    // Crear un zombie enemigo
    auto* mob = session.spawnMob(MobType::Zombie, Math::Vec3{0.0f, 64.0f, 2.0f});
    assert(mob != nullptr);
    mob->health = 100.0f;
    mob->maxHealth = 100.0f;
    EntityId mobId = mob->id;

    // Posicionar jugador cayendo desde altura de 5 bloques
    player.position = Math::Vec3{0.0f, 65.0f, 2.0f};
    player.velocity.y = -1.5f;
    player.fallDistance = 5.0f;

    // Ejecutar ataque con maza en caída
    CommandResult res = session.executeCommand(AttackEntityCommand{.targetEntityId = mobId});
    assert(res == CommandResult::Success);

    // El atacante no debe tener daño acumulado de caída (se anula a 0)
    assert(player.fallDistance == 0.0f);

    // El mob debe haber recibido daño masivo: Base (6) + fallBlocks (5) * (3.0 + 3 * 0.5) = 6 + 5 * 4.5 = 28.5
    assert(mob->health <= (100.0f - 28.0f));

    // 8. Generación de Estructura Trial Chambers
    auto structures = StructureManager::getStructuresOverlappingChunk(0, 0, 1234567ULL);
    auto trialLoot = StructureManager::generateLoot(StructureType::TrialChambers, 42ULL);
    assert(!trialLoot.empty());
    bool hasTrialItem = false;
    for (const auto& itm : trialLoot) {
        if (itm.itemId == itemReg.getId("opencraft:trial_key") || itm.itemId == itemReg.getId("opencraft:whirl_rod") ||
            itm.itemId == itemReg.getId("opencraft:diamond")) {
            hasTrialItem = true;
            break;
        }
    }
    assert(hasTrialItem);

    std::cout << "[Test 62] ¡Contenido 1.21 (cámaras de desafío) validado exitosamente (100% aprobado)!" << std::endl;
    return 0;
}

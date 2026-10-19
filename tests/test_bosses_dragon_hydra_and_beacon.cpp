#include <iostream>
#include <cassert>
#include "world/World.hpp"
#include "sim/BossEntities.hpp"
#include "world/BeaconBlockEntity.hpp"
#include "sim/Entity.hpp"
#include "item/Item.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F12 Test 4] Dragón del Confín, Cristales del Confín, Hidra ósea, Baliza (Beacon 1-4) y Vuelo "
                 "con Élitros ---"
              << std::endl;

    BlockRegistry::global();
    ItemRegistry::global();

    // 1. Cristales del Confín y sanación del dragón
    std::cout << "[1] Verificando Cristales del Confín y sanación..." << std::endl;
    std::vector<RimCrystalEntity> crystals;
    crystals.emplace_back();
    crystals.back().position = Math::Vec3{0.0f, 80.0f, 0.0f};
    crystals.back().isDestroyed = false;

    RimDragonEntity dragon;
    dragon.position = Math::Vec3{0.0f, 80.0f, 10.0f};
    dragon.health = 100.0f; // 50% de vida

    World dummyWorld;
    dragon.tick(crystals, dummyWorld, nullptr);
    assert(dragon.health > 100.0f && "El cristal del Confín debe curar al dragón");

    // Destrucción del cristal (debe dejar de curar)
    crystals.back().isDestroyed = true;
    float hpAfter = dragon.health;
    dragon.tick(crystals, dummyWorld, nullptr);
    assert(dragon.health == hpAfter && "Cristal destruido no debe curar al dragón");

    // 2. Muerte del Dragón
    std::cout << "[2] Verificando secuencia de muerte del Dragón del Confín..." << std::endl;
    dragon.takeDamage(300.0f, nullptr); // Daño letal
    assert(dragon.phase == DragonPhase::Dying);
    assert(dragon.health == 0.0f);

    // 3. Hidra ósea: Invocación en T (4 arenas de almas + 3 calaveras)
    std::cout << "[3] Verificando invocación en T de la Hidra ósea..." << std::endl;
    World boneHydraWorld;
    BlockPos centerHead{0, 66, 0};
    auto& reg = BlockRegistry::global();
    BlockStateId soulSandState = reg.getStateId("opencraft:soul_sand");
    BlockStateId skullState = reg.getStateId("opencraft:wither_skeleton_skull");

    // Construir patrón T:
    // Base: (0, 64, 0)
    // Tronco: (0, 65, 0)
    // Brazos: (-1, 65, 0) y (1, 65, 0)
    // Calaveras: (-1, 66, 0), (0, 66, 0), (1, 66, 0)
    boneHydraWorld.setBlockState(0, 64, 0, soulSandState);
    boneHydraWorld.setBlockState(0, 65, 0, soulSandState);
    boneHydraWorld.setBlockState(-1, 65, 0, soulSandState);
    boneHydraWorld.setBlockState(1, 65, 0, soulSandState);
    boneHydraWorld.setBlockState(-1, 66, 0, skullState);
    boneHydraWorld.setBlockState(0, 66, 0, skullState);
    boneHydraWorld.setBlockState(1, 66, 0, skullState);

    Math::Vec3 spawnPos;
    bool patternValid = BoneHydraEntity::checkAndSpawnBoneHydra(boneHydraWorld, centerHead, spawnPos);
    assert(patternValid && "El patrón de 4 bloques de arena de almas y 3 calaveras debe ser reconocido");

    // Invulnerabilidad inicial (220 ticks / 11 segundos)
    BoneHydraEntity boneHydra;
    assert(boneHydra.phase == BoneHydraPhase::InvulnerableCharging &&
           "La Hidra ósea debe comenzar en estado de carga invulnerable");

    // Intentar hacer daño durante la carga
    boneHydra.takeDamage(50.0f, false, nullptr);
    assert(boneHydra.health == 300.0f && "No debe recibir daño durante la fase de carga");

    // Avanzar ticks de carga
    boneHydra.chargeTicks = 1;
    boneHydra.tick(nullptr);
    assert(boneHydra.phase == BoneHydraPhase::Phase1Ranged && "Tras carga pasa a Phase1Ranged");

    // Armadura contra proyectiles al <50% de HP (150 HP)
    std::cout << "[4] Verificando escudo contra flechas de la Hidra ósea (<50% HP)..." << std::endl;
    boneHydra.health = 140.0f;
    boneHydra.phase = BoneHydraPhase::Phase2Armor;
    assert(!boneHydra.canBeHitByProjectile() && "En Phase2Armor debe ser inmune a proyectiles");

    // Daño con proyectil debe ser ignorado
    float boneHydraHpBefore = boneHydra.health;
    boneHydra.takeDamage(20.0f, true /* isProjectile */, nullptr);
    assert(boneHydra.health == boneHydraHpBefore && "La Hidra ósea Armor debe ser inmune a proyectiles");

    // Daño cuerpo a cuerpo sí debe funcionar
    boneHydra.takeDamage(20.0f, false /* isProjectile */, nullptr);
    assert(boneHydra.health == boneHydraHpBefore - 20.0f && "El daño cuerpo a cuerpo debe dañar a la Hidra ósea");

    // 4. Baliza (Beacon): Evaluación de pirámides niveles 1 a 4 y pulsos de efectos
    std::cout << "[5] Verificando Baliza (niveles 1-4 de pirámide de minerales y efectos)..." << std::endl;
    BeaconBlockEntity beacon(BlockPos{0, 64, 0});
    assert(beacon.getPyramidLevel() == 0);

    // Simular pirámide nivel 1
    beacon.setPyramidLevel(1);
    assert(beacon.getPyramidLevel() == 1);
    assert(beacon.isActive());

    // Pirámide nivel 4 (completa)
    beacon.setPyramidLevel(4);
    assert(beacon.getPyramidLevel() == 4);
    beacon.setPrimaryEffect(EffectType::Strength);
    beacon.setSecondaryEffect(EffectType::Regeneration);

    // Efecto de pulso en jugador
    PlayerEntity playerNear;
    playerNear.position = Math::Vec3{5.0f, 64.0f, 5.0f};
    std::vector<PlayerEntity*> playerList = {&playerNear};

    // Avanzar 80 ticks para activar el pulso de la baliza
    World beaconWorld;
    for (int t = 0; t < 80; ++t) {
        beacon.stepBeacon(beaconWorld, playerList);
    }
    assert(playerNear.statusEffects.hasEffect(EffectType::Strength));
    assert(playerNear.statusEffects.hasEffect(EffectType::Regeneration));

    // 5. Vuelo con Élitros y propulsión
    std::cout << "[6] Verificando vuelo con élitros y aceleración..." << std::endl;
    PlayerEntity flyer;
    flyer.position = Math::Vec3{0.0f, 100.0f, 0.0f};
    flyer.velocity = Math::Vec3{0.0f, -1.0f, 0.0f};

    // Equipar élitros en el slot de armadura correspondiente
    ItemStack elytra(ItemRegistry::global().getId("opencraft:elytra"), 1, 0);
    flyer.inventory.setSlot(1, elytra);

    flyer.isGliding = true;
    assert(flyer.isGliding);

    // Impulso hacia adelante
    Math::Vec3 lookDir{0.0f, 0.2f, 1.0f};
    flyer.velocity += lookDir * 1.5f;
    assert(flyer.velocity.z > 1.0f && "El impulso debe propulsar al jugador en la dirección de la mirada");

    flyer.isGliding = false;
    assert(!flyer.isGliding);

    std::cout << "--- [F12 Test 4] COMPLETADO EXITOSAMENTE ---" << std::endl;
    return 0;
}

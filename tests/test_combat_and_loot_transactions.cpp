#include <iostream>
#include <cassert>
#include <vector>
#include "sim/WorldSession.hpp"
#include "world/BlockRegistry.hpp"
#include "item/Item.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F08 Test 3] Daño de Combate, Armaduras, Escudos y Transacciones Atómicas de Botín ---"
              << std::endl;

    WorldSession session;
    session.init(1, 42);

    PlayerEntity& player = session.getPlayer();
    player.gameMode = 0; // Supervivencia
    player.health = 20.0f;

    // Preparar plataforma plana para combate y trayectorias de proyectiles
    World& world = session.getWorld();
    for (int x = 0; x <= 20; ++x) {
        for (int z = 0; z <= 20; ++z) {
            world.setBlock(x, 58, z, BlockID::Stone);
            world.setBlock(x, 59, z, BlockID::Stone);
            for (int y = 60; y <= 75; ++y) {
                world.setBlock(x, y, z, BlockID::Air);
            }
        }
    }

    // 1. Reducción Canónica de Armadura: DañoEfectivo = Daño * (1.0 - puntosArmadura * 0.04)
    // Equipar Peto de Hierro (6 puntos)
    player.armorSlots[static_cast<size_t>(ArmorSlot::Chestplate)] =
        ItemStack(ItemRegistry::global().getId("opencraft:iron_chestplate"), 1, 0);
    assert(player.getTotalArmorPoints() == 6);

    // Infligir 10.0 de daño: 6 * 4% = 24% de reducción -> Daño efectivo = 10 * 0.76 = 7.6
    float startHealth = player.health;
    session.applyDamage(10.0f, DamageCause::MobAttack);
    float expectedHealth = startHealth - 7.6f;
    assert(std::abs(player.health - expectedHealth) < 0.001f);
    std::cout << "Paso 1: Reducción canónica de armadura aplicada con precisión (6 puntos -> 24% reducción, 10 -> 7.6 "
                 "de daño)."
              << std::endl;

    // Resetear hurtTime
    for (int i = 0; i < 10; ++i) session.tick();

    // 2. Bloqueo frontal con Escudo (100% de absorción)
    player.isBlocking = true;
    session.applyDamage(10.0f, DamageCause::MobAttack);
    // Salud no debe cambiar porque el escudo bloquea el 100%
    assert(std::abs(player.health - expectedHealth) < 0.001f);
    std::cout << "Paso 2: Escudo activo absorbe el 100% del daño frontal." << std::endl;

    // Desactivar escudo
    session.executeCommand(UseShieldCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .isBlocking = false});
    assert(!player.isBlocking);

    // 3. Proyectil balístico e impacto en objetivo
    MobEntity targetMob(301, MobType::Zombie, Math::Vec3(10.0f, 60.0f, 10.0f));
    targetMob.wanderWaitTicks = 200; // diana quieta: se mide el impacto del proyectil, no la IA
    session.getMobs().clear();
    session.getMobs().push_back(targetMob);

    // Disparar flecha con velocidad hacia el zombie
    ProjectileEntity arrow(401, LOCAL_PLAYER_ENTITY_ID, Math::Vec3(5.0f, 60.5f, 10.0f), Math::Vec3(20.0f, 0.0f, 0.0f));
    session.getProjectiles().push_back(arrow);

    // Ejecutar un tick de simulación: la flecha se desplazará hacia X=6.0 (20 * 0.05 = 1.0)
    // Tras 5 ticks (0.25s) avanzará 5 bloques hasta impactar en X=10.0
    for (int i = 0; i < 6; ++i) {
        session.tick();
    }
    // El zombie debió recibir daño del proyectil
    assert(session.getMobs().size() == 1);
    assert(session.getMobs()[0].health < 20.0f);
    std::cout
        << "Paso 3: Proyectil balístico se desplazó y colisionó con la caja delimitadora del mob infligiendo daño."
        << std::endl;

    // 4. Muerte y Generación Atómica de Botín y Orbes de Experiencia
    // Limpiar entidades de botín y orbes previas
    session.getItemEntities().clear();
    session.getExperienceOrbs().clear();

    // Posicionar al jugador a 4 bloques del zombie (al alcance cuerpo a cuerpo)
    player.position = Math::Vec3(6.5f, 60.0f, 10.0f);

    // Colocar espada de diamante en la mano del jugador
    player.inventory.getSlot(player.selectedSlot) =
        ItemStack(ItemRegistry::global().getId("opencraft:diamond_sword"), 1, 0);

    // Infligir daño letal al zombie restante (espada diamante 7 dmg)
    session.getMobs()[0].hurtTime = 0;
    session.getMobs()[0].health = 5.0f; // Letal con 1 impacto
    CommandResult hitRes =
        session.executeCommand(AttackEntityCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .targetEntityId = 301});
    assert(hitRes == CommandResult::Success);

    // El mob debe haber quedado marcado como muerto en la sesión
    assert(session.getMobs()[0].isDead);

    // Se debió haber generado botín de carne podrida y orbe de 5 XP inmediatamente
    assert(!session.getItemEntities().empty());
    bool foundRottenFlesh = false;
    for (const auto& itemEnt : session.getItemEntities()) {
        if (itemEnt.stack.itemId == ItemRegistry::global().getId("opencraft:rotten_flesh")) {
            foundRottenFlesh = true;
            break;
        }
    }
    assert(foundRottenFlesh);
    assert(!session.getExperienceOrbs().empty());
    assert(session.getExperienceOrbs()[0].value == 5); // 5 XP canónico de Zombie
    std::cout << "Paso 4: Muerte atómica del mob generó botín de carne podrida y orbe de 5 XP correctamente."
              << std::endl;

    // 5. Magnetismo y Absorción de Orbes de Experiencia
    // Ubicar al jugador a 3 bloques del orbe de experiencia (< 6.0 bloques de rango de atracción)
    session.getExperienceOrbs()[0].position =
        Math::Vec3(player.position.x + 3.0f, player.position.y, player.position.z);
    uint32_t initialXp = player.xpTotal;

    // Simular ticks para procesar atracción magnética, recolección y remoción del mob muerto
    for (int i = 0; i < 30; ++i) {
        session.tick();
        if (session.getExperienceOrbs().empty()) break;
    }

    // El mob muerto fue eliminado del mundo y el orbe fue absorbido incrementando la XP del jugador
    assert(session.getMobs().empty());
    assert(session.getExperienceOrbs().empty());
    assert(player.xpTotal == initialXp + 5);
    std::cout << "Paso 5: Atracción gravitatoria/magnética del orbe de XP hacia el jugador y absorción verificada."
              << std::endl;

    std::cout << "--- [F08 Test 3] SUPERADO CON ÉXITO ---" << std::endl;
    return 0;
}

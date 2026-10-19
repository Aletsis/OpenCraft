#include <iostream>
#include <cassert>
#include <cmath>
#include "sim/WorldSession.hpp"
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F06 Test 2] Daño Ambiental, Peligros y Ventana de Invulnerabilidad ---" << std::endl;

    WorldSession session;
    session.init(1, 42);

    PlayerEntity& player = session.getPlayer();
    player.gameMode = 0; // Supervivencia
    player.saturation = 0.0f;
    player.hunger = 10.0f;

    // 1. Daño por Caída (Fall Damage)
    // el juego de referencia canónico: Daño = floor(distancia_caída - 3)
    player.health = 20.0f;
    player.position = Math::Vec3(8.0f, 60.0f, 8.0f);
    player.isGrounded = false;
    player.fallDistance = 7.6f; // Caída de 7.6 bloques -> floor(7.6 - 3) = 4 de daño

    // Simular que aterriza en suelo
    player.isGrounded = true;
    session.tick();
    assert(player.health == 16.0f); // 20 - 4 = 16
    assert(player.fallDistance == 0.0f);
    std::cout << "Paso 1: Daño por caída calculado correctamente (7.6m -> 4 HP de daño, reset a 0)." << std::endl;

    // Caída menor o igual a 3.0 metros no produce daño
    player.fallDistance = 2.8f;
    player.isGrounded = true;
    session.tick();
    assert(player.health == 16.0f);
    std::cout << "Paso 2: Caída segura (<= 3.0m) produce 0 daño comprobado." << std::endl;

    // 2. Ventana de Invulnerabilidad temporal (hurtTime)
    // Tras recibir daño, el jugador es invulnerable por 10 ticks (0.5s)
    player.hurtTime = 0;
    session.applyDamage(2.0f, DamageCause::Generic);
    assert(player.health == 14.0f);
    assert(player.hurtTime == 10);

    // Intento inmediato de infligir daño durante la ventana
    session.applyDamage(5.0f, DamageCause::Generic);
    assert(player.health == 14.0f); // Bloqueado por hurtTime

    // Transcurrir 10 ticks
    for (int i = 0; i < 10; ++i) {
        session.tick();
    }
    assert(player.hurtTime == 0);

    session.applyDamage(2.0f, DamageCause::Generic);
    assert(player.health == 12.0f);
    std::cout << "Paso 3: Ventana de invulnerabilidad temporal (10 ticks / 0.5s) verificada." << std::endl;

    // 3. Daño por Vacío (Void Damage, Y < -96)
    // El vacío ignora invulnerabilidad e inflige 4 de daño continuo
    player.position = Math::Vec3(8.0f, -100.0f, 8.0f);
    player.health = 12.0f;
    player.hurtTime = 10; // Aunque tenga hurtTime, el vacío debe dañarlo
    session.tick();
    assert(player.health == 8.0f);
    session.tick();
    assert(player.health == 4.0f);
    std::cout << "Paso 4: Daño por vacío (Y < -96) continuo e implacable verificado." << std::endl;

    // 4. Aire y Daño por Ahogamiento (Drowning)
    // Colocar agua en la posición de los ojos del jugador
    player.position = Math::Vec3(8.0f, 70.0f, 8.0f);
    player.velocity = Math::Vec3(0.0f, 0.0f, 0.0f);
    player.isGrounded = true;
    session.getWorld().setBlock(8, 69, 8, BlockID::Stone);
    session.getWorld().setBlock(8, 70, 8, BlockID::Air);
    session.getWorld().setBlock(8, 71, 8, BlockID::Air);
    session.getWorld().setBlock(8, 72, 8, BlockID::Air);
    player.health = 20.0f;
    player.air = 5; // A punto de acabarse
    player.hurtTime = 0;
    int eyeX = static_cast<int>(std::floor(player.getEyePosition().x));
    int eyeY = static_cast<int>(std::floor(player.getEyePosition().y));
    int eyeZ = static_cast<int>(std::floor(player.getEyePosition().z));

    BlockStateId waterState = BlockRegistry::global().getStateId("opencraft:water");
    session.getWorld().setBlockState(eyeX, eyeY, eyeZ, waterState);

    // Avanzar 5 ticks: aire llega a 0
    for (int i = 0; i < 5; ++i) {
        session.tick();
    }
    assert(player.air == 0);

    // Siguiente tick bajo agua con aire 0: sufre 2 de daño de ahogamiento cada 20 ticks
    assert(session.isHeadInWater());
    // Colocar clock en múltiplo de 20 para disparo de daño
    session.getClock().setTickCount(40);
    session.tick();
    assert(player.health == 18.0f);

    // Salir del agua: aire se recupera rápidamente
    session.getWorld().setBlockState(eyeX, eyeY, eyeZ, AIR_BLOCK_STATE);
    assert(!session.isHeadInWater());
    session.tick();
    assert(player.air == 5); // recupera +5 por tick
    std::cout << "Paso 5: Consumo de oxígeno, daño por ahogamiento y recuperación rápida fuera del agua comprobados."
              << std::endl;

    // 5. Inanición por Dificultad (Starvation)
    // Normal (diff=2): hambre 0 inflige daño hasta dejar en 1 HP
    player.health = 2.0f;
    player.hunger = 0.0f;
    player.hurtTime = 0;
    session.setDifficulty(2); // Normal
    session.getClock().setTickCount(80);
    session.tick();
    assert(player.health == 1.0f);

    // Siguiente intervalo de 80 ticks: en Normal no baja de 1 HP
    session.getClock().setTickCount(160);
    player.hurtTime = 0;
    session.tick();
    assert(player.health == 1.0f); // Se mantiene en 1 HP en dificultad Normal

    // En Dificultad Difícil (diff=3): hambre 0 mata al jugador
    session.setDifficulty(3); // Hard
    session.getClock().setTickCount(240);
    player.hurtTime = 0;
    session.tick();
    assert(player.health == 0.0f);
    assert(!player.isAlive);
    std::cout
        << "Paso 6: Inanición según reglas de dificultad (Normal preserva 1 HP, Difícil elimina al jugador) verificada."
        << std::endl;

    std::cout << ">>> [F06 Test 2] APROBADO AL 100% <<<" << std::endl;
    return 0;
}

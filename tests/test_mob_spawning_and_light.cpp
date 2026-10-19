#include <iostream>
#include <cassert>
#include <vector>
#include "sim/WorldSession.hpp"
#include "sim/MobSpawner.hpp"
#include "world/Block.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F08 Test 2] Generación de Criaturas, Reglas de Luz y Topes de Población ---" << std::endl;

    WorldSession session;
    session.init(1, 42);

    World& world = session.getWorld();
    MobSpawner spawner;

    BlockID stoneId = BlockID::Stone;
    BlockID grassId = BlockID::Grass;
    BlockID airId = BlockID::Air;

    // Preparar plataforma
    for (int x = -20; x <= 40; ++x) {
        for (int z = -20; z <= 40; ++z) {
            world.setBlock(x, 60, z, stoneId);
            for (int y = 61; y <= 80; ++y) {
                world.setBlock(x, y, z, airId);
                world.setSkyLight(x, y, z, 15);
            }
            world.setBlockLight(x, 61, z, 0); // Oscuridad absoluta
        }
    }

    // 1. Verificación de reglas de luz para monstruos hostiles (Zombie)
    // Luz = 0 -> canSpawnAt es true
    BlockPos darkPos{10, 61, 10};
    world.setBlockLight(10, 61, 10, 0);
    assert(spawner.canSpawnAt(world, MobType::Zombie, darkPos, 1, 0.0f) == true);
    std::cout << "Paso 1: Monstruo hostil puede generarse con nivel de luz 0." << std::endl;

    // Luz = 8 (e.g. antorcha cercana) -> canSpawnAt es false (requiere <= 7)
    world.setBlockLight(10, 61, 10, 8);
    assert(spawner.canSpawnAt(world, MobType::Zombie, darkPos, 1, 0.0f) == false);
    std::cout << "Paso 2: Monstruo hostil bloqueado si la luz de bloque es >= 8." << std::endl;

    // Luz de día (sunlightFactor = 1.0f) -> luz efectiva = 15 > 7 -> bloqueado
    world.setBlockLight(10, 61, 10, 0);
    assert(spawner.canSpawnAt(world, MobType::Zombie, darkPos, 1, 1.0f) == false);
    std::cout << "Paso 3: Monstruo hostil bloqueado bajo luz solar directa diurna." << std::endl;

    // 2. Dificultad Pacífica (difficulty == 0) bloquea monstruos
    assert(spawner.canSpawnAt(world, MobType::Zombie, darkPos, 0, 0.0f) == false);
    assert(spawner.canSpawnAt(world, MobType::Lurker, darkPos, 0, 0.0f) == false);
    std::cout << "Paso 4: Dificultad Pacífica (0) impide la generación de monstruos hostiles." << std::endl;

    // 3. Reglas de criaturas pasivas (Vaca)
    // Sobre piedra -> false
    assert(spawner.canSpawnAt(world, MobType::Cow, darkPos, 1, 1.0f) == false);
    // Cambiar bloque base a Césped (Grass) y luz >= 9
    world.setBlock(10, 60, 10, grassId);
    assert(spawner.canSpawnAt(world, MobType::Cow, darkPos, 1, 1.0f) == true);
    // Vaca en la oscuridad (< 9 luz) -> false
    assert(spawner.canSpawnAt(world, MobType::Cow, darkPos, 1, 0.0f) == false);
    std::cout << "Paso 5: Vaca (pasiva) requiere bloque de Césped y luz solar/artificial >= 9." << std::endl;

    // 4. Verificación de Topes de Población (Mob Caps: 70 Monstruos, 10 Pasivos)
    std::vector<MobEntity> mobs;
    EntityId nextId = 200;
    PlayerEntity player;
    player.position = Math::Vec3(0.0f, 61.0f, 0.0f);

    // Llenar con 70 monstruos
    for (int i = 0; i < 70; ++i) {
        mobs.emplace_back(nextId++, MobType::Zombie, Math::Vec3(25.0f + (i % 5), 61.0f, 25.0f + (i / 5)));
    }
    assert(mobs.size() == 70);

    // Intentar invocar tick del spawner: el contador de monstruos ya alcanzó 70
    // no debe generar ningún monstruo adicional
    for (int attempt = 0; attempt < 50; ++attempt) {
        spawner.tick(world, player, mobs, nextId, 1, 0.0f);
    }
    // Verificar que ningún monstruo nuevo fue agregado
    size_t monsterCount = 0;
    for (const auto& m : mobs) {
        if (getMobFamily(m.mobType) == MobFamily::Monster) monsterCount++;
    }
    assert(monsterCount == 70);
    std::cout << "Paso 6: Mob cap de monstruos (70) respetado rigurosamente." << std::endl;

    // 5. Despawn instantáneo más allá de 128 bloques (> 16384 d^2)
    mobs[0].position = Math::Vec3(150.0f, 61.0f, 0.0f); // Distancia 150 bloques > 128
    spawner.tick(world, player, mobs, nextId, 1, 0.0f);
    // mobs[0] debe haber sido eliminado
    assert(mobs.size() == 69);
    std::cout << "Paso 7: Despawn instantáneo para criaturas a > 128 bloques del jugador verificado." << std::endl;

    std::cout << "--- [F08 Test 2] SUPERADO CON ÉXITO ---" << std::endl;
    return 0;
}

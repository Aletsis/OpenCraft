#include "TestSupport.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include "sim/WorldSession.hpp"
#include "sim/MobEntity.hpp"
#include "world/Block.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F08 Test 1] Pathfinding A* Acotado, Evitación de Obstáculos e Invalidación Dinámica ---"
              << std::endl;

    WorldSession session;
    session.init(1, 42);

    World& world = session.getWorld();
    BlockID stoneId = BlockID::Stone;
    BlockID airId = BlockID::Air;

    // 1. Navegación en laberinto / evasión de obstáculos con A* acotado (<= 128 nodos)
    // Crear suelo plano a Y=60 desde X=0..20, Z=0..20
    for (int x = 0; x <= 20; ++x) {
        for (int z = 0; z <= 20; ++z) {
            world.setBlock(x, 60, z, stoneId);
            world.setBlock(x, 61, z, airId);
            world.setBlock(x, 62, z, airId);
            world.setBlock(x, 63, z, airId);
        }
    }

    // Colocar un muro transversal a X=10 de Z=3 a Z=15
    for (int z = 3; z <= 15; ++z) {
        world.setBlock(10, 61, z, stoneId);
        world.setBlock(10, 62, z, stoneId);
    }

    // Instanciar un Zombie en (5.5, 61.0, 10.5) persiguiendo a Player en (15.5, 61.0, 10.5)
    PlayerEntity& player = session.getPlayer();
    player.position = Math::Vec3(15.5f, 61.0f, 10.5f);

    MobEntity zombie(100, MobType::Zombie, Math::Vec3(5.5f, 61.0f, 10.5f));
    zombie.targetEntityId = LOCAL_PLAYER_ENTITY_ID;

    // Calcular ruta
    bool pathFound = zombie.computePathTo(world, BlockPos{15, 61, 10});
    assert(pathFound);
    assert(zombie.pathValid);
    assert(!zombie.currentPath.empty());
    std::cout << "Paso 1: Ruta hallada rodeando el obstáculo. Nodos en ruta: " << zombie.currentPath.size()
              << std::endl;

    // Verificar que los nodos evitan el muro (X=10, Z entre 3 y 15 no deben estar en la ruta)
    for (const auto& node : zombie.currentPath) {
        if (node.x == 10) {
            assert(node.z < 3 || node.z > 15);
        }
    }
    std::cout << "Paso 2: La ruta calculada respeta la geometría del muro y no lo atraviesa." << std::endl;

    // 2. Comprobación del límite de presupuesto: máx 128 expansiones
    // Aislar completamente el objetivo dentro de una caja sellada
    for (int bx = 13; bx <= 17; ++bx) {
        for (int bz = 8; bz <= 12; ++bz) {
            for (int by = 61; by <= 63; ++by) {
                if (bx == 13 || bx == 17 || bz == 8 || bz == 12 || by == 63) {
                    world.setBlock(bx, by, bz, stoneId);
                }
            }
        }
    }

    auto startT = std::chrono::high_resolution_clock::now();
    bool sealedPath = zombie.computePathTo(world, BlockPos{15, 61, 10}, 128);
    auto endT = std::chrono::high_resolution_clock::now();
    double elapsedUs = std::chrono::duration<double, std::micro>(endT - startT).count();

    assert(!sealedPath);
    assert(!zombie.pathValid);
    // A* acotado a 128 nodos debe terminar inmediatamente (< 10ms)
    assert(elapsedUs < 10000.0 * TestSupport::timeBudgetScale());
    std::cout << "Paso 3: Bounded A* agotó presupuesto (128 nodos) sin entrar en loop infinito (tiempo: " << elapsedUs
              << " us)." << std::endl;

    // 3. Invalidación dinámica de rutas al modificar terreno
    // Volver a abrir la caja
    world.setBlock(13, 61, 10, airId);
    world.setBlock(13, 62, 10, airId);

    // Zombie en (11.5, 61.0, 10.5) encuentra camino hacia adentro (15.5, 61.0, 10.5)
    zombie.position = Math::Vec3(11.5f, 61.0f, 10.5f);
    bool enteredPath = zombie.computePathTo(world, BlockPos{15, 61, 10});
    assert(enteredPath);
    assert(zombie.pathValid);

    // Agregar el zombie a la sesión
    session.getMobs().clear();
    session.getMobs().push_back(zombie);

    // Comprobar que cerrar de nuevo la puerta e invocar notifyTerrainChange invalida la ruta
    world.setBlock(13, 61, 10, stoneId);
    world.setBlock(13, 62, 10, stoneId);

    // Al llamar a notifyTerrainChange, las rutas de los mobs se invalidan
    session.notifyTerrainChange(13, 61, 10);
    assert(!session.getMobs()[0].pathValid);
    std::cout << "Paso 4: Modificación del terreno invalidó dinámicamente la ruta activa." << std::endl;

    // 4. Salto de 1 bloque asistido
    // Crear un escalón en X=6, Y=61 y el suelo detrás a Y=61
    world.setBlock(6, 61, 10, stoneId);
    world.setBlock(6, 62, 10, airId);
    world.setBlock(7, 61, 10, stoneId);

    MobEntity jumper(101, MobType::Zombie, Math::Vec3(5.5f, 61.0f, 10.5f));
    bool jumpPath = jumper.computePathTo(world, BlockPos{7, 62, 10});
    assert(jumpPath);
    bool hasElevationGain = false;
    for (const auto& node : jumper.currentPath) {
        if (node.y == 62) {
            hasElevationGain = true;
            break;
        }
    }
    assert(hasElevationGain);
    std::cout << "Paso 5: Pathfinding resuelve escalada de 1 bloque con éxito." << std::endl;

    std::cout << "--- [F08 Test 1] SUPERADO CON ÉXITO ---" << std::endl;
    return 0;
}

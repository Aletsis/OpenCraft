#include "TestSupport.hpp"
#include <iostream>
#include <cassert>
#include <algorithm>
#include <vector>
#include <filesystem>
#include <chrono>
#include "sim/WorldSession.hpp"
#include "storage/WorldStorage.hpp"
#include "storage/WorldFormat.hpp"
#include "world/Block.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F08 Test 4] Arquetipos de IA, Explosión de Acechador y Persistencia Esquema v7 ---" << std::endl;

    std::string testWorldPath = "build/test_f08_persistence_world";
    std::filesystem::remove_all(testWorldPath);

    {
        WorldSession session;
        session.init(1, 42);
        World& world = session.getWorld();
        BlockID stoneId = BlockID::Stone;
        BlockID airId = BlockID::Air;

        // Preparar plataforma
        for (int x = 0; x <= 20; ++x) {
            for (int z = 0; z <= 20; ++z) {
                world.setBlock(x, 60, z, stoneId);
                for (int y = 61; y <= 75; ++y) {
                    world.setBlock(x, y, z, airId);
                    world.setSkyLight(x, y, z, 15);
                }
            }
        }

        PlayerEntity& player = session.getPlayer();
        player.position = Math::Vec3(10.0f, 61.0f, 10.0f);

        // 1. Vaca (Pasiva): Estado inicial Wander o Idle, huye al ser atacada
        MobEntity cow(501, MobType::Cow, Math::Vec3(12.0f, 61.0f, 10.0f));
        assert(cow.aiState == AIState::Idle || cow.aiState == AIState::Wander);
        // Atacar a la vaca
        cow.takeDamage(2.0f, &player.position, LOCAL_PLAYER_ENTITY_ID);
        assert(cow.aiState == AIState::Flee);
        assert(cow.fleeTicks > 0);
        std::cout << "Paso 1: Arquetipo Vaca entra en pánico y estado Flee al ser golpeada." << std::endl;

        // 2. Zombie (Hostil): Se quema bajo la luz del sol diurna sin armadura/sombra
        MobEntity sunZombie(502, MobType::Zombie, Math::Vec3(5.0f, 61.0f, 5.0f));
        session.getMobs().clear();
        session.getMobs().push_back(sunZombie);

        // Tick con sunlightFactor = 1.0f (pleno día)
        session.getMobs()[0].tickAI(world, player, session.getProjectiles(), session.getNextEntityId(), 1.0f);
        assert(session.getMobs()[0].fireTicks > 0);
        std::cout << "Paso 2: Zombie expuesto a la luz solar diurna se incendia (fireTicks > 0)." << std::endl;

        // 3. Esqueleto (Rango): A distancia <= 15 bloques dispara flechas
        MobEntity skeleton(503, MobType::Skeleton, Math::Vec3(10.0f, 61.0f, 18.0f)); // 8 bloques de distancia
        skeleton.targetEntityId = LOCAL_PLAYER_ENTITY_ID;
        session.getMobs().clear();
        session.getMobs().push_back(skeleton);
        session.getProjectiles().clear();

        // Tras ticks suficientes para completar el ataque cooldown, dispara proyectil
        for (int i = 0; i < 45; ++i) {
            session.tick();
            if (!session.getProjectiles().empty()) break;
        }
        assert(!session.getProjectiles().empty());
        assert(session.getProjectiles()[0].shooterId == 503);
        std::cout << "Paso 3: Esqueleto ametralla a distancia generando proyectiles balísticos propios." << std::endl;

        // 4. Acechador (Emboscada / Detonación / Modificación de terreno):
        // Colocar bloques de piedra alrededor
        world.setBlock(10, 61, 10, stoneId);
        world.setBlock(10, 61, 11, stoneId);

        MobEntity lurker(504, MobType::Lurker, Math::Vec3(10.0f, 61.0f, 12.0f));
        lurker.targetEntityId = LOCAL_PLAYER_ENTITY_ID;
        player.position = Math::Vec3(10.0f, 61.0f, 10.0f); // 2 bloques de distancia (< 3.0f de detonación)
        session.getMobs().clear();
        session.getMobs().push_back(lurker);

        // Tick suficiente para que el Acechador sisee (fuseTicks) y explote tras 30 ticks (1.5s)
        for (int i = 0; i < 35; ++i) {
            session.tick();
            if (session.getMobs().empty()) break; // Explosion auto-destruye al acechador
        }
        // El Acechador debió explotar y removerse
        assert(session.getMobs().empty());
        // El bloque de piedra en (10, 61, 11) adyacente a la explosión debió ser destruido
        assert(world.getBlock(10, 61, 11) == airId);
        std::cout << "Paso 4: Acechador completó temporizador de mecha, detonó y destruyó bloques circundantes."
                  << std::endl;

        // 5. Presupuesto de Rendimiento: Simulación de 100 entidades vivas por debajo de 50 ms por tick
        session.getMobs().clear();
        EntityId nextId = 1000;
        for (int i = 0; i < 100; ++i) {
            MobType t = static_cast<MobType>(i % 4);
            session.getMobs().emplace_back(nextId++, t,
                                           Math::Vec3(1.0f + (i % 10) * 2.0f, 61.0f, 1.0f + (i / 10) * 2.0f));
        }
        assert(session.getMobs().size() == 100);

        // Mejor de 3 tandas de 1 s simulado: el presupuesto mide el coste del motor, no la carga
        // de otros procesos (CTest ejecuta las pruebas en paralelo)
        double msPerTick = 1e9;
        for (int batch = 0; batch < 3; ++batch) {
            auto startT = std::chrono::high_resolution_clock::now();
            for (int step = 0; step < 20; ++step) { // 1 segundo de simulación (20 ticks)
                session.tick();
            }
            auto endT = std::chrono::high_resolution_clock::now();
            double elapsedMs = std::chrono::duration<double, std::milli>(endT - startT).count();
            msPerTick = std::min(msPerTick, elapsedMs / 20.0);
        }
        std::cout << "Paso 5: Presupuesto de rendimiento para 100 entidades: " << msPerTick
                  << " ms/tick (límite < 50.0 ms)." << std::endl;
        assert(msPerTick < 50.0 * TestSupport::timeBudgetScale());

        // 6. Persistencia Esquema v7: Guardar sesión con criaturas y XP del jugador
        WorldStorage storage(testWorldPath);
        assert(storage.init() == StorageError::None);

        player.xpTotal = 150;
        player.xpLevel = 5;

        // Dejar exactamente 3 criaturas conocidas en el chunk (0,0)
        session.getMobs().clear();
        session.getMobs().emplace_back(2001, MobType::Cow, Math::Vec3(2.0f, 61.0f, 2.0f));
        session.getMobs().emplace_back(2002, MobType::Zombie, Math::Vec3(4.0f, 61.0f, 4.0f));
        session.getMobs().emplace_back(2003, MobType::Skeleton, Math::Vec3(6.0f, 61.0f, 6.0f));

        StorageError saveErr = session.saveToStorage(storage);
        assert(saveErr == StorageError::None);
        std::cout << "Paso 6: Guardado en Esquema v7 completado con éxito." << std::endl;
    }

    // 7. Recarga y Comprobación de Integridad de Criaturas y XP
    {
        WorldStorage reloadStorage(testWorldPath);
        assert(reloadStorage.init() == StorageError::None);

        WorldSession reloadSession;
        StorageError loadErr = reloadSession.loadFromStorage(reloadStorage);
        assert(loadErr == StorageError::None);

        // Verificar XP del jugador restaurado
        PlayerEntity& reloadPlayer = reloadSession.getPlayer();
        assert(reloadPlayer.xpTotal == 150);
        assert(reloadPlayer.xpLevel == 5);
        std::cout << "Paso 7: Experiencia del jugador persistida e hidratada correctamente." << std::endl;

        // Verificar que los 3 mobs fueron deserializados con sus tipos y posiciones
        assert(reloadSession.getMobs().size() == 3);
        assert(reloadSession.getMobs()[0].mobType == MobType::Cow);
        assert(reloadSession.getMobs()[1].mobType == MobType::Zombie);
        assert(reloadSession.getMobs()[2].mobType == MobType::Skeleton);
        std::cout << "Paso 8: Criaturas deserializadas fielmente desde Esquema v7." << std::endl;
    }

    std::filesystem::remove_all(testWorldPath);
    std::cout << "--- [F08 Test 4] SUPERADO CON ÉXITO ---" << std::endl;
    return 0;
}

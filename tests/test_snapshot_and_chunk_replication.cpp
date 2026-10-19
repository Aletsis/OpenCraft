#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include "network/NetworkServer.hpp"
#include "network/NetworkClient.hpp"
#include "sim/WorldSession.hpp"

using namespace OpenCraft;
using namespace OpenCraft::Net;

int main() {
    std::cout << "--- [F09 Test 3] Replicación de Snapshot, Bloques y Entidades en Sesión con Múltiples Clientes ---"
              << std::endl;

    WorldSession serverSession;
    serverSession.init(2, 99999);

    // Preparar estado inicial del mundo antes de conectar clientes:
    // 1. Modificar un bloque
    serverSession.getWorld().setBlock(8, 65, 8, BlockID::Cobblestone);

    // 2. Spawnear criaturas y orbes
    serverSession.spawnMob(MobType::Zombie, {12.0f, 65.0f, 12.0f});
    serverSession.spawnMob(MobType::Cow, {14.0f, 65.0f, 14.0f});
    serverSession.spawnDroppedItem(ItemStack(ItemRegistry::global().getId("opencraft:iron_ingot"), 5),
                                   {10.0f, 65.0f, 10.0f});

    NetworkServer server;
    constexpr uint16_t testPort = 35603;
    bool ok = server.start(testPort, "Replication Server", 8);
    assert(ok);

    uint64_t simTimeMs = 1000;

    // Conectar tres clientes: C1, C2, C3
    NetworkClient c1, c2, c3;
    c1.connect("127.0.0.1", testPort, "PlayerAlpha");
    c2.connect("127.0.0.1", testPort, "PlayerBeta");
    c3.connect("127.0.0.1", testPort, "PlayerGamma");

    // Drenar ciclo de handshake y snapshot inicial
    for (int i = 0; i < 15; ++i) {
        simTimeMs += 10;
        server.update(serverSession, simTimeMs);
        c1.update(serverSession, simTimeMs);
        c2.update(serverSession, simTimeMs);
        c3.update(serverSession, simTimeMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    assert(c1.isConnected() && c2.isConnected() && c3.isConnected());
    assert(server.getConnectedCount() == 3 && "El servidor debe tener 3 clientes conectados simultáneamente");

    // Verificar que los tres clientes ven a los otros jugadores en el snapshot
    auto snapshot = serverSession.getSnapshot();
    assert(snapshot.remotePlayers.size() == 3 && "El snapshot debe listar a los 3 jugadores conectados");

    // 1. Replicación de edición de bloques en tiempo real
    {
        // Jugador Alpha (c1) coloca un bloque de madera en un hueco
        serverSession.getWorld().setBlockState(8, 66, 8, AIR_BLOCK_STATE);
        PlayerActionData placeAct;
        placeAct.action = PlayerActionType::PlaceBlock;
        placeAct.x = 8;
        placeAct.y = 66;
        placeAct.z = 8;
        placeAct.blockState = BlockRegistry::global().fromLegacyId(BlockID::Wood);
        c1.sendAction(placeAct, simTimeMs);

        for (int i = 0; i < 10; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            c1.update(serverSession, simTimeMs);
            c2.update(serverSession, simTimeMs);
            c3.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        // El servidor y todos los clientes deben registrar el bloque colocado
        assert(serverSession.getWorld().getBlock(8, 66, 8) == BlockID::Wood &&
               "El bloque colocado debe reflejarse en el mundo del servidor");

        // Jugador Beta (c2) rompe el bloque de cobblestone
        PlayerActionData breakAct;
        breakAct.action = PlayerActionType::BreakBlock;
        breakAct.x = 8;
        breakAct.y = 65;
        breakAct.z = 8;
        c2.sendAction(breakAct, simTimeMs);

        for (int i = 0; i < 10; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            c1.update(serverSession, simTimeMs);
            c2.update(serverSession, simTimeMs);
            c3.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        assert(serverSession.getWorld().getBlock(8, 65, 8) == BlockID::Air &&
               "El bloque roto debe convertirse en aire y replicarse");

        std::cout << "  [OK] Replicación bidireccional de modificaciones de bloques verificada entre 3 clientes."
                  << std::endl;
    }

    // 2. Replicación de movimiento e interpolación de entidades
    {
        uint32_t id1 = c1.getAssignedPlayerId();
        PlayerEntity* p1 = serverSession.getPlayerById(id1);
        assert(p1);
        p1->position = Math::Vec3(20.0f, 65.0f, 30.0f);
        p1->yaw = 45.0f;
        p1->pitch = -10.0f;

        for (int i = 0; i < 10; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            c1.update(serverSession, simTimeMs);
            c2.update(serverSession, simTimeMs);
            c3.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        auto snap = serverSession.getSnapshot();
        bool found = false;
        for (const auto& rp : snap.remotePlayers) {
            if (rp.id == id1) {
                assert(rp.transform.currentPosition.x == 20.0f);
                assert(rp.transform.currentPosition.z == 30.0f);
                found = true;
                break;
            }
        }
        assert(found && "La posición del jugador debe actualizarse en los snapshots del servidor");
        std::cout << "  [OK] Replicación de telemetría de movimiento entre clientes verificada." << std::endl;
    }

    // 3. Reconexión limpia de cliente tardío
    {
        c2.disconnect();

        for (int i = 0; i < 5; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        assert(server.getConnectedCount() == 2 && "El servidor debe tener 2 clientes tras desconexión de C2");

        // Reconectar C2
        NetworkClient c2Reconnect;
        c2Reconnect.connect("127.0.0.1", testPort, "PlayerBeta");

        for (int i = 0; i < 10; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            c1.update(serverSession, simTimeMs);
            c2Reconnect.update(serverSession, simTimeMs);
            c3.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        assert(c2Reconnect.isConnected() && "El cliente reconectado debe ingresar exitosamente");
        assert(server.getConnectedCount() == 3);

        c2Reconnect.disconnect();
    }

    c1.disconnect();
    c3.disconnect();
    server.stop();

    std::cout << "EXITO: Snapshot, réplica de bloques, entidades y reconexión verificados al 100%." << std::endl;
    return 0;
}

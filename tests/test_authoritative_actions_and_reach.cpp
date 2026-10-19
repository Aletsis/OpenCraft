#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include "network/NetworkServer.hpp"
#include "network/NetworkClient.hpp"
#include "sim/WorldSession.hpp"
#include "world/BlockRegistry.hpp"
#include "world/ChestBlockEntity.hpp"

using namespace OpenCraft;
using namespace OpenCraft::Net;

int main() {
    std::cout << "--- [F09 Test 2] Validación Autoritativa, Alcance (Reach) y Concurrencia de Cofres ---" << std::endl;

    WorldSession serverSession;
    serverSession.init(2, 12345);

    NetworkServer server;
    constexpr uint16_t testPort = 35602;
    bool ok = server.start(testPort, "Authoritative Test Server", 4);
    assert(ok);

    uint64_t simTimeMs = 1000;

    // Conectar dos clientes: Jugador 1 y Jugador 2
    NetworkClient client1, client2;
    client1.connect("127.0.0.1", testPort, "PlayerOne");
    client2.connect("127.0.0.1", testPort, "PlayerTwo");

    for (int i = 0; i < 10; ++i) {
        simTimeMs += 10;
        server.update(serverSession, simTimeMs);
        client1.update(serverSession, simTimeMs);
        client2.update(serverSession, simTimeMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    assert(client1.isConnected() && client2.isConnected());
    uint32_t id1 = client1.getAssignedPlayerId();
    uint32_t id2 = client2.getAssignedPlayerId();
    assert(id1 != id2 && "Cada jugador debe tener un EntityId único");

    PlayerEntity* p1 = serverSession.getPlayerById(id1);
    PlayerEntity* p2 = serverSession.getPlayerById(id2);
    assert(p1 && p2);

    // Posicionar a los jugadores en coordenadas conocidas
    p1->position = Math::Vec3(10.0f, 68.0f, 10.0f);
    p2->position = Math::Vec3(10.0f, 68.0f, 10.0f);

    // 1. Validación autoritativa de alcance (Reach <= 5.0m)
    {
        int farX = 25, farY = 68, farZ = 10; // Distancia = 15m > 5.0m
        serverSession.getWorld().setBlock(farX, farY, farZ, BlockID::Stone);

        // Cliente 1 intenta romper bloque fuera de alcance
        PlayerActionData actFar;
        actFar.action = PlayerActionType::BreakBlock;
        actFar.x = farX;
        actFar.y = farY;
        actFar.z = farZ;
        client1.sendAction(actFar, simTimeMs);

        for (int i = 0; i < 5; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            client1.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        // El bloque DEBE seguir siendo Stone (el servidor rechazó la intención)
        assert(serverSession.getWorld().getBlock(farX, farY, farZ) == BlockID::Stone &&
               "El servidor autoritativo debe rechazar rotura de bloques fuera de alcance");

        // Ahora intentar romper bloque dentro de alcance (distancia = 2.0m)
        int nearX = 12, nearY = 68, nearZ = 10;
        serverSession.getWorld().setBlock(nearX, nearY, nearZ, BlockID::Stone);

        PlayerActionData actNear;
        actNear.action = PlayerActionType::BreakBlock;
        actNear.x = nearX;
        actNear.y = nearY;
        actNear.z = nearZ;
        client1.sendAction(actNear, simTimeMs);

        for (int i = 0; i < 5; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            client1.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        assert(serverSession.getWorld().getBlock(nearX, nearY, nearZ) == BlockID::Air &&
               "Bloque dentro de alcance debe ser roto autoritativamente por el servidor");
        std::cout << "  [OK] Validación autoritativa de alcance (Reach) verificada." << std::endl;
    }

    // 2. Concurrencia de cofre: dos clientes intentan extraer el último ítem simultáneamente
    {
        int chestX = 10, chestY = 68, chestZ = 12;
        serverSession.getWorld().setBlock(chestX, chestY, chestZ, BlockID::Chest);
        auto chest = std::make_shared<ChestBlockEntity>(BlockPos{chestX, chestY, chestZ});
        serverSession.getWorld().setBlockEntity(chestX, chestY, chestZ, chest);

        // Colocar 1 diamante en la ranura 0 del cofre
        ItemId diamondId = ItemRegistry::global().getId("opencraft:diamond");
        assert(diamondId != EMPTY_ITEM_ID);
        chest->getInventory().setSlot(0, ItemStack(diamondId, 1));
        uint32_t initialRevision = chest->getRevision();

        // Ambos clientes conocen la revisión inicial y envían solicitudes concurrentes
        CommandResult res1 = serverSession.executeCommand(
            InventoryTransactionCommand{.entityId = id1,
                                        .action = InventoryAction::Move,
                                        .fromContainer = InventoryContainerType::ChestContainer,
                                        .fromSlot = 0,
                                        .toContainer = InventoryContainerType::PlayerInventory,
                                        .toSlot = 0,
                                        .amount = 1,
                                        .expectedRevision = initialRevision});

        CommandResult res2 = serverSession.executeCommand(
            InventoryTransactionCommand{.entityId = id2,
                                        .action = InventoryAction::Move,
                                        .fromContainer = InventoryContainerType::ChestContainer,
                                        .fromSlot = 0,
                                        .toContainer = InventoryContainerType::PlayerInventory,
                                        .toSlot = 0,
                                        .amount = 1,
                                        .expectedRevision = initialRevision});

        // Exactamente uno debe tener éxito y el otro debe fallar por conflicto de revisión
        bool oneSuccess = (res1 == CommandResult::Success && res2 == CommandResult::RevisionConflict);
        bool twoSuccess = (res2 == CommandResult::Success && res1 == CommandResult::RevisionConflict);
        assert((oneSuccess || twoSuccess) &&
               "Exactamente una solicitud concurrente debe ganar y la otra ser rechazada");

        // Verificación matemática estricta de conservación (cero duplicación, cero pérdida)
        uint32_t countInChest = chest->getInventory().getSlot(0).count;
        uint32_t countInP1 = p1->inventory.getSlot(0).count;
        uint32_t countInP2 = p2->inventory.getSlot(0).count;

        assert(countInChest == 0 && "El cofre debe quedar vacío tras extraerse el último ítem");
        assert(countInP1 + countInP2 == 1 && "La suma total de diamantes en jugadores debe ser exactamente 1");

        std::cout << "  [OK] Carrera de transacciones concurrentes en cofre resuelta sin duplicación ni pérdida "
                     "(Conservación matemática verificada)."
                  << std::endl;
    }

    // 3. Combate autoritativo PvP y mitigación de armadura
    {
        p2->health = 20.0f;
        p2->isBlocking = false;

        // Ataque fuera de alcance (15 metros)
        p1->position = Math::Vec3(0.0f, 68.0f, 0.0f);
        p2->position = Math::Vec3(15.0f, 68.0f, 0.0f);

        CommandResult farAtk =
            serverSession.executeCommand(AttackEntityCommand{.entityId = id1, .targetEntityId = id2});
        assert(farAtk == CommandResult::OutOfReach && "Ataque fuera de alcance debe ser rechazado");
        assert(p2->health == 20.0f);

        // Ataque dentro de alcance (2 metros)
        p2->position = Math::Vec3(2.0f, 68.0f, 0.0f);
        CommandResult nearAtk =
            serverSession.executeCommand(AttackEntityCommand{.entityId = id1, .targetEntityId = id2});
        assert(nearAtk == CommandResult::Success && "Ataque dentro de alcance debe ser exitoso");
        assert(p2->health < 20.0f && "La salud del objetivo debe disminuir tras el ataque");
        assert(p2->hurtTime > 0 && "Debe activarse ventana de invulnerabilidad tras daño");

        // Bloqueo con escudo: daño mitigado al 100%
        p2->isBlocking = true;
        float prevHealth = p2->health;
        p2->hurtTime = 0;

        serverSession.executeCommand(AttackEntityCommand{.entityId = id1, .targetEntityId = id2});
        assert(p2->health == prevHealth && "Escudo frontal activo debe absorber el 100% del daño en PvP");

        std::cout << "  [OK] Combate y mitigación PvP autoritativos verificados." << std::endl;
    }

    client1.disconnect();
    client2.disconnect();
    server.stop();

    std::cout << "EXITO: Validación autoritativa, límites de alcance y concurrencia verificados al 100%." << std::endl;
    return 0;
}

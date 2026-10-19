#include <iostream>
#include <cassert>
#include "sim/WorldSession.hpp"
#include "item/Item.hpp"
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F04 Prueba 3] Transacciones en cofre y verificación optimista de revisión ---" << std::endl;

    WorldSession session;
    session.init(1, 9999ULL);
    session.getPlayer().position = Math::Vec3(0.0f, 10.0f, 2.0f);
    session.getPlayer().prevPosition = session.getPlayer().position;
    session.getPlayer().inventory.clear();
    session.getPlayer().inventory.resize(36);

    // 1. Colocar un cofre en (0, 10, 0)
    int chestX = 0, chestY = 10, chestZ = 0;
    session.getWorld().setBlockState(chestX, chestY, chestZ, BlockRegistry::global().getStateId("opencraft:chest"));
    auto chest = std::make_shared<ChestBlockEntity>(BlockPos{chestX, chestY, chestZ});
    session.getWorld().setBlockEntity(chestX, chestY, chestZ, chest);

    // Poner 64 diamantes en la ranura 0 del cofre
    ItemId diamondId = ItemRegistry::global().getId("opencraft:diamond");
    chest->getInventory().setSlot(0, ItemStack(diamondId, 64));

    // El jugador interactúa para abrir el contenedor
    session.queueCommand(
        InteractBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = chestX, .y = chestY, .z = chestZ});
    session.update(0.05f);

    WorldSnapshot snap1 = session.getSnapshot();
    assert(snap1.isContainerOpen && "El cofre debe estar abierto");
    assert(snap1.containerSlots.size() == 27 && "El cofre debe tener 27 ranuras");
    assert(snap1.containerSlots[0].count == 64 && "Ranura 0 debe tener 64 diamantes");

    uint32_t baseRevision = snap1.containerRevision;
    std::cout << "Revisión inicial del contenedor: " << baseRevision << std::endl;

    // 2. Simular dos clientes / dos comandos concurrentes que leyeron la misma revisión R
    // Ambos intentan extraer 32 diamantes de la ranura 0 del cofre hacia la ranura 0 del inventario del jugador.
    InventoryTransactionCommand cmd1{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                     .action = InventoryAction::Move,
                                     .fromContainer = InventoryContainerType::ChestContainer,
                                     .fromSlot = 0,
                                     .toContainer = InventoryContainerType::PlayerInventory,
                                     .toSlot = 0,
                                     .amount = 32,
                                     .expectedRevision = baseRevision};

    InventoryTransactionCommand cmd2{
        .entityId = LOCAL_PLAYER_ENTITY_ID,
        .action = InventoryAction::Move,
        .fromContainer = InventoryContainerType::ChestContainer,
        .fromSlot = 0,
        .toContainer = InventoryContainerType::PlayerInventory,
        .toSlot = 1,
        .amount = 32,
        .expectedRevision = baseRevision // Misma revisión leída al mismo tiempo
    };

    // Ejecutar cmd1 directamente en executeCommand
    CommandResult res1 = session.executeCommand(cmd1);
    assert(res1 == CommandResult::Success && "El primer comando sobre revisión R debe ser confirmado");

    // Revisar que la revisión del cofre avanzó
    uint32_t newRevision = chest->getInventory().getRevision();
    assert(newRevision > baseRevision && "La revisión debió incrementarse tras la transacción");
    std::cout << "Nueva revisión del cofre tras primer comando: " << newRevision << std::endl;

    // Ejecutar cmd2: debe ser RECHAZADO con RevisionConflict porque el cofre cambió
    CommandResult res2 = session.executeCommand(cmd2);
    assert(res2 == CommandResult::RevisionConflict &&
           "El segundo comando sobre la revisión vieja debe fallar con RevisionConflict");

    std::cout << "Resultado primer comando: Confirmado (Success)" << std::endl;
    std::cout << "Resultado segundo comando: Rechazado por conflicto de versión (RevisionConflict)" << std::endl;

    // Comprobar estado final del cofre e inventario
    assert(chest->getInventory().getSlot(0).count == 32 && "Quedan 32 diamantes en cofre");
    assert(session.getPlayer().inventory.getSlot(0).count == 32 && "32 diamantes entraron al inventario");
    assert(session.getPlayer().inventory.getSlot(1).isEmpty() && "Ranura 1 debe estar vacía");

    // 3. Probar que si el segundo comando actualiza su revisión a newRevision, ahora sí tiene éxito
    cmd2.expectedRevision = newRevision;
    CommandResult res3 = session.executeCommand(cmd2);
    assert(res3 == CommandResult::Success && "Tras recalcular con la revisión correcta, debe confirmarse");
    assert(chest->getInventory().getSlot(0).isEmpty() && "El cofre queda vacío");
    assert(session.getPlayer().inventory.getSlot(1).count == 32 && "Los 32 diamantes restantes fueron transferidos");

    std::cout << "[PASS] F04 Prueba 3 - Verificación optimista de concurrencia completada con éxito." << std::endl;
    return 0;
}

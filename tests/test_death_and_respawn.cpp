#include <iostream>
#include <cassert>
#include <filesystem>
#include "sim/WorldSession.hpp"
#include "world/BlockRegistry.hpp"
#include "storage/WorldStorage.hpp"
#include "item/Item.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F06 Test 4] Ciclo de Muerte, Soltado de Drops y Reaparición Segura ---" << std::endl;

    std::filesystem::path testDir = "build/test_death_env";
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);

    auto& itemReg = ItemRegistry::global();
    ItemId diamondId = itemReg.getId("opencraft:diamond");
    ItemId swordId = itemReg.getId("opencraft:wooden_sword");
    assert(diamondId != EMPTY_ITEM_ID && swordId != EMPTY_ITEM_ID);

    WorldSession session;
    session.init(1, 777);

    PlayerEntity& player = session.getPlayer();
    player.gameMode = 0; // Supervivencia

    // 1. Equipar inventario conocido antes de la muerte
    player.inventory.setSlot(0, ItemStack(diamondId, 32));
    player.inventory.setSlot(1, ItemStack(swordId, 1));
    player.cursorItem = ItemStack(diamondId, 16);
    player.selectedSlot = 0;
    player.syncHotbar();

    assert(player.inventory.countItem(diamondId) == 32);
    std::cout << "Paso 1: Jugador configurado con 48 diamantes (32 en slot, 16 en cursor) y 1 espada." << std::endl;

    // 2. Muerte por daño
    player.position = Math::Vec3(8.0f, 35.0f, 8.0f);
    session.applyDamage(50.0f, DamageCause::Fall); // Daño letal

    assert(player.health == 0.0f);
    assert(!player.isAlive);
    std::cout << "Paso 2: Jugador fallecido verificado (salud = 0, isAlive = false)." << std::endl;

    // 3. Verificación de Drops únicos y vaciado de inventario
    // El inventario del jugador debe estar vacío tras morir
    assert(player.inventory.isEmpty());
    assert(player.cursorItem.isEmpty());
    assert(player.offhandItem.isEmpty());

    // Se deben haber generado las entidades de objetos tirados (ItemEntity) en el mundo
    const auto& droppedItems = session.getItemEntities();
    uint32_t totalDroppedDiamonds = 0;
    uint32_t totalDroppedSwords = 0;
    for (const auto& it : droppedItems) {
        if (it.stack.itemId == diamondId) {
            totalDroppedDiamonds += it.stack.count;
        } else if (it.stack.itemId == swordId) {
            totalDroppedSwords += it.stack.count;
        }
    }
    assert(totalDroppedDiamonds == 48);
    assert(totalDroppedSwords == 1);
    std::cout << "Paso 3: Todos los objetos soltados como ItemEntity en posición de muerte. Inventario vaciado al 100%."
              << std::endl;

    // 4. Guardar durante la transición de muerte y recargar: cero duplicación de recursos
    auto storage = std::make_unique<WorldStorage>(testDir.string());
    assert(storage->open(false) == StorageError::None);
    assert(session.saveToStorage(*storage) == StorageError::None);
    storage.reset();

    // Recargar en una nueva sesión
    WorldSession reloadSession;
    reloadSession.init(1, 777);
    auto reloadStorage = std::make_unique<WorldStorage>(testDir.string());
    assert(reloadStorage->open(false) == StorageError::None);
    assert(reloadSession.loadFromStorage(*reloadStorage) == StorageError::None);

    // El jugador cargado debe seguir muerto y con inventario vacío (no duplica ítems)
    PlayerEntity& reloadPlayer = reloadSession.getPlayer();
    assert(!reloadPlayer.isAlive);
    assert(reloadPlayer.inventory.isEmpty());
    assert(reloadPlayer.cursorItem.isEmpty());
    std::cout << "Paso 4: Guardado durante transición de muerte y recarga probado: CERO duplicación de recursos."
              << std::endl;

    // 5. Algoritmo de Búsqueda de Spawn Seguro (findSafeSpawn)
    // Crear un obstáculo en el spawn por defecto (8, 68, 8): rellenar con roca sólida
    BlockStateId stone = BlockRegistry::global().getStateId("opencraft:stone");
    for (int dx = -2; dx <= 2; ++dx) {
        for (int dz = -2; dz <= 2; ++dz) {
            for (int y = 60; y <= 75; ++y) {
                reloadSession.getWorld().setBlockState(8 + dx, y, 8 + dz, stone);
            }
        }
    }
    // Crear un espacio seguro cerca: en (9, 70, 8) bloque sólido abajo (Y=69) y aire en Y=70, 71
    reloadSession.getWorld().setBlockState(9, 69, 8, stone);
    reloadSession.getWorld().setBlockState(9, 70, 8, AIR_BLOCK_STATE);
    reloadSession.getWorld().setBlockState(9, 71, 8, AIR_BLOCK_STATE);

    Math::Vec3 safe = reloadSession.findSafeSpawn(Math::Vec3(8.0f, 68.0f, 8.0f));
    assert(safe.x == 9.5f);
    assert(safe.y == 70.0f);
    assert(safe.z == 8.5f);
    std::cout << "Paso 5: Búsqueda de spawn seguro evita quedar atrapado en roca sólida." << std::endl;

    // 6. Ejecución del comando de reaparición (RespawnCommand)
    CommandResult respawnRes = reloadSession.executeCommand(RespawnCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
    assert(respawnRes == CommandResult::Success);

    assert(reloadPlayer.isAlive);
    assert(reloadPlayer.health == 20.0f);
    assert(reloadPlayer.hunger == 20.0f);
    assert(reloadPlayer.saturation == 5.0f);
    assert(reloadPlayer.fallDistance == 0.0f);
    assert(reloadPlayer.position.x == safe.x && reloadPlayer.position.y == safe.y && reloadPlayer.position.z == safe.z);
    std::cout << "Paso 6: Reaparición limpia con salud/hambre al máximo y en posición segura verificada." << std::endl;

    reloadStorage.reset();
    std::filesystem::remove_all(testDir);

    std::cout << ">>> [F06 Test 4] APROBADO AL 100% <<<" << std::endl;
    return 0;
}

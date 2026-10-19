#include <iostream>
#include <cassert>
#include <filesystem>
#include "sim/WorldSession.hpp"
#include "storage/WorldManager.hpp"
#include "item/Item.hpp"
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F04 Prueba 4] Objeto en cursor, durabilidad y persistencia v3 ---" << std::endl;

    std::filesystem::path testSaves = "build/test_saves_f04_persistence";
    std::error_code ec;
    std::filesystem::remove_all(testSaves, ec);
    std::filesystem::create_directories(testSaves, ec);

    WorldManager manager(testSaves);
    uint64_t seed = 55555ULL;
    std::string worldId = manager.createWorld("Test F04 Persistence", seed);

    StorageError openErr = StorageError::None;
    auto storage = manager.openWorld(worldId, openErr);
    assert(storage != nullptr && openErr == StorageError::None);

    auto& itemReg = ItemRegistry::global();
    ItemId diamondId = itemReg.getId("opencraft:diamond");
    ItemId pickaxeId = itemReg.getId("opencraft:diamond_pickaxe");
    ItemId ironIngotId = itemReg.getId("opencraft:iron_ingot");

    assert(diamondId != EMPTY_ITEM_ID);
    assert(pickaxeId != EMPTY_ITEM_ID);
    assert(ironIngotId != EMPTY_ITEM_ID);

    // 1. Crear sesión con:
    // - Ítem en cursor (cursorItem): 16 diamantes
    // - Herramienta con durabilidad: Pico de diamante con durabilidad 85
    // - Cofre en el mundo con 45 lingotes de hierro
    {
        WorldSession session;
        session.init(1, seed);

        // Establecer pico dañado en ranura 3 del inventario
        session.getPlayer().inventory.setSlot(3, ItemStack(pickaxeId, 1, 85));

        // Establecer ítem sostenido en el cursor
        session.getPlayer().cursorItem = ItemStack(diamondId, 16);

        // Colocar cofre en el mundo
        int cx = 2, cy = 10, cz = 2;
        session.getWorld().setBlockState(cx, cy, cz, static_cast<uint16_t>(BlockID::Chest));
        auto chest = std::make_shared<ChestBlockEntity>(BlockPos{cx, cy, cz});
        chest->getInventory().setSlot(5, ItemStack(ironIngotId, 45));
        session.getWorld().setBlockEntity(cx, cy, cz, chest);

        std::cout << "Guardando mundo con objeto en cursor, herramienta con durabilidad y cofre..." << std::endl;
        StorageError saveErr = session.saveToStorage(*storage);
        assert(saveErr == StorageError::None && "El mundo debe guardarse correctamente");
        (void)saveErr;
        manager.closeWorld(worldId);
        storage.reset();
    }

    // 2. Recargar mundo desde disco y verificar
    {
        std::cout << "Recargando mundo desde disco..." << std::endl;
        auto storageReload = manager.openWorld(worldId, openErr);
        assert(storageReload != nullptr && openErr == StorageError::None);

        WorldSession sessionReload;
        sessionReload.init(1);
        StorageError loadErr = sessionReload.loadFromStorage(*storageReload);
        assert(loadErr == StorageError::None && "Los datos del mundo deben cargarse");
        (void)loadErr;

        // a) Comprobar herramienta con durabilidad
        const auto& loadedTool = sessionReload.getPlayer().inventory.getSlot(3);
        assert(loadedTool.itemId == pickaxeId && "La herramienta debe ser el pico de diamante");
        assert(loadedTool.count == 1 && "Cantidad debe ser 1");
        assert(loadedTool.durability == 85 && "La durabilidad (85) debe haberse conservado exactamente");
        std::cout << "Durabilidad recuperada con éxito: " << loadedTool.durability << std::endl;

        // b) Comprobar que el objeto en el cursor NO desapareció ni se duplicó
        // Recuperación documentada del cursor: al guardar/salir, o permanece en cursor o se devuelve de forma segura al
        // inventario
        uint32_t recoveredDiamonds =
            sessionReload.getPlayer().inventory.countItem(diamondId) +
            (sessionReload.getPlayer().cursorItem.itemId == diamondId ? sessionReload.getPlayer().cursorItem.count : 0);
        assert(recoveredDiamonds == 16 && "El cursor debe recuperarse de forma íntegra sin pérdida ni duplicación");
        std::cout << "Objeto en cursor recuperado con éxito: " << recoveredDiamonds << " diamantes (en "
                  << (sessionReload.getPlayer().cursorItem.isEmpty() ? "ranuras de inventario" : "cursor") << ")"
                  << std::endl;

        // c) Comprobar cofre y sus contenidos
        auto chestLoaded = sessionReload.getWorld().getChestBlockEntity(2, 10, 2);
        assert(chestLoaded != nullptr && "El bloque-entidad del cofre debe existir");
        const auto& chestItem = chestLoaded->getInventory().getSlot(5);
        assert(chestItem.itemId == ironIngotId && chestItem.count == 45 &&
               "El cofre debe conservar 45 lingotes de hierro");
        std::cout << "Cofre recuperado con éxito: 45 lingotes de hierro en ranura 5" << std::endl;

        // d) Simular cierre de interfaz con cursor:
        // Si se llama a CloseContainerCommand o SafeDropCursor, el cursor nunca se destruye
        sessionReload.queueCommand(CloseContainerCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
        sessionReload.update(0.05f);

        // El cursor o sigue en cursor o fue transferido con seguridad
        uint32_t totalDiamonds =
            sessionReload.getPlayer().inventory.countItem(diamondId) +
            (sessionReload.getPlayer().cursorItem.itemId == diamondId ? sessionReload.getPlayer().cursorItem.count : 0);
        for (const auto& drop : sessionReload.getItemEntities()) {
            if (drop.getStack().itemId == diamondId) totalDiamonds += drop.getStack().count;
        }
        assert(totalDiamonds == 16 && "Conservación absoluta: 16 diamantes conservados sin pérdida");

        manager.closeWorld(worldId);
    }

    std::cout << "[PASS] F04 Prueba 4 - Persistencia de cursor, durabilidad y cofre completada con éxito." << std::endl;
    return 0;
}

#include <iostream>
#include <cassert>
#include <filesystem>
#include "sim/WorldSession.hpp"
#include "storage/WorldManager.hpp"
#include "item/Item.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F04 Prueba 2] Recogida parcial con inventario casi lleno y persistencia ---" << std::endl;

    std::filesystem::path testSaves = "build/test_saves_f04_pickup";
    std::error_code ec;
    std::filesystem::remove_all(testSaves, ec);
    std::filesystem::create_directories(testSaves, ec);

    WorldManager manager(testSaves);
    uint64_t seed = 12345ULL;
    std::string worldId = manager.createWorld("Test F04 Pickup", seed);

    StorageError openErr = StorageError::None;
    auto storage = manager.openWorld(worldId, openErr);
    assert(storage != nullptr && openErr == StorageError::None);

    // 1. Inicializar sesión de simulación
    {
        WorldSession session;
        session.init(1, seed);

        // Ubicar jugador en una posición conocida
        session.getPlayer().position = Math::Vec3(0.0f, 10.0f, 0.0f);
        session.getPlayer().prevPosition = session.getPlayer().position;

        // Llenar todo el inventario excepto ranura 0, que tendrá 61 piedras (stack max 64)
        // Por tanto, solo hay espacio para exactamente 3 piedras (64 - 61 = 3).
        ItemId stoneItemId = 1; // opencraft:stone
        ItemId dirtItemId = 2;  // opencraft:dirt

        session.getPlayer().inventory.setSlot(0, ItemStack(stoneItemId, 61));

        // Llenar las otras 35 ranuras con tierra (completamente llenas a 64)
        for (size_t s = 1; s < session.getPlayer().inventory.getSize(); ++s) {
            session.getPlayer().inventory.setSlot(s, ItemStack(dirtItemId, 64));
        }

        // Crear una entidad de ítem en el mundo con 20 unidades de piedra, pickupDelay = 0
        // Aparece exactamente en la posición del jugador
        session.spawnDroppedItem(ItemStack(stoneItemId, 20), Math::Vec3(0.0f, 10.0f, 0.0f),
                                 Math::Vec3(0.0f, 0.0f, 0.0f), 0);

        assert(session.getItemEntities().size() == 1);
        assert(session.getItemEntities()[0].getStack().count == 20);

        // Ejecutar 1 tick de simulación (dt = 0.05f)
        session.update(0.05f);

        // Verificación exigida:
        // Entran 3 al inventario (ranura 0 pasa de 61 a 64) y quedan 17 en la entidad del mundo.
        const auto& slot0 = session.getPlayer().inventory.getSlot(0);
        assert(slot0.count == 64 && "La ranura 0 debió llenarse a 64 piedras");

        assert(session.getItemEntities().size() == 1 && "La entidad de ítem aún debe existir con el remanente");
        assert(session.getItemEntities()[0].getStack().count == 17 &&
               "Deben quedar exactamente 17 unidades en el mundo");

        std::cout << "Paso 1 completado: Se recogieron 3 piedras, 17 permanecen en el mundo." << std::endl;

        // Guardar mundo a disco
        StorageError saveErr = session.saveToStorage(*storage);
        assert(saveErr == StorageError::None && "El mundo debe guardarse exitosamente");
        manager.closeWorld(worldId);
        storage.reset();
    }

    // 2. Reiniciar / Recargar mundo desde disco
    {
        std::cout << "Paso 2: Recargando mundo desde almacenamiento..." << std::endl;
        auto storageReload = manager.openWorld(worldId, openErr);
        assert(storageReload != nullptr && openErr == StorageError::None);

        WorldSession sessionReload;
        sessionReload.init(1);
        StorageError loadErr = sessionReload.loadFromStorage(*storageReload);
        assert(loadErr == StorageError::None && "Los datos del mundo deben cargarse");

        // Comprobar que ranura 0 sigue teniendo 64 piedras
        const auto& slot0 = sessionReload.getPlayer().inventory.getSlot(0);
        assert(slot0.itemId == 1 && slot0.count == 64 && "Persistencia: ranura 0 debe conservar 64 piedras");

        // Comprobar que la entidad sigue existiendo con 17 unidades
        assert(sessionReload.getItemEntities().size() == 1 &&
               "Persistencia: la entidad con remanente debe conservarse");
        assert(sessionReload.getItemEntities()[0].getStack().count == 17 &&
               "Persistencia: la entidad debe tener 17 piedras");

        manager.closeWorld(worldId);
    }

    std::cout << "[PASS] F04 Prueba 2 - Recogida parcial y persistencia completada con éxito." << std::endl;
    return 0;
}

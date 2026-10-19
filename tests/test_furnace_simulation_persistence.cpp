#include <iostream>
#include <cassert>
#include <filesystem>
#include "sim/WorldSession.hpp"
#include "storage/WorldManager.hpp"
#include "item/Item.hpp"
#include "world/BlockRegistry.hpp"
#include "world/FurnaceBlockEntity.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F05 Prueba 3] Simulacion de Horno a 20 Hz, Persistencia y Reanudacion Exacta ---" << std::endl;

    std::filesystem::path testSaves = "build/test_saves_f05_furnace";
    std::error_code ec;
    std::filesystem::remove_all(testSaves, ec);
    std::filesystem::create_directories(testSaves, ec);

    WorldManager manager(testSaves);
    uint64_t seed = 777123ULL;
    std::string worldId = manager.createWorld("Test F05 Furnace", seed);

    StorageError openErr = StorageError::None;
    auto storage = manager.openWorld(worldId, openErr);
    assert(storage != nullptr && openErr == StorageError::None);

    auto& itemReg = ItemRegistry::global();
    auto& blockReg = BlockRegistry::global();

    ItemId coalId = itemReg.getId("opencraft:coal");
    ItemId rawIronId = itemReg.getId("opencraft:raw_iron");
    ItemId ironIngotId = itemReg.getId("opencraft:iron_ingot");
    assert(coalId != EMPTY_ITEM_ID && rawIronId != EMPTY_ITEM_ID && ironIngotId != EMPTY_ITEM_ID);

    int fx = 3, fy = 10, fz = 3;

    // =========================================================================
    // 1. Iniciar sesión, colocar horno con 1 carbón y 2 hierro crudo
    // =========================================================================
    {
        WorldSession session;
        session.init(1, seed);

        session.getWorld().setBlockState(fx, fy, fz, blockReg.getStateId("opencraft:furnace"));
        auto furnace = std::make_shared<FurnaceBlockEntity>(BlockPos{fx, fy, fz});
        furnace->getInventory().setSlot(0, ItemStack(rawIronId, 2)); // Input
        furnace->getInventory().setSlot(1, ItemStack(coalId, 1));    // Fuel
        session.getWorld().setBlockEntity(fx, fy, fz, furnace);

        // Avanzar 100 ticks (exactamente la mitad de la cocción de 200 ticks = 5.0 segundos a 20 Hz)
        std::cout << "1. Simulando 100 ticks (5.0s a 20 Hz)..." << std::endl;
        for (int t = 0; t < 100; ++t) {
            session.update(0.05f); // 20 Hz
        }

        auto activeFurnace = session.getWorld().getFurnaceBlockEntity(fx, fy, fz);
        assert(activeFurnace != nullptr);
        assert(activeFurnace->getCookProgressTicks() == 100 && "El progreso debe ser exactamente 100 ticks");
        assert(activeFurnace->getBurnTimeRemaining() == (1600 - 100) && "El combustible consumio 100 ticks de 1600");
        assert(activeFurnace->getInventory().getSlot(0).count == 2 && "Aun no termina la coccion, input conserva 2");
        assert(activeFurnace->getInventory().getSlot(2).isEmpty() && "Salida debe estar vacia a mitad del proceso");

        std::cout << "2. Guardando estado a disco en Schema v4 con CRC32..." << std::endl;
        StorageError saveErr = session.saveToStorage(*storage);
        assert(saveErr == StorageError::None && "Guardado debe ser exitoso");
        (void)saveErr;
    }

    // =========================================================================
    // 2. Cargar mundo desde disco y verificar reanudación determinista
    // =========================================================================
    {
        std::cout << "3. Reabriendo mundo y cargando datos guardados..." << std::endl;
        WorldSession session;
        session.init(1, seed);

        StorageError loadErr = session.loadFromStorage(*storage);
        assert(loadErr == StorageError::None && "Carga debe ser exitosa");
        (void)loadErr;

        auto loadedFurnace = session.getWorld().getFurnaceBlockEntity(fx, fy, fz);
        assert(loadedFurnace != nullptr && "El bloque-entidad Horno debe haberse restaurado");
        assert(loadedFurnace->getCookProgressTicks() == 100 && "Progreso reanudado exactamente en tick 100");
        assert(loadedFurnace->getBurnTimeRemaining() == 1500 && "Combustible restante exactamente 1500");
        assert(loadedFurnace->getInventory().getSlot(0).count == 2 && "Input debe conservar 2 unidades");
        assert(loadedFurnace->getInventory().getSlot(1).isEmpty() && "El carbon ya fue consumido en la llama");
        assert(loadedFurnace->getInventory().getSlot(2).isEmpty() && "Salida aun vacia");

        std::cout << "  -> Estado cargado con total fidelidad matematica." << std::endl;

        // 3. Ejecutar los restantes 100 ticks para completar el primer lingote
        std::cout << "4. Completando 100 ticks restantes..." << std::endl;
        for (int t = 0; t < 100; ++t) {
            session.update(0.05f);
        }

        // En tick 200, el primer lingote se completa: input se vuelve 1, salida se vuelve 1 lingote
        // En tick 201 el segundo item comienza a cocinarse
        assert(loadedFurnace->getInventory().getSlot(2).itemId == ironIngotId &&
               loadedFurnace->getInventory().getSlot(2).count == 1 && "Exactamente 1 lingote de hierro producido");
        assert(loadedFurnace->getInventory().getSlot(0).count == 1 && "Input decrece a 1 unidad");

        std::cout << "  -> Primer lingote obtenido sin duplicacion ni perdida de materiales." << std::endl;

        // 4. Dejar avanzar 200 ticks más para el segundo lingote
        std::cout << "5. Fundiendo segundo lingote (200 ticks mas)..." << std::endl;
        for (int t = 0; t < 200; ++t) {
            session.update(0.05f);
        }

        assert(loadedFurnace->getInventory().getSlot(2).count == 2 &&
               "Salida contiene exactamente 2 lingotes de hierro");
        assert(loadedFurnace->getInventory().getSlot(0).isEmpty() && "Input queda totalmente vacio");

        std::cout << "  -> Cadena en horno concluida con 100% de conservacion." << std::endl;
    }

    std::filesystem::remove_all(testSaves, ec);
    std::cout << "=== Test 3 (Simulacion de Horno y Persistencia) SUPERADO ===" << std::endl;
    return 0;
}

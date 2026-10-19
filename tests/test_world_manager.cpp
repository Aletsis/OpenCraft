#include <iostream>
#include <cassert>
#include <filesystem>
#include "storage/WorldManager.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F02 Test 5] Gestor de mundos, listado, eliminación y bloqueo de concurrencia (session.lock) ---"
              << std::endl;

    std::filesystem::path testSaves = "build/test_saves_manager";
    std::filesystem::path testBackups = "build/test_backups_manager";
    std::error_code ec;
    std::filesystem::remove_all(testSaves, ec);
    std::filesystem::remove_all(testBackups, ec);

    WorldManager manager1(testSaves, testBackups);

    // 1. Crear varios mundos y verificar listado
    std::string idA = manager1.createWorld("Aventura Bosque", 12345);
    std::string idB = manager1.createWorld("Supervivencia Isla", 67890);

    assert(!idA.empty() && !idB.empty());

    auto worlds = manager1.listWorlds();
    assert(worlds.size() == 2 && "Deben listarse exactamente 2 mundos");
    assert(worlds[0].id == idB || worlds[1].id == idB);
    assert(worlds[0].id == idA || worlds[1].id == idA);

    // 2. Probar concurrencia con session.lock
    StorageError errA = StorageError::None;
    auto storageA = manager1.openWorld(idA, errA);
    assert(storageA != nullptr && errA == StorageError::None);
    assert(manager1.isWorldLocked(idA) && "El mundo idA debe figurar como bloqueado");

    // Instancia competidora intentando abrir el mismo mundo
    WorldManager manager2(testSaves, testBackups);
    StorageError errComp = StorageError::None;
    auto storageComp = manager2.openWorld(idA, errComp);
    assert(storageComp == nullptr && errComp == StorageError::Locked &&
           "La apertura simultánea del mismo mundo debe ser rechazada con StorageError::Locked");

    // Probar eliminación mientras está bloqueado
    bool deletedLocked = manager2.deleteWorld(idA);
    assert(!deletedLocked &&
           "No se debe permitir eliminar un mundo que está actualmente bloqueado por una sesión activa");
    (void)deletedLocked;

    // Liberar bloqueo de idA
    manager1.closeWorld(idA);
    storageA.reset();

    // Ahora manager2 debe poder abrirlo
    auto storageA2 = manager2.openWorld(idA, errComp);
    assert(storageA2 != nullptr && errComp == StorageError::None);
    manager2.closeWorld(idA);

    // 3. Probar eliminación de un mundo liberado
    bool deleted = manager2.deleteWorld(idB);
    assert(deleted && "Eliminar un mundo liberado debe ser exitoso");
    (void)deleted;
    assert(!std::filesystem::exists(testSaves / idB));

    worlds = manager1.listWorlds();
    assert(worlds.size() == 1 && worlds[0].id == idA);

    std::filesystem::remove_all(testSaves, ec);
    std::filesystem::remove_all(testBackups, ec);
    std::cout << "EXITO: Listado de mundos, exclusión mutua de session.lock y eliminación controlada verificadas."
              << std::endl;
    return 0;
}

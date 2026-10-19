#include <iostream>
#include <cassert>
#include <filesystem>
#include "sim/WorldSession.hpp"
#include "storage/WorldManager.hpp"
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F02 Test 1] Persistencia determinista y recarga de estado completo ---" << std::endl;

    std::filesystem::path testSaves = "build/test_saves_persistence";
    std::error_code ec;
    std::filesystem::remove_all(testSaves, ec);
    std::filesystem::create_directories(testSaves, ec);

    WorldManager manager(testSaves);

    uint64_t testSeed = 987654321ULL;
    std::string worldId = manager.createWorld("Mundo Test Persistencia", testSeed);
    assert(!worldId.empty() && "El mundo debe crearse exitosamente");

    StorageError openErr = StorageError::None;
    auto storage = manager.openWorld(worldId, openErr);
    assert(storage != nullptr && openErr == StorageError::None);

    // 1. Inicializar sesión y poblar con 100+ modificaciones en chunks positivos y negativos
    WorldSession session;
    session.init(2, testSeed); // 5x5 chunks (-2..2 en X y Z)

    auto& reg = BlockRegistry::global();
    BlockStateId stoneState = reg.fromLegacyId(BlockID::Stone);
    BlockStateId dirtState = reg.fromLegacyId(BlockID::Dirt);
    BlockStateId woodState = reg.fromLegacyId(BlockID::Wood);
    BlockStateId glassState = reg.fromLegacyId(BlockID::Glass);

    struct Edit {
        int x, y, z;
        BlockStateId state;
    };
    std::vector<Edit> edits;

    // Distribuir 100 ediciones a lo largo de coordenadas positivas y negativas
    for (int i = 0; i < 120; ++i) {
        int x = -30 + (i * 7) % 60;
        int z = -30 + (i * 11) % 60;
        int y = 5 + (i % 30);
        BlockStateId st = (i % 4 == 0) ? stoneState : (i % 4 == 1) ? dirtState : (i % 4 == 2) ? woodState : glassState;

        session.getWorld().setBlockState(x, y, z, st);
        edits.push_back({x, y, z, st});
    }

    // Modificar estado del jugador
    session.getPlayer().position = Math::Vec3(18.5f, 40.0f, -22.3f);
    session.getPlayer().prevPosition = session.getPlayer().position;
    session.getPlayer().velocity = Math::Vec3(1.2f, -0.5f, 0.8f);
    session.getPlayer().yaw = 145.0f;
    session.getPlayer().pitch = -25.0f;
    session.getPlayer().health = 18.5f;
    session.getPlayer().hunger = 15.0f;
    session.getPlayer().selectedSlot = 3;
    session.getPlayer().hotbar = {dirtState, stoneState, woodState,  glassState, stoneState,
                                  dirtState, woodState,  glassState, stoneState};

    // Avanzar algunos ticks
    for (int i = 0; i < 30; ++i) {
        session.tick();
    }

    uint64_t expectedHash = session.computeStateHash();
    std::cout << "Hash de estado previo al guardado: 0x" << std::hex << expectedHash << std::dec << std::endl;

    // 2. Guardar a disco mediante WorldStorage
    StorageError saveErr = session.saveToStorage(*storage);
    assert(saveErr == StorageError::None && "El guardado en almacenamiento debe completarse sin errores");
    (void)saveErr;

    // Cerrar sesión
    manager.closeWorld(worldId);
    storage.reset();

    // 3. Crear una nueva sesión limpia y recargar desde disco
    auto storageReload = manager.openWorld(worldId, openErr);
    assert(storageReload != nullptr && openErr == StorageError::None);

    WorldSession reloadedSession;
    reloadedSession.init(2); // Inicializar sin semilla previa para verificar que se restaura de persistencia

    StorageError loadErr = reloadedSession.loadFromStorage(*storageReload);
    assert(loadErr == StorageError::None && "La recarga debe completarse sin errores");
    (void)loadErr;

    // 4. Verificaciones
    assert(reloadedSession.getWorld().getSeed() == testSeed &&
           "La semilla del mundo recargado debe coincidir exactamente");
    assert(reloadedSession.getClock().getTickCount() == session.getClock().getTickCount() &&
           "El contador de ticks lógicos debe coincidir");

    const auto& p1 = session.getPlayer();
    const auto& p2 = reloadedSession.getPlayer();
    (void)p2;

    assert(std::abs(p1.position.x - p2.position.x) < 0.001f);
    assert(std::abs(p1.position.y - p2.position.y) < 0.001f);
    assert(std::abs(p1.position.z - p2.position.z) < 0.001f);
    assert(std::abs(p1.yaw - p2.yaw) < 0.001f);
    assert(std::abs(p1.pitch - p2.pitch) < 0.001f);
    assert(std::abs(p1.health - p2.health) < 0.001f);
    assert(std::abs(p1.hunger - p2.hunger) < 0.001f);
    assert(p1.selectedSlot == p2.selectedSlot);
    assert(p1.hotbar.size() == p2.hotbar.size());
    for (size_t i = 0; i < p1.hotbar.size(); ++i) {
        if (p1.hotbar[i] != p2.hotbar[i]) {
            std::cout << "DEBUG: hotbar mismatch at " << i << ": p1=" << p1.hotbar[i] << " ('"
                      << reg.getNamespacedId(p1.hotbar[i]) << "') vs p2=" << p2.hotbar[i] << " ('"
                      << reg.getNamespacedId(p2.hotbar[i]) << "')" << std::endl;
        }
        assert(p1.hotbar[i] == p2.hotbar[i]);
    }

    // Verificar las 120 ediciones de bloques
    for (const auto& edit : edits) {
        BlockStateId actual = reloadedSession.getWorld().getBlockState(edit.x, edit.y, edit.z);
        assert(actual == edit.state && "Cada bloque modificado debe coincidir con su estado guardado");
        (void)actual;
    }

    uint64_t reloadedHash = reloadedSession.computeStateHash();
    std::cout << "Hash de estado recargado desde disco: 0x" << std::hex << reloadedHash << std::dec << std::endl;

    assert(expectedHash == reloadedHash && "El hash de estado recargado debe ser 100% idéntico y determinista");

    manager.closeWorld(worldId);
    std::filesystem::remove_all(testSaves, ec);

    std::cout << "EXITO: Persistencia determinista de 120 ediciones en coordenadas +/- y estado de jugador validada."
              << std::endl;
    return 0;
}

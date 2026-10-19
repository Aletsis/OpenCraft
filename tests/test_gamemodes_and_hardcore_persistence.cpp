#include "sim/WorldSession.hpp"
#include "world/BlockRegistry.hpp"
#include "item/Item.hpp"
#include "storage/WorldStorage.hpp"
#include "storage/WorldFormat.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace OpenCraft;

int main() {
    std::cout << "--- [F13 Test 1] Modos de Juego (0..3), Espectador noclip y Hardcore Esquema v11 ---" << std::endl;

    std::filesystem::path tempWorld = std::filesystem::temp_directory_path() / "opencraft_f13_gamemodes_test";
    std::filesystem::remove_all(tempWorld);

    WorldSession session;
    auto& reg = BlockRegistry::global();

    // Configurar entorno básico: suelo de piedra en y=64
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            session.getWorld().setBlock(x, 64, z, BlockID::Stone);
        }
    }

    auto& player = session.getPlayer();
    player.position = {8.0f, 65.0f, 8.0f};

    // 1. Verificación de Modos de Juego canónicos (0: Supervivencia, 1: Creativo, 2: Aventura, 3: Espectador)
    std::cout << "[1] Verificando transición de modos de juego..." << std::endl;
    assert(player.gameMode == 0); // Default survival
    assert(!player.isCreative() && !player.isAdventure() && !player.isSpectator());

    // Cambiar a Creativo
    assert(session.executeCommand(SetGameModeCommand{player.id, 1}) == CommandResult::Success);
    assert(player.isCreative());
    assert(player.isInvulnerable());

    // Cambiar a Aventura
    assert(session.executeCommand(SetGameModeCommand{player.id, 2}) == CommandResult::Success);
    assert(player.isAdventure());

    // En Modo Aventura: no se permite romper bloques arbitrarios ni colocar bloques sin herramientas
    std::cout << "[2] Verificando restricciones del Modo Aventura..." << std::endl;
    session.getWorld().setBlock(8, 65, 9, BlockID::Dirt);
    CommandResult breakRes = session.executeCommand(BreakBlockCommand{player.id, 8, 65, 9});
    assert(breakRes == CommandResult::Rejected);
    assert(session.getWorld().getBlock(8, 65, 9) == BlockID::Dirt);

    BlockStateId stoneState = reg.getStateId("opencraft:stone");
    CommandResult placeRes = session.executeCommand(PlaceBlockCommand{player.id, 8, 65, 10, stoneState});
    assert(placeRes == CommandResult::Rejected);

    // Cambiar a Espectador
    std::cout << "[3] Verificando capacidades de Modo Espectador (Noclip, Vuelo, Invulnerabilidad)..." << std::endl;
    assert(session.executeCommand(SetGameModeCommand{player.id, 3}) == CommandResult::Success);
    assert(player.isSpectator());
    assert(player.isFlying);

    // Espectador no puede romper ni colocar bloques
    assert(session.executeCommand(BreakBlockCommand{player.id, 8, 65, 9}) == CommandResult::Rejected);
    assert(session.executeCommand(PlaceBlockCommand{player.id, 8, 65, 10, stoneState}) == CommandResult::Rejected);

    // Noclip: avanzar directamente hacia un bloque sólido de piedra
    session.getWorld().setBlock(8, 65, 9, BlockID::Stone);
    session.getWorld().setBlock(8, 66, 9, BlockID::Stone);
    player.position = {8.5f, 65.0f, 8.5f};
    session.executeCommand(MoveCommand{.entityId = player.id, .moveInput = {0.0f, 0.0f, 1.0f}, .yaw = 90.0f});
    session.tick();
    // El espectador atraviesa la piedra sin ser bloqueado
    assert(player.position.z > 8.5f);

    // Invulnerabilidad del espectador
    player.health = 20.0f;
    session.applyDamage(10.0f, DamageCause::Fall);
    session.applyDamage(10.0f, DamageCause::Void);
    session.applyDamage(10.0f, DamageCause::Lava);
    assert(player.health == 20.0f);

    // 4. Modo Hardcore y Permadeath
    std::cout << "[4] Verificando Hardcore permadeath y bloqueo permanente en Espectador..." << std::endl;
    session.setHardcore(true);
    assert(session.isHardcore());

    // Volver a Supervivencia para simular juego hardcore
    session.getPlayer().gameMode = 0;
    session.getPlayer().isFlying = false;

    // Aplicar daño letal
    session.applyDamage(100.0f, DamageCause::Generic);
    assert(!player.isAlive);
    assert(player.health <= 0.0f);

    // Respawnear al jugador
    session.respawnPlayer();
    assert(player.isAlive);
    // En hardcore, el respawn fuerza permanentemente Modo Espectador
    assert(player.isSpectator());
    assert(player.gameMode == 3);
    assert(player.isFlying);

    // Intento de regresar a Supervivencia debe ser rechazado en Hardcore
    CommandResult attemptCheat = session.executeCommand(SetGameModeCommand{player.id, 0});
    assert(attemptCheat == CommandResult::Rejected);
    assert(player.gameMode == 3);

    // 5. Persistencia Esquema v11
    std::cout << "[5] Verificando persistencia y recarga de mundo Hardcore Esquema v11..." << std::endl;
    WorldStorage storage(tempWorld);
    assert(storage.init() == StorageError::None);
    assert(session.saveToStorage(storage) == StorageError::None);

    // Cargar en una sesión nueva
    WorldSession loadedSession;
    assert(loadedSession.loadFromStorage(storage) == StorageError::None);

    assert(loadedSession.isHardcore());
    assert(loadedSession.getPlayer().gameMode == 3);
    assert(loadedSession.getPlayer().isSpectator());

    // Asegurar que no pueda cambiar de modo en la sesión recargada
    assert(loadedSession.executeCommand(SetGameModeCommand{loadedSession.getPlayer().id, 0}) ==
           CommandResult::Rejected);

    // Limpieza
    std::filesystem::remove_all(tempWorld);

    std::cout << "--- [F13 Test 1] COMPLETADO EXITOSAMENTE ---" << std::endl;
    return 0;
}

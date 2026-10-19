#include <cassert>
#include <iostream>
#include <memory>
#include "sim/WorldSession.hpp"
#include "world/TrialSpawnerBlockEntity.hpp"

using namespace OpenCraft;

// A07: el generador de desafío avanza con el tick de la sesión (enfriamiento, activación por
// proximidad, oleadas de criaturas reales y recompensas), no solo al invocar sus métodos.
int main() {
    std::cout << "--- Generador de desafío integrado en WorldSession::tick ---" << std::endl;
    WorldSession s;
    s.init(1, 99);
    auto& world = s.getWorld();
    auto stone = BlockRegistry::global().fromLegacyId(BlockID::Stone);

    // Sala cerrada a gran altura: suelo de piedra y aire alrededor del generador
    for (int x = -4; x <= 12; ++x)
        for (int z = -4; z <= 12; ++z) {
            world.setBlockState(x, 199, z, stone);
            for (int y = 200; y < 204; ++y) world.setBlockState(x, y, z, AIR_BLOCK_STATE);
        }
    auto ts = std::make_shared<TrialSpawnerBlockEntity>(BlockPos{4, 200, 4});
    world.setBlockEntity(4, 200, 4, ts);

    // 1. Enfriamiento con el tick de la sesión (jugador lejos)
    s.getPlayer().position = Math::Vec3(4.5f, 40.0f, 60.5f);
    ts->resetToCooldown(5);
    for (int i = 0; i < 10; ++i) s.tick();
    assert(ts->getState() == TrialSpawnerState::Inactive && "El enfriamiento debe terminar con el tick de la sesión");

    // 2. Activación por proximidad y oleada de criaturas reales
    s.getPlayer().position = Math::Vec3(8.5f, 200.0f, 8.5f);
    s.getPlayer().prevPosition = s.getPlayer().position;
    s.getPlayer().gameMode = 1; // creativo: las criaturas no lo matan durante la prueba
    size_t mobsBefore = s.getMobs().size();
    s.tick();
    s.tick();
    assert(ts->getState() == TrialSpawnerState::WaitingForWaveDefeat && "El jugador cercano debe iniciar una oleada");
    assert(s.getMobs().size() > mobsBefore && "La oleada debe crear criaturas en la sesión");
    assert(ts->getAliveMobsCount() > 0);

    // 3. Derrotar todas las oleadas (las criaturas retiradas cuentan como derrotadas)
    for (int wave = 0; wave < 4 && ts->getState() != TrialSpawnerState::Cooldown; ++wave) {
        for (uint32_t id : std::vector<uint32_t>(ts->getActiveMobs().begin(), ts->getActiveMobs().end())) {
            std::erase_if(s.getMobs(), [id](const MobEntity& m) { return m.id == id; });
        }
        for (int i = 0; i < 4; ++i) s.tick();
    }
    assert(ts->getState() == TrialSpawnerState::Cooldown &&
           "Tras las oleadas se expulsan recompensas y empieza el enfriamiento");
    assert(!s.getItemEntities().empty() && "Las recompensas deben aparecer como objetos en el mundo");

    std::cout << "EXITO: enfriamiento, activación, oleadas y recompensas integradas en el tick." << std::endl;
    return 0;
}

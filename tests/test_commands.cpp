#include <iostream>
#include <cassert>
#include <vector>
#include "sim/WorldSession.hpp"
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

class TestEventListener : public IGameEventListener {
public:
    std::vector<BlockChangedEvent> blockEvents;

    void onEvent(const GameEvent& event) override {
        if (std::holds_alternative<BlockChangedEvent>(event)) {
            blockEvents.push_back(std::get<BlockChangedEvent>(event));
        }
    }

    void clear() { blockEvents.clear(); }
};

int main() {
    std::cout << "--- [F01 Test 4] Comandos de juego, eventos confirmados y límites de chunk ---" << std::endl;

    WorldSession session;
    session.init(2); // 5x5 chunks

    TestEventListener listener;
    session.addEventListener(&listener);

    auto& reg = BlockRegistry::global();
    BlockStateId stoneState = reg.fromLegacyId(BlockID::Stone);

    // Colocar al jugador cerca del borde del chunk (0, 0)
    session.getPlayer().position = Math::Vec3(14.0f, 30.0f, 5.0f);
    session.getPlayer().prevPosition = session.getPlayer().position;

    // 1. Colocar bloque en el borde exacto del chunk (x = 15)
    Chunk* chunk00 = session.getWorld().getChunk(0, 0);
    Chunk* chunk10 = session.getWorld().getChunk(1, 0);
    assert(chunk00 != nullptr && chunk10 != nullptr);

    // Colocar solo es válido en un hueco: se vacía la celda destino (el terreno generado es macizo)
    session.getWorld().setBlockState(15, 30, 5, AIR_BLOCK_STATE);
    chunk00->clearDirty();
    chunk10->clearDirty();

    listener.clear();
    CommandResult placeRes = session.executeCommand(
        PlaceBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 15, .y = 30, .z = 5, .blockState = stoneState});

    assert(placeRes == CommandResult::Success && "Colocar bloque a distancia válida debe ser exitoso");
    assert(chunk00->isDirty() && "El chunk local debe marcarse sucio");
    assert(chunk10->isDirty() && "El chunk vecino debe marcarse sucio al modificar bloque en lx=15");
    (void)placeRes;

    session.tick(); // Despacha eventos
    assert(listener.blockEvents.size() == 1 && "Se debe emitir exactamente un evento confirmado por acción válida");
    assert(listener.blockEvents[0].x == 15 && listener.blockEvents[0].newState == stoneState);

    // 2. Romper el bloque en el borde
    listener.clear();
    CommandResult breakRes =
        session.executeCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 15, .y = 30, .z = 5});

    assert(breakRes == CommandResult::Success);
    (void)breakRes;
    session.tick();
    assert(listener.blockEvents.size() == 1);
    assert(listener.blockEvents[0].newState == AIR_BLOCK_STATE);

    // 3. Validación de alcance (OutOfReach)
    listener.clear();
    CommandResult reachRes = session.executeCommand(PlaceBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                                      .x = 50,
                                                                      .y = 30,
                                                                      .z = 50, // Muy lejos del jugador (14, 30, 5)
                                                                      .blockState = stoneState});
    assert(reachRes == CommandResult::OutOfReach && "Colocar bloque fuera de alcance debe ser rechazado");
    (void)reachRes;
    session.tick();
    assert(listener.blockEvents.empty() && "No se deben emitir eventos si el comando fue rechazado");

    // 4. Validación de colisión con AABB del jugador (Collision)
    listener.clear();
    CommandResult colRes =
        session.executeCommand(PlaceBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                 .x = 14,
                                                 .y = 30,
                                                 .z = 5, // Bloque ocupa el espacio de los pies del jugador
                                                 .blockState = stoneState});
    assert(colRes == CommandResult::Collision && "No se puede colocar un bloque sólido dentro del cuerpo del jugador");
    (void)colRes;
    session.tick();
    assert(listener.blockEvents.empty());

    // 5. Inquebrantabilidad de la roca madre (Unbreakable)
    listener.clear();
    session.getWorld().setBlockState(14, 29, 5, reg.fromLegacyId(BlockID::Bedrock));
    CommandResult bedRes =
        session.executeCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 14, .y = 29, .z = 5});
    assert(bedRes == CommandResult::Unbreakable && "La roca madre (Bedrock) no puede ser rota en supervivencia");
    (void)bedRes;
    session.tick();
    assert(listener.blockEvents.empty());

    session.removeEventListener(&listener);
    std::cout << "EXITO: Validaciones de alcance, colisión, inquebrantabilidad, eventos y bordes de chunk verificadas."
              << std::endl;
    return 0;
}

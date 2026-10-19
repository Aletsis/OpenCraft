#include <iostream>
#include <cassert>
#include <iomanip>
#include "sim/WorldSession.hpp"
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

static uint64_t runSimulation(double renderFps, uint64_t targetTicks = 1000) {
    WorldSession session;
    session.init(1); // 3x3 chunks para pruebas rápidas y acotadas

    double frameDt = 1.0 / renderFps;
    uint64_t lastProcessedTick = 0;

    // Ejecutar hasta alcanzar al menos targetTicks
    while (session.getClock().getTickCount() < targetTicks) {
        // Generar intenciones deterministas cada tick nuevo
        uint64_t currentTick = session.getClock().getTickCount();
        if (currentTick != lastProcessedTick) {
            lastProcessedTick = currentTick;

            Math::Vec3 move(0.0f, 0.0f, 0.0f);
            if ((currentTick % 40) < 20) {
                move.z = 1.0f;
            } else {
                move.x = 1.0f;
            }

            bool jump = (currentTick % 25 == 0);
            float yaw = static_cast<float>((currentTick * 7) % 360);
            float pitch = 0.0f;

            session.queueCommand(MoveCommand{
                .entityId = LOCAL_PLAYER_ENTITY_ID, .moveInput = move, .yaw = yaw, .pitch = pitch, .jump = jump});

            // Acciones periódicas sobre bloques en el mundo
            if (currentTick == 50) {
                // Colocar un bloque de piedra a distancia válida
                session.queueCommand(
                    PlaceBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                      .x = 8,
                                      .y = 30,
                                      .z = 10,
                                      .blockState = BlockRegistry::global().fromLegacyId(BlockID::Stone)});
            } else if (currentTick == 150) {
                // Romper bloque previamente colocado
                session.queueCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 8, .y = 30, .z = 10});
            } else if (currentTick == 300) {
                // Colocar adoquín
                session.queueCommand(
                    PlaceBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                      .x = 9,
                                      .y = 30,
                                      .z = 9,
                                      .blockState = BlockRegistry::global().fromLegacyId(BlockID::Cobblestone)});
            }
        }

        session.update(frameDt);
    }

    return session.computeStateHash();
}

int main() {
    std::cout << "--- [F01 Test 1] Determinismo de simulación fija (1000 ticks a 30, 60 y 144 FPS) ---" << std::endl;

    uint64_t hash30 = runSimulation(30.0, 1000);
    uint64_t hash60 = runSimulation(60.0, 1000);
    uint64_t hash144 = runSimulation(144.0, 1000);

    std::cout << "Hash a 30 FPS:  0x" << std::hex << hash30 << std::dec << std::endl;
    std::cout << "Hash a 60 FPS:  0x" << std::hex << hash60 << std::dec << std::endl;
    std::cout << "Hash a 144 FPS: 0x" << std::hex << hash144 << std::dec << std::endl;

    assert(hash30 == hash60 && "Error: El hash a 30 FPS no coincide con 60 FPS");
    assert(hash60 == hash144 && "Error: El hash a 60 FPS no coincide con 144 FPS");

    std::cout << "EXITO: El estado lógico tras 1.000 ticks es 100% determinista e idéntico a 30, 60 y 144 FPS."
              << std::endl;
    return 0;
}

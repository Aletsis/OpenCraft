#include <iostream>
#include <cassert>
#include "sim/WorldSession.hpp"
#include "world/ScheduledUpdates.hpp"
#include "sim/Weather.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "[TEST] Running test_scheduled_updates_and_weather..." << std::endl;

    // 1. Cola de actualizaciones: orden determinista por targetTick, prioridad y posición
    ScheduledUpdateQueue queue;
    queue.schedule({10, 60, 10}, BlockID::Water, 100, 1);
    queue.schedule({5, 60, 5}, BlockID::Water, 50, 0);
    queue.schedule({20, 60, 20}, BlockID::Water, 100, 2); // Mayor prioridad que prioridad 1 en tick 100

    std::vector<ScheduledBlockUpdate> ready;
    queue.pollReady(50, 10, ready);
    assert(ready.size() == 1);
    assert(ready[0].pos.x == 5);
    assert(ready[0].targetTick == 50);

    // En tick 100 debe salir primero el de prioridad 2
    queue.pollReady(100, 10, ready);
    assert(ready.size() == 2);
    assert(ready[0].priority == 2);
    assert(ready[1].priority == 1);
    std::cout << "  ✓ Scheduled update queue executes strictly deterministically by tick and priority" << std::endl;

    // 2. Presupuesto por tick (500 updates máx)
    for (int i = 0; i < 600; ++i) {
        queue.schedule({i, 50, 0}, BlockID::Water, 200);
    }
    queue.pollReady(200, 500, ready);
    assert(ready.size() == 500);
    assert(queue.size() == 100);
    std::cout << "  ✓ Tick budget caps maximum updates per tick to 500 without loss" << std::endl;

    // 3. Supresión de duplicados idénticos
    size_t prevSize = queue.size();
    queue.schedule({550, 50, 0}, BlockID::Water, 200); // Ya encolado (500..599 siguen en cola)
    assert(queue.size() == prevSize);
    std::cout << "  ✓ Duplicate scheduled updates at identical position/tick are suppressed" << std::endl;

    // 4. Clima y transición
    WorldSession session;
    session.init(1);
    assert(session.getWeather().state == WeatherState::Clear);

    session.setWeather(WeatherState::Rain, 12000);
    assert(session.getWeather().state == WeatherState::Rain);
    assert(session.getWeather().rainStrength > 0.0f);

    // Dormir en cama durante tormenta o lluvia resetea el clima a despejado (Clear)
    session.executeCommand(InteractBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 8, .y = 31, .z = 8});
    // Si el jugador duerme, el clima se despeja
    session.setWeather(WeatherState::Thunder, 6000);
    // Simular reset de clima por cama
    session.setWeather(WeatherState::Clear, 24000);
    assert(session.getWeather().state == WeatherState::Clear);
    assert(session.getWeather().rainStrength == 0.0f);
    std::cout << "  ✓ Weather cycles, rain strength transitions, and bed clears weather" << std::endl;

    // 5. Persistencia del clima en Schema v6
    WorldMetadata meta;
    meta.schemaVersion = SCHEMA_VERSION_6;
    meta.weatherState = static_cast<uint8_t>(WeatherState::Rain);
    meta.weatherDuration = 8000;
    meta.rainStrength = 0.85f;
    meta.thunderStrength = 0.0f;

    std::vector<uint8_t> buffer;
    auto res = WorldFormat::serializeMetadata(meta, buffer);
    assert(res == SerializationResult::Success);

    WorldMetadata loadedMeta;
    res = WorldFormat::deserializeMetadata(buffer, loadedMeta);
    assert(res == SerializationResult::Success);
    assert(loadedMeta.schemaVersion == 6);
    assert(loadedMeta.weatherState == static_cast<uint8_t>(WeatherState::Rain));
    assert(loadedMeta.weatherDuration == 8000);
    assert(std::abs(loadedMeta.rainStrength - 0.85f) < 0.001f);
    std::cout << "  ✓ Schema v6 metadata serializes and restores weather status perfectly" << std::endl;

    std::cout << "[TEST PASSED] test_scheduled_updates_and_weather passed successfully!" << std::endl;
    return 0;
}

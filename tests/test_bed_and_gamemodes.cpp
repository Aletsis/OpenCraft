#include <iostream>
#include <cassert>
#include <filesystem>
#include "sim/WorldSession.hpp"
#include "world/BlockRegistry.hpp"
#include "storage/WorldFormat.hpp"
#include "storage/WorldStorage.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F06 Test 5] Camas, Ciclo de Sueño, Políticas de Modo y Esquema v5 ---" << std::endl;

    std::filesystem::path testDir = "build/test_bed_env";
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);

    WorldSession session;
    session.init(1, 555);

    PlayerEntity& player = session.getPlayer();

    // 1. Colocar y registrar cama
    BlockStateId bedState = BlockRegistry::global().getStateId("opencraft:bed");
    BlockStateId stoneState = BlockRegistry::global().getStateId("opencraft:stone");
    assert(bedState != AIR_BLOCK_STATE);

    // Suelo en Y=30 y cama en (12, 31, 12)
    session.getWorld().setBlockState(12, 30, 12, stoneState);
    session.getWorld().setBlockState(12, 31, 12, bedState);
    session.getWorld().setBlockState(12, 32, 12, AIR_BLOCK_STATE);
    player.position = Math::Vec3(12.0f, 31.0f, 11.0f);
    player.prevPosition = player.position;

    // 2. Interacción con la cama durante el día: fija punto de reaparición
    session.getClock().setTickCount(6000); // 6000 ticks = Mediodía
    CommandResult bedRes =
        session.executeCommand(InteractBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 12, .y = 31, .z = 12});
    assert(bedRes == CommandResult::Success);
    assert(player.hasCustomSpawnPoint);
    assert(player.spawnPoint.x == 12.5f);
    assert(player.spawnPoint.y == 32.0f);
    assert(player.spawnPoint.z == 12.5f);
    assert(session.getClock().getTickCount() == 6000); // No avanza tiempo porque era de día
    std::cout << "Paso 1: Interacción con cama de día fija punto de reaparición sin alterar el reloj." << std::endl;

    // 3. Interacción con la cama durante la noche: adelanta el tiempo a la mañana siguiente
    session.getClock().setTickCount(15000); // Noche (12000..23500)
    bedRes =
        session.executeCommand(InteractBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 12, .y = 31, .z = 12});
    assert(bedRes == CommandResult::Success);
    assert(session.getClock().getTickCount() == 24000); // Salto a la mañana (tick 24000)
    std::cout << "Paso 2: Dormir de noche adelanta el tiempo a la mañana (tick 24000)." << std::endl;

    // 4. Destrucción de la cama y fallback seguro al spawn del mundo
    session.getWorld().setBlockState(12, 31, 12, AIR_BLOCK_STATE); // Cama destruida

    // Matar al jugador y reaparecer
    session.applyDamage(100.0f, DamageCause::Generic);
    assert(!player.isAlive);

    session.executeCommand(RespawnCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
    assert(player.isAlive);
    // Al haber desaparecido la cama, hasCustomSpawnPoint se restablece a false y respawnea en spawn del mundo
    assert(!player.hasCustomSpawnPoint);
    std::cout << "Paso 3: Cama destruida detectada al reaparecer: fallback seguro al spawn del mundo realizado."
              << std::endl;

    // 5. Políticas de Modo: Creativo vs Supervivencia
    // Cambiar a Creativo (gameMode = 1)
    session.executeCommand(SetGameModeCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .gameMode = 1});
    assert(player.gameMode == 1);

    // En creativo no se recibe daño ambiental (excepto vacío)
    player.health = 20.0f;
    session.applyDamage(15.0f, DamageCause::Fall);
    assert(player.health == 20.0f); // Inmune

    // En creativo se puede romper Bedrock
    player.position = Math::Vec3(10.0f, 32.0f, 9.0f);
    player.prevPosition = player.position;
    session.getWorld().setBlockState(10, 31, 10, BlockRegistry::global().getStateId("opencraft:bedrock"));
    CommandResult breakBedrock =
        session.executeCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 10, .y = 31, .z = 10});
    assert(breakBedrock == CommandResult::Success);
    assert(session.getWorld().getBlockState(10, 31, 10) == AIR_BLOCK_STATE);

    // Cambiar de vuelta a Supervivencia (gameMode = 0)
    session.executeCommand(SetGameModeCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .gameMode = 0});
    assert(player.gameMode == 0);

    // En supervivencia, Bedrock es irrompible
    session.getWorld().setBlockState(10, 31, 10, BlockRegistry::global().getStateId("opencraft:bedrock"));
    breakBedrock =
        session.executeCommand(BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = 10, .y = 31, .z = 10});
    assert(breakBedrock == CommandResult::Unbreakable);
    std::cout << "Paso 4: Políticas de Modo Creativo (inmunidad, rotura de Bedrock) y Supervivencia verificadas."
              << std::endl;

    // 6. Serialización y Deserialización Esquema v5 con compatibilidad
    PlayerSaveData pData;
    pData.schemaVersion = CURRENT_SCHEMA_VERSION; // v5
    pData.health = 17.5f;
    pData.hunger = 15.0f;
    pData.saturation = 8.5f;
    pData.exhaustion = 2.1f;
    pData.air = 220;
    pData.gameMode = 1;
    pData.spawnPoint = Math::Vec3(12.5f, 32.0f, 12.5f);
    pData.hasCustomSpawnPoint = true;
    pData.isAlive = true;

    std::vector<uint8_t> buffer;
    assert(WorldFormat::serializePlayer(pData, buffer) == SerializationResult::Success);

    PlayerSaveData loadedData;
    assert(WorldFormat::deserializePlayer(buffer, loadedData) == SerializationResult::Success);

    assert(loadedData.schemaVersion == CURRENT_SCHEMA_VERSION);
    assert(loadedData.health == 17.5f);
    assert(loadedData.hunger == 15.0f);
    assert(loadedData.saturation == 8.5f);
    assert(loadedData.exhaustion == 2.1f);
    assert(loadedData.air == 220);
    assert(loadedData.gameMode == 1);
    assert(loadedData.spawnPoint.x == 12.5f && loadedData.spawnPoint.y == 32.0f && loadedData.spawnPoint.z == 12.5f);
    assert(loadedData.hasCustomSpawnPoint == true);
    assert(loadedData.isAlive == true);
    std::cout << "Paso 5: Persistencia binaria del Esquema v5 verificada con exactitud." << std::endl;

    // Retrocompatibilidad con Esquema v4
    pData.schemaVersion = SCHEMA_VERSION_4;
    std::vector<uint8_t> bufferV4;
    assert(WorldFormat::serializePlayer(pData, bufferV4) == SerializationResult::Success);

    PlayerSaveData loadedV4;
    assert(WorldFormat::deserializePlayer(bufferV4, loadedV4) == SerializationResult::Success);
    assert(loadedV4.schemaVersion == SCHEMA_VERSION_4);
    // Campos v5 adoptan valores por defecto seguros al cargar v4
    assert(loadedV4.saturation == 5.0f);
    assert(loadedV4.exhaustion == 0.0f);
    assert(loadedV4.air == 300);
    assert(loadedV4.isAlive == true);
    std::cout << "Paso 6: Retrocompatibilidad con mundos y jugadores de Esquema v4 confirmada." << std::endl;

    std::filesystem::remove_all(testDir);
    std::cout << ">>> [F06 Test 5] APROBADO AL 100% <<<" << std::endl;
    return 0;
}

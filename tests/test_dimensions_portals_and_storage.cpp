#include <iostream>
#include <cassert>
#include <filesystem>
#include "world/Dimension.hpp"
#include "world/PortalSystem.hpp"
#include "storage/WorldStorage.hpp"
#include "storage/WorldFormat.hpp"
#include "sim/Entity.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F12 Test 3] Dimensiones (Abismo, El Confín), Portales 8:1, Plataforma de obsidiana y "
                 "Persistencia Esquema v10 ---"
              << std::endl;

    BlockRegistry::global();
    ItemRegistry::global();

    // 1. Instanciación y generación procedural del Abismo
    std::cout << "[1] Verificando generación procedural del Abismo..." << std::endl;
    Dimension abyss(DimensionType::Abyss, 12345);
    abyss.generateChunk(0, 0);

    auto& reg = BlockRegistry::global();
    BlockStateId bedrockState = reg.getStateId("opencraft:bedrock");
    BlockStateId abyssalRockState = reg.getStateId("opencraft:abyssal_rock");

    assert(abyss.getWorld().getBlockState(0, 0, 0) == bedrockState && "Y=0 en Abismo debe ser roca madre");
    assert(abyss.getWorld().getBlockState(8, 127, 8) == bedrockState && "Y=127 en Abismo debe ser techo de roca madre");

    // Verificar presencia de netherrack
    bool foundAbyssalRock = false;
    for (int y = 35; y < 100; ++y) {
        if (abyss.getWorld().getBlockState(8, y, 8) == abyssalRockState) {
            foundAbyssalRock = true;
            break;
        }
    }
    assert(foundAbyssalRock && "El chunk del Abismo debe contener netherrack en su relieve");

    // 2. Instanciación y generación de El Confín (isla central y 10 pilares)
    std::cout << "[2] Verificando generación de El Confín (isla central y pilares)..." << std::endl;
    Dimension rim(DimensionType::Rim, 54321);
    rim.generateChunk(0, 0);

    BlockStateId rimStoneState = reg.getStateId("opencraft:rim_stone");
    assert(rim.getWorld().getBlockState(0, 60, 0) == rimStoneState && "El centro de El Confín debe ser end stone");

    // 3. Portales y escalado de coordenadas 8:1
    std::cout << "[3] Verificando escalado canónico 8:1 Overworld <-> Abismo..." << std::endl;
    Math::Vec3 overworldPos{800.0f, 64.0f, -400.0f};
    Math::Vec3 abyssTarget = PortalSystem::mapCoordinates(DimensionType::Overworld, DimensionType::Abyss, overworldPos);
    assert(abyssTarget.x == 100.0f && abyssTarget.z == -50.0f && "Overworld -> Abismo debe dividir X y Z entre 8");

    Math::Vec3 overworldReturn =
        PortalSystem::mapCoordinates(DimensionType::Abyss, DimensionType::Overworld, abyssTarget);
    assert(overworldReturn.x == 800.0f && overworldReturn.z == -400.0f &&
           "Abismo -> Overworld debe multiplicar X y Z por 8");

    // 4. Portal del Confín y plataforma fija de obsidiana en (100, 48, 0)
    std::cout << "[4] Verificando portal del Confín y plataforma de obsidiana..." << std::endl;
    World endWorld;
    Math::Vec3 endSpawn = PortalSystem::setupEndSpawnPlatform(endWorld);
    assert(endSpawn.x == 100.0f && endSpawn.y == 49.0f && endSpawn.z == 0.0f &&
           "La plataforma de El Confín debe ubicar al jugador en (100, 49, 0)");

    BlockStateId obsidianState = reg.getStateId("opencraft:obsidian");
    assert(endWorld.getBlockState(100, 48, 0) == obsidianState && "La plataforma debe ser de obsidiana en Y=48");

    // 5. Marco de portal del Confín: activación con Ojo del Confín
    std::cout << "[5] Verificando activación de portal del Confín..." << std::endl;
    BlockPos framePos{0, 64, 0};
    endWorld.setBlockState(0, 64, 0, reg.getStateId("opencraft:rim_portal_frame"));
    bool activated = PortalSystem::insertRimEye(endWorld, framePos);
    assert(activated && "Insertar Ojo del Confín en el marco debe devolver éxito");

    // 6. Persistencia y almacenamiento multidimensional Esquema v10
    std::cout << "[6] Verificando persistencia y almacenamiento multidimensional Esquema v10..." << std::endl;
    std::filesystem::path tempWorld = std::filesystem::temp_directory_path() / "opencraft_f12_test_world";
    std::filesystem::remove_all(tempWorld);

    WorldStorage storage(tempWorld);
    assert(storage.init() == StorageError::None);

    // Guardar chunk en Overworld
    Chunk owChunk(0, 0);
    owChunk.setBlockState(0, 10, 0, reg.getStateId("opencraft:stone"));
    assert(storage.beginTransaction() == StorageError::None);
    assert(storage.stageChunk(owChunk, {}, DimensionType::Overworld) == StorageError::None);
    assert(storage.commitTransaction() == StorageError::None);
    assert(storage.hasChunk(0, 0, DimensionType::Overworld));

    // Guardar chunk en Abismo
    Chunk nChunk(0, 0);
    nChunk.setBlockState(0, 10, 0, abyssalRockState);
    assert(storage.beginTransaction() == StorageError::None);
    assert(storage.stageChunk(nChunk, {}, DimensionType::Abyss) == StorageError::None);
    assert(storage.commitTransaction() == StorageError::None);
    assert(storage.hasChunk(0, 0, DimensionType::Abyss));

    // Guardar chunk en El Confín
    Chunk eChunk(0, 0);
    eChunk.setBlockState(0, 10, 0, rimStoneState);
    assert(storage.beginTransaction() == StorageError::None);
    assert(storage.stageChunk(eChunk, {}, DimensionType::Rim) == StorageError::None);
    assert(storage.commitTransaction() == StorageError::None);
    assert(storage.hasChunk(0, 0, DimensionType::Rim));

    // Cargar y comprobar aislamiento de dimensiones
    Chunk loadedAbyss(0, 0);
    assert(storage.loadChunk(0, 0, loadedAbyss, reg, nullptr, DimensionType::Abyss) == StorageError::None);
    assert(loadedAbyss.getBlockState(0, 10, 0) == abyssalRockState &&
           "El chunk cargado del Abismo debe mantener netherrack");

    Chunk loadedEnd(0, 0);
    assert(storage.loadChunk(0, 0, loadedEnd, reg, nullptr, DimensionType::Rim) == StorageError::None);
    assert(loadedEnd.getBlockState(0, 10, 0) == rimStoneState && "El chunk cargado del Confín debe mantener end stone");

    // Guardar y cargar datos de jugador con Esquema v10 (dimensión, vuelo, efectos activos)
    PlayerSaveData pdata;
    pdata.schemaVersion = CURRENT_SCHEMA_VERSION; // 10
    pdata.currentDimension = static_cast<uint8_t>(DimensionType::Rim);
    pdata.isGliding = true;
    pdata.activeEffects.push_back(
        PlayerSaveData::ActiveEffectSaveData{.type = static_cast<uint8_t>(EffectType::SlowFalling),
                                             .durationTicks = 300,
                                             .amplifier = 0,
                                             .isAmbient = false});

    assert(storage.beginTransaction() == StorageError::None);
    assert(storage.stagePlayer(pdata) == StorageError::None);
    assert(storage.commitTransaction() == StorageError::None);

    PlayerSaveData loadedPlayer;
    assert(storage.loadPlayer(loadedPlayer) == StorageError::None);
    assert(loadedPlayer.schemaVersion >= 10);
    assert(loadedPlayer.currentDimension == static_cast<uint8_t>(DimensionType::Rim));
    assert(loadedPlayer.isGliding == true);
    assert(loadedPlayer.activeEffects.size() == 1);
    assert(loadedPlayer.activeEffects[0].type == static_cast<uint8_t>(EffectType::SlowFalling));

    // Limpieza
    std::filesystem::remove_all(tempWorld);

    std::cout << "--- [F12 Test 3] COMPLETADO EXITOSAMENTE ---" << std::endl;
    return 0;
}

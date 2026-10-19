#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <cmath>
#include "sim/WorldSession.hpp"
#include "storage/WorldStorage.hpp"
#include "world/PortalSystem.hpp"
#include "item/Item.hpp"

using namespace OpenCraft;

// Regresiones de la auditoría A06: la dimensión, sus chunks y sus entidades deben sobrevivir al
// guardado/carga, y los portales deben transferir al jugador entre mundos reales.

static void buildAbyssFrame(World& world, int x, int y, int z) {
    // Marco de 4x5 en el eje X con esquinas; interior libre de 2x3 en (x+1..x+2, y+1..y+3)
    for (int h = 0; h < 5; ++h) {
        for (int w = 0; w < 4; ++w) {
            bool border = (w == 0 || w == 3 || h == 0 || h == 4);
            world.setBlock(x + w, y + h, z, border ? BlockID::Obsidian : BlockID::Air);
        }
    }
    // Suelo de apoyo frente al marco
    for (int w = -1; w <= 4; ++w) {
        for (int dz = -2; dz <= 2; ++dz) {
            if (dz != 0) world.setBlock(x + w, y, z + dz, BlockID::Stone);
        }
    }
}

static void tickN(WorldSession& session, int n) {
    for (int i = 0; i < n; ++i) {
        session.tick();
    }
}

void test_dimension_survives_save_and_load(const std::filesystem::path& dir) {
    std::cout << "--- Subtest 1: La dimensión, efectos y planeo persisten ---" << std::endl;
    std::filesystem::remove_all(dir);
    WorldStorage storage(dir);
    assert(storage.initializeWorld("DimWorld", 123) == StorageError::None);

    {
        WorldSession session;
        session.init(1, 123);
        bool changed = session.setCurrentDimension(2);
        assert(changed);
        assert(session.getCurrentDimension() == 2);
        assert(session.getPlayer().dimension == 2);
        // El mundo activo es el Confín: isla central de piedra del Confín
        assert(session.getWorld().getBlock(0, 58, 0) == BlockID::RimStone);

        session.getPlayer().isGliding = true;
        session.getPlayer().statusEffects.addEffect(
            ActiveEffect{.type = EffectType::SlowFalling, .durationTicks = 600, .amplifier = 1, .isAmbient = false});
        StorageError saveErr = session.saveToStorage(storage);
        assert(saveErr == StorageError::None);
    }

    assert(std::filesystem::exists(storage.getChunkPath(0, 0, DimensionType::Rim)));

    WorldSession loaded;
    loaded.init(1, 123);
    StorageError loadErr = loaded.loadFromStorage(storage);
    assert(loadErr == StorageError::None);
    assert(loaded.getCurrentDimension() == 2);
    assert(loaded.getPlayer().dimension == 2);
    assert(loaded.getWorld().getBlock(0, 58, 0) == BlockID::RimStone);
    assert(loaded.getPlayer().isGliding);
    const ActiveEffect* eff = loaded.getPlayer().statusEffects.getEffect(EffectType::SlowFalling);
    assert(eff != nullptr);
    assert(eff->durationTicks == 600);
    assert(eff->amplifier == 1);
    std::cout << "[OK] Dimensión 2, efecto y planeo restaurados tras recargar" << std::endl;
}

void test_each_dimension_keeps_its_chunks_and_items(const std::filesystem::path& dir) {
    std::cout << "--- Subtest 2: Cada dimensión conserva sus chunks y objetos ---" << std::endl;
    std::filesystem::remove_all(dir);
    WorldStorage storage(dir);
    assert(storage.initializeWorld("DimWorld", 7) == StorageError::None);

    ItemId diamondId = ItemRegistry::global().getId("opencraft:diamond");
    assert(diamondId != EMPTY_ITEM_ID);

    {
        WorldSession session;
        session.init(1, 7);
        session.attachStorage(&storage);
        session.getWorld().setBlock(3, 90, 3, BlockID::GlowRock);
        session.spawnDroppedItem(ItemStack(diamondId, 3), {4.0f, 91.0f, 4.0f});

        bool changed = session.changeDimension(DimensionType::Abyss, {0.5f, 64.0f, 0.5f});
        assert(changed);
        // El Overworld queda aparcado: su bloque y su objeto no están en el mundo activo
        assert(session.getWorld().getBlock(3, 90, 3) != BlockID::GlowRock);
        assert(session.getItemEntities().empty());
        session.getWorld().setBlock(5, 60, 5, BlockID::Glass);

        StorageError saveErr = session.saveToStorage(storage);
        assert(saveErr == StorageError::None);
    }

    // Los chunks de cada dimensión están en su carpeta y no se pisan entre sí
    assert(std::filesystem::exists(storage.getChunkPath(0, 0, DimensionType::Overworld)));
    assert(std::filesystem::exists(storage.getChunkPath(0, 0, DimensionType::Abyss)));

    WorldSession loaded;
    loaded.init(1, 7);
    StorageError loadErr = loaded.loadFromStorage(storage);
    assert(loadErr == StorageError::None);
    assert(loaded.getCurrentDimension() == 1);
    assert(loaded.getWorld().getBlock(5, 60, 5) == BlockID::Glass);
    assert(loaded.getItemEntities().empty());

    // Volver al Overworld: se carga del disco (no se regenera) junto con su objeto aparcado
    bool back = loaded.changeDimension(DimensionType::Overworld, {8.0f, 92.0f, 8.0f});
    assert(back);
    assert(loaded.getWorld().getBlock(3, 90, 3) == BlockID::GlowRock);
    assert(loaded.getItemEntities().size() == 1);
    assert(loaded.getItemEntities()[0].stack.itemId == diamondId);
    assert(loaded.getItemEntities()[0].stack.count == 3);
    std::cout << "[OK] Overworld y Abismo recuperan sus chunks y objetos por separado" << std::endl;
}

void test_unreadable_target_dimension_aborts_transfer(const std::filesystem::path& dir) {
    std::cout << "--- Subtest 3: Un chunk de destino ilegible bloquea la transferencia ---" << std::endl;
    std::filesystem::remove_all(dir);
    WorldStorage storage(dir);
    assert(storage.initializeWorld("DimWorld", 9) == StorageError::None);

    {
        WorldSession session;
        session.init(1, 9);
        bool changed = session.setCurrentDimension(1);
        assert(changed);
        assert(session.saveToStorage(storage) == StorageError::None);
    }

    // Corromper un chunk del Abismo
    auto abyssChunk = storage.getChunkPath(0, 0, DimensionType::Abyss);
    {
        std::ofstream out(abyssChunk, std::ios::binary | std::ios::trunc);
        out << "basura";
    }

    WorldSession session;
    session.init(1, 9);
    session.attachStorage(&storage);
    session.getWorld().setBlock(2, 80, 2, BlockID::GlowRock);
    bool changed = session.changeDimension(DimensionType::Abyss, {0.5f, 64.0f, 0.5f});
    assert(!changed);
    assert(session.getCurrentDimension() == 0);
    assert(session.getWorld().getBlock(2, 80, 2) == BlockID::GlowRock);

    // El archivo dañado no se sobrescribe con terreno regenerado
    assert(session.saveToStorage(storage) == StorageError::None);
    std::ifstream in(abyssChunk, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    assert(content == "basura");
    std::cout << "[OK] Transferencia rechazada sin alterar la sesión ni el chunk dañado" << std::endl;
}

void test_abyss_portal_round_trip_without_commands() {
    std::cout << "--- Subtest 4: Portal del Abismo encendido con mechero, ida y vuelta ---" << std::endl;
    auto& itemReg = ItemRegistry::global();
    ItemId flintAndSteelId = itemReg.getId("opencraft:flint_and_steel");
    assert(flintAndSteelId != EMPTY_ITEM_ID);

    WorldSession session;
    session.init(2, 31);
    World& overworld = session.getWorld();

    const int fx = 16, fy = 100, fz = 16;
    buildAbyssFrame(overworld, fx, fy, fz);

    PlayerEntity& player = session.getPlayer();
    player.gameMode = 0;
    player.position = {static_cast<float>(fx) + 1.5f, static_cast<float>(fy) + 1.0f, static_cast<float>(fz) - 1.5f};
    player.prevPosition = player.position;
    player.inventory.setSlot(player.selectedSlot, ItemStack(flintAndSteelId, 1));

    // Un marco incompleto no se enciende
    overworld.setBlock(fx + 3, fy + 2, fz, BlockID::Air);
    CommandResult rejected = session.executeCommand(InteractBlockCommand{.x = fx + 1, .y = fy, .z = fz});
    assert(rejected == CommandResult::Rejected);
    assert(overworld.getBlock(fx + 1, fy + 1, fz) == BlockID::Air);
    overworld.setBlock(fx + 3, fy + 2, fz, BlockID::Obsidian);

    CommandResult lit = session.executeCommand(InteractBlockCommand{.x = fx + 1, .y = fy, .z = fz});
    assert(lit == CommandResult::Success);
    for (int h = 1; h <= 3; ++h) {
        assert(overworld.getBlock(fx + 1, fy + h, fz) == BlockID::AbyssPortal);
        assert(overworld.getBlock(fx + 2, fy + h, fz) == BlockID::AbyssPortal);
    }
    assert(player.inventory.getSlot(player.selectedSlot).durability == 1);

    // Entrar al portal y esperar el tiempo de Supervivencia
    player.position = {static_cast<float>(fx) + 1.5f, static_cast<float>(fy) + 1.0f, static_cast<float>(fz) + 0.5f};
    player.prevPosition = player.position;
    tickN(session, static_cast<int>(PortalSystem::ABYSS_PORTAL_WAIT_TICKS) - 5);
    assert(session.getCurrentDimension() == 0);
    tickN(session, 10);
    assert(session.getCurrentDimension() == 1);
    assert(player.dimension == 1);
    // Coordenadas escaladas 8:1 y llegada sobre un portal de regreso
    assert(std::abs(player.position.x - (static_cast<float>(fx) + 1.5f) / 8.0f) < 4.0f);
    int px = static_cast<int>(std::floor(player.position.x));
    int py = static_cast<int>(std::floor(player.position.y));
    int pz = static_cast<int>(std::floor(player.position.z));
    assert(session.getWorld().getBlock(px, py, pz) == BlockID::AbyssPortal);
    // El Overworld queda aparcado con el portal encendido
    assert(session.isDimensionResident(DimensionType::Overworld));

    // Permanecer en el portal de llegada: tras el enfriamiento se regresa al Overworld
    tickN(session, static_cast<int>(PortalSystem::PORTAL_COOLDOWN_TICKS + PortalSystem::ABYSS_PORTAL_WAIT_TICKS) + 5);
    assert(session.getCurrentDimension() == 0);
    assert(session.getWorld().getBlock(fx + 1, fy + 1, fz) == BlockID::AbyssPortal);
    std::cout << "[OK] Overworld -> Abismo -> Overworld mediante portal encendido en juego" << std::endl;
}

void test_rim_portal_round_trip() {
    std::cout << "--- Subtest 5: Portal del Confín hacia la plataforma y salida al Overworld ---" << std::endl;
    WorldSession session;
    session.init(1, 55);
    PlayerEntity& player = session.getPlayer();
    player.gameMode = 0;
    player.position = {4.5f, 90.0f, 4.5f};
    player.prevPosition = player.position;
    session.getWorld().setBlock(4, 89, 4, BlockID::Stone);
    session.getWorld().setBlock(4, 90, 4, BlockID::RimPortal);

    session.tick();
    assert(session.getCurrentDimension() == 2);
    assert(session.getWorld().getBlock(100, 48, 0) == BlockID::Obsidian);
    assert(std::abs(player.position.x - 100.0f) < 0.01f);

    // Portal de salida en el Confín, tras el enfriamiento
    int ex = static_cast<int>(std::floor(player.position.x));
    int ey = static_cast<int>(std::floor(player.position.y));
    int ez = static_cast<int>(std::floor(player.position.z));
    session.getWorld().setBlock(ex, ey, ez, BlockID::RimPortal);
    session.tick();
    assert(session.getCurrentDimension() == 2);
    tickN(session, static_cast<int>(PortalSystem::PORTAL_COOLDOWN_TICKS));
    assert(session.getCurrentDimension() == 0);
    assert(player.dimension == 0);
    std::cout << "[OK] Overworld -> Fin -> Overworld mediante portales del Confín" << std::endl;
}

int main() {
    auto base = std::filesystem::temp_directory_path() / "opencraft_test_dimension_travel";
    test_dimension_survives_save_and_load(base / "persist");
    test_each_dimension_keeps_its_chunks_and_items(base / "separate");
    test_unreadable_target_dimension_aborts_transfer(base / "corrupt");
    test_abyss_portal_round_trip_without_commands();
    test_rim_portal_round_trip();
    std::filesystem::remove_all(base);
    std::cout << "Todas las pruebas de viaje y persistencia de dimensiones superadas." << std::endl;
    return 0;
}

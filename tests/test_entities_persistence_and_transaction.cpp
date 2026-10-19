#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <cstring>
#include "sim/WorldSession.hpp"
#include "storage/WorldStorage.hpp"
#include "storage/WorldFormat.hpp"
#include "item/Item.hpp"

using namespace OpenCraft;

void test_components_preservation_and_checksum(const std::filesystem::path& testDir) {
    std::cout << "--- Subtest 1: Preservación de componentes y verificación CRC32 ---" << std::endl;
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);

    auto& itemReg = ItemRegistry::global();
    ItemId swordId = itemReg.getId("opencraft:diamond_sword");
    assert(swordId != EMPTY_ITEM_ID);

    WorldStorage storage(testDir);
    assert(storage.initializeWorld("AuditWorld", 42) == StorageError::None);

    // 1. Guardar una entidad de ítem con componentes
    {
        WorldSession session;
        session.init(1, 42);

        ItemStack stack(swordId, 1, 10, {0xDE, 0xAD, 0xBE, 0xEF, 0x42});
        session.spawnDroppedItem(std::move(stack), {10.0f, 65.0f, 10.0f}, {0.0f, 0.1f, 0.0f}, 15);

        assert(session.getItemEntities().size() == 1);
        assert(session.getItemEntities()[0].stack.components.size() == 5);

        StorageError saveErr = session.saveToStorage(storage);
        assert(saveErr == StorageError::None);
    }

    // 2. Verificar que el archivo existe en entities/items.dat y contiene cabecera mágica OCEN
    std::filesystem::path itemsPath = storage.getEntitiesPath();
    assert(std::filesystem::exists(itemsPath));

    {
        std::ifstream in(itemsPath, std::ios::binary);
        assert(in.is_open());
        uint32_t magic = 0;
        in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        assert(magic == MAGIC_ENTITIES && "El archivo debe tener el Magic Byte MAGIC_ENTITIES (OCEN)");
    }

    // 3. Recargar en una nueva sesión y verificar que los componentes se restauraron al 100%
    {
        WorldSession sessionReload;
        sessionReload.init(1, 42);
        StorageError loadErr = sessionReload.loadFromStorage(storage);
        assert(loadErr == StorageError::None);

        const auto& items = sessionReload.getItemEntities();
        assert(items.size() == 1);
        assert(items[0].stack.itemId == swordId);
        assert(items[0].stack.count == 1);
        assert(items[0].stack.durability == 10);
        assert(items[0].pickupDelayTicks == 15);
        assert(items[0].stack.components.size() == 5);
        std::vector<uint8_t> expectedComps = {0xDE, 0xAD, 0xBE, 0xEF, 0x42};
        assert(items[0].stack.components == expectedComps);
        std::cout << "Paso 1.1: Componentes restaurados exitosamente e idénticos byte a byte." << std::endl;
    }

    // 4. Corromper 1 byte dentro del payload de entities/items.dat y verificar ChecksumMismatch
    {
        std::fstream file(itemsPath, std::ios::in | std::ios::out | std::ios::binary);
        assert(file.is_open());
        file.seekp(16, std::ios::beg); // Dentro del payload (offset >= 12)
        char originalByte = 0;
        file.read(&originalByte, 1);
        char corruptedByte = static_cast<char>(originalByte ^ 0xFF);
        file.seekp(16, std::ios::beg);
        file.write(&corruptedByte, 1);
        file.close();

        WorldSession corruptSession;
        corruptSession.init(1, 42);
        StorageError err = corruptSession.loadFromStorage(storage);
        assert(err == StorageError::ChecksumMismatch &&
               "Un archivo de entidades alterado debe disparar ChecksumMismatch");
        std::cout << "Paso 1.2: ChecksumMismatch detectado correctamente ante corrupción de datos." << std::endl;
    }
}

void test_death_cycle_and_components(const std::filesystem::path& testDir) {
    std::cout << "--- Subtest 2: Muerte con ítems complejos y persistencia de drops ---" << std::endl;
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);

    auto& itemReg = ItemRegistry::global();
    ItemId swordId = itemReg.getId("opencraft:diamond_sword");
    ItemId potionId = itemReg.getId("opencraft:diamond"); // recurso con metadatos

    WorldStorage storage(testDir);
    assert(storage.initializeWorld("DeathWorld", 100) == StorageError::None);

    // 1. Equipar al jugador con objetos que contienen componentes
    {
        WorldSession session;
        session.init(1, 100);
        PlayerEntity& player = session.getPlayer();
        player.gameMode = 0; // Supervivencia

        ItemStack customSword(swordId, 1, 0, {0x01, 0x02, 0x03, 0x04});
        ItemStack customItem(potionId, 5, 0, {0xAA, 0xBB});

        player.inventory.setSlot(0, customSword);
        player.inventory.setSlot(1, customItem);
        player.cursorItem = ItemStack(potionId, 2, 0, {0xCC});
        player.syncHotbar();

        // Aplicar daño letal
        player.position = Math::Vec3(10.0f, 65.0f, 10.0f);
        session.applyDamage(100.0f, DamageCause::Fall);

        assert(!player.isAlive);
        assert(player.inventory.isEmpty());
        assert(player.cursorItem.isEmpty());

        // Verificar que los objetos fueron soltados en el suelo
        assert(session.getItemEntities().size() >= 2);

        // Guardar durante la transición de muerte
        assert(session.saveToStorage(storage) == StorageError::None);
    }

    // 2. Recargar en nueva sesión: inventario del jugador vacío, drops con componentes intactos
    {
        WorldSession reloadSession;
        reloadSession.init(1, 100);
        assert(reloadSession.loadFromStorage(storage) == StorageError::None);

        PlayerEntity& reloadPlayer = reloadSession.getPlayer();
        assert(!reloadPlayer.isAlive);
        assert(reloadPlayer.inventory.isEmpty());
        assert(reloadPlayer.cursorItem.isEmpty());

        const auto& dropped = reloadSession.getItemEntities();
        assert(dropped.size() >= 2);

        bool foundSword = false;
        bool foundCustom = false;
        for (const auto& item : dropped) {
            if (item.stack.itemId == swordId) {
                std::vector<uint8_t> expectedSword = {0x01, 0x02, 0x03, 0x04};
                assert(item.stack.components == expectedSword);
                foundSword = true;
            } else if (item.stack.itemId == potionId && item.stack.count == 5) {
                std::vector<uint8_t> expectedCustom = {0xAA, 0xBB};
                assert(item.stack.components == expectedCustom);
                foundCustom = true;
            }
        }
        assert(foundSword && foundCustom);

        // Reaparición segura
        CommandResult respawn = reloadSession.executeCommand(RespawnCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
        assert(respawn == CommandResult::Success);
        assert(reloadPlayer.isAlive);
        assert(reloadPlayer.inventory.isEmpty());
        std::cout << "Paso 2.1: Ciclo de muerte completado con preservación íntegra de metadatos." << std::endl;
    }
}

void test_pickup_and_inventory_roundtrip(const std::filesystem::path& testDir) {
    std::cout << "--- Subtest 3: Recogida de ítem con componentes y ciclo suelo-inventario-suelo ---" << std::endl;
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);

    auto& itemReg = ItemRegistry::global();
    ItemId swordId = itemReg.getId("opencraft:diamond_sword");

    WorldStorage storage(testDir);
    assert(storage.initializeWorld("PickupWorld", 200) == StorageError::None);

    std::vector<uint8_t> testComps = {0x77, 0x88, 0x99};

    // 1. Crear sesión con ítem en el suelo con retardo de recogida 0
    {
        WorldSession session;
        session.init(1, 200);
        session.getPlayer().inventory.clear();
        session.getPlayer().syncHotbar();
        session.getPlayer().position = Math::Vec3(0.0f, 10.0f, 0.0f);

        ItemStack stack(swordId, 1, 5, testComps);
        session.spawnDroppedItem(std::move(stack), Math::Vec3(0.0f, 10.0f, 0.0f), {0, 0, 0}, 0);

        assert(session.getItemEntities().size() == 1);

        // Ejecutar 1 tick de simulación para que el jugador recoja el ítem
        session.update(0.05f);

        assert(session.getItemEntities().empty() && "El ítem debió ser recogido del suelo");
        const auto& slot0 = session.getPlayer().inventory.getSlot(0);
        assert(slot0.itemId == swordId);
        assert(slot0.components == testComps && "El inventario del jugador debe tener los componentes");

        // Guardar a almacenamiento
        assert(session.saveToStorage(storage) == StorageError::None);
    }

    // 2. Recargar y verificar que el inventario del jugador conserva los componentes
    {
        WorldSession reloadSession;
        reloadSession.init(1, 200);
        assert(reloadSession.loadFromStorage(storage) == StorageError::None);

        assert(reloadSession.getItemEntities().empty());
        const auto& slot0 = reloadSession.getPlayer().inventory.getSlot(0);
        assert(slot0.itemId == swordId);
        assert(slot0.components == testComps);

        // Tirar el ítem al suelo de nuevo
        ItemStack toDrop = reloadSession.getPlayer().inventory.removeItem(0, 1);
        reloadSession.spawnDroppedItem(std::move(toDrop), Math::Vec3(5.0f, 10.0f, 5.0f), {0, 0, 0}, 20);

        assert(reloadSession.getItemEntities().size() == 1);
        assert(reloadSession.saveToStorage(storage) == StorageError::None);
    }

    // 3. Recargar y verificar que el ítem tirado en el suelo mantiene los componentes
    {
        WorldSession finalSession;
        finalSession.init(1, 200);
        assert(finalSession.loadFromStorage(storage) == StorageError::None);

        assert(finalSession.getPlayer().inventory.isEmpty());
        const auto& items = finalSession.getItemEntities();
        assert(items.size() == 1);
        assert(items[0].stack.itemId == swordId);
        assert(items[0].stack.components == testComps);
        std::cout << "Paso 3.1: Ciclo bidireccional suelo-inventario-suelo verificado con éxito." << std::endl;
    }
}

void test_inter_file_crash_and_journal_recovery(const std::filesystem::path& testDir) {
    std::cout << "--- Subtest 4: Recuperación de journal ante fallos entre archivos ---" << std::endl;
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);

    WorldStorage storage(testDir);
    assert(storage.initializeWorld("JournalWorld", 300) == StorageError::None);

    // =========================================================================
    // Escenario A: Caída ANTES del commit con journal PENDING (0)
    // =========================================================================
    {
        std::filesystem::path itemsTmp = storage.getEntitiesPath().string() + ".tmp";
        std::ofstream fItems(itemsTmp, std::ios::binary);
        fItems << "INCOMPLETE_ENTITIES_DATA";
        fItems.close();

        // Escribir journal con status = 0 (PENDING)
        std::vector<uint8_t> journalPending;
        journalPending.push_back(static_cast<uint8_t>(MAGIC_JOURNAL & 0xFF));
        journalPending.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 8) & 0xFF));
        journalPending.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 16) & 0xFF));
        journalPending.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 24) & 0xFF));
        journalPending.push_back(0); // PENDING
        // count = 1
        journalPending.push_back(1);
        journalPending.push_back(0);

        std::string relTmp = "entities/items.dat.tmp";
        std::string relFinal = "entities/items.dat";

        uint16_t tLen = static_cast<uint16_t>(relTmp.size());
        journalPending.push_back(static_cast<uint8_t>(tLen & 0xFF));
        journalPending.push_back(static_cast<uint8_t>((tLen >> 8) & 0xFF));
        for (char c : relTmp) journalPending.push_back(static_cast<uint8_t>(c));

        uint16_t fLen = static_cast<uint16_t>(relFinal.size());
        journalPending.push_back(static_cast<uint8_t>(fLen & 0xFF));
        journalPending.push_back(static_cast<uint8_t>((fLen >> 8) & 0xFF));
        for (char c : relFinal) journalPending.push_back(static_cast<uint8_t>(c));

        std::ofstream jOut(storage.getJournalPath(), std::ios::binary | std::ios::trunc);
        jOut.write(reinterpret_cast<const char*>(journalPending.data()), journalPending.size());
        jOut.close();

        // Al iniciar o recuperar el almacenamiento, se debe descartar el archivo temporal incompleto
        StorageError recErr = storage.recoverJournalIfNeeded();
        assert(recErr == StorageError::None);
        assert(!std::filesystem::exists(itemsTmp) && "El archivo .tmp debió eliminarse tras rollback");
        assert(!std::filesystem::exists(storage.getJournalPath()) && "El journal debe limpiarse");
        std::cout << "Paso 4.1: Interrupción previa a confirmación descartó temporales correctamente." << std::endl;
    }

    // =========================================================================
    // Escenario B: Caída TRAS el commit con journal COMMITTED (1)
    // =========================================================================
    {
        // Simular que items.dat.tmp fue escrito con formato válido, y el journal se guardó con status=1
        auto& itemReg = ItemRegistry::global();
        ItemId swordId = itemReg.getId("opencraft:diamond_sword");

        std::vector<ItemEntitySaveData> entities;
        ItemEntitySaveData ed;
        ed.id = 99;
        ed.position = {1, 2, 3};
        ed.velocity = {0, 0, 0};
        ed.pickupDelayTicks = 10;
        ed.stack.itemName = itemReg.get(swordId).namespacedId;
        ed.stack.count = 1;
        ed.stack.durability = 0;
        ed.stack.components = {0xFE, 0xED};
        entities.push_back(ed);

        std::vector<uint8_t> validData;
        assert(WorldFormat::serializeEntities(entities, validData) == SerializationResult::Success);

        std::filesystem::path itemsTmp = storage.getEntitiesPath().string() + ".tmp";
        std::filesystem::path itemsFinal = storage.getEntitiesPath();
        std::error_code ec;
        std::filesystem::remove(itemsFinal, ec);

        std::ofstream fItems(itemsTmp, std::ios::binary);
        fItems.write(reinterpret_cast<const char*>(validData.data()), validData.size());
        fItems.close();

        // Escribir journal con status = 1 (COMMITTED)
        std::vector<uint8_t> journalCommitted;
        journalCommitted.push_back(static_cast<uint8_t>(MAGIC_JOURNAL & 0xFF));
        journalCommitted.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 8) & 0xFF));
        journalCommitted.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 16) & 0xFF));
        journalCommitted.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 24) & 0xFF));
        journalCommitted.push_back(1); // COMMITTED
        journalCommitted.push_back(1); // count = 1
        journalCommitted.push_back(0);

        std::string relTmp = "entities/items.dat.tmp";
        std::string relFinal = "entities/items.dat";

        uint16_t tLen = static_cast<uint16_t>(relTmp.size());
        journalCommitted.push_back(static_cast<uint8_t>(tLen & 0xFF));
        journalCommitted.push_back(static_cast<uint8_t>((tLen >> 8) & 0xFF));
        for (char c : relTmp) journalCommitted.push_back(static_cast<uint8_t>(c));

        uint16_t fLen = static_cast<uint16_t>(relFinal.size());
        journalCommitted.push_back(static_cast<uint8_t>(fLen & 0xFF));
        journalCommitted.push_back(static_cast<uint8_t>((fLen >> 8) & 0xFF));
        for (char c : relFinal) journalCommitted.push_back(static_cast<uint8_t>(c));

        std::ofstream jOut(storage.getJournalPath(), std::ios::binary | std::ios::trunc);
        jOut.write(reinterpret_cast<const char*>(journalCommitted.data()), journalCommitted.size());
        jOut.close();

        // Recuperar journal: debe completar el renombre atómico pendiente
        StorageError recErr = storage.recoverJournalIfNeeded();
        assert(recErr == StorageError::None);
        assert(!std::filesystem::exists(itemsTmp) && "El archivo .tmp fue renombrado");
        assert(std::filesystem::exists(itemsFinal) && "El destino final debe existir");

        // Cargar y comprobar que los datos recuperados son válidos
        std::vector<ItemEntitySaveData> loaded;
        assert(storage.loadEntities(loaded) == StorageError::None);
        assert(loaded.size() == 1);
        assert(loaded[0].id == 99);
        assert(loaded[0].stack.components.size() == 2);
        assert((loaded[0].stack.components == std::vector<uint8_t>{0xFE, 0xED}));
        std::cout << "Paso 4.2: Recuperación de transacción confirmada completó renombres con éxito." << std::endl;
    }
}

int main() {
    std::cout << "=== Test Suite: Persistencia Transaccional y Versionada de Entidades (A02) ===" << std::endl;

    std::filesystem::path baseDir = "build/test_a02_entities_env";
    test_components_preservation_and_checksum(baseDir / "checksum");
    test_death_cycle_and_components(baseDir / "death");
    test_pickup_and_inventory_roundtrip(baseDir / "pickup");
    test_inter_file_crash_and_journal_recovery(baseDir / "journal");

    std::error_code ec;
    std::filesystem::remove_all(baseDir, ec);

    std::cout << ">>> [TEST PASSED] Persistencia y Transacciones de Entidades Aprobadas al 100% <<<" << std::endl;
    return 0;
}

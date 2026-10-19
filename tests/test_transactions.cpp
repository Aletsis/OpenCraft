#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include "storage/WorldStorage.hpp"
#include "world/Chunk.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F02 Test 2] Transacciones atómicas, recuperación de journal y resistencia a caídas ---"
              << std::endl;

    std::filesystem::path testDir = "build/test_transaction_recovery";
    std::error_code ec;
    std::filesystem::remove_all(testDir, ec);

    WorldStorage storage(testDir);
    storage.init();

    // 1. Crear un estado base coherente confirmado (Versión Inicial)
    WorldMetadata metaInitial;
    metaInitial.worldName = "Trans World Initial";
    metaInitial.seed = 1111;

    Chunk chunkInitial(0, 0);
    chunkInitial.generateTerrain(1111);

    storage.beginTransaction();
    storage.stageMetadata(metaInitial);
    storage.stageChunk(chunkInitial);
    StorageError err = storage.commitTransaction();
    assert(err == StorageError::None);
    (void)err;

    assert(std::filesystem::exists(storage.getMetadataPath()));
    assert(std::filesystem::exists(storage.getChunkPath(0, 0)));

    // =========================================================================
    // Escenario A: Caída ANTES del commit (status = PENDING)
    // =========================================================================
    {
        // Simular que el proceso escribió archivos .tmp y un journal PENDING (0), y cayó antes del commit
        std::filesystem::path metaTmp = storage.getMetadataPath().string() + ".tmp";
        std::filesystem::path chunkTmp = storage.getChunkPath(0, 0).string() + ".tmp";

        std::ofstream fMeta(metaTmp, std::ios::binary);
        fMeta << "CORRUPTED_INCOMPLETE_METADATA_TMP";
        fMeta.close();

        std::ofstream fChunk(chunkTmp, std::ios::binary);
        fChunk << "CORRUPTED_INCOMPLETE_CHUNK_TMP";
        fChunk.close();

        // Escribir journal con status = 0 (PENDING)
        std::vector<uint8_t> journalPending;
        journalPending.push_back(static_cast<uint8_t>(MAGIC_JOURNAL & 0xFF));
        journalPending.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 8) & 0xFF));
        journalPending.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 16) & 0xFF));
        journalPending.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 24) & 0xFF));
        journalPending.push_back(0); // PENDING
        // count = 2
        journalPending.push_back(2);
        journalPending.push_back(0);

        auto addEntry = [&](const std::string& tmpRel, const std::string& finalRel) {
            uint16_t tLen = static_cast<uint16_t>(tmpRel.size());
            journalPending.push_back(static_cast<uint8_t>(tLen & 0xFF));
            journalPending.push_back(static_cast<uint8_t>((tLen >> 8) & 0xFF));
            for (char c : tmpRel) journalPending.push_back(static_cast<uint8_t>(c));

            uint16_t fLen = static_cast<uint16_t>(finalRel.size());
            journalPending.push_back(static_cast<uint8_t>(fLen & 0xFF));
            journalPending.push_back(static_cast<uint8_t>((fLen >> 8) & 0xFF));
            for (char c : finalRel) journalPending.push_back(static_cast<uint8_t>(c));
        };

        addEntry("world.meta.tmp", "world.meta");
        addEntry("chunks/c.0.0.dat.tmp", "chunks/c.0.0.dat");

        std::ofstream jOut(storage.getJournalPath(), std::ios::binary);
        jOut.write(reinterpret_cast<const char*>(journalPending.data()), journalPending.size());
        jOut.close();

        assert(std::filesystem::exists(metaTmp));
        assert(std::filesystem::exists(chunkTmp));
        assert(std::filesystem::exists(storage.getJournalPath()));

        // Recuperar en reinicio: debe descartar .tmp y journal, preservando el estado inicial
        StorageError recErr = storage.recoverJournalIfNeeded();
        assert(recErr == StorageError::None);
        (void)recErr;

        assert(!std::filesystem::exists(metaTmp) && "El archivo .tmp debe eliminarse tras caída en estado PENDING");
        assert(!std::filesystem::exists(chunkTmp) && "El archivo .tmp debe eliminarse");
        assert(!std::filesystem::exists(storage.getJournalPath()) && "El journal pendiente debe limpiarse");

        // Comprobar que el estado previo sigue siendo 100% válido
        WorldMetadata metaCheck;
        assert(storage.loadMetadata(metaCheck) == StorageError::None);
        assert(metaCheck.worldName == "Trans World Initial");
        assert(metaCheck.seed == 1111);
    }

    // =========================================================================
    // Escenario B: Caída DESPUÉS del commit (status = COMMITTED pero antes de renombres)
    // =========================================================================
    {
        WorldMetadata metaCommitted;
        metaCommitted.worldName = "Trans World Updated";
        metaCommitted.seed = 2222;

        std::vector<uint8_t> validMetaBuf;
        WorldFormat::serializeMetadata(metaCommitted, validMetaBuf);

        std::filesystem::path metaTmp = storage.getMetadataPath().string() + ".tmp";
        std::ofstream fMeta(metaTmp, std::ios::binary);
        fMeta.write(reinterpret_cast<const char*>(validMetaBuf.data()), validMetaBuf.size());
        fMeta.close();

        // Escribir journal con status = 1 (COMMITTED)
        std::vector<uint8_t> journalCommitted;
        journalCommitted.push_back(static_cast<uint8_t>(MAGIC_JOURNAL & 0xFF));
        journalCommitted.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 8) & 0xFF));
        journalCommitted.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 16) & 0xFF));
        journalCommitted.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 24) & 0xFF));
        journalCommitted.push_back(1); // COMMITTED
        journalCommitted.push_back(1); // count = 1
        journalCommitted.push_back(0);

        std::string tmpRel = "world.meta.tmp";
        std::string finalRel = "world.meta";
        uint16_t tLen = static_cast<uint16_t>(tmpRel.size());
        journalCommitted.push_back(static_cast<uint8_t>(tLen & 0xFF));
        journalCommitted.push_back(static_cast<uint8_t>((tLen >> 8) & 0xFF));
        for (char c : tmpRel) journalCommitted.push_back(static_cast<uint8_t>(c));

        uint16_t fLen = static_cast<uint16_t>(finalRel.size());
        journalCommitted.push_back(static_cast<uint8_t>(fLen & 0xFF));
        journalCommitted.push_back(static_cast<uint8_t>((fLen >> 8) & 0xFF));
        for (char c : finalRel) journalCommitted.push_back(static_cast<uint8_t>(c));

        std::ofstream jOut(storage.getJournalPath(), std::ios::binary);
        jOut.write(reinterpret_cast<const char*>(journalCommitted.data()), journalCommitted.size());
        jOut.close();

        // Recuperar en reinicio: debe completar el renombre a finalPath y limpiar journal
        StorageError recErr = storage.recoverJournalIfNeeded();
        assert(recErr == StorageError::None);
        (void)recErr;

        assert(!std::filesystem::exists(metaTmp));
        assert(!std::filesystem::exists(storage.getJournalPath()));

        WorldMetadata metaCheck;
        assert(storage.loadMetadata(metaCheck) == StorageError::None);
        assert(metaCheck.worldName == "Trans World Updated" &&
               "La transacción confirmada debe haberse promovido al estado final");
        assert(metaCheck.seed == 2222);
    }

    // =========================================================================
    // Escenario C: Rollback explícito en transacción
    // =========================================================================
    {
        storage.beginTransaction();
        WorldMetadata metaDiscard;
        metaDiscard.worldName = "Should Discard";
        storage.stageMetadata(metaDiscard);
        storage.rollbackTransaction();

        assert(!std::filesystem::exists(storage.getMetadataPath().string() + ".tmp"));
        assert(!std::filesystem::exists(storage.getJournalPath()));

        WorldMetadata metaCheck;
        assert(storage.loadMetadata(metaCheck) == StorageError::None);
        assert(metaCheck.worldName == "Trans World Updated");
    }

    std::filesystem::remove_all(testDir, ec);
    std::cout << "EXITO: Recuperación transaccional ante caídas (PENDING y COMMITTED) y rollback verificados."
              << std::endl;
    return 0;
}

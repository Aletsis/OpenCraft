#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include "storage/WorldStorage.hpp"
#include "sim/WorldSession.hpp"
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout
        << "--- [F02 Test 3] Detección de corrupción, verificación CRC32 y rechazo de sobreescritura silenciosa ---"
        << std::endl;

    std::filesystem::path testDir = "build/test_corruption_check";
    std::error_code ec;
    std::filesystem::remove_all(testDir, ec);

    WorldStorage storage(testDir);
    storage.init();

    // 1. Guardar chunk válido
    Chunk validChunk(0, 0);
    validChunk.generateTerrain(42);
    validChunk.setBlock(5, 20, 5, BlockID::Wood);

    storage.beginTransaction();
    WorldMetadata meta;
    meta.worldName = "Corruption Test World";
    meta.seed = 42;
    storage.stageMetadata(meta);
    storage.stageChunk(validChunk);
    assert(storage.commitTransaction() == StorageError::None);

    std::filesystem::path chunkFile = storage.getChunkPath(0, 0);
    assert(std::filesystem::exists(chunkFile));

    // 2. Corromper 1 byte dentro del payload del chunk
    {
        std::fstream file(chunkFile, std::ios::in | std::ios::out | std::ios::binary);
        assert(file.is_open());
        file.seekp(24, std::ios::beg); // Posición interna del payload
        char originalByte = 0;
        file.read(&originalByte, 1);
        char corruptedByte = static_cast<char>(originalByte ^ 0xFF);
        file.seekp(24, std::ios::beg);
        file.write(&corruptedByte, 1);
        file.close();

        Chunk readChunk(0, 0);
        StorageError err = storage.loadChunk(0, 0, readChunk);
        assert(err == StorageError::ChecksumMismatch && "Un byte alterado debe detectarse como ChecksumMismatch");
        (void)err;
    }

    // 3. Probar que WorldSession rechaza cargar chunks corruptos y NO los sobreescribe silenciosamente
    {
        WorldSession session;
        session.init(1, 42); // Radio 1 (chunks -1..1)

        StorageError sessionLoadErr = session.loadFromStorage(storage);
        assert(sessionLoadErr == StorageError::ChecksumMismatch &&
               "WorldSession debe devolver error y no continuar silenciosamente");
        (void)sessionLoadErr;

        // El archivo corrupto en disco debe conservarse tal cual, no regenerarse silenciosamente
        assert(std::filesystem::exists(chunkFile));
    }

    // 4. Corromper bytes mágicos en world.meta
    {
        std::filesystem::path metaFile = storage.getMetadataPath();
        std::fstream file(metaFile, std::ios::in | std::ios::out | std::ios::binary);
        char badMagic[4] = {'B', 'A', 'D', '!'};
        file.write(badMagic, 4);
        file.close();

        WorldMetadata readMeta;
        StorageError err = storage.loadMetadata(readMeta);
        assert(err == StorageError::InvalidMagic && "Magic incorrecto debe rechazarse como InvalidMagic");
        (void)err;
    }

    // 5. Archivo truncado (demasiado pequeño para contener cabecera y CRC)
    {
        std::filesystem::path playerFile = storage.getPlayerPath();
        std::ofstream pOut(playerFile, std::ios::binary);
        pOut << "SHORT"; // Solo 5 bytes (< 12 bytes de cabecera)
        pOut.close();

        PlayerSaveData readPlayer;
        StorageError err = storage.loadPlayer(readPlayer);
        assert(err == StorageError::CorruptedData && "Archivo truncado debe rechazarse");
        (void)err;
    }

    // 6. Manejo seguro de bloques desconocidos (unknown namespaces)
    {
        ChunkSaveData customData;
        customData.schemaVersion = SCHEMA_VERSION_2;
        customData.chunkX = 1;
        customData.chunkZ = 1;

        SectionSaveData sec;
        sec.sectionY = 0;
        sec.isEmpty = false;
        sec.palette = {"opencraft:air",
                       "mod_unknown:alien_ore", // Bloque que no existe en BlockRegistry
                       "opencraft:stone"};
        sec.blockIndices.resize(ChunkSection::BLOCKS_PER_SECTION, 0);
        sec.blockIndices[ChunkSection::getIndex(2, 10, 2)] = 1; // alien_ore
        sec.blockIndices[ChunkSection::getIndex(3, 10, 3)] = 2; // stone
        customData.sections.push_back(std::move(sec));

        Chunk testChunk(1, 1);
        // Debe resolver alien_ore a AIR_BLOCK_STATE sin crashear y conservar stone
        WorldFormat::applyChunkData(customData, testChunk);

        assert(testChunk.getBlockState(2, 10, 2) == AIR_BLOCK_STATE &&
               "Bloque desconocido debe mapear a aire de forma segura");
        assert(testChunk.getBlockState(3, 10, 3) == BlockRegistry::global().getStateId("opencraft:stone") &&
               "Bloques conocidos deben conservarse");
    }

    std::filesystem::remove_all(testDir, ec);
    std::cout << "EXITO: Sumas de comprobación CRC32, rechazo de sobreescritura silenciosa y tolerancia a bloques "
                 "desconocidos verificados."
              << std::endl;
    return 0;
}

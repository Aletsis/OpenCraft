#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cassert>
#include <filesystem>
#include <vector>
#include "world/World.hpp"
#include "world/ChunkWorkerPool.hpp"
#include "world/ChunkManager.hpp"
#include "storage/WorldStorage.hpp"
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F03 Test 3] Persistencia en streaming: descarga, reinicio y coherencia de 10 chunks editados ---"
              << std::endl;

    std::filesystem::path testDir = "build/test_saves_streaming_persistence";
    std::error_code ec;
    std::filesystem::remove_all(testDir, ec);

    WorldStorage storage(testDir);
    storage.init();

    auto& reg = BlockRegistry::global();
    BlockStateId stoneState = reg.getStateId("opencraft:stone");
    BlockStateId woodState = reg.getStateId("opencraft:wood");
    BlockStateId glassState = reg.getStateId("opencraft:glass");

    struct ChunkEdit {
        int cx, cz;
        int localX, worldY, localZ;
        BlockStateId blockState;
    };

    // 10 chunks editados en coordenadas dispersas positivas y negativas
    std::vector<ChunkEdit> edits = {{0, 0, 4, 10, 4, woodState},    {1, 0, 8, 20, 8, glassState},
                                    {-1, 0, 2, 15, 2, stoneState},  {0, 1, 6, 30, 6, woodState},
                                    {0, -1, 7, -10, 7, glassState}, {2, 2, 5, 40, 5, stoneState},
                                    {-2, -2, 9, -20, 9, woodState}, {3, -1, 3, 50, 3, glassState},
                                    {-1, 3, 1, 25, 1, stoneState},  {-3, 1, 0, 35, 0, woodState}};

    // FASE 1: Crear mundo, poblar los 10 chunks y editarlos
    {
        World world;
        world.setRenderDistance(3);
        world.setSeed(99999);

        ChunkWorkerPool workerPool(2);
        ChunkManager manager(world, workerPool, &storage);

        // Inicializar área central
        manager.loadAreaSync(0, 0, 3);

        for (const auto& e : edits) {
            int wx = e.cx * Chunk::CHUNK_WIDTH + e.localX;
            int wz = e.cz * Chunk::CHUNK_DEPTH + e.localZ;
            world.setBlockState(wx, e.worldY, wz, e.blockState);
            assert(world.getBlockState(wx, e.worldY, wz) == e.blockState);
        }

        // Alejarse 1000 bloques hacia el este (X = 1000, Z = 0) para forzar descarga y autoguardado de los 10 chunks
        for (int step = 0; step <= 1000; step += 32) {
            manager.update(static_cast<float>(step), 0.0f);
        }

        // Comprobar que los chunks centrales fueron desalojados de RAM
        for (const auto& e : edits) {
            assert(!world.isChunkLoaded(e.cx, e.cz) && "Los chunks deben haberse descargado tras alejarse");
            (void)e;
        }

        manager.saveAllDirtyChunks();
        workerPool.stop();
    }

    // FASE 2: Reiniciar sesión limpia y volver a explorar los 10 chunks
    {
        World world2;
        world2.setRenderDistance(3);
        world2.setSeed(99999);

        ChunkWorkerPool workerPool2(2);
        ChunkManager manager2(world2, workerPool2, &storage);

        // El jugador regresa caminando desde X = 1000 hacia el origen X = 0
        for (int step = 1000; step >= 0; step -= 32) {
            manager2.update(static_cast<float>(step), 0.0f);
        }

        // Cargar síncronamente el radio de los 10 chunks editados desde disco
        for (const auto& e : edits) {
            if (!world2.isChunkLoaded(e.cx, e.cz)) {
                auto c = std::make_unique<Chunk>(e.cx, e.cz);
                StorageError err = storage.loadChunk(e.cx, e.cz, *c);
                if (err != StorageError::None) {
                    std::cout << "DEBUG: loadChunk(" << e.cx << ", " << e.cz << ") failed with error: " << (int)err
                              << ", path exists: " << storage.hasChunk(e.cx, e.cz) << " ("
                              << storage.getChunkPath(e.cx, e.cz) << ")" << std::endl;
                }
                assert(err == StorageError::None && "El chunk editado debe existir en storage");
                (void)err;
                world2.setChunk(e.cx, e.cz, std::move(c));
            }
        }

        // Verificar que los 10 bloques modificados sobrevivieron exactamente a la descarga y recarga
        for (const auto& e : edits) {
            int wx = e.cx * Chunk::CHUNK_WIDTH + e.localX;
            int wz = e.cz * Chunk::CHUNK_DEPTH + e.localZ;
            BlockStateId readState = world2.getBlockState(wx, e.worldY, wz);
            assert(readState == e.blockState && "El bloque editado debe coincidir exactamente tras descarga y recarga");
            (void)readState;
        }

        workerPool2.stop();
    }

    // FASE 3: Fallo de guardado en saveAllDirtyChunks con ruta no escribible y recuperación posterior
    {
        std::cout << "--- [F03 Test 3.1] Retención de chunks sucios ante fallo de E/S en saveAllDirtyChunks ---"
                  << std::endl;
        std::filesystem::path badDir = "build/test_bad_save_path_file";
        std::filesystem::remove(badDir, ec);
        {
            std::ofstream blocker(badDir);
            blocker << "blocker";
        }

        World badWorld;
        badWorld.setRenderDistance(0);
        auto c = std::make_unique<Chunk>(0, 0);
        c->setBlock(0, 100, 0, BlockID::DiamondOre);
        assert(c->isDirty());
        badWorld.setChunk(0, 0, std::move(c));

        ChunkWorkerPool badWorkers(1);
        WorldStorage badStorage(badDir);
        ChunkManager badManager(badWorld, badWorkers, &badStorage);

        StorageError err = StorageError::None;
        size_t saved = badManager.saveAllDirtyChunks(&err);
        assert(saved == 0 && "saveAllDirtyChunks debe retornar 0 si commitTransaction falla");
        assert(err != StorageError::None && "saveAllDirtyChunks debe propagar el error de almacenamiento");
        assert(badManager.getLastError() != StorageError::None);
        assert(badWorld.getChunk(0, 0)->isDirty() && "El chunk DEBE permanecer dirty si el guardado falló");
        assert(!badStorage.hasChunk(0, 0) && "No debe existir archivo de chunk si falló el guardado");

        // Recuperación: desbloquear la ruta convirtiéndola en directorio válido
        std::filesystem::remove(badDir, ec);
        std::filesystem::create_directories(badDir, ec);
        badStorage.init();

        saved = badManager.saveAllDirtyChunks(&err);
        assert(saved == 1 && "Al recuperarse la E/S, debe guardarse 1 chunk");
        assert(err == StorageError::None);
        assert(!badWorld.getChunk(0, 0)->isDirty() && "Tras commit confirmado, isDirty debe limpiarse");
        assert(badStorage.hasChunk(0, 0) && "El chunk debe existir en disco tras guardado exitoso");

        badWorkers.stop();
        std::filesystem::remove_all(badDir, ec);
    }

    // FASE 4: Desalojo con fallos de E/S y posterior recuperación
    {
        std::cout << "--- [F03 Test 3.2] Desalojo seguro con fallo de E/S y recuperación de datos ---" << std::endl;
        std::filesystem::path evictDir = "build/test_eviction_recovery";
        std::filesystem::remove_all(evictDir, ec);

        // Bloquear escritura inicializando con ruta bloqueada por archivo
        std::filesystem::path blockedDir = "build/test_eviction_blocked";
        std::filesystem::remove(blockedDir, ec);
        {
            std::ofstream blocker(blockedDir);
            blocker << "blocker";
        }

        World evictWorld;
        evictWorld.setRenderDistance(1); // unloadDist = 3
        evictWorld.setSeed(12345);

        ChunkWorkerPool evictWorkers(1);
        WorldStorage blockedStorage(blockedDir);
        ChunkManager evictManager(evictWorld, evictWorkers, &blockedStorage);

        // Crear chunk (0, 0) sucio con bloque modificado
        auto evictChunk = std::make_unique<Chunk>(0, 0);
        evictChunk->setBlock(5, 60, 5, BlockID::GoldOre);
        evictChunk->setState(ChunkState::Ready);
        assert(evictChunk->isDirty());
        evictWorld.setChunk(0, 0, std::move(evictChunk));

        // Mover jugador lejos (X = 1000) para forzar intento de desalojo del chunk (0, 0)
        evictManager.update(1000.0f, 0.0f);

        // Debido al fallo de E/S, el chunk (0, 0) NO debe haberse desalojado ni limpiado
        assert(evictWorld.isChunkLoaded(0, 0) &&
               "Chunk sucio DEBE mantenerse residente si falla el guardado de desalojo");
        Chunk* retainedChunk = evictWorld.getChunk(0, 0);
        assert(retainedChunk != nullptr && retainedChunk->isDirty() &&
               "Chunk residente debe conservar isDirty == true");
        assert(evictManager.getStats().unloadedChunks == 0 && "No debe contarse como descargado");
        assert(evictManager.getStats().failedSaves > 0 && "Debe registrarse el intento fallido de guardado");

        // Recuperación de almacenamiento: asociar storage válido
        std::filesystem::create_directories(evictDir, ec);
        WorldStorage validStorage(evictDir);
        validStorage.init();
        evictManager.setStorage(&validStorage);

        // Ejecutar update nuevamente con jugador lejos: ahora el guardado y desalojo deben tener éxito
        evictManager.update(1000.0f, 0.0f);
        assert(!evictWorld.isChunkLoaded(0, 0) && "Tras confirmar guardado, el chunk debe desalojarse");
        assert(validStorage.hasChunk(0, 0) && "El chunk modificado debe existir en disco");

        // El jugador regresa a (0, 0): el chunk debe recargarse intacto con el bloque GoldOre
        evictManager.update(0.0f, 0.0f);
        assert(evictWorld.isChunkLoaded(0, 0) && "El chunk debe recargarse al acercarse el jugador");
        Chunk* reloadedChunk = evictWorld.getChunk(0, 0);
        assert(reloadedChunk != nullptr);
        assert(reloadedChunk->getBlock(5, 60, 5) == BlockID::GoldOre &&
               "El bloque modificado debe haberse preservado íntegramente");

        evictWorkers.stop();
        std::filesystem::remove(blockedDir, ec);
        std::filesystem::remove_all(evictDir, ec);
    }

    // FASE 5: Bloqueo de regeneración de terreno ante chunk corrupto
    {
        std::cout << "--- [F03 Test 3.3] Bloqueo de regeneración silenciosa ante chunk corrupto ---" << std::endl;
        std::filesystem::path corruptDir = "build/test_corrupt_chunk_block";
        std::filesystem::remove_all(corruptDir, ec);

        WorldStorage corruptStorage(corruptDir);
        corruptStorage.init();

        // 1. Guardar chunk válido en (0, 0)
        Chunk validChunk(0, 0);
        validChunk.generateTerrain(12345);
        validChunk.setBlock(2, 70, 2, BlockID::DiamondOre);

        corruptStorage.beginTransaction();
        corruptStorage.stageChunk(validChunk);
        assert(corruptStorage.commitTransaction() == StorageError::None);
        assert(corruptStorage.hasChunk(0, 0));

        std::filesystem::path chunkFilePath = corruptStorage.getChunkPath(0, 0);
        assert(std::filesystem::exists(chunkFilePath));
        auto originalFileSize = std::filesystem::file_size(chunkFilePath);

        // 2. Corromper intencionalmente el archivo en disco (alterar byte de payload)
        {
            std::fstream file(chunkFilePath, std::ios::in | std::ios::out | std::ios::binary);
            assert(file.is_open());
            file.seekp(20, std::ios::beg);
            char b = 0;
            file.read(&b, 1);
            b = static_cast<char>(b ^ 0xAA);
            file.seekp(20, std::ios::beg);
            file.write(&b, 1);
        }

        // 3. Probar que ChunkManager detecta el fallo y NO regenera terreno sobre el chunk dañado
        World streamWorld;
        streamWorld.setRenderDistance(2);
        streamWorld.setSeed(12345);

        ChunkWorkerPool streamWorkers(1);
        ChunkManager streamManager(streamWorld, streamWorkers, &corruptStorage);

        // update en posición (0, 0)
        streamManager.update(0.0f, 0.0f);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        assert(!streamWorld.isChunkLoaded(0, 0) && "Chunk corrupto NO debe cargarse en World");
        assert(streamManager.isChunkFailed(0, 0) && "Chunk (0, 0) debe estar marcado como fallido");
        assert(streamManager.getLastError() != StorageError::None && "Debe registrarse el error de carga");

        // El archivo corrupto no debe haber sido truncado o sobreescrito por un terreno regenerado
        assert(std::filesystem::exists(chunkFilePath));
        assert(std::filesystem::file_size(chunkFilePath) == originalFileSize &&
               "El archivo corrupto debe conservarse intacto sin regeneración");

        // Probar también que loadAreaSync no regenera sobre el chunk corrupto
        streamManager.loadAreaSync(0, 0, 0);
        assert(!streamWorld.isChunkLoaded(0, 0) && "loadAreaSync NO debe regenerar terreno sobre chunk dañado");

        streamWorkers.stop();
        std::filesystem::remove_all(corruptDir, ec);
    }

    std::filesystem::remove_all(testDir, ec);
    std::cout << "EXITO: Persistencia en streaming, retención de datos sucios, desalojo seguro y bloqueo de "
                 "regeneración verificadas."
              << std::endl;
    return 0;
}

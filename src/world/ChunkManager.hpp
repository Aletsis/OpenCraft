#pragma once

#include <vector>
#include <unordered_set>
#include <memory>
#include "world/World.hpp"
#include "world/ChunkWorkerPool.hpp"
#include "storage/WorldStorage.hpp"

namespace OpenCraft {

struct StreamingStats {
    size_t loadedChunks = 0;
    size_t pendingGenerations = 0;
    size_t pendingMeshes = 0;
    size_t savedChunks = 0;
    size_t unloadedChunks = 0;
    size_t failedSaves = 0;
    size_t failedLoads = 0;
};

class ChunkManager {
public:
    ChunkManager(World& world, ChunkWorkerPool& workerPool, WorldStorage* storage = nullptr);
    ~ChunkManager() = default;

    void setStorage(WorldStorage* storage) noexcept { m_storage = storage; }

    // Dimensión cuyo mundo se transmite: determina la carpeta de almacenamiento y el generador.
    // Al cambiarla se descartan las solicitudes pendientes de la dimensión anterior.
    void setDimension(DimensionType dim);
    DimensionType getDimension() const noexcept { return m_dimension; }

    // Actualiza streaming de chunks centrado en la posición mundial del jugador
    void update(float playerWorldX, float playerWorldZ);

    // Guarda todos los chunks sucios antes de salir o forzar guardado
    size_t saveAllDirtyChunks(StorageError* outError = nullptr);

    // Fuerza la carga o generación síncrona en un radio inicial
    void loadAreaSync(int centerChunkX, int centerChunkZ, int radius);

    // Consulta si un chunk está disponible para la simulación
    bool isChunkReady(int chunkX, int chunkZ) const noexcept;

    // Estadísticas operativas del sistema de streaming
    StreamingStats getStats() const noexcept;

    // Métodos de diagnóstico e inspección de errores
    StorageError getLastError() const noexcept { return m_lastError; }
    void clearLastError() noexcept { m_lastError = StorageError::None; }
    bool isChunkFailed(int cx, int cz) const noexcept { return m_failedChunks.contains(ChunkCoord{cx, cz}); }
    void clearFailedChunks() noexcept { m_failedChunks.clear(); }
    const std::unordered_set<ChunkCoord, ChunkCoordHash>& getFailedChunks() const noexcept { return m_failedChunks; }

private:
    World& m_world;
    ChunkWorkerPool& m_workerPool;
    WorldStorage* m_storage = nullptr;
    DimensionType m_dimension = DimensionType::Overworld;

    int m_lastPlayerChunkX = 1000000;
    int m_lastPlayerChunkZ = 1000000;

    std::unordered_set<ChunkCoord, ChunkCoordHash> m_requestedChunks;
    std::unordered_set<ChunkCoord, ChunkCoordHash> m_failedChunks;
    StorageError m_lastError = StorageError::None;
    size_t m_savedCount = 0;
    size_t m_unloadedCount = 0;
    size_t m_failedSaveCount = 0;
    size_t m_failedLoadCount = 0;

    void processPendingWorkers();
    void onChunkPlaced(int cx, int cz);
    std::unique_ptr<Chunk> generateSync(int cx, int cz) const;
};

} // namespace OpenCraft

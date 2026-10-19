#pragma once

#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <unordered_set>
#include "world/Chunk.hpp"
#include "world/ChunkMesh.hpp"
#include "world/ChunkMesher.hpp"

namespace OpenCraft {

class ChunkWorkerPool {
public:
    explicit ChunkWorkerPool(size_t threadCount = 2);
    ~ChunkWorkerPool();

    ChunkWorkerPool(const ChunkWorkerPool&) = delete;
    ChunkWorkerPool& operator=(const ChunkWorkerPool&) = delete;

    // Enviar solicitud de generación procedural de terreno
    void requestGeneration(int chunkX, int chunkZ, uint64_t seed, uint32_t version);

    // Enviar solicitud de cálculo de malla en CPU sobre una copia capturada en el hilo principal
    void requestMesh(ChunkMeshInput input);

    // Cancelar tareas pendientes para un chunk específico (p. ej. jugador se alejó)
    void cancelTasks(int chunkX, int chunkZ);

    // Vaciar y obtener resultados listos en el hilo principal
    void pollGenerations(std::vector<std::unique_ptr<Chunk>>& outGenerated);
    void pollMeshes(std::vector<ChunkMeshBuildResult>& outMeshes);

    size_t getPendingTaskCount() const noexcept;
    void stop();

private:
    enum class TaskType {
        Generation,
        Meshing
    };

    struct WorkerTask {
        TaskType type;
        int chunkX = 0;
        int chunkZ = 0;
        uint64_t seed = 0;
        uint32_t version = 0;
        std::unique_ptr<ChunkMeshInput> meshInput;
    };

    std::vector<std::thread> m_workers;
    std::deque<WorkerTask> m_taskQueue;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_queueCv;
    std::atomic<bool> m_running{true};

    // Colas de resultados completados
    std::mutex m_resultsMutex;
    std::vector<std::unique_ptr<Chunk>> m_completedGenerations;
    std::vector<ChunkMeshBuildResult> m_completedMeshes;

    void workerLoop();
};

} // namespace OpenCraft

#include "world/ChunkWorkerPool.hpp"
#include "world/BlockRegistry.hpp"
#include "world/WorldGenerator.hpp"
#include "world/ChunkLighting.hpp"
#include <algorithm>

namespace OpenCraft {

ChunkWorkerPool::ChunkWorkerPool(size_t threadCount) {
    if (threadCount == 0) {
        threadCount = 1;
    }
    m_workers.reserve(threadCount);
    for (size_t i = 0; i < threadCount; ++i) {
        m_workers.emplace_back(&ChunkWorkerPool::workerLoop, this);
    }
}

ChunkWorkerPool::~ChunkWorkerPool() { stop(); }

void ChunkWorkerPool::stop() {
    m_running = false;
    m_queueCv.notify_all();
    for (auto& w : m_workers) {
        if (w.joinable()) {
            w.join();
        }
    }
    m_workers.clear();
}

void ChunkWorkerPool::requestGeneration(int chunkX, int chunkZ, uint64_t seed, uint32_t version) {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_taskQueue.push_back(WorkerTask{.type = TaskType::Generation,
                                         .chunkX = chunkX,
                                         .chunkZ = chunkZ,
                                         .seed = seed,
                                         .version = version,
                                         .meshInput = nullptr});
    }
    m_queueCv.notify_one();
}

void ChunkWorkerPool::requestMesh(ChunkMeshInput input) {
    int cx = input.chunkX;
    int cz = input.chunkZ;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        // Una malla pendiente del mismo chunk queda obsoleta: se sustituye por la nueva captura
        for (auto& t : m_taskQueue) {
            if (t.type == TaskType::Meshing && t.chunkX == cx && t.chunkZ == cz) {
                t.meshInput = std::make_unique<ChunkMeshInput>(std::move(input));
                return;
            }
        }
        m_taskQueue.push_back(WorkerTask{.type = TaskType::Meshing,
                                         .chunkX = cx,
                                         .chunkZ = cz,
                                         .seed = 0,
                                         .version = 0,
                                         .meshInput = std::make_unique<ChunkMeshInput>(std::move(input))});
    }
    m_queueCv.notify_one();
}

void ChunkWorkerPool::cancelTasks(int chunkX, int chunkZ) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    std::erase_if(m_taskQueue,
                  [chunkX, chunkZ](const WorkerTask& t) { return t.chunkX == chunkX && t.chunkZ == chunkZ; });
}

void ChunkWorkerPool::pollGenerations(std::vector<std::unique_ptr<Chunk>>& outGenerated) {
    std::lock_guard<std::mutex> lock(m_resultsMutex);
    if (!m_completedGenerations.empty()) {
        outGenerated.insert(outGenerated.end(), std::make_move_iterator(m_completedGenerations.begin()),
                            std::make_move_iterator(m_completedGenerations.end()));
        m_completedGenerations.clear();
    }
}

void ChunkWorkerPool::pollMeshes(std::vector<ChunkMeshBuildResult>& outMeshes) {
    std::lock_guard<std::mutex> lock(m_resultsMutex);
    if (!m_completedMeshes.empty()) {
        outMeshes.insert(outMeshes.end(), std::make_move_iterator(m_completedMeshes.begin()),
                         std::make_move_iterator(m_completedMeshes.end()));
        m_completedMeshes.clear();
    }
}

size_t ChunkWorkerPool::getPendingTaskCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_taskQueue.size();
}

void ChunkWorkerPool::workerLoop() {
    while (m_running) {
        WorkerTask task;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCv.wait(lock, [this]() { return !m_running || !m_taskQueue.empty(); });

            if (!m_running && m_taskQueue.empty()) {
                break;
            }

            task = std::move(m_taskQueue.front());
            m_taskQueue.pop_front();
        }

        if (task.type == TaskType::Generation) {
            auto chunk = std::make_unique<Chunk>(task.chunkX, task.chunkZ);
            chunk->generateTerrain(task.seed, task.version);
            ChunkLighting::computeChunkLight(*chunk);
            chunk->setState(ChunkState::Ready);

            std::lock_guard<std::mutex> lock(m_resultsMutex);
            m_completedGenerations.push_back(std::move(chunk));
        } else if (task.type == TaskType::Meshing && task.meshInput) {
            ChunkMeshBuildResult result;
            ChunkMesher::build(*task.meshInput, result);

            std::lock_guard<std::mutex> lock(m_resultsMutex);
            m_completedMeshes.push_back(std::move(result));
        }
    }
}

} // namespace OpenCraft

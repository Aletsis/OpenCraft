#include "world/ChunkManager.hpp"
#include "core/Log.hpp"
#include <algorithm>
#include <cmath>

namespace OpenCraft {

static inline int floorDiv16(int a) noexcept {
    int res = a / 16;
    int rem = a % 16;
    if (rem != 0 && (a < 0)) {
        res--;
    }
    return res;
}

ChunkManager::ChunkManager(World& world, ChunkWorkerPool& workerPool, WorldStorage* storage)
    : m_world(world),
      m_workerPool(workerPool),
      m_storage(storage) {
    m_world.setAutoCreateChunks(false);
}

void ChunkManager::setDimension(DimensionType dim) {
    if (dim == m_dimension) return;
    for (const auto& coord : m_requestedChunks) {
        m_workerPool.cancelTasks(coord.x, coord.z);
    }
    m_requestedChunks.clear();
    m_failedChunks.clear();
    m_dimension = dim;
}

std::unique_ptr<Chunk> ChunkManager::generateSync(int cx, int cz) const {
    auto chunk = std::make_unique<Chunk>(cx, cz);
    Dimension::generateTerrain(m_dimension, *chunk, m_world.getSeed(), m_world.getGeneratorVersion());
    chunk->setState(ChunkState::Ready);
    // Un chunk recién generado es reproducible desde la semilla: solo se guarda si se modifica
    chunk->clearDirty();
    return chunk;
}

bool ChunkManager::isChunkReady(int chunkX, int chunkZ) const noexcept {
    const Chunk* c = m_world.getChunk(chunkX, chunkZ);
    return c != nullptr && c->getState() != ChunkState::Unloaded && c->getState() != ChunkState::Requested;
}

StreamingStats ChunkManager::getStats() const noexcept {
    StreamingStats stats;
    stats.loadedChunks = m_world.getLoadedChunkCount();
    stats.pendingGenerations = m_requestedChunks.size();
    stats.pendingMeshes = m_workerPool.getPendingTaskCount();
    stats.savedChunks = m_savedCount;
    stats.unloadedChunks = m_unloadedCount;
    stats.failedSaves = m_failedSaveCount;
    stats.failedLoads = m_failedLoadCount;
    return stats;
}

void ChunkManager::onChunkPlaced(int cx, int cz) {
    // Los vecinos ya mallados ocultaban sus caras de borde hacia este chunk: deben remallarse
    static constexpr int OFFSETS[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const auto& o : OFFSETS) {
        if (Chunk* n = m_world.getChunk(cx + o[0], cz + o[1])) {
            n->incrementMeshVersion();
        }
    }
}

void ChunkManager::processPendingWorkers() {
    std::vector<std::unique_ptr<Chunk>> generated;
    m_workerPool.pollGenerations(generated);

    for (auto& chunk : generated) {
        int cx = chunk->getChunkX();
        int cz = chunk->getChunkZ();
        // Los trabajadores solo generan terreno del Overworld; se descartan resultados que ya no se
        // esperan (p. ej. solicitados antes de un cambio de dimensión)
        if (m_requestedChunks.erase(ChunkCoord{cx, cz}) == 0 || m_dimension != DimensionType::Overworld) {
            continue;
        }

        Chunk* existing = m_world.getChunk(cx, cz);
        if (!existing) {
            chunk->setState(ChunkState::Ready);
            chunk->clearDirty(); // reproducible desde la semilla mientras no se modifique
            m_world.setChunk(cx, cz, std::move(chunk));
            onChunkPlaced(cx, cz);
        }
    }
}

void ChunkManager::update(float playerWorldX, float playerWorldZ) {
    processPendingWorkers();

    int playerChunkX = floorDiv16(static_cast<int>(playerWorldX));
    int playerChunkZ = floorDiv16(static_cast<int>(playerWorldZ));

    m_lastPlayerChunkX = playerChunkX;
    m_lastPlayerChunkZ = playerChunkZ;

    int renderDist = m_world.getRenderDistance();
    int unloadDist = renderDist + 2; // Margen de histeresis para evitar thrashing

    // 1. Identificar chunks en el radio de visión
    struct Candidate {
        int cx, cz;
        int distSq;
    };
    std::vector<Candidate> candidates;

    for (int dx = -renderDist; dx <= renderDist; ++dx) {
        for (int dz = -renderDist; dz <= renderDist; ++dz) {
            int distSq = dx * dx + dz * dz;
            if (distSq <= renderDist * renderDist + renderDist) {
                int cx = playerChunkX + dx;
                int cz = playerChunkZ + dz;
                candidates.push_back({cx, cz, distSq});
            }
        }
    }

    // Ordenar de más cercanos al jugador a más lejanos
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) { return a.distSq < b.distSq; });

    // 2. Solicitar carga o generación de chunks faltantes
    for (const auto& cand : candidates) {
        ChunkCoord coord{cand.cx, cand.cz};
        if (!m_world.isChunkLoaded(cand.cx, cand.cz) && !m_requestedChunks.contains(coord) &&
            !m_failedChunks.contains(coord)) {
            m_requestedChunks.insert(coord);

            if (m_storage && m_storage->hasChunk(cand.cx, cand.cz, m_dimension)) {
                auto chunk = std::make_unique<Chunk>(cand.cx, cand.cz);
                StorageError err =
                    m_storage->loadChunk(cand.cx, cand.cz, *chunk, BlockRegistry::global(), nullptr, m_dimension);
                if (err == StorageError::None) {
                    chunk->setState(ChunkState::Ready);
                    m_world.setChunk(cand.cx, cand.cz, std::move(chunk));
                    m_requestedChunks.erase(coord);
                    onChunkPlaced(cand.cx, cand.cz);
                    continue;
                } else {
                    // Carga fallida: el chunk existe en almacenamiento pero falló la lectura/deserialización
                    // (corrupción, fallo de checksum, error de E/S, etc.)
                    // BLOQUEAR regeneración para proteger los datos persistentes del usuario y no sobreescribir.
                    m_lastError = err;
                    ++m_failedLoadCount;
                    m_failedChunks.insert(coord);
                    m_requestedChunks.erase(coord);
                    Log::error("Fallo al cargar chunk ({}, {}): {}. Regeneración bloqueada para preservar datos.",
                               cand.cx, cand.cz, storageErrorToString(err));
                    continue;
                }
            }

            // Generación SOLO si el chunk no existía en almacenamiento: asíncrona en el Overworld,
            // síncrona con el generador de la dimensión en el Abismo y el Confín
            if (m_dimension == DimensionType::Overworld) {
                m_workerPool.requestGeneration(cand.cx, cand.cz, m_world.getSeed(), m_world.getGeneratorVersion());
            } else {
                m_world.setChunk(cand.cx, cand.cz, generateSync(cand.cx, cand.cz));
                m_requestedChunks.erase(coord);
                onChunkPlaced(cand.cx, cand.cz);
            }
        }
    }

    // 3. Desalojo seguro de chunks lejanos (Caché acotada)
    std::vector<ChunkCoord> chunksToUnload;
    for (const auto& [coord, chunkPtr] : m_world.getChunks()) {
        int dx = coord.x - playerChunkX;
        int dz = coord.z - playerChunkZ;
        if (dx * dx + dz * dz > unloadDist * unloadDist) {
            chunksToUnload.push_back(coord);
        }
    }

    // Los chunks modificados se guardan en una única transacción antes de descargarlos: si falla,
    // ninguno se descarga y siguen residentes y marcados como sucios (A01)
    std::vector<Chunk*> dirtyToSave;
    for (const auto& coord : chunksToUnload) {
        Chunk* chunk = m_world.getChunk(coord.x, coord.z);
        if (chunk && chunk->isDirty()) dirtyToSave.push_back(chunk);
    }
    bool saveOk = true;
    if (!dirtyToSave.empty() && m_storage) {
        StorageError err = m_storage->beginTransaction();
        for (size_t i = 0; err == StorageError::None && i < dirtyToSave.size(); ++i) {
            err = m_storage->stageChunk(*dirtyToSave[i], {}, m_dimension);
        }
        if (err == StorageError::None) {
            err = m_storage->commitTransaction();
        }
        if (err != StorageError::None) {
            saveOk = false;
            m_lastError = err;
            m_failedSaveCount += dirtyToSave.size();
            Log::error("Fallo al guardar {} chunks antes de descargarlos: {}", dirtyToSave.size(),
                       storageErrorToString(err));
            if (m_storage->isInTransaction()) m_storage->rollbackTransaction();
        } else {
            for (Chunk* c : dirtyToSave) c->clearDirty();
            m_savedCount += dirtyToSave.size();
        }
    }

    for (const auto& coord : chunksToUnload) {
        Chunk* chunk = m_world.getChunk(coord.x, coord.z);
        if (!chunk) continue;
        if (chunk->isDirty() && (m_storage || !saveOk)) {
            continue; // NO desalojar: mantener datos residentes y sucios hasta poder guardarlos
        }
        m_workerPool.cancelTasks(coord.x, coord.z);
        m_world.removeChunk(coord.x, coord.z);
        ++m_unloadedCount;
    }
}

size_t ChunkManager::saveAllDirtyChunks(StorageError* outError) {
    if (outError) *outError = StorageError::None;
    if (!m_storage) return 0;

    std::vector<Chunk*> dirtyChunks;
    for (const auto& [coord, chunkPtr] : m_world.getChunks()) {
        (void)coord;
        if (chunkPtr && chunkPtr->isDirty()) {
            dirtyChunks.push_back(chunkPtr.get());
        }
    }

    if (dirtyChunks.empty()) return 0;

    StorageError err = m_storage->beginTransaction();
    if (err != StorageError::None) {
        m_lastError = err;
        ++m_failedSaveCount;
        if (outError) *outError = err;
        Log::error("Fallo beginTransaction en saveAllDirtyChunks: {}", storageErrorToString(err));
        return 0;
    }

    std::vector<Chunk*> stagedChunks;
    for (Chunk* c : dirtyChunks) {
        err = m_storage->stageChunk(*c, {}, m_dimension);
        if (err != StorageError::None) {
            m_lastError = err;
            ++m_failedSaveCount;
            if (outError) *outError = err;
            Log::error("Fallo stageChunk en saveAllDirtyChunks para ({}, {}): {}", c->getChunkX(), c->getChunkZ(),
                       storageErrorToString(err));
            m_storage->rollbackTransaction();
            return 0;
        }
        stagedChunks.push_back(c);
    }

    err = m_storage->commitTransaction();
    if (err != StorageError::None) {
        m_lastError = err;
        ++m_failedSaveCount;
        if (outError) *outError = err;
        Log::error("Fallo commitTransaction en saveAllDirtyChunks: {}", storageErrorToString(err));
        if (m_storage->isInTransaction()) {
            m_storage->rollbackTransaction();
        }
        return 0;
    }

    // Limpiar banderas sucias exclusivamente tras confirmación exitosa de la transacción
    for (Chunk* c : stagedChunks) {
        c->clearDirty();
    }
    size_t count = stagedChunks.size();
    m_savedCount += count;
    return count;
}

void ChunkManager::loadAreaSync(int centerChunkX, int centerChunkZ, int radius) {
    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dz = -radius; dz <= radius; ++dz) {
            int cx = centerChunkX + dx;
            int cz = centerChunkZ + dz;
            if (!m_world.isChunkLoaded(cx, cz)) {
                if (m_storage && m_storage->hasChunk(cx, cz, m_dimension)) {
                    auto chunk = std::make_unique<Chunk>(cx, cz);
                    StorageError err =
                        m_storage->loadChunk(cx, cz, *chunk, BlockRegistry::global(), nullptr, m_dimension);
                    if (err != StorageError::None) {
                        m_lastError = err;
                        ++m_failedLoadCount;
                        m_failedChunks.insert(ChunkCoord{cx, cz});
                        Log::error("Fallo al cargar chunk ({}, {}) síncronamente: {}. Regeneración bloqueada.", cx, cz,
                                   storageErrorToString(err));
                        continue; // No cargar chunk corrupto ni generar terreno encima
                    }
                    chunk->setState(ChunkState::Ready);
                    m_world.setChunk(cx, cz, std::move(chunk));
                } else {
                    m_world.setChunk(cx, cz, generateSync(cx, cz));
                }
                onChunkPlaced(cx, cz);
            }
        }
    }
}

} // namespace OpenCraft

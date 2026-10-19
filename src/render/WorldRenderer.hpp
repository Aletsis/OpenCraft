#pragma once

#include <unordered_map>
#include <vector>
#include <array>
#include "world/World.hpp"
#include "world/ChunkMesh.hpp"
#include "world/ChunkMesher.hpp"
#include "world/ChunkWorkerPool.hpp"
#include "render/Shader.hpp"
#include "render/Frustum.hpp"

namespace OpenCraft {

struct RenderStats {
    size_t renderedSections = 0;
    size_t culledSections = 0;
    size_t totalMeshes = 0;
    size_t pendingChunks = 0;
};

class WorldRenderer {
public:
    WorldRenderer() = default;
    ~WorldRenderer() = default;

    WorldRenderer(const WorldRenderer&) = delete;
    WorldRenderer& operator=(const WorldRenderer&) = delete;

    // Programa el mallado de los chunks nuevos o modificados. Los cambios junto al jugador se
    // mallan en el acto (respuesta inmediata al colocar/romper); el resto, en los trabajadores.
    void update(const World& world, ChunkWorkerPool& pool, int playerChunkX, int playerChunkZ);
    void applyCompletedMeshes(const std::vector<ChunkMeshBuildResult>& completedMeshes, const World& world);

    // Renderizado por pases con descarte por frustum y orden de translúcidos de atrás hacia delante
    void render(const Shader& shader, const Frustum* frustum, const Math::Vec3& cameraPos) const;

    void clear();
    void removeChunkMesh(int chunkX, int chunkZ);

    bool isChunkMeshed(int chunkX, int chunkZ) const;
    RenderStats getStats() const noexcept { return m_stats; }

private:
    struct ChunkMeshEntry {
        uint32_t uploadedVersion = 0;  // 0 = nunca subida
        uint32_t requestedVersion = 0; // versión enviada a los trabajadores
        std::array<std::array<ChunkMesh, 3>, Chunk::SECTION_COUNT> sections{};
    };

    std::unordered_map<ChunkCoord, ChunkMeshEntry, ChunkCoordHash> m_chunkMeshes;
    mutable RenderStats m_stats{};

    void upload(ChunkMeshEntry& entry, const ChunkMeshBuildResult& result);
};

} // namespace OpenCraft

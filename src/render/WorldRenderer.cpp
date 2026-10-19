#include "render/WorldRenderer.hpp"
#include <algorithm>
#include <cstdlib>

namespace OpenCraft {

void WorldRenderer::clear() { m_chunkMeshes.clear(); }

void WorldRenderer::removeChunkMesh(int chunkX, int chunkZ) { m_chunkMeshes.erase(ChunkCoord{chunkX, chunkZ}); }

bool WorldRenderer::isChunkMeshed(int chunkX, int chunkZ) const {
    auto it = m_chunkMeshes.find(ChunkCoord{chunkX, chunkZ});
    return it != m_chunkMeshes.end() && it->second.uploadedVersion != 0;
}

void WorldRenderer::upload(ChunkMeshEntry& entry, const ChunkMeshBuildResult& result) {
    for (const auto& sec : result.sections) {
        int idx = Chunk::sectionYToIndex(sec.sectionY);
        if (idx < 0 || idx >= Chunk::SECTION_COUNT) continue;
        for (size_t p = 0; p < 3; ++p) {
            entry.sections[static_cast<size_t>(idx)][p].upload(sec.vertices[p], sec.indices[p]);
        }
    }
    entry.uploadedVersion = result.meshVersion;
}

void WorldRenderer::update(const World& world, ChunkWorkerPool& pool, int playerChunkX, int playerChunkZ) {
    // Liberar mallas de chunks descargados
    for (auto it = m_chunkMeshes.begin(); it != m_chunkMeshes.end();) {
        if (!world.getChunk(it->first.x, it->first.z)) {
            it = m_chunkMeshes.erase(it);
        } else {
            ++it;
        }
    }

    int syncBudget = 2;
    size_t pending = 0;
    for (const auto& [coord, chunkPtr] : world.getChunks()) {
        if (!chunkPtr) continue;
        uint32_t version = chunkPtr->getMeshVersion();
        auto& entry = m_chunkMeshes[coord];
        if (entry.uploadedVersion == version) continue;
        ++pending;
        if (entry.requestedVersion == version) continue;

        bool nearPlayer = std::abs(coord.x - playerChunkX) <= 1 && std::abs(coord.z - playerChunkZ) <= 1;
        if (nearPlayer && entry.uploadedVersion != 0 && syncBudget > 0) {
            ChunkMeshBuildResult result;
            ChunkMesher::build(ChunkMesher::capture(world, *chunkPtr), result);
            upload(entry, result);
            entry.requestedVersion = version;
            --syncBudget;
            --pending;
        } else {
            pool.requestMesh(ChunkMesher::capture(world, *chunkPtr));
            entry.requestedVersion = version;
        }
    }
    m_stats.pendingChunks = pending;
}

void WorldRenderer::applyCompletedMeshes(const std::vector<ChunkMeshBuildResult>& completedMeshes, const World& world) {
    for (const auto& res : completedMeshes) {
        if (!world.getChunk(res.chunkX, res.chunkZ)) continue;
        auto it = m_chunkMeshes.find(ChunkCoord{res.chunkX, res.chunkZ});
        if (it == m_chunkMeshes.end()) continue;
        // Una malla anterior a la ya subida se descarta; una intermedia se muestra mientras llega
        // la versión más reciente (evita huecos en el terreno durante ráfagas de cambios)
        if (it->second.uploadedVersion != 0 && res.meshVersion <= it->second.uploadedVersion) continue;
        upload(it->second, res);
    }
}

void WorldRenderer::render(const Shader& shader, const Frustum* frustum, const Math::Vec3& cameraPos) const {
    m_stats.renderedSections = 0;
    m_stats.culledSections = 0;
    m_stats.totalMeshes = 0;

    struct VisibleSection {
        const ChunkMeshEntry* entry;
        int secIdx;
        float minX, minY, minZ;
        float distSq;
    };
    std::vector<VisibleSection> visible;
    visible.reserve(m_chunkMeshes.size() * 4);

    for (const auto& [coord, entry] : m_chunkMeshes) {
        for (int secIdx = 0; secIdx < Chunk::SECTION_COUNT; ++secIdx) {
            const auto& passes = entry.sections[static_cast<size_t>(secIdx)];
            if (passes[0].isEmpty() && passes[1].isEmpty() && passes[2].isEmpty()) continue;
            ++m_stats.totalMeshes;

            float minX = static_cast<float>(coord.x * Chunk::CHUNK_WIDTH);
            float minY = static_cast<float>(Chunk::indexToSectionY(secIdx) * Chunk::SECTION_SIZE);
            float minZ = static_cast<float>(coord.z * Chunk::CHUNK_DEPTH);
            Math::AABB box{.min = {minX, minY, minZ}, .max = {minX + 16.0f, minY + 16.0f, minZ + 16.0f}};
            if (frustum && !frustum->intersectsAABB(box)) {
                ++m_stats.culledSections;
                continue;
            }
            float dx = minX + 8.0f - cameraPos.x, dy = minY + 8.0f - cameraPos.y, dz = minZ + 8.0f - cameraPos.z;
            visible.push_back({&entry, secIdx, minX, minY, minZ, dx * dx + dy * dy + dz * dz});
            ++m_stats.renderedSections;
        }
    }

    // Opacos de delante hacia atrás (menos sobredibujado), translúcidos de atrás hacia delante
    std::sort(visible.begin(), visible.end(),
              [](const VisibleSection& a, const VisibleSection& b) { return a.distSq < b.distSq; });

    auto drawPass = [&](size_t pass, bool reverse) {
        auto drawOne = [&](const VisibleSection& v) {
            const auto& mesh = v.entry->sections[static_cast<size_t>(v.secIdx)][pass];
            if (mesh.isEmpty()) return;
            shader.setMat4("uModel", Math::Mat4::translate(Math::Vec3(v.minX, v.minY, v.minZ)));
            mesh.render();
        };
        if (reverse) {
            for (auto it = visible.rbegin(); it != visible.rend(); ++it) drawOne(*it);
        } else {
            for (const auto& v : visible) drawOne(v);
        }
    };

    drawPass(static_cast<size_t>(RenderPass::Opaque), false);

    // Plantas y cruces se ven por ambas caras: se emiten con cara trasera explícita
    drawPass(static_cast<size_t>(RenderPass::Cutout), false);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE); // la superficie del agua debe verse también desde abajo
    drawPass(static_cast<size_t>(RenderPass::Translucent), true);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

} // namespace OpenCraft

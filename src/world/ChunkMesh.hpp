#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include "math/Math.hpp"

namespace OpenCraft {

enum class RenderPass : uint8_t {
    Opaque = 0,
    Cutout = 1,
    Translucent = 2,
    Count = 3
};

struct VoxelVertex {
    float x, y, z;
    float u, v;
    float nx, ny, nz;
    float ao;         // Ambient occlusion (0.2 to 1.0)
    float skyLight;   // 0.0 to 15.0
    float blockLight; // 0.0 to 15.0
};

class ChunkMesh {
public:
    ChunkMesh();
    ~ChunkMesh();

    ChunkMesh(const ChunkMesh&) = delete;
    ChunkMesh& operator=(const ChunkMesh&) = delete;
    ChunkMesh(ChunkMesh&& other) noexcept;
    ChunkMesh& operator=(ChunkMesh&& other) noexcept;

    void upload(const std::vector<VoxelVertex>& vertices, const std::vector<uint32_t>& indices);
    void render() const;

    size_t getIndexCount() const noexcept { return m_indexCount; }
    bool isEmpty() const noexcept { return m_indexCount == 0; }

private:
    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ebo = 0;
    size_t m_indexCount = 0;

    void cleanup();
};

} // namespace OpenCraft

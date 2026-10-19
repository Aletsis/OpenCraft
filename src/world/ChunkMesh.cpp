#include "world/ChunkMesh.hpp"
#include <GLES3/gl3.h>

namespace OpenCraft {

ChunkMesh::ChunkMesh() = default;

ChunkMesh::~ChunkMesh() { cleanup(); }

ChunkMesh::ChunkMesh(ChunkMesh&& other) noexcept
    : m_vao(other.m_vao),
      m_vbo(other.m_vbo),
      m_ebo(other.m_ebo),
      m_indexCount(other.m_indexCount) {
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
    other.m_indexCount = 0;
}

ChunkMesh& ChunkMesh::operator=(ChunkMesh&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_indexCount = other.m_indexCount;

        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
        other.m_indexCount = 0;
    }
    return *this;
}

void ChunkMesh::cleanup() {
    if (m_ebo) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    m_indexCount = 0;
}

void ChunkMesh::upload(const std::vector<VoxelVertex>& vertices, const std::vector<uint32_t>& indices) {
    cleanup();

    if (vertices.empty() || indices.empty()) {
        return;
    }

    m_indexCount = indices.size();

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VoxelVertex), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Layout attributes:
    // 0: Position (x, y, z) - 3 floats
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), reinterpret_cast<void*>(0));

    // 1: UV (u, v) - 2 floats
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), reinterpret_cast<void*>(3 * sizeof(float)));

    // 2: Normal (nx, ny, nz) - 3 floats
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), reinterpret_cast<void*>(5 * sizeof(float)));

    // 3: Ambient Occlusion - 1 float
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), reinterpret_cast<void*>(8 * sizeof(float)));

    // 4: Sky Light - 1 float
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), reinterpret_cast<void*>(9 * sizeof(float)));

    // 5: Block Light - 1 float
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(VoxelVertex), reinterpret_cast<void*>(10 * sizeof(float)));

    glBindVertexArray(0);
}

void ChunkMesh::render() const {
    if (m_vao && m_indexCount > 0) {
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
}

} // namespace OpenCraft

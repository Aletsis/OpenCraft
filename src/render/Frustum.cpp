#include "render/Frustum.hpp"

namespace OpenCraft {

void Frustum::updateFromViewProj(const Math::Mat4& vp) noexcept {
    const float* m = vp.data();

    // Helper para acceder a componentes (row r, col c) en column-major
    auto get = [&](int r, int c) -> float { return m[c * 4 + r]; };

    // Row 0, 1, 2, 3
    // Left:   Row 3 + Row 0
    m_planes[Left].normal.x = get(3, 0) + get(0, 0);
    m_planes[Left].normal.y = get(3, 1) + get(0, 1);
    m_planes[Left].normal.z = get(3, 2) + get(0, 2);
    m_planes[Left].distance = get(3, 3) + get(0, 3);

    // Right:  Row 3 - Row 0
    m_planes[Right].normal.x = get(3, 0) - get(0, 0);
    m_planes[Right].normal.y = get(3, 1) - get(0, 1);
    m_planes[Right].normal.z = get(3, 2) - get(0, 2);
    m_planes[Right].distance = get(3, 3) - get(0, 3);

    // Bottom: Row 3 + Row 1
    m_planes[Bottom].normal.x = get(3, 0) + get(1, 0);
    m_planes[Bottom].normal.y = get(3, 1) + get(1, 1);
    m_planes[Bottom].normal.z = get(3, 2) + get(1, 2);
    m_planes[Bottom].distance = get(3, 3) + get(1, 3);

    // Top:    Row 3 - Row 1
    m_planes[Top].normal.x = get(3, 0) - get(1, 0);
    m_planes[Top].normal.y = get(3, 1) - get(1, 1);
    m_planes[Top].normal.z = get(3, 2) - get(1, 2);
    m_planes[Top].distance = get(3, 3) - get(1, 3);

    // Near:   Row 3 + Row 2
    m_planes[Near].normal.x = get(3, 0) + get(2, 0);
    m_planes[Near].normal.y = get(3, 1) + get(2, 1);
    m_planes[Near].normal.z = get(3, 2) + get(2, 2);
    m_planes[Near].distance = get(3, 3) + get(2, 3);

    // Far:    Row 3 - Row 2
    m_planes[Far].normal.x = get(3, 0) - get(2, 0);
    m_planes[Far].normal.y = get(3, 1) - get(2, 1);
    m_planes[Far].normal.z = get(3, 2) - get(2, 2);
    m_planes[Far].distance = get(3, 3) - get(2, 3);

    for (int i = 0; i < PlaneCount; ++i) {
        m_planes[i].normalize();
    }
}

bool Frustum::containsPoint(const Math::Vec3& point) const noexcept {
    for (int i = 0; i < PlaneCount; ++i) {
        if (m_planes[i].distanceToPoint(point) < 0.0f) {
            return false;
        }
    }
    return true;
}

bool Frustum::intersectsAABB(const Math::AABB& box) const noexcept {
    for (int i = 0; i < PlaneCount; ++i) {
        const auto& p = m_planes[i];

        // Vértice p-vertex (el más alejado en dirección de la normal)
        Math::Vec3 pVertex{(p.normal.x >= 0.0f) ? box.max.x : box.min.x, (p.normal.y >= 0.0f) ? box.max.y : box.min.y,
                           (p.normal.z >= 0.0f) ? box.max.z : box.min.z};

        if (p.distanceToPoint(pVertex) < 0.0f) {
            // Toda la caja está fuera de este plano
            return false;
        }
    }
    return true;
}

} // namespace OpenCraft

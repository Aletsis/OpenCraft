#pragma once

#include "math/Math.hpp"
#include <array>

namespace OpenCraft {

struct Plane {
    Math::Vec3 normal{0.0f, 1.0f, 0.0f};
    float distance = 0.0f;

    void normalize() noexcept {
        float len = normal.length();
        if (len > 0.000001f) {
            normal = normal * (1.0f / len);
            distance /= len;
        }
    }

    float distanceToPoint(const Math::Vec3& pt) const noexcept { return normal.dot(pt) + distance; }
};

class Frustum {
public:
    enum PlaneIndex {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Far,
        PlaneCount
    };

    Frustum() = default;

    // Actualiza los 6 planos a partir de la matriz View-Projection
    void updateFromViewProj(const Math::Mat4& vp) noexcept;

    // Comprueba si un punto está dentro del frustum
    bool containsPoint(const Math::Vec3& point) const noexcept;

    // Comprueba si un AABB intersecta o está dentro del frustum
    bool intersectsAABB(const Math::AABB& box) const noexcept;

    const std::array<Plane, PlaneCount>& getPlanes() const noexcept { return m_planes; }

private:
    std::array<Plane, PlaneCount> m_planes{};
};

} // namespace OpenCraft

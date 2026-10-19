#pragma once

#include "math/Math.hpp"

namespace OpenCraft {

class Camera {
public:
    Math::Vec3 position{0.0f, 2.0f, 5.0f};
    float yaw{-90.0f};
    float pitch{0.0f};
    float fov{70.0f};
    float nearPlane{0.1f};
    float farPlane{500.0f};

    Camera() { updateVectors(); }

    Math::Mat4 getViewMatrix() const { return Math::Mat4::lookAt(position, position + m_front, m_up); }

    Math::Mat4 getProjectionMatrix(float aspectRatio) const {
        return Math::Mat4::perspective(Math::radians(fov), aspectRatio, nearPlane, farPlane);
    }

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        yaw += xoffset;
        pitch += yoffset;

        if (constrainPitch) {
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
        }

        updateVectors();
    }

    const Math::Vec3& getFront() const noexcept { return m_front; }
    const Math::Vec3& getRight() const noexcept { return m_right; }
    const Math::Vec3& getUp() const noexcept { return m_up; }

private:
    Math::Vec3 m_front{0.0f, 0.0f, -1.0f};
    Math::Vec3 m_right{1.0f, 0.0f, 0.0f};
    Math::Vec3 m_up{0.0f, 1.0f, 0.0f};
    const Math::Vec3 m_worldUp{0.0f, 1.0f, 0.0f};

    void updateVectors() {
        float yawRad = Math::radians(yaw);
        float pitchRad = Math::radians(pitch);

        Math::Vec3 front;
        front.x = std::cos(yawRad) * std::cos(pitchRad);
        front.y = std::sin(pitchRad);
        front.z = std::sin(yawRad) * std::cos(pitchRad);
        m_front = front.normalized();

        m_right = m_front.cross(m_worldUp).normalized();
        m_up = m_right.cross(m_front).normalized();
    }
};

} // namespace OpenCraft

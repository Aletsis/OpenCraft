#pragma once

#include <cmath>
#include <array>
#include <cstdint>

namespace OpenCraft::Math {

constexpr float PI = 3.14159265358979323846f;

inline constexpr float radians(float degrees) noexcept { return degrees * (PI / 180.0f); }

inline constexpr float degrees(float rad) noexcept { return rad * (180.0f / PI); }

struct Vec2 {
    float x{0.0f}, y{0.0f};

    constexpr Vec2() = default;
    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}

    constexpr Vec2 operator+(const Vec2& o) const noexcept { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(const Vec2& o) const noexcept { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(float s) const noexcept { return {x * s, y * s}; }
    constexpr Vec2 operator/(float s) const noexcept { return {x / s, y / s}; }
    constexpr float dot(const Vec2& o) const noexcept { return x * o.x + y * o.y; }
    float length() const noexcept { return std::sqrt(dot(*this)); }
};

struct Vec3 {
    float x{0.0f}, y{0.0f}, z{0.0f};

    constexpr Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    constexpr Vec3 operator+(const Vec3& o) const noexcept { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(const Vec3& o) const noexcept { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(float s) const noexcept { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator/(float s) const noexcept { return {x / s, y / s, z / s}; }

    Vec3& operator+=(const Vec3& o) noexcept {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
    Vec3& operator-=(const Vec3& o) noexcept {
        x -= o.x;
        y -= o.y;
        z -= o.z;
        return *this;
    }
    Vec3& operator*=(float s) noexcept {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    constexpr float dot(const Vec3& o) const noexcept { return x * o.x + y * o.y + z * o.z; }

    constexpr Vec3 cross(const Vec3& o) const noexcept {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }

    float length() const noexcept { return std::sqrt(dot(*this)); }

    Vec3 normalized() const noexcept {
        float len = length();
        if (len > 0.00001f) {
            return *this / len;
        }
        return {0.0f, 0.0f, 0.0f};
    }
};

struct Vec4 {
    float x{0.0f}, y{0.0f}, z{0.0f}, w{0.0f};

    constexpr Vec4() = default;
    constexpr Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    constexpr Vec4(const Vec3& v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}
};

// 4x4 Matrix, column-major order: elements are [col * 4 + row]
struct Mat4 {
    std::array<float, 16> m{0.0f};

    constexpr Mat4() = default;

    static constexpr Mat4 identity() noexcept {
        Mat4 res;
        res.m[0] = 1.0f;
        res.m[5] = 1.0f;
        res.m[10] = 1.0f;
        res.m[15] = 1.0f;
        return res;
    }

    const float* data() const noexcept { return m.data(); }

    Mat4 operator*(const Mat4& rhs) const noexcept {
        Mat4 out;
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += m[k * 4 + r] * rhs.m[c * 4 + k];
                }
                out.m[c * 4 + r] = sum;
            }
        }
        return out;
    }

    static Mat4 translate(const Vec3& v) noexcept {
        Mat4 res = identity();
        res.m[12] = v.x;
        res.m[13] = v.y;
        res.m[14] = v.z;
        return res;
    }

    static Mat4 scale(const Vec3& v) noexcept {
        Mat4 res = identity();
        res.m[0] = v.x;
        res.m[5] = v.y;
        res.m[10] = v.z;
        return res;
    }

    static Mat4 rotate(float angleRad, const Vec3& axis) noexcept {
        Mat4 res = identity();
        Vec3 a = axis.normalized();
        float c = std::cos(angleRad);
        float s = std::sin(angleRad);
        float oc = 1.0f - c;

        res.m[0] = oc * a.x * a.x + c;
        res.m[1] = oc * a.x * a.y + a.z * s;
        res.m[2] = oc * a.z * a.x - a.y * s;

        res.m[4] = oc * a.x * a.y - a.z * s;
        res.m[5] = oc * a.y * a.y + c;
        res.m[6] = oc * a.y * a.z + a.x * s;

        res.m[8] = oc * a.z * a.x + a.y * s;
        res.m[9] = oc * a.y * a.z - a.x * s;
        res.m[10] = oc * a.z * a.z + c;

        return res;
    }

    static Mat4 perspective(float fovRad, float aspect, float zNear, float zFar) noexcept {
        Mat4 res;
        float tanHalfFov = std::tan(fovRad / 2.0f);

        res.m[0] = 1.0f / (aspect * tanHalfFov);
        res.m[5] = 1.0f / tanHalfFov;
        res.m[10] = -(zFar + zNear) / (zFar - zNear);
        res.m[11] = -1.0f;
        res.m[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
        res.m[15] = 0.0f;

        return res;
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) noexcept {
        Vec3 f = (center - eye).normalized();
        Vec3 s = f.cross(up).normalized();
        Vec3 u = s.cross(f);

        Mat4 res = identity();
        res.m[0] = s.x;
        res.m[4] = s.y;
        res.m[8] = s.z;

        res.m[1] = u.x;
        res.m[5] = u.y;
        res.m[9] = u.z;

        res.m[2] = -f.x;
        res.m[6] = -f.y;
        res.m[10] = -f.z;

        res.m[12] = -s.dot(eye);
        res.m[13] = -u.dot(eye);
        res.m[14] = f.dot(eye);

        return res;
    }
};

struct AABB {
    Vec3 min{0.0f, 0.0f, 0.0f};
    Vec3 max{0.0f, 0.0f, 0.0f};

    constexpr bool intersects(const AABB& o) const noexcept {
        return (min.x <= o.max.x && max.x >= o.min.x) && (min.y <= o.max.y && max.y >= o.min.y) &&
               (min.z <= o.max.z && max.z >= o.min.z);
    }
};

} // namespace OpenCraft::Math

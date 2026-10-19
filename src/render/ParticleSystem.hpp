#pragma once

#include <vector>
#include <cstdint>
#include "math/Math.hpp"

namespace OpenCraft {

class World;

enum class ParticleType {
    BlockBreak,
    Smoke,
    Flame,
    CriticalHit,
    Heart,
    Splash,
    CinnabarDust
};

struct Particle {
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    Math::Vec3 velocity{0.0f, 0.0f, 0.0f};
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    float size = 0.1f;
    float age = 0.0f;
    float maxAge = 1.0f;
    float gravity = 9.8f;
    ParticleType type = ParticleType::BlockBreak;
    bool alive = false;
};

struct ToolSwingAnimation {
    bool isSwinging = false;
    float progress = 0.0f;  // 0.0f a 1.0f
    float duration = 0.25f; // 250 ms por golpe

    void trigger() {
        isSwinging = true;
        progress = 0.0f;
    }

    void update(float dt) {
        if (!isSwinging) return;
        progress += dt / duration;
        if (progress >= 1.0f) {
            progress = 0.0f;
            isSwinging = false;
        }
    }

    float getAngleDegrees() const {
        if (!isSwinging) return 0.0f;
        // Curva senoidal de ida y vuelta
        return std::sin(progress * Math::PI) * 45.0f;
    }
};

class ParticleSystem {
public:
    explicit ParticleSystem(size_t maxParticles = 1024);

    void setMaxParticles(size_t maxParticles);
    size_t getMaxParticles() const { return m_maxParticles; }
    size_t getActiveParticleCount() const { return m_activeCount; }

    void spawnParticle(const Particle& p);
    void spawnBlockBreakParticles(const Math::Vec3& blockCenter, float r, float g, float b, int count = 16);
    void spawnSmoke(const Math::Vec3& pos, int count = 4);
    void spawnCrit(const Math::Vec3& pos, int count = 8);
    void spawnSplash(const Math::Vec3& pos, int count = 12);

    void update(float dt, const World* world = nullptr);
    void clear();

    const std::vector<Particle>& getParticles() const { return m_particles; }

    // Animación de herramienta en primera persona
    ToolSwingAnimation& getToolSwing() { return m_toolSwing; }
    const ToolSwingAnimation& getToolSwing() const { return m_toolSwing; }

private:
    size_t m_maxParticles = 1024;
    size_t m_activeCount = 0;
    size_t m_nextIndex = 0;
    std::vector<Particle> m_particles;
    ToolSwingAnimation m_toolSwing;
};

} // namespace OpenCraft

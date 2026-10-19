#include "render/ParticleSystem.hpp"
#include "world/World.hpp"
#include <random>
#include <cmath>
#include <algorithm>

namespace OpenCraft {

ParticleSystem::ParticleSystem(size_t maxParticles) : m_maxParticles(maxParticles), m_particles(maxParticles) {}

void ParticleSystem::setMaxParticles(size_t maxParticles) {
    m_maxParticles = std::max(size_t(64), maxParticles);
    m_particles.resize(m_maxParticles);
    if (m_nextIndex >= m_maxParticles) m_nextIndex = 0;
    if (m_activeCount > m_maxParticles) m_activeCount = m_maxParticles;
}

void ParticleSystem::clear() {
    for (auto& p : m_particles) {
        p.alive = false;
    }
    m_activeCount = 0;
    m_nextIndex = 0;
}

void ParticleSystem::spawnParticle(const Particle& p) {
    if (m_particles.empty()) return;

    m_particles[m_nextIndex] = p;
    m_particles[m_nextIndex].alive = true;
    m_particles[m_nextIndex].age = 0.0f;

    m_nextIndex = (m_nextIndex + 1) % m_maxParticles;
    if (m_activeCount < m_maxParticles) {
        m_activeCount++;
    }
}

void ParticleSystem::spawnBlockBreakParticles(const Math::Vec3& blockCenter, float r, float g, float b, int count) {
    std::mt19937 rng(static_cast<uint32_t>(blockCenter.x * 1000 + blockCenter.z * 10));
    std::uniform_real_distribution<float> offsetDist(-0.4f, 0.4f);
    std::uniform_real_distribution<float> velHorizDist(-2.0f, 2.0f);
    std::uniform_real_distribution<float> velVertDist(1.5f, 4.0f);
    std::uniform_real_distribution<float> sizeDist(0.04f, 0.10f);
    std::uniform_real_distribution<float> ageDist(0.4f, 0.8f);

    for (int i = 0; i < count; ++i) {
        Particle p;
        p.type = ParticleType::BlockBreak;
        p.position = blockCenter + Math::Vec3{offsetDist(rng), offsetDist(rng), offsetDist(rng)};
        p.velocity = Math::Vec3{velHorizDist(rng), velVertDist(rng), velHorizDist(rng)};
        p.r = r;
        p.g = g;
        p.b = b;
        p.a = 1.0f;
        p.size = sizeDist(rng);
        p.gravity = 14.0f;
        p.maxAge = ageDist(rng);
        spawnParticle(p);
    }
}

void ParticleSystem::spawnSmoke(const Math::Vec3& pos, int count) {
    std::mt19937 rng(456);
    std::uniform_real_distribution<float> offsetDist(-0.1f, 0.1f);
    std::uniform_real_distribution<float> velDist(-0.2f, 0.2f);
    std::uniform_real_distribution<float> upDist(0.5f, 1.2f);

    for (int i = 0; i < count; ++i) {
        Particle p;
        p.type = ParticleType::Smoke;
        p.position = pos + Math::Vec3{offsetDist(rng), 0.0f, offsetDist(rng)};
        p.velocity = Math::Vec3{velDist(rng), upDist(rng), velDist(rng)};
        p.r = 0.3f;
        p.g = 0.3f;
        p.b = 0.3f;
        p.a = 0.8f;
        p.size = 0.12f;
        p.gravity = -0.5f; // Flotabilidad hacia arriba
        p.maxAge = 1.2f;
        spawnParticle(p);
    }
}

void ParticleSystem::spawnCrit(const Math::Vec3& pos, int count) {
    std::mt19937 rng(789);
    std::uniform_real_distribution<float> velDist(-1.5f, 1.5f);
    std::uniform_real_distribution<float> upDist(0.5f, 2.5f);

    for (int i = 0; i < count; ++i) {
        Particle p;
        p.type = ParticleType::CriticalHit;
        p.position = pos;
        p.velocity = Math::Vec3{velDist(rng), upDist(rng), velDist(rng)};
        p.r = 0.85f;
        p.g = 0.75f;
        p.b = 0.15f;
        p.a = 1.0f;
        p.size = 0.08f;
        p.gravity = 6.0f;
        p.maxAge = 0.6f;
        spawnParticle(p);
    }
}

void ParticleSystem::spawnSplash(const Math::Vec3& pos, int count) {
    std::mt19937 rng(321);
    std::uniform_real_distribution<float> velDist(-1.2f, 1.2f);
    std::uniform_real_distribution<float> upDist(1.0f, 3.0f);

    for (int i = 0; i < count; ++i) {
        Particle p;
        p.type = ParticleType::Splash;
        p.position = pos;
        p.velocity = Math::Vec3{velDist(rng), upDist(rng), velDist(rng)};
        p.r = 0.2f;
        p.g = 0.5f;
        p.b = 0.9f;
        p.a = 0.8f;
        p.size = 0.06f;
        p.gravity = 12.0f;
        p.maxAge = 0.5f;
        spawnParticle(p);
    }
}

void ParticleSystem::update(float dt, const World* world) {
    m_toolSwing.update(dt);

    size_t count = 0;
    for (auto& p : m_particles) {
        if (!p.alive) continue;

        p.age += dt;
        if (p.age >= p.maxAge) {
            p.alive = false;
            continue;
        }

        // Desvanecimiento suave según edad
        p.a = 1.0f - (p.age / p.maxAge);

        // Integración física Euler semi-implícita
        p.velocity.y -= p.gravity * dt;
        p.position += p.velocity * dt;

        // Colisión simple con suelo del mundo si se suministra
        if (world) {
            int bx = static_cast<int>(std::floor(p.position.x));
            int by = static_cast<int>(std::floor(p.position.y));
            int bz = static_cast<int>(std::floor(p.position.z));

            uint16_t state = world->getBlockState(bx, by, bz);
            if (state != AIR_BLOCK_STATE) {
                // Detener en el suelo y rebotar ligeramente o morir
                p.position.y = static_cast<float>(by + 1);
                p.velocity.y = -p.velocity.y * 0.3f;
                p.velocity.x *= 0.6f;
                p.velocity.z *= 0.6f;
                if (std::abs(p.velocity.y) < 0.2f) {
                    p.velocity = {0.0f, 0.0f, 0.0f};
                }
            }
        }

        count++;
    }
    m_activeCount = count;
}

} // namespace OpenCraft

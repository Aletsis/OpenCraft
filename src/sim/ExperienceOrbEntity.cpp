#include "sim/ExperienceOrbEntity.hpp"
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include <cmath>

namespace OpenCraft {

void ExperienceOrbEntity::tick(World& world, const Math::Vec3& playerPos, float dt) {
    if (isDead) return;

    ageTicks++;
    if (ageTicks >= MAX_AGE_TICKS) {
        isDead = true;
        return;
    }

    Math::Vec3 target = playerPos + Math::Vec3(0.0f, 0.9f, 0.0f);
    Math::Vec3 diff = target - position;
    float distSq = diff.dot(diff);

    // Atracción gravitatoria hacia el jugador si está dentro de 6 metros
    if (distSq <= 36.0f && distSq > 0.01f) {
        float dist = std::sqrt(distSq);
        Math::Vec3 dir = diff * (1.0f / dist);
        float pullStrength = (1.0f - dist / 6.0f) * 12.0f;
        velocity = velocity + dir * (pullStrength * dt);
        // Límite de velocidad
        float speed = velocity.length();
        if (speed > 10.0f) {
            velocity = velocity * (10.0f / speed);
        }
    } else {
        velocity.y -= 14.0f * dt;
        velocity.x *= 0.95f;
        velocity.z *= 0.95f;
        velocity.y *= 0.98f;
    }

    // Comprobar colisión simple con suelo
    Math::Vec3 nextPos = position + velocity * dt;
    int bx = static_cast<int>(std::floor(nextPos.x));
    int by = static_cast<int>(std::floor(nextPos.y));
    int bz = static_cast<int>(std::floor(nextPos.z));

    int cx = (bx >= 0) ? (bx / 16) : ((bx - 15) / 16);
    int cz = (bz >= 0) ? (bz / 16) : ((bz - 15) / 16);

    if (world.isChunkLoaded(cx, cz)) {
        BlockStateId st = world.getBlockState(bx, by, bz);
        if (BlockRegistry::global().get(st).isSolid) {
            if (velocity.y < 0.0f) {
                velocity.y = 0.0f;
                nextPos.y = static_cast<float>(by + 1);
            }
        }
    }

    position = nextPos;
}

} // namespace OpenCraft

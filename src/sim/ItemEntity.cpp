#include "sim/ItemEntity.hpp"
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include <cmath>

namespace OpenCraft {

void ItemEntity::tick(World& world, float dt) {
    if (isDead) return;

    ++ageTicks;
    if (ageTicks >= MAX_LIFETIME_TICKS) {
        isDead = true;
        return;
    }

    if (pickupDelayTicks > 0) {
        --pickupDelayTicks;
    }

    resolvePhysics(world, dt);
}

void ItemEntity::resolvePhysics(World& world, float dt) {
    if (isDead) return;

    // Aplicar gravedad
    velocity.y -= 26.0f * dt;

    // Amortiguación horizontal en el aire / suelo
    float drag = isGrounded ? 0.70f : 0.98f;
    velocity.x *= drag;
    velocity.z *= drag;

    float halfW = width * 0.5f;
    const auto& reg = BlockRegistry::global();

    auto isSolidAt = [&](int x, int y, int z) -> bool {
        int cx = (x >= 0) ? (x / 16) : ((x - 15) / 16);
        int cz = (z >= 0) ? (z / 16) : ((z - 15) / 16);
        if (!world.isChunkLoaded(cx, cz)) {
            return true; // Chunks no cargados bloquean
        }
        return reg.get(world.getBlockState(x, y, z)).isSolid;
    };

    // 1. Colisión Eje Y
    position.y += velocity.y * dt;
    Math::AABB boxY{.min = {position.x - halfW, position.y, position.z - halfW},
                    .max = {position.x + halfW, position.y + height, position.z + halfW}};

    isGrounded = false;
    int minX = static_cast<int>(std::floor(boxY.min.x));
    int maxX = static_cast<int>(std::floor(boxY.max.x));
    int minY = static_cast<int>(std::floor(boxY.min.y));
    int maxY = static_cast<int>(std::floor(boxY.max.y));
    int minZ = static_cast<int>(std::floor(boxY.min.z));
    int maxZ = static_cast<int>(std::floor(boxY.max.z));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            for (int z = minZ; z <= maxZ; ++z) {
                if (isSolidAt(x, y, z)) {
                    if (velocity.y < 0.0f) {
                        position.y = static_cast<float>(y + 1);
                        velocity.y = 0.0f;
                        isGrounded = true;
                    } else if (velocity.y > 0.0f) {
                        position.y = static_cast<float>(y) - height;
                        velocity.y = 0.0f;
                    }
                    goto resolvedY;
                }
            }
        }
    }
resolvedY:

    // 2. Colisión Eje X
    position.x += velocity.x * dt;
    // 3. Colisión Eje Z
    position.z += velocity.z * dt;
}

} // namespace OpenCraft

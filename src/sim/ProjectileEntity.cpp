#include "sim/ProjectileEntity.hpp"
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include <cmath>

namespace OpenCraft {

void ProjectileEntity::tick(World& world, float dt) {
    if (isDead) return;

    lifeTicks++;
    if (lifeTicks >= MAX_LIFE_TICKS) {
        isDead = true;
        return;
    }

    if (inGround) {
        return;
    }

    prevPosition = position;

    // Gravedad y resistencia aerodinámica
    velocity.y -= 16.0f * dt;
    velocity.x *= 0.99f;
    velocity.z *= 0.99f;
    velocity.y *= 0.99f;

    // Orientación balística
    float hSpeed = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
    if (hSpeed > 0.001f) {
        yaw = Math::degrees(std::atan2(velocity.x, velocity.z));
        pitch = Math::degrees(std::atan2(velocity.y, hSpeed));
    }

    // Comprobación de colisión continua contra bloques del mundo
    Math::Vec3 nextPos = position + velocity * dt;
    int steps = std::max(1, static_cast<int>(std::ceil((nextPos - position).length() / 0.2f)));
    Math::Vec3 stepDir = (nextPos - position) * (1.0f / static_cast<float>(steps));

    const auto& reg = BlockRegistry::global();
    Math::Vec3 curPos = position;

    for (int i = 0; i < steps; ++i) {
        curPos = curPos + stepDir;
        int bx = static_cast<int>(std::floor(curPos.x));
        int by = static_cast<int>(std::floor(curPos.y));
        int bz = static_cast<int>(std::floor(curPos.z));

        int cx = (bx >= 0) ? (bx / 16) : ((bx - 15) / 16);
        int cz = (bz >= 0) ? (bz / 16) : ((bz - 15) / 16);

        if (world.isChunkLoaded(cx, cz)) {
            BlockStateId state = world.getBlockState(bx, by, bz);
            if (reg.get(state).isSolid) {
                inGround = true;
                velocity = Math::Vec3(0.0f, 0.0f, 0.0f);
                position = curPos - stepDir * 0.5f;
                return;
            }
        }
    }

    position = nextPos;
}

} // namespace OpenCraft

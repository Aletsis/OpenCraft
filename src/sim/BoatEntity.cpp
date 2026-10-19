#include "sim/BoatEntity.hpp"
#include "world/World.hpp"
#include <cmath>

namespace OpenCraft {

BoatEntity::BoatEntity() {
    id = INVALID_ENTITY_ID;
    width = 1.4f;
    height = 0.6f;
}

BoatEntity::BoatEntity(EntityId eId, const Math::Vec3& pos) {
    id = eId;
    position = pos;
    prevPosition = pos;
    width = 1.4f;
    height = 0.6f;
}

void BoatEntity::tickPhysics(World& world, float dt) {
    if (!isAlive) return;

    prevPosition = position;

    int bx = static_cast<int>(std::floor(position.x));
    int by = static_cast<int>(std::floor(position.y));
    int bz = static_cast<int>(std::floor(position.z));

    BlockID blockCurrent = world.getBlock(bx, by, bz);
    BlockID blockUnder = world.getBlock(bx, by - 1, bz);

    bool inWater = (blockCurrent == BlockID::Water || blockCurrent == BlockID::FlowingWater ||
                    blockUnder == BlockID::Water || blockUnder == BlockID::FlowingWater);

    if (inWater) {
        // Flotabilidad hidrostática: estabilizar cerca de la superficie del agua
        float targetY = static_cast<float>(by) + 0.2f;
        if (blockCurrent == BlockID::Water || blockCurrent == BlockID::FlowingWater) {
            targetY = static_cast<float>(by) + 0.8f;
        }

        float diffY = targetY - position.y;
        velocity.y = diffY * 4.0f; // Fuerza restauradora suave

        // Resistencia hidrodinámica mínima (deslizamiento suave)
        velocity.x *= 0.96f;
        velocity.z *= 0.96f;
    } else {
        // Gravedad normal fuera del agua
        velocity.y -= 18.0f * dt;
        velocity.x *= 0.8f;
        velocity.z *= 0.8f;
    }

    position += velocity * dt;

    if (position.y < static_cast<float>(Chunk::MIN_Y)) {
        position.y = static_cast<float>(Chunk::MIN_Y);
        velocity.y = 0.0f;
    }
}

} // namespace OpenCraft

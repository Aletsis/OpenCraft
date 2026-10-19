#pragma once

#include "sim/Entity.hpp"
#include <cstdint>

namespace OpenCraft {

class World;

struct BoatEntity : public Entity {
    EntityId passengerId = INVALID_ENTITY_ID;
    bool isUnderwater = false;
    float boatSpeed = 6.0f;

    BoatEntity();
    BoatEntity(EntityId eId, const Math::Vec3& pos);

    bool hasPassenger() const noexcept { return passengerId != INVALID_ENTITY_ID; }

    void setPassenger(EntityId pId) noexcept { passengerId = pId; }

    void removePassenger() noexcept { passengerId = INVALID_ENTITY_ID; }

    void tickPhysics(World& world, float dt = 0.05f);
};

} // namespace OpenCraft

#pragma once

#include <cstdint>
#include "sim/Entity.hpp"

namespace OpenCraft {

class World;

struct ExperienceOrbEntity {
    EntityId id = INVALID_ENTITY_ID;
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    Math::Vec3 velocity{0.0f, 0.0f, 0.0f};

    uint32_t value = 1;
    uint32_t ageTicks = 0;
    static constexpr uint32_t MAX_AGE_TICKS = 6000; // 5 minutos

    bool isDead = false;
    float width = 0.25f;
    float height = 0.25f;

    ExperienceOrbEntity() = default;
    ExperienceOrbEntity(EntityId orbId, const Math::Vec3& pos, uint32_t val = 1,
                        const Math::Vec3& vel = {0.0f, 1.0f, 0.0f})
        : id(orbId),
          position(pos),
          velocity(vel),
          value(val) {}

    Math::AABB getAABB() const noexcept {
        float halfW = width * 0.5f;
        return Math::AABB{.min = {position.x - halfW, position.y, position.z - halfW},
                          .max = {position.x + halfW, position.y + height, position.z + halfW}};
    }

    void tick(World& world, const Math::Vec3& playerPos, float dt = 0.05f);
};

} // namespace OpenCraft

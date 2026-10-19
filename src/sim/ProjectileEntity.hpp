#pragma once

#include <cstdint>
#include "sim/Entity.hpp"

namespace OpenCraft {

class World;

enum class ProjectileType : uint8_t {
    Arrow = 0,
    WindCharge = 1,
    PoisonArrow = 2
};

struct ProjectileEntity {
    EntityId id = INVALID_ENTITY_ID;
    EntityId shooterId = INVALID_ENTITY_ID;
    ProjectileType type = ProjectileType::Arrow;

    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    Math::Vec3 prevPosition{0.0f, 0.0f, 0.0f};
    Math::Vec3 velocity{0.0f, 0.0f, 0.0f};

    float yaw = 0.0f;
    float pitch = 0.0f;

    float damage = 4.0f;
    float width = 0.25f;
    float height = 0.25f;

    bool inGround = false;
    bool isDead = false;
    uint32_t lifeTicks = 0;
    static constexpr uint32_t MAX_LIFE_TICKS = 1200; // 60 segundos

    ProjectileEntity() = default;
    ProjectileEntity(EntityId pId, EntityId shooter, const Math::Vec3& pos, const Math::Vec3& vel, float dmg = 4.0f)
        : id(pId),
          shooterId(shooter),
          position(pos),
          prevPosition(pos),
          velocity(vel),
          damage(dmg) {}

    Math::AABB getAABB() const noexcept {
        float halfW = width * 0.5f;
        return Math::AABB{.min = {position.x - halfW, position.y - height * 0.5f, position.z - halfW},
                          .max = {position.x + halfW, position.y + height * 0.5f, position.z + halfW}};
    }

    void tick(World& world, float dt = 0.05f);
};

} // namespace OpenCraft

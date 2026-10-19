#pragma once

#include "sim/Entity.hpp"
#include "item/ItemStack.hpp"

namespace OpenCraft {

class World;

struct ItemEntity {
    EntityId id = INVALID_ENTITY_ID;
    ItemStack stack;

    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    Math::Vec3 velocity{0.0f, 0.0f, 0.0f};

    uint32_t pickupDelayTicks = 20; // 1 segundo inicial para no re-recoger de inmediato
    uint32_t ageTicks = 0;
    static constexpr uint32_t MAX_LIFETIME_TICKS = 6000; // 5 minutos

    bool isGrounded = false;
    bool isDead = false;

    float width = 0.25f;
    float height = 0.25f;

    ItemEntity() = default;
    ItemEntity(EntityId entityId, ItemStack itemStack, const Math::Vec3& pos,
               const Math::Vec3& vel = {0.0f, 0.0f, 0.0f}, uint32_t delay = 20)
        : id(entityId),
          stack(std::move(itemStack)),
          position(pos),
          velocity(vel),
          pickupDelayTicks(delay) {}

    Math::AABB getAABB() const noexcept {
        float halfW = width * 0.5f;
        return Math::AABB{.min = {position.x - halfW, position.y, position.z - halfW},
                          .max = {position.x + halfW, position.y + height, position.z + halfW}};
    }

    const ItemStack& getStack() const noexcept { return stack; }
    ItemStack& getStack() noexcept { return stack; }

    bool canBePickedUp() const noexcept { return !isDead && pickupDelayTicks == 0 && !stack.isEmpty(); }

    void tick(World& world, float dt = 0.05f);

private:
    void resolvePhysics(World& world, float dt);
};

} // namespace OpenCraft

#pragma once

#include "sim/Entity.hpp"
#include "item/Inventory.hpp"
#include "world/RailSystem.hpp"
#include <vector>

namespace OpenCraft {

class World;
class CinnabarEngine;
struct ItemEntity;

enum class MinecartType : uint8_t {
    Standard = 0,
    Chest = 1,
    Hopper = 2
};

struct MinecartEntity : public Entity {
    MinecartType minecartType = MinecartType::Standard;
    EntityId passengerId = INVALID_ENTITY_ID;
    Inventory inventory{0};
    float maxSpeed = 8.0f; // 8 bloques/s (canónico)
    bool isOnRail = false;

    MinecartEntity();
    MinecartEntity(EntityId eId, const Math::Vec3& pos, MinecartType type = MinecartType::Standard);

    bool hasPassenger() const noexcept { return passengerId != INVALID_ENTITY_ID; }

    void setPassenger(EntityId pId) noexcept { passengerId = pId; }

    void removePassenger() noexcept { passengerId = INVALID_ENTITY_ID; }

    void tickPhysics(World& world, CinnabarEngine* cinnabar = nullptr, std::vector<ItemEntity>* worldItems = nullptr,
                     float dt = 0.05f);
};

} // namespace OpenCraft

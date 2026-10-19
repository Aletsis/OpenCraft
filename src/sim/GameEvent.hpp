#pragma once

#include <cstdint>
#include <variant>
#include <vector>
#include <functional>
#include "math/Math.hpp"
#include "sim/Entity.hpp"

namespace OpenCraft {

struct BlockChangedEvent {
    int x = 0;
    int y = 0;
    int z = 0;
    uint16_t oldState = 0;
    uint16_t newState = 0;
    EntityId causeEntityId = INVALID_ENTITY_ID;
};

struct EntityMovedEvent {
    EntityId entityId = INVALID_ENTITY_ID;
    Math::Vec3 oldPos{0.0f, 0.0f, 0.0f};
    Math::Vec3 newPos{0.0f, 0.0f, 0.0f};
    bool isGrounded = false;
};

struct InventoryChangedEvent {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    size_t slot = 0;
    uint16_t itemId = 0;
    uint16_t count = 0;
};

struct ItemSpawnedEvent {
    EntityId entityId = INVALID_ENTITY_ID;
    uint16_t itemId = 0;
    uint16_t count = 0;
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
};

struct ItemPickedUpEvent {
    EntityId playerEntityId = LOCAL_PLAYER_ENTITY_ID;
    EntityId itemEntityId = INVALID_ENTITY_ID;
    uint16_t itemId = 0;
    uint16_t count = 0;
};

struct ContainerOpenedEvent {
    EntityId playerEntityId = LOCAL_PLAYER_ENTITY_ID;
    int x = 0;
    int y = 0;
    int z = 0;
};

struct ContainerClosedEvent {
    EntityId playerEntityId = LOCAL_PLAYER_ENTITY_ID;
};

enum class DamageCause : uint8_t {
    Generic = 0,
    Fall = 1,
    Void = 2,
    Suffocation = 3,
    Drowning = 4,
    Starvation = 5,
    Lava = 6,
    Fire = 7,
    MobAttack = 8,
    Explosion = 9,
    Projectile = 10,
    Magic = 11,
    Decay = 12
};

struct PlayerHurtEvent {
    EntityId playerEntityId = LOCAL_PLAYER_ENTITY_ID;
    float damage = 0.0f;
    DamageCause cause = DamageCause::Generic;
    float newHealth = 0.0f;
};

struct PlayerDiedEvent {
    EntityId playerEntityId = LOCAL_PLAYER_ENTITY_ID;
    Math::Vec3 deathPosition{0.0f, 0.0f, 0.0f};
    DamageCause cause = DamageCause::Generic;
};

struct PlayerRespawnedEvent {
    EntityId playerEntityId = LOCAL_PLAYER_ENTITY_ID;
    Math::Vec3 spawnPosition{0.0f, 0.0f, 0.0f};
};

struct PlayerAteEvent {
    EntityId playerEntityId = LOCAL_PLAYER_ENTITY_ID;
    uint16_t itemId = 0;
    uint16_t foodGained = 0;
    float saturationGained = 0.0f;
};

struct BedInteractedEvent {
    EntityId playerEntityId = LOCAL_PLAYER_ENTITY_ID;
    int x = 0;
    int y = 0;
    int z = 0;
    bool timeSkipped = false;
    bool spawnSet = false;
};

struct GameModeChangedEvent {
    EntityId playerEntityId = LOCAL_PLAYER_ENTITY_ID;
    uint32_t newGameMode = 0;
};

struct MobSpawnedEvent {
    EntityId entityId = INVALID_ENTITY_ID;
    std::string mobType;
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
};

struct MobHurtEvent {
    EntityId entityId = INVALID_ENTITY_ID;
    EntityId attackerId = INVALID_ENTITY_ID;
    float damage = 0.0f;
    float newHealth = 0.0f;
};

struct MobDiedEvent {
    EntityId entityId = INVALID_ENTITY_ID;
    EntityId killerId = INVALID_ENTITY_ID;
    Math::Vec3 deathPosition{0.0f, 0.0f, 0.0f};
};

struct ExplosionEvent {
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    float radius = 3.0f;
    uint32_t blocksDestroyed = 0;
};

struct ProjectileSpawnedEvent {
    EntityId entityId = INVALID_ENTITY_ID;
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    Math::Vec3 velocity{0.0f, 0.0f, 0.0f};
};

struct ExperienceGainedEvent {
    EntityId playerEntityId = LOCAL_PLAYER_ENTITY_ID;
    uint32_t amount = 0;
    uint32_t newLevel = 0;
};

using GameEvent =
    std::variant<BlockChangedEvent, EntityMovedEvent, InventoryChangedEvent, ItemSpawnedEvent, ItemPickedUpEvent,
                 ContainerOpenedEvent, ContainerClosedEvent, PlayerHurtEvent, PlayerDiedEvent, PlayerRespawnedEvent,
                 PlayerAteEvent, BedInteractedEvent, GameModeChangedEvent, MobSpawnedEvent, MobHurtEvent, MobDiedEvent,
                 ExplosionEvent, ProjectileSpawnedEvent, ExperienceGainedEvent>;

class IGameEventListener {
public:
    virtual ~IGameEventListener() = default;
    virtual void onEvent(const GameEvent& event) = 0;
};

} // namespace OpenCraft

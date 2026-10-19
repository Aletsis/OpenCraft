#pragma once

#include <cstdint>
#include <vector>
#include "math/Math.hpp"
#include "sim/Entity.hpp"
#include "world/BlockEntity.hpp"

namespace OpenCraft {

struct EntitySnapshot {
    EntityId id = INVALID_ENTITY_ID;
    Math::Vec3 prevPosition{0.0f, 0.0f, 0.0f};
    Math::Vec3 currentPosition{0.0f, 0.0f, 0.0f};
    float prevYaw = 0.0f;
    float currentYaw = 0.0f;
    float prevPitch = 0.0f;
    float currentPitch = 0.0f;

    Math::Vec3 getInterpolatedPosition(float alpha) const noexcept {
        return prevPosition * (1.0f - alpha) + currentPosition * alpha;
    }

    Math::Vec3 getInterpolatedEyePosition(float alpha) const noexcept {
        return getInterpolatedPosition(alpha) + Math::Vec3(0.0f, 1.62f, 0.0f);
    }

    float getInterpolatedYaw(float alpha) const noexcept { return prevYaw * (1.0f - alpha) + currentYaw * alpha; }

    float getInterpolatedPitch(float alpha) const noexcept { return prevPitch * (1.0f - alpha) + currentPitch * alpha; }
};

enum class OpenContainerKind : uint8_t {
    None = 0,
    Chest = 1,
    CraftingTable = 2,
    Furnace = 3,
    Hopper = 4,
    Dropper = 5,
    Dispenser = 6,
    Crafter = 7
};

struct WorldSnapshot {
    uint64_t tick = 0;
    EntitySnapshot player;
    size_t selectedSlot = 0;
    std::vector<uint16_t> hotbar;
    float health = 20.0f;
    float hunger = 20.0f;
    float saturation = 5.0f;
    int32_t air = 300;
    bool isAlive = true;
    bool isSprinting = false;
    bool isSneaking = false;
    uint32_t gameMode = 0;

    std::vector<ItemStack> inventorySlots;
    ItemStack cursorItem;
    bool isContainerOpen = false;
    OpenContainerKind openContainerKind = OpenContainerKind::None;
    BlockPos openContainerPos{0, 0, 0};
    std::vector<ItemStack> containerSlots;
    uint32_t containerRevision = 0;

    // Cuadrícula de fabricación y salida
    std::vector<ItemStack> craftingGrid;
    ItemStack craftingOutput;

    // Estado del horno si está abierto
    uint32_t furnaceLitTime = 0;
    uint32_t furnaceLitDuration = 0;
    uint32_t furnaceCookProgress = 0;
    uint32_t furnaceCookTotal = 200;

    // Minado progresivo
    float miningProgress = 0.0f;
    BlockPos miningBlockPos{0, 0, 0};

    // Esquema v7: Experiencia y combate
    uint32_t xpLevel = 0;
    float xpProgress = 0.0f;
    bool isBlocking = false;

    // Entidades del mundo
    struct MobSnapshot {
        EntityId id = INVALID_ENTITY_ID;
        uint8_t mobType = 0;
        uint8_t aiState = 0;
        EntitySnapshot transform;
        float health = 20.0f;
        float maxHealth = 20.0f;
        bool isIgnited = false;
        bool isDead = false;
    };
    std::vector<MobSnapshot> mobs;

    struct ProjectileSnapshot {
        EntityId id = INVALID_ENTITY_ID;
        EntitySnapshot transform;
        bool inGround = false;
    };
    std::vector<ProjectileSnapshot> projectiles;

    // Jugadores remotos conectados (Fase 09)
    struct RemotePlayerSnapshot {
        EntityId id = INVALID_ENTITY_ID;
        std::string name;
        EntitySnapshot transform;
        float health = 20.0f;
        bool isSprinting = false;
        bool isSneaking = false;
        bool isBlocking = false;
    };
    std::vector<RemotePlayerSnapshot> remotePlayers;

    // Objetos tirados en el suelo (render)
    struct ItemEntitySnapshot {
        EntityId id = INVALID_ENTITY_ID;
        uint16_t itemId = 0;
        uint16_t count = 0;
        Math::Vec3 position{0.0f, 0.0f, 0.0f};
        uint32_t ageTicks = 0;
    };
    std::vector<ItemEntitySnapshot> itemEntities;

    // Estado adicional del jugador para HUD y cámara
    float armorPoints = 0.0f;
    bool isFlying = false;
    bool isHeadInWater = false;
    bool isHeadInLava = false;
    uint8_t dimension = 0;
    uint32_t hurtTime = 0;
    uint32_t deathTime = 0;
    bool isHardcore = false;
};

} // namespace OpenCraft

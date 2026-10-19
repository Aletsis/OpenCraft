#pragma once

#include <cstdint>
#include <vector>
#include "math/Math.hpp"
#include "item/Inventory.hpp"
#include "progression/StatusEffect.hpp"

namespace OpenCraft {

using EntityId = uint32_t;
constexpr EntityId INVALID_ENTITY_ID = 0;
constexpr EntityId LOCAL_PLAYER_ENTITY_ID = 1;

struct Entity {
    EntityId id = INVALID_ENTITY_ID;
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    Math::Vec3 prevPosition{0.0f, 0.0f, 0.0f};
    Math::Vec3 velocity{0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    float prevYaw = 0.0f;
    float prevPitch = 0.0f;

    bool isGrounded = false;
    bool isAlive = true;
    float width = 0.6f;
    float height = 1.8f;

    Math::AABB getAABB() const noexcept {
        float halfW = width * 0.5f;
        return Math::AABB{.min = {position.x - halfW, position.y, position.z - halfW},
                          .max = {position.x + halfW, position.y + height, position.z + halfW}};
    }

    Math::Vec3 getEyePosition() const noexcept { return position + Math::Vec3(0.0f, 1.62f, 0.0f); }
};

struct PlayerEntity : public Entity {
    float health = 20.0f;
    float maxHealth = 20.0f;
    float hunger = 20.0f;
    float saturation = 5.0f;
    float exhaustion = 0.0f;
    int32_t air = 300;
    int32_t maxAir = 300;
    float fallDistance = 0.0f;
    uint32_t hurtTime = 0;

    bool isFlying = false;
    bool isGliding = false;
    bool isSprinting = false;
    bool isSneaking = false;
    bool isInWater = false;
    bool isInLava = false;
    uint32_t fireTicks = 0;

    // Dimensión en la que se encuentra el jugador (0: Overworld, 1: Abismo, 2: TheEnd)
    uint8_t dimension = 0;
    // Ticks consecutivos dentro de un bloque de portal y enfriamiento tras cruzarlo
    uint32_t portalTicks = 0;
    uint32_t portalCooldown = 0;

    enum class GameModeType : uint32_t {
        Survival = 0,
        Creative = 1,
        Adventure = 2,
        Spectator = 3
    };

    uint32_t gameMode = 0; // 0 = Survival, 1 = Creative, 2 = Adventure, 3 = Spectator

    bool isSurvival() const noexcept { return gameMode == 0; }
    bool isCreative() const noexcept { return gameMode == 1; }
    bool isAdventure() const noexcept { return gameMode == 2; }
    bool isSpectator() const noexcept { return gameMode == 3; }
    bool canBuildAndBreak() const noexcept { return gameMode == 0 || gameMode == 1; }
    bool canInteractWithBlocks() const noexcept { return gameMode != 3; }
    bool hasCollision() const noexcept { return gameMode != 3; }
    bool isInvulnerable() const noexcept { return gameMode == 1 || gameMode == 3; }

    Math::Vec3 spawnPoint{8.0f, 68.0f, 8.0f};
    bool hasCustomSpawnPoint = false;
    uint32_t deathTime = 0;

    Inventory inventory{36};
    ItemStack cursorItem;
    ItemStack offhandItem;

    // Esquema v7: Experiencia y nivel
    uint32_t xpTotal = 0;
    uint32_t xpLevel = 0;
    float xpProgress = 0.0f;

    // Equipamiento de armadura (0: Botas, 1: Pantalones, 2: Peto, 3: Casco)
    std::array<ItemStack, 4> armorSlots;

    // Estado de combate
    bool isBlocking = false;
    uint32_t attackCooldown = 0;

    // Efectos de estado activos (Fase 12)
    StatusEffectManager statusEffects;

    std::vector<uint16_t> hotbar;
    size_t selectedSlot = 0;

    PlayerEntity() {
        id = LOCAL_PLAYER_ENTITY_ID;
        width = 0.6f;
        height = 1.8f;
        hotbar.resize(9, 0);
    }

    static uint32_t getXpNeededForNextLevel(uint32_t level) {
        if (level >= 30) return 9 * level - 158;
        if (level >= 15) return 5 * level - 38;
        return 2 * level + 7;
    }

    void addExperience(uint32_t points) {
        xpTotal += points;
        while (points > 0) {
            uint32_t needed = getXpNeededForNextLevel(xpLevel);
            uint32_t currentInLevel = static_cast<uint32_t>(std::round(xpProgress * static_cast<float>(needed)));
            uint32_t remaining = (needed > currentInLevel) ? (needed - currentInLevel) : 1;
            if (points >= remaining) {
                points -= remaining;
                xpLevel++;
                xpProgress = 0.0f;
            } else {
                currentInLevel += points;
                xpProgress = static_cast<float>(currentInLevel) / static_cast<float>(needed);
                points = 0;
            }
        }
    }

    float getTotalArmorPoints() const {
        float total = 0.0f;
        const auto& itemReg = ItemRegistry::global();
        for (const auto& piece : armorSlots) {
            if (!piece.isEmpty()) {
                total += itemReg.get(piece.itemId).armorPoints;
            }
        }
        return total;
    }

    void syncHotbar() {
        if (hotbar.size() != 9) hotbar.resize(9);
        for (size_t i = 0; i < 9; ++i) {
            const auto& stack = inventory.getSlot(i);
            if (!stack.isEmpty()) {
                const auto& def = ItemRegistry::global().get(stack.itemId);
                hotbar[i] = (def.placeableBlockState != AIR_BLOCK_STATE) ? def.placeableBlockState : stack.itemId;
            } else {
                hotbar[i] = 0;
            }
        }
    }

    void selectSlot(size_t slot) noexcept {
        if (slot < hotbar.size()) {
            selectedSlot = slot;
        }
    }

    const ItemStack& getHeldItem() const noexcept {
        if (selectedSlot < inventory.getSize()) {
            return inventory.getSlot(selectedSlot);
        }
        return cursorItem;
    }
};

} // namespace OpenCraft

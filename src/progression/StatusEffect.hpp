#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include "math/Math.hpp"

namespace OpenCraft {

class WorldSession;
struct Entity;
struct PlayerEntity;
struct MobEntity;

enum class EffectType : uint8_t {
    Speed = 1,
    Slowness = 2,
    Haste = 3,
    MiningFatigue = 4,
    Strength = 5,
    InstantHealth = 6,
    InstantDamage = 7,
    JumpBoost = 8,
    Nausea = 9,
    Regeneration = 10,
    Resistance = 11,
    FireResistance = 12,
    WaterBreathing = 13,
    Invisibility = 14,
    Blindness = 15,
    NightVision = 16,
    Hunger = 17,
    Weakness = 18,
    Poison = 19,
    Decay = 20, // marchitez
    HealthBoost = 21,
    Absorption = 22,
    Saturation = 23,
    Glowing = 24,
    Levitation = 25,
    SlowFalling = 26
};

struct ActiveEffect {
    EffectType type = EffectType::Speed;
    uint32_t durationTicks = 0;
    uint8_t amplifier = 0;  // 0 = Nivel I, 1 = Nivel II
    bool isAmbient = false; // Producido por baliza/entorno

    bool operator==(const ActiveEffect& o) const noexcept {
        return type == o.type && durationTicks == o.durationTicks && amplifier == o.amplifier &&
               isAmbient == o.isAmbient;
    }
};

class StatusEffectManager {
public:
    StatusEffectManager() = default;
    ~StatusEffectManager() = default;

    static std::string_view getEffectName(EffectType type) noexcept;
    static EffectType getEffectFromName(std::string_view name) noexcept;

    void addEffect(ActiveEffect effect);
    void removeEffect(EffectType type);
    void clearAllEffects(); // Antídoto (ej. beber cubeta de leche)

    bool hasEffect(EffectType type) const;
    const ActiveEffect* getEffect(EffectType type) const;
    uint8_t getAmplifier(EffectType type) const;

    const std::vector<ActiveEffect>& getActiveEffects() const noexcept { return m_effects; }
    std::vector<ActiveEffect>& getActiveEffects() noexcept { return m_effects; }

    // Actualización determinista por tick de juego
    void tick(PlayerEntity& player, WorldSession* session);
    void tickMob(MobEntity& mob, WorldSession* session);

    // Modificadores de combate y supervivencia
    float getSpeedMultiplier() const;
    float getDamageMultiplier() const;
    float getIncomingDamageFactor() const;
    bool hasFireImmunity() const;

private:
    std::vector<ActiveEffect> m_effects;
};

} // namespace OpenCraft

#include "progression/StatusEffect.hpp"
#include "sim/Entity.hpp"
#include "sim/MobEntity.hpp"
#include "sim/WorldSession.hpp"
#include <algorithm>

namespace OpenCraft {

std::string_view StatusEffectManager::getEffectName(EffectType type) noexcept {
    switch (type) {
        case EffectType::Speed: return "speed";
        case EffectType::Slowness: return "slowness";
        case EffectType::Haste: return "haste";
        case EffectType::MiningFatigue: return "mining_fatigue";
        case EffectType::Strength: return "strength";
        case EffectType::InstantHealth: return "instant_health";
        case EffectType::InstantDamage: return "instant_damage";
        case EffectType::JumpBoost: return "jump_boost";
        case EffectType::Nausea: return "nausea";
        case EffectType::Regeneration: return "regeneration";
        case EffectType::Resistance: return "resistance";
        case EffectType::FireResistance: return "fire_resistance";
        case EffectType::WaterBreathing: return "water_breathing";
        case EffectType::Invisibility: return "invisibility";
        case EffectType::Blindness: return "blindness";
        case EffectType::NightVision: return "night_vision";
        case EffectType::Hunger: return "hunger";
        case EffectType::Weakness: return "weakness";
        case EffectType::Poison: return "poison";
        case EffectType::Decay: return "decay"; // marchitez
        case EffectType::HealthBoost: return "health_boost";
        case EffectType::Absorption: return "absorption";
        case EffectType::Saturation: return "saturation";
        case EffectType::Glowing: return "glowing";
        case EffectType::Levitation: return "levitation";
        case EffectType::SlowFalling: return "slow_falling";
    }
    return "unknown";
}

EffectType StatusEffectManager::getEffectFromName(std::string_view name) noexcept {
    if (name == "wither") return EffectType::Decay; // nombre anterior a 1.0.0-rc1
    for (uint8_t i = 1; i <= 26; ++i) {
        EffectType t = static_cast<EffectType>(i);
        if (getEffectName(t) == name) return t;
    }
    return EffectType::Speed;
}

void StatusEffectManager::addEffect(ActiveEffect effect) {
    if (effect.durationTicks == 0) return;

    for (auto& existing : m_effects) {
        if (existing.type == effect.type) {
            if (effect.amplifier > existing.amplifier) {
                existing.amplifier = effect.amplifier;
                existing.durationTicks = effect.durationTicks;
                existing.isAmbient = effect.isAmbient;
            } else if (effect.amplifier == existing.amplifier) {
                existing.durationTicks = std::max(existing.durationTicks, effect.durationTicks);
            }
            return;
        }
    }
    m_effects.push_back(effect);
}

void StatusEffectManager::removeEffect(EffectType type) {
    std::erase_if(m_effects, [&](const ActiveEffect& e) { return e.type == type; });
}

void StatusEffectManager::clearAllEffects() { m_effects.clear(); }

bool StatusEffectManager::hasEffect(EffectType type) const {
    return std::any_of(m_effects.begin(), m_effects.end(), [&](const ActiveEffect& e) { return e.type == type; });
}

const ActiveEffect* StatusEffectManager::getEffect(EffectType type) const {
    for (const auto& e : m_effects) {
        if (e.type == type) return &e;
    }
    return nullptr;
}

uint8_t StatusEffectManager::getAmplifier(EffectType type) const {
    const auto* e = getEffect(type);
    return e ? e->amplifier : 0;
}

void StatusEffectManager::tick(PlayerEntity& player, WorldSession* session) {
    for (auto it = m_effects.begin(); it != m_effects.end();) {
        // Ejecución periódica según el tipo de efecto
        uint32_t amp = it->amplifier;

        if (it->type == EffectType::Regeneration) {
            uint32_t period = 50 >> std::min(amp, 4u);
            if (period == 0) period = 1;
            if (it->durationTicks % period == 0) {
                player.health = std::min(player.maxHealth, player.health + 1.0f);
            }
        } else if (it->type == EffectType::Poison) {
            uint32_t period = 25 >> std::min(amp, 4u);
            if (period == 0) period = 1;
            if (it->durationTicks % period == 0) {
                if (player.health > 1.0f) {
                    player.health = std::max(1.0f, player.health - 1.0f);
                }
            }
        } else if (it->type == EffectType::Decay) {
            uint32_t period = 40 >> std::min(amp, 4u);
            if (period == 0) period = 1;
            if (it->durationTicks % period == 0) {
                player.health = std::max(0.0f, player.health - 1.0f);
                if (player.health <= 0.0f && session) {
                    session->applyDamage(1.0f, DamageCause::Magic);
                }
            }
        }

        if (it->durationTicks > 0) {
            --it->durationTicks;
        }

        if (it->durationTicks == 0) {
            it = m_effects.erase(it);
        } else {
            ++it;
        }
    }
}

void StatusEffectManager::tickMob(MobEntity& mob, WorldSession* session) {
    for (auto it = m_effects.begin(); it != m_effects.end();) {
        uint32_t amp = it->amplifier;

        if (it->type == EffectType::Regeneration) {
            uint32_t period = 50 >> std::min(amp, 4u);
            if (period == 0) period = 1;
            if (it->durationTicks % period == 0) {
                mob.health = std::min(mob.maxHealth, mob.health + 1.0f);
            }
        } else if (it->type == EffectType::Poison) {
            uint32_t period = 25 >> std::min(amp, 4u);
            if (period == 0) period = 1;
            if (it->durationTicks % period == 0) {
                if (mob.health > 1.0f) {
                    mob.health = std::max(1.0f, mob.health - 1.0f);
                }
            }
        } else if (it->type == EffectType::Decay) {
            uint32_t period = 40 >> std::min(amp, 4u);
            if (period == 0) period = 1;
            if (it->durationTicks % period == 0) {
                mob.health = std::max(0.0f, mob.health - 1.0f);
            }
        }

        if (it->durationTicks > 0) {
            --it->durationTicks;
        }

        if (it->durationTicks == 0) {
            it = m_effects.erase(it);
        } else {
            ++it;
        }
    }
    (void)session;
}

float StatusEffectManager::getSpeedMultiplier() const {
    float mult = 1.0f;
    if (const auto* spd = getEffect(EffectType::Speed)) {
        mult += 0.2f * (spd->amplifier + 1);
    }
    if (const auto* slow = getEffect(EffectType::Slowness)) {
        mult = std::max(0.1f, mult - 0.15f * (slow->amplifier + 1));
    }
    return mult;
}

float StatusEffectManager::getDamageMultiplier() const {
    float mult = 1.0f;
    if (const auto* str = getEffect(EffectType::Strength)) {
        mult += 0.2f * (str->amplifier + 1); // Or damage bonus scaled
    }
    if (const auto* weak = getEffect(EffectType::Weakness)) {
        mult = std::max(0.1f, mult - 0.2f * (weak->amplifier + 1));
    }
    return mult;
}

float StatusEffectManager::getIncomingDamageFactor() const {
    if (const auto* res = getEffect(EffectType::Resistance)) {
        float reduction = 0.2f * (res->amplifier + 1);
        return std::max(0.0f, 1.0f - reduction);
    }
    return 1.0f;
}

bool StatusEffectManager::hasFireImmunity() const { return hasEffect(EffectType::FireResistance); }

} // namespace OpenCraft

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include "sim/MobEntity.hpp"

namespace OpenCraft {

class WorldSession;

class AnimalBreeding {
public:
    static constexpr uint32_t LOVE_DURATION_TICKS = 600;      // 30 segundos
    static constexpr uint32_t BREEDING_COOLDOWN_TICKS = 6000; // 5 minutos
    static constexpr uint32_t BABY_GROWTH_TICKS = 24000;      // 20 minutos

    static bool isBreedingFood(MobType type, const std::string& itemName) noexcept;
    static bool isTamingFood(MobType type, const std::string& itemName) noexcept;

    // Alimenta una criatura con el ítem sostenido. Activa Love Mode o intento de domesticación
    static bool feedAnimal(MobEntity& mob, const std::string& itemName, EntityId playerId, uint64_t seed);

    // Intenta la reproducción de dos criaturas adultas en celo cercanas
    static bool tryBreeding(MobEntity& parentA, MobEntity& parentB, WorldSession& session, uint64_t seed);
};

} // namespace OpenCraft

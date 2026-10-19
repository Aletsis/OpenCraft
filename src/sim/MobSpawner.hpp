#pragma once

#include <vector>
#include <cstdint>
#include "core/DeterministicRandom.hpp"
#include "sim/MobEntity.hpp"

namespace OpenCraft {

class World;
struct PlayerEntity;

class MobSpawner {
public:
    static constexpr size_t MAX_MONSTERS = 70;
    static constexpr size_t MAX_PASSIVES = 10;
    static constexpr float MIN_SPAWN_DIST = 24.0f;
    static constexpr float MAX_SPAWN_DIST = 64.0f;
    static constexpr float INSTANT_DESPAWN_DIST = 128.0f;
    static constexpr float RANDOM_DESPAWN_DIST = 32.0f;

    MobSpawner() = default;
    ~MobSpawner() = default;

    void tick(World& world, const PlayerEntity& player, std::vector<MobEntity>& mobs, EntityId& nextEntityId,
              uint32_t difficulty, float sunlightFactor, bool allowSpawning = true);

    // Intentar aparición manual de una criatura en coordenadas específicas respetando reglas
    bool canSpawnAt(World& world, MobType type, const BlockPos& pos, uint32_t difficulty, float sunlightFactor) const;

    // Semilla de las apariciones (la sesión usa la del mundo): misma secuencia en todas las plataformas
    void setSeed(uint64_t seed) noexcept { m_rng.seed(seed ^ 0x5350415752ULL); }

private:
    uint32_t m_spawnTimer = 0;
    DeterministicRandom m_rng;
};

} // namespace OpenCraft

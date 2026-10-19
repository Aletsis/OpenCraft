#pragma once

#include <vector>
#include <cstdint>
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
              uint32_t difficulty, float sunlightFactor);

    // Intentar aparición manual de una criatura en coordenadas específicas respetando reglas
    bool canSpawnAt(World& world, MobType type, const BlockPos& pos, uint32_t difficulty, float sunlightFactor) const;

private:
    uint32_t m_spawnTimer = 0;
};

} // namespace OpenCraft

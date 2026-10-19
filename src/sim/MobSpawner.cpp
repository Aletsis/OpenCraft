#include "sim/MobSpawner.hpp"
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include "sim/Entity.hpp"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace OpenCraft {

bool MobSpawner::canSpawnAt(World& world, MobType type, const BlockPos& pos, uint32_t difficulty,
                            float sunlightFactor) const {
    int bx = pos.x;
    int by = pos.y;
    int bz = pos.z;

    if (by < Chunk::MIN_Y + 1 || by >= Chunk::MAX_Y - 2) return false;

    int cx = (bx >= 0) ? (bx / 16) : ((bx - 15) / 16);
    int cz = (bz >= 0) ? (bz / 16) : ((bz - 15) / 16);
    if (!world.isChunkLoaded(cx, cz)) return false;

    const auto& reg = BlockRegistry::global();
    BlockStateId stateAtPos = world.getBlockState(bx, by, bz);
    BlockStateId floorState;
    BlockStateId bodyState;
    BlockStateId headState;
    uint8_t blockLight;
    uint8_t skyLight;

    if (reg.get(stateAtPos).isSolid) {
        // pos es el bloque de suelo
        floorState = stateAtPos;
        bodyState = world.getBlockState(bx, by + 1, bz);
        headState = world.getBlockState(bx, by + 2, bz);
        blockLight = world.getBlockLight(bx, by + 1, bz);
        skyLight = world.getSkyLight(bx, by + 1, bz);
    } else {
        // pos es el bloque donde se posiciona la criatura (bloque de aire sobre el suelo)
        floorState = world.getBlockState(bx, by - 1, bz);
        bodyState = stateAtPos;
        headState = world.getBlockState(bx, by + 1, bz);
        blockLight = world.getBlockLight(bx, by, bz);
        skyLight = world.getSkyLight(bx, by, bz);
    }

    if (!reg.get(floorState).isSolid) return false;
    if (reg.get(bodyState).isSolid || reg.get(headState).isSolid) return false;

    float effectiveLight = static_cast<float>(blockLight) + static_cast<float>(skyLight) * sunlightFactor;

    MobFamily family = getMobFamily(type);
    if (family == MobFamily::Monster) {
        if (difficulty == 0) return false; // En pacífico no aparecen hostiles
        // Hostiles requieren oscuridad total de bloque y cielo oscuro
        if (blockLight > 0) return false;
        if (effectiveLight > 7.0f) return false;
        return true;
    } else {
        // Pasivos requieren luz (>= 9) y suelo de césped
        const auto& floorProps = reg.get(floorState);
        if (floorProps.namespacedId != "opencraft:grass_block" && floorProps.legacyId != BlockID::Grass) {
            return false;
        }
        if (effectiveLight < 9.0f) return false;
        return true;
    }
}

void MobSpawner::tick(World& world, const PlayerEntity& player, std::vector<MobEntity>& mobs, EntityId& nextEntityId,
                      uint32_t difficulty, float sunlightFactor) {
    // 1. Política de Despawn
    for (auto& mob : mobs) {
        if (!mob.isAlive || mob.isDead || mob.isPersistent) continue;

        Math::Vec3 diff = mob.position - player.position;
        float distSq = diff.dot(diff);

        // Despawn instantáneo si está a más de 128 bloques
        if (distSq > INSTANT_DESPAWN_DIST * INSTANT_DESPAWN_DIST) {
            mob.isDead = true;
            mob.isAlive = false;
        }
        // Despawn aleatorio si está a más de 32 bloques y lleva más de 30 segundos (600 ticks)
        else if (distSq > RANDOM_DESPAWN_DIST * RANDOM_DESPAWN_DIST && mob.aiTicks > 600) {
            if (rand() % 800 == 0) {
                mob.isDead = true;
                mob.isAlive = false;
            }
        }
    }

    // Limpiar entidades marcadas como muertas/despawneadas
    mobs.erase(std::remove_if(mobs.begin(), mobs.end(), [](const MobEntity& m) { return m.isDead || !m.isAlive; }),
               mobs.end());

    // 2. Comprobar intervalo de aparición (cada 20 ticks / 1 segundo)
    m_spawnTimer++;
    if (m_spawnTimer < 20) return;
    m_spawnTimer = 0;

    // Contar entidades por familia
    size_t monsterCount = 0;
    size_t passiveCount = 0;
    for (const auto& mob : mobs) {
        if (!mob.isDead && mob.isAlive) {
            if (getMobFamily(mob.mobType) == MobFamily::Monster) {
                monsterCount++;
            } else {
                passiveCount++;
            }
        }
    }

    // Intentar hasta 4 apariciones alrededor del jugador
    for (int attempt = 0; attempt < 4; ++attempt) {
        float angle = static_cast<float>(rand() % 360);
        float rad = Math::radians(angle);
        float dist = MIN_SPAWN_DIST + static_cast<float>(rand() % static_cast<int>(MAX_SPAWN_DIST - MIN_SPAWN_DIST));

        int targetX = static_cast<int>(std::floor(player.position.x + std::cos(rad) * dist));
        int targetZ = static_cast<int>(std::floor(player.position.z + std::sin(rad) * dist));

        // Encontrar la altura del suelo en el rango de altura del jugador ± 10
        int startY =
            std::clamp(static_cast<int>(std::floor(player.position.y)) + 5, Chunk::MIN_Y + 2, Chunk::MAX_Y - 3);
        int foundY = -1;

        for (int y = startY; y >= startY - 12; --y) {
            int cx = (targetX >= 0) ? (targetX / 16) : ((targetX - 15) / 16);
            int cz = (targetZ >= 0) ? (targetZ / 16) : ((targetZ - 15) / 16);
            if (!world.isChunkLoaded(cx, cz)) break;

            if (BlockRegistry::global().get(world.getBlockState(targetX, y, targetZ)).isSolid) {
                foundY = y;
                break;
            }
        }

        if (foundY == -1) continue;

        BlockPos spawnPos{targetX, foundY, targetZ};

        // Decidir si generar monstruo o pasivo
        if (monsterCount < MAX_MONSTERS && difficulty > 0) {
            int roll = rand() % 100;
            MobType mType = MobType::Zombie;
            if (roll < 40)
                mType = MobType::Zombie;
            else if (roll < 70)
                mType = MobType::Skeleton;
            else
                mType = MobType::Lurker;

            if (canSpawnAt(world, mType, spawnPos, difficulty, sunlightFactor)) {
                Math::Vec3 spawnVec(static_cast<float>(targetX) + 0.5f, static_cast<float>(foundY + 1),
                                    static_cast<float>(targetZ) + 0.5f);
                mobs.emplace_back(nextEntityId++, mType, spawnVec);
                monsterCount++;
            }
        } else if (passiveCount < MAX_PASSIVES) {
            MobType pType = MobType::Cow;
            if (canSpawnAt(world, pType, spawnPos, difficulty, sunlightFactor)) {
                Math::Vec3 spawnVec(static_cast<float>(targetX) + 0.5f, static_cast<float>(foundY + 1),
                                    static_cast<float>(targetZ) + 0.5f);
                mobs.emplace_back(nextEntityId++, pType, spawnVec);
                passiveCount++;
            }
        }
    }
}

} // namespace OpenCraft

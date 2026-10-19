#include "sim/BossEntities.hpp"
#include "world/World.hpp"
#include "sim/WorldSession.hpp"
#include "item/Item.hpp"
#include <cmath>
#include <numbers>

namespace OpenCraft {

// ==============================================================================
// EndCrystalEntity
// ==============================================================================
void RimCrystalEntity::explode(WorldSession* session) {
    if (isDestroyed) return;
    isDestroyed = true;
    isAlive = false;
    if (session) {
        session->explode(position, 3.0f, true);
    }
}

// ==============================================================================
// EnderDragonEntity
// ==============================================================================
RimDragonEntity::RimDragonEntity() {
    id = 2000;
    width = 16.0f;
    height = 4.0f;
    position = Math::Vec3(0.0f, 75.0f, 0.0f);
    health = 200.0f;
    maxHealth = 200.0f;
    phase = DragonPhase::Circling;
}

void RimDragonEntity::tick(std::vector<RimCrystalEntity>& crystals, World& world, WorldSession* session) {
    if (phase == DragonPhase::Defeated) return;

    if (phase == DragonPhase::Dying) {
        ++phaseTicks;
        if (phaseTicks >= 200) { // 10 segundos de agonía y luz
            phase = DragonPhase::Defeated;
            isDefeated = true;
            isAlive = false;

            // 1. Activar fuente de salida con portal en (0, 60, 0)
            for (int dz = -1; dz <= 1; ++dz) {
                for (int dx = -1; dx <= 1; ++dx) {
                    world.setBlock(dx, 60, dz, BlockID::RimPortal);
                }
            }

            // 2. Colocar huevo de dragón en la cúspide
            world.setBlock(0, 64, 0, BlockID::DragonEgg);

            // 3. Eyectar 12,000 XP en el centro
            if (session) {
                session->spawnExperienceOrb(Math::Vec3(0.0f, 62.0f, 0.0f), 12000);
            }
        }
        return;
    }

    ++phaseTicks;

    // Curación mediante cristales del Confín activos
    for (const auto& crystal : crystals) {
        if (!crystal.isDestroyed) {
            float dist = (position - crystal.position).length();
            if (dist <= 64.0f) {
                health = std::min(maxHealth, health + 0.1f);
                break;
            }
        }
    }

    // Alternar fases de vuelo
    if (phase == DragonPhase::Circling && phaseTicks >= 600) {
        phase = DragonPhase::Perching;
        phaseTicks = 0;
    } else if (phase == DragonPhase::Perching && phaseTicks >= 300) {
        phase = DragonPhase::Circling;
        phaseTicks = 0;
    }

    (void)world;
    (void)session;
}

void RimDragonEntity::takeDamage(float amount, WorldSession* session) {
    if (phase == DragonPhase::Dying || phase == DragonPhase::Defeated) return;

    health = std::max(0.0f, health - amount);
    if (health <= 0.0f) {
        health = 0.0f;
        phase = DragonPhase::Dying;
        phaseTicks = 0;
    }
    (void)session;
}

bool RimDragonEntity::tryResummon(std::vector<RimCrystalEntity>& crystals, RimDragonEntity& dragon, World& world) {
    if (!dragon.isDefeated) return false;

    // Reinvocación colocando 4 cristales en la fuente
    dragon.health = 200.0f;
    dragon.maxHealth = 200.0f;
    dragon.phase = DragonPhase::Circling;
    dragon.phaseTicks = 0;
    dragon.isDefeated = false;
    dragon.isAlive = true;
    dragon.position = Math::Vec3(0.0f, 75.0f, 0.0f);

    // Regenerar cristales de los pilares
    crystals.clear();
    for (int i = 0; i < 10; ++i) {
        float angle = static_cast<float>(i) * (2.0f * std::numbers::pi_v<float> / 10.0f);
        RimCrystalEntity c;
        c.id = 2100 + i;
        c.position =
            Math::Vec3(42.0f * std::cos(angle), static_cast<float>(76 + i * 3) + 1.0f, 42.0f * std::sin(angle));
        crystals.push_back(c);
    }

    (void)world;
    return true;
}

// ==============================================================================
// WitherEntity
// ==============================================================================
BoneHydraEntity::BoneHydraEntity() {
    id = 3000;
    width = 0.9f;
    height = 3.5f;
    health = 300.0f;
    maxHealth = 300.0f;
    phase = BoneHydraPhase::InvulnerableCharging;
    chargeTicks = 220;
}

bool BoneHydraEntity::checkAndSpawnBoneHydra(World& world, const BlockPos& headPos, Math::Vec3& outSpawnPos) {
    // Comprobar patrón en T:
    // 3 Calaveras en Y: headPos.x - 1, headPos.x, headPos.x + 1
    // 3 Soul Sand en Y-1: headPos.x - 1, headPos.x, headPos.x + 1
    // 1 Soul Sand en Y-2: headPos.x
    int y = headPos.y;

    auto checkAxis = [&](int dx, int dz) -> bool {
        // Comprobar arena de almas
        bool bodyOk = (world.getBlock(headPos.x, y - 1, headPos.z) == BlockID::SoulSand ||
                       world.getBlock(headPos.x, y - 1, headPos.z) == BlockID::SoulSoil) &&
                      (world.getBlock(headPos.x - dx, y - 1, headPos.z - dz) == BlockID::SoulSand ||
                       world.getBlock(headPos.x - dx, y - 1, headPos.z - dz) == BlockID::SoulSoil) &&
                      (world.getBlock(headPos.x + dx, y - 1, headPos.z + dz) == BlockID::SoulSand ||
                       world.getBlock(headPos.x + dx, y - 1, headPos.z + dz) == BlockID::SoulSoil) &&
                      (world.getBlock(headPos.x, y - 2, headPos.z) == BlockID::SoulSand ||
                       world.getBlock(headPos.x, y - 2, headPos.z) == BlockID::SoulSoil);

        if (!bodyOk) return false;

        // Limpiar los bloques de la estructura para invocar al jefe
        world.setBlock(headPos.x, y, headPos.z, BlockID::Air);
        world.setBlock(headPos.x - dx, y, headPos.z - dz, BlockID::Air);
        world.setBlock(headPos.x + dx, y, headPos.z + dz, BlockID::Air);

        world.setBlock(headPos.x, y - 1, headPos.z, BlockID::Air);
        world.setBlock(headPos.x - dx, y - 1, headPos.z - dz, BlockID::Air);
        world.setBlock(headPos.x + dx, y - 1, headPos.z + dz, BlockID::Air);
        world.setBlock(headPos.x, y - 2, headPos.z, BlockID::Air);

        outSpawnPos = Math::Vec3(static_cast<float>(headPos.x) + 0.5f, static_cast<float>(y - 1),
                                 static_cast<float>(headPos.z) + 0.5f);
        return true;
    };

    if (checkAxis(1, 0)) return true;
    if (checkAxis(0, 1)) return true;

    return false;
}

void BoneHydraEntity::tick(WorldSession* session) {
    if (phase == BoneHydraPhase::Defeated) return;

    if (phase == BoneHydraPhase::InvulnerableCharging) {
        if (chargeTicks > 0) {
            --chargeTicks;
            if (chargeTicks == 0) {
                // Explosión masiva de nacimiento
                phase = BoneHydraPhase::Phase1Ranged;
                if (session) {
                    session->explode(position, 7.0f, true);
                }
            }
        }
        return;
    }

    if (phase == BoneHydraPhase::Phase1Ranged) {
        if (health <= 150.0f) {
            // Activar armadura de la Hidra ósea inmune a flechas
            phase = BoneHydraPhase::Phase2Armor;
        }
    }

    (void)session;
}

bool BoneHydraEntity::canBeHitByProjectile() const noexcept { return phase == BoneHydraPhase::Phase1Ranged; }

void BoneHydraEntity::takeDamage(float amount, bool isProjectile, WorldSession* session) {
    if (phase == BoneHydraPhase::InvulnerableCharging || phase == BoneHydraPhase::Defeated) return;

    if (phase == BoneHydraPhase::Phase2Armor && isProjectile) {
        // La armadura de la Hidra ósea refleja / rechaza flechas y proyectiles
        return;
    }

    health = std::max(0.0f, health - amount);
    if (health <= 0.0f) {
        health = 0.0f;
        phase = BoneHydraPhase::Defeated;
        isDefeated = true;
        isAlive = false;

        if (session) {
            // Eyectar Estrella del Abismo garantizada y 50 XP
            ItemId abyssStarId = ItemRegistry::global().getId("opencraft:abyss_star");
            session->spawnDroppedItem(ItemStack(abyssStarId, 1), position, Math::Vec3(0, 1, 0), 0);
            session->spawnExperienceOrb(position, 50);
        }
    }
}

} // namespace OpenCraft

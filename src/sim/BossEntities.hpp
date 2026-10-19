#pragma once

#include "sim/Entity.hpp"
#include "world/BlockEntity.hpp"
#include <vector>

namespace OpenCraft {

class World;
class WorldSession;

// ==============================================================================
// Cristal del Confín (End Crystal)
// ==============================================================================
struct RimCrystalEntity : public Entity {
    bool isDestroyed = false;

    RimCrystalEntity() {
        width = 1.0f;
        height = 1.0f;
    }

    void explode(WorldSession* session);
};

// ==============================================================================
// Dragón del Confín (Dragón del Confín Boss)
// ==============================================================================
enum class DragonPhase : uint8_t {
    Circling = 0,
    StrafeBreath,
    Perching,
    Dying,
    Defeated
};

class RimDragonEntity : public Entity {
public:
    float health = 200.0f;
    float maxHealth = 200.0f;
    DragonPhase phase = DragonPhase::Circling;
    uint32_t phaseTicks = 0;
    bool isDefeated = false;

    RimDragonEntity();

    void tick(std::vector<RimCrystalEntity>& crystals, World& world, WorldSession* session);
    void takeDamage(float amount, WorldSession* session);

    // Reinvocación colocando 4 cristales en la fuente
    static bool tryResummon(std::vector<RimCrystalEntity>& crystals, RimDragonEntity& dragon, World& world);
};

// ==============================================================================
// La Hidra ósea (jefe)
// ==============================================================================
enum class BoneHydraPhase : uint8_t {
    InvulnerableCharging = 0,
    Phase1Ranged,
    Phase2Armor,
    Defeated
};

class BoneHydraEntity : public Entity {
public:
    float health = 300.0f;
    float maxHealth = 300.0f;
    BoneHydraPhase phase = BoneHydraPhase::InvulnerableCharging;
    uint32_t chargeTicks = 220; // 11 segundos de invulnerabilidad
    bool isDefeated = false;

    BoneHydraEntity();

    // Detección y construcción de la estructura invocadora en T de 4 soul sand + 3 calaveras
    static bool checkAndSpawnBoneHydra(World& world, const BlockPos& centerHeadPos, Math::Vec3& outSpawnPos);

    void tick(WorldSession* session);
    void takeDamage(float amount, bool isProjectile, WorldSession* session);
    bool canBeHitByProjectile() const noexcept;
};

} // namespace OpenCraft

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include "sim/Entity.hpp"
#include "sim/ProjectileEntity.hpp"
#include "item/ItemStack.hpp"
#include "world/BlockEntity.hpp"
#include "sim/VillagerEntity.hpp"

namespace OpenCraft {

class World;

enum class MobType : uint8_t {
    Cow = 0,
    Zombie = 1,
    Skeleton = 2,
    Lurker = 3, // antes "acechador"
    Sheep = 4,
    Pig = 5,
    Chicken = 6,
    Wolf = 7,
    Villager = 8,
    Whirl = 9,         // antes "torbellino"
    SwampSkeleton = 10 // antes "bogged"
};

enum class MobFamily : uint8_t {
    Passive = 0,
    Monster = 1
};

enum class AIState : uint8_t {
    Idle = 0,
    Wander = 1,
    Target = 2,
    Chase = 3,
    Attack = 4,
    Flee = 5,
    Ignited = 6,
    Sitting = 7
};

std::string mobTypeToString(MobType type);
MobType mobTypeFromString(std::string_view name);
MobFamily getMobFamily(MobType type);

struct MobEntity : public Entity {
    MobType mobType = MobType::Zombie;
    float health = 20.0f;
    float maxHealth = 20.0f;
    float attackDamage = 3.0f;
    float movementSpeed = 3.0f;

    uint32_t hurtTime = 0;
    uint32_t deathTime = 0;
    bool isDead = false;
    bool isLootDropped = false;
    bool isPersistent = false; // Si fue nombrado, domesticado o protegido contra despawn

    // Cría y domesticación (Fase 10)
    bool inLove = false;
    uint32_t loveTicks = 0;
    uint32_t breedingCooldown = 0;
    bool isBaby = false;
    uint32_t babyAge = 0;
    bool isTamed = false;
    EntityId ownerPlayerId = INVALID_ENTITY_ID;
    bool isSitting = false;

    // Datos de aldeano (si mobType == MobType::Villager)
    VillagerData villagerData;

    AIState aiState = AIState::Idle;
    EntityId targetEntityId = INVALID_ENTITY_ID;

    uint32_t aiTicks = 0;
    uint32_t attackCooldown = 0;
    uint32_t fleeTicks = 0;
    uint32_t fuseTicks = 0; // Para Acechador (detona en 30 ticks)
    bool isIgnited = false;
    uint32_t fireTicks = 0;

    Math::Vec3 wanderTarget{0.0f, 0.0f, 0.0f};
    uint32_t wanderWaitTicks = 0;

    // Navegación A*
    std::vector<BlockPos> currentPath;
    size_t pathIndex = 0;
    bool pathValid = false;
    uint32_t pathCooldown = 0;

    // Registro de ataque cuerpo a cuerpo pendiente en este tick
    bool hasMeleeHit = false;
    float pendingMeleeDamage = 0.0f;

    // Registro de explosión pendiente
    bool hasExploded = false;
    float explosionRadius = 3.0f;

    MobEntity();
    MobEntity(EntityId eId, MobType type, const Math::Vec3& pos);

    void invalidatePath() noexcept {
        pathValid = false;
        currentPath.clear();
        pathIndex = 0;
    }

    // Daño atómico. Retorna true si este impacto causó la muerte de la criatura.
    bool takeDamage(float amount, const Math::Vec3* attackerPos = nullptr, EntityId attackerId = INVALID_ENTITY_ID);

    // Generación garantizada de botín y experiencia en defunción
    std::vector<ItemStack> generateLoot() const;
    uint32_t getExperienceDrop() const;

    // Ticking de IA, movimiento y física
    void tickAI(World& world, const PlayerEntity& player, std::vector<ProjectileEntity>& outProjectiles,
                EntityId& nextEntityId, float sunlightFactor = 1.0f);
    void tickPhysics(World& world, float dt = 0.05f);

    // Búsqueda de ruta con límite estricto de nodos explorados (presupuesto CPU)
    bool computePathTo(World& world, const BlockPos& targetPos, int maxNodes = 256);

private:
    void resolveCollisions(World& world, Math::Vec3& vel, float dt);
    // Paseo aleatorio con pausas; speedScale < 1 para hostiles sin objetivo
    void wander(float speedScale = 1.0f);
};

} // namespace OpenCraft

#include "sim/AnimalBreeding.hpp"
#include "sim/WorldSession.hpp"
#include "world/WorldGenerator.hpp"
#include <cmath>

namespace OpenCraft {

bool AnimalBreeding::isBreedingFood(MobType type, const std::string& itemName) noexcept {
    switch (type) {
        case MobType::Cow:
        case MobType::Sheep: return itemName == "opencraft:wheat";
        case MobType::Pig: return itemName == "opencraft:carrot" || itemName == "opencraft:potato";
        case MobType::Chicken: return itemName == "opencraft:wheat_seeds";
        case MobType::Wolf:
            return itemName == "opencraft:raw_beef" || itemName == "opencraft:cooked_beef" ||
                   itemName == "opencraft:rotten_flesh";
        default: return false;
    }
}

bool AnimalBreeding::isTamingFood(MobType type, const std::string& itemName) noexcept {
    if (type == MobType::Wolf) {
        return itemName == "opencraft:bone";
    }
    return false;
}

bool AnimalBreeding::feedAnimal(MobEntity& mob, const std::string& itemName, EntityId playerId, uint64_t seed) {
    if (mob.isDead || !mob.isAlive) return false;

    // 1. Domesticación de lobo
    if (mob.mobType == MobType::Wolf && !mob.isTamed) {
        if (isTamingFood(mob.mobType, itemName)) {
            // Chance de domesticación (1 de cada 3 o determinista)
            uint64_t roll = WorldGenerator::hash3D(mob.id, static_cast<int64_t>(mob.position.x),
                                                   static_cast<int64_t>(mob.position.z), seed);
            if ((roll % 3) == 0) {
                mob.isTamed = true;
                mob.ownerPlayerId = playerId;
                mob.isPersistent = true;
                mob.maxHealth = 20.0f;
                mob.health = 20.0f;
                return true;
            }
            return false; // Alimento consumido pero falló intento
        }
    }

    // 2. Si ya está domesticado y es lobo, clic derecho alterna sentarse
    if (mob.mobType == MobType::Wolf && mob.isTamed && mob.ownerPlayerId == playerId) {
        if (itemName.empty() || itemName == "opencraft:air") {
            mob.isSitting = !mob.isSitting;
            mob.aiState = mob.isSitting ? AIState::Sitting : AIState::Idle;
            return true;
        }
    }

    // 3. Alimentación de cría para acelerar crecimiento
    if (mob.isBaby && isBreedingFood(mob.mobType, itemName)) {
        mob.babyAge += 2400; // Adelanta 10% del tiempo de maduración
        if (mob.babyAge >= BABY_GROWTH_TICKS) {
            mob.isBaby = false;
        }
        return true;
    }

    // 4. Modo Amor para cría entre adultos
    if (!mob.isBaby && mob.breedingCooldown == 0 && !mob.inLove) {
        if (isBreedingFood(mob.mobType, itemName)) {
            mob.inLove = true;
            mob.loveTicks = LOVE_DURATION_TICKS;
            return true;
        }
    }

    return false;
}

bool AnimalBreeding::tryBreeding(MobEntity& parentA, MobEntity& parentB, WorldSession& session, uint64_t seed) {
    if (parentA.mobType != parentB.mobType) return false;
    if (parentA.id == parentB.id) return false;
    if (!parentA.inLove || !parentB.inLove) return false;
    if (parentA.isBaby || parentB.isBaby) return false;
    if (parentA.breedingCooldown > 0 || parentB.breedingCooldown > 0) return false;

    Math::Vec3 diff = parentA.position - parentB.position;
    float distSq = diff.dot(diff);
    if (distSq > 9.0f) { // Máximo 3 metros
        return false;
    }

    // Procreación exitosa: restablecer padres
    parentA.inLove = false;
    parentA.loveTicks = 0;
    parentA.breedingCooldown = BREEDING_COOLDOWN_TICKS;

    parentB.inLove = false;
    parentB.loveTicks = 0;
    parentB.breedingCooldown = BREEDING_COOLDOWN_TICKS;

    // Generar cría
    Math::Vec3 babyPos = (parentA.position + parentB.position) * 0.5f;
    MobEntity* baby = session.spawnMob(parentA.mobType, babyPos);
    if (baby) {
        baby->isBaby = true;
        baby->babyAge = 0;
        baby->width = parentA.width * 0.5f;
        baby->height = parentA.height * 0.5f;
    }

    // Orbe de XP
    uint32_t xpAmount = static_cast<uint32_t>(1 + (seed % 7));
    session.spawnExperienceOrb(babyPos, xpAmount);

    return true;
}

} // namespace OpenCraft

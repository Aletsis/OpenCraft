#include "sim/MinecartEntity.hpp"
#include "world/World.hpp"
#include "world/CinnabarEngine.hpp"
#include "sim/ItemEntity.hpp"
#include <cmath>

namespace OpenCraft {

MinecartEntity::MinecartEntity() : Entity() {}

MinecartEntity::MinecartEntity(EntityId eId, const Math::Vec3& pos, MinecartType mType)
    : Entity(eId, pos),
      minecartType(mType) {
    if (minecartType == MinecartType::Chest) {
        inventory.resize(27);
    } else if (minecartType == MinecartType::Hopper) {
        inventory.resize(5);
    }
}

void MinecartEntity::tickPhysics(World& world, CinnabarEngine* cinnabar, std::vector<ItemEntity>* worldItems,
                                 float dt) {
    // 1. Detectar bloque de riel debajo o en la posición de la vagoneta
    int bx = static_cast<int>(std::floor(position.x));
    int by = static_cast<int>(std::floor(position.y));
    int bz = static_cast<int>(std::floor(position.z));

    BlockPos railPos{bx, by, bz};
    BlockID curBlock = world.getBlock(bx, by, bz);

    if (!RailSystem::isRail(curBlock)) {
        // Probar medio bloque por debajo
        int byBelow = static_cast<int>(std::floor(position.y - 0.4f));
        if (RailSystem::isRail(world.getBlock(bx, byBelow, bz))) {
            railPos = {bx, byBelow, bz};
            curBlock = world.getBlock(bx, byBelow, bz);
        }
    }

    if (RailSystem::isRail(curBlock)) {
        isOnRail = true;
        // Ajustar altura a la cota del riel
        position.y = static_cast<float>(railPos.y) + 0.1f;
        velocity.y = 0.0f;

        float horizSpeed = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);

        if (RailSystem::isPoweredRail(curBlock)) {
            bool isPowered = cinnabar && (cinnabar->getSignal(railPos) > 0 ||
                                          cinnabar->getDirectOrIndirectPower(world, railPos) > 0);
            if (isPowered) {
                // Riel propulsor activo: acelera hacia la velocidad máxima
                if (horizSpeed < 0.2f) {
                    // Impulso inicial en la dirección del movimiento previo o por defecto
                    velocity.x = (std::abs(velocity.x) > 0.01f) ? (velocity.x > 0 ? 2.0f : -2.0f) : 2.0f;
                    velocity.z = 0.0f;
                } else {
                    float factor = std::min(maxSpeed, horizSpeed + 15.0f * dt) / horizSpeed;
                    velocity.x *= factor;
                    velocity.z *= factor;
                }
            } else {
                // Riel propulsor inactivo: freno de emergencia
                velocity.x *= 0.4f;
                velocity.z *= 0.4f;
            }
        } else if (RailSystem::isDetectorRail(curBlock)) {
            // Riel detector: activa señal 15
            if (cinnabar) {
                cinnabar->setSignal(railPos, 15);
            }
            velocity.x *= 0.98f;
            velocity.z *= 0.98f;
        } else {
            // Riel estándar: fricción canónica leve
            velocity.x *= 0.98f;
            velocity.z *= 0.98f;
        }

        // Integrar movimiento
        position.x += velocity.x * dt;
        position.z += velocity.z * dt;
    } else {
        isOnRail = false;
        // Caída libre si no está sobre vía
        velocity.y -= 9.8f * dt;
        velocity.x *= 0.85f;
        velocity.z *= 0.85f;
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        position.z += velocity.z * dt;
    }

    // 2. Vagoneta con tolva: succión de ítems sobre la vía
    if (minecartType == MinecartType::Hopper && worldItems) {
        for (auto& item : *worldItems) {
            if (item.isDead || item.stack.isEmpty()) continue;

            float distSq = (item.position.x - position.x) * (item.position.x - position.x) +
                           (item.position.y - position.y) * (item.position.y - position.y) +
                           (item.position.z - position.z) * (item.position.z - position.z);

            if (distSq <= 1.5f * 1.5f) {
                uint16_t space = static_cast<uint16_t>(inventory.getAvailableSpace(item.stack));
                if (space > 0) {
                    inventory.addItem(item.stack);
                    if (item.stack.isEmpty()) {
                        item.isDead = true;
                    }
                }
            }
        }
    }
}

} // namespace OpenCraft

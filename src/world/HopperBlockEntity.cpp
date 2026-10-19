#include "world/HopperBlockEntity.hpp"
#include "world/World.hpp"
#include "world/ChestBlockEntity.hpp"
#include "world/FurnaceBlockEntity.hpp"
#include "world/DispenserBlockEntity.hpp"
#include "world/DropperBlockEntity.hpp"
#include "world/CrafterBlockEntity.hpp"
#include "sim/ItemEntity.hpp"

namespace OpenCraft {

BlockPos HopperBlockEntity::getTargetPos() const noexcept {
    switch (m_facing) {
        case HopperFacing::Down: return {m_pos.x, m_pos.y - 1, m_pos.z};
        case HopperFacing::North: return {m_pos.x, m_pos.y, m_pos.z - 1};
        case HopperFacing::South: return {m_pos.x, m_pos.y, m_pos.z + 1};
        case HopperFacing::West: return {m_pos.x - 1, m_pos.y, m_pos.z};
        case HopperFacing::East: return {m_pos.x + 1, m_pos.y, m_pos.z};
        default: return {m_pos.x, m_pos.y - 1, m_pos.z};
    }
}

Inventory* HopperBlockEntity::resolveContainerInventory(World& world, const BlockPos& pos) {
    auto entity = world.getBlockEntity(pos.x, pos.y, pos.z);
    if (!entity) return nullptr;

    switch (entity->getType()) {
        case BlockEntityType::Chest: {
            auto chest = std::static_pointer_cast<ChestBlockEntity>(entity);
            return &chest->getInventory();
        }
        case BlockEntityType::Furnace: {
            auto furnace = std::static_pointer_cast<FurnaceBlockEntity>(entity);
            return &furnace->getInventory();
        }
        case BlockEntityType::Hopper: {
            auto hopper = std::static_pointer_cast<HopperBlockEntity>(entity);
            return &hopper->getInventory();
        }
        case BlockEntityType::Dropper: {
            auto dropper = std::static_pointer_cast<DropperBlockEntity>(entity);
            return &dropper->getInventory();
        }
        case BlockEntityType::Dispenser: {
            auto dispenser = std::static_pointer_cast<DispenserBlockEntity>(entity);
            return &dispenser->getInventory();
        }
        case BlockEntityType::Crafter: {
            auto crafter = std::static_pointer_cast<CrafterBlockEntity>(entity);
            return &crafter->getInventory();
        }
        default: return nullptr;
    }
}

bool HopperBlockEntity::stepTransfer(World& world, std::vector<ItemEntity>& worldItemEntities) {
    if (m_isLocked) {
        return false;
    }

    if (m_cooldown > 0) {
        --m_cooldown;
        return false;
    }

    // 1. Succión de entidades de ítem en la parte superior [x, y+0.5, z] a [x+1, y+1.5, z+1]
    for (auto& ie : worldItemEntities) {
        if (ie.isDead || ie.stack.isEmpty()) continue;

        float minX = static_cast<float>(m_pos.x);
        float maxX = minX + 1.0f;
        float minY = static_cast<float>(m_pos.y) + 0.5f;
        float maxY = minY + 1.5f;
        float minZ = static_cast<float>(m_pos.z);
        float maxZ = minZ + 1.0f;

        if (ie.position.x >= minX && ie.position.x <= maxX && ie.position.y >= minY && ie.position.y <= maxY &&
            ie.position.z >= minZ && ie.position.z <= maxZ) {
            uint16_t space = static_cast<uint16_t>(m_inventory.getAvailableSpace(ie.stack));
            if (space > 0) {
                m_inventory.addItem(ie.stack);
                if (ie.stack.isEmpty()) {
                    ie.isDead = true;
                }
                m_cooldown = TRANSFER_COOLDOWN_TICKS;
                m_inventory.incrementRevision();
                return true;
            }
        }
    }

    // 2. Extracción desde contenedor superior
    BlockPos topPos{m_pos.x, m_pos.y + 1, m_pos.z};
    Inventory* topInv = resolveContainerInventory(world, topPos);
    if (topInv) {
        for (size_t i = 0; i < topInv->getSize(); ++i) {
            auto& slot = topInv->getSlot(i);
            if (!slot.isEmpty()) {
                ItemStack single(slot.itemId, 1, slot.durability);
                if (m_inventory.getAvailableSpace(single) >= 1) {
                    m_inventory.addItem(single);
                    slot.consume(1);
                    topInv->incrementRevision();
                    m_inventory.incrementRevision();
                    m_cooldown = TRANSFER_COOLDOWN_TICKS;
                    return true;
                }
            }
        }
    }

    // 3. Inserción hacia el contenedor de destino
    BlockPos targetPos = getTargetPos();
    Inventory* targetInv = resolveContainerInventory(world, targetPos);
    if (targetInv) {
        auto targetEntity = world.getBlockEntity(targetPos.x, targetPos.y, targetPos.z);
        CrafterBlockEntity* crafterTarget = (targetEntity && targetEntity->getType() == BlockEntityType::Crafter)
                                                ? static_cast<CrafterBlockEntity*>(targetEntity.get())
                                                : nullptr;

        for (size_t i = 0; i < m_inventory.getSize(); ++i) {
            auto& slot = m_inventory.getSlot(i);
            if (!slot.isEmpty()) {
                ItemStack single(slot.itemId, 1, slot.durability);

                if (crafterTarget) {
                    // Si el destino es un crafter, insertar en la primera ranura habilitada con espacio
                    bool insertedInCrafter = false;
                    for (size_t cs = 0; cs < CrafterBlockEntity::CRAFTER_SLOT_COUNT; ++cs) {
                        if (crafterTarget->isSlotDisabled(cs)) continue;
                        auto& cSlot = crafterTarget->getInventory().getSlot(cs);
                        if (cSlot.isEmpty()) {
                            cSlot = single;
                            insertedInCrafter = true;
                            break;
                        } else if (cSlot.canStackWith(single) && cSlot.count < 64) {
                            cSlot.count += 1;
                            insertedInCrafter = true;
                            break;
                        }
                    }
                    if (insertedInCrafter) {
                        slot.consume(1);
                        m_inventory.incrementRevision();
                        crafterTarget->incrementRevision();
                        m_cooldown = TRANSFER_COOLDOWN_TICKS;
                        return true;
                    }
                } else {
                    if (targetInv->getAvailableSpace(single) >= 1) {
                        targetInv->addItem(single);
                        slot.consume(1);
                        m_inventory.incrementRevision();
                        targetInv->incrementRevision();
                        m_cooldown = TRANSFER_COOLDOWN_TICKS;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

} // namespace OpenCraft

#pragma once

#include "world/BlockEntity.hpp"
#include "item/Inventory.hpp"
#include <memory>
#include <vector>

namespace OpenCraft {

class World;
struct ItemEntity;

enum class HopperFacing : uint8_t {
    Down = 0,
    North = 2,
    South = 3,
    West = 4,
    East = 5
};

class HopperBlockEntity : public BlockEntity {
public:
    static constexpr size_t HOPPER_SLOT_COUNT = 5;
    static constexpr uint32_t TRANSFER_COOLDOWN_TICKS = 8; // 8 ticks canónicos (0.4s)

    explicit HopperBlockEntity(BlockPos pos, HopperFacing facing = HopperFacing::Down)
        : BlockEntity(pos),
          m_inventory(HOPPER_SLOT_COUNT),
          m_facing(facing) {}

    BlockEntityType getType() const noexcept override { return BlockEntityType::Hopper; }

    Inventory& getInventory() noexcept { return m_inventory; }
    const Inventory& getInventory() const noexcept { return m_inventory; }

    HopperFacing getFacing() const noexcept { return m_facing; }
    void setFacing(HopperFacing facing) noexcept { m_facing = facing; }

    uint32_t getCooldown() const noexcept { return m_cooldown; }
    void setCooldown(uint32_t cd) noexcept { m_cooldown = cd; }

    bool isLocked() const noexcept { return m_isLocked; }
    void setLocked(bool locked) noexcept { m_isLocked = locked; }

    uint32_t getRevision() const noexcept { return m_inventory.getRevision(); }
    void incrementRevision() noexcept { m_inventory.incrementRevision(); }

    // Ejecuta 1 ciclo de simulación de tolva (succión de ítems superiores, extracción de contenedor superior e
    // inyección en destino)
    bool stepTransfer(World& world, std::vector<ItemEntity>& worldItemEntities);

private:
    Inventory m_inventory;
    HopperFacing m_facing = HopperFacing::Down;
    uint32_t m_cooldown = 0;
    bool m_isLocked = false;

    BlockPos getTargetPos() const noexcept;
    Inventory* resolveContainerInventory(World& world, const BlockPos& pos);
};

} // namespace OpenCraft

#pragma once

#include "world/BlockEntity.hpp"
#include "item/Inventory.hpp"

namespace OpenCraft {

enum class BlockDirection : uint8_t {
    Down = 0,
    Up = 1,
    North = 2,
    South = 3,
    West = 4,
    East = 5
};

class DispenserBlockEntity : public BlockEntity {
public:
    static constexpr size_t DISPENSER_SLOT_COUNT = 9;

    explicit DispenserBlockEntity(BlockPos pos, BlockDirection facing = BlockDirection::North)
        : BlockEntity(pos),
          m_inventory(DISPENSER_SLOT_COUNT),
          m_facing(facing) {}

    BlockEntityType getType() const noexcept override { return BlockEntityType::Dispenser; }

    Inventory& getInventory() noexcept { return m_inventory; }
    const Inventory& getInventory() const noexcept { return m_inventory; }

    BlockDirection getFacing() const noexcept { return m_facing; }
    void setFacing(BlockDirection facing) noexcept { m_facing = facing; }

    bool isTriggered() const noexcept { return m_triggered; }
    void setTriggered(bool triggered) noexcept { m_triggered = triggered; }

    uint32_t getRevision() const noexcept { return m_inventory.getRevision(); }
    void incrementRevision() noexcept { m_inventory.incrementRevision(); }

private:
    Inventory m_inventory;
    BlockDirection m_facing = BlockDirection::North;
    bool m_triggered = false;
};

} // namespace OpenCraft

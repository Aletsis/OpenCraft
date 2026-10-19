#pragma once

#include "world/BlockEntity.hpp"
#include "world/DispenserBlockEntity.hpp"
#include "item/Inventory.hpp"

namespace OpenCraft {

class DropperBlockEntity : public BlockEntity {
public:
    static constexpr size_t DROPPER_SLOT_COUNT = 9;

    explicit DropperBlockEntity(BlockPos pos, BlockDirection facing = BlockDirection::North)
        : BlockEntity(pos),
          m_inventory(DROPPER_SLOT_COUNT),
          m_facing(facing) {}

    BlockEntityType getType() const noexcept override { return BlockEntityType::Dropper; }

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

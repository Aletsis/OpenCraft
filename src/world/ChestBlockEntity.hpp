#pragma once

#include "world/BlockEntity.hpp"
#include "item/Inventory.hpp"
#include <memory>

namespace OpenCraft {

class ChestBlockEntity : public BlockEntity {
public:
    static constexpr size_t CHEST_SLOT_COUNT = 27;

    explicit ChestBlockEntity(BlockPos pos) : BlockEntity(pos), m_inventory(CHEST_SLOT_COUNT) {}

    BlockEntityType getType() const noexcept override { return BlockEntityType::Chest; }

    Inventory& getInventory() noexcept { return m_inventory; }
    const Inventory& getInventory() const noexcept { return m_inventory; }

    uint32_t getRevision() const noexcept { return m_inventory.getRevision(); }
    void incrementRevision() noexcept { m_inventory.incrementRevision(); }

private:
    Inventory m_inventory;
};

} // namespace OpenCraft

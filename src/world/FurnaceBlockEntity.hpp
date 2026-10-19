#pragma once

#include "world/BlockEntity.hpp"
#include "item/Inventory.hpp"
#include "crafting/RecipeRegistry.hpp"
#include <memory>

namespace OpenCraft {

class FurnaceBlockEntity : public BlockEntity {
public:
    static constexpr size_t FURNACE_SLOT_COUNT = 3;
    static constexpr size_t INPUT_SLOT = 0;
    static constexpr size_t FUEL_SLOT = 1;
    static constexpr size_t OUTPUT_SLOT = 2;

    explicit FurnaceBlockEntity(BlockPos pos);

    BlockEntityType getType() const noexcept override { return BlockEntityType::Furnace; }

    Inventory& getInventory() noexcept { return m_inventory; }
    const Inventory& getInventory() const noexcept { return m_inventory; }

    bool isLit() const noexcept { return m_litTimeTicks > 0; }
    uint32_t getLitTimeTicks() const noexcept { return m_litTimeTicks; }
    uint32_t getBurnTimeRemaining() const noexcept { return m_litTimeTicks; }
    void setLitTimeTicks(uint32_t t) noexcept { m_litTimeTicks = t; }

    uint32_t getLitDurationTicks() const noexcept { return m_litDurationTicks; }
    void setLitDurationTicks(uint32_t t) noexcept { m_litDurationTicks = t; }

    uint32_t getCookProgressTicks() const noexcept { return m_cookProgressTicks; }
    void setCookProgressTicks(uint32_t t) noexcept { m_cookProgressTicks = t; }

    uint32_t getCookTotalTicks() const noexcept { return m_cookTotalTicks; }
    void setCookTotalTicks(uint32_t t) noexcept { m_cookTotalTicks = t; }

    uint32_t getRevision() const noexcept { return m_inventory.getRevision(); }
    void incrementRevision() noexcept { m_inventory.incrementRevision(); }

    void tick(const RecipeRegistry& recipes = RecipeRegistry::global());

private:
    Inventory m_inventory;
    uint32_t m_litTimeTicks = 0;
    uint32_t m_litDurationTicks = 0;
    uint32_t m_cookProgressTicks = 0;
    uint32_t m_cookTotalTicks = 200;
};

} // namespace OpenCraft

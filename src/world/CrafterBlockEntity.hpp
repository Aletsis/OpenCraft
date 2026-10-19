#pragma once

#include "world/BlockEntity.hpp"
#include "world/DispenserBlockEntity.hpp"
#include "item/Inventory.hpp"
#include <cstdint>

namespace OpenCraft {

class CrafterBlockEntity : public BlockEntity {
public:
    static constexpr size_t CRAFTER_SLOT_COUNT = 9;

    explicit CrafterBlockEntity(BlockPos pos, BlockDirection facing = BlockDirection::North)
        : BlockEntity(pos),
          m_inventory(CRAFTER_SLOT_COUNT),
          m_facing(facing) {}

    BlockEntityType getType() const noexcept override { return BlockEntityType::Crafter; }

    Inventory& getInventory() noexcept { return m_inventory; }
    const Inventory& getInventory() const noexcept { return m_inventory; }

    BlockDirection getFacing() const noexcept { return m_facing; }
    void setFacing(BlockDirection facing) noexcept { m_facing = facing; }

    bool isTriggered() const noexcept { return m_triggered; }
    void setTriggered(bool triggered) noexcept { m_triggered = triggered; }

    uint16_t getDisabledSlotsMask() const noexcept { return m_disabledSlotsMask; }
    void setDisabledSlotsMask(uint16_t mask) noexcept { m_disabledSlotsMask = mask & 0x01FF; }

    bool isSlotDisabled(size_t slot) const noexcept {
        if (slot >= CRAFTER_SLOT_COUNT) return false;
        return (m_disabledSlotsMask & (1u << slot)) != 0;
    }

    void toggleSlotDisabled(size_t slot) noexcept {
        if (slot >= CRAFTER_SLOT_COUNT) return;
        m_disabledSlotsMask ^= (1u << slot);
        m_inventory.incrementRevision();
    }

    // En el juego de referencia 1.21, la señal de comparador es el número de ranuras deshabilitadas + ranuras ocupadas
    // (0 a 9)
    uint8_t getComparatorSignal() const noexcept {
        uint8_t signal = 0;
        for (size_t i = 0; i < CRAFTER_SLOT_COUNT; ++i) {
            if (isSlotDisabled(i) || !m_inventory.getSlot(i).isEmpty()) {
                ++signal;
            }
        }
        return signal;
    }

    uint32_t getRevision() const noexcept { return m_inventory.getRevision(); }
    void incrementRevision() noexcept { m_inventory.incrementRevision(); }

private:
    Inventory m_inventory;
    BlockDirection m_facing = BlockDirection::North;
    uint16_t m_disabledSlotsMask = 0;
    bool m_triggered = false;
};

} // namespace OpenCraft

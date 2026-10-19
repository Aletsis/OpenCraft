#include "item/Inventory.hpp"
#include <algorithm>

namespace OpenCraft {

Inventory::Inventory(size_t size) : m_slots(size), m_revision(1) {}

void Inventory::resize(size_t newSize) {
    m_slots.resize(newSize);
    incrementRevision();
}

const ItemStack& Inventory::getSlot(size_t index) const {
    if (index < m_slots.size()) {
        return m_slots[index];
    }
    return s_emptyStack;
}

ItemStack& Inventory::getSlot(size_t index) {
    if (index < m_slots.size()) {
        return m_slots[index];
    }
    static ItemStack dummy;
    dummy.clear();
    return dummy;
}

bool Inventory::setSlot(size_t index, const ItemStack& stack) {
    if (index >= m_slots.size()) return false;
    m_slots[index] = stack;
    incrementRevision();
    return true;
}

bool Inventory::moveItem(size_t fromSlot, size_t toSlot, uint16_t amount, uint32_t expectedRevision) {
    if (expectedRevision != 0 && m_revision != expectedRevision) {
        return false;
    }
    if (fromSlot >= m_slots.size() || toSlot >= m_slots.size() || fromSlot == toSlot) {
        return false;
    }
    if (m_slots[fromSlot].isEmpty() || amount == 0) {
        return false;
    }

    uint16_t toMove = std::min(amount, m_slots[fromSlot].count);
    if (m_slots[toSlot].isEmpty()) {
        m_slots[toSlot] = m_slots[fromSlot].split(toMove);
        incrementRevision();
        return true;
    }

    if (m_slots[toSlot].canStackWith(m_slots[fromSlot])) {
        uint16_t maxS = m_slots[toSlot].getMaxStack();
        if (m_slots[toSlot].count >= maxS) {
            return false;
        }
        uint16_t space = maxS - m_slots[toSlot].count;
        uint16_t transfer = std::min(toMove, space);
        m_slots[toSlot].count += transfer;
        m_slots[fromSlot].count -= transfer;
        if (m_slots[fromSlot].count == 0) {
            m_slots[fromSlot].clear();
        }
        incrementRevision();
        return true;
    }

    return false;
}

bool Inventory::swapSlots(size_t slotA, size_t slotB, uint32_t expectedRevision) {
    if (expectedRevision != 0 && m_revision != expectedRevision) {
        return false;
    }
    if (slotA >= m_slots.size() || slotB >= m_slots.size() || slotA == slotB) {
        return false;
    }
    std::swap(m_slots[slotA], m_slots[slotB]);
    incrementRevision();
    return true;
}

bool Inventory::splitSlot(size_t fromSlot, size_t toSlot, uint16_t amount, uint32_t expectedRevision) {
    return moveItem(fromSlot, toSlot, amount, expectedRevision);
}

uint16_t Inventory::addItem(ItemStack& incoming) {
    if (incoming.isEmpty()) return 0;
    uint16_t totalAdded = 0;

    // 1. Intentar apilar en ranuras existentes compatibles
    for (auto& slot : m_slots) {
        if (!slot.isEmpty() && slot.canStackWith(incoming)) {
            uint16_t added = slot.merge(incoming);
            totalAdded += added;
            if (incoming.isEmpty()) {
                incrementRevision();
                return totalAdded;
            }
        }
    }

    // 2. Intentar colocar en ranuras vacías
    for (auto& slot : m_slots) {
        if (slot.isEmpty()) {
            uint16_t added = slot.merge(incoming);
            totalAdded += added;
            if (incoming.isEmpty()) {
                incrementRevision();
                return totalAdded;
            }
        }
    }

    if (totalAdded > 0) {
        incrementRevision();
    }
    return totalAdded;
}

ItemStack Inventory::removeItem(size_t slot, uint16_t count) {
    if (slot >= m_slots.size() || m_slots[slot].isEmpty() || count == 0) {
        return ItemStack();
    }
    ItemStack removed = m_slots[slot].split(count);
    incrementRevision();
    return removed;
}

bool Inventory::hasItem(ItemId id, uint16_t count) const { return countItem(id) >= count; }

uint32_t Inventory::countItem(ItemId id) const {
    uint32_t total = 0;
    for (const auto& slot : m_slots) {
        if (slot.itemId == id) {
            total += slot.count;
        }
    }
    return total;
}

uint32_t Inventory::getTotalItemCount() const noexcept {
    uint32_t total = 0;
    for (const auto& slot : m_slots) {
        total += slot.count;
    }
    return total;
}

bool Inventory::isEmpty() const noexcept {
    for (const auto& slot : m_slots) {
        if (!slot.isEmpty()) return false;
    }
    return true;
}

uint32_t Inventory::getAvailableSpace(const ItemStack& stack) const {
    if (stack.isEmpty()) return 0;
    uint32_t space = 0;
    uint16_t maxSt = stack.getMaxStack();
    for (const auto& slot : m_slots) {
        if (slot.isEmpty()) {
            space += maxSt;
        } else if (slot.canStackWith(stack)) {
            if (slot.count < maxSt) {
                space += (maxSt - slot.count);
            }
        }
    }
    return space;
}

void Inventory::clear() {
    for (auto& slot : m_slots) {
        slot.clear();
    }
    incrementRevision();
}

} // namespace OpenCraft

#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include "item/ItemStack.hpp"

namespace OpenCraft {

class Inventory {
public:
    explicit Inventory(size_t size = 36);
    ~Inventory() = default;

    size_t getSize() const noexcept { return m_slots.size(); }
    void resize(size_t newSize);

    uint32_t getRevision() const noexcept { return m_revision; }
    void incrementRevision() noexcept { ++m_revision; }

    const ItemStack& getSlot(size_t index) const;
    ItemStack& getSlot(size_t index);
    bool setSlot(size_t index, const ItemStack& stack);

    // Operaciones transaccionales entre ranuras del mismo inventario
    bool moveItem(size_t fromSlot, size_t toSlot, uint16_t amount, uint32_t expectedRevision = 0);
    bool swapSlots(size_t slotA, size_t slotB, uint32_t expectedRevision = 0);
    bool splitSlot(size_t fromSlot, size_t toSlot, uint16_t amount, uint32_t expectedRevision = 0);

    // Inserción y extracción segura
    uint16_t addItem(ItemStack& incoming);
    ItemStack removeItem(size_t slot, uint16_t count);

    // Consultas
    bool hasItem(ItemId id, uint16_t count = 1) const;
    uint32_t countItem(ItemId id) const;
    uint32_t getTotalItemCount() const noexcept;
    uint32_t getAvailableSpace(const ItemStack& stack) const;

    bool isEmpty() const noexcept;
    void clear();

    const std::vector<ItemStack>& getSlots() const noexcept { return m_slots; }
    std::vector<ItemStack>& getSlots() noexcept { return m_slots; }

private:
    std::vector<ItemStack> m_slots;
    uint32_t m_revision = 1;

    static inline const ItemStack s_emptyStack{};
};

} // namespace OpenCraft

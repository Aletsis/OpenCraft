#pragma once

#include <cstdint>
#include <vector>
#include "item/Item.hpp"

namespace OpenCraft {

struct ItemStack {
    ItemId itemId = EMPTY_ITEM_ID;
    uint16_t count = 0;
    uint16_t durability = 0;
    std::vector<uint8_t> components; // Datos extensibles (componentes de ítem)

    ItemStack() = default;
    ItemStack(ItemId id, uint16_t cnt = 1, uint16_t dur = 0, std::vector<uint8_t> comps = {})
        : itemId(id),
          count(cnt),
          durability(dur),
          components(std::move(comps)) {
        clamp();
    }

    bool isEmpty() const noexcept { return itemId == EMPTY_ITEM_ID || count == 0; }

    explicit operator bool() const noexcept { return !isEmpty(); }

    uint16_t getMaxStack() const noexcept;

    bool canStackWith(const ItemStack& other) const noexcept;

    // Divide 'amount' unidades de este stack y las devuelve en un nuevo ItemStack
    ItemStack split(uint16_t amount);

    // Une tanto como sea posible desde 'other' a este stack. Devuelve la cantidad transferida.
    uint16_t merge(ItemStack& other);

    // Consume 'amount' unidades. Devuelve verdadero si se pudieron consumir.
    bool consume(uint16_t amount = 1);

    // Inflige daño a herramientas con durabilidad. Devuelve verdadero si el ítem se destruye.
    bool damage(uint16_t amount = 1);

    void clear() noexcept {
        itemId = EMPTY_ITEM_ID;
        count = 0;
        durability = 0;
        components.clear();
    }

    bool operator==(const ItemStack& other) const noexcept {
        if (isEmpty() && other.isEmpty()) return true;
        return itemId == other.itemId && count == other.count && durability == other.durability &&
               components == other.components;
    }

    bool operator!=(const ItemStack& other) const noexcept { return !(*this == other); }

private:
    void clamp() noexcept;
};

} // namespace OpenCraft

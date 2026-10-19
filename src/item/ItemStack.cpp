#include "item/ItemStack.hpp"
#include <algorithm>

namespace OpenCraft {

uint16_t ItemStack::getMaxStack() const noexcept {
    if (itemId == EMPTY_ITEM_ID) return 0;
    return ItemRegistry::global().get(itemId).maxStackSize;
}

void ItemStack::clamp() noexcept {
    if (itemId == EMPTY_ITEM_ID || count == 0) {
        clear();
        return;
    }
    uint16_t maxS = getMaxStack();
    if (maxS > 0 && count > maxS) {
        count = maxS;
    }
}

bool ItemStack::canStackWith(const ItemStack& other) const noexcept {
    if (isEmpty() || other.isEmpty()) return false;
    if (itemId != other.itemId) return false;
    if (durability != other.durability) return false;
    if (components != other.components) return false;
    return getMaxStack() > 1;
}

ItemStack ItemStack::split(uint16_t amount) {
    if (isEmpty() || amount == 0) {
        return ItemStack();
    }
    if (amount >= count) {
        ItemStack result = *this;
        clear();
        return result;
    }
    ItemStack result(itemId, amount, durability, components);
    count -= amount;
    return result;
}

uint16_t ItemStack::merge(ItemStack& other) {
    if (other.isEmpty()) return 0;

    if (isEmpty()) {
        uint16_t maxS = ItemRegistry::global().get(other.itemId).maxStackSize;
        uint16_t toMove = std::min(other.count, maxS);
        *this = other.split(toMove);
        return toMove;
    }

    if (!canStackWith(other)) {
        return 0;
    }

    uint16_t maxS = getMaxStack();
    if (count >= maxS) {
        return 0;
    }

    uint16_t space = maxS - count;
    uint16_t toMove = std::min(space, other.count);
    count += toMove;
    other.count -= toMove;
    if (other.count == 0) {
        other.clear();
    }
    return toMove;
}

bool ItemStack::consume(uint16_t amount) {
    if (isEmpty() || count < amount) {
        return false;
    }
    count -= amount;
    if (count == 0) {
        clear();
    }
    return true;
}

bool ItemStack::damage(uint16_t amount) {
    if (isEmpty()) return false;
    const auto& def = ItemRegistry::global().get(itemId);
    if (def.maxDurability == 0) return false;

    durability += amount;
    if (durability >= def.maxDurability) {
        clear(); // Ítem se rompió por durabilidad
        return true;
    }
    return false;
}

} // namespace OpenCraft

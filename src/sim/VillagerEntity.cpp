#include "sim/VillagerEntity.hpp"
#include "sim/Entity.hpp"
#include "item/Item.hpp"

namespace OpenCraft {

void VillagerData::assignProfession(VillagerProfession prof) {
    profession = prof;
    offers.clear();
    auto& reg = ItemRegistry::global();

    if (prof == VillagerProfession::Farmer) {
        offers.push_back(TradeOffer{.costA = ItemStack(reg.getId("opencraft:wheat"), 20),
                                    .costB = ItemStack(),
                                    .result = ItemStack(reg.getId("opencraft:emerald"), 1),
                                    .uses = 0,
                                    .maxUses = 16});
        offers.push_back(TradeOffer{.costA = ItemStack(reg.getId("opencraft:emerald"), 1),
                                    .costB = ItemStack(),
                                    .result = ItemStack(reg.getId("opencraft:bread"), 6),
                                    .uses = 0,
                                    .maxUses = 12});
        offers.push_back(TradeOffer{.costA = ItemStack(reg.getId("opencraft:carrot"), 15),
                                    .costB = ItemStack(),
                                    .result = ItemStack(reg.getId("opencraft:emerald"), 1),
                                    .uses = 0,
                                    .maxUses = 16});
    } else if (prof == VillagerProfession::Librarian) {
        offers.push_back(TradeOffer{.costA = ItemStack(reg.getId("opencraft:emerald"), 9),
                                    .costB = ItemStack(),
                                    .result = ItemStack(reg.getId("opencraft:bookshelf"), 1),
                                    .uses = 0,
                                    .maxUses = 12});
        offers.push_back(TradeOffer{.costA = ItemStack(reg.getId("opencraft:emerald"), 1),
                                    .costB = ItemStack(),
                                    .result = ItemStack(reg.getId("opencraft:glass"), 4),
                                    .uses = 0,
                                    .maxUses = 12});
    } else if (prof == VillagerProfession::Armorer) {
        offers.push_back(TradeOffer{.costA = ItemStack(reg.getId("opencraft:coal"), 15),
                                    .costB = ItemStack(),
                                    .result = ItemStack(reg.getId("opencraft:emerald"), 1),
                                    .uses = 0,
                                    .maxUses = 16});
        offers.push_back(TradeOffer{.costA = ItemStack(reg.getId("opencraft:iron_ingot"), 4),
                                    .costB = ItemStack(),
                                    .result = ItemStack(reg.getId("opencraft:emerald"), 1),
                                    .uses = 0,
                                    .maxUses = 12});
        offers.push_back(TradeOffer{.costA = ItemStack(reg.getId("opencraft:emerald"), 5),
                                    .costB = ItemStack(),
                                    .result = ItemStack(reg.getId("opencraft:iron_helmet"), 1),
                                    .uses = 0,
                                    .maxUses = 12});
    }
}

TradeResult VillagerData::executeTrade(PlayerEntity& player, size_t offerIndex) {
    if (offerIndex >= offers.size()) {
        return TradeResult::InvalidOffer;
    }

    auto& offer = offers[offerIndex];
    if (offer.isLocked()) {
        return TradeResult::OutOfStock;
    }

    auto& inv = player.inventory;

    // Verificar requisitos
    if (!offer.costA.isEmpty()) {
        if (!inv.hasItem(offer.costA.itemId, offer.costA.count)) {
            return TradeResult::InsufficientItems;
        }
    }
    if (!offer.costB.isEmpty()) {
        if (!inv.hasItem(offer.costB.itemId, offer.costB.count)) {
            return TradeResult::InsufficientItems;
        }
    }

    // Deducción atómica
    if (!offer.costA.isEmpty()) {
        uint16_t toRemove = offer.costA.count;
        for (size_t s = 0; s < inv.getSize() && toRemove > 0; ++s) {
            auto& slot = inv.getSlot(s);
            if (slot.itemId == offer.costA.itemId) {
                uint16_t rem = std::min(toRemove, slot.count);
                inv.removeItem(s, rem);
                toRemove -= rem;
            }
        }
    }

    if (!offer.costB.isEmpty()) {
        uint16_t toRemove = offer.costB.count;
        for (size_t s = 0; s < inv.getSize() && toRemove > 0; ++s) {
            auto& slot = inv.getSlot(s);
            if (slot.itemId == offer.costB.itemId) {
                uint16_t rem = std::min(toRemove, slot.count);
                inv.removeItem(s, rem);
                toRemove -= rem;
            }
        }
    }

    // Entrega de resultado
    ItemStack resCopy = offer.result;
    inv.addItem(resCopy);

    // Incrementar usos de la oferta
    offer.uses++;

    return TradeResult::Success;
}

void VillagerData::restock() {
    for (auto& off : offers) {
        off.uses = 0;
    }
    restocksToday++;
}

} // namespace OpenCraft

#include "world/BrewingStandBlockEntity.hpp"
#include "world/World.hpp"
#include "item/Item.hpp"
#include "progression/StatusEffect.hpp"
#include <cstring>

namespace OpenCraft {

namespace {

// Codificación de pociones en ItemStack::components:
// [0..3]: 'P','O','T','1'
// [4]: EffectType
// [5]: Amplifier (0 o 1)
// [6..7]: DurationTicks (uint16_t)
// [8]: IsSplash (0 o 1)
constexpr uint8_t MAGIC_POT[4] = {'P', 'O', 'T', '1'};

struct DecodedPotion {
    EffectType effect = EffectType::Speed;
    uint8_t amplifier = 0;
    uint16_t durationTicks = 3600; // 3 minutos
    bool isSplash = false;
    bool isAwkward = false;
};

DecodedPotion decodePotion(const ItemStack& stack) {
    DecodedPotion res;
    if (stack.components.size() >= 9 && std::memcmp(stack.components.data(), MAGIC_POT, 4) == 0) {
        res.effect = static_cast<EffectType>(stack.components[4]);
        res.amplifier = stack.components[5];
        res.durationTicks =
            static_cast<uint16_t>(stack.components[6]) | (static_cast<uint16_t>(stack.components[7]) << 8);
        res.isSplash = (stack.components[8] != 0);
        res.isAwkward = (stack.components[4] == 0);
    } else {
        // Frasco de agua base
        res.isAwkward = false;
    }
    return res;
}

std::vector<uint8_t> encodePotion(const DecodedPotion& pot) {
    std::vector<uint8_t> comps(9);
    std::memcpy(comps.data(), MAGIC_POT, 4);
    comps[4] = pot.isAwkward ? 0 : static_cast<uint8_t>(pot.effect);
    comps[5] = pot.amplifier;
    comps[6] = static_cast<uint8_t>(pot.durationTicks & 0xFF);
    comps[7] = static_cast<uint8_t>((pot.durationTicks >> 8) & 0xFF);
    comps[8] = pot.isSplash ? 1 : 0;
    return comps;
}

} // namespace

BrewingStandBlockEntity::BrewingStandBlockEntity(BlockPos pos) : BlockEntity(pos), m_inventory(TOTAL_SLOTS) {}

bool BrewingStandBlockEntity::canBrew() const {
    const auto& reg = ItemRegistry::global();
    const auto& ing = m_inventory.getSlot(SLOT_INGREDIENT);
    if (ing.isEmpty()) return false;

    const std::string& ingName = reg.get(ing.itemId).namespacedId;

    bool hasValidBottle = false;
    for (size_t slot = SLOT_POTION_1; slot <= SLOT_POTION_3; ++slot) {
        const auto& bottle = m_inventory.getSlot(slot);
        if (bottle.isEmpty()) continue;

        const std::string& bName = reg.get(bottle.itemId).namespacedId;
        bool isWaterBottle =
            (bName == "opencraft:water_bottle") ||
            (bName == "opencraft:potion" &&
             (bottle.components.size() < 9 || std::memcmp(bottle.components.data(), MAGIC_POT, 4) != 0));

        if (isWaterBottle && ingName == "opencraft:crimson_fungus") {
            hasValidBottle = true;
            break;
        }

        if (bName == "opencraft:potion" || bName == "opencraft:splash_potion") {
            auto decoded = decodePotion(bottle);
            if (decoded.isAwkward) {
                // Poción rara acepta ingredientes de efectos base
                if (ingName == "opencraft:sugar" || ingName == "opencraft:wraith_tear" ||
                    ingName == "opencraft:ember_powder" || ingName == "opencraft:magma_cream" ||
                    ingName == "opencraft:spider_eye") {
                    hasValidBottle = true;
                    break;
                }
            } else if (!isWaterBottle) {
                // Modificadores: Cinabrio (extensión), Glow Rock (nivel II), Pólvora (splash)
                if (ingName == "opencraft:cinnabar" || ingName == "opencraft:glow_rock_dust" ||
                    ingName == "opencraft:gunpowder") {
                    hasValidBottle = true;
                    break;
                }
            }
        }
    }

    return hasValidBottle;
}

bool BrewingStandBlockEntity::stepBrewing(World& world) {
    const auto& reg = ItemRegistry::global();

    // 1. Recargar combustible desde SLOT_FUEL si está agotado
    if (m_fuel == 0) {
        auto& fuelSlot = m_inventory.getSlot(SLOT_FUEL);
        if (!fuelSlot.isEmpty() && reg.get(fuelSlot.itemId).namespacedId == "opencraft:ember_powder") {
            fuelSlot.consume(1);
            m_fuel = 20; // 20 tandas por polvo de ígneo
        }
    }

    if (!canBrew()) {
        m_brewTime = 0;
        return false;
    }

    if (m_fuel == 0) {
        return false; // Sin combustible
    }

    if (m_brewTime == 0) {
        m_brewTime = BREW_TIME_TOTAL;
    }

    --m_brewTime;

    if (m_brewTime == 0) {
        // Completar tanda de destilación
        auto& ing = m_inventory.getSlot(SLOT_INGREDIENT);
        std::string ingName = reg.get(ing.itemId).namespacedId;

        ItemId potionItemId = reg.getId("opencraft:potion");
        ItemId splashItemId = reg.getId("opencraft:splash_potion");

        for (size_t slot = SLOT_POTION_1; slot <= SLOT_POTION_3; ++slot) {
            auto& bottle = m_inventory.getSlot(slot);
            if (bottle.isEmpty()) continue;

            std::string bName = reg.get(bottle.itemId).namespacedId;
            bool isWaterBottle =
                (bName == "opencraft:water_bottle") ||
                (bName == "opencraft:potion" &&
                 (bottle.components.size() < 9 || std::memcmp(bottle.components.data(), MAGIC_POT, 4) != 0));

            if (isWaterBottle && ingName == "opencraft:crimson_fungus") {
                DecodedPotion p;
                p.isAwkward = true;
                bottle = ItemStack(potionItemId, 1, 0, encodePotion(p));
            } else if (bName == "opencraft:potion" || bName == "opencraft:splash_potion") {
                auto p = decodePotion(bottle);
                if (p.isAwkward) {
                    p.isAwkward = false;
                    if (ingName == "opencraft:sugar") {
                        p.effect = EffectType::Speed;
                        p.durationTicks = 3600; // 3 minutos
                    } else if (ingName == "opencraft:wraith_tear") {
                        p.effect = EffectType::Regeneration;
                        p.durationTicks = 900; // 45 segundos
                    } else if (ingName == "opencraft:ember_powder") {
                        p.effect = EffectType::Strength;
                        p.durationTicks = 3600;
                    } else if (ingName == "opencraft:magma_cream") {
                        p.effect = EffectType::FireResistance;
                        p.durationTicks = 3600;
                    } else if (ingName == "opencraft:spider_eye") {
                        p.effect = EffectType::Poison;
                        p.durationTicks = 900;
                    }
                    bottle = ItemStack(potionItemId, 1, 0, encodePotion(p));
                } else {
                    if (ingName == "opencraft:cinnabar") {
                        p.durationTicks = static_cast<uint16_t>(p.durationTicks * 8 / 3); // Extensión (8 min)
                    } else if (ingName == "opencraft:glow_rock_dust") {
                        p.amplifier = 1; // Nivel II
                        p.durationTicks = static_cast<uint16_t>(p.durationTicks / 2);
                    } else if (ingName == "opencraft:gunpowder") {
                        p.isSplash = true;
                        bottle.itemId = splashItemId;
                    }
                    bottle.components = encodePotion(p);
                }
            }
        }

        ing.consume(1);
        if (m_fuel > 0) {
            --m_fuel;
        }

        (void)world;
        return true;
    }

    return false;
}

} // namespace OpenCraft

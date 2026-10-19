#include "progression/Enchantment.hpp"
#include "item/Item.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace OpenCraft {

namespace {

constexpr uint8_t MAGIC_ENC[4] = {'E', 'N', 'C', '1'};

struct DecodedComponents {
    uint32_t priorWorkPenalty = 0;
    std::vector<EnchantmentInstance> enchantments;
    std::string customName;
};

DecodedComponents decode(const std::vector<uint8_t>& comps) {
    DecodedComponents res;
    if (comps.size() < 7) return res;
    if (std::memcmp(comps.data(), MAGIC_ENC, 4) != 0) return res;

    res.priorWorkPenalty = static_cast<uint32_t>(comps[4]) | (static_cast<uint32_t>(comps[5]) << 8);
    uint8_t count = comps[6];
    size_t offset = 7;

    for (uint8_t i = 0; i < count && offset + 2 <= comps.size(); ++i) {
        EnchantmentId id = static_cast<EnchantmentId>(comps[offset]);
        uint8_t lvl = comps[offset + 1];
        offset += 2;
        res.enchantments.push_back({id, lvl});
    }

    if (offset < comps.size()) {
        uint8_t nameLen = comps[offset++];
        if (offset + nameLen <= comps.size()) {
            res.customName = std::string(reinterpret_cast<const char*>(&comps[offset]), nameLen);
        }
    }

    return res;
}

std::vector<uint8_t> encode(const DecodedComponents& decoded) {
    std::vector<uint8_t> comps;
    comps.insert(comps.end(), MAGIC_ENC, MAGIC_ENC + 4);

    comps.push_back(static_cast<uint8_t>(decoded.priorWorkPenalty & 0xFF));
    comps.push_back(static_cast<uint8_t>((decoded.priorWorkPenalty >> 8) & 0xFF));

    comps.push_back(static_cast<uint8_t>(decoded.enchantments.size()));
    for (const auto& enc : decoded.enchantments) {
        comps.push_back(static_cast<uint8_t>(enc.id));
        comps.push_back(enc.level);
    }

    if (!decoded.customName.empty()) {
        uint8_t nameLen = static_cast<uint8_t>(std::min<size_t>(decoded.customName.size(), 255));
        comps.push_back(nameLen);
        comps.insert(comps.end(), decoded.customName.begin(), decoded.customName.begin() + nameLen);
    }

    return comps;
}

} // namespace

static const EnchantmentInfo s_infoTable[] = {{EnchantmentId::Protection, "Protection", 4, 10},
                                              {EnchantmentId::FireProtection, "Fire Protection", 4, 5},
                                              {EnchantmentId::FeatherFalling, "Feather Falling", 4, 5},
                                              {EnchantmentId::BlastProtection, "Blast Protection", 4, 2},
                                              {EnchantmentId::ProjectileProtection, "Projectile Protection", 4, 5},
                                              {EnchantmentId::Respiration, "Respiration", 3, 2},
                                              {EnchantmentId::AquaAffinity, "Aqua Affinity", 1, 2},
                                              {EnchantmentId::Thorns, "Thorns", 3, 1},
                                              {EnchantmentId::Sharpness, "Sharpness", 5, 10},
                                              {EnchantmentId::Smite, "Smite", 5, 5},
                                              {EnchantmentId::BaneOfArthropods, "Bane of Arthropods", 5, 5},
                                              {EnchantmentId::Knockback, "Knockback", 2, 5},
                                              {EnchantmentId::FireAspect, "Fire Aspect", 2, 2},
                                              {EnchantmentId::Looting, "Looting", 3, 2},
                                              {EnchantmentId::Efficiency, "Efficiency", 5, 10},
                                              {EnchantmentId::SilkTouch, "Silk Touch", 1, 1},
                                              {EnchantmentId::Unbreaking, "Unbreaking", 3, 5},
                                              {EnchantmentId::Fortune, "Fortune", 3, 2},
                                              {EnchantmentId::Power, "Power", 5, 10},
                                              {EnchantmentId::Punch, "Punch", 2, 2},
                                              {EnchantmentId::Flame, "Flame", 1, 2},
                                              {EnchantmentId::Infinity, "Infinity", 1, 1},
                                              {EnchantmentId::Mending, "Mending", 1, 2},
                                              {EnchantmentId::Density, "Density", 5, 5},
                                              {EnchantmentId::Breach, "Breach", 4, 2},
                                              {EnchantmentId::WindBurst, "Wind Burst", 3, 2}};

const EnchantmentInfo& EnchantmentSystem::getInfo(EnchantmentId id) noexcept {
    size_t idx = static_cast<size_t>(id);
    if (idx < sizeof(s_infoTable) / sizeof(s_infoTable[0])) {
        return s_infoTable[idx];
    }
    return s_infoTable[0];
}

std::string_view EnchantmentSystem::getName(EnchantmentId id) noexcept { return getInfo(id).name; }

uint8_t EnchantmentSystem::getMaxLevel(EnchantmentId id) noexcept { return getInfo(id).maxLevel; }

bool EnchantmentSystem::areIncompatible(EnchantmentId a, EnchantmentId b) noexcept {
    if (a == b) return false;

    // Mutuamente excluyentes de daño
    bool aDmg = (a == EnchantmentId::Sharpness || a == EnchantmentId::Smite || a == EnchantmentId::BaneOfArthropods);
    bool bDmg = (b == EnchantmentId::Sharpness || b == EnchantmentId::Smite || b == EnchantmentId::BaneOfArthropods);
    if (aDmg && bDmg) return true;

    // Mutuamente excluyentes de protección
    bool aProt = (a == EnchantmentId::Protection || a == EnchantmentId::FireProtection ||
                  a == EnchantmentId::BlastProtection || a == EnchantmentId::ProjectileProtection);
    bool bProt = (b == EnchantmentId::Protection || b == EnchantmentId::FireProtection ||
                  b == EnchantmentId::BlastProtection || b == EnchantmentId::ProjectileProtection);
    if (aProt && bProt) return true;

    // Densidad y Ruptura (Density y Breach de la Maza)
    if ((a == EnchantmentId::Density && b == EnchantmentId::Breach) ||
        (a == EnchantmentId::Breach && b == EnchantmentId::Density))
        return true;

    // Toque de seda y Fortuna
    if ((a == EnchantmentId::SilkTouch && b == EnchantmentId::Fortune) ||
        (a == EnchantmentId::Fortune && b == EnchantmentId::SilkTouch))
        return true;

    // Infinidad y Reparación
    if ((a == EnchantmentId::Infinity && b == EnchantmentId::Mending) ||
        (a == EnchantmentId::Mending && b == EnchantmentId::Infinity))
        return true;

    return false;
}

std::vector<EnchantmentInstance> EnchantmentSystem::getEnchantments(const ItemStack& stack) {
    return decode(stack.components).enchantments;
}

void EnchantmentSystem::setEnchantments(ItemStack& stack, const std::vector<EnchantmentInstance>& enchants) {
    auto d = decode(stack.components);
    d.enchantments = enchants;
    stack.components = encode(d);
}

void EnchantmentSystem::addEnchantment(ItemStack& stack, EnchantmentInstance instance) {
    auto d = decode(stack.components);
    for (auto& e : d.enchantments) {
        if (e.id == instance.id) {
            e.level = std::max(e.level, instance.level);
            stack.components = encode(d);
            return;
        }
    }
    d.enchantments.push_back(instance);
    stack.components = encode(d);
}

uint8_t EnchantmentSystem::getEnchantmentLevel(const ItemStack& stack, EnchantmentId id) {
    for (const auto& e : getEnchantments(stack)) {
        if (e.id == id) return e.level;
    }
    return 0;
}

bool EnchantmentSystem::hasEnchantment(const ItemStack& stack, EnchantmentId id) {
    return getEnchantmentLevel(stack, id) > 0;
}

uint32_t EnchantmentSystem::getPriorWorkPenalty(const ItemStack& stack) {
    return decode(stack.components).priorWorkPenalty;
}

void EnchantmentSystem::setPriorWorkPenalty(ItemStack& stack, uint32_t penalty) {
    auto d = decode(stack.components);
    d.priorWorkPenalty = penalty;
    stack.components = encode(d);
}

AnvilResult EnchantmentSystem::combineInAnvil(const ItemStack& left, const ItemStack& right,
                                              const std::string& newCustomName, bool isCreative) {
    AnvilResult result;
    if (left.isEmpty()) return result;

    const auto& itemReg = ItemRegistry::global();
    auto leftDecoded = decode(left.components);
    auto rightDecoded = decode(right.components);

    uint32_t basePenaltyCost = leftDecoded.priorWorkPenalty + rightDecoded.priorWorkPenalty;
    uint32_t enchantCost = 0;
    uint32_t repairCost = 0;
    uint32_t renameCost = 0;

    ItemStack out = left;
    auto outDecoded = leftDecoded;

    bool hasOperation = false;

    // 1. Renombrado
    if (!newCustomName.empty() && newCustomName != leftDecoded.customName) {
        outDecoded.customName = newCustomName;
        renameCost = 1;
        hasOperation = true;
    }

    // 2. Combinación / Reparación
    if (!right.isEmpty()) {
        bool isBook = (itemReg.get(right.itemId).namespacedId == "opencraft:enchanted_book");
        bool isSameItem = (left.itemId == right.itemId);

        if (isSameItem || isBook) {
            hasOperation = true;

            // Fusión de durabilidad si es el mismo ítem
            if (isSameItem && left.durability > 0) {
                const auto& def = itemReg.get(left.itemId);
                if (def.maxDurability > 0) {
                    uint16_t rightDurabilityLeft =
                        (right.durability < def.maxDurability) ? (def.maxDurability - right.durability) : 0;
                    uint16_t bonus = static_cast<uint16_t>(def.maxDurability * 12 / 100);
                    uint16_t repairAmount = rightDurabilityLeft + bonus;

                    if (out.durability > repairAmount) {
                        out.durability -= repairAmount;
                    } else {
                        out.durability = 0;
                    }
                    repairCost += 2;
                }
            }

            // Fusión de encantamientos
            for (const auto& rEnc : rightDecoded.enchantments) {
                bool conflict = false;
                for (const auto& lEnc : outDecoded.enchantments) {
                    if (areIncompatible(lEnc.id, rEnc.id)) {
                        conflict = true;
                        break;
                    }
                }
                if (conflict) continue;

                auto it = std::find_if(outDecoded.enchantments.begin(), outDecoded.enchantments.end(),
                                       [&](const EnchantmentInstance& e) { return e.id == rEnc.id; });

                if (it == outDecoded.enchantments.end()) {
                    outDecoded.enchantments.push_back(rEnc);
                    enchantCost += rEnc.level * 1;
                } else {
                    uint8_t maxL = getMaxLevel(rEnc.id);
                    if (it->level == rEnc.level) {
                        it->level = std::min<uint8_t>(maxL, it->level + 1);
                        enchantCost += it->level * 2;
                    } else if (it->level < rEnc.level) {
                        it->level = rEnc.level;
                        enchantCost += it->level * 1;
                    } else {
                        enchantCost += 1;
                    }
                }
            }
        }
    }

    if (!hasOperation) {
        return result;
    }

    uint32_t totalLevelCost = basePenaltyCost + enchantCost + repairCost + renameCost;
    if (totalLevelCost == 0) totalLevelCost = 1;

    // Actualizar penalización de trabajo previo: (max * 2 + 1)
    uint32_t maxPrevPenalty = std::max(leftDecoded.priorWorkPenalty, rightDecoded.priorWorkPenalty);
    outDecoded.priorWorkPenalty = (maxPrevPenalty * 2) + 1;

    out.components = encode(outDecoded);

    result.resultItem = out;
    result.xpLevelCost = totalLevelCost;
    result.success = true;

    if (totalLevelCost >= 40 && !isCreative) {
        result.tooExpensive = true;
        result.success = false;
    }

    return result;
}

std::pair<ItemStack, uint32_t> EnchantmentSystem::disenchant(const ItemStack& item) {
    if (item.isEmpty()) return {item, 0};

    auto decoded = decode(item.components);
    if (decoded.enchantments.empty()) {
        return {item, 0};
    }

    uint32_t refundedXp = 0;
    for (const auto& enc : decoded.enchantments) {
        refundedXp += static_cast<uint32_t>(enc.level) * 15;
    }

    ItemStack cleanItem = item;
    // Si era un libro encantado, se convierte en un libro común
    if (ItemRegistry::global().get(item.itemId).namespacedId == "opencraft:enchanted_book") {
        cleanItem.itemId = ItemRegistry::global().getId("opencraft:book");
    }

    decoded.enchantments.clear();
    decoded.priorWorkPenalty = 0;
    cleanItem.components = encode(decoded);

    return {cleanItem, refundedXp};
}

bool EnchantmentSystem::canUpgradeInSmithingTable(const ItemStack& base, const ItemStack& templateItem,
                                                  const ItemStack& addition) {
    if (base.isEmpty() || templateItem.isEmpty() || addition.isEmpty()) return false;

    const auto& reg = ItemRegistry::global();
    if (reg.get(templateItem.itemId).namespacedId != "opencraft:orichalcum_upgrade_smithing_template") return false;
    if (reg.get(addition.itemId).namespacedId != "opencraft:orichalcum_ingot") return false;

    const std::string& baseName = reg.get(base.itemId).namespacedId;
    return baseName.starts_with("opencraft:diamond_");
}

ItemStack EnchantmentSystem::upgradeInSmithingTable(const ItemStack& base, const ItemStack& templateItem,
                                                    const ItemStack& addition) {
    if (!canUpgradeInSmithingTable(base, templateItem, addition)) return ItemStack();

    const auto& reg = ItemRegistry::global();
    const std::string& baseName = reg.get(base.itemId).namespacedId;
    std::string orichalcumName = "opencraft:netherite_" + baseName.substr(std::string("opencraft:diamond_").length());

    ItemId targetId = reg.getId(orichalcumName);
    if (targetId == EMPTY_ITEM_ID) return ItemStack();

    ItemStack result = base;
    result.itemId = targetId;
    // Preserva componentes (encantamientos, custom name, prior work penalty) y durabilidad
    return result;
}

std::array<EnchantmentOffer, 3> EnchantmentSystem::generateOffers(uint64_t seed, uint32_t bookshelfCount) {
    uint32_t cappedBookshelves = std::min(bookshelfCount, 15u);
    uint32_t base = 1 + (cappedBookshelves / 2);

    std::array<EnchantmentOffer, 3> offers;

    // Nivel 1 (Bajo)
    offers[0].levelRequirement = std::max(1u, base / 3);
    offers[0].lapisCost = 1;
    offers[0].preview = {EnchantmentId::Unbreaking, 1};

    // Nivel 2 (Medio)
    offers[1].levelRequirement = std::max(2u, (base * 2) / 3);
    offers[1].lapisCost = 2;
    offers[1].preview = {EnchantmentId::Protection, (cappedBookshelves >= 8) ? (uint8_t)2 : (uint8_t)1};

    // Nivel 3 (Alto, hasta 30)
    uint32_t topLevel = std::min(30u, base * 2);
    if (cappedBookshelves >= 15) topLevel = 30;
    offers[2].levelRequirement = topLevel;
    offers[2].lapisCost = 3;
    offers[2].preview = {EnchantmentId::Sharpness, (topLevel >= 30) ? (uint8_t)4 : (uint8_t)3};

    (void)seed;
    return offers;
}

bool EnchantmentSystem::applyEnchantingOffer(ItemStack& item, uint32_t offerIndex, uint32_t bookshelfCount,
                                             uint32_t playerLevel, uint32_t lapisCount) {
    if (offerIndex >= 3) return false;
    auto offers = generateOffers(0, bookshelfCount);
    const auto& offer = offers[offerIndex];

    if (playerLevel < offer.levelRequirement) return false;
    if (lapisCount < offer.lapisCost) return false;

    // Si es un libro común, se convierte en un libro encantado
    if (ItemRegistry::global().get(item.itemId).namespacedId == "opencraft:book") {
        item.itemId = ItemRegistry::global().getId("opencraft:enchanted_book");
    }

    addEnchantment(item, offer.preview);
    return true;
}

} // namespace OpenCraft

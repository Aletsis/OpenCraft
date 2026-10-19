#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <span>
#include "item/ItemStack.hpp"

namespace OpenCraft {

enum class EnchantmentId : uint8_t {
    Protection = 0,
    FireProtection,
    FeatherFalling,
    BlastProtection,
    ProjectileProtection,
    Respiration,
    AquaAffinity,
    Thorns,
    Sharpness,
    Smite,
    BaneOfArthropods,
    Knockback,
    FireAspect,
    Looting,
    Efficiency,
    SilkTouch,
    Unbreaking,
    Fortune,
    Power,
    Punch,
    Flame,
    Infinity,
    Mending,
    Density,
    Breach,
    WindBurst,
    Count
};

struct EnchantmentInstance {
    EnchantmentId id = EnchantmentId::Protection;
    uint8_t level = 1;

    bool operator==(const EnchantmentInstance& o) const noexcept { return id == o.id && level == o.level; }
};

struct EnchantmentInfo {
    EnchantmentId id;
    std::string_view name;
    uint8_t maxLevel;
    uint8_t weight; // Probabilidad de aparición
};

struct AnvilResult {
    ItemStack resultItem;
    uint32_t xpLevelCost = 0;
    bool success = false;
    bool tooExpensive = false; // > 39 niveles en supervivencia
};

struct EnchantmentOffer {
    uint32_t levelRequirement = 1;
    uint32_t lapisCost = 1;
    EnchantmentInstance preview;
};

class EnchantmentSystem {
public:
    static const EnchantmentInfo& getInfo(EnchantmentId id) noexcept;
    static std::string_view getName(EnchantmentId id) noexcept;
    static uint8_t getMaxLevel(EnchantmentId id) noexcept;

    // Reglas canónicas de incompatibilidad
    static bool areIncompatible(EnchantmentId a, EnchantmentId b) noexcept;

    // Serialización y Deserialización en ItemStack::components
    static std::vector<EnchantmentInstance> getEnchantments(const ItemStack& stack);
    static void setEnchantments(ItemStack& stack, const std::vector<EnchantmentInstance>& enchants);
    static void addEnchantment(ItemStack& stack, EnchantmentInstance instance);
    static uint8_t getEnchantmentLevel(const ItemStack& stack, EnchantmentId id);
    static bool hasEnchantment(const ItemStack& stack, EnchantmentId id);

    // Penalización por trabajo previo (Prior Work Penalty: 2^n - 1)
    static uint32_t getPriorWorkPenalty(const ItemStack& stack);
    static void setPriorWorkPenalty(ItemStack& stack, uint32_t penalty);

    // Lógica del Yunque (Anvil)
    static AnvilResult combineInAnvil(const ItemStack& left, const ItemStack& right,
                                      const std::string& newCustomName = "", bool isCreative = false);

    // Lógica de la Afiladora (Grindstone): remueve encantamientos y reembolsa XP
    static std::pair<ItemStack, uint32_t> disenchant(const ItemStack& item);

    // Lógica de la Mesa de Herrería (Smithing Table): mejora a Oricalco
    static bool canUpgradeInSmithingTable(const ItemStack& base, const ItemStack& templateItem,
                                          const ItemStack& addition);
    static ItemStack upgradeInSmithingTable(const ItemStack& base, const ItemStack& templateItem,
                                            const ItemStack& addition);

    // Lógica de la Mesa de Encantamientos (Enchanting Table)
    static std::array<EnchantmentOffer, 3> generateOffers(uint64_t seed, uint32_t bookshelfCount);
    static bool applyEnchantingOffer(ItemStack& item, uint32_t offerIndex, uint32_t bookshelfCount,
                                     uint32_t playerLevel, uint32_t lapisCount);
};

} // namespace OpenCraft

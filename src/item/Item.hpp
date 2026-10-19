#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include "world/BlockRegistry.hpp"

namespace OpenCraft {

using ItemId = uint16_t;
constexpr ItemId EMPTY_ITEM_ID = 0;

enum class ArmorSlot : uint8_t {
    None = 0,
    Helmet = 1,
    Chestplate = 2,
    Leggings = 3,
    Boots = 4,
    Shield = 5
};

struct ItemDefinition {
    ItemId id = EMPTY_ITEM_ID;
    std::string namespacedId = "opencraft:air";
    std::string displayName = "Air";
    uint16_t maxStackSize = 64;
    uint16_t maxDurability = 0;                         // 0 significa que no sufre daño/durabilidad
    BlockStateId placeableBlockState = AIR_BLOCK_STATE; // Si se coloca como bloque
    uint8_t iconTileIndex = 0;
    ToolType toolType = ToolType::None;
    ToolTier toolTier = ToolTier::None;
    float miningEfficiency = 1.0f;
    uint32_t fuelBurnTicks = 0; // Ticks de combustible en horno (0 = no es combustible)
    bool isFood = false;
    uint16_t foodRestore = 0;
    float saturationRestore = 0.0f;
    float attackDamage = 1.0f; // Daño base (1.0f = puño)
    float armorPoints = 0.0f;  // Puntos de armadura (0.0f - 20.0f)
    ArmorSlot armorSlot = ArmorSlot::None;
};

class ItemRegistry {
public:
    ItemRegistry();
    ~ItemRegistry() = default;

    static ItemRegistry& global();

    ItemId registerItem(ItemDefinition def);

    const ItemDefinition& get(ItemId id) const noexcept;
    const ItemDefinition& get(std::string_view namespacedId) const noexcept;

    ItemId getId(std::string_view namespacedId) const noexcept;
    bool hasItem(std::string_view namespacedId) const noexcept;

    ItemId getItemForBlock(BlockStateId blockState) const noexcept;

    size_t getItemCount() const noexcept { return m_definitions.size(); }

    void reset();

private:
    std::vector<ItemDefinition> m_definitions;
    std::unordered_map<std::string, ItemId> m_nameToId;
    std::unordered_map<BlockStateId, ItemId> m_blockToItem;

    void registerDefaults();
    void assignSpriteIcons();
};

} // namespace OpenCraft

#include "item/Item.hpp"
#include "world/IdAliases.hpp"
#include "render/AtlasPainter.hpp"
#include <stdexcept>

namespace OpenCraft {

ItemRegistry::ItemRegistry() { reset(); }

ItemRegistry& ItemRegistry::global() {
    static ItemRegistry instance;
    return instance;
}

void ItemRegistry::reset() {
    m_definitions.clear();
    m_nameToId.clear();
    m_blockToItem.clear();
    registerDefaults();
    assignSpriteIcons();
}

ItemId ItemRegistry::registerItem(ItemDefinition def) {
    if (m_nameToId.find(def.namespacedId) != m_nameToId.end()) {
        ItemId existingId = m_nameToId[def.namespacedId];
        m_definitions[existingId] = def;
        def.id = existingId;
        if (def.placeableBlockState != AIR_BLOCK_STATE) {
            m_blockToItem[def.placeableBlockState] = existingId;
        }
        return existingId;
    }

    ItemId newId = static_cast<ItemId>(m_definitions.size());
    def.id = newId;
    m_definitions.push_back(def);
    m_nameToId[def.namespacedId] = newId;
    if (def.placeableBlockState != AIR_BLOCK_STATE) {
        m_blockToItem[def.placeableBlockState] = newId;
    }
    return newId;
}

const ItemDefinition& ItemRegistry::get(ItemId id) const noexcept {
    if (id < m_definitions.size()) {
        return m_definitions[id];
    }
    return m_definitions[EMPTY_ITEM_ID];
}

const ItemDefinition& ItemRegistry::get(std::string_view namespacedId) const noexcept {
    auto it = m_nameToId.find(IdAliases::canonical(namespacedId));
    if (it != m_nameToId.end()) {
        return m_definitions[it->second];
    }
    return m_definitions[EMPTY_ITEM_ID];
}

ItemId ItemRegistry::getId(std::string_view namespacedId) const noexcept {
    auto it = m_nameToId.find(IdAliases::canonical(namespacedId));
    if (it != m_nameToId.end()) {
        return it->second;
    }
    return EMPTY_ITEM_ID;
}

bool ItemRegistry::hasItem(std::string_view namespacedId) const noexcept {
    return m_nameToId.find(IdAliases::canonical(namespacedId)) != m_nameToId.end();
}

ItemId ItemRegistry::getItemForBlock(BlockStateId blockState) const noexcept {
    auto it = m_blockToItem.find(blockState);
    if (it != m_blockToItem.end()) {
        return it->second;
    }
    return EMPTY_ITEM_ID;
}

void ItemRegistry::assignSpriteIcons() {
    // Los objetos con sprite propio en el atlas lo usan en lugar de la textura de un bloque
    for (auto& def : m_definitions) {
        int tile = AtlasPainter::itemIconTile(def.namespacedId);
        if (tile >= 0) def.iconTileIndex = static_cast<uint8_t>(tile);
    }
}

void ItemRegistry::registerDefaults() {
    // 0. Aire / Vacío
    registerItem(ItemDefinition{.id = EMPTY_ITEM_ID,
                                .namespacedId = "opencraft:air",
                                .displayName = "Air",
                                .maxStackSize = 0,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 0});

    auto& blockReg = BlockRegistry::global();

    // Mapeo automático de bloques vanilla existentes a ítems
    for (size_t i = 1; i < blockReg.getStateCount(); ++i) {
        BlockStateId st = static_cast<BlockStateId>(i);
        const auto& bProps = blockReg.get(st);

        uint8_t iconTile = bProps.textureIndices[0]; // Vista previa cara superior
        uint32_t fuel = 0;
        if (bProps.namespacedId == "opencraft:wood_log" || bProps.namespacedId == "opencraft:planks") {
            fuel = 300; // Quema 1.5 ítems por pieza
        }

        registerItem(ItemDefinition{.id = 0, // Se autoasigna
                                    .namespacedId = bProps.namespacedId,
                                    .displayName = bProps.displayName,
                                    .maxStackSize = 64,
                                    .maxDurability = 0,
                                    .placeableBlockState = st,
                                    .iconTileIndex = iconTile,
                                    .fuelBurnTicks = fuel});
    }

    // Ítems no-bloque de OpenCraft
    registerItem(ItemDefinition{
        .id = 0,
        .namespacedId = "opencraft:stick",
        .displayName = "Stick",
        .maxStackSize = 64,
        .maxDurability = 0,
        .placeableBlockState = AIR_BLOCK_STATE,
        .iconTileIndex = 5,
        .fuelBurnTicks = 100 // 1/2 ítem fundido por palo
    });

    registerItem(ItemDefinition{
        .id = 0,
        .namespacedId = "opencraft:coal",
        .displayName = "Coal",
        .maxStackSize = 64,
        .maxDurability = 0,
        .placeableBlockState = AIR_BLOCK_STATE,
        .iconTileIndex = 17,
        .fuelBurnTicks = 1600 // Quema 8 ítems completos
    });

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:raw_iron",
                                .displayName = "Raw Iron",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 18});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:iron_ingot",
                                .displayName = "Iron Ingot",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 4});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:diamond",
                                .displayName = "Diamond",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 3});

    // Cubetas y fluidos portátiles
    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:bucket",
                                .displayName = "Bucket",
                                .maxStackSize = 16,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 24});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:water_bucket",
                                .displayName = "Water Bucket",
                                .maxStackSize = 1,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 25});

    registerItem(ItemDefinition{
        .id = 0,
        .namespacedId = "opencraft:lava_bucket",
        .displayName = "Lava Bucket",
        .maxStackSize = 1,
        .maxDurability = 0,
        .placeableBlockState = AIR_BLOCK_STATE,
        .iconTileIndex = 26,
        .fuelBurnTicks = 20000 // Quema 100 ítems completos en horno
    });

    // Alimentos
    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:raw_beef",
                                .displayName = "Raw Beef",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 19,
                                .isFood = true,
                                .foodRestore = 3,
                                .saturationRestore = 1.8f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:cooked_beef",
                                .displayName = "Steak",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 19,
                                .isFood = true,
                                .foodRestore = 8,
                                .saturationRestore = 12.8f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:apple",
                                .displayName = "Apple",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 18,
                                .isFood = true,
                                .foodRestore = 4,
                                .saturationRestore = 2.4f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:bread",
                                .displayName = "Bread",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 18,
                                .isFood = true,
                                .foodRestore = 5,
                                .saturationRestore = 6.0f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:bed",
                                .displayName = "Bed",
                                .maxStackSize = 1,
                                .maxDurability = 0,
                                .placeableBlockState = blockReg.getStateId("opencraft:bed"),
                                .iconTileIndex = 17});

    // Herramientas de Madera (durabilidad: 59)
    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:wooden_pickaxe",
                                .displayName = "Wooden Pickaxe",
                                .maxStackSize = 1,
                                .maxDurability = 59,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 7,
                                .toolType = ToolType::Pickaxe,
                                .toolTier = ToolTier::Wood,
                                .miningEfficiency = 2.0f,
                                .fuelBurnTicks = 200});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:wooden_axe",
                                .displayName = "Wooden Axe",
                                .maxStackSize = 1,
                                .maxDurability = 59,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 7,
                                .toolType = ToolType::Axe,
                                .toolTier = ToolTier::Wood,
                                .miningEfficiency = 2.0f,
                                .fuelBurnTicks = 200});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:wooden_shovel",
                                .displayName = "Wooden Shovel",
                                .maxStackSize = 1,
                                .maxDurability = 59,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 7,
                                .toolType = ToolType::Shovel,
                                .toolTier = ToolTier::Wood,
                                .miningEfficiency = 2.0f,
                                .fuelBurnTicks = 200});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:wooden_sword",
                                .displayName = "Wooden Sword",
                                .maxStackSize = 1,
                                .maxDurability = 59,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 7,
                                .toolType = ToolType::Sword,
                                .toolTier = ToolTier::Wood,
                                .miningEfficiency = 1.0f,
                                .fuelBurnTicks = 200,
                                .attackDamage = 4.0f});

    // Herramientas de Piedra (durabilidad: 131)
    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:stone_pickaxe",
                                .displayName = "Stone Pickaxe",
                                .maxStackSize = 1,
                                .maxDurability = 131,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 3,
                                .toolType = ToolType::Pickaxe,
                                .toolTier = ToolTier::Stone,
                                .miningEfficiency = 4.0f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:stone_axe",
                                .displayName = "Stone Axe",
                                .maxStackSize = 1,
                                .maxDurability = 131,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 3,
                                .toolType = ToolType::Axe,
                                .toolTier = ToolTier::Stone,
                                .miningEfficiency = 4.0f,
                                .attackDamage = 3.0f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:stone_shovel",
                                .displayName = "Stone Shovel",
                                .maxStackSize = 1,
                                .maxDurability = 131,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 3,
                                .toolType = ToolType::Shovel,
                                .toolTier = ToolTier::Stone,
                                .miningEfficiency = 4.0f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:stone_sword",
                                .displayName = "Stone Sword",
                                .maxStackSize = 1,
                                .maxDurability = 131,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 3,
                                .toolType = ToolType::Sword,
                                .toolTier = ToolTier::Stone,
                                .miningEfficiency = 1.0f,
                                .attackDamage = 5.0f});

    // Herramientas de Hierro (durabilidad: 250)
    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:iron_pickaxe",
                                .displayName = "Iron Pickaxe",
                                .maxStackSize = 1,
                                .maxDurability = 250,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 4,
                                .toolType = ToolType::Pickaxe,
                                .toolTier = ToolTier::Iron,
                                .miningEfficiency = 6.0f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:iron_axe",
                                .displayName = "Iron Axe",
                                .maxStackSize = 1,
                                .maxDurability = 250,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 4,
                                .toolType = ToolType::Axe,
                                .toolTier = ToolTier::Iron,
                                .miningEfficiency = 6.0f,
                                .attackDamage = 4.0f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:iron_shovel",
                                .displayName = "Iron Shovel",
                                .maxStackSize = 1,
                                .maxDurability = 250,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 4,
                                .toolType = ToolType::Shovel,
                                .toolTier = ToolTier::Iron,
                                .miningEfficiency = 6.0f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:iron_sword",
                                .displayName = "Iron Sword",
                                .maxStackSize = 1,
                                .maxDurability = 250,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 4,
                                .toolType = ToolType::Sword,
                                .toolTier = ToolTier::Iron,
                                .miningEfficiency = 1.0f,
                                .attackDamage = 6.0f});

    // Diamante
    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:diamond_pickaxe",
                                .displayName = "Diamond Pickaxe",
                                .maxStackSize = 1,
                                .maxDurability = 1561,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 3,
                                .toolType = ToolType::Pickaxe,
                                .toolTier = ToolTier::Diamond,
                                .miningEfficiency = 8.0f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:diamond_sword",
                                .displayName = "Diamond Sword",
                                .maxStackSize = 1,
                                .maxDurability = 1561,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 3,
                                .toolType = ToolType::Sword,
                                .toolTier = ToolTier::Diamond,
                                .miningEfficiency = 1.0f,
                                .attackDamage = 7.0f});

    // ==========================================
    // Fase 08: Botín de Criaturas y Combate
    // ==========================================
    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:rotten_flesh",
                                .displayName = "Rotten Flesh",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 19,
                                .isFood = true,
                                .foodRestore = 4,
                                .saturationRestore = 0.8f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:bone",
                                .displayName = "Bone",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 8});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:arrow",
                                .displayName = "Arrow",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 8});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:gunpowder",
                                .displayName = "Gunpowder",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 8});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:leather",
                                .displayName = "Leather",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 8});

    // Armas y Equipo de Combate
    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:bow",
                                .displayName = "Bow",
                                .maxStackSize = 1,
                                .maxDurability = 384,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 8});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:shield",
                                .displayName = "Shield",
                                .maxStackSize = 1,
                                .maxDurability = 336,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 8,
                                .armorSlot = ArmorSlot::Shield});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:iron_helmet",
                                .displayName = "Iron Helmet",
                                .maxStackSize = 1,
                                .maxDurability = 165,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 4,
                                .armorPoints = 2.0f,
                                .armorSlot = ArmorSlot::Helmet});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:iron_chestplate",
                                .displayName = "Iron Chestplate",
                                .maxStackSize = 1,
                                .maxDurability = 240,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 4,
                                .armorPoints = 6.0f,
                                .armorSlot = ArmorSlot::Chestplate});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:iron_leggings",
                                .displayName = "Iron Leggings",
                                .maxStackSize = 1,
                                .maxDurability = 225,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 4,
                                .armorPoints = 5.0f,
                                .armorSlot = ArmorSlot::Leggings});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:iron_boots",
                                .displayName = "Iron Boots",
                                .maxStackSize = 1,
                                .maxDurability = 195,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 4,
                                .armorPoints = 2.0f,
                                .armorSlot = ArmorSlot::Boots});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:diamond_helmet",
                                .displayName = "Diamond Helmet",
                                .maxStackSize = 1,
                                .maxDurability = 363,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 3,
                                .armorPoints = 3.0f,
                                .armorSlot = ArmorSlot::Helmet});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:diamond_chestplate",
                                .displayName = "Diamond Chestplate",
                                .maxStackSize = 1,
                                .maxDurability = 528,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 3,
                                .armorPoints = 8.0f,
                                .armorSlot = ArmorSlot::Chestplate});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:diamond_leggings",
                                .displayName = "Diamond Leggings",
                                .maxStackSize = 1,
                                .maxDurability = 495,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 3,
                                .armorPoints = 6.0f,
                                .armorSlot = ArmorSlot::Leggings});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:diamond_boots",
                                .displayName = "Diamond Boots",
                                .maxStackSize = 1,
                                .maxDurability = 429,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 3,
                                .armorPoints = 3.0f,
                                .armorSlot = ArmorSlot::Boots});

    // ==========================================
    // Fase 10: Agricultura, Pesca, Comercio y Navegación
    // ==========================================
    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:wheat_seeds",
                                .displayName = "Wheat Seeds",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 18});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:wheat",
                                .displayName = "Wheat",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 18});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:carrot",
                                .displayName = "Carrot",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 18,
                                .isFood = true,
                                .foodRestore = 3,
                                .saturationRestore = 3.6f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:potato",
                                .displayName = "Potato",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 18,
                                .isFood = true,
                                .foodRestore = 1,
                                .saturationRestore = 0.6f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:baked_potato",
                                .displayName = "Baked Potato",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 18,
                                .isFood = true,
                                .foodRestore = 5,
                                .saturationRestore = 6.0f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:emerald",
                                .displayName = "Emerald",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 3});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:gold_ingot",
                                .displayName = "Gold Ingot",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 4});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:fishing_rod",
                                .displayName = "Fishing Rod",
                                .maxStackSize = 1,
                                .maxDurability = 64,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 8});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:cod",
                                .displayName = "Raw Cod",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 19,
                                .isFood = true,
                                .foodRestore = 2,
                                .saturationRestore = 0.4f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:cooked_cod",
                                .displayName = "Cooked Cod",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 19,
                                .isFood = true,
                                .foodRestore = 5,
                                .saturationRestore = 6.0f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:salmon",
                                .displayName = "Raw Salmon",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 19,
                                .isFood = true,
                                .foodRestore = 2,
                                .saturationRestore = 0.4f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:saddle",
                                .displayName = "Saddle",
                                .maxStackSize = 1,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 8});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:boat",
                                .displayName = "Boat",
                                .maxStackSize = 1,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 7});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:compass",
                                .displayName = "Compass",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 8});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:clock",
                                .displayName = "Clock",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 8});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:iron_hoe",
                                .displayName = "Iron Hoe",
                                .maxStackSize = 1,
                                .maxDurability = 250,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 7,
                                .toolType = ToolType::Hoe,
                                .toolTier = ToolTier::Iron,
                                .miningEfficiency = 6.0f});

    // ==========================================
    // Fase 11: Cinabrio, Máquinas y Transporte
    // ==========================================
    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:cinnabar",
                                .displayName = "Cinnabar Dust",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:cinnabar_wire"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:cinnabar_block",
                                .displayName = "Block of Cinnabar",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:cinnabar_block"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:cinnabar_torch",
                                .displayName = "Cinnabar Torch",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:cinnabar_torch"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:repeater",
                                .displayName = "Cinnabar Repeater",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:repeater"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:comparator",
                                .displayName = "Cinnabar Comparator",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:comparator"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:lever",
                                .displayName = "Lever",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:lever"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:stone_button",
                                .displayName = "Stone Button",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:stone_button"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:oak_button",
                                .displayName = "Oak Button",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:oak_button"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:pressure_plate",
                                .displayName = "Pressure Plate",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:pressure_plate"),
                                .iconTileIndex = 9});

    registerItem(
        ItemDefinition{.id = 0,
                       .namespacedId = "opencraft:daylight_detector",
                       .displayName = "Daylight Detector",
                       .maxStackSize = 64,
                       .maxDurability = 0,
                       .placeableBlockState = BlockRegistry::global().getStateId("opencraft:daylight_detector"),
                       .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:cinnabar_lamp",
                                .displayName = "Cinnabar Lamp",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:cinnabar_lamp"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:iron_door",
                                .displayName = "Iron Door",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:iron_door"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:oak_door",
                                .displayName = "Oak Door",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:oak_door"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:piston",
                                .displayName = "Piston",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:piston"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:sticky_piston",
                                .displayName = "Sticky Piston",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:sticky_piston"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:hopper",
                                .displayName = "Hopper",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:hopper"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:dropper",
                                .displayName = "Dropper",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:dropper"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:dispenser",
                                .displayName = "Dispenser",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:dispenser"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:crafter",
                                .displayName = "Crafter",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:crafter"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:rail",
                                .displayName = "Rail",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:rail"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:powered_rail",
                                .displayName = "Powered Rail",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:powered_rail"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:detector_rail",
                                .displayName = "Detector Rail",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:detector_rail"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:activator_rail",
                                .displayName = "Activator Rail",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:activator_rail"),
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:minecart",
                                .displayName = "Minecart",
                                .maxStackSize = 1,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:chest_minecart",
                                .displayName = "Minecart with Chest",
                                .maxStackSize = 1,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 9});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:hopper_minecart",
                                .displayName = "Minecart with Hopper",
                                .maxStackSize = 1,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 9});

    // --- Fase 12: Encantamientos, Alquimia, Dimensiones y Jefes ---
    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:enchanted_book",
                                .displayName = "Enchanted Book",
                                .maxStackSize = 1,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 25});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:orichalcum_upgrade_smithing_template",
                                .displayName = "Orichalcum Upgrade Smithing Template",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 26});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:orichalcum_ingot",
                                .displayName = "Orichalcum Ingot",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 27});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:orichalcum_scrap",
                                .displayName = "Orichalcum Scrap",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 28});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:orichalcum_sword",
                                .displayName = "Orichalcum Sword",
                                .maxStackSize = 1,
                                .maxDurability = 2031,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 29});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:orichalcum_pickaxe",
                                .displayName = "Orichalcum Pickaxe",
                                .maxStackSize = 1,
                                .maxDurability = 2031,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 30});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:orichalcum_chestplate",
                                .displayName = "Orichalcum Chestplate",
                                .maxStackSize = 1,
                                .maxDurability = 592,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 31});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:ember_rod",
                                .displayName = "Ember Rod",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 32});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:ember_powder",
                                .displayName = "Ember Powder",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 33});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:crimson_fungus",
                                .displayName = "Crimson Fungus",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 34});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:potion",
                                .displayName = "Potion",
                                .maxStackSize = 1,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 35});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:glass_bottle",
                                .displayName = "Glass Bottle",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 36});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:rim_pearl",
                                .displayName = "Rim Pearl",
                                .maxStackSize = 16,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 37});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:rim_eye",
                                .displayName = "Rim Eye",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 38});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:elytra",
                                .displayName = "Elytra",
                                .maxStackSize = 1,
                                .maxDurability = 432,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 39});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:abyss_star",
                                .displayName = "Abyss Star",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 40});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:dragon_egg",
                                .displayName = "Dragon Egg",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:dragon_egg"),
                                .iconTileIndex = 41});

    // --- Fase 15: Contenido 1.21 (cámaras de desafío) y Variedades de Catálogo ---
    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:copper_block",
                                .displayName = "Copper Block",
                                .maxStackSize = 64,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:copper_block"),
                                .iconTileIndex = 42});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:cut_copper",
                                .displayName = "Cut Copper",
                                .maxStackSize = 64,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:cut_copper"),
                                .iconTileIndex = 43});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:chiseled_copper",
                                .displayName = "Chiseled Copper",
                                .maxStackSize = 64,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:chiseled_copper"),
                                .iconTileIndex = 44});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:copper_grate",
                                .displayName = "Copper Grate",
                                .maxStackSize = 64,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:copper_grate"),
                                .iconTileIndex = 45});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:copper_bulb",
                                .displayName = "Copper Bulb",
                                .maxStackSize = 64,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:copper_bulb"),
                                .iconTileIndex = 46});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:tuff",
                                .displayName = "Tuff",
                                .maxStackSize = 64,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:tuff"),
                                .iconTileIndex = 47});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:tuff_bricks",
                                .displayName = "Tuff Bricks",
                                .maxStackSize = 64,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:tuff_bricks"),
                                .iconTileIndex = 48});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:chiseled_tuff",
                                .displayName = "Chiseled Tuff",
                                .maxStackSize = 64,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:chiseled_tuff"),
                                .iconTileIndex = 49});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:trial_spawner",
                                .displayName = "Trial Spawner",
                                .maxStackSize = 64,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:trial_spawner"),
                                .iconTileIndex = 50});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:vault",
                                .displayName = "Vault",
                                .maxStackSize = 64,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:vault"),
                                .iconTileIndex = 51});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:oak_sign",
                                .displayName = "Oak Sign",
                                .maxStackSize = 16,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:oak_sign"),
                                .iconTileIndex = 52});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:decorated_pot",
                                .displayName = "Decorated Pot",
                                .maxStackSize = 64,
                                .placeableBlockState = BlockRegistry::global().getStateId("opencraft:decorated_pot"),
                                .iconTileIndex = 53});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:mace",
                                .displayName = "Mace",
                                .maxStackSize = 1,
                                .maxDurability = 500,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 54,
                                .attackDamage = 6.0f});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:whirl_rod",
                                .displayName = "Whirl Rod",
                                .maxStackSize = 64,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 55});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:wind_charge",
                                .displayName = "Wind Charge",
                                .maxStackSize = 64,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 56});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:trial_key",
                                .displayName = "Trial Key",
                                .maxStackSize = 64,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 57});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:ominous_trial_key",
                                .displayName = "Ominous Trial Key",
                                .maxStackSize = 64,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 58});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:heavy_core",
                                .displayName = "Heavy Core",
                                .maxStackSize = 64,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 59});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:red_dye",
                                .displayName = "Red Dye",
                                .maxStackSize = 64,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 60});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:blue_dye",
                                .displayName = "Blue Dye",
                                .maxStackSize = 64,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 61});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:writable_book",
                                .displayName = "Book and Quill",
                                .maxStackSize = 1,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 62});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:map",
                                .displayName = "Empty Map",
                                .maxStackSize = 64,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 63});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:brush",
                                .displayName = "Brush",
                                .maxStackSize = 1,
                                .maxDurability = 64,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 64});

    // Encendido de portales del Abismo (Fase 12)
    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:flint",
                                .displayName = "Flint",
                                .maxStackSize = 64,
                                .maxDurability = 0,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 17});

    registerItem(ItemDefinition{.id = 0,
                                .namespacedId = "opencraft:flint_and_steel",
                                .displayName = "Flint and Steel",
                                .maxStackSize = 1,
                                .maxDurability = 64,
                                .placeableBlockState = AIR_BLOCK_STATE,
                                .iconTileIndex = 4});
}

} // namespace OpenCraft

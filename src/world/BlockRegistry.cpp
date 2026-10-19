#include "world/BlockRegistry.hpp"
#include "world/IdAliases.hpp"

namespace OpenCraft {

BlockRegistry& BlockRegistry::global() {
    static BlockRegistry s_instance;
    return s_instance;
}

BlockRegistry::BlockRegistry() { registerDefaults(); }

void BlockRegistry::reset() {
    m_properties.clear();
    m_idToState.clear();
    m_legacyToState.fill(0);
    registerDefaults();
}

BlockStateId BlockRegistry::registerBlock(const std::string& namespacedId, BlockProperties props) {
    auto it = m_idToState.find(namespacedId);
    if (it != m_idToState.end()) {
        // Ya existe, actualizar propiedades
        BlockStateId existingId = it->second;
        props.namespacedId = namespacedId;
        m_properties[existingId] = props;
        return existingId;
    }

    BlockStateId newId = static_cast<BlockStateId>(m_properties.size());
    props.namespacedId = namespacedId;
    m_properties.push_back(props);
    m_idToState[namespacedId] = newId;

    size_t legIdx = static_cast<size_t>(props.legacyId);
    if (legIdx < m_legacyToState.size()) {
        m_legacyToState[legIdx] = newId;
    }

    return newId;
}

const BlockProperties& BlockRegistry::get(BlockStateId stateId) const noexcept {
    if (stateId < m_properties.size()) {
        return m_properties[stateId];
    }
    return m_properties[0]; // fallback a Air
}

const std::string& BlockRegistry::getNamespacedId(BlockStateId stateId) const noexcept {
    if (stateId < m_properties.size()) {
        return m_properties[stateId].namespacedId;
    }
    static const std::string s_unknown = "opencraft:unknown";
    return s_unknown;
}

BlockStateId BlockRegistry::getStateId(std::string_view namespacedId) const noexcept {
    auto it = m_idToState.find(IdAliases::canonical(namespacedId));
    if (it != m_idToState.end()) {
        return it->second;
    }
    return AIR_BLOCK_STATE;
}

bool BlockRegistry::hasBlock(std::string_view namespacedId) const noexcept {
    return m_idToState.contains(IdAliases::canonical(namespacedId));
}

BlockStateId BlockRegistry::fromLegacyId(BlockID id) const noexcept {
    size_t idx = static_cast<size_t>(id);
    if (idx < m_legacyToState.size()) {
        return m_legacyToState[idx];
    }
    return AIR_BLOCK_STATE;
}

BlockID BlockRegistry::toLegacyId(BlockStateId stateId) const noexcept {
    if (stateId < m_properties.size()) {
        return m_properties[stateId].legacyId;
    }
    return BlockID::Air;
}

void BlockRegistry::registerDefaults() {
    registerBlock("opencraft:air", BlockProperties{.namespacedId = "opencraft:air",
                                                   .displayName = "Air",
                                                   .isSolid = false,
                                                   .isOpaque = false,
                                                   .isTransparent = true,
                                                   .textureIndices = {0, 0, 0, 0, 0, 0},
                                                   .legacyId = BlockID::Air,
                                                   .hardness = 0.0f,
                                                   .requiredTool = ToolType::None,
                                                   .minHarvestTier = ToolTier::None,
                                                   .dropItemName = "",
                                                   .lightEmission = 0,
                                                   .lightFilter = 1});

    registerBlock("opencraft:grass_block", BlockProperties{.namespacedId = "opencraft:grass_block",
                                                           .displayName = "Grass",
                                                           .isSolid = true,
                                                           .isOpaque = true,
                                                           .isTransparent = false,
                                                           .textureIndices = {0, 2, 1, 1, 1, 1},
                                                           .legacyId = BlockID::Grass,
                                                           .hardness = 0.6f,
                                                           .requiredTool = ToolType::Shovel,
                                                           .minHarvestTier = ToolTier::None,
                                                           .dropItemName = "opencraft:dirt"});

    registerBlock("opencraft:dirt", BlockProperties{.namespacedId = "opencraft:dirt",
                                                    .displayName = "Dirt",
                                                    .isSolid = true,
                                                    .isOpaque = true,
                                                    .isTransparent = false,
                                                    .textureIndices = {2, 2, 2, 2, 2, 2},
                                                    .legacyId = BlockID::Dirt,
                                                    .hardness = 0.5f,
                                                    .requiredTool = ToolType::Shovel,
                                                    .minHarvestTier = ToolTier::None,
                                                    .dropItemName = ""});

    registerBlock("opencraft:stone", BlockProperties{.namespacedId = "opencraft:stone",
                                                     .displayName = "Stone",
                                                     .isSolid = true,
                                                     .isOpaque = true,
                                                     .isTransparent = false,
                                                     .textureIndices = {3, 3, 3, 3, 3, 3},
                                                     .legacyId = BlockID::Stone,
                                                     .hardness = 1.5f,
                                                     .requiredTool = ToolType::Pickaxe,
                                                     .minHarvestTier = ToolTier::Wood,
                                                     .dropItemName = "opencraft:cobblestone"});

    registerBlock("opencraft:cobblestone", BlockProperties{.namespacedId = "opencraft:cobblestone",
                                                           .displayName = "Cobblestone",
                                                           .isSolid = true,
                                                           .isOpaque = true,
                                                           .isTransparent = false,
                                                           .textureIndices = {4, 4, 4, 4, 4, 4},
                                                           .legacyId = BlockID::Cobblestone,
                                                           .hardness = 2.0f,
                                                           .requiredTool = ToolType::Pickaxe,
                                                           .minHarvestTier = ToolTier::Wood,
                                                           .dropItemName = ""});

    registerBlock("opencraft:wood_log", BlockProperties{.namespacedId = "opencraft:wood_log",
                                                        .displayName = "Wood",
                                                        .isSolid = true,
                                                        .isOpaque = true,
                                                        .isTransparent = false,
                                                        .textureIndices = {6, 6, 5, 5, 5, 5},
                                                        .legacyId = BlockID::Wood,
                                                        .hardness = 2.0f,
                                                        .requiredTool = ToolType::Axe,
                                                        .minHarvestTier = ToolTier::None,
                                                        .dropItemName = ""});

    registerBlock("opencraft:planks", BlockProperties{.namespacedId = "opencraft:planks",
                                                      .displayName = "Planks",
                                                      .isSolid = true,
                                                      .isOpaque = true,
                                                      .isTransparent = false,
                                                      .textureIndices = {7, 7, 7, 7, 7, 7},
                                                      .legacyId = BlockID::Planks,
                                                      .hardness = 2.0f,
                                                      .requiredTool = ToolType::Axe,
                                                      .minHarvestTier = ToolTier::None,
                                                      .dropItemName = ""});

    registerBlock("opencraft:leaves", BlockProperties{.namespacedId = "opencraft:leaves",
                                                      .displayName = "Leaves",
                                                      .isSolid = true,
                                                      .isOpaque = false,
                                                      .isTransparent = true,
                                                      .textureIndices = {8, 8, 8, 8, 8, 8},
                                                      .legacyId = BlockID::Leaves,
                                                      .hardness = 0.2f,
                                                      .requiredTool = ToolType::None,
                                                      .minHarvestTier = ToolTier::None,
                                                      .dropItemName = "opencraft:air",
                                                      .lightEmission = 0,
                                                      .lightFilter = 2});

    registerBlock("opencraft:sand", BlockProperties{.namespacedId = "opencraft:sand",
                                                    .displayName = "Sand",
                                                    .isSolid = true,
                                                    .isOpaque = true,
                                                    .isTransparent = false,
                                                    .textureIndices = {9, 9, 9, 9, 9, 9},
                                                    .legacyId = BlockID::Sand,
                                                    .hardness = 0.5f,
                                                    .requiredTool = ToolType::Shovel,
                                                    .minHarvestTier = ToolTier::None,
                                                    .dropItemName = "",
                                                    .lightEmission = 0,
                                                    .lightFilter = 15});

    registerBlock("opencraft:glass", BlockProperties{.namespacedId = "opencraft:glass",
                                                     .displayName = "Glass",
                                                     .isSolid = true,
                                                     .isOpaque = false,
                                                     .isTransparent = true,
                                                     .textureIndices = {10, 10, 10, 10, 10, 10},
                                                     .legacyId = BlockID::Glass,
                                                     .hardness = 0.3f,
                                                     .requiredTool = ToolType::None,
                                                     .minHarvestTier = ToolTier::None,
                                                     .dropItemName = "opencraft:air",
                                                     .lightEmission = 0,
                                                     .lightFilter = 1});

    registerBlock("opencraft:bedrock", BlockProperties{.namespacedId = "opencraft:bedrock",
                                                       .displayName = "Bedrock",
                                                       .isSolid = true,
                                                       .isOpaque = true,
                                                       .isTransparent = false,
                                                       .textureIndices = {11, 11, 11, 11, 11, 11},
                                                       .legacyId = BlockID::Bedrock,
                                                       .hardness = -1.0f,
                                                       .requiredTool = ToolType::None,
                                                       .minHarvestTier = ToolTier::None,
                                                       .dropItemName = "opencraft:air",
                                                       .lightEmission = 0,
                                                       .lightFilter = 15});

    registerBlock("opencraft:water", BlockProperties{.namespacedId = "opencraft:water",
                                                     .displayName = "Water",
                                                     .isSolid = false,
                                                     .isOpaque = false,
                                                     .isTransparent = true,
                                                     .textureIndices = {12, 12, 12, 12, 12, 12},
                                                     .legacyId = BlockID::Water,
                                                     .hardness = 100.0f,
                                                     .requiredTool = ToolType::None,
                                                     .minHarvestTier = ToolTier::None,
                                                     .dropItemName = "opencraft:air",
                                                     .lightEmission = 0,
                                                     .lightFilter = 2});

    registerBlock("opencraft:chest", BlockProperties{.namespacedId = "opencraft:chest",
                                                     .displayName = "Chest",
                                                     .isSolid = true,
                                                     .isOpaque = true,
                                                     .isTransparent = false,
                                                     .textureIndices = {92, 92, 93, 92, 92, 92},
                                                     .legacyId = BlockID::Chest,
                                                     .hardness = 2.5f,
                                                     .requiredTool = ToolType::Axe,
                                                     .minHarvestTier = ToolTier::None,
                                                     .dropItemName = ""});

    registerBlock("opencraft:crafting_table", BlockProperties{.namespacedId = "opencraft:crafting_table",
                                                              .displayName = "Crafting Table",
                                                              .isSolid = true,
                                                              .isOpaque = true,
                                                              .isTransparent = false,
                                                              .textureIndices = {13, 7, 91, 91, 91, 91},
                                                              .legacyId = BlockID::CraftingTable,
                                                              .hardness = 2.5f,
                                                              .requiredTool = ToolType::Axe,
                                                              .minHarvestTier = ToolTier::None,
                                                              .dropItemName = ""});

    registerBlock("opencraft:furnace", BlockProperties{.namespacedId = "opencraft:furnace",
                                                       .displayName = "Furnace",
                                                       .isSolid = true,
                                                       .isOpaque = true,
                                                       .isTransparent = false,
                                                       .textureIndices = {4, 4, 14, 95, 95, 95},
                                                       .legacyId = BlockID::Furnace,
                                                       .hardness = 3.5f,
                                                       .requiredTool = ToolType::Pickaxe,
                                                       .minHarvestTier = ToolTier::Wood,
                                                       .dropItemName = ""});

    registerBlock("opencraft:coal_ore", BlockProperties{.namespacedId = "opencraft:coal_ore",
                                                        .displayName = "Coal Ore",
                                                        .isSolid = true,
                                                        .isOpaque = true,
                                                        .isTransparent = false,
                                                        .textureIndices = {15, 15, 15, 15, 15, 15},
                                                        .legacyId = BlockID::CoalOre,
                                                        .hardness = 3.0f,
                                                        .requiredTool = ToolType::Pickaxe,
                                                        .minHarvestTier = ToolTier::Wood,
                                                        .dropItemName = "opencraft:coal"});

    registerBlock("opencraft:iron_ore", BlockProperties{.namespacedId = "opencraft:iron_ore",
                                                        .displayName = "Iron Ore",
                                                        .isSolid = true,
                                                        .isOpaque = true,
                                                        .isTransparent = false,
                                                        .textureIndices = {16, 16, 16, 16, 16, 16},
                                                        .legacyId = BlockID::IronOre,
                                                        .hardness = 3.0f,
                                                        .requiredTool = ToolType::Pickaxe,
                                                        .minHarvestTier = ToolTier::Stone,
                                                        .dropItemName = "opencraft:raw_iron"});

    registerBlock("opencraft:bed", BlockProperties{.namespacedId = "opencraft:bed",
                                                   .displayName = "Bed",
                                                   .isSolid = true,
                                                   .isOpaque = false,
                                                   .isTransparent = true,
                                                   .textureIndices = {17, 7, 94, 94, 94, 94},
                                                   .legacyId = BlockID::Bed,
                                                   .hardness = 0.2f,
                                                   .requiredTool = ToolType::None,
                                                   .minHarvestTier = ToolTier::None,
                                                   .dropItemName = "opencraft:bed",
                                                   .lightEmission = 0,
                                                   .lightFilter = 1});

    registerBlock("opencraft:flowing_water", BlockProperties{.namespacedId = "opencraft:flowing_water",
                                                             .displayName = "Flowing Water",
                                                             .isSolid = false,
                                                             .isOpaque = false,
                                                             .isTransparent = true,
                                                             .textureIndices = {12, 12, 12, 12, 12, 12},
                                                             .legacyId = BlockID::FlowingWater,
                                                             .hardness = 100.0f,
                                                             .requiredTool = ToolType::None,
                                                             .minHarvestTier = ToolTier::None,
                                                             .dropItemName = "opencraft:air",
                                                             .lightEmission = 0,
                                                             .lightFilter = 2});

    registerBlock("opencraft:lava", BlockProperties{.namespacedId = "opencraft:lava",
                                                    .displayName = "Lava",
                                                    .isSolid = false,
                                                    .isOpaque = false,
                                                    .isTransparent = true,
                                                    .textureIndices = {20, 20, 20, 20, 20, 20},
                                                    .legacyId = BlockID::Lava,
                                                    .hardness = 100.0f,
                                                    .requiredTool = ToolType::None,
                                                    .minHarvestTier = ToolTier::None,
                                                    .dropItemName = "opencraft:air",
                                                    .lightEmission = 15,
                                                    .lightFilter = 2});

    registerBlock("opencraft:flowing_lava", BlockProperties{.namespacedId = "opencraft:flowing_lava",
                                                            .displayName = "Flowing Lava",
                                                            .isSolid = false,
                                                            .isOpaque = false,
                                                            .isTransparent = true,
                                                            .textureIndices = {20, 20, 20, 20, 20, 20},
                                                            .legacyId = BlockID::FlowingLava,
                                                            .hardness = 100.0f,
                                                            .requiredTool = ToolType::None,
                                                            .minHarvestTier = ToolTier::None,
                                                            .dropItemName = "opencraft:air",
                                                            .lightEmission = 15,
                                                            .lightFilter = 2});

    registerBlock("opencraft:obsidian", BlockProperties{.namespacedId = "opencraft:obsidian",
                                                        .displayName = "Obsidian",
                                                        .isSolid = true,
                                                        .isOpaque = true,
                                                        .isTransparent = false,
                                                        .textureIndices = {21, 21, 21, 21, 21, 21},
                                                        .legacyId = BlockID::Obsidian,
                                                        .hardness = 50.0f,
                                                        .requiredTool = ToolType::Pickaxe,
                                                        .minHarvestTier = ToolTier::Diamond,
                                                        .dropItemName = "",
                                                        .lightEmission = 0,
                                                        .lightFilter = 15});

    registerBlock("opencraft:torch", BlockProperties{.namespacedId = "opencraft:torch",
                                                     .displayName = "Torch",
                                                     .isSolid = false,
                                                     .isOpaque = false,
                                                     .isTransparent = true,
                                                     .textureIndices = {22, 22, 22, 22, 22, 22},
                                                     .legacyId = BlockID::Torch,
                                                     .hardness = 0.0f,
                                                     .requiredTool = ToolType::None,
                                                     .minHarvestTier = ToolTier::None,
                                                     .dropItemName = "opencraft:torch",
                                                     .lightEmission = 14,
                                                     .lightFilter = 1});

    registerBlock("opencraft:fire", BlockProperties{.namespacedId = "opencraft:fire",
                                                    .displayName = "Fire",
                                                    .isSolid = false,
                                                    .isOpaque = false,
                                                    .isTransparent = true,
                                                    .textureIndices = {23, 23, 23, 23, 23, 23},
                                                    .legacyId = BlockID::Fire,
                                                    .hardness = 0.0f,
                                                    .requiredTool = ToolType::None,
                                                    .minHarvestTier = ToolTier::None,
                                                    .dropItemName = "opencraft:air",
                                                    .lightEmission = 15,
                                                    .lightFilter = 1});

    registerBlock("opencraft:farmland", BlockProperties{.namespacedId = "opencraft:farmland",
                                                        .displayName = "Farmland",
                                                        .isSolid = true,
                                                        .isOpaque = true,
                                                        .isTransparent = false,
                                                        .textureIndices = {88, 2, 2, 2, 2, 2},
                                                        .legacyId = BlockID::Farmland,
                                                        .hardness = 0.6f,
                                                        .requiredTool = ToolType::Shovel,
                                                        .minHarvestTier = ToolTier::None,
                                                        .dropItemName = "opencraft:dirt",
                                                        .lightEmission = 0,
                                                        .lightFilter = 15});

    registerBlock("opencraft:wheat_crops", BlockProperties{.namespacedId = "opencraft:wheat_crops",
                                                           .displayName = "Wheat Crops",
                                                           .isSolid = false,
                                                           .isOpaque = false,
                                                           .isTransparent = true,
                                                           .textureIndices = {19, 19, 19, 19, 19, 19},
                                                           .legacyId = BlockID::WheatCrops,
                                                           .hardness = 0.0f,
                                                           .requiredTool = ToolType::None,
                                                           .minHarvestTier = ToolTier::None,
                                                           .dropItemName = "opencraft:wheat_seeds",
                                                           .lightEmission = 0,
                                                           .lightFilter = 1});

    registerBlock("opencraft:carrots", BlockProperties{.namespacedId = "opencraft:carrots",
                                                       .displayName = "Carrots",
                                                       .isSolid = false,
                                                       .isOpaque = false,
                                                       .isTransparent = true,
                                                       .textureIndices = {89, 89, 89, 89, 89, 89},
                                                       .legacyId = BlockID::Carrots,
                                                       .hardness = 0.0f,
                                                       .requiredTool = ToolType::None,
                                                       .minHarvestTier = ToolTier::None,
                                                       .dropItemName = "opencraft:carrot",
                                                       .lightEmission = 0,
                                                       .lightFilter = 1});

    registerBlock("opencraft:potatoes", BlockProperties{.namespacedId = "opencraft:potatoes",
                                                        .displayName = "Potatoes",
                                                        .isSolid = false,
                                                        .isOpaque = false,
                                                        .isTransparent = true,
                                                        .textureIndices = {90, 90, 90, 90, 90, 90},
                                                        .legacyId = BlockID::Potatoes,
                                                        .hardness = 0.0f,
                                                        .requiredTool = ToolType::None,
                                                        .minHarvestTier = ToolTier::None,
                                                        .dropItemName = "opencraft:potato",
                                                        .lightEmission = 0,
                                                        .lightFilter = 1});

    registerBlock("opencraft:snow", BlockProperties{.namespacedId = "opencraft:snow",
                                                    .displayName = "Snow",
                                                    .isSolid = false,
                                                    .isOpaque = false,
                                                    .isTransparent = true,
                                                    .textureIndices = {18, 18, 18, 18, 18, 18},
                                                    .legacyId = BlockID::Snow,
                                                    .hardness = 0.1f,
                                                    .requiredTool = ToolType::Shovel,
                                                    .minHarvestTier = ToolTier::None,
                                                    .dropItemName = "opencraft:air",
                                                    .lightEmission = 0,
                                                    .lightFilter = 1});

    registerBlock("opencraft:snow_block", BlockProperties{.namespacedId = "opencraft:snow_block",
                                                          .displayName = "Snow Block",
                                                          .isSolid = true,
                                                          .isOpaque = true,
                                                          .isTransparent = false,
                                                          .textureIndices = {18, 18, 18, 18, 18, 18},
                                                          .legacyId = BlockID::SnowBlock,
                                                          .hardness = 0.2f,
                                                          .requiredTool = ToolType::Shovel,
                                                          .minHarvestTier = ToolTier::None,
                                                          .dropItemName = "",
                                                          .lightEmission = 0,
                                                          .lightFilter = 15});

    registerBlock("opencraft:ice", BlockProperties{.namespacedId = "opencraft:ice",
                                                   .displayName = "Ice",
                                                   .isSolid = true,
                                                   .isOpaque = false,
                                                   .isTransparent = true,
                                                   .textureIndices = {87, 87, 87, 87, 87, 87},
                                                   .legacyId = BlockID::Ice,
                                                   .hardness = 0.5f,
                                                   .requiredTool = ToolType::Pickaxe,
                                                   .minHarvestTier = ToolTier::None,
                                                   .dropItemName = "opencraft:water",
                                                   .lightEmission = 0,
                                                   .lightFilter = 2});

    registerBlock("opencraft:sandstone", BlockProperties{.namespacedId = "opencraft:sandstone",
                                                         .displayName = "Sandstone",
                                                         .isSolid = true,
                                                         .isOpaque = true,
                                                         .isTransparent = false,
                                                         .textureIndices = {24, 24, 24, 24, 24, 24},
                                                         .legacyId = BlockID::Sandstone,
                                                         .hardness = 0.8f,
                                                         .requiredTool = ToolType::Pickaxe,
                                                         .minHarvestTier = ToolTier::Wood,
                                                         .dropItemName = "",
                                                         .lightEmission = 0,
                                                         .lightFilter = 15});

    registerBlock("opencraft:gravel", BlockProperties{.namespacedId = "opencraft:gravel",
                                                      .displayName = "Gravel",
                                                      .isSolid = true,
                                                      .isOpaque = true,
                                                      .isTransparent = false,
                                                      .textureIndices = {25, 25, 25, 25, 25, 25},
                                                      .legacyId = BlockID::Gravel,
                                                      .hardness = 0.6f,
                                                      .requiredTool = ToolType::Shovel,
                                                      .minHarvestTier = ToolTier::None,
                                                      .dropItemName = "",
                                                      .lightEmission = 0,
                                                      .lightFilter = 15});

    registerBlock("opencraft:gold_ore", BlockProperties{.namespacedId = "opencraft:gold_ore",
                                                        .displayName = "Gold Ore",
                                                        .isSolid = true,
                                                        .isOpaque = true,
                                                        .isTransparent = false,
                                                        .textureIndices = {26, 26, 26, 26, 26, 26},
                                                        .legacyId = BlockID::GoldOre,
                                                        .hardness = 3.0f,
                                                        .requiredTool = ToolType::Pickaxe,
                                                        .minHarvestTier = ToolTier::Iron,
                                                        .dropItemName = "opencraft:gold_ingot",
                                                        .lightEmission = 0,
                                                        .lightFilter = 15});

    registerBlock("opencraft:diamond_ore", BlockProperties{.namespacedId = "opencraft:diamond_ore",
                                                           .displayName = "Diamond Ore",
                                                           .isSolid = true,
                                                           .isOpaque = true,
                                                           .isTransparent = false,
                                                           .textureIndices = {27, 27, 27, 27, 27, 27},
                                                           .legacyId = BlockID::DiamondOre,
                                                           .hardness = 3.0f,
                                                           .requiredTool = ToolType::Pickaxe,
                                                           .minHarvestTier = ToolTier::Iron,
                                                           .dropItemName = "opencraft:diamond",
                                                           .lightEmission = 0,
                                                           .lightFilter = 15});

    registerBlock("opencraft:emerald_ore", BlockProperties{.namespacedId = "opencraft:emerald_ore",
                                                           .displayName = "Emerald Ore",
                                                           .isSolid = true,
                                                           .isOpaque = true,
                                                           .isTransparent = false,
                                                           .textureIndices = {28, 28, 28, 28, 28, 28},
                                                           .legacyId = BlockID::EmeraldOre,
                                                           .hardness = 3.0f,
                                                           .requiredTool = ToolType::Pickaxe,
                                                           .minHarvestTier = ToolTier::Iron,
                                                           .dropItemName = "opencraft:emerald",
                                                           .lightEmission = 0,
                                                           .lightFilter = 15});

    registerBlock("opencraft:mossy_cobblestone", BlockProperties{.namespacedId = "opencraft:mossy_cobblestone",
                                                                 .displayName = "Mossy Cobblestone",
                                                                 .isSolid = true,
                                                                 .isOpaque = true,
                                                                 .isTransparent = false,
                                                                 .textureIndices = {29, 29, 29, 29, 29, 29},
                                                                 .legacyId = BlockID::MossyCobblestone,
                                                                 .hardness = 2.0f,
                                                                 .requiredTool = ToolType::Pickaxe,
                                                                 .minHarvestTier = ToolTier::Wood,
                                                                 .dropItemName = "",
                                                                 .lightEmission = 0,
                                                                 .lightFilter = 15});

    registerBlock("opencraft:monster_spawner", BlockProperties{.namespacedId = "opencraft:monster_spawner",
                                                               .displayName = "Monster Spawner",
                                                               .isSolid = true,
                                                               .isOpaque = false,
                                                               .isTransparent = true,
                                                               .textureIndices = {30, 30, 30, 30, 30, 30},
                                                               .legacyId = BlockID::MonsterSpawner,
                                                               .hardness = 5.0f,
                                                               .requiredTool = ToolType::Pickaxe,
                                                               .minHarvestTier = ToolTier::Wood,
                                                               .dropItemName = "opencraft:air",
                                                               .lightEmission = 0,
                                                               .lightFilter = 1});

    registerBlock("opencraft:composter", BlockProperties{.namespacedId = "opencraft:composter",
                                                         .displayName = "Composter",
                                                         .isSolid = true,
                                                         .isOpaque = true,
                                                         .isTransparent = false,
                                                         .textureIndices = {31, 31, 31, 31, 31, 31},
                                                         .legacyId = BlockID::Composter,
                                                         .hardness = 2.0f,
                                                         .requiredTool = ToolType::Axe,
                                                         .minHarvestTier = ToolTier::None,
                                                         .dropItemName = "",
                                                         .lightEmission = 0,
                                                         .lightFilter = 15});

    registerBlock("opencraft:lectern", BlockProperties{.namespacedId = "opencraft:lectern",
                                                       .displayName = "Lectern",
                                                       .isSolid = true,
                                                       .isOpaque = true,
                                                       .isTransparent = false,
                                                       .textureIndices = {32, 32, 32, 32, 32, 32},
                                                       .legacyId = BlockID::Lectern,
                                                       .hardness = 2.0f,
                                                       .requiredTool = ToolType::Axe,
                                                       .minHarvestTier = ToolTier::None,
                                                       .dropItemName = "",
                                                       .lightEmission = 0,
                                                       .lightFilter = 15});

    registerBlock("opencraft:cinnabar_wire", BlockProperties{.namespacedId = "opencraft:cinnabar_wire",
                                                             .displayName = "Cinnabar Dust",
                                                             .isSolid = false,
                                                             .isOpaque = false,
                                                             .isTransparent = true,
                                                             .textureIndices = {33, 33, 33, 33, 33, 33},
                                                             .legacyId = BlockID::CinnabarWire,
                                                             .hardness = 0.0f,
                                                             .requiredTool = ToolType::None,
                                                             .minHarvestTier = ToolTier::None,
                                                             .dropItemName = "opencraft:cinnabar",
                                                             .lightEmission = 0,
                                                             .lightFilter = 1});

    registerBlock("opencraft:cinnabar_block", BlockProperties{.namespacedId = "opencraft:cinnabar_block",
                                                              .displayName = "Block of Cinnabar",
                                                              .isSolid = true,
                                                              .isOpaque = true,
                                                              .isTransparent = false,
                                                              .textureIndices = {34, 34, 34, 34, 34, 34},
                                                              .legacyId = BlockID::CinnabarBlock,
                                                              .hardness = 5.0f,
                                                              .requiredTool = ToolType::Pickaxe,
                                                              .minHarvestTier = ToolTier::Wood,
                                                              .dropItemName = "",
                                                              .lightEmission = 7,
                                                              .lightFilter = 15});

    registerBlock("opencraft:cinnabar_torch", BlockProperties{.namespacedId = "opencraft:cinnabar_torch",
                                                              .displayName = "Cinnabar Torch",
                                                              .isSolid = false,
                                                              .isOpaque = false,
                                                              .isTransparent = true,
                                                              .textureIndices = {35, 35, 35, 35, 35, 35},
                                                              .legacyId = BlockID::CinnabarTorch,
                                                              .hardness = 0.0f,
                                                              .requiredTool = ToolType::None,
                                                              .minHarvestTier = ToolTier::None,
                                                              .dropItemName = "opencraft:cinnabar_torch",
                                                              .lightEmission = 7,
                                                              .lightFilter = 1});

    registerBlock("opencraft:unlit_cinnabar_torch", BlockProperties{.namespacedId = "opencraft:unlit_cinnabar_torch",
                                                                    .displayName = "Unlit Cinnabar Torch",
                                                                    .isSolid = false,
                                                                    .isOpaque = false,
                                                                    .isTransparent = true,
                                                                    .textureIndices = {36, 36, 36, 36, 36, 36},
                                                                    .legacyId = BlockID::CinnabarTorchOff,
                                                                    .hardness = 0.0f,
                                                                    .requiredTool = ToolType::None,
                                                                    .minHarvestTier = ToolTier::None,
                                                                    .dropItemName = "opencraft:cinnabar_torch",
                                                                    .lightEmission = 0,
                                                                    .lightFilter = 1});

    registerBlock("opencraft:repeater", BlockProperties{.namespacedId = "opencraft:repeater",
                                                        .displayName = "Cinnabar Repeater",
                                                        .isSolid = false,
                                                        .isOpaque = false,
                                                        .isTransparent = true,
                                                        .textureIndices = {37, 37, 37, 37, 37, 37},
                                                        .legacyId = BlockID::Repeater,
                                                        .hardness = 0.0f,
                                                        .requiredTool = ToolType::None,
                                                        .minHarvestTier = ToolTier::None,
                                                        .dropItemName = "opencraft:repeater",
                                                        .lightEmission = 0,
                                                        .lightFilter = 1});

    registerBlock("opencraft:comparator", BlockProperties{.namespacedId = "opencraft:comparator",
                                                          .displayName = "Cinnabar Comparator",
                                                          .isSolid = false,
                                                          .isOpaque = false,
                                                          .isTransparent = true,
                                                          .textureIndices = {38, 38, 38, 38, 38, 38},
                                                          .legacyId = BlockID::Comparator,
                                                          .hardness = 0.0f,
                                                          .requiredTool = ToolType::None,
                                                          .minHarvestTier = ToolTier::None,
                                                          .dropItemName = "opencraft:comparator",
                                                          .lightEmission = 0,
                                                          .lightFilter = 1});

    registerBlock("opencraft:lever", BlockProperties{.namespacedId = "opencraft:lever",
                                                     .displayName = "Lever",
                                                     .isSolid = false,
                                                     .isOpaque = false,
                                                     .isTransparent = true,
                                                     .textureIndices = {39, 39, 39, 39, 39, 39},
                                                     .legacyId = BlockID::Lever,
                                                     .hardness = 0.5f,
                                                     .requiredTool = ToolType::None,
                                                     .minHarvestTier = ToolTier::None,
                                                     .dropItemName = "opencraft:lever",
                                                     .lightEmission = 0,
                                                     .lightFilter = 1});

    registerBlock("opencraft:stone_button", BlockProperties{.namespacedId = "opencraft:stone_button",
                                                            .displayName = "Stone Button",
                                                            .isSolid = false,
                                                            .isOpaque = false,
                                                            .isTransparent = true,
                                                            .textureIndices = {40, 40, 40, 40, 40, 40},
                                                            .legacyId = BlockID::StoneButton,
                                                            .hardness = 0.5f,
                                                            .requiredTool = ToolType::Pickaxe,
                                                            .minHarvestTier = ToolTier::Wood,
                                                            .dropItemName = "opencraft:stone_button",
                                                            .lightEmission = 0,
                                                            .lightFilter = 1});

    registerBlock("opencraft:oak_button", BlockProperties{.namespacedId = "opencraft:oak_button",
                                                          .displayName = "Oak Button",
                                                          .isSolid = false,
                                                          .isOpaque = false,
                                                          .isTransparent = true,
                                                          .textureIndices = {41, 41, 41, 41, 41, 41},
                                                          .legacyId = BlockID::OakButton,
                                                          .hardness = 0.5f,
                                                          .requiredTool = ToolType::Axe,
                                                          .minHarvestTier = ToolTier::None,
                                                          .dropItemName = "opencraft:oak_button",
                                                          .lightEmission = 0,
                                                          .lightFilter = 1});

    registerBlock("opencraft:pressure_plate", BlockProperties{.namespacedId = "opencraft:pressure_plate",
                                                              .displayName = "Pressure Plate",
                                                              .isSolid = false,
                                                              .isOpaque = false,
                                                              .isTransparent = true,
                                                              .textureIndices = {42, 42, 42, 42, 42, 42},
                                                              .legacyId = BlockID::PressurePlate,
                                                              .hardness = 0.5f,
                                                              .requiredTool = ToolType::Axe,
                                                              .minHarvestTier = ToolTier::None,
                                                              .dropItemName = "opencraft:pressure_plate",
                                                              .lightEmission = 0,
                                                              .lightFilter = 1});

    registerBlock("opencraft:daylight_detector", BlockProperties{.namespacedId = "opencraft:daylight_detector",
                                                                 .displayName = "Daylight Detector",
                                                                 .isSolid = true,
                                                                 .isOpaque = false,
                                                                 .isTransparent = true,
                                                                 .textureIndices = {43, 43, 43, 43, 43, 43},
                                                                 .legacyId = BlockID::DaylightDetector,
                                                                 .hardness = 0.2f,
                                                                 .requiredTool = ToolType::Axe,
                                                                 .minHarvestTier = ToolTier::None,
                                                                 .dropItemName = "opencraft:daylight_detector",
                                                                 .lightEmission = 0,
                                                                 .lightFilter = 1});

    registerBlock("opencraft:cinnabar_lamp", BlockProperties{.namespacedId = "opencraft:cinnabar_lamp",
                                                             .displayName = "Cinnabar Lamp",
                                                             .isSolid = true,
                                                             .isOpaque = true,
                                                             .isTransparent = false,
                                                             .textureIndices = {44, 44, 44, 44, 44, 44},
                                                             .legacyId = BlockID::CinnabarLamp,
                                                             .hardness = 0.3f,
                                                             .requiredTool = ToolType::None,
                                                             .minHarvestTier = ToolTier::None,
                                                             .dropItemName = "opencraft:cinnabar_lamp",
                                                             .lightEmission = 0,
                                                             .lightFilter = 15});

    registerBlock("opencraft:lit_cinnabar_lamp", BlockProperties{.namespacedId = "opencraft:lit_cinnabar_lamp",
                                                                 .displayName = "Lit Cinnabar Lamp",
                                                                 .isSolid = true,
                                                                 .isOpaque = true,
                                                                 .isTransparent = false,
                                                                 .textureIndices = {45, 45, 45, 45, 45, 45},
                                                                 .legacyId = BlockID::LitCinnabarLamp,
                                                                 .hardness = 0.3f,
                                                                 .requiredTool = ToolType::None,
                                                                 .minHarvestTier = ToolTier::None,
                                                                 .dropItemName = "opencraft:cinnabar_lamp",
                                                                 .lightEmission = 15,
                                                                 .lightFilter = 15});

    registerBlock("opencraft:iron_door", BlockProperties{.namespacedId = "opencraft:iron_door",
                                                         .displayName = "Iron Door",
                                                         .isSolid = true,
                                                         .isOpaque = false,
                                                         .isTransparent = true,
                                                         .textureIndices = {46, 46, 46, 46, 46, 46},
                                                         .legacyId = BlockID::IronDoor,
                                                         .hardness = 5.0f,
                                                         .requiredTool = ToolType::Pickaxe,
                                                         .minHarvestTier = ToolTier::Wood,
                                                         .dropItemName = "opencraft:iron_door",
                                                         .lightEmission = 0,
                                                         .lightFilter = 1});

    registerBlock("opencraft:oak_door", BlockProperties{.namespacedId = "opencraft:oak_door",
                                                        .displayName = "Oak Door",
                                                        .isSolid = true,
                                                        .isOpaque = false,
                                                        .isTransparent = true,
                                                        .textureIndices = {47, 47, 47, 47, 47, 47},
                                                        .legacyId = BlockID::OakDoor,
                                                        .hardness = 3.0f,
                                                        .requiredTool = ToolType::Axe,
                                                        .minHarvestTier = ToolTier::None,
                                                        .dropItemName = "opencraft:oak_door",
                                                        .lightEmission = 0,
                                                        .lightFilter = 1});

    registerBlock("opencraft:piston", BlockProperties{.namespacedId = "opencraft:piston",
                                                      .displayName = "Piston",
                                                      .isSolid = true,
                                                      .isOpaque = true,
                                                      .isTransparent = false,
                                                      .textureIndices = {48, 48, 48, 48, 48, 48},
                                                      .legacyId = BlockID::Piston,
                                                      .hardness = 1.5f,
                                                      .requiredTool = ToolType::Pickaxe,
                                                      .minHarvestTier = ToolTier::Wood,
                                                      .dropItemName = "opencraft:piston",
                                                      .lightEmission = 0,
                                                      .lightFilter = 15});

    registerBlock("opencraft:sticky_piston", BlockProperties{.namespacedId = "opencraft:sticky_piston",
                                                             .displayName = "Sticky Piston",
                                                             .isSolid = true,
                                                             .isOpaque = true,
                                                             .isTransparent = false,
                                                             .textureIndices = {49, 49, 49, 49, 49, 49},
                                                             .legacyId = BlockID::StickyPiston,
                                                             .hardness = 1.5f,
                                                             .requiredTool = ToolType::Pickaxe,
                                                             .minHarvestTier = ToolTier::Wood,
                                                             .dropItemName = "opencraft:sticky_piston",
                                                             .lightEmission = 0,
                                                             .lightFilter = 15});

    registerBlock("opencraft:piston_head", BlockProperties{.namespacedId = "opencraft:piston_head",
                                                           .displayName = "Piston Head",
                                                           .isSolid = true,
                                                           .isOpaque = false,
                                                           .isTransparent = true,
                                                           .textureIndices = {50, 50, 50, 50, 50, 50},
                                                           .legacyId = BlockID::PistonHead,
                                                           .hardness = 1.5f,
                                                           .requiredTool = ToolType::Pickaxe,
                                                           .minHarvestTier = ToolTier::Wood,
                                                           .dropItemName = "opencraft:air",
                                                           .lightEmission = 0,
                                                           .lightFilter = 1});

    registerBlock("opencraft:hopper", BlockProperties{.namespacedId = "opencraft:hopper",
                                                      .displayName = "Hopper",
                                                      .isSolid = true,
                                                      .isOpaque = false,
                                                      .isTransparent = true,
                                                      .textureIndices = {51, 51, 51, 51, 51, 51},
                                                      .legacyId = BlockID::Hopper,
                                                      .hardness = 3.0f,
                                                      .requiredTool = ToolType::Pickaxe,
                                                      .minHarvestTier = ToolTier::Wood,
                                                      .dropItemName = "opencraft:hopper",
                                                      .lightEmission = 0,
                                                      .lightFilter = 1});

    registerBlock("opencraft:dropper", BlockProperties{.namespacedId = "opencraft:dropper",
                                                       .displayName = "Dropper",
                                                       .isSolid = true,
                                                       .isOpaque = true,
                                                       .isTransparent = false,
                                                       .textureIndices = {52, 52, 52, 52, 52, 52},
                                                       .legacyId = BlockID::Dropper,
                                                       .hardness = 3.5f,
                                                       .requiredTool = ToolType::Pickaxe,
                                                       .minHarvestTier = ToolTier::Wood,
                                                       .dropItemName = "opencraft:dropper",
                                                       .lightEmission = 0,
                                                       .lightFilter = 15});

    registerBlock("opencraft:dispenser", BlockProperties{.namespacedId = "opencraft:dispenser",
                                                         .displayName = "Dispenser",
                                                         .isSolid = true,
                                                         .isOpaque = true,
                                                         .isTransparent = false,
                                                         .textureIndices = {53, 53, 53, 53, 53, 53},
                                                         .legacyId = BlockID::Dispenser,
                                                         .hardness = 3.5f,
                                                         .requiredTool = ToolType::Pickaxe,
                                                         .minHarvestTier = ToolTier::Wood,
                                                         .dropItemName = "opencraft:dispenser",
                                                         .lightEmission = 0,
                                                         .lightFilter = 15});

    registerBlock("opencraft:crafter", BlockProperties{.namespacedId = "opencraft:crafter",
                                                       .displayName = "Crafter",
                                                       .isSolid = true,
                                                       .isOpaque = true,
                                                       .isTransparent = false,
                                                       .textureIndices = {54, 54, 54, 54, 54, 54},
                                                       .legacyId = BlockID::Crafter,
                                                       .hardness = 3.5f,
                                                       .requiredTool = ToolType::Pickaxe,
                                                       .minHarvestTier = ToolTier::Wood,
                                                       .dropItemName = "opencraft:crafter",
                                                       .lightEmission = 0,
                                                       .lightFilter = 15});

    registerBlock("opencraft:rail", BlockProperties{.namespacedId = "opencraft:rail",
                                                    .displayName = "Rail",
                                                    .isSolid = false,
                                                    .isOpaque = false,
                                                    .isTransparent = true,
                                                    .textureIndices = {55, 55, 55, 55, 55, 55},
                                                    .legacyId = BlockID::Rail,
                                                    .hardness = 0.7f,
                                                    .requiredTool = ToolType::Pickaxe,
                                                    .minHarvestTier = ToolTier::Wood,
                                                    .dropItemName = "opencraft:rail",
                                                    .lightEmission = 0,
                                                    .lightFilter = 1});

    registerBlock("opencraft:powered_rail", BlockProperties{.namespacedId = "opencraft:powered_rail",
                                                            .displayName = "Powered Rail",
                                                            .isSolid = false,
                                                            .isOpaque = false,
                                                            .isTransparent = true,
                                                            .textureIndices = {56, 56, 56, 56, 56, 56},
                                                            .legacyId = BlockID::PoweredRail,
                                                            .hardness = 0.7f,
                                                            .requiredTool = ToolType::Pickaxe,
                                                            .minHarvestTier = ToolTier::Wood,
                                                            .dropItemName = "opencraft:powered_rail",
                                                            .lightEmission = 0,
                                                            .lightFilter = 1});

    registerBlock("opencraft:detector_rail", BlockProperties{.namespacedId = "opencraft:detector_rail",
                                                             .displayName = "Detector Rail",
                                                             .isSolid = false,
                                                             .isOpaque = false,
                                                             .isTransparent = true,
                                                             .textureIndices = {57, 57, 57, 57, 57, 57},
                                                             .legacyId = BlockID::DetectorRail,
                                                             .hardness = 0.7f,
                                                             .requiredTool = ToolType::Pickaxe,
                                                             .minHarvestTier = ToolTier::Wood,
                                                             .dropItemName = "opencraft:detector_rail",
                                                             .lightEmission = 0,
                                                             .lightFilter = 1});

    registerBlock("opencraft:activator_rail", BlockProperties{.namespacedId = "opencraft:activator_rail",
                                                              .displayName = "Activator Rail",
                                                              .isSolid = false,
                                                              .isOpaque = false,
                                                              .isTransparent = true,
                                                              .textureIndices = {58, 58, 58, 58, 58, 58},
                                                              .legacyId = BlockID::ActivatorRail,
                                                              .hardness = 0.7f,
                                                              .requiredTool = ToolType::Pickaxe,
                                                              .minHarvestTier = ToolTier::Wood,
                                                              .dropItemName = "opencraft:activator_rail",
                                                              .lightEmission = 0,
                                                              .lightFilter = 1});

    registerBlock("opencraft:abyssal_rock", BlockProperties{.namespacedId = "opencraft:abyssal_rock",
                                                            .displayName = "Abyssal Rock",
                                                            .isSolid = true,
                                                            .isOpaque = true,
                                                            .isTransparent = false,
                                                            .textureIndices = {59, 59, 59, 59, 59, 59},
                                                            .legacyId = BlockID::AbyssalRock,
                                                            .hardness = 0.4f,
                                                            .requiredTool = ToolType::Pickaxe,
                                                            .minHarvestTier = ToolTier::Wood,
                                                            .dropItemName = "opencraft:abyssal_rock"});

    registerBlock("opencraft:soul_sand", BlockProperties{.namespacedId = "opencraft:soul_sand",
                                                         .displayName = "Soul Sand",
                                                         .isSolid = true,
                                                         .isOpaque = true,
                                                         .isTransparent = false,
                                                         .textureIndices = {60, 60, 60, 60, 60, 60},
                                                         .legacyId = BlockID::SoulSand,
                                                         .hardness = 0.5f,
                                                         .requiredTool = ToolType::Shovel,
                                                         .minHarvestTier = ToolTier::None,
                                                         .dropItemName = "opencraft:soul_sand"});

    registerBlock("opencraft:soul_soil", BlockProperties{.namespacedId = "opencraft:soul_soil",
                                                         .displayName = "Soul Soil",
                                                         .isSolid = true,
                                                         .isOpaque = true,
                                                         .isTransparent = false,
                                                         .textureIndices = {61, 61, 61, 61, 61, 61},
                                                         .legacyId = BlockID::SoulSoil,
                                                         .hardness = 0.5f,
                                                         .requiredTool = ToolType::Shovel,
                                                         .minHarvestTier = ToolTier::None,
                                                         .dropItemName = "opencraft:soul_soil"});

    registerBlock("opencraft:abyssal_quartz_ore", BlockProperties{.namespacedId = "opencraft:abyssal_quartz_ore",
                                                                  .displayName = "Abyssal Quartz Ore",
                                                                  .isSolid = true,
                                                                  .isOpaque = true,
                                                                  .isTransparent = false,
                                                                  .textureIndices = {62, 62, 62, 62, 62, 62},
                                                                  .legacyId = BlockID::AbyssalQuartzOre,
                                                                  .hardness = 3.0f,
                                                                  .requiredTool = ToolType::Pickaxe,
                                                                  .minHarvestTier = ToolTier::Wood,
                                                                  .dropItemName = "opencraft:quartz"});

    registerBlock("opencraft:glow_rock", BlockProperties{.namespacedId = "opencraft:glow_rock",
                                                         .displayName = "Glow Rock",
                                                         .isSolid = true,
                                                         .isOpaque = true,
                                                         .isTransparent = false,
                                                         .textureIndices = {63, 63, 63, 63, 63, 63},
                                                         .legacyId = BlockID::GlowRock,
                                                         .hardness = 0.3f,
                                                         .dropItemName = "opencraft:glow_rock_dust",
                                                         .lightEmission = 15});

    registerBlock("opencraft:orichalcum_vein", BlockProperties{.namespacedId = "opencraft:orichalcum_vein",
                                                               .displayName = "Orichalcum Vein",
                                                               .isSolid = true,
                                                               .isOpaque = true,
                                                               .isTransparent = false,
                                                               .textureIndices = {64, 64, 64, 64, 64, 64},
                                                               .legacyId = BlockID::OrichalcumVein,
                                                               .hardness = 30.0f,
                                                               .requiredTool = ToolType::Pickaxe,
                                                               .minHarvestTier = ToolTier::Diamond,
                                                               .dropItemName = "opencraft:orichalcum_vein"});

    registerBlock("opencraft:rim_stone", BlockProperties{.namespacedId = "opencraft:rim_stone",
                                                         .displayName = "Rim Stone",
                                                         .isSolid = true,
                                                         .isOpaque = true,
                                                         .isTransparent = false,
                                                         .textureIndices = {65, 65, 65, 65, 65, 65},
                                                         .legacyId = BlockID::RimStone,
                                                         .hardness = 3.0f,
                                                         .requiredTool = ToolType::Pickaxe,
                                                         .minHarvestTier = ToolTier::Wood,
                                                         .dropItemName = "opencraft:rim_stone"});

    registerBlock("opencraft:obsidian_pillar", BlockProperties{.namespacedId = "opencraft:obsidian_pillar",
                                                               .displayName = "Obsidian Pillar",
                                                               .isSolid = true,
                                                               .isOpaque = true,
                                                               .isTransparent = false,
                                                               .textureIndices = {21, 21, 21, 21, 21, 21},
                                                               .legacyId = BlockID::ObsidianPillar,
                                                               .hardness = 50.0f,
                                                               .requiredTool = ToolType::Pickaxe,
                                                               .minHarvestTier = ToolTier::Diamond,
                                                               .dropItemName = "opencraft:obsidian"});

    registerBlock("opencraft:brewing_stand", BlockProperties{.namespacedId = "opencraft:brewing_stand",
                                                             .displayName = "Brewing Stand",
                                                             .isSolid = false,
                                                             .isOpaque = false,
                                                             .isTransparent = true,
                                                             .textureIndices = {66, 66, 66, 66, 66, 66},
                                                             .legacyId = BlockID::BrewingStand,
                                                             .hardness = 0.5f,
                                                             .dropItemName = "opencraft:brewing_stand"});

    registerBlock("opencraft:beacon", BlockProperties{.namespacedId = "opencraft:beacon",
                                                      .displayName = "Beacon",
                                                      .isSolid = true,
                                                      .isOpaque = false,
                                                      .isTransparent = true,
                                                      .textureIndices = {67, 67, 67, 67, 67, 67},
                                                      .legacyId = BlockID::BeaconBlock,
                                                      .hardness = 3.0f,
                                                      .dropItemName = "opencraft:beacon",
                                                      .lightEmission = 15});

    registerBlock("opencraft:abyss_portal", BlockProperties{.namespacedId = "opencraft:abyss_portal",
                                                            .displayName = "Abyss Portal",
                                                            .isSolid = false,
                                                            .isOpaque = false,
                                                            .isTransparent = true,
                                                            .textureIndices = {68, 68, 68, 68, 68, 68},
                                                            .legacyId = BlockID::AbyssPortal,
                                                            .hardness = -1.0f,
                                                            .dropItemName = "",
                                                            .lightEmission = 11});

    registerBlock("opencraft:rim_portal_frame", BlockProperties{.namespacedId = "opencraft:rim_portal_frame",
                                                                .displayName = "Rim Portal Frame",
                                                                .isSolid = true,
                                                                .isOpaque = true,
                                                                .isTransparent = false,
                                                                .textureIndices = {69, 69, 69, 69, 69, 69},
                                                                .legacyId = BlockID::RimPortalFrame,
                                                                .hardness = -1.0f,
                                                                .dropItemName = "",
                                                                .lightEmission = 1});

    registerBlock("opencraft:rim_portal", BlockProperties{.namespacedId = "opencraft:rim_portal",
                                                          .displayName = "Rim Portal",
                                                          .isSolid = false,
                                                          .isOpaque = false,
                                                          .isTransparent = true,
                                                          .textureIndices = {70, 70, 70, 70, 70, 70},
                                                          .legacyId = BlockID::RimPortal,
                                                          .hardness = -1.0f,
                                                          .dropItemName = "",
                                                          .lightEmission = 15});

    registerBlock("opencraft:dragon_egg", BlockProperties{.namespacedId = "opencraft:dragon_egg",
                                                          .displayName = "Dragon Egg",
                                                          .isSolid = true,
                                                          .isOpaque = true,
                                                          .isTransparent = false,
                                                          .textureIndices = {71, 71, 71, 71, 71, 71},
                                                          .legacyId = BlockID::DragonEgg,
                                                          .hardness = 3.0f,
                                                          .dropItemName = "opencraft:dragon_egg",
                                                          .lightEmission = 1});

    registerBlock("opencraft:command_block", BlockProperties{.namespacedId = "opencraft:command_block",
                                                             .displayName = "Command Block",
                                                             .isSolid = true,
                                                             .isOpaque = true,
                                                             .isTransparent = false,
                                                             .textureIndices = {72, 72, 72, 72, 72, 72},
                                                             .legacyId = BlockID::CommandBlock,
                                                             .hardness = -1.0f,
                                                             .dropItemName = "",
                                                             .lightEmission = 0});

    registerBlock("opencraft:repeating_command_block",
                  BlockProperties{.namespacedId = "opencraft:repeating_command_block",
                                  .displayName = "Repeating Command Block",
                                  .isSolid = true,
                                  .isOpaque = true,
                                  .isTransparent = false,
                                  .textureIndices = {73, 73, 73, 73, 73, 73},
                                  .legacyId = BlockID::RepeatingCommandBlock,
                                  .hardness = -1.0f,
                                  .dropItemName = "",
                                  .lightEmission = 0});

    registerBlock("opencraft:chain_command_block", BlockProperties{.namespacedId = "opencraft:chain_command_block",
                                                                   .displayName = "Chain Command Block",
                                                                   .isSolid = true,
                                                                   .isOpaque = true,
                                                                   .isTransparent = false,
                                                                   .textureIndices = {74, 74, 74, 74, 74, 74},
                                                                   .legacyId = BlockID::ChainCommandBlock,
                                                                   .hardness = -1.0f,
                                                                   .dropItemName = "",
                                                                   .lightEmission = 0});

    // --- Fase 15: Contenido 1.21 (cámaras de desafío) y Variedades ---
    registerBlock("opencraft:copper_block", BlockProperties{.namespacedId = "opencraft:copper_block",
                                                            .displayName = "Copper Block",
                                                            .isSolid = true,
                                                            .isOpaque = true,
                                                            .isTransparent = false,
                                                            .textureIndices = {75, 75, 75, 75, 75, 75},
                                                            .legacyId = BlockID::CopperBlock,
                                                            .hardness = 3.0f,
                                                            .requiredTool = ToolType::Pickaxe,
                                                            .minHarvestTier = ToolTier::Stone,
                                                            .dropItemName = "opencraft:copper_block"});

    registerBlock("opencraft:cut_copper", BlockProperties{.namespacedId = "opencraft:cut_copper",
                                                          .displayName = "Cut Copper",
                                                          .isSolid = true,
                                                          .isOpaque = true,
                                                          .isTransparent = false,
                                                          .textureIndices = {76, 76, 76, 76, 76, 76},
                                                          .legacyId = BlockID::CutCopper,
                                                          .hardness = 3.0f,
                                                          .requiredTool = ToolType::Pickaxe,
                                                          .minHarvestTier = ToolTier::Stone,
                                                          .dropItemName = "opencraft:cut_copper"});

    registerBlock("opencraft:chiseled_copper", BlockProperties{.namespacedId = "opencraft:chiseled_copper",
                                                               .displayName = "Chiseled Copper",
                                                               .isSolid = true,
                                                               .isOpaque = true,
                                                               .isTransparent = false,
                                                               .textureIndices = {77, 77, 77, 77, 77, 77},
                                                               .legacyId = BlockID::ChiseledCopper,
                                                               .hardness = 3.0f,
                                                               .requiredTool = ToolType::Pickaxe,
                                                               .minHarvestTier = ToolTier::Stone,
                                                               .dropItemName = "opencraft:chiseled_copper"});

    registerBlock("opencraft:copper_grate", BlockProperties{.namespacedId = "opencraft:copper_grate",
                                                            .displayName = "Copper Grate",
                                                            .isSolid = true,
                                                            .isOpaque = false,
                                                            .isTransparent = true,
                                                            .textureIndices = {78, 78, 78, 78, 78, 78},
                                                            .legacyId = BlockID::CopperGrate,
                                                            .hardness = 3.0f,
                                                            .requiredTool = ToolType::Pickaxe,
                                                            .minHarvestTier = ToolTier::Stone,
                                                            .dropItemName = "opencraft:copper_grate",
                                                            .lightFilter = 1});

    registerBlock("opencraft:copper_bulb", BlockProperties{.namespacedId = "opencraft:copper_bulb",
                                                           .displayName = "Copper Bulb",
                                                           .isSolid = true,
                                                           .isOpaque = true,
                                                           .isTransparent = false,
                                                           .textureIndices = {79, 79, 79, 79, 79, 79},
                                                           .legacyId = BlockID::CopperBulb,
                                                           .hardness = 3.0f,
                                                           .requiredTool = ToolType::Pickaxe,
                                                           .minHarvestTier = ToolTier::Stone,
                                                           .dropItemName = "opencraft:copper_bulb",
                                                           .lightEmission = 15});

    registerBlock("opencraft:tuff", BlockProperties{.namespacedId = "opencraft:tuff",
                                                    .displayName = "Tuff",
                                                    .isSolid = true,
                                                    .isOpaque = true,
                                                    .isTransparent = false,
                                                    .textureIndices = {80, 80, 80, 80, 80, 80},
                                                    .legacyId = BlockID::Tuff,
                                                    .hardness = 1.5f,
                                                    .requiredTool = ToolType::Pickaxe,
                                                    .minHarvestTier = ToolTier::Wood,
                                                    .dropItemName = "opencraft:tuff"});

    registerBlock("opencraft:tuff_bricks", BlockProperties{.namespacedId = "opencraft:tuff_bricks",
                                                           .displayName = "Tuff Bricks",
                                                           .isSolid = true,
                                                           .isOpaque = true,
                                                           .isTransparent = false,
                                                           .textureIndices = {81, 81, 81, 81, 81, 81},
                                                           .legacyId = BlockID::TuffBricks,
                                                           .hardness = 1.5f,
                                                           .requiredTool = ToolType::Pickaxe,
                                                           .minHarvestTier = ToolTier::Wood,
                                                           .dropItemName = "opencraft:tuff_bricks"});

    registerBlock("opencraft:chiseled_tuff", BlockProperties{.namespacedId = "opencraft:chiseled_tuff",
                                                             .displayName = "Chiseled Tuff",
                                                             .isSolid = true,
                                                             .isOpaque = true,
                                                             .isTransparent = false,
                                                             .textureIndices = {82, 82, 82, 82, 82, 82},
                                                             .legacyId = BlockID::ChiseledTuff,
                                                             .hardness = 1.5f,
                                                             .requiredTool = ToolType::Pickaxe,
                                                             .minHarvestTier = ToolTier::Wood,
                                                             .dropItemName = "opencraft:chiseled_tuff"});

    registerBlock("opencraft:trial_spawner", BlockProperties{.namespacedId = "opencraft:trial_spawner",
                                                             .displayName = "Trial Spawner",
                                                             .isSolid = true,
                                                             .isOpaque = true,
                                                             .isTransparent = false,
                                                             .textureIndices = {83, 83, 83, 83, 83, 83},
                                                             .legacyId = BlockID::TrialSpawner,
                                                             .hardness = 50.0f,
                                                             .requiredTool = ToolType::Pickaxe,
                                                             .minHarvestTier = ToolTier::Iron,
                                                             .dropItemName = "",
                                                             .lightEmission = 7});

    registerBlock("opencraft:vault", BlockProperties{.namespacedId = "opencraft:vault",
                                                     .displayName = "Vault",
                                                     .isSolid = true,
                                                     .isOpaque = true,
                                                     .isTransparent = false,
                                                     .textureIndices = {84, 84, 84, 84, 84, 84},
                                                     .legacyId = BlockID::Vault,
                                                     .hardness = 50.0f,
                                                     .requiredTool = ToolType::Pickaxe,
                                                     .minHarvestTier = ToolTier::Iron,
                                                     .dropItemName = "",
                                                     .lightEmission = 6});

    registerBlock("opencraft:oak_sign", BlockProperties{.namespacedId = "opencraft:oak_sign",
                                                        .displayName = "Oak Sign",
                                                        .isSolid = false,
                                                        .isOpaque = false,
                                                        .isTransparent = true,
                                                        .textureIndices = {85, 85, 85, 85, 85, 85},
                                                        .legacyId = BlockID::Sign,
                                                        .hardness = 1.0f,
                                                        .requiredTool = ToolType::Axe,
                                                        .minHarvestTier = ToolTier::None,
                                                        .dropItemName = "opencraft:oak_sign",
                                                        .lightFilter = 1});

    registerBlock("opencraft:decorated_pot", BlockProperties{.namespacedId = "opencraft:decorated_pot",
                                                             .displayName = "Decorated Pot",
                                                             .isSolid = true,
                                                             .isOpaque = false,
                                                             .isTransparent = true,
                                                             .textureIndices = {86, 86, 86, 86, 86, 86},
                                                             .legacyId = BlockID::DecoratedPot,
                                                             .hardness = 0.5f,
                                                             .requiredTool = ToolType::None,
                                                             .minHarvestTier = ToolTier::None,
                                                             .dropItemName = "opencraft:decorated_pot",
                                                             .lightFilter = 1});
}

} // namespace OpenCraft

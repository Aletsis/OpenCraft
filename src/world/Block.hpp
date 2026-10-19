#pragma once

#include <cstdint>
#include <string_view>
#include <array>

#include "world/ToolTypes.hpp"

namespace OpenCraft {

enum class BlockID : uint8_t {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Cobblestone,
    Wood,
    Planks,
    Leaves,
    Sand,
    Glass,
    Bedrock,
    Water,
    FlowingWater,
    Lava,
    FlowingLava,
    Obsidian,
    Torch,
    Fire,
    Chest,
    CraftingTable,
    Furnace,
    CoalOre,
    IronOre,
    Bed,
    Farmland,
    WheatCrops,
    Carrots,
    Potatoes,
    Snow,
    SnowBlock,
    Ice,
    Sandstone,
    Gravel,
    GoldOre,
    DiamondOre,
    EmeraldOre,
    MossyCobblestone,
    MonsterSpawner,
    Composter,
    Lectern,
    CinnabarWire,
    CinnabarBlock,
    CinnabarTorch,
    CinnabarTorchOff,
    Repeater,
    Comparator,
    Lever,
    StoneButton,
    OakButton,
    PressurePlate,
    DaylightDetector,
    CinnabarLamp,
    LitCinnabarLamp,
    IronDoor,
    OakDoor,
    Piston,
    StickyPiston,
    PistonHead,
    Hopper,
    Dropper,
    Dispenser,
    Crafter,
    Rail,
    PoweredRail,
    DetectorRail,
    ActivatorRail,
    AbyssalRock,
    SoulSand,
    SoulSoil,
    AbyssalQuartzOre,
    GlowRock,
    OrichalcumVein,
    RimStone,
    ObsidianPillar,
    BrewingStand,
    BeaconBlock,
    AbyssPortal,
    RimPortalFrame,
    RimPortal,
    DragonEgg,
    CommandBlock,
    RepeatingCommandBlock,
    ChainCommandBlock,
    CopperBlock,
    CutCopper,
    ChiseledCopper,
    CopperGrate,
    CopperBulb,
    Tuff,
    TuffBricks,
    ChiseledTuff,
    TrialSpawner,
    Vault,
    Sign,
    DecoratedPot,
    Count
};

enum class BlockFace : uint8_t {
    Top = 0, // +Y
    Bottom,  // -Y
    North,   // -Z
    South,   // +Z
    West,    // -X
    East     // +X
};

struct BlockInfo {
    std::string_view name;
    bool isSolid{true};
    bool isOpaque{true};
    bool isTransparent{false};
    // Texture tile index on 16x16 atlas for each face: [Top, Bottom, North, South, West, East]
    std::array<uint8_t, 6> textureIndices{0, 0, 0, 0, 0, 0};
    float hardness{1.0f};
    ToolType requiredTool{ToolType::None};
    ToolTier minHarvestTier{ToolTier::None};
    uint8_t lightEmission{0};
    uint8_t lightFilter{15}; // 15 = blocks light completely, 1 = transparent air/glass, 2 = water/leaves
};

class BlockDatabase {
public:
    static const BlockInfo& get(BlockID id) noexcept {
        size_t index = static_cast<size_t>(id);
        if (index >= static_cast<size_t>(BlockID::Count)) {
            return s_blocks[0]; // fallback to Air
        }
        return s_blocks[index];
    }

private:
    static inline const std::array<BlockInfo, static_cast<size_t>(BlockID::Count)> s_blocks = []() constexpr {
        std::array<BlockInfo, static_cast<size_t>(BlockID::Count)> b{};

        // Air
        b[static_cast<size_t>(BlockID::Air)] = BlockInfo{.name = "Air",
                                                         .isSolid = false,
                                                         .isOpaque = false,
                                                         .isTransparent = true,
                                                         .textureIndices = {0, 0, 0, 0, 0, 0},
                                                         .hardness = 0.0f,
                                                         .requiredTool = ToolType::None,
                                                         .minHarvestTier = ToolTier::None,
                                                         .lightEmission = 0,
                                                         .lightFilter = 1};

        // Grass: Top (0), Bottom Dirt (2), Sides (1)
        b[static_cast<size_t>(BlockID::Grass)] = BlockInfo{.name = "Grass",
                                                           .isSolid = true,
                                                           .isOpaque = true,
                                                           .isTransparent = false,
                                                           .textureIndices = {0, 2, 1, 1, 1, 1},
                                                           .hardness = 0.6f,
                                                           .requiredTool = ToolType::Shovel,
                                                           .minHarvestTier = ToolTier::None,
                                                           .lightEmission = 0,
                                                           .lightFilter = 15};

        // Dirt: tile 2
        b[static_cast<size_t>(BlockID::Dirt)] = BlockInfo{.name = "Dirt",
                                                          .isSolid = true,
                                                          .isOpaque = true,
                                                          .isTransparent = false,
                                                          .textureIndices = {2, 2, 2, 2, 2, 2},
                                                          .hardness = 0.5f,
                                                          .requiredTool = ToolType::Shovel,
                                                          .minHarvestTier = ToolTier::None,
                                                          .lightEmission = 0,
                                                          .lightFilter = 15};

        // Stone: tile 3
        b[static_cast<size_t>(BlockID::Stone)] = BlockInfo{.name = "Stone",
                                                           .isSolid = true,
                                                           .isOpaque = true,
                                                           .isTransparent = false,
                                                           .textureIndices = {3, 3, 3, 3, 3, 3},
                                                           .hardness = 1.5f,
                                                           .requiredTool = ToolType::Pickaxe,
                                                           .minHarvestTier = ToolTier::Wood,
                                                           .lightEmission = 0,
                                                           .lightFilter = 15};

        // Cobblestone: tile 4
        b[static_cast<size_t>(BlockID::Cobblestone)] = BlockInfo{.name = "Cobblestone",
                                                                 .isSolid = true,
                                                                 .isOpaque = true,
                                                                 .isTransparent = false,
                                                                 .textureIndices = {4, 4, 4, 4, 4, 4},
                                                                 .hardness = 2.0f,
                                                                 .requiredTool = ToolType::Pickaxe,
                                                                 .minHarvestTier = ToolTier::Wood,
                                                                 .lightEmission = 0,
                                                                 .lightFilter = 15};

        // Wood Log: Top/Bottom ring (6), Sides bark (5)
        b[static_cast<size_t>(BlockID::Wood)] = BlockInfo{.name = "Wood",
                                                          .isSolid = true,
                                                          .isOpaque = true,
                                                          .isTransparent = false,
                                                          .textureIndices = {6, 6, 5, 5, 5, 5},
                                                          .hardness = 2.0f,
                                                          .requiredTool = ToolType::Axe,
                                                          .minHarvestTier = ToolTier::None,
                                                          .lightEmission = 0,
                                                          .lightFilter = 15};

        // Planks: tile 7
        b[static_cast<size_t>(BlockID::Planks)] = BlockInfo{.name = "Planks",
                                                            .isSolid = true,
                                                            .isOpaque = true,
                                                            .isTransparent = false,
                                                            .textureIndices = {7, 7, 7, 7, 7, 7},
                                                            .hardness = 2.0f,
                                                            .requiredTool = ToolType::Axe,
                                                            .minHarvestTier = ToolTier::None,
                                                            .lightEmission = 0,
                                                            .lightFilter = 15};

        // Leaves: tile 8 (transparent/cutout)
        b[static_cast<size_t>(BlockID::Leaves)] = BlockInfo{.name = "Leaves",
                                                            .isSolid = true,
                                                            .isOpaque = false,
                                                            .isTransparent = true,
                                                            .textureIndices = {8, 8, 8, 8, 8, 8},
                                                            .hardness = 0.2f,
                                                            .requiredTool = ToolType::None,
                                                            .minHarvestTier = ToolTier::None,
                                                            .lightEmission = 0,
                                                            .lightFilter = 2};

        // Sand: tile 9
        b[static_cast<size_t>(BlockID::Sand)] = BlockInfo{.name = "Sand",
                                                          .isSolid = true,
                                                          .isOpaque = true,
                                                          .isTransparent = false,
                                                          .textureIndices = {9, 9, 9, 9, 9, 9},
                                                          .hardness = 0.5f,
                                                          .requiredTool = ToolType::Shovel,
                                                          .minHarvestTier = ToolTier::None,
                                                          .lightEmission = 0,
                                                          .lightFilter = 15};

        // Glass: tile 10 (transparent)
        b[static_cast<size_t>(BlockID::Glass)] = BlockInfo{.name = "Glass",
                                                           .isSolid = true,
                                                           .isOpaque = false,
                                                           .isTransparent = true,
                                                           .textureIndices = {10, 10, 10, 10, 10, 10},
                                                           .hardness = 0.3f,
                                                           .requiredTool = ToolType::None,
                                                           .minHarvestTier = ToolTier::None,
                                                           .lightEmission = 0,
                                                           .lightFilter = 1};

        // Bedrock: tile 11
        b[static_cast<size_t>(BlockID::Bedrock)] = BlockInfo{.name = "Bedrock",
                                                             .isSolid = true,
                                                             .isOpaque = true,
                                                             .isTransparent = false,
                                                             .textureIndices = {11, 11, 11, 11, 11, 11},
                                                             .hardness = -1.0f,
                                                             .requiredTool = ToolType::None,
                                                             .minHarvestTier = ToolTier::None,
                                                             .lightEmission = 0,
                                                             .lightFilter = 15};

        // Water: tile 12 (fuente)
        b[static_cast<size_t>(BlockID::Water)] = BlockInfo{.name = "Water",
                                                           .isSolid = false,
                                                           .isOpaque = false,
                                                           .isTransparent = true,
                                                           .textureIndices = {12, 12, 12, 12, 12, 12},
                                                           .hardness = 100.0f,
                                                           .requiredTool = ToolType::None,
                                                           .minHarvestTier = ToolTier::None,
                                                           .lightEmission = 0,
                                                           .lightFilter = 2};

        // FlowingWater: tile 12 (flujo)
        b[static_cast<size_t>(BlockID::FlowingWater)] = BlockInfo{.name = "Flowing Water",
                                                                  .isSolid = false,
                                                                  .isOpaque = false,
                                                                  .isTransparent = true,
                                                                  .textureIndices = {12, 12, 12, 12, 12, 12},
                                                                  .hardness = 100.0f,
                                                                  .requiredTool = ToolType::None,
                                                                  .minHarvestTier = ToolTier::None,
                                                                  .lightEmission = 0,
                                                                  .lightFilter = 2};

        // Lava: tile 20 (fuente)
        b[static_cast<size_t>(BlockID::Lava)] = BlockInfo{.name = "Lava",
                                                          .isSolid = false,
                                                          .isOpaque = false,
                                                          .isTransparent = true,
                                                          .textureIndices = {20, 20, 20, 20, 20, 20},
                                                          .hardness = 100.0f,
                                                          .requiredTool = ToolType::None,
                                                          .minHarvestTier = ToolTier::None,
                                                          .lightEmission = 15,
                                                          .lightFilter = 2};

        // FlowingLava: tile 20 (flujo)
        b[static_cast<size_t>(BlockID::FlowingLava)] = BlockInfo{.name = "Flowing Lava",
                                                                 .isSolid = false,
                                                                 .isOpaque = false,
                                                                 .isTransparent = true,
                                                                 .textureIndices = {20, 20, 20, 20, 20, 20},
                                                                 .hardness = 100.0f,
                                                                 .requiredTool = ToolType::None,
                                                                 .minHarvestTier = ToolTier::None,
                                                                 .lightEmission = 15,
                                                                 .lightFilter = 2};

        // Obsidian: tile 21
        b[static_cast<size_t>(BlockID::Obsidian)] = BlockInfo{.name = "Obsidian",
                                                              .isSolid = true,
                                                              .isOpaque = true,
                                                              .isTransparent = false,
                                                              .textureIndices = {21, 21, 21, 21, 21, 21},
                                                              .hardness = 50.0f,
                                                              .requiredTool = ToolType::Pickaxe,
                                                              .minHarvestTier = ToolTier::Diamond,
                                                              .lightEmission = 0,
                                                              .lightFilter = 15};

        // Torch: tile 22
        b[static_cast<size_t>(BlockID::Torch)] = BlockInfo{.name = "Torch",
                                                           .isSolid = false,
                                                           .isOpaque = false,
                                                           .isTransparent = true,
                                                           .textureIndices = {22, 22, 22, 22, 22, 22},
                                                           .hardness = 0.0f,
                                                           .requiredTool = ToolType::None,
                                                           .minHarvestTier = ToolTier::None,
                                                           .lightEmission = 14,
                                                           .lightFilter = 1};

        // Fire: tile 23
        b[static_cast<size_t>(BlockID::Fire)] = BlockInfo{.name = "Fire",
                                                          .isSolid = false,
                                                          .isOpaque = false,
                                                          .isTransparent = true,
                                                          .textureIndices = {23, 23, 23, 23, 23, 23},
                                                          .hardness = 0.0f,
                                                          .requiredTool = ToolType::None,
                                                          .minHarvestTier = ToolTier::None,
                                                          .lightEmission = 15,
                                                          .lightFilter = 1};

        // Chest: tile 7 (wood texture fallback)
        b[static_cast<size_t>(BlockID::Chest)] = BlockInfo{.name = "Chest",
                                                           .isSolid = true,
                                                           .isOpaque = true,
                                                           .isTransparent = false,
                                                           .textureIndices = {7, 7, 7, 7, 7, 7},
                                                           .hardness = 2.5f,
                                                           .requiredTool = ToolType::Axe,
                                                           .minHarvestTier = ToolTier::None,
                                                           .lightEmission = 0,
                                                           .lightFilter = 15};

        // CraftingTable: tile 13
        b[static_cast<size_t>(BlockID::CraftingTable)] = BlockInfo{.name = "Crafting Table",
                                                                   .isSolid = true,
                                                                   .isOpaque = true,
                                                                   .isTransparent = false,
                                                                   .textureIndices = {13, 7, 13, 13, 13, 13},
                                                                   .hardness = 2.5f,
                                                                   .requiredTool = ToolType::Axe,
                                                                   .minHarvestTier = ToolTier::None,
                                                                   .lightEmission = 0,
                                                                   .lightFilter = 15};

        // Furnace: tile 14
        b[static_cast<size_t>(BlockID::Furnace)] = BlockInfo{.name = "Furnace",
                                                             .isSolid = true,
                                                             .isOpaque = true,
                                                             .isTransparent = false,
                                                             .textureIndices = {4, 4, 14, 4, 4, 4},
                                                             .hardness = 3.5f,
                                                             .requiredTool = ToolType::Pickaxe,
                                                             .minHarvestTier = ToolTier::Wood,
                                                             .lightEmission = 0,
                                                             .lightFilter = 15};

        // CoalOre: tile 15
        b[static_cast<size_t>(BlockID::CoalOre)] = BlockInfo{.name = "Coal Ore",
                                                             .isSolid = true,
                                                             .isOpaque = true,
                                                             .isTransparent = false,
                                                             .textureIndices = {15, 15, 15, 15, 15, 15},
                                                             .hardness = 3.0f,
                                                             .requiredTool = ToolType::Pickaxe,
                                                             .minHarvestTier = ToolTier::Wood,
                                                             .lightEmission = 0,
                                                             .lightFilter = 15};

        // IronOre: tile 16
        b[static_cast<size_t>(BlockID::IronOre)] = BlockInfo{.name = "Iron Ore",
                                                             .isSolid = true,
                                                             .isOpaque = true,
                                                             .isTransparent = false,
                                                             .textureIndices = {16, 16, 16, 16, 16, 16},
                                                             .hardness = 3.0f,
                                                             .requiredTool = ToolType::Pickaxe,
                                                             .minHarvestTier = ToolTier::Stone,
                                                             .lightEmission = 0,
                                                             .lightFilter = 15};

        // Bed: tile 17
        b[static_cast<size_t>(BlockID::Bed)] = BlockInfo{.name = "Bed",
                                                         .isSolid = true,
                                                         .isOpaque = false,
                                                         .isTransparent = true,
                                                         .textureIndices = {17, 7, 17, 17, 17, 17},
                                                         .hardness = 0.2f,
                                                         .requiredTool = ToolType::None,
                                                         .minHarvestTier = ToolTier::None,
                                                         .lightEmission = 0,
                                                         .lightFilter = 1};

        // Farmland: tile 18 top, tile 2 rest
        b[static_cast<size_t>(BlockID::Farmland)] = BlockInfo{.name = "Farmland",
                                                              .isSolid = true,
                                                              .isOpaque = true,
                                                              .isTransparent = false,
                                                              .textureIndices = {18, 2, 2, 2, 2, 2},
                                                              .hardness = 0.6f,
                                                              .requiredTool = ToolType::Shovel,
                                                              .minHarvestTier = ToolTier::None,
                                                              .lightEmission = 0,
                                                              .lightFilter = 15};

        // WheatCrops: tile 19
        b[static_cast<size_t>(BlockID::WheatCrops)] = BlockInfo{.name = "Wheat Crops",
                                                                .isSolid = false,
                                                                .isOpaque = false,
                                                                .isTransparent = true,
                                                                .textureIndices = {19, 19, 19, 19, 19, 19},
                                                                .hardness = 0.0f,
                                                                .requiredTool = ToolType::None,
                                                                .minHarvestTier = ToolTier::None,
                                                                .lightEmission = 0,
                                                                .lightFilter = 1};

        // Carrots: tile 19
        b[static_cast<size_t>(BlockID::Carrots)] = BlockInfo{.name = "Carrots",
                                                             .isSolid = false,
                                                             .isOpaque = false,
                                                             .isTransparent = true,
                                                             .textureIndices = {19, 19, 19, 19, 19, 19},
                                                             .hardness = 0.0f,
                                                             .requiredTool = ToolType::None,
                                                             .minHarvestTier = ToolTier::None,
                                                             .lightEmission = 0,
                                                             .lightFilter = 1};

        // Potatoes: tile 19
        b[static_cast<size_t>(BlockID::Potatoes)] = BlockInfo{.name = "Potatoes",
                                                              .isSolid = false,
                                                              .isOpaque = false,
                                                              .isTransparent = true,
                                                              .textureIndices = {19, 19, 19, 19, 19, 19},
                                                              .hardness = 0.0f,
                                                              .requiredTool = ToolType::None,
                                                              .minHarvestTier = ToolTier::None,
                                                              .lightEmission = 0,
                                                              .lightFilter = 1};

        // Snow: tile 18
        b[static_cast<size_t>(BlockID::Snow)] = BlockInfo{.name = "Snow",
                                                          .isSolid = false,
                                                          .isOpaque = false,
                                                          .isTransparent = true,
                                                          .textureIndices = {18, 18, 18, 18, 18, 18},
                                                          .hardness = 0.1f,
                                                          .requiredTool = ToolType::Shovel,
                                                          .minHarvestTier = ToolTier::None,
                                                          .lightEmission = 0,
                                                          .lightFilter = 1};

        // SnowBlock: tile 18
        b[static_cast<size_t>(BlockID::SnowBlock)] = BlockInfo{.name = "Snow Block",
                                                               .isSolid = true,
                                                               .isOpaque = true,
                                                               .isTransparent = false,
                                                               .textureIndices = {18, 18, 18, 18, 18, 18},
                                                               .hardness = 0.2f,
                                                               .requiredTool = ToolType::Shovel,
                                                               .minHarvestTier = ToolTier::None,
                                                               .lightEmission = 0,
                                                               .lightFilter = 15};

        // Ice: tile 19
        b[static_cast<size_t>(BlockID::Ice)] = BlockInfo{.name = "Ice",
                                                         .isSolid = true,
                                                         .isOpaque = false,
                                                         .isTransparent = true,
                                                         .textureIndices = {19, 19, 19, 19, 19, 19},
                                                         .hardness = 0.5f,
                                                         .requiredTool = ToolType::Pickaxe,
                                                         .minHarvestTier = ToolTier::None,
                                                         .lightEmission = 0,
                                                         .lightFilter = 2};

        // Sandstone: tile 24
        b[static_cast<size_t>(BlockID::Sandstone)] = BlockInfo{.name = "Sandstone",
                                                               .isSolid = true,
                                                               .isOpaque = true,
                                                               .isTransparent = false,
                                                               .textureIndices = {24, 24, 24, 24, 24, 24},
                                                               .hardness = 0.8f,
                                                               .requiredTool = ToolType::Pickaxe,
                                                               .minHarvestTier = ToolTier::Wood,
                                                               .lightEmission = 0,
                                                               .lightFilter = 15};

        // Gravel: tile 25
        b[static_cast<size_t>(BlockID::Gravel)] = BlockInfo{.name = "Gravel",
                                                            .isSolid = true,
                                                            .isOpaque = true,
                                                            .isTransparent = false,
                                                            .textureIndices = {25, 25, 25, 25, 25, 25},
                                                            .hardness = 0.6f,
                                                            .requiredTool = ToolType::Shovel,
                                                            .minHarvestTier = ToolTier::None,
                                                            .lightEmission = 0,
                                                            .lightFilter = 15};

        // GoldOre: tile 26
        b[static_cast<size_t>(BlockID::GoldOre)] = BlockInfo{.name = "Gold Ore",
                                                             .isSolid = true,
                                                             .isOpaque = true,
                                                             .isTransparent = false,
                                                             .textureIndices = {26, 26, 26, 26, 26, 26},
                                                             .hardness = 3.0f,
                                                             .requiredTool = ToolType::Pickaxe,
                                                             .minHarvestTier = ToolTier::Iron,
                                                             .lightEmission = 0,
                                                             .lightFilter = 15};

        // DiamondOre: tile 27
        b[static_cast<size_t>(BlockID::DiamondOre)] = BlockInfo{.name = "Diamond Ore",
                                                                .isSolid = true,
                                                                .isOpaque = true,
                                                                .isTransparent = false,
                                                                .textureIndices = {27, 27, 27, 27, 27, 27},
                                                                .hardness = 3.0f,
                                                                .requiredTool = ToolType::Pickaxe,
                                                                .minHarvestTier = ToolTier::Iron,
                                                                .lightEmission = 0,
                                                                .lightFilter = 15};

        // EmeraldOre: tile 28
        b[static_cast<size_t>(BlockID::EmeraldOre)] = BlockInfo{.name = "Emerald Ore",
                                                                .isSolid = true,
                                                                .isOpaque = true,
                                                                .isTransparent = false,
                                                                .textureIndices = {28, 28, 28, 28, 28, 28},
                                                                .hardness = 3.0f,
                                                                .requiredTool = ToolType::Pickaxe,
                                                                .minHarvestTier = ToolTier::Iron,
                                                                .lightEmission = 0,
                                                                .lightFilter = 15};

        // MossyCobblestone: tile 29
        b[static_cast<size_t>(BlockID::MossyCobblestone)] = BlockInfo{.name = "Mossy Cobblestone",
                                                                      .isSolid = true,
                                                                      .isOpaque = true,
                                                                      .isTransparent = false,
                                                                      .textureIndices = {29, 29, 29, 29, 29, 29},
                                                                      .hardness = 2.0f,
                                                                      .requiredTool = ToolType::Pickaxe,
                                                                      .minHarvestTier = ToolTier::Wood,
                                                                      .lightEmission = 0,
                                                                      .lightFilter = 15};

        // MonsterSpawner: tile 30
        b[static_cast<size_t>(BlockID::MonsterSpawner)] = BlockInfo{.name = "Monster Spawner",
                                                                    .isSolid = true,
                                                                    .isOpaque = false,
                                                                    .isTransparent = true,
                                                                    .textureIndices = {30, 30, 30, 30, 30, 30},
                                                                    .hardness = 5.0f,
                                                                    .requiredTool = ToolType::Pickaxe,
                                                                    .minHarvestTier = ToolTier::Wood,
                                                                    .lightEmission = 0,
                                                                    .lightFilter = 1};

        // Composter: tile 31
        b[static_cast<size_t>(BlockID::Composter)] = BlockInfo{.name = "Composter",
                                                               .isSolid = true,
                                                               .isOpaque = true,
                                                               .isTransparent = false,
                                                               .textureIndices = {31, 31, 31, 31, 31, 31},
                                                               .hardness = 2.0f,
                                                               .requiredTool = ToolType::Axe,
                                                               .minHarvestTier = ToolTier::None,
                                                               .lightEmission = 0,
                                                               .lightFilter = 15};

        // Lectern: tile 32
        b[static_cast<size_t>(BlockID::Lectern)] = BlockInfo{.name = "Lectern",
                                                             .isSolid = true,
                                                             .isOpaque = true,
                                                             .isTransparent = false,
                                                             .textureIndices = {32, 32, 32, 32, 32, 32},
                                                             .hardness = 2.0f,
                                                             .requiredTool = ToolType::Axe,
                                                             .minHarvestTier = ToolTier::None,
                                                             .lightEmission = 0,
                                                             .lightFilter = 15};

        // RedstoneWire: tile 33
        b[static_cast<size_t>(BlockID::CinnabarWire)] = BlockInfo{.name = "Cinnabar Wire",
                                                                  .isSolid = false,
                                                                  .isOpaque = false,
                                                                  .isTransparent = true,
                                                                  .textureIndices = {33, 33, 33, 33, 33, 33},
                                                                  .hardness = 0.0f,
                                                                  .requiredTool = ToolType::None,
                                                                  .minHarvestTier = ToolTier::None,
                                                                  .lightEmission = 0,
                                                                  .lightFilter = 1};

        // RedstoneBlock: tile 34
        b[static_cast<size_t>(BlockID::CinnabarBlock)] = BlockInfo{.name = "Block of Cinnabar",
                                                                   .isSolid = true,
                                                                   .isOpaque = true,
                                                                   .isTransparent = false,
                                                                   .textureIndices = {34, 34, 34, 34, 34, 34},
                                                                   .hardness = 5.0f,
                                                                   .requiredTool = ToolType::Pickaxe,
                                                                   .minHarvestTier = ToolTier::Wood,
                                                                   .lightEmission = 7,
                                                                   .lightFilter = 15};

        // RedstoneTorch: tile 35
        b[static_cast<size_t>(BlockID::CinnabarTorch)] = BlockInfo{.name = "Cinnabar Torch",
                                                                   .isSolid = false,
                                                                   .isOpaque = false,
                                                                   .isTransparent = true,
                                                                   .textureIndices = {35, 35, 35, 35, 35, 35},
                                                                   .hardness = 0.0f,
                                                                   .requiredTool = ToolType::None,
                                                                   .minHarvestTier = ToolTier::None,
                                                                   .lightEmission = 7,
                                                                   .lightFilter = 1};

        // RedstoneTorchOff: tile 36
        b[static_cast<size_t>(BlockID::CinnabarTorchOff)] = BlockInfo{.name = "Unlit Cinnabar Torch",
                                                                      .isSolid = false,
                                                                      .isOpaque = false,
                                                                      .isTransparent = true,
                                                                      .textureIndices = {36, 36, 36, 36, 36, 36},
                                                                      .hardness = 0.0f,
                                                                      .requiredTool = ToolType::None,
                                                                      .minHarvestTier = ToolTier::None,
                                                                      .lightEmission = 0,
                                                                      .lightFilter = 1};

        // Repeater: tile 37
        b[static_cast<size_t>(BlockID::Repeater)] = BlockInfo{.name = "Cinnabar Repeater",
                                                              .isSolid = false,
                                                              .isOpaque = false,
                                                              .isTransparent = true,
                                                              .textureIndices = {37, 37, 37, 37, 37, 37},
                                                              .hardness = 0.0f,
                                                              .requiredTool = ToolType::None,
                                                              .minHarvestTier = ToolTier::None,
                                                              .lightEmission = 0,
                                                              .lightFilter = 1};

        // Comparator: tile 38
        b[static_cast<size_t>(BlockID::Comparator)] = BlockInfo{.name = "Cinnabar Comparator",
                                                                .isSolid = false,
                                                                .isOpaque = false,
                                                                .isTransparent = true,
                                                                .textureIndices = {38, 38, 38, 38, 38, 38},
                                                                .hardness = 0.0f,
                                                                .requiredTool = ToolType::None,
                                                                .minHarvestTier = ToolTier::None,
                                                                .lightEmission = 0,
                                                                .lightFilter = 1};

        // Lever: tile 39
        b[static_cast<size_t>(BlockID::Lever)] = BlockInfo{.name = "Lever",
                                                           .isSolid = false,
                                                           .isOpaque = false,
                                                           .isTransparent = true,
                                                           .textureIndices = {39, 39, 39, 39, 39, 39},
                                                           .hardness = 0.5f,
                                                           .requiredTool = ToolType::None,
                                                           .minHarvestTier = ToolTier::None,
                                                           .lightEmission = 0,
                                                           .lightFilter = 1};

        // StoneButton: tile 40
        b[static_cast<size_t>(BlockID::StoneButton)] = BlockInfo{.name = "Stone Button",
                                                                 .isSolid = false,
                                                                 .isOpaque = false,
                                                                 .isTransparent = true,
                                                                 .textureIndices = {40, 40, 40, 40, 40, 40},
                                                                 .hardness = 0.5f,
                                                                 .requiredTool = ToolType::Pickaxe,
                                                                 .minHarvestTier = ToolTier::Wood,
                                                                 .lightEmission = 0,
                                                                 .lightFilter = 1};

        // OakButton: tile 41
        b[static_cast<size_t>(BlockID::OakButton)] = BlockInfo{.name = "Oak Button",
                                                               .isSolid = false,
                                                               .isOpaque = false,
                                                               .isTransparent = true,
                                                               .textureIndices = {41, 41, 41, 41, 41, 41},
                                                               .hardness = 0.5f,
                                                               .requiredTool = ToolType::Axe,
                                                               .minHarvestTier = ToolTier::None,
                                                               .lightEmission = 0,
                                                               .lightFilter = 1};

        // PressurePlate: tile 42
        b[static_cast<size_t>(BlockID::PressurePlate)] = BlockInfo{.name = "Pressure Plate",
                                                                   .isSolid = false,
                                                                   .isOpaque = false,
                                                                   .isTransparent = true,
                                                                   .textureIndices = {42, 42, 42, 42, 42, 42},
                                                                   .hardness = 0.5f,
                                                                   .requiredTool = ToolType::Axe,
                                                                   .minHarvestTier = ToolTier::None,
                                                                   .lightEmission = 0,
                                                                   .lightFilter = 1};

        // DaylightDetector: tile 43
        b[static_cast<size_t>(BlockID::DaylightDetector)] = BlockInfo{.name = "Daylight Detector",
                                                                      .isSolid = true,
                                                                      .isOpaque = false,
                                                                      .isTransparent = true,
                                                                      .textureIndices = {43, 43, 43, 43, 43, 43},
                                                                      .hardness = 0.2f,
                                                                      .requiredTool = ToolType::Axe,
                                                                      .minHarvestTier = ToolTier::None,
                                                                      .lightEmission = 0,
                                                                      .lightFilter = 1};

        // RedstoneLamp: tile 44
        b[static_cast<size_t>(BlockID::CinnabarLamp)] = BlockInfo{.name = "Cinnabar Lamp",
                                                                  .isSolid = true,
                                                                  .isOpaque = true,
                                                                  .isTransparent = false,
                                                                  .textureIndices = {44, 44, 44, 44, 44, 44},
                                                                  .hardness = 0.3f,
                                                                  .requiredTool = ToolType::None,
                                                                  .minHarvestTier = ToolTier::None,
                                                                  .lightEmission = 0,
                                                                  .lightFilter = 15};

        // LitRedstoneLamp: tile 45
        b[static_cast<size_t>(BlockID::LitCinnabarLamp)] = BlockInfo{.name = "Lit Cinnabar Lamp",
                                                                     .isSolid = true,
                                                                     .isOpaque = true,
                                                                     .isTransparent = false,
                                                                     .textureIndices = {45, 45, 45, 45, 45, 45},
                                                                     .hardness = 0.3f,
                                                                     .requiredTool = ToolType::None,
                                                                     .minHarvestTier = ToolTier::None,
                                                                     .lightEmission = 15,
                                                                     .lightFilter = 15};

        // IronDoor: tile 46
        b[static_cast<size_t>(BlockID::IronDoor)] = BlockInfo{.name = "Iron Door",
                                                              .isSolid = true,
                                                              .isOpaque = false,
                                                              .isTransparent = true,
                                                              .textureIndices = {46, 46, 46, 46, 46, 46},
                                                              .hardness = 5.0f,
                                                              .requiredTool = ToolType::Pickaxe,
                                                              .minHarvestTier = ToolTier::Wood,
                                                              .lightEmission = 0,
                                                              .lightFilter = 1};

        // OakDoor: tile 47
        b[static_cast<size_t>(BlockID::OakDoor)] = BlockInfo{.name = "Oak Door",
                                                             .isSolid = true,
                                                             .isOpaque = false,
                                                             .isTransparent = true,
                                                             .textureIndices = {47, 47, 47, 47, 47, 47},
                                                             .hardness = 3.0f,
                                                             .requiredTool = ToolType::Axe,
                                                             .minHarvestTier = ToolTier::None,
                                                             .lightEmission = 0,
                                                             .lightFilter = 1};

        // Piston: tile 48
        b[static_cast<size_t>(BlockID::Piston)] = BlockInfo{.name = "Piston",
                                                            .isSolid = true,
                                                            .isOpaque = true,
                                                            .isTransparent = false,
                                                            .textureIndices = {48, 48, 48, 48, 48, 48},
                                                            .hardness = 1.5f,
                                                            .requiredTool = ToolType::Pickaxe,
                                                            .minHarvestTier = ToolTier::Wood,
                                                            .lightEmission = 0,
                                                            .lightFilter = 15};

        // StickyPiston: tile 49
        b[static_cast<size_t>(BlockID::StickyPiston)] = BlockInfo{.name = "Sticky Piston",
                                                                  .isSolid = true,
                                                                  .isOpaque = true,
                                                                  .isTransparent = false,
                                                                  .textureIndices = {49, 49, 49, 49, 49, 49},
                                                                  .hardness = 1.5f,
                                                                  .requiredTool = ToolType::Pickaxe,
                                                                  .minHarvestTier = ToolTier::Wood,
                                                                  .lightEmission = 0,
                                                                  .lightFilter = 15};

        // PistonHead: tile 50
        b[static_cast<size_t>(BlockID::PistonHead)] = BlockInfo{.name = "Piston Head",
                                                                .isSolid = true,
                                                                .isOpaque = false,
                                                                .isTransparent = true,
                                                                .textureIndices = {50, 50, 50, 50, 50, 50},
                                                                .hardness = 1.5f,
                                                                .requiredTool = ToolType::Pickaxe,
                                                                .minHarvestTier = ToolTier::Wood,
                                                                .lightEmission = 0,
                                                                .lightFilter = 1};

        // Hopper: tile 51
        b[static_cast<size_t>(BlockID::Hopper)] = BlockInfo{.name = "Hopper",
                                                            .isSolid = true,
                                                            .isOpaque = false,
                                                            .isTransparent = true,
                                                            .textureIndices = {51, 51, 51, 51, 51, 51},
                                                            .hardness = 3.0f,
                                                            .requiredTool = ToolType::Pickaxe,
                                                            .minHarvestTier = ToolTier::Wood,
                                                            .lightEmission = 0,
                                                            .lightFilter = 1};

        // Dropper: tile 52
        b[static_cast<size_t>(BlockID::Dropper)] = BlockInfo{.name = "Dropper",
                                                             .isSolid = true,
                                                             .isOpaque = true,
                                                             .isTransparent = false,
                                                             .textureIndices = {52, 52, 52, 52, 52, 52},
                                                             .hardness = 3.5f,
                                                             .requiredTool = ToolType::Pickaxe,
                                                             .minHarvestTier = ToolTier::Wood,
                                                             .lightEmission = 0,
                                                             .lightFilter = 15};

        // Dispenser: tile 53
        b[static_cast<size_t>(BlockID::Dispenser)] = BlockInfo{.name = "Dispenser",
                                                               .isSolid = true,
                                                               .isOpaque = true,
                                                               .isTransparent = false,
                                                               .textureIndices = {53, 53, 53, 53, 53, 53},
                                                               .hardness = 3.5f,
                                                               .requiredTool = ToolType::Pickaxe,
                                                               .minHarvestTier = ToolTier::Wood,
                                                               .lightEmission = 0,
                                                               .lightFilter = 15};

        // Crafter: tile 54
        b[static_cast<size_t>(BlockID::Crafter)] = BlockInfo{.name = "Crafter",
                                                             .isSolid = true,
                                                             .isOpaque = true,
                                                             .isTransparent = false,
                                                             .textureIndices = {54, 54, 54, 54, 54, 54},
                                                             .hardness = 3.5f,
                                                             .requiredTool = ToolType::Pickaxe,
                                                             .minHarvestTier = ToolTier::Wood,
                                                             .lightEmission = 0,
                                                             .lightFilter = 15};

        // Rail: tile 55
        b[static_cast<size_t>(BlockID::Rail)] = BlockInfo{.name = "Rail",
                                                          .isSolid = false,
                                                          .isOpaque = false,
                                                          .isTransparent = true,
                                                          .textureIndices = {55, 55, 55, 55, 55, 55},
                                                          .hardness = 0.7f,
                                                          .requiredTool = ToolType::Pickaxe,
                                                          .minHarvestTier = ToolTier::Wood,
                                                          .lightEmission = 0,
                                                          .lightFilter = 1};

        // PoweredRail: tile 56
        b[static_cast<size_t>(BlockID::PoweredRail)] = BlockInfo{.name = "Powered Rail",
                                                                 .isSolid = false,
                                                                 .isOpaque = false,
                                                                 .isTransparent = true,
                                                                 .textureIndices = {56, 56, 56, 56, 56, 56},
                                                                 .hardness = 0.7f,
                                                                 .requiredTool = ToolType::Pickaxe,
                                                                 .minHarvestTier = ToolTier::Wood,
                                                                 .lightEmission = 0,
                                                                 .lightFilter = 1};

        // DetectorRail: tile 57
        b[static_cast<size_t>(BlockID::DetectorRail)] = BlockInfo{.name = "Detector Rail",
                                                                  .isSolid = false,
                                                                  .isOpaque = false,
                                                                  .isTransparent = true,
                                                                  .textureIndices = {57, 57, 57, 57, 57, 57},
                                                                  .hardness = 0.7f,
                                                                  .requiredTool = ToolType::Pickaxe,
                                                                  .minHarvestTier = ToolTier::Wood,
                                                                  .lightEmission = 0,
                                                                  .lightFilter = 1};

        // ActivatorRail: tile 58
        b[static_cast<size_t>(BlockID::ActivatorRail)] = BlockInfo{.name = "Activator Rail",
                                                                   .isSolid = false,
                                                                   .isOpaque = false,
                                                                   .isTransparent = true,
                                                                   .textureIndices = {58, 58, 58, 58, 58, 58},
                                                                   .hardness = 0.7f,
                                                                   .requiredTool = ToolType::Pickaxe,
                                                                   .minHarvestTier = ToolTier::Wood,
                                                                   .lightEmission = 0,
                                                                   .lightFilter = 1};

        // Roca abisal: tile 59
        b[static_cast<size_t>(BlockID::AbyssalRock)] = BlockInfo{.name = "Abyssal Rock",
                                                                 .isSolid = true,
                                                                 .isOpaque = true,
                                                                 .isTransparent = false,
                                                                 .textureIndices = {59, 59, 59, 59, 59, 59},
                                                                 .hardness = 0.4f,
                                                                 .requiredTool = ToolType::Pickaxe,
                                                                 .minHarvestTier = ToolTier::Wood,
                                                                 .lightEmission = 0,
                                                                 .lightFilter = 15};

        // Soul Sand: tile 60
        b[static_cast<size_t>(BlockID::SoulSand)] = BlockInfo{.name = "Soul Sand",
                                                              .isSolid = true,
                                                              .isOpaque = true,
                                                              .isTransparent = false,
                                                              .textureIndices = {60, 60, 60, 60, 60, 60},
                                                              .hardness = 0.5f,
                                                              .requiredTool = ToolType::Shovel,
                                                              .minHarvestTier = ToolTier::None,
                                                              .lightEmission = 0,
                                                              .lightFilter = 15};

        // Soul Soil: tile 61
        b[static_cast<size_t>(BlockID::SoulSoil)] = BlockInfo{.name = "Soul Soil",
                                                              .isSolid = true,
                                                              .isOpaque = true,
                                                              .isTransparent = false,
                                                              .textureIndices = {61, 61, 61, 61, 61, 61},
                                                              .hardness = 0.5f,
                                                              .requiredTool = ToolType::Shovel,
                                                              .minHarvestTier = ToolTier::None,
                                                              .lightEmission = 0,
                                                              .lightFilter = 15};

        // Abismo Quartz Ore: tile 62
        b[static_cast<size_t>(BlockID::AbyssalQuartzOre)] = BlockInfo{.name = "Abyssal Quartz Ore",
                                                                      .isSolid = true,
                                                                      .isOpaque = true,
                                                                      .isTransparent = false,
                                                                      .textureIndices = {62, 62, 62, 62, 62, 62},
                                                                      .hardness = 3.0f,
                                                                      .requiredTool = ToolType::Pickaxe,
                                                                      .minHarvestTier = ToolTier::Wood,
                                                                      .lightEmission = 0,
                                                                      .lightFilter = 15};

        // Piedra luminosa: tile 63
        b[static_cast<size_t>(BlockID::GlowRock)] = BlockInfo{.name = "Glow Rock",
                                                              .isSolid = true,
                                                              .isOpaque = true,
                                                              .isTransparent = false,
                                                              .textureIndices = {63, 63, 63, 63, 63, 63},
                                                              .hardness = 0.3f,
                                                              .requiredTool = ToolType::None,
                                                              .minHarvestTier = ToolTier::None,
                                                              .lightEmission = 15,
                                                              .lightFilter = 15};

        // Ancient Debris: tile 64
        b[static_cast<size_t>(BlockID::OrichalcumVein)] = BlockInfo{.name = "Orichalcum Vein",
                                                                    .isSolid = true,
                                                                    .isOpaque = true,
                                                                    .isTransparent = false,
                                                                    .textureIndices = {64, 64, 64, 64, 64, 64},
                                                                    .hardness = 30.0f,
                                                                    .requiredTool = ToolType::Pickaxe,
                                                                    .minHarvestTier = ToolTier::Diamond,
                                                                    .lightEmission = 0,
                                                                    .lightFilter = 15};

        // Piedra del Confín: tile 65
        b[static_cast<size_t>(BlockID::RimStone)] = BlockInfo{.name = "Rim Stone",
                                                              .isSolid = true,
                                                              .isOpaque = true,
                                                              .isTransparent = false,
                                                              .textureIndices = {65, 65, 65, 65, 65, 65},
                                                              .hardness = 3.0f,
                                                              .requiredTool = ToolType::Pickaxe,
                                                              .minHarvestTier = ToolTier::Wood,
                                                              .lightEmission = 0,
                                                              .lightFilter = 15};

        // Obsidian Pillar: tile 21
        b[static_cast<size_t>(BlockID::ObsidianPillar)] = BlockInfo{.name = "Obsidian Pillar",
                                                                    .isSolid = true,
                                                                    .isOpaque = true,
                                                                    .isTransparent = false,
                                                                    .textureIndices = {21, 21, 21, 21, 21, 21},
                                                                    .hardness = 50.0f,
                                                                    .requiredTool = ToolType::Pickaxe,
                                                                    .minHarvestTier = ToolTier::Diamond,
                                                                    .lightEmission = 0,
                                                                    .lightFilter = 15};

        // Brewing Stand: tile 66
        b[static_cast<size_t>(BlockID::BrewingStand)] = BlockInfo{.name = "Brewing Stand",
                                                                  .isSolid = false,
                                                                  .isOpaque = false,
                                                                  .isTransparent = true,
                                                                  .textureIndices = {66, 66, 66, 66, 66, 66},
                                                                  .hardness = 0.5f,
                                                                  .requiredTool = ToolType::Pickaxe,
                                                                  .minHarvestTier = ToolTier::None,
                                                                  .lightEmission = 0,
                                                                  .lightFilter = 1};

        // Beacon: tile 67
        b[static_cast<size_t>(BlockID::BeaconBlock)] = BlockInfo{.name = "Beacon",
                                                                 .isSolid = true,
                                                                 .isOpaque = false,
                                                                 .isTransparent = true,
                                                                 .textureIndices = {67, 67, 67, 67, 67, 67},
                                                                 .hardness = 3.0f,
                                                                 .requiredTool = ToolType::None,
                                                                 .minHarvestTier = ToolTier::None,
                                                                 .lightEmission = 15,
                                                                 .lightFilter = 1};

        // Abismo Portal: tile 68
        b[static_cast<size_t>(BlockID::AbyssPortal)] = BlockInfo{.name = "Abyss Portal",
                                                                 .isSolid = false,
                                                                 .isOpaque = false,
                                                                 .isTransparent = true,
                                                                 .textureIndices = {68, 68, 68, 68, 68, 68},
                                                                 .hardness = -1.0f,
                                                                 .requiredTool = ToolType::None,
                                                                 .minHarvestTier = ToolTier::None,
                                                                 .lightEmission = 11,
                                                                 .lightFilter = 1};

        // Rim Portal Frame: tile 69
        b[static_cast<size_t>(BlockID::RimPortalFrame)] = BlockInfo{.name = "Rim Portal Frame",
                                                                    .isSolid = true,
                                                                    .isOpaque = true,
                                                                    .isTransparent = false,
                                                                    .textureIndices = {69, 69, 69, 69, 69, 69},
                                                                    .hardness = -1.0f,
                                                                    .requiredTool = ToolType::None,
                                                                    .minHarvestTier = ToolTier::None,
                                                                    .lightEmission = 1,
                                                                    .lightFilter = 15};

        // End Portal: tile 70
        b[static_cast<size_t>(BlockID::RimPortal)] = BlockInfo{.name = "Rim Portal",
                                                               .isSolid = false,
                                                               .isOpaque = false,
                                                               .isTransparent = true,
                                                               .textureIndices = {70, 70, 70, 70, 70, 70},
                                                               .hardness = -1.0f,
                                                               .requiredTool = ToolType::None,
                                                               .minHarvestTier = ToolTier::None,
                                                               .lightEmission = 15,
                                                               .lightFilter = 1};

        // Dragon Egg: tile 71
        b[static_cast<size_t>(BlockID::DragonEgg)] = BlockInfo{.name = "Dragon Egg",
                                                               .isSolid = true,
                                                               .isOpaque = true,
                                                               .isTransparent = false,
                                                               .textureIndices = {71, 71, 71, 71, 71, 71},
                                                               .hardness = 3.0f,
                                                               .requiredTool = ToolType::None,
                                                               .minHarvestTier = ToolTier::None,
                                                               .lightEmission = 1,
                                                               .lightFilter = 15};

        // Command Block: tile 72
        b[static_cast<size_t>(BlockID::CommandBlock)] = BlockInfo{.name = "Command Block",
                                                                  .isSolid = true,
                                                                  .isOpaque = true,
                                                                  .isTransparent = false,
                                                                  .textureIndices = {72, 72, 72, 72, 72, 72},
                                                                  .hardness = -1.0f,
                                                                  .requiredTool = ToolType::None,
                                                                  .minHarvestTier = ToolTier::None,
                                                                  .lightEmission = 0,
                                                                  .lightFilter = 15};

        // Repeating Command Block: tile 73
        b[static_cast<size_t>(BlockID::RepeatingCommandBlock)] = BlockInfo{.name = "Repeating Command Block",
                                                                           .isSolid = true,
                                                                           .isOpaque = true,
                                                                           .isTransparent = false,
                                                                           .textureIndices = {73, 73, 73, 73, 73, 73},
                                                                           .hardness = -1.0f,
                                                                           .requiredTool = ToolType::None,
                                                                           .minHarvestTier = ToolTier::None,
                                                                           .lightEmission = 0,
                                                                           .lightFilter = 15};

        // Chain Command Block: tile 74
        b[static_cast<size_t>(BlockID::ChainCommandBlock)] = BlockInfo{.name = "Chain Command Block",
                                                                       .isSolid = true,
                                                                       .isOpaque = true,
                                                                       .isTransparent = false,
                                                                       .textureIndices = {74, 74, 74, 74, 74, 74},
                                                                       .hardness = -1.0f,
                                                                       .requiredTool = ToolType::None,
                                                                       .minHarvestTier = ToolTier::None,
                                                                       .lightEmission = 0,
                                                                       .lightFilter = 15};

        // Copper Block: tile 75
        b[static_cast<size_t>(BlockID::CopperBlock)] = BlockInfo{.name = "Copper Block",
                                                                 .isSolid = true,
                                                                 .isOpaque = true,
                                                                 .isTransparent = false,
                                                                 .textureIndices = {75, 75, 75, 75, 75, 75},
                                                                 .hardness = 3.0f,
                                                                 .requiredTool = ToolType::Pickaxe,
                                                                 .minHarvestTier = ToolTier::Stone,
                                                                 .lightEmission = 0,
                                                                 .lightFilter = 15};

        // Cut Copper: tile 76
        b[static_cast<size_t>(BlockID::CutCopper)] = BlockInfo{.name = "Cut Copper",
                                                               .isSolid = true,
                                                               .isOpaque = true,
                                                               .isTransparent = false,
                                                               .textureIndices = {76, 76, 76, 76, 76, 76},
                                                               .hardness = 3.0f,
                                                               .requiredTool = ToolType::Pickaxe,
                                                               .minHarvestTier = ToolTier::Stone,
                                                               .lightEmission = 0,
                                                               .lightFilter = 15};

        // Chiseled Copper: tile 77
        b[static_cast<size_t>(BlockID::ChiseledCopper)] = BlockInfo{.name = "Chiseled Copper",
                                                                    .isSolid = true,
                                                                    .isOpaque = true,
                                                                    .isTransparent = false,
                                                                    .textureIndices = {77, 77, 77, 77, 77, 77},
                                                                    .hardness = 3.0f,
                                                                    .requiredTool = ToolType::Pickaxe,
                                                                    .minHarvestTier = ToolTier::Stone,
                                                                    .lightEmission = 0,
                                                                    .lightFilter = 15};

        // Copper Grate: tile 78
        b[static_cast<size_t>(BlockID::CopperGrate)] = BlockInfo{.name = "Copper Grate",
                                                                 .isSolid = true,
                                                                 .isOpaque = false,
                                                                 .isTransparent = true,
                                                                 .textureIndices = {78, 78, 78, 78, 78, 78},
                                                                 .hardness = 3.0f,
                                                                 .requiredTool = ToolType::Pickaxe,
                                                                 .minHarvestTier = ToolTier::Stone,
                                                                 .lightEmission = 0,
                                                                 .lightFilter = 1};

        // Copper Bulb: tile 79
        b[static_cast<size_t>(BlockID::CopperBulb)] = BlockInfo{.name = "Copper Bulb",
                                                                .isSolid = true,
                                                                .isOpaque = true,
                                                                .isTransparent = false,
                                                                .textureIndices = {79, 79, 79, 79, 79, 79},
                                                                .hardness = 3.0f,
                                                                .requiredTool = ToolType::Pickaxe,
                                                                .minHarvestTier = ToolTier::Stone,
                                                                .lightEmission = 15,
                                                                .lightFilter = 15};

        // Tuff: tile 80
        b[static_cast<size_t>(BlockID::Tuff)] = BlockInfo{.name = "Tuff",
                                                          .isSolid = true,
                                                          .isOpaque = true,
                                                          .isTransparent = false,
                                                          .textureIndices = {80, 80, 80, 80, 80, 80},
                                                          .hardness = 1.5f,
                                                          .requiredTool = ToolType::Pickaxe,
                                                          .minHarvestTier = ToolTier::Wood,
                                                          .lightEmission = 0,
                                                          .lightFilter = 15};

        // Tuff Bricks: tile 81
        b[static_cast<size_t>(BlockID::TuffBricks)] = BlockInfo{.name = "Tuff Bricks",
                                                                .isSolid = true,
                                                                .isOpaque = true,
                                                                .isTransparent = false,
                                                                .textureIndices = {81, 81, 81, 81, 81, 81},
                                                                .hardness = 1.5f,
                                                                .requiredTool = ToolType::Pickaxe,
                                                                .minHarvestTier = ToolTier::Wood,
                                                                .lightEmission = 0,
                                                                .lightFilter = 15};

        // Chiseled Tuff: tile 82
        b[static_cast<size_t>(BlockID::ChiseledTuff)] = BlockInfo{.name = "Chiseled Tuff",
                                                                  .isSolid = true,
                                                                  .isOpaque = true,
                                                                  .isTransparent = false,
                                                                  .textureIndices = {82, 82, 82, 82, 82, 82},
                                                                  .hardness = 1.5f,
                                                                  .requiredTool = ToolType::Pickaxe,
                                                                  .minHarvestTier = ToolTier::Wood,
                                                                  .lightEmission = 0,
                                                                  .lightFilter = 15};

        // Trial Spawner: tile 83
        b[static_cast<size_t>(BlockID::TrialSpawner)] = BlockInfo{.name = "Trial Spawner",
                                                                  .isSolid = true,
                                                                  .isOpaque = true,
                                                                  .isTransparent = false,
                                                                  .textureIndices = {83, 83, 83, 83, 83, 83},
                                                                  .hardness = 50.0f,
                                                                  .requiredTool = ToolType::Pickaxe,
                                                                  .minHarvestTier = ToolTier::Iron,
                                                                  .lightEmission = 7,
                                                                  .lightFilter = 15};

        // Vault: tile 84
        b[static_cast<size_t>(BlockID::Vault)] = BlockInfo{.name = "Vault",
                                                           .isSolid = true,
                                                           .isOpaque = true,
                                                           .isTransparent = false,
                                                           .textureIndices = {84, 84, 84, 84, 84, 84},
                                                           .hardness = 50.0f,
                                                           .requiredTool = ToolType::Pickaxe,
                                                           .minHarvestTier = ToolTier::Iron,
                                                           .lightEmission = 6,
                                                           .lightFilter = 15};

        // Sign: tile 85
        b[static_cast<size_t>(BlockID::Sign)] = BlockInfo{.name = "Sign",
                                                          .isSolid = false,
                                                          .isOpaque = false,
                                                          .isTransparent = true,
                                                          .textureIndices = {85, 85, 85, 85, 85, 85},
                                                          .hardness = 1.0f,
                                                          .requiredTool = ToolType::Axe,
                                                          .minHarvestTier = ToolTier::None,
                                                          .lightEmission = 0,
                                                          .lightFilter = 1};

        // Decorated Pot: tile 86
        b[static_cast<size_t>(BlockID::DecoratedPot)] = BlockInfo{.name = "Decorated Pot",
                                                                  .isSolid = true,
                                                                  .isOpaque = false,
                                                                  .isTransparent = true,
                                                                  .textureIndices = {86, 86, 86, 86, 86, 86},
                                                                  .hardness = 0.5f,
                                                                  .requiredTool = ToolType::None,
                                                                  .minHarvestTier = ToolTier::None,
                                                                  .lightEmission = 0,
                                                                  .lightFilter = 1};

        return b;
    }();
};

} // namespace OpenCraft

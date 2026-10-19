#include "crafting/RecipeRegistry.hpp"
#include "item/Item.hpp"
#include <algorithm>
#include <map>

namespace OpenCraft {

RecipeRegistry::RecipeRegistry() { reset(); }

RecipeRegistry& RecipeRegistry::global() {
    static RecipeRegistry instance;
    return instance;
}

void RecipeRegistry::reset() {
    m_shapedRecipes.clear();
    m_shapelessRecipes.clear();
    m_smeltingRecipes.clear();
    registerDefaults();
}

void RecipeRegistry::registerShaped(ShapedRecipe recipe) { m_shapedRecipes.push_back(std::move(recipe)); }

void RecipeRegistry::registerShapeless(ShapelessRecipe recipe) { m_shapelessRecipes.push_back(std::move(recipe)); }

void RecipeRegistry::registerSmelting(SmeltingRecipe recipe) { m_smeltingRecipes.push_back(std::move(recipe)); }

bool RecipeRegistry::matchShaped(const ShapedRecipe& recipe, std::span<const ItemStack> grid, int gridWidth,
                                 int gridHeight) const {
    if (recipe.width > gridWidth || recipe.height > gridHeight) {
        return false;
    }

    int maxOffsetX = gridWidth - recipe.width;
    int maxOffsetY = gridHeight - recipe.height;

    // Probar cada desplazamiento posible en la cuadrícula
    for (int offY = 0; offY <= maxOffsetY; ++offY) {
        for (int offX = 0; offX <= maxOffsetX; ++offX) {
            // Probar orientación normal y espejo horizontal
            for (bool mirrored : {false, true}) {
                bool matches = true;

                for (int gy = 0; gy < gridHeight && matches; ++gy) {
                    for (int gx = 0; gx < gridWidth; ++gx) {
                        size_t gridIdx = static_cast<size_t>(gy * gridWidth + gx);
                        const auto& gridStack = grid[gridIdx];
                        ItemId gridItemId = gridStack.isEmpty() ? EMPTY_ITEM_ID : gridStack.itemId;

                        bool inside =
                            (gx >= offX && gx < offX + recipe.width && gy >= offY && gy < offY + recipe.height);

                        if (inside) {
                            int rx = gx - offX;
                            if (mirrored) {
                                rx = recipe.width - 1 - rx;
                            }
                            int ry = gy - offY;
                            ItemId expectedId = recipe.pattern[static_cast<size_t>(ry * recipe.width + rx)];

                            if (gridItemId != expectedId) {
                                matches = false;
                                break;
                            }
                        } else {
                            if (gridItemId != EMPTY_ITEM_ID) {
                                matches = false;
                                break;
                            }
                        }
                    }
                }

                if (matches) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool RecipeRegistry::matchShapeless(const ShapelessRecipe& recipe, std::span<const ItemStack> grid) const {
    std::vector<ItemId> gridItems;
    for (const auto& stack : grid) {
        if (!stack.isEmpty()) {
            gridItems.push_back(stack.itemId);
        }
    }

    if (gridItems.size() != recipe.ingredients.size()) {
        return false;
    }

    std::vector<ItemId> sortedRecipe = recipe.ingredients;
    std::sort(gridItems.begin(), gridItems.end());
    std::sort(sortedRecipe.begin(), sortedRecipe.end());

    return gridItems == sortedRecipe;
}

ItemStack RecipeRegistry::matchCrafting(std::span<const ItemStack> grid, int gridWidth, int gridHeight,
                                        std::string* outRecipeId) const {
    // 1. Recetas con forma
    for (const auto& recipe : m_shapedRecipes) {
        if (matchShaped(recipe, grid, gridWidth, gridHeight)) {
            if (outRecipeId) *outRecipeId = recipe.id;
            return recipe.result;
        }
    }

    // 2. Recetas sin forma
    for (const auto& recipe : m_shapelessRecipes) {
        if (matchShapeless(recipe, grid)) {
            if (outRecipeId) *outRecipeId = recipe.id;
            return recipe.result;
        }
    }

    if (outRecipeId) outRecipeId->clear();
    return ItemStack();
}

bool RecipeRegistry::craft(std::vector<ItemStack>& grid, int gridWidth, int gridHeight, ItemStack& outResult,
                           bool craftAll) const {
    std::string recipeId;
    ItemStack match = matchCrafting(grid, gridWidth, gridHeight, &recipeId);
    if (match.isEmpty()) {
        outResult.clear();
        return false;
    }

    uint16_t multiplier = 1;

    if (craftAll) {
        // Encontrar la menor cantidad disponible en las ranuras no vacías
        uint16_t minCount = 64;
        for (const auto& stack : grid) {
            if (!stack.isEmpty() && stack.count < minCount) {
                minCount = stack.count;
            }
        }
        multiplier = (minCount > 0) ? minCount : 1;
    }

    // Deducir ingredientes
    for (auto& stack : grid) {
        if (!stack.isEmpty()) {
            if (stack.count >= multiplier) {
                stack.count -= multiplier;
                if (stack.count == 0) {
                    stack.clear();
                }
            } else {
                stack.clear();
            }
        }
    }

    outResult = match;
    outResult.count = match.count * multiplier;
    return true;
}

const SmeltingRecipe* RecipeRegistry::findSmeltingRecipe(ItemId inputItemId) const {
    for (const auto& recipe : m_smeltingRecipes) {
        if (recipe.inputItemId == inputItemId) {
            return &recipe;
        }
    }
    return nullptr;
}

bool RecipeRegistry::isFuel(ItemId itemId) const {
    const auto& def = ItemRegistry::global().get(itemId);
    return def.fuelBurnTicks > 0;
}

uint32_t RecipeRegistry::getFuelBurnTicks(ItemId itemId) const {
    const auto& def = ItemRegistry::global().get(itemId);
    return def.fuelBurnTicks;
}

void RecipeRegistry::registerDefaults() {
    auto& itemReg = ItemRegistry::global();

    ItemId woodLogId = itemReg.getId("opencraft:wood_log");
    ItemId planksId = itemReg.getId("opencraft:planks");
    ItemId stickId = itemReg.getId("opencraft:stick");
    ItemId craftingTableId = itemReg.getId("opencraft:crafting_table");
    ItemId stoneId = itemReg.getId("opencraft:stone");
    ItemId cobblestoneId = itemReg.getId("opencraft:cobblestone");
    ItemId furnaceId = itemReg.getId("opencraft:furnace");
    ItemId coalId = itemReg.getId("opencraft:coal");
    ItemId rawIronId = itemReg.getId("opencraft:raw_iron");
    ItemId ironIngotId = itemReg.getId("opencraft:iron_ingot");
    ItemId rawBeefId = itemReg.getId("opencraft:raw_beef");
    ItemId cookedBeefId = itemReg.getId("opencraft:cooked_beef");

    ItemId woodenPickaxeId = itemReg.getId("opencraft:wooden_pickaxe");
    ItemId woodenAxeId = itemReg.getId("opencraft:wooden_axe");
    ItemId woodenShovelId = itemReg.getId("opencraft:wooden_shovel");
    ItemId woodenSwordId = itemReg.getId("opencraft:wooden_sword");

    ItemId stonePickaxeId = itemReg.getId("opencraft:stone_pickaxe");
    ItemId stoneAxeId = itemReg.getId("opencraft:stone_axe");
    ItemId stoneShovelId = itemReg.getId("opencraft:stone_shovel");
    ItemId stoneSwordId = itemReg.getId("opencraft:stone_sword");

    ItemId ironPickaxeId = itemReg.getId("opencraft:iron_pickaxe");
    ItemId ironAxeId = itemReg.getId("opencraft:iron_axe");
    ItemId ironShovelId = itemReg.getId("opencraft:iron_shovel");
    ItemId ironSwordId = itemReg.getId("opencraft:iron_sword");

    // Mechero: 1 Lingote de hierro + 1 Pedernal (Sin forma)
    ItemId flintId = itemReg.getId("opencraft:flint");
    ItemId flintAndSteelId = itemReg.getId("opencraft:flint_and_steel");
    if (ironIngotId != EMPTY_ITEM_ID && flintId != EMPTY_ITEM_ID && flintAndSteelId != EMPTY_ITEM_ID) {
        registerShapeless(ShapelessRecipe{.id = "opencraft:flint_and_steel",
                                          .ingredients = {ironIngotId, flintId},
                                          .result = ItemStack(flintAndSteelId, 1)});
    }

    // 1. Tronco -> 4 Planks (Sin forma)
    if (woodLogId != EMPTY_ITEM_ID && planksId != EMPTY_ITEM_ID) {
        registerShapeless(ShapelessRecipe{
            .id = "opencraft:planks_from_log", .ingredients = {woodLogId}, .result = ItemStack(planksId, 4)});
    }

    // 2. 4 Planks (2x2) -> Mesa de Trabajo
    if (planksId != EMPTY_ITEM_ID && craftingTableId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:crafting_table",
                                    .width = 2,
                                    .height = 2,
                                    .pattern = {planksId, planksId, planksId, planksId},
                                    .result = ItemStack(craftingTableId, 1)});
    }

    // 3. 2 Planks verticales (1x2) -> 4 Palos
    if (planksId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:stick_from_planks",
                                    .width = 1,
                                    .height = 2,
                                    .pattern = {planksId, planksId},
                                    .result = ItemStack(stickId, 4)});
    }

    // 4. Pico de madera (3x3: 3 tablones arriba, 2 palos al centro)
    if (planksId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID && woodenPickaxeId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:wooden_pickaxe",
                                    .width = 3,
                                    .height = 3,
                                    .pattern = {planksId, planksId, planksId, EMPTY_ITEM_ID, stickId, EMPTY_ITEM_ID,
                                                EMPTY_ITEM_ID, stickId, EMPTY_ITEM_ID},
                                    .result = ItemStack(woodenPickaxeId, 1)});
    }

    // 5. 8 Adoquines en anillo (3x3) -> Horno
    if (cobblestoneId != EMPTY_ITEM_ID && furnaceId != EMPTY_ITEM_ID) {
        registerShaped(
            ShapedRecipe{.id = "opencraft:furnace",
                         .width = 3,
                         .height = 3,
                         .pattern = {cobblestoneId, cobblestoneId, cobblestoneId, cobblestoneId, EMPTY_ITEM_ID,
                                     cobblestoneId, cobblestoneId, cobblestoneId, cobblestoneId},
                         .result = ItemStack(furnaceId, 1)});
    }

    // 6. Pico de piedra (3x3: 3 cobblestone + 2 sticks)
    if (cobblestoneId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID && stonePickaxeId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:stone_pickaxe",
                                    .width = 3,
                                    .height = 3,
                                    .pattern = {cobblestoneId, cobblestoneId, cobblestoneId, EMPTY_ITEM_ID, stickId,
                                                EMPTY_ITEM_ID, EMPTY_ITEM_ID, stickId, EMPTY_ITEM_ID},
                                    .result = ItemStack(stonePickaxeId, 1)});
    }

    // 7. Pico de hierro (3x3: 3 iron ingots + 2 sticks)
    if (ironIngotId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID && ironPickaxeId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:iron_pickaxe",
                                    .width = 3,
                                    .height = 3,
                                    .pattern = {ironIngotId, ironIngotId, ironIngotId, EMPTY_ITEM_ID, stickId,
                                                EMPTY_ITEM_ID, EMPTY_ITEM_ID, stickId, EMPTY_ITEM_ID},
                                    .result = ItemStack(ironPickaxeId, 1)});
    }

    // 8. Hachas, palas y espadas
    if (woodenSwordId != EMPTY_ITEM_ID && planksId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:wooden_sword",
                                    .width = 1,
                                    .height = 3,
                                    .pattern = {planksId, planksId, stickId},
                                    .result = ItemStack(woodenSwordId, 1)});
    }
    if (woodenAxeId != EMPTY_ITEM_ID && planksId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:wooden_axe",
                                    .width = 2,
                                    .height = 3,
                                    .pattern = {planksId, planksId, planksId, stickId, EMPTY_ITEM_ID, stickId},
                                    .result = ItemStack(woodenAxeId, 1)});
    }
    if (woodenShovelId != EMPTY_ITEM_ID && planksId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:wooden_shovel",
                                    .width = 1,
                                    .height = 3,
                                    .pattern = {planksId, stickId, stickId},
                                    .result = ItemStack(woodenShovelId, 1)});
    }

    if (stoneSwordId != EMPTY_ITEM_ID && cobblestoneId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:stone_sword",
                                    .width = 1,
                                    .height = 3,
                                    .pattern = {cobblestoneId, cobblestoneId, stickId},
                                    .result = ItemStack(stoneSwordId, 1)});
    }
    if (stoneAxeId != EMPTY_ITEM_ID && cobblestoneId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID) {
        registerShaped(
            ShapedRecipe{.id = "opencraft:stone_axe",
                         .width = 2,
                         .height = 3,
                         .pattern = {cobblestoneId, cobblestoneId, cobblestoneId, stickId, EMPTY_ITEM_ID, stickId},
                         .result = ItemStack(stoneAxeId, 1)});
    }
    if (stoneShovelId != EMPTY_ITEM_ID && cobblestoneId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:stone_shovel",
                                    .width = 1,
                                    .height = 3,
                                    .pattern = {cobblestoneId, stickId, stickId},
                                    .result = ItemStack(stoneShovelId, 1)});
    }

    if (ironSwordId != EMPTY_ITEM_ID && ironIngotId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:iron_sword",
                                    .width = 1,
                                    .height = 3,
                                    .pattern = {ironIngotId, ironIngotId, stickId},
                                    .result = ItemStack(ironSwordId, 1)});
    }
    if (ironAxeId != EMPTY_ITEM_ID && ironIngotId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:iron_axe",
                                    .width = 2,
                                    .height = 3,
                                    .pattern = {ironIngotId, ironIngotId, ironIngotId, stickId, EMPTY_ITEM_ID, stickId},
                                    .result = ItemStack(ironAxeId, 1)});
    }
    if (ironShovelId != EMPTY_ITEM_ID && ironIngotId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:iron_shovel",
                                    .width = 1,
                                    .height = 3,
                                    .pattern = {ironIngotId, stickId, stickId},
                                    .result = ItemStack(ironShovelId, 1)});
    }

    // Cubeta (Bucket): 3 lingotes de hierro en forma de V
    ItemId bucketId = itemReg.getId("opencraft:bucket");
    if (bucketId != EMPTY_ITEM_ID && ironIngotId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{
            .id = "opencraft:bucket",
            .width = 3,
            .height = 2,
            .pattern = {ironIngotId, EMPTY_ITEM_ID, ironIngotId, EMPTY_ITEM_ID, ironIngotId, EMPTY_ITEM_ID},
            .result = ItemStack(bucketId, 1)});
    }

    // Antorchas (Torch): 1 Carbón sobre 1 Palo -> 4 Antorchas
    ItemId torchId = itemReg.getId("opencraft:torch");
    if (torchId != EMPTY_ITEM_ID && coalId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:torch",
                                    .width = 1,
                                    .height = 2,
                                    .pattern = {coalId, stickId},
                                    .result = ItemStack(torchId, 4)});
    }

    // ==========================================
    // Fase 11: Recetas de Automatización
    // ==========================================
    ItemId cinnabarId = itemReg.getId("opencraft:cinnabar");
    ItemId cinnabarBlockId = itemReg.getId("opencraft:cinnabar_block");
    ItemId cinnabarTorchId = itemReg.getId("opencraft:cinnabar_torch");
    ItemId leverId = itemReg.getId("opencraft:lever");
    ItemId stoneButtonId = itemReg.getId("opencraft:stone_button");
    ItemId oakButtonId = itemReg.getId("opencraft:oak_button");
    ItemId pressurePlateId = itemReg.getId("opencraft:pressure_plate");
    ItemId pistonId = itemReg.getId("opencraft:piston");
    ItemId stickyPistonId = itemReg.getId("opencraft:sticky_piston");
    ItemId hopperId = itemReg.getId("opencraft:hopper");
    ItemId dropperId = itemReg.getId("opencraft:dropper");
    ItemId dispenserId = itemReg.getId("opencraft:dispenser");
    ItemId crafterId = itemReg.getId("opencraft:crafter");
    ItemId railId = itemReg.getId("opencraft:rail");
    ItemId poweredRailId = itemReg.getId("opencraft:powered_rail");
    ItemId minecartId = itemReg.getId("opencraft:minecart");
    ItemId chestId = itemReg.getId("opencraft:chest");
    ItemId chestMinecartId = itemReg.getId("opencraft:chest_minecart");
    ItemId hopperMinecartId = itemReg.getId("opencraft:hopper_minecart");

    // 9 Cinnabar Dust -> Bloque de Cinabrio
    if (cinnabarId != EMPTY_ITEM_ID && cinnabarBlockId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:cinnabar_block",
                                    .width = 3,
                                    .height = 3,
                                    .pattern = {cinnabarId, cinnabarId, cinnabarId, cinnabarId, cinnabarId, cinnabarId,
                                                cinnabarId, cinnabarId, cinnabarId},
                                    .result = ItemStack(cinnabarBlockId, 1)});

        // 1 Bloque de Cinabrio -> 9 Cinabrio Dust
        registerShapeless(ShapelessRecipe{.id = "opencraft:redstone_from_block",
                                          .ingredients = {cinnabarBlockId},
                                          .result = ItemStack(cinnabarId, 9)});
    }

    // Antorcha de Cinabrio: 1 Cinabrio sobre 1 Palo
    if (cinnabarId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID && cinnabarTorchId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:cinnabar_torch",
                                    .width = 1,
                                    .height = 2,
                                    .pattern = {cinnabarId, stickId},
                                    .result = ItemStack(cinnabarTorchId, 1)});
    }

    // Palanca: 1 Palo sobre 1 Adoquín
    if (stickId != EMPTY_ITEM_ID && cobblestoneId != EMPTY_ITEM_ID && leverId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:lever",
                                    .width = 1,
                                    .height = 2,
                                    .pattern = {stickId, cobblestoneId},
                                    .result = ItemStack(leverId, 1)});
    }

    // Botón de piedra y de roble
    if (stoneId != EMPTY_ITEM_ID && stoneButtonId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:stone_button",
                                    .width = 1,
                                    .height = 1,
                                    .pattern = {stoneId},
                                    .result = ItemStack(stoneButtonId, 1)});
    }
    if (planksId != EMPTY_ITEM_ID && oakButtonId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:oak_button",
                                    .width = 1,
                                    .height = 1,
                                    .pattern = {planksId},
                                    .result = ItemStack(oakButtonId, 1)});
    }

    // Placa de presión (2 tablones)
    if (planksId != EMPTY_ITEM_ID && pressurePlateId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:pressure_plate",
                                    .width = 2,
                                    .height = 1,
                                    .pattern = {planksId, planksId},
                                    .result = ItemStack(pressurePlateId, 1)});
    }

    // Pistón (3 tablones, 4 adoquines, 1 lingote de hierro, 1 cinabrio)
    if (planksId != EMPTY_ITEM_ID && cobblestoneId != EMPTY_ITEM_ID && ironIngotId != EMPTY_ITEM_ID &&
        cinnabarId != EMPTY_ITEM_ID && pistonId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:piston",
                                    .width = 3,
                                    .height = 3,
                                    .pattern = {planksId, planksId, planksId, cobblestoneId, ironIngotId, cobblestoneId,
                                                cobblestoneId, cinnabarId, cobblestoneId},
                                    .result = ItemStack(pistonId, 1)});
    }

    // Tolva (5 lingotes de hierro + 1 cofre)
    if (ironIngotId != EMPTY_ITEM_ID && chestId != EMPTY_ITEM_ID && hopperId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:hopper",
                                    .width = 3,
                                    .height = 3,
                                    .pattern = {ironIngotId, EMPTY_ITEM_ID, ironIngotId, ironIngotId, chestId,
                                                ironIngotId, EMPTY_ITEM_ID, ironIngotId, EMPTY_ITEM_ID},
                                    .result = ItemStack(hopperId, 1)});
    }

    // Soltador (7 adoquines + 1 cinabrio)
    if (cobblestoneId != EMPTY_ITEM_ID && cinnabarId != EMPTY_ITEM_ID && dropperId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:dropper",
                                    .width = 3,
                                    .height = 3,
                                    .pattern = {cobblestoneId, cobblestoneId, cobblestoneId, cobblestoneId,
                                                EMPTY_ITEM_ID, cobblestoneId, cobblestoneId, cinnabarId, cobblestoneId},
                                    .result = ItemStack(dropperId, 1)});
    }

    // Dispensador (7 adoquines, 1 arco y 1 cinabrio)
    {
        ItemId bowId = itemReg.getId("opencraft:bow");
        if (cobblestoneId != EMPTY_ITEM_ID && cinnabarId != EMPTY_ITEM_ID && bowId != EMPTY_ITEM_ID &&
            dispenserId != EMPTY_ITEM_ID) {
            registerShaped(ShapedRecipe{.id = "opencraft:dispenser",
                                        .width = 3,
                                        .height = 3,
                                        .pattern = {cobblestoneId, cobblestoneId, cobblestoneId, cobblestoneId, bowId,
                                                    cobblestoneId, cobblestoneId, cinnabarId, cobblestoneId},
                                        .result = ItemStack(dispenserId, 1)});
        }
    }

    // Raíl propulsor (6 lingotes de oro, 1 palo y 1 cinabrio -> 6)
    {
        ItemId goldIngotId = itemReg.getId("opencraft:gold_ingot");
        ItemId stickItemId = itemReg.getId("opencraft:stick");
        if (goldIngotId != EMPTY_ITEM_ID && stickItemId != EMPTY_ITEM_ID && cinnabarId != EMPTY_ITEM_ID &&
            poweredRailId != EMPTY_ITEM_ID) {
            registerShaped(ShapedRecipe{.id = "opencraft:powered_rail",
                                        .width = 3,
                                        .height = 3,
                                        .pattern = {goldIngotId, EMPTY_ITEM_ID, goldIngotId, goldIngotId, stickItemId,
                                                    goldIngotId, goldIngotId, cinnabarId, goldIngotId},
                                        .result = ItemStack(poweredRailId, 6)});
        }
    }

    // El pistón pegajoso necesita bola de slime, que aún no existe en el registro
    (void)stickyPistonId;

    // Crafter (5 lingotes de hierro, 1 mesa de crafteo, 2 cinabrio, 1 dropper)
    if (ironIngotId != EMPTY_ITEM_ID && craftingTableId != EMPTY_ITEM_ID && cinnabarId != EMPTY_ITEM_ID &&
        dropperId != EMPTY_ITEM_ID && crafterId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:crafter",
                                    .width = 3,
                                    .height = 3,
                                    .pattern = {ironIngotId, ironIngotId, ironIngotId, ironIngotId, craftingTableId,
                                                ironIngotId, cinnabarId, dropperId, cinnabarId},
                                    .result = ItemStack(crafterId, 1)});
    }

    // Rieles (6 lingotes de hierro + 1 palo -> 16 rieles)
    if (ironIngotId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID && railId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:rail",
                                    .width = 3,
                                    .height = 3,
                                    .pattern = {ironIngotId, EMPTY_ITEM_ID, ironIngotId, ironIngotId, stickId,
                                                ironIngotId, ironIngotId, EMPTY_ITEM_ID, ironIngotId},
                                    .result = ItemStack(railId, 16)});
    }

    // Vagoneta (5 lingotes de hierro en U)
    if (ironIngotId != EMPTY_ITEM_ID && minecartId != EMPTY_ITEM_ID) {
        registerShaped(
            ShapedRecipe{.id = "opencraft:minecart",
                         .width = 3,
                         .height = 2,
                         .pattern = {ironIngotId, EMPTY_ITEM_ID, ironIngotId, ironIngotId, ironIngotId, ironIngotId},
                         .result = ItemStack(minecartId, 1)});
    }

    // Vagoneta con cofre
    if (minecartId != EMPTY_ITEM_ID && chestId != EMPTY_ITEM_ID && chestMinecartId != EMPTY_ITEM_ID) {
        registerShapeless(ShapelessRecipe{.id = "opencraft:chest_minecart",
                                          .ingredients = {minecartId, chestId},
                                          .result = ItemStack(chestMinecartId, 1)});
    }

    // Vagoneta con tolva
    if (minecartId != EMPTY_ITEM_ID && hopperId != EMPTY_ITEM_ID && hopperMinecartId != EMPTY_ITEM_ID) {
        registerShapeless(ShapelessRecipe{.id = "opencraft:hopper_minecart",
                                          .ingredients = {minecartId, hopperId},
                                          .result = ItemStack(hopperMinecartId, 1)});
    }

    // Recetas de Fundición (Smelting)
    // Mena de hierro / Hierro crudo -> Lingote de hierro
    if (rawIronId != EMPTY_ITEM_ID && ironIngotId != EMPTY_ITEM_ID) {
        registerSmelting(SmeltingRecipe{.id = "opencraft:smelt_raw_iron",
                                        .inputItemId = rawIronId,
                                        .result = ItemStack(ironIngotId, 1),
                                        .cookTimeTicks = 200,
                                        .experience = 0.7f});
    }
    ItemId ironOreBlockItemId = itemReg.getId("opencraft:iron_ore");
    if (ironOreBlockItemId != EMPTY_ITEM_ID && ironIngotId != EMPTY_ITEM_ID) {
        registerSmelting(SmeltingRecipe{.id = "opencraft:smelt_iron_ore",
                                        .inputItemId = ironOreBlockItemId,
                                        .result = ItemStack(ironIngotId, 1),
                                        .cookTimeTicks = 200,
                                        .experience = 0.7f});
    }

    // Tronco de madera -> Carbón vegetal
    if (woodLogId != EMPTY_ITEM_ID && coalId != EMPTY_ITEM_ID) {
        registerSmelting(SmeltingRecipe{.id = "opencraft:smelt_wood_to_charcoal",
                                        .inputItemId = woodLogId,
                                        .result = ItemStack(coalId, 1),
                                        .cookTimeTicks = 200,
                                        .experience = 0.15f});
    }

    // Cobblestone -> Piedra
    if (cobblestoneId != EMPTY_ITEM_ID && stoneId != EMPTY_ITEM_ID) {
        registerSmelting(SmeltingRecipe{.id = "opencraft:smelt_cobblestone",
                                        .inputItemId = cobblestoneId,
                                        .result = ItemStack(stoneId, 1),
                                        .cookTimeTicks = 200,
                                        .experience = 0.1f});
    }

    // Carne cruda -> Carne cocinada
    if (rawBeefId != EMPTY_ITEM_ID && cookedBeefId != EMPTY_ITEM_ID) {
        registerSmelting(SmeltingRecipe{.id = "opencraft:smelt_raw_beef",
                                        .inputItemId = rawBeefId,
                                        .result = ItemStack(cookedBeefId, 1),
                                        .cookTimeTicks = 200,
                                        .experience = 0.35f});
    }

    // --- Fase 15: Recetas 1.21 (cámaras de desafío) y Variedades ---
    ItemId heavyCoreId = itemReg.getId("opencraft:heavy_core");
    ItemId whirlRodId = itemReg.getId("opencraft:whirl_rod");
    ItemId maceId = itemReg.getId("opencraft:mace");
    ItemId windChargeId = itemReg.getId("opencraft:wind_charge");
    ItemId copperBlockId = itemReg.getId("opencraft:copper_block");
    ItemId cutCopperId = itemReg.getId("opencraft:cut_copper");
    ItemId copperGrateId = itemReg.getId("opencraft:copper_grate");
    ItemId tuffId = itemReg.getId("opencraft:tuff");
    ItemId tuffBricksId = itemReg.getId("opencraft:tuff_bricks");
    ItemId chiseledTuffId = itemReg.getId("opencraft:chiseled_tuff");
    ItemId oakSignId = itemReg.getId("opencraft:oak_sign");

    // Maza: Heavy Core sobre Whirl Rod (1x2)
    if (heavyCoreId != EMPTY_ITEM_ID && whirlRodId != EMPTY_ITEM_ID && maceId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:mace",
                                    .width = 1,
                                    .height = 2,
                                    .pattern = {heavyCoreId, whirlRodId},
                                    .result = ItemStack(maceId, 1)});
    }

    // Carga de Viento: 1 Whirl Rod -> 4 Wind Charges
    if (whirlRodId != EMPTY_ITEM_ID && windChargeId != EMPTY_ITEM_ID) {
        registerShapeless(ShapelessRecipe{
            .id = "opencraft:wind_charge", .ingredients = {whirlRodId}, .result = ItemStack(windChargeId, 4)});
    }

    // Cobre Cortado (2x2 bloques de cobre)
    if (copperBlockId != EMPTY_ITEM_ID && cutCopperId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:cut_copper",
                                    .width = 2,
                                    .height = 2,
                                    .pattern = {copperBlockId, copperBlockId, copperBlockId, copperBlockId},
                                    .result = ItemStack(cutCopperId, 4)});
    }

    // Rejilla de Cobre (2x2 cobre cortado)
    if (cutCopperId != EMPTY_ITEM_ID && copperGrateId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:copper_grate",
                                    .width = 2,
                                    .height = 2,
                                    .pattern = {cutCopperId, cutCopperId, cutCopperId, cutCopperId},
                                    .result = ItemStack(copperGrateId, 4)});
    }

    // Ladrillos de Toba (2x2 toba)
    if (tuffId != EMPTY_ITEM_ID && tuffBricksId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:tuff_bricks",
                                    .width = 2,
                                    .height = 2,
                                    .pattern = {tuffId, tuffId, tuffId, tuffId},
                                    .result = ItemStack(tuffBricksId, 4)});
    }

    // Toba Cincelada (1x2 ladrillos de toba)
    if (tuffBricksId != EMPTY_ITEM_ID && chiseledTuffId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:chiseled_tuff",
                                    .width = 1,
                                    .height = 2,
                                    .pattern = {tuffBricksId, tuffBricksId},
                                    .result = ItemStack(chiseledTuffId, 1)});
    }

    // Cartel de Roble (6 tablones + 1 palo)
    if (planksId != EMPTY_ITEM_ID && stickId != EMPTY_ITEM_ID && oakSignId != EMPTY_ITEM_ID) {
        registerShaped(ShapedRecipe{.id = "opencraft:oak_sign",
                                    .width = 3,
                                    .height = 3,
                                    .pattern = {planksId, planksId, planksId, planksId, planksId, planksId,
                                                EMPTY_ITEM_ID, stickId, EMPTY_ITEM_ID},
                                    .result = ItemStack(oakSignId, 3)});
    }
}

} // namespace OpenCraft

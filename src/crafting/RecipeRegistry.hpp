#pragma once

#include <vector>
#include <string>
#include <span>
#include <memory>
#include <optional>
#include "crafting/Recipe.hpp"
#include "item/Inventory.hpp"

namespace OpenCraft {

class RecipeRegistry {
public:
    RecipeRegistry();
    ~RecipeRegistry() = default;

    static RecipeRegistry& global();

    void registerShaped(ShapedRecipe recipe);
    void registerShapeless(ShapelessRecipe recipe);
    void registerSmelting(SmeltingRecipe recipe);

    // Consulta de crafteo en una cuadrícula (gridWidth x gridHeight)
    // Devuelve el ItemStack resultado o ItemStack vacío si no coincide ninguna receta
    ItemStack matchCrafting(std::span<const ItemStack> grid, int gridWidth, int gridHeight,
                            std::string* outRecipeId = nullptr) const;

    // Ejecuta la fabricación deduciendo 1 unidad de cada ingrediente del grid
    // Si craftAll es true, fabrica la cantidad máxima posible que permite el grid
    // Devuelve true si se pudo fabricar con éxito
    bool craft(std::vector<ItemStack>& grid, int gridWidth, int gridHeight, ItemStack& outResult,
               bool craftAll = false) const;

    // Consulta de fundición
    const SmeltingRecipe* findSmeltingRecipe(ItemId inputItemId) const;

    // Consulta de combustible
    bool isFuel(ItemId itemId) const;
    uint32_t getFuelBurnTicks(ItemId itemId) const;

    // Métodos para libro de recetas
    const std::vector<ShapedRecipe>& getShapedRecipes() const noexcept { return m_shapedRecipes; }
    const std::vector<ShapelessRecipe>& getShapelessRecipes() const noexcept { return m_shapelessRecipes; }
    const std::vector<SmeltingRecipe>& getSmeltingRecipes() const noexcept { return m_smeltingRecipes; }

    void reset();

private:
    std::vector<ShapedRecipe> m_shapedRecipes;
    std::vector<ShapelessRecipe> m_shapelessRecipes;
    std::vector<SmeltingRecipe> m_smeltingRecipes;

    void registerDefaults();
    bool matchShaped(const ShapedRecipe& recipe, std::span<const ItemStack> grid, int gridWidth, int gridHeight) const;
    bool matchShapeless(const ShapelessRecipe& recipe, std::span<const ItemStack> grid) const;
};

} // namespace OpenCraft

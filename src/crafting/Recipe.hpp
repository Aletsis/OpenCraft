#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <span>
#include "item/ItemStack.hpp"

namespace OpenCraft {

enum class RecipeType : uint8_t {
    Shaped = 0,
    Shapeless,
    Smelting
};

struct ShapedRecipe {
    std::string id;
    int width = 0;
    int height = 0;
    std::vector<ItemId> pattern; // Tamaño width * height; 0 / EMPTY_ITEM_ID significa vacío
    ItemStack result;
};

struct ShapelessRecipe {
    std::string id;
    std::vector<ItemId> ingredients; // Ítems requeridos sin importar posición
    ItemStack result;
};

struct SmeltingRecipe {
    std::string id;
    ItemId inputItemId = EMPTY_ITEM_ID;
    ItemStack result;
    uint32_t cookTimeTicks = 200; // 200 ticks = 10 segundos a 20 Hz
    float experience = 0.1f;
};

} // namespace OpenCraft

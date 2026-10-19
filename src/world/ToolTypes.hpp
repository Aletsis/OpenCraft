#pragma once

#include <cstdint>

namespace OpenCraft {

enum class ToolType : uint8_t {
    None = 0,
    Pickaxe,
    Axe,
    Shovel,
    Sword,
    Hoe
};

enum class ToolTier : uint8_t {
    None = 0,
    Wood = 1,
    Stone = 2,
    Iron = 3,
    Diamond = 4,
    Orichalcum = 5
};

} // namespace OpenCraft

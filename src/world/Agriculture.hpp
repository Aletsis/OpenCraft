#pragma once

#include <cstdint>
#include <vector>
#include "world/Block.hpp"
#include "world/BlockEntity.hpp"
#include "item/ItemStack.hpp"

namespace OpenCraft {

class World;

struct HarvestResult {
    ItemStack product;
    ItemStack seeds;
};

enum class CropType : uint8_t {
    Wheat = 0,
    Carrot,
    Potato
};

class Agriculture {
public:
    static constexpr uint8_t MAX_CROP_AGE = 7;
    static constexpr int WATER_HYDRATION_RADIUS = 4;
    static constexpr uint8_t MIN_GROWTH_LIGHT = 9;

    // Comprueba si un bloque de Farmland tiene agua en un radio horizontal <= 4
    static bool isFarmlandHydrated(const World& world, const BlockPos& farmlandPos) noexcept;

    // Comprueba si las condiciones de luz permiten el crecimiento de cultivos (luz >= 9)
    static bool canCropGrow(const World& world, const BlockPos& cropPos, float daylightFactor = 1.0f) noexcept;

    // Cosecha un cultivo maduro devolviendo el alimento y semillas adicionales para ciclo renovable
    static HarvestResult harvestCrop(CropType crop, uint64_t seed);

    // Sistema de Pesca
    struct FishingBite {
        bool validWater = false;
        uint32_t waitTicks = 100;
        ItemStack caughtItem;
        uint32_t xp = 2;
    };

    static FishingBite evaluateFishing(const World& world, const BlockPos& waterPos, uint64_t seed);
};

} // namespace OpenCraft

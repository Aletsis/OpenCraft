#include "world/Agriculture.hpp"
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include <algorithm>

namespace OpenCraft {

bool Agriculture::isFarmlandHydrated(const World& world, const BlockPos& farmlandPos) noexcept {
    for (int dx = -WATER_HYDRATION_RADIUS; dx <= WATER_HYDRATION_RADIUS; ++dx) {
        for (int dz = -WATER_HYDRATION_RADIUS; dz <= WATER_HYDRATION_RADIUS; ++dz) {
            for (int dy = 0; dy <= 1; ++dy) {
                BlockID b = world.getBlock(farmlandPos.x + dx, farmlandPos.y + dy, farmlandPos.z + dz);
                if (b == BlockID::Water || b == BlockID::FlowingWater) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool Agriculture::canCropGrow(const World& world, const BlockPos& cropPos, float daylightFactor) noexcept {
    // Debe haber Farmland debajo
    BlockID under = world.getBlock(cropPos.x, cropPos.y - 1, cropPos.z);
    if (under != BlockID::Farmland) {
        return false;
    }

    uint8_t skyL = world.getSkyLight(cropPos.x, cropPos.y, cropPos.z);
    uint8_t blockL = world.getBlockLight(cropPos.x, cropPos.y, cropPos.z);
    float effectiveSky = static_cast<float>(skyL) * daylightFactor;
    float effectiveTotal = std::max(effectiveSky, static_cast<float>(blockL));

    return effectiveTotal >= static_cast<float>(MIN_GROWTH_LIGHT);
}

HarvestResult Agriculture::harvestCrop(CropType crop, uint64_t seed) {
    HarvestResult res;
    auto& reg = ItemRegistry::global();
    if (crop == CropType::Wheat) {
        res.product = ItemStack(reg.getId("opencraft:wheat"), 1);
        uint16_t seedCount = static_cast<uint16_t>(1 + (seed % 3)); // 1 a 3 semillas
        res.seeds = ItemStack(reg.getId("opencraft:wheat_seeds"), seedCount);
    } else if (crop == CropType::Carrot) {
        uint16_t carrotCount = static_cast<uint16_t>(2 + (seed % 3)); // 2 a 4 zanahorias
        res.product = ItemStack(reg.getId("opencraft:carrot"), carrotCount);
        res.seeds = ItemStack(reg.getId("opencraft:carrot"), 1); // La zanahoria es semilla y producto
    } else if (crop == CropType::Potato) {
        uint16_t potatoCount = static_cast<uint16_t>(2 + (seed % 3)); // 2 a 4 patatas
        res.product = ItemStack(reg.getId("opencraft:potato"), potatoCount);
        res.seeds = ItemStack(reg.getId("opencraft:potato"), 1);
    }
    return res;
}

Agriculture::FishingBite Agriculture::evaluateFishing(const World& world, const BlockPos& waterPos, uint64_t seed) {
    FishingBite bite;
    BlockID block = world.getBlock(waterPos.x, waterPos.y, waterPos.z);
    if (block != BlockID::Water && block != BlockID::FlowingWater) {
        bite.validWater = false;
        return bite;
    }

    bite.validWater = true;
    bite.waitTicks = static_cast<uint32_t>(100 + (seed % 200)); // 5 a 15 segundos (100 a 300 ticks)

    auto& reg = ItemRegistry::global();
    uint32_t roll = static_cast<uint32_t>(seed % 100);
    if (roll < 80) { // 80% Pez (Pez crudo o Salmón)
        if ((seed & 1) == 0) {
            bite.caughtItem = ItemStack(reg.getId("opencraft:cod"), 1);
        } else {
            bite.caughtItem = ItemStack(reg.getId("opencraft:salmon"), 1);
        }
        bite.xp = 2;
    } else if (roll < 90) { // 10% Tesoro
        uint32_t treasureRoll = (seed >> 4) % 3;
        if (treasureRoll == 0) {
            bite.caughtItem = ItemStack(reg.getId("opencraft:saddle"), 1);
        } else if (treasureRoll == 1) {
            bite.caughtItem = ItemStack(reg.getId("opencraft:bow"), 1);
        } else {
            bite.caughtItem = ItemStack(reg.getId("opencraft:fishing_rod"), 1);
        }
        bite.xp = 5;
    } else { // 10% Chatarra
        uint32_t junkRoll = (seed >> 4) % 3;
        if (junkRoll == 0) {
            bite.caughtItem = ItemStack(reg.getId("opencraft:leather"), 1);
        } else if (junkRoll == 1) {
            bite.caughtItem = ItemStack(reg.getId("opencraft:bone"), 1);
        } else {
            bite.caughtItem = ItemStack(reg.getId("opencraft:stick"), 1);
        }
        bite.xp = 1;
    }

    return bite;
}

} // namespace OpenCraft

#pragma once

#include "world/Block.hpp"
#include "world/BlockEntity.hpp"
#include "world/DispenserBlockEntity.hpp"
#include <vector>

namespace OpenCraft {

class World;
struct ItemEntity;

class PistonSimulator {
public:
    static constexpr size_t PISTON_PUSH_LIMIT = 12; // Límite canónico de 12 bloques

    static bool isImmovable(BlockID id) noexcept;
    static bool isBreakableByPiston(BlockID id) noexcept;

    // Intenta extender un pistón en la posición dada
    static bool extendPiston(World& world, const BlockPos& pistonPos, BlockDirection facing, bool isSticky,
                             std::vector<ItemEntity>& outDroppedItems);

    // Intenta retraer un pistón (si isOneTickPulse es true, pistón pegajoso suelta el bloque sin arrastrarlo)
    static bool retractPiston(World& world, const BlockPos& pistonPos, BlockDirection facing, bool isSticky,
                              bool isOneTickPulse, std::vector<ItemEntity>& outDroppedItems);
};

} // namespace OpenCraft

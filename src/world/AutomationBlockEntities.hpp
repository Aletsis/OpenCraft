#pragma once

#include "world/DispenserBlockEntity.hpp"
#include "world/DropperBlockEntity.hpp"
#include "world/CrafterBlockEntity.hpp"
#include <vector>

namespace OpenCraft {

class World;
struct ItemEntity;
struct ProjectileEntity;

class AutomationActions {
public:
    static BlockPos getFacingOffset(BlockDirection facing) noexcept;

    // Dispara un soltador (Dropper)
    static bool triggerDropper(World& world, DropperBlockEntity& dropper, std::vector<ItemEntity>& outItems);

    // Dispara un dispensador (Dispenser)
    static bool triggerDispenser(World& world, DispenserBlockEntity& dispenser, std::vector<ItemEntity>& outItems,
                                 std::vector<ProjectileEntity>& outProjectiles);

    // Dispara un fabricador automático (Crafter) del juego de referencia 1.21
    static bool triggerCrafter(World& world, CrafterBlockEntity& crafter, std::vector<ItemEntity>& outItems);
};

} // namespace OpenCraft

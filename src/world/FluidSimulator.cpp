#include "world/FluidSimulator.hpp"
#include "world/World.hpp"
#include <array>

namespace OpenCraft {

static constexpr std::array<BlockPos, 6> s_allNeighbors = {
    {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};

static constexpr std::array<BlockPos, 4> s_horizNeighbors = {{{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}}};

bool FluidSimulator::checkFluidInteractions(World& world, const BlockPos& pos) {
    BlockID current = world.getBlock(pos.x, pos.y, pos.z);

    if (current == BlockID::Lava) {
        // Fuente de lava: si toca agua, se convierte en Obsidiana
        for (const auto& off : s_allNeighbors) {
            BlockPos npos{pos.x + off.x, pos.y + off.y, pos.z + off.z};
            if (isWater(world.getBlock(npos.x, npos.y, npos.z))) {
                world.setBlock(pos.x, pos.y, pos.z, BlockID::Obsidian);
                return true;
            }
        }
    } else if (current == BlockID::FlowingLava) {
        // Flujo de lava: si toca agua, se convierte en Adoquín
        for (const auto& off : s_allNeighbors) {
            BlockPos npos{pos.x + off.x, pos.y + off.y, pos.z + off.z};
            if (isWater(world.getBlock(npos.x, npos.y, npos.z))) {
                world.setBlock(pos.x, pos.y, pos.z, BlockID::Cobblestone);
                return true;
            }
        }
    } else if (current == BlockID::Water) {
        // Fuente de agua: si toca lava por encima o al lado, puede volverse Piedra
        for (const auto& off : s_allNeighbors) {
            BlockPos npos{pos.x + off.x, pos.y + off.y, pos.z + off.z};
            BlockID nb = world.getBlock(npos.x, npos.y, npos.z);
            if (isLava(nb)) {
                world.setBlock(pos.x, pos.y, pos.z, BlockID::Stone);
                return true;
            }
        }
    } else if (current == BlockID::FlowingWater) {
        // Flujo de agua: si toca lava, se convierte en Adoquín
        for (const auto& off : s_allNeighbors) {
            BlockPos npos{pos.x + off.x, pos.y + off.y, pos.z + off.z};
            if (isLava(world.getBlock(npos.x, npos.y, npos.z))) {
                world.setBlock(pos.x, pos.y, pos.z, BlockID::Cobblestone);
                return true;
            }
        }
    }

    return false;
}

bool FluidSimulator::tryFormInfiniteWaterSource(World& world, const BlockPos& pos) {
    // Para formar fuente infinita, el bloque debe tener soporte sólido abajo (o agua)
    BlockID below = world.getBlock(pos.x, pos.y - 1, pos.z);
    const auto& belowInfo = BlockDatabase::get(below);
    if (!belowInfo.isSolid && below != BlockID::Water) {
        return false;
    }

    // Contar fuentes de agua adyacentes horizontalmente
    int sourceCount = 0;
    for (const auto& h : s_horizNeighbors) {
        BlockPos npos{pos.x + h.x, pos.y, pos.z + h.z};
        if (world.getBlock(npos.x, npos.y, npos.z) == BlockID::Water) {
            ++sourceCount;
        }
    }

    if (sourceCount >= 2) {
        world.setBlock(pos.x, pos.y, pos.z, BlockID::Water);
        return true;
    }

    return false;
}

void FluidSimulator::onFluidPlaced(World& world, ScheduledUpdateQueue& queue, const BlockPos& pos, BlockID fluidBlock,
                                   uint64_t currentTick) {
    if (checkFluidInteractions(world, pos)) {
        return;
    }

    uint64_t delay = isWater(fluidBlock) ? WATER_TICK_DELAY : LAVA_TICK_DELAY;
    queue.schedule(pos, fluidBlock, currentTick + delay, 0);
}

void FluidSimulator::onFluidRemoved(World& world, ScheduledUpdateQueue& queue, const BlockPos& pos,
                                    uint64_t currentTick) {
    (void)world;
    (void)queue;
    (void)pos;
    (void)currentTick;
}

void FluidSimulator::processUpdate(World& world, ScheduledUpdateQueue& queue, const ScheduledBlockUpdate& update,
                                   uint64_t currentTick) {
    BlockID current = world.getBlock(update.pos.x, update.pos.y, update.pos.z);
    if (!isFluid(current)) {
        return;
    }

    if (isWater(current)) {
        updateWaterFlow(world, queue, update.pos, currentTick);
    } else if (isLava(current)) {
        updateLavaFlow(world, queue, update.pos, currentTick);
    }
}

void FluidSimulator::updateWaterFlow(World& world, ScheduledUpdateQueue& queue, const BlockPos& pos,
                                     uint64_t currentTick) {
    if (checkFluidInteractions(world, pos)) return;

    // 1. Caída vertical
    BlockPos downPos{pos.x, pos.y - 1, pos.z};
    if (downPos.y >= Chunk::MIN_Y) {
        BlockID downBlock = world.getBlock(downPos.x, downPos.y, downPos.z);
        if (downBlock == BlockID::Air) {
            world.setBlock(downPos.x, downPos.y, downPos.z, BlockID::FlowingWater);
            queue.schedule(downPos, BlockID::FlowingWater, currentTick + WATER_TICK_DELAY, 0);
            return;
        } else if (downBlock == BlockID::Lava) {
            world.setBlock(downPos.x, downPos.y, downPos.z, BlockID::Obsidian);
            return;
        } else if (downBlock == BlockID::FlowingLava) {
            world.setBlock(downPos.x, downPos.y, downPos.z, BlockID::Cobblestone);
            return;
        }
    }

    // 2. Extensión horizontal
    for (const auto& h : s_horizNeighbors) {
        BlockPos npos{pos.x + h.x, pos.y, pos.z + h.z};
        BlockID nb = world.getBlock(npos.x, npos.y, npos.z);
        if (nb == BlockID::Air) {
            if (!tryFormInfiniteWaterSource(world, npos)) {
                world.setBlock(npos.x, npos.y, npos.z, BlockID::FlowingWater);
                queue.schedule(npos, BlockID::FlowingWater, currentTick + WATER_TICK_DELAY, 0);
            }
        } else if (nb == BlockID::Lava) {
            world.setBlock(npos.x, npos.y, npos.z, BlockID::Obsidian);
        } else if (nb == BlockID::FlowingLava) {
            world.setBlock(npos.x, npos.y, npos.z, BlockID::Cobblestone);
        } else if (nb == BlockID::FlowingWater) {
            tryFormInfiniteWaterSource(world, npos);
        }
    }
}

void FluidSimulator::updateLavaFlow(World& world, ScheduledUpdateQueue& queue, const BlockPos& pos,
                                    uint64_t currentTick) {
    if (checkFluidInteractions(world, pos)) return;

    // 1. Caída vertical
    BlockPos downPos{pos.x, pos.y - 1, pos.z};
    if (downPos.y >= Chunk::MIN_Y) {
        BlockID downBlock = world.getBlock(downPos.x, downPos.y, downPos.z);
        if (downBlock == BlockID::Air) {
            world.setBlock(downPos.x, downPos.y, downPos.z, BlockID::FlowingLava);
            queue.schedule(downPos, BlockID::FlowingLava, currentTick + LAVA_TICK_DELAY, 0);
            return;
        } else if (downBlock == BlockID::Water || downBlock == BlockID::FlowingWater) {
            world.setBlock(downPos.x, downPos.y, downPos.z, BlockID::Cobblestone);
            return;
        }
    }

    // 2. Extensión horizontal
    for (const auto& h : s_horizNeighbors) {
        BlockPos npos{pos.x + h.x, pos.y, pos.z + h.z};
        BlockID nb = world.getBlock(npos.x, npos.y, npos.z);
        if (nb == BlockID::Air) {
            world.setBlock(npos.x, npos.y, npos.z, BlockID::FlowingLava);
            queue.schedule(npos, BlockID::FlowingLava, currentTick + LAVA_TICK_DELAY, 0);
        } else if (nb == BlockID::Water) {
            world.setBlock(npos.x, npos.y, npos.z, BlockID::Stone);
        } else if (nb == BlockID::FlowingWater) {
            world.setBlock(npos.x, npos.y, npos.z, BlockID::Cobblestone);
        }
    }
}

} // namespace OpenCraft

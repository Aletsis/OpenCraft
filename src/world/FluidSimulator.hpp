#pragma once

#include "world/Block.hpp"
#include "world/BlockEntity.hpp"
#include "world/ScheduledUpdates.hpp"
#include "math/Math.hpp"

namespace OpenCraft {

class World;

class FluidSimulator {
public:
    static constexpr uint64_t WATER_TICK_DELAY = 5; // 5 ticks (250ms a 20Hz)
    static constexpr uint64_t LAVA_TICK_DELAY = 30; // 30 ticks (1.5s a 20Hz)
    static constexpr int WATER_MAX_DISTANCE = 7;    // Extensión máxima de flujo horizontal
    static constexpr int LAVA_MAX_DISTANCE = 3;     // Extensión máxima en sobremundo

    FluidSimulator() = default;
    ~FluidSimulator() = default;

    static bool isWater(BlockID id) noexcept { return id == BlockID::Water || id == BlockID::FlowingWater; }

    static bool isLava(BlockID id) noexcept { return id == BlockID::Lava || id == BlockID::FlowingLava; }

    static bool isFluid(BlockID id) noexcept { return isWater(id) || isLava(id); }

    // Procesa una actualización de fluido disparada desde la cola programada
    void processUpdate(World& world, ScheduledUpdateQueue& queue, const ScheduledBlockUpdate& update,
                       uint64_t currentTick);

    // Notificación cuando se coloca o retira un fluido en el mundo
    void onFluidPlaced(World& world, ScheduledUpdateQueue& queue, const BlockPos& pos, BlockID fluidBlock,
                       uint64_t currentTick);
    void onFluidRemoved(World& world, ScheduledUpdateQueue& queue, const BlockPos& pos, uint64_t currentTick);

    // Comprueba interacciones agua <-> lava (Obsidiana, Adoquín, Piedra)
    bool checkFluidInteractions(World& world, const BlockPos& pos);

    // Regla de fuente infinita de agua: comprueba si un bloque rodeado por 2 fuentes se convierte en fuente
    bool tryFormInfiniteWaterSource(World& world, const BlockPos& pos);

private:
    void updateWaterFlow(World& world, ScheduledUpdateQueue& queue, const BlockPos& pos, uint64_t currentTick);
    void updateLavaFlow(World& world, ScheduledUpdateQueue& queue, const BlockPos& pos, uint64_t currentTick);
};

} // namespace OpenCraft

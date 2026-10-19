#pragma once

#include "world/Block.hpp"
#include "world/BlockEntity.hpp"
#include "world/DispenserBlockEntity.hpp"
#include "world/ScheduledUpdates.hpp"
#include "math/Math.hpp"
#include <unordered_map>
#include <vector>

namespace OpenCraft {

class World;
struct ItemEntity;
struct ProjectileEntity;

struct CinnabarComponentState {
    uint8_t power = 0;
    BlockDirection facing = BlockDirection::North;
    uint8_t delayTicks = 2;         // Para repetidores: 2, 4, 6, 8 game ticks
    bool isLocked = false;          // Latch de repetidor
    bool isSubtractionMode = false; // Comparador
    bool isActive = false;          // Palancas, botones, antorchas, placas
    uint32_t activeTimer = 0;       // Botones (20 ticks piedra, 30 ticks madera)
    uint32_t burnoutCounter = 0;    // Antorcha burnout
    uint32_t lastBurnoutTick = 0;
    bool isBurnedOut = false;
    uint32_t poweredDuration = 0; // Para detectar pulso de 1 tick en pistones
    bool wasPowered = false;
};

class CinnabarEngine {
public:
    static constexpr size_t DEFAULT_CINNABAR_BUDGET = 1000;

    CinnabarEngine() = default;
    ~CinnabarEngine() = default;

    // Consulta de nivel de señal [0..15]
    uint8_t getSignal(const BlockPos& pos) const noexcept;
    void setSignal(const BlockPos& pos, uint8_t level) noexcept;

    // Componentes interactivos y lógicos
    CinnabarComponentState& getOrCreateComponent(const BlockPos& pos);
    const CinnabarComponentState* getComponent(const BlockPos& pos) const noexcept;
    void removeComponent(const BlockPos& pos) noexcept;

    // Consulta de potencia canónica
    uint8_t getStrongPower(World& world, const BlockPos& pos, BlockDirection toDirection) const;
    uint8_t getWeakPower(World& world, const BlockPos& pos, BlockDirection toDirection) const;
    uint8_t getDirectOrIndirectPower(World& world, const BlockPos& pos) const;

    // Cuasiconectividad Java: comprueba si la posición o el bloque superior recibe energía
    bool isPoweredWithQuasiConnectivity(World& world, const BlockPos& pos) const;

    // Cálculo analógico de comparador para contenedores contiguos
    static uint8_t calculateContainerComparatorSignal(World& world, const BlockPos& containerPos);

    // Ciclo de simulación por tick
    void tick(World& world, ScheduledUpdateQueue& scheduledUpdates, std::vector<ItemEntity>& itemEntities,
              std::vector<ProjectileEntity>& projectiles, uint64_t currentTick, float sunlightFactor);

    // Notificaciones de colocación / rotura / interacción
    void onBlockPlaced(World& world, const BlockPos& pos, BlockID id, uint64_t currentTick);
    void onBlockRemoved(World& world, const BlockPos& pos, BlockID id, uint64_t currentTick);
    bool onBlockInteracted(World& world, const BlockPos& pos, BlockID id, uint64_t currentTick);

    // Propagación de polvo de cinabrio
    void propagateDust(World& world, const BlockPos& startPos);

    // Limpieza
    void clear() noexcept {
        m_signals.clear();
        m_components.clear();
    }

private:
    std::unordered_map<BlockPos, uint8_t, BlockPosHash> m_signals;
    std::unordered_map<BlockPos, CinnabarComponentState, BlockPosHash> m_components;

    void updateMechanisms(World& world, const BlockPos& pos, BlockID block, std::vector<ItemEntity>& itemEntities,
                          std::vector<ProjectileEntity>& projectiles, uint64_t currentTick);
};

} // namespace OpenCraft

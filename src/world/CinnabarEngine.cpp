#include "world/CinnabarEngine.hpp"
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include "world/ChestBlockEntity.hpp"
#include "world/FurnaceBlockEntity.hpp"
#include "world/HopperBlockEntity.hpp"
#include "world/DispenserBlockEntity.hpp"
#include "world/DropperBlockEntity.hpp"
#include "world/CrafterBlockEntity.hpp"
#include "world/AutomationBlockEntities.hpp"
#include "world/PistonSimulator.hpp"
#include "sim/ItemEntity.hpp"
#include "sim/ProjectileEntity.hpp"
#include <queue>
#include <algorithm>

namespace OpenCraft {

static BlockPos getFaceOffset(BlockDirection dir) noexcept {
    switch (dir) {
        case BlockDirection::Down: return {0, -1, 0};
        case BlockDirection::Up: return {0, 1, 0};
        case BlockDirection::North: return {0, 0, -1};
        case BlockDirection::South: return {0, 0, 1};
        case BlockDirection::West: return {-1, 0, 0};
        case BlockDirection::East: return {1, 0, 0};
        default: return {0, 0, -1};
    }
}

static BlockDirection getOppositeDirection(BlockDirection dir) noexcept {
    switch (dir) {
        case BlockDirection::Down: return BlockDirection::Up;
        case BlockDirection::Up: return BlockDirection::Down;
        case BlockDirection::North: return BlockDirection::South;
        case BlockDirection::South: return BlockDirection::North;
        case BlockDirection::West: return BlockDirection::East;
        case BlockDirection::East: return BlockDirection::West;
        default: return BlockDirection::North;
    }
}

uint8_t CinnabarEngine::getSignal(const BlockPos& pos) const noexcept {
    auto it = m_signals.find(pos);
    return (it != m_signals.end()) ? it->second : 0;
}

void CinnabarEngine::setSignal(const BlockPos& pos, uint8_t level) noexcept {
    if (level == 0) {
        m_signals.erase(pos);
    } else {
        m_signals[pos] = std::min<uint8_t>(level, 15);
    }
}

CinnabarComponentState& CinnabarEngine::getOrCreateComponent(const BlockPos& pos) { return m_components[pos]; }

const CinnabarComponentState* CinnabarEngine::getComponent(const BlockPos& pos) const noexcept {
    auto it = m_components.find(pos);
    return (it != m_components.end()) ? &it->second : nullptr;
}

void CinnabarEngine::removeComponent(const BlockPos& pos) noexcept {
    m_components.erase(pos);
    m_signals.erase(pos);
}

uint8_t CinnabarEngine::getStrongPower(World& world, const BlockPos& pos, BlockDirection toDirection) const {
    BlockID id = world.getBlock(pos.x, pos.y, pos.z);
    if (id == BlockID::CinnabarBlock) {
        return 15;
    }

    const auto* comp = getComponent(pos);
    if (comp && comp->isActive) {
        if (id == BlockID::Lever || id == BlockID::StoneButton || id == BlockID::OakButton ||
            id == BlockID::PressurePlate) {
            return 15;
        }
        if (id == BlockID::CinnabarTorch && toDirection == BlockDirection::Up) {
            return 15;
        }
        if (id == BlockID::Repeater && toDirection == comp->facing) {
            return 15;
        }
        if (id == BlockID::Comparator && toDirection == comp->facing) {
            return comp->power;
        }
    }

    // Si pos es un bloque sólido opaco, comprobar si recibe energía fuerte de un vecino
    const auto& props = BlockRegistry::global().get(world.getBlockState(pos.x, pos.y, pos.z));
    if (props.isOpaque) {
        // Un bloque sólido energizado fuertemente por repetidor o antorcha
        static constexpr std::array<BlockDirection, 6> dirs = {BlockDirection::Down,  BlockDirection::Up,
                                                               BlockDirection::North, BlockDirection::South,
                                                               BlockDirection::West,  BlockDirection::East};
        uint8_t maxStrong = 0;
        for (auto d : dirs) {
            if (d == getOppositeDirection(toDirection)) continue;
            BlockPos nb = {pos.x + getFaceOffset(d).x, pos.y + getFaceOffset(d).y, pos.z + getFaceOffset(d).z};
            BlockID nbId = world.getBlock(nb.x, nb.y, nb.z);
            const auto* nbComp = getComponent(nb);
            if (nbComp && nbComp->isActive) {
                if (nbId == BlockID::Repeater && nbComp->facing == getOppositeDirection(d)) {
                    maxStrong = std::max<uint8_t>(maxStrong, 15);
                } else if (nbId == BlockID::CinnabarTorch && d == BlockDirection::Down) {
                    maxStrong = std::max<uint8_t>(maxStrong, 15);
                } else if (nbId == BlockID::Lever || nbId == BlockID::StoneButton || nbId == BlockID::OakButton) {
                    maxStrong = std::max<uint8_t>(maxStrong, 15);
                }
            }
        }
        return maxStrong;
    }

    return 0;
}

uint8_t CinnabarEngine::getWeakPower(World& world, const BlockPos& pos, BlockDirection toDirection) const {
    uint8_t strong = getStrongPower(world, pos, toDirection);
    if (strong > 0) return strong;

    BlockID id = world.getBlock(pos.x, pos.y, pos.z);
    if (id == BlockID::CinnabarWire) {
        return getSignal(pos);
    }
    if (id == BlockID::DaylightDetector) {
        return getSignal(pos);
    }

    const auto* comp = getComponent(pos);
    if (comp && comp->isActive) {
        if (id == BlockID::CinnabarTorch) {
            // Una antorcha colocada en un bloque NO alimenta hacia abajo al bloque de soporte
            if (toDirection == BlockDirection::Down) {
                return 0;
            }
            return 15;
        }
    }

    // Bloque sólido que recibe señal débil de polvo de cinabrio adyacente
    const auto& props = BlockRegistry::global().get(world.getBlockState(pos.x, pos.y, pos.z));
    if (props.isOpaque) {
        static constexpr std::array<BlockDirection, 6> dirs = {BlockDirection::Down,  BlockDirection::Up,
                                                               BlockDirection::North, BlockDirection::South,
                                                               BlockDirection::West,  BlockDirection::East};
        uint8_t maxWeak = 0;
        for (auto d : dirs) {
            BlockPos nb = {pos.x + getFaceOffset(d).x, pos.y + getFaceOffset(d).y, pos.z + getFaceOffset(d).z};
            if (world.getBlock(nb.x, nb.y, nb.z) == BlockID::CinnabarWire) {
                maxWeak = std::max<uint8_t>(maxWeak, getSignal(nb));
            }
        }
        return maxWeak;
    }

    uint8_t directSignal = getSignal(pos);
    if (directSignal > 0) return directSignal;

    return 0;
}

uint8_t CinnabarEngine::getDirectOrIndirectPower(World& world, const BlockPos& pos) const {
    uint8_t selfSignal = getSignal(pos);
    if (selfSignal > 0) return selfSignal;

    static constexpr std::array<BlockDirection, 6> dirs = {BlockDirection::Down,  BlockDirection::Up,
                                                           BlockDirection::North, BlockDirection::South,
                                                           BlockDirection::West,  BlockDirection::East};

    uint8_t maxP = 0;
    for (auto d : dirs) {
        BlockPos nb = {pos.x + getFaceOffset(d).x, pos.y + getFaceOffset(d).y, pos.z + getFaceOffset(d).z};
        uint8_t p = getWeakPower(world, nb, getOppositeDirection(d));
        if (p > maxP) maxP = p;
    }
    return maxP;
}

bool CinnabarEngine::isPoweredWithQuasiConnectivity(World& world, const BlockPos& pos) const {
    if (getDirectOrIndirectPower(world, pos) > 0) {
        return true;
    }
    // Paridad Java: Cuasiconectividad por el bloque superior
    BlockPos abovePos{pos.x, pos.y + 1, pos.z};
    return getDirectOrIndirectPower(world, abovePos) > 0;
}

uint8_t CinnabarEngine::calculateContainerComparatorSignal(World& world, const BlockPos& containerPos) {
    auto entity = world.getBlockEntity(containerPos.x, containerPos.y, containerPos.z);
    if (!entity) return 0;

    if (entity->getType() == BlockEntityType::Crafter) {
        auto crafter = std::static_pointer_cast<CrafterBlockEntity>(entity);
        return crafter->getComparatorSignal();
    }

    Inventory* inv = nullptr;
    if (entity->getType() == BlockEntityType::Chest) {
        inv = &std::static_pointer_cast<ChestBlockEntity>(entity)->getInventory();
    } else if (entity->getType() == BlockEntityType::Furnace) {
        inv = &std::static_pointer_cast<FurnaceBlockEntity>(entity)->getInventory();
    } else if (entity->getType() == BlockEntityType::Hopper) {
        inv = &std::static_pointer_cast<HopperBlockEntity>(entity)->getInventory();
    } else if (entity->getType() == BlockEntityType::Dropper) {
        inv = &std::static_pointer_cast<DropperBlockEntity>(entity)->getInventory();
    } else if (entity->getType() == BlockEntityType::Dispenser) {
        inv = &std::static_pointer_cast<DispenserBlockEntity>(entity)->getInventory();
    }

    if (!inv || inv->getSize() == 0) return 0;

    double totalFraction = 0.0;
    size_t nonEmptySlots = 0;
    for (size_t i = 0; i < inv->getSize(); ++i) {
        const auto& slot = inv->getSlot(i);
        if (!slot.isEmpty()) {
            ++nonEmptySlots;
            uint16_t maxStack = ItemRegistry::global().get(slot.itemId).maxStackSize;
            if (maxStack == 0) maxStack = 64;
            totalFraction += static_cast<double>(slot.count) / static_cast<double>(maxStack);
        }
    }

    if (nonEmptySlots == 0) return 0;
    double fullness = totalFraction / static_cast<double>(inv->getSize());
    return static_cast<uint8_t>(1 + std::floor(fullness * 14.0));
}

void CinnabarEngine::propagateDust(World& world, const BlockPos& startPos) {
    // BFS con presupuesto acotado para propagar atenuación (15 a 0)
    std::queue<BlockPos> queue;
    queue.push(startPos);

    size_t workCount = 0;
    static constexpr std::array<BlockPos, 4> horizOffsets = {{{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}}};

    while (!queue.empty() && workCount < DEFAULT_CINNABAR_BUDGET) {
        BlockPos current = queue.front();
        queue.pop();
        ++workCount;

        BlockID curBlock = world.getBlock(current.x, current.y, current.z);
        if (curBlock != BlockID::CinnabarWire) continue;

        // Calcular potencia máxima recibida de fuentes adyacentes o polvo vecino
        uint8_t maxInPower = 0;

        // 1. Fuentes directas en los 6 lados
        static constexpr std::array<BlockDirection, 6> dirs = {BlockDirection::Down,  BlockDirection::Up,
                                                               BlockDirection::North, BlockDirection::South,
                                                               BlockDirection::West,  BlockDirection::East};
        for (auto d : dirs) {
            BlockPos nb = {current.x + getFaceOffset(d).x, current.y + getFaceOffset(d).y,
                           current.z + getFaceOffset(d).z};
            BlockID nbId = world.getBlock(nb.x, nb.y, nb.z);
            if (nbId == BlockID::CinnabarWire) {
                uint8_t p = getSignal(nb);
                if (p > 0 && p - 1 > maxInPower) {
                    maxInPower = p - 1;
                }
            } else {
                uint8_t p = getStrongPower(world, nb, getOppositeDirection(d));
                if (p > maxInPower) maxInPower = p;
                p = getWeakPower(world, nb, getOppositeDirection(d));
                if (p > maxInPower) maxInPower = p;
            }
        }

        // 2. Conectividad escalonada (arriba/abajo)
        for (const auto& ho : horizOffsets) {
            // Escalón arriba
            BlockPos upPos{current.x + ho.x, current.y + 1, current.z + ho.z};
            if (world.getBlock(upPos.x, upPos.y, upPos.z) == BlockID::CinnabarWire) {
                uint8_t p = getSignal(upPos);
                if (p > 0 && p - 1 > maxInPower) maxInPower = p - 1;
            }
            // Escalón abajo
            BlockPos downPos{current.x + ho.x, current.y - 1, current.z + ho.z};
            if (world.getBlock(downPos.x, downPos.y, downPos.z) == BlockID::CinnabarWire) {
                uint8_t p = getSignal(downPos);
                if (p > 0 && p - 1 > maxInPower) maxInPower = p - 1;
            }
        }

        uint8_t curPower = getSignal(current);
        if (curPower != maxInPower) {
            setSignal(current, maxInPower);

            // Re-encolar vecinos para propagar cambio
            for (auto d : dirs) {
                BlockPos nb = {current.x + getFaceOffset(d).x, current.y + getFaceOffset(d).y,
                               current.z + getFaceOffset(d).z};
                if (world.getBlock(nb.x, nb.y, nb.z) == BlockID::CinnabarWire) {
                    queue.push(nb);
                }
            }
            for (const auto& ho : horizOffsets) {
                BlockPos upPos{current.x + ho.x, current.y + 1, current.z + ho.z};
                if (world.getBlock(upPos.x, upPos.y, upPos.z) == BlockID::CinnabarWire) queue.push(upPos);
                BlockPos downPos{current.x + ho.x, current.y - 1, current.z + ho.z};
                if (world.getBlock(downPos.x, downPos.y, downPos.z) == BlockID::CinnabarWire) queue.push(downPos);
            }
        }
    }
}

void CinnabarEngine::onBlockPlaced(World& world, const BlockPos& pos, BlockID id, uint64_t currentTick) {
    (void)currentTick;
    auto& comp = getOrCreateComponent(pos);

    switch (id) {
        case BlockID::CinnabarBlock:
            setSignal(pos, 15);
            for (auto d : {BlockDirection::Down, BlockDirection::Up, BlockDirection::North, BlockDirection::South,
                           BlockDirection::West, BlockDirection::East}) {
                BlockPos nb = {pos.x + getFaceOffset(d).x, pos.y + getFaceOffset(d).y, pos.z + getFaceOffset(d).z};
                if (world.getBlock(nb.x, nb.y, nb.z) == BlockID::CinnabarWire) {
                    propagateDust(world, nb);
                }
            }
            break;
        case BlockID::CinnabarTorch:
            comp.isActive = true;
            setSignal(pos, 15);
            for (auto d : {BlockDirection::Down, BlockDirection::Up, BlockDirection::North, BlockDirection::South,
                           BlockDirection::West, BlockDirection::East}) {
                BlockPos nb = {pos.x + getFaceOffset(d).x, pos.y + getFaceOffset(d).y, pos.z + getFaceOffset(d).z};
                if (world.getBlock(nb.x, nb.y, nb.z) == BlockID::CinnabarWire) {
                    propagateDust(world, nb);
                }
            }
            break;
        case BlockID::CinnabarWire: propagateDust(world, pos); break;
        case BlockID::Repeater:
            comp.delayTicks = 2;
            comp.isActive = false;
            break;
        case BlockID::Comparator:
            comp.isSubtractionMode = false;
            comp.isActive = false;
            break;
        case BlockID::Lever: comp.isActive = false; break;
        default: break;
    }
}

void CinnabarEngine::onBlockRemoved(World& world, const BlockPos& pos, BlockID id, uint64_t currentTick) {
    (void)id;
    (void)currentTick;
    removeComponent(pos);

    // Notificar a vecinos para recalcular polvo
    static constexpr std::array<BlockPos, 6> offsets = {
        {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
    for (const auto& off : offsets) {
        BlockPos nb{pos.x + off.x, pos.y + off.y, pos.z + off.z};
        if (world.getBlock(nb.x, nb.y, nb.z) == BlockID::CinnabarWire) {
            propagateDust(world, nb);
        }
    }
}

bool CinnabarEngine::onBlockInteracted(World& world, const BlockPos& pos, BlockID id, uint64_t currentTick) {
    (void)currentTick;
    auto& comp = getOrCreateComponent(pos);

    if (id == BlockID::Lever) {
        comp.isActive = !comp.isActive;
        setSignal(pos, comp.isActive ? 15 : 0);
        // Notificar vecinos
        static constexpr std::array<BlockPos, 6> offsets = {
            {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
        for (const auto& off : offsets) {
            BlockPos nb{pos.x + off.x, pos.y + off.y, pos.z + off.z};
            if (world.getBlock(nb.x, nb.y, nb.z) == BlockID::CinnabarWire) {
                propagateDust(world, nb);
            }
        }
        return true;
    } else if (id == BlockID::StoneButton) {
        comp.isActive = true;
        comp.activeTimer = 20; // 20 ticks (1s)
        setSignal(pos, 15);
        return true;
    } else if (id == BlockID::OakButton) {
        comp.isActive = true;
        comp.activeTimer = 30; // 30 ticks (1.5s)
        setSignal(pos, 15);
        return true;
    } else if (id == BlockID::Repeater) {
        // Ciclar retardo: 2 -> 4 -> 6 -> 8 -> 2 ticks
        comp.delayTicks = (comp.delayTicks >= 8) ? 2 : (comp.delayTicks + 2);
        return true;
    } else if (id == BlockID::Comparator) {
        // Conmutar entre comparación y sustracción
        comp.isSubtractionMode = !comp.isSubtractionMode;
        return true;
    }

    return false;
}

void CinnabarEngine::tick(World& world, ScheduledUpdateQueue& scheduledUpdates, std::vector<ItemEntity>& itemEntities,
                          std::vector<ProjectileEntity>& projectiles, uint64_t currentTick, float sunlightFactor) {
    (void)scheduledUpdates;

    // 1. Actualizar botones temporizados
    for (auto& [pos, comp] : m_components) {
        if (comp.isActive && comp.activeTimer > 0) {
            --comp.activeTimer;
            if (comp.activeTimer == 0) {
                comp.isActive = false;
                setSignal(pos, 0);
                // Propagar apagado
                for (auto d : {BlockDirection::Down, BlockDirection::Up, BlockDirection::North, BlockDirection::South,
                               BlockDirection::West, BlockDirection::East}) {
                    BlockPos nb = {pos.x + getFaceOffset(d).x, pos.y + getFaceOffset(d).y, pos.z + getFaceOffset(d).z};
                    if (world.getBlock(nb.x, nb.y, nb.z) == BlockID::CinnabarWire) {
                        propagateDust(world, nb);
                    }
                }
            }
        }

        // Antorchas con protección contra burnout
        BlockID curBlock = world.getBlock(pos.x, pos.y, pos.z);
        if (curBlock == BlockID::CinnabarTorch || curBlock == BlockID::CinnabarTorchOff) {
            BlockPos basePos{pos.x, pos.y - 1, pos.z};
            uint8_t basePower = getDirectOrIndirectPower(world, basePos);
            bool shouldBeOff = (basePower > 0);

            if (shouldBeOff && curBlock == BlockID::CinnabarTorch) {
                // Registrar toggle para burnout
                if (currentTick - comp.lastBurnoutTick <= 60) {
                    ++comp.burnoutCounter;
                } else {
                    comp.burnoutCounter = 1;
                }
                comp.lastBurnoutTick = static_cast<uint32_t>(currentTick);

                if (comp.burnoutCounter > 8) {
                    comp.isBurnedOut = true;
                }

                world.setBlock(pos.x, pos.y, pos.z, BlockID::CinnabarTorchOff);
                comp.isActive = false;
                setSignal(pos, 0);
            } else if (!shouldBeOff && curBlock == BlockID::CinnabarTorchOff) {
                if (comp.isBurnedOut && currentTick - comp.lastBurnoutTick < 60) {
                    // Esperar enfriamiento
                } else {
                    comp.isBurnedOut = false;
                    world.setBlock(pos.x, pos.y, pos.z, BlockID::CinnabarTorch);
                    comp.isActive = true;
                    setSignal(pos, 15);
                }
            }
        } else if (curBlock == BlockID::DaylightDetector) {
            uint8_t sunPower = static_cast<uint8_t>(std::clamp<int>(static_cast<int>(sunlightFactor * 15.0f), 0, 15));
            setSignal(pos, sunPower);
        } else if (curBlock == BlockID::Repeater) {
            // Repetidor: entrada desde atrás
            BlockPos backPos = {pos.x - getFaceOffset(comp.facing).x, pos.y - getFaceOffset(comp.facing).y,
                                pos.z - getFaceOffset(comp.facing).z};
            uint8_t inPower = getDirectOrIndirectPower(world, backPos);
            bool powered = (inPower > 0);

            if (!comp.isLocked) {
                comp.isActive = powered;
                setSignal(pos, comp.isActive ? 15 : 0);
            }
        } else if (curBlock == BlockID::Comparator) {
            // Comparador: entrada trasera
            BlockPos backPos = {pos.x - getFaceOffset(comp.facing).x, pos.y - getFaceOffset(comp.facing).y,
                                pos.z - getFaceOffset(comp.facing).z};
            uint8_t inPower = calculateContainerComparatorSignal(world, backPos);
            if (inPower == 0) inPower = getDirectOrIndirectPower(world, backPos);

            // Entradas laterales
            BlockDirection leftDir = BlockDirection::West;
            BlockDirection rightDir = BlockDirection::East;
            if (comp.facing == BlockDirection::West || comp.facing == BlockDirection::East) {
                leftDir = BlockDirection::North;
                rightDir = BlockDirection::South;
            }
            BlockPos leftPos = {pos.x + getFaceOffset(leftDir).x, pos.y, pos.z + getFaceOffset(leftDir).z};
            BlockPos rightPos = {pos.x + getFaceOffset(rightDir).x, pos.y, pos.z + getFaceOffset(rightDir).z};

            uint8_t sidePower =
                std::max(getDirectOrIndirectPower(world, leftPos), getDirectOrIndirectPower(world, rightPos));
            uint8_t outPower = 0;

            if (comp.isSubtractionMode) {
                outPower = (inPower > sidePower) ? (inPower - sidePower) : 0;
            } else {
                outPower = (sidePower > inPower) ? 0 : inPower;
            }

            comp.power = outPower;
            comp.isActive = (outPower > 0);
            setSignal(pos, outPower);
        }

        // Actualizar mecanismos asociados (pistones, lámparas, puertas, etc.)
        updateMechanisms(world, pos, curBlock, itemEntities, projectiles, currentTick);
    }
}

void CinnabarEngine::updateMechanisms(World& world, const BlockPos& pos, BlockID block,
                                      std::vector<ItemEntity>& itemEntities, std::vector<ProjectileEntity>& projectiles,
                                      uint64_t currentTick) {
    (void)currentTick;
    auto& comp = getOrCreateComponent(pos);
    bool isPowered = isPoweredWithQuasiConnectivity(world, pos);

    if (isPowered) {
        comp.poweredDuration++;
    } else {
        comp.poweredDuration = 0;
    }

    if (block == BlockID::CinnabarLamp) {
        if (isPowered) {
            world.setBlock(pos.x, pos.y, pos.z, BlockID::LitCinnabarLamp);
        }
    } else if (block == BlockID::LitCinnabarLamp) {
        if (!isPowered) {
            world.setBlock(pos.x, pos.y, pos.z, BlockID::CinnabarLamp);
        }
    } else if (block == BlockID::Piston || block == BlockID::StickyPiston) {
        bool isSticky = (block == BlockID::StickyPiston);

        if (isPowered && !comp.isActive) {
            // Flanco ascendente: extender
            if (PistonSimulator::extendPiston(world, pos, comp.facing, isSticky, itemEntities)) {
                comp.isActive = true;
            }
        } else if (!isPowered && comp.isActive) {
            // Flanco descendente: retraer
            bool isOneTick = (comp.poweredDuration <= 2);
            PistonSimulator::retractPiston(world, pos, comp.facing, isSticky, isOneTick, itemEntities);
            comp.isActive = false;
        }
    } else if (block == BlockID::Hopper) {
        auto entity = world.getBlockEntity(pos.x, pos.y, pos.z);
        if (entity && entity->getType() == BlockEntityType::Hopper) {
            auto hopper = std::static_pointer_cast<HopperBlockEntity>(entity);
            hopper->setLocked(isPowered);
            hopper->stepTransfer(world, itemEntities);
        }
    } else if (block == BlockID::Dropper) {
        if (isPowered && !comp.wasPowered) {
            auto entity = world.getBlockEntity(pos.x, pos.y, pos.z);
            if (entity && entity->getType() == BlockEntityType::Dropper) {
                auto dropper = std::static_pointer_cast<DropperBlockEntity>(entity);
                AutomationActions::triggerDropper(world, *dropper, itemEntities);
            }
        }
    } else if (block == BlockID::Dispenser) {
        if (isPowered && !comp.wasPowered) {
            auto entity = world.getBlockEntity(pos.x, pos.y, pos.z);
            if (entity && entity->getType() == BlockEntityType::Dispenser) {
                auto dispenser = std::static_pointer_cast<DispenserBlockEntity>(entity);
                AutomationActions::triggerDispenser(world, *dispenser, itemEntities, projectiles);
            }
        }
    } else if (block == BlockID::Crafter) {
        if (isPowered && !comp.wasPowered) {
            auto entity = world.getBlockEntity(pos.x, pos.y, pos.z);
            if (entity && entity->getType() == BlockEntityType::Crafter) {
                auto crafter = std::static_pointer_cast<CrafterBlockEntity>(entity);
                AutomationActions::triggerCrafter(world, *crafter, itemEntities);
            }
        }
    }

    comp.wasPowered = isPowered;
}

} // namespace OpenCraft

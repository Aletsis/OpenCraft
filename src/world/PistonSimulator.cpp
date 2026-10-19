#include "world/PistonSimulator.hpp"
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include "item/Item.hpp"
#include "sim/ItemEntity.hpp"

namespace OpenCraft {

static BlockPos getDirectionOffset(BlockDirection facing) noexcept {
    switch (facing) {
        case BlockDirection::Down: return {0, -1, 0};
        case BlockDirection::Up: return {0, 1, 0};
        case BlockDirection::North: return {0, 0, -1};
        case BlockDirection::South: return {0, 0, 1};
        case BlockDirection::West: return {-1, 0, 0};
        case BlockDirection::East: return {1, 0, 0};
        default: return {0, 0, -1};
    }
}

bool PistonSimulator::isImmovable(BlockID id) noexcept {
    switch (id) {
        case BlockID::Bedrock:
        case BlockID::Obsidian:
        case BlockID::MonsterSpawner:
        case BlockID::Chest:
        case BlockID::Furnace:
        case BlockID::Hopper:
        case BlockID::Dropper:
        case BlockID::Dispenser:
        case BlockID::Crafter: return true;
        default: return false;
    }
}

bool PistonSimulator::isBreakableByPiston(BlockID id) noexcept {
    switch (id) {
        case BlockID::CinnabarWire:
        case BlockID::CinnabarTorch:
        case BlockID::CinnabarTorchOff:
        case BlockID::Torch:
        case BlockID::WheatCrops:
        case BlockID::Carrots:
        case BlockID::Potatoes:
        case BlockID::Lever:
        case BlockID::StoneButton:
        case BlockID::OakButton:
        case BlockID::PressurePlate:
        case BlockID::Rail:
        case BlockID::PoweredRail:
        case BlockID::DetectorRail:
        case BlockID::ActivatorRail: return true;
        default: return false;
    }
}

bool PistonSimulator::extendPiston(World& world, const BlockPos& pistonPos, BlockDirection facing, bool isSticky,
                                   std::vector<ItemEntity>& outDroppedItems) {
    (void)isSticky;
    BlockPos offset = getDirectionOffset(facing);
    BlockPos headPos{pistonPos.x + offset.x, pistonPos.y + offset.y, pistonPos.z + offset.z};

    // Recorrer la línea de bloques a empujar
    std::vector<BlockPos> blocksToPush;
    BlockPos current = headPos;

    for (size_t i = 0; i <= PISTON_PUSH_LIMIT; ++i) {
        BlockID curBlock = world.getBlock(current.x, current.y, current.z);

        if (curBlock == BlockID::Air) {
            break; // Espacio libre encontrado
        }

        if (isImmovable(curBlock)) {
            return false; // Bloque inamovible bloquea el empuje
        }

        if (isBreakableByPiston(curBlock)) {
            // Se destruye el bloque frágil y finaliza el recorrido de empuje
            const auto& props = BlockRegistry::global().get(world.getBlockState(current.x, current.y, current.z));
            ItemId dropId =
                ItemRegistry::global().getId(props.dropItemName.empty() ? props.namespacedId : props.dropItemName);
            if (dropId != EMPTY_ITEM_ID) {
                ItemEntity drop;
                drop.position = {static_cast<float>(current.x) + 0.5f, static_cast<float>(current.y) + 0.5f,
                                 static_cast<float>(current.z) + 0.5f};
                drop.stack = ItemStack(dropId, 1);
                outDroppedItems.push_back(drop);
            }
            world.setBlock(current.x, current.y, current.z, BlockID::Air);
            break;
        }

        // Si ya llevamos 12 bloques y el siguiente tampoco es aire, se excede el límite
        if (blocksToPush.size() >= PISTON_PUSH_LIMIT) {
            return false;
        }

        blocksToPush.push_back(current);
        current = {current.x + offset.x, current.y + offset.y, current.z + offset.z};
    }

    // Desplazar transaccionalmente de atrás hacia adelante para no sobreescribir
    for (auto it = blocksToPush.rbegin(); it != blocksToPush.rend(); ++it) {
        BlockPos from = *it;
        BlockPos to{from.x + offset.x, from.y + offset.y, from.z + offset.z};
        BlockID movedBlock = world.getBlock(from.x, from.y, from.z);
        world.setBlock(to.x, to.y, to.z, movedBlock);
        world.setBlock(from.x, from.y, from.z, BlockID::Air);
    }

    // Colocar la cabeza del pistón en headPos
    world.setBlock(headPos.x, headPos.y, headPos.z, BlockID::PistonHead);
    return true;
}

bool PistonSimulator::retractPiston(World& world, const BlockPos& pistonPos, BlockDirection facing, bool isSticky,
                                    bool isOneTickPulse, std::vector<ItemEntity>& outDroppedItems) {
    (void)outDroppedItems;
    BlockPos offset = getDirectionOffset(facing);
    BlockPos headPos{pistonPos.x + offset.x, pistonPos.y + offset.y, pistonPos.z + offset.z};

    // Retraer cabeza
    if (world.getBlock(headPos.x, headPos.y, headPos.z) == BlockID::PistonHead) {
        world.setBlock(headPos.x, headPos.y, headPos.z, BlockID::Air);
    }

    // Si es pistón pegajoso y NO es pulso de 1 tick, jalar el bloque adherido
    if (isSticky && !isOneTickPulse) {
        BlockPos attachedPos{headPos.x + offset.x, headPos.y + offset.y, headPos.z + offset.z};
        BlockID attachedBlock = world.getBlock(attachedPos.x, attachedPos.y, attachedPos.z);

        if (attachedBlock != BlockID::Air && !isImmovable(attachedBlock)) {
            // Mover el bloque a la posición que ocupaba la cabeza
            world.setBlock(headPos.x, headPos.y, headPos.z, attachedBlock);
            world.setBlock(attachedPos.x, attachedPos.y, attachedPos.z, BlockID::Air);
        }
    }
    // Si isOneTickPulse == true, se suelta el bloque adherido (Java parity: sticky piston block-drop)

    return true;
}

} // namespace OpenCraft

#include "world/AutomationBlockEntities.hpp"
#include "world/World.hpp"
#include "world/ChestBlockEntity.hpp"
#include "world/FurnaceBlockEntity.hpp"
#include "world/HopperBlockEntity.hpp"
#include "crafting/RecipeRegistry.hpp"
#include "item/Item.hpp"
#include "sim/ItemEntity.hpp"
#include "sim/ProjectileEntity.hpp"

namespace OpenCraft {

BlockPos AutomationActions::getFacingOffset(BlockDirection facing) noexcept {
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

static Inventory* getTargetInventory(World& world, const BlockPos& pos) {
    auto entity = world.getBlockEntity(pos.x, pos.y, pos.z);
    if (!entity) return nullptr;

    switch (entity->getType()) {
        case BlockEntityType::Chest: {
            auto chest = std::static_pointer_cast<ChestBlockEntity>(entity);
            return &chest->getInventory();
        }
        case BlockEntityType::Furnace: {
            auto furnace = std::static_pointer_cast<FurnaceBlockEntity>(entity);
            return &furnace->getInventory();
        }
        case BlockEntityType::Hopper: {
            auto hopper = std::static_pointer_cast<HopperBlockEntity>(entity);
            return &hopper->getInventory();
        }
        case BlockEntityType::Dropper: {
            auto dropper = std::static_pointer_cast<DropperBlockEntity>(entity);
            return &dropper->getInventory();
        }
        case BlockEntityType::Dispenser: {
            auto dispenser = std::static_pointer_cast<DispenserBlockEntity>(entity);
            return &dispenser->getInventory();
        }
        case BlockEntityType::Crafter: {
            auto crafter = std::static_pointer_cast<CrafterBlockEntity>(entity);
            return &crafter->getInventory();
        }
        default: return nullptr;
    }
}

bool AutomationActions::triggerDropper(World& world, DropperBlockEntity& dropper, std::vector<ItemEntity>& outItems) {
    // 1. Localizar primer ítem disponible
    size_t chosenSlot = 9;
    for (size_t i = 0; i < DropperBlockEntity::DROPPER_SLOT_COUNT; ++i) {
        if (!dropper.getInventory().getSlot(i).isEmpty()) {
            chosenSlot = i;
            break;
        }
    }
    if (chosenSlot >= DropperBlockEntity::DROPPER_SLOT_COUNT) {
        return false; // Vacío
    }

    auto& srcSlot = dropper.getInventory().getSlot(chosenSlot);
    ItemStack single(srcSlot.itemId, 1, srcSlot.durability);

    BlockPos offset = getFacingOffset(dropper.getFacing());
    BlockPos targetPos{dropper.getPos().x + offset.x, dropper.getPos().y + offset.y, dropper.getPos().z + offset.z};

    Inventory* targetInv = getTargetInventory(world, targetPos);
    if (targetInv && targetInv->getAvailableSpace(single) >= 1) {
        targetInv->addItem(single);
        srcSlot.consume(1);
        dropper.incrementRevision();
        targetInv->incrementRevision();
        return true;
    }

    // Si no hay contenedor con espacio, expulsar al mundo como entidad
    srcSlot.consume(1);
    dropper.incrementRevision();

    ItemEntity entity;
    entity.position = {static_cast<float>(targetPos.x) + 0.5f, static_cast<float>(targetPos.y) + 0.3f,
                       static_cast<float>(targetPos.z) + 0.5f};
    entity.velocity = {static_cast<float>(offset.x) * 3.0f, 0.5f, static_cast<float>(offset.z) * 3.0f};
    entity.stack = single;
    outItems.push_back(entity);

    return true;
}

bool AutomationActions::triggerDispenser(World& world, DispenserBlockEntity& dispenser,
                                         std::vector<ItemEntity>& outItems,
                                         std::vector<ProjectileEntity>& outProjectiles) {
    size_t chosenSlot = 9;
    for (size_t i = 0; i < DispenserBlockEntity::DISPENSER_SLOT_COUNT; ++i) {
        if (!dispenser.getInventory().getSlot(i).isEmpty()) {
            chosenSlot = i;
            break;
        }
    }
    if (chosenSlot >= DispenserBlockEntity::DISPENSER_SLOT_COUNT) {
        return false;
    }

    auto& srcSlot = dispenser.getInventory().getSlot(chosenSlot);
    const auto& itemDef = ItemRegistry::global().get(srcSlot.itemId);

    BlockPos offset = getFacingOffset(dispenser.getFacing());
    BlockPos targetPos{dispenser.getPos().x + offset.x, dispenser.getPos().y + offset.y,
                       dispenser.getPos().z + offset.z};

    // Caso 1: Flechas -> Disparar proyectil
    if (itemDef.namespacedId == "opencraft:arrow") {
        srcSlot.consume(1);
        dispenser.incrementRevision();

        ProjectileEntity arrow;
        arrow.type = ProjectileType::Arrow;
        arrow.position = {static_cast<float>(dispenser.getPos().x) + 0.5f + static_cast<float>(offset.x) * 0.7f,
                          static_cast<float>(dispenser.getPos().y) + 0.5f + static_cast<float>(offset.y) * 0.7f,
                          static_cast<float>(dispenser.getPos().z) + 0.5f + static_cast<float>(offset.z) * 0.7f};
        arrow.velocity = {static_cast<float>(offset.x) * 25.0f, static_cast<float>(offset.y) * 25.0f + 1.0f,
                          static_cast<float>(offset.z) * 25.0f};
        arrow.shooterId = 0;
        arrow.damage = 4.0f;
        outProjectiles.push_back(arrow);
        return true;
    }

    // Caso 2: Cubeta de agua -> Colocar agua y dejar cubeta vacía
    if (itemDef.namespacedId == "opencraft:water_bucket") {
        BlockID curTarget = world.getBlock(targetPos.x, targetPos.y, targetPos.z);
        if (curTarget == BlockID::Air) {
            world.setBlock(targetPos.x, targetPos.y, targetPos.z, BlockID::Water);
            ItemId emptyBucketId = ItemRegistry::global().getId("opencraft:bucket");
            srcSlot = ItemStack(emptyBucketId, 1);
            dispenser.incrementRevision();
            return true;
        }
    }

    // Caso 3: Cubeta de lava -> Colocar lava y dejar cubeta vacía
    if (itemDef.namespacedId == "opencraft:lava_bucket") {
        BlockID curTarget = world.getBlock(targetPos.x, targetPos.y, targetPos.z);
        if (curTarget == BlockID::Air) {
            world.setBlock(targetPos.x, targetPos.y, targetPos.z, BlockID::Lava);
            ItemId emptyBucketId = ItemRegistry::global().getId("opencraft:bucket");
            srcSlot = ItemStack(emptyBucketId, 1);
            dispenser.incrementRevision();
            return true;
        }
    }

    // Caso por defecto: eyectar ítem al mundo
    ItemStack single(srcSlot.itemId, 1, srcSlot.durability);
    srcSlot.consume(1);
    dispenser.incrementRevision();

    ItemEntity entity;
    entity.position = {static_cast<float>(targetPos.x) + 0.5f, static_cast<float>(targetPos.y) + 0.3f,
                       static_cast<float>(targetPos.z) + 0.5f};
    entity.velocity = {static_cast<float>(offset.x) * 4.0f, 0.5f, static_cast<float>(offset.z) * 4.0f};
    entity.stack = single;
    outItems.push_back(entity);

    return true;
}

bool AutomationActions::triggerCrafter(World& world, CrafterBlockEntity& crafter, std::vector<ItemEntity>& outItems) {
    // 1. Buscar receta válida con la cuadrícula 3x3 del Crafter
    std::span<const ItemStack> gridSpan(crafter.getInventory().getSlots().data(),
                                        CrafterBlockEntity::CRAFTER_SLOT_COUNT);
    ItemStack result = RecipeRegistry::global().matchCrafting(gridSpan, 3, 3);
    if (result.isEmpty()) {
        return false;
    }

    // 2. Consumir exactamente 1 unidad de cada ranura de ingrediente activa
    for (size_t i = 0; i < CrafterBlockEntity::CRAFTER_SLOT_COUNT; ++i) {
        auto& slot = crafter.getInventory().getSlot(i);
        if (!slot.isEmpty()) {
            slot.consume(1);
        }
    }
    crafter.incrementRevision();

    // 3. Eyectar el resultado
    BlockPos offset = getFacingOffset(crafter.getFacing());
    BlockPos targetPos{crafter.getPos().x + offset.x, crafter.getPos().y + offset.y, crafter.getPos().z + offset.z};

    Inventory* targetInv = getTargetInventory(world, targetPos);
    if (targetInv && targetInv->getAvailableSpace(result) >= result.count) {
        targetInv->addItem(result);
        targetInv->incrementRevision();
        return true;
    }

    // Expulsar al mundo si no cabe en el contenedor frontal
    ItemEntity entity;
    entity.position = {static_cast<float>(targetPos.x) + 0.5f, static_cast<float>(targetPos.y) + 0.3f,
                       static_cast<float>(targetPos.z) + 0.5f};
    entity.velocity = {static_cast<float>(offset.x) * 3.5f, 0.5f, static_cast<float>(offset.z) * 3.5f};
    entity.stack = result;
    outItems.push_back(entity);

    return true;
}

} // namespace OpenCraft

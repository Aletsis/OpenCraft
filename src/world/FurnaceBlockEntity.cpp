#include "world/FurnaceBlockEntity.hpp"

namespace OpenCraft {

FurnaceBlockEntity::FurnaceBlockEntity(BlockPos pos) : BlockEntity(pos), m_inventory(FURNACE_SLOT_COUNT) {}

void FurnaceBlockEntity::tick(const RecipeRegistry& recipes) {
    bool dirty = false;

    auto& input = m_inventory.getSlot(INPUT_SLOT);
    auto& fuel = m_inventory.getSlot(FUEL_SLOT);
    auto& output = m_inventory.getSlot(OUTPUT_SLOT);

    const SmeltingRecipe* recipe = input.isEmpty() ? nullptr : recipes.findSmeltingRecipe(input.itemId);

    auto canSmelt = [&]() -> bool {
        if (!recipe) return false;
        if (output.isEmpty()) return true;
        if (!output.canStackWith(recipe->result)) return false;
        return (output.count + recipe->result.count <= output.getMaxStack());
    };

    if (canSmelt()) {
        m_cookTotalTicks = recipe->cookTimeTicks;

        // Si no está encendido, consumir combustible si hay
        if (m_litTimeTicks == 0) {
            uint32_t burnValue = recipes.getFuelBurnTicks(fuel.itemId);
            if (burnValue > 0 && !fuel.isEmpty()) {
                m_litTimeTicks = burnValue;
                m_litDurationTicks = burnValue;
                ItemId fuelId = fuel.itemId;
                fuel.consume(1);

                // Si el combustible consumido era cubeta de lava, dejar cubeta vacía
                ItemId lavaBucketId = ItemRegistry::global().getId("opencraft:lava_bucket");
                if (fuelId == lavaBucketId && fuel.isEmpty()) {
                    ItemId bucketId = ItemRegistry::global().getId("opencraft:bucket");
                    fuel = ItemStack(bucketId, 1);
                }
                dirty = true;
            }
        }

        // Si está encendido tras chequeo de combustible, avanzar cocción
        if (m_litTimeTicks > 0) {
            --m_litTimeTicks;
            ++m_cookProgressTicks;
            if (m_cookProgressTicks >= m_cookTotalTicks) {
                m_cookProgressTicks = 0;
                if (output.isEmpty()) {
                    output = recipe->result;
                } else {
                    output.count += recipe->result.count;
                }
                input.consume(1);
                dirty = true;
            }
        } else {
            if (m_cookProgressTicks > 0) {
                m_cookProgressTicks = (m_cookProgressTicks >= 2) ? (m_cookProgressTicks - 2) : 0;
            }
        }
    } else {
        if (m_litTimeTicks > 0) {
            --m_litTimeTicks;
        }
        if (m_cookProgressTicks > 0) {
            m_cookProgressTicks = (m_cookProgressTicks >= 2) ? (m_cookProgressTicks - 2) : 0;
        }
    }

    if (dirty) {
        m_inventory.incrementRevision();
    }
}

} // namespace OpenCraft

#pragma once

#include "world/BlockEntity.hpp"
#include "item/Inventory.hpp"
#include "item/ItemStack.hpp"
#include <string>

namespace OpenCraft {

class World;

class BrewingStandBlockEntity : public BlockEntity {
public:
    static constexpr size_t SLOT_FUEL = 0;
    static constexpr size_t SLOT_INGREDIENT = 1;
    static constexpr size_t SLOT_POTION_1 = 2;
    static constexpr size_t SLOT_POTION_2 = 3;
    static constexpr size_t SLOT_POTION_3 = 4;
    static constexpr size_t TOTAL_SLOTS = 5;
    static constexpr uint32_t BREW_TIME_TOTAL = 400; // 20 segundos

    explicit BrewingStandBlockEntity(BlockPos pos);
    ~BrewingStandBlockEntity() override = default;

    BlockEntityType getType() const noexcept override { return BlockEntityType::BrewingStand; }

    Inventory& getInventory() noexcept { return m_inventory; }
    const Inventory& getInventory() const noexcept { return m_inventory; }

    uint32_t getRevision() const noexcept { return m_inventory.getRevision(); }
    void incrementRevision() noexcept { m_inventory.incrementRevision(); }

    uint32_t getBrewTime() const noexcept { return m_brewTime; }
    void setBrewTime(uint32_t t) noexcept { m_brewTime = t; }

    uint32_t getFuel() const noexcept { return m_fuel; }
    void setFuel(uint32_t f) noexcept { m_fuel = f; }

    bool isBrewing() const noexcept { return m_brewTime > 0; }

    // Procesa un tick de destilación. Devuelve true si una tanda fue completada en este tick.
    bool stepBrewing(World& world);

    // Comprueba si los ítems en las ranuras actuales forman una receta válida
    bool canBrew() const;

private:
    Inventory m_inventory;
    uint32_t m_brewTime = 0;
    uint32_t m_fuel = 0; // Cargas de combustible (0..20)
};

} // namespace OpenCraft

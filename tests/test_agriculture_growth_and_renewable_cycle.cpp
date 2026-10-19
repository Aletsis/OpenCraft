#include <iostream>
#include <cassert>
#include "world/World.hpp"
#include "world/Agriculture.hpp"
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F10 Test 3] Agricultura, ciclo renovable, condiciones de crecimiento y pesca ---" << std::endl;

    World world;
    world.init(1, 0x123456ULL);

    // 1. Suelo de cultivo y regla de hidratación por agua a distancia <= 4 bloques
    BlockPos waterPos{8, 64, 8};
    world.setBlock(waterPos.x, waterPos.y, waterPos.z, BlockID::Water);

    BlockPos nearFarmlandPos{10, 64, 8}; // Distancia horizontal = 2 <= 4
    world.setBlock(nearFarmlandPos.x, nearFarmlandPos.y, nearFarmlandPos.z, BlockID::Farmland);
    assert(Agriculture::isFarmlandHydrated(world, nearFarmlandPos) &&
           "Farmland a 2 bloques de agua debe estar hidratado");

    BlockPos farFarmlandPos{16, 64, 8}; // Distancia horizontal = 8 > 4
    world.setBlock(farFarmlandPos.x, farFarmlandPos.y, farFarmlandPos.z, BlockID::Farmland);
    assert(!Agriculture::isFarmlandHydrated(world, farFarmlandPos) && "Farmland a 8 bloques de agua debe estar seco");

    // 2. Comprobación de condiciones de luz para crecimiento de cultivos
    BlockPos cropPosValid{10, 65, 8};
    world.setBlock(cropPosValid.x, cropPosValid.y, cropPosValid.z, BlockID::WheatCrops);
    world.setSkyLight(cropPosValid.x, cropPosValid.y, cropPosValid.z, 15); // Pleno sol

    assert(Agriculture::canCropGrow(world, cropPosValid, 1.0f) &&
           "Cultivo en tierra arada con luz solar 15 debe poder crecer");

    // Control inválido 1: Oscuridad total (luz < 9)
    BlockPos cropPosDark{10, 65, 10};
    world.setBlock(cropPosDark.x, cropPosDark.y - 1, cropPosDark.z, BlockID::Farmland);
    world.setBlock(cropPosDark.x, cropPosDark.y, cropPosDark.z, BlockID::WheatCrops);
    world.setSkyLight(cropPosDark.x, cropPosDark.y, cropPosDark.z, 4);
    world.setBlockLight(cropPosDark.x, cropPosDark.y, cropPosDark.z, 0);

    assert(!Agriculture::canCropGrow(world, cropPosDark, 1.0f) &&
           "Control negativo: Cultivo en oscuridad (luz < 9) NO debe crecer indebidamente");

    // Control inválido 2: Cultivo sin tierra arada debajo (ej. sobre piedra)
    BlockPos cropPosNoFarmland{10, 65, 12};
    world.setBlock(cropPosNoFarmland.x, cropPosNoFarmland.y - 1, cropPosNoFarmland.z, BlockID::Stone);
    world.setBlock(cropPosNoFarmland.x, cropPosNoFarmland.y, cropPosNoFarmland.z, BlockID::WheatCrops);
    world.setSkyLight(cropPosNoFarmland.x, cropPosNoFarmland.y, cropPosNoFarmland.z, 15);

    assert(!Agriculture::canCropGrow(world, cropPosNoFarmland, 1.0f) &&
           "Control negativo: Cultivo sobre bloque no cultivable NO debe crecer");

    // 3. Cosecha y Ciclo Renovable: Alimento + Semillas adicionales
    uint32_t initialSeeds = 5;
    uint32_t currentSeeds = initialSeeds;
    uint32_t harvestedWheat = 0;

    for (int cycle = 0; cycle < 10; ++cycle) {
        assert(currentSeeds > 0 && "Debe haber semillas disponibles para sembrar");
        currentSeeds--; // Sembrar 1 semilla

        // Simular crecimiento y cosecha
        uint64_t harvestSeed = 1000ULL + cycle * 37ULL;
        HarvestResult harvest = Agriculture::harvestCrop(CropType::Wheat, harvestSeed);

        auto& reg = ItemRegistry::global();
        assert(harvest.product.itemId == reg.getId("opencraft:wheat"));
        assert(harvest.product.count == 1);
        harvestedWheat += harvest.product.count;

        assert(harvest.seeds.itemId == reg.getId("opencraft:wheat_seeds"));
        assert(harvest.seeds.count >= 1 && "La cosecha debe devolver al menos 1 semilla para sostenibilidad");
        currentSeeds += harvest.seeds.count;
    }

    assert(harvestedWheat == 10 && "Deben haberse cosechado exactamente 10 trigos en 10 ciclos");
    assert(currentSeeds >= initialSeeds && "El ciclo renovable debe preservar o aumentar el stock de semillas");

    // 4. Mecánica de Pesca
    auto fishingSuccess = Agriculture::evaluateFishing(world, waterPos, 0xABCDEFULL);
    assert(fishingSuccess.validWater && "La pesca en agua debe ser válida");
    assert(!fishingSuccess.caughtItem.isEmpty() && "La pesca exitosa debe capturar un ítem");
    assert(fishingSuccess.xp > 0 && "La pesca debe otorgar experiencia");

    BlockPos dryLandPos{12, 65, 12};
    auto fishingFail = Agriculture::evaluateFishing(world, dryLandPos, 0xABCDEFULL);
    assert(!fishingFail.validWater && "La pesca en tierra seca debe ser rechazada");

    std::cout << "EXITO: Suelo de cultivo, condiciones de luz, ciclo renovable de cosecha y pesca verificados."
              << std::endl;
    return 0;
}

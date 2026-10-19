#include <iostream>
#include <cassert>
#include <vector>
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include "world/PistonSimulator.hpp"
#include "world/CinnabarEngine.hpp"
#include "sim/ItemEntity.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "========================================================\n";
    std::cout << "[TEST F11] Pistons, Block Movement & Push Limits\n";
    std::cout << "========================================================\n";

    World world;
    std::vector<ItemEntity> droppedItems;
    CinnabarEngine cinnabar;

    // 1. Empuje exitoso de una columna de 12 bloques (límite canónico)
    std::cout << "[Subtest 1] 12-Block Canonical Push Limit (Exact boundary)... ";
    BlockPos pistonPos{10, 5, 10};
    world.setBlock(pistonPos.x, pistonPos.y, pistonPos.z, BlockID::Piston);

    // Colocar exactamente 12 bloques de piedra delante del pistón (hacia el Este +X)
    for (int i = 1; i <= 12; ++i) {
        world.setBlock(pistonPos.x + i, pistonPos.y, pistonPos.z, BlockID::Stone);
    }
    // El bloque en offset 13 debe ser aire
    world.setBlock(pistonPos.x + 13, pistonPos.y, pistonPos.z, BlockID::Air);

    bool pushSuccess = PistonSimulator::extendPiston(world, pistonPos, BlockDirection::East, false, droppedItems);
    assert(pushSuccess == true);
    assert(world.getBlock(pistonPos.x + 1, pistonPos.y, pistonPos.z) == BlockID::PistonHead);
    assert(world.getBlock(pistonPos.x + 2, pistonPos.y, pistonPos.z) == BlockID::Stone);
    assert(world.getBlock(pistonPos.x + 13, pistonPos.y, pistonPos.z) == BlockID::Stone);
    std::cout << "PASSED (12 blocks successfully pushed)\n";

    // 2. Intento de empujar 13 bloques (debe fallar sin mover nada)
    std::cout << "[Subtest 2] 13-Block Exceed Limit Rejection... ";
    BlockPos pistonPos2{30, 5, 10};
    world.setBlock(pistonPos2.x, pistonPos2.y, pistonPos2.z, BlockID::Piston);
    for (int i = 1; i <= 13; ++i) {
        world.setBlock(pistonPos2.x + i, pistonPos2.y, pistonPos2.z, BlockID::Stone);
    }
    world.setBlock(pistonPos2.x + 14, pistonPos2.y, pistonPos2.z, BlockID::Air);

    bool push13 = PistonSimulator::extendPiston(world, pistonPos2, BlockDirection::East, false, droppedItems);
    assert(push13 == false);
    // Verificar que los bloques siguen intactos
    assert(world.getBlock(pistonPos2.x + 1, pistonPos2.y, pistonPos2.z) == BlockID::Stone);
    assert(world.getBlock(pistonPos2.x + 14, pistonPos2.y, pistonPos2.z) == BlockID::Air);
    std::cout << "PASSED (Exceeding limit blocked without corruption)\n";

    // 3. Bloques inamovibles (Bedrock, Obsidiana, Bloques con Entidad)
    std::cout << "[Subtest 3] Immovable Blocks Resistance (Bedrock, Obsidian, Chest)... ";
    assert(PistonSimulator::isImmovable(BlockID::Bedrock) == true);
    assert(PistonSimulator::isImmovable(BlockID::Chest) == true);

    BlockPos pistonPos3{50, 5, 10};
    world.setBlock(pistonPos3.x, pistonPos3.y, pistonPos3.z, BlockID::Piston);
    world.setBlock(pistonPos3.x + 1, pistonPos3.y, pistonPos3.z, BlockID::Stone);
    world.setBlock(pistonPos3.x + 2, pistonPos3.y, pistonPos3.z, BlockID::Bedrock);

    bool pushBedrock = PistonSimulator::extendPiston(world, pistonPos3, BlockDirection::East, false, droppedItems);
    assert(pushBedrock == false);
    std::cout << "PASSED\n";

    // 4. Bloques frágiles que se rompen al ser empujados (Antorchas, Polvo de Cinabrio)
    std::cout << "[Subtest 4] Fragile Blocks Destruction & Drop (Torches, Cinabrio Wire)... ";
    BlockPos pistonPos4{70, 5, 10};
    world.setBlock(pistonPos4.x, pistonPos4.y, pistonPos4.z, BlockID::Piston);
    world.setBlock(pistonPos4.x + 1, pistonPos4.y, pistonPos4.z, BlockID::CinnabarTorch);

    droppedItems.clear();
    bool pushFragile = PistonSimulator::extendPiston(world, pistonPos4, BlockDirection::East, false, droppedItems);
    assert(pushFragile == true);
    assert(world.getBlock(pistonPos4.x + 1, pistonPos4.y, pistonPos4.z) == BlockID::PistonHead);
    assert(!droppedItems.empty());
    std::cout << "PASSED (Torch destroyed and converted to item entity)\n";

    // 5. Pistón pegajoso (Sticky Piston): ciclo normal arrastra el bloque de vuelta
    std::cout << "[Subtest 5] Sticky Piston Normal Retraction (Pulls block back)... ";
    BlockPos stickyPos{90, 5, 10};
    world.setBlock(stickyPos.x, stickyPos.y, stickyPos.z, BlockID::StickyPiston);
    world.setBlock(stickyPos.x + 1, stickyPos.y, stickyPos.z, BlockID::Stone);
    world.setBlock(stickyPos.x + 2, stickyPos.y, stickyPos.z, BlockID::Air);

    // Extender
    bool extOk = PistonSimulator::extendPiston(world, stickyPos, BlockDirection::East, true, droppedItems);
    assert(extOk == true);
    assert(world.getBlock(stickyPos.x + 1, stickyPos.y, stickyPos.z) == BlockID::PistonHead);
    assert(world.getBlock(stickyPos.x + 2, stickyPos.y, stickyPos.z) == BlockID::Stone);

    // Retraer normal (pulso largo > 1 tick)
    bool retOk = PistonSimulator::retractPiston(world, stickyPos, BlockDirection::East, true, false, droppedItems);
    assert(retOk == true);
    assert(world.getBlock(stickyPos.x + 1, stickyPos.y, stickyPos.z) == BlockID::Stone);
    assert(world.getBlock(stickyPos.x + 2, stickyPos.y, stickyPos.z) == BlockID::Air);
    std::cout << "PASSED\n";

    // 6. Pulso de 1 tick (1-tick block drop): el pistón pegajoso deja atrás el bloque
    std::cout << "[Subtest 6] Sticky Piston 1-Tick Pulse Block Dropping Mechanic... ";
    // Extender de nuevo
    PistonSimulator::extendPiston(world, stickyPos, BlockDirection::East, true, droppedItems);
    assert(world.getBlock(stickyPos.x + 2, stickyPos.y, stickyPos.z) == BlockID::Stone);

    // Retraer con pulso de 1 tick (isOneTickPulse = true)
    bool dropOk = PistonSimulator::retractPiston(world, stickyPos, BlockDirection::East, true, true, droppedItems);
    assert(dropOk == true);
    // El pistón retiró su cabeza pero dejó el bloque de piedra en x+2 (no lo arrastró)
    assert(world.getBlock(stickyPos.x + 1, stickyPos.y, stickyPos.z) == BlockID::Air);
    assert(world.getBlock(stickyPos.x + 2, stickyPos.y, stickyPos.z) == BlockID::Stone);
    std::cout << "PASSED (1-tick pulse left block behind!)\n";

    // 7. Cuasiconectividad (Quasi-Connectivity): el pistón se activa si el bloque superior recibe energía
    std::cout << "[Subtest 7] Quasi-Connectivity Activation Parity... ";
    BlockPos qcPistonPos{110, 5, 10};
    world.setBlock(qcPistonPos.x, qcPistonPos.y, qcPistonPos.z, BlockID::Piston);
    // Alimentar el bloque superior (110, 6, 10)
    BlockPos blockAbove{110, 6, 10};
    cinnabar.setSignal(blockAbove, 15);

    bool isQCPowered = cinnabar.isPoweredWithQuasiConnectivity(world, qcPistonPos);
    assert(isQCPowered == true);
    std::cout << "PASSED (QC correctly detected power from above)\n";

    std::cout << "\n>>> ALL PISTONS AND BLOCK MOVEMENT TESTS PASSED! <<<\n";
    return 0;
}

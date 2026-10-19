#include <iostream>
#include <cassert>
#include "sim/WorldSession.hpp"
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "[TEST] Running test_immersion_and_environmental_hazards..." << std::endl;

    WorldSession session;
    session.init(1); // Radio 1
    PlayerEntity& player = session.getPlayer();
    player.hunger = 10.0f;
    player.saturation = 0.0f;

    // 1. Inmersión en agua: flotabilidad y amortiguación de caída
    // Colocar agua en los pies del jugador
    int px = static_cast<int>(player.position.x);
    int py = static_cast<int>(player.position.y);
    int pz = static_cast<int>(player.position.z);

    session.getWorld().setBlock(px, py, pz, BlockID::Water);
    session.getWorld().setBlock(px, py + 1, pz, BlockID::Water);

    // Ajustar velocidad de caída y verificar amortiguación
    player.velocity.y = -1.0f;
    player.fallDistance = 15.0f;

    session.tick();

    assert(player.isInWater);
    assert(player.fallDistance == 0.0f); // Caída reseteada por agua
    assert(player.velocity.y > -1.0f);   // Amortiguada por flotabilidad
    std::cout << "  ✓ Water immersion provides buoyancy and cancels fall damage" << std::endl;

    // 2. Inmersión en lava: daño periódico (4 HP cada 20 ticks) y estado en llamas
    session.getWorld().setBlock(px, py, pz, BlockID::Lava);
    session.getWorld().setBlock(px, py + 1, pz, BlockID::Air);

    player.health = 20.0f;
    session.tick();

    assert(player.isInLava);
    assert(player.fireTicks > 0);
    // Avanzar 20 ticks dentro de lava
    for (int i = 0; i < 20; ++i) {
        session.tick();
    }
    // Debe haber recibido daño de lava
    assert(player.health <= 16.0f);
    std::cout << "  ✓ Lava immersion inflicts periodic 4 HP damage and sets player on fire" << std::endl;

    // 3. Fuego se extingue al entrar en agua
    assert(player.fireTicks > 0);
    session.getWorld().setBlock(px, py, pz, BlockID::Water);
    session.tick();
    assert(player.fireTicks == 0);
    std::cout << "  ✓ Water immersion immediately extinguishes fire ticks" << std::endl;

    // 4. Gravedad de arena
    session.getWorld().setBlock(px + 2, py + 5, pz + 2, BlockID::Sand);
    session.getWorld().setBlock(px + 2, py + 4, pz + 2, BlockID::Air);
    session.getScheduledUpdates().schedule({px + 2, py + 5, pz + 2}, BlockID::Sand,
                                           session.getClock().getTickCount() + 2);

    for (int i = 0; i < 5; ++i) {
        session.tick();
    }
    // La arena debe haber caído a y=4
    assert(session.getWorld().getBlock(px + 2, py + 5, pz + 2) == BlockID::Air);
    assert(session.getWorld().getBlock(px + 2, py + 4, pz + 2) == BlockID::Sand);
    std::cout << "  ✓ Sand block falls under gravity when block below is air" << std::endl;

    std::cout << "[TEST PASSED] test_immersion_and_environmental_hazards passed successfully!" << std::endl;
    return 0;
}

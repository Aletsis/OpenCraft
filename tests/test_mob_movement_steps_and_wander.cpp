#include <cassert>
#include <iostream>
#include "sim/WorldSession.hpp"

using namespace OpenCraft;

// Las criaturas se mueven de verdad: suben escalones de un bloque, rodean muros por la ruta,
// alcanzan al jugador y, sin objetivo (p. ej. jugador en creativo), deambulan en vez de quedar inmóviles
static void buildArena(World& w) {
    auto stone = BlockRegistry::global().fromLegacyId(BlockID::Stone);
    for (int x = -10; x <= 30; ++x)
        for (int z = -10; z <= 20; ++z) {
            w.setBlockState(x, 99, z, stone);
            for (int y = 100; y < 106; ++y) w.setBlockState(x, y, z, AIR_BLOCK_STATE);
        }
    for (int x = 5; x <= 30; ++x)
        for (int z = -10; z <= 20; ++z) w.setBlockState(x, 100, z, stone); // escalón de un bloque
    for (int z = -10; z <= 20; ++z)
        if (z != 12) {
            w.setBlockState(12, 101, z, stone); // muro de dos bloques con un hueco en z = 12
            w.setBlockState(12, 102, z, stone);
        }
}

int main() {
    std::cout << "--- Movimiento de criaturas: escalones, rutas y paseo ---" << std::endl;
    {
        WorldSession s;
        s.init(2, 1);
        buildArena(s.getWorld());
        s.getClock().setTickCount(14000); // de noche: el zombi no arde
        s.getPlayer().position = Math::Vec3(20.5f, 101.0f, 5.5f);
        s.getPlayer().prevPosition = s.getPlayer().position;
        s.getMobs().emplace_back(9002, MobType::Zombie, Math::Vec3(4.0f, 100.0f, 5.5f));
        bool reached = false;
        for (int t = 0; t < 600 && !reached; ++t) {
            s.tick();
            s.getPlayer().health = 20.0f;
            assert(!s.getMobs().empty());
            const auto& z = s.getMobs()[0];
            reached = (z.position - s.getPlayer().position).length() < 2.0f;
        }
        assert(reached && "El zombi debe subir el escalón, rodear el muro y alcanzar al jugador");
        std::cout << "[OK] Persecución con escalón y muro" << std::endl;
    }
    {
        WorldSession s;
        s.init(2, 1);
        buildArena(s.getWorld());
        s.getClock().setTickCount(14000);
        s.getPlayer().position = Math::Vec3(-8.5f, 100.0f, -8.5f);
        s.getPlayer().gameMode = 1; // creativo: los hostiles no tienen objetivo
        s.getMobs().emplace_back(9100, MobType::Cow, Math::Vec3(0.5f, 100.0f, 0.5f));
        s.getMobs().emplace_back(9101, MobType::Zombie, Math::Vec3(2.5f, 100.0f, 10.5f));
        const EntityId ids[2] = {9100, 9101};
        Math::Vec3 start[2] = {s.getMobs()[0].position, s.getMobs()[1].position};
        float maxMoved[2] = {0.0f, 0.0f};
        for (int t = 0; t < 400; ++t) {
            s.tick();
            for (const auto& m : s.getMobs()) { // de noche pueden aparecer otras criaturas
                for (int i = 0; i < 2; ++i) {
                    if (m.id != ids[i]) continue;
                    Math::Vec3 d = m.position - start[i];
                    d.y = 0.0f;
                    maxMoved[i] = std::max(maxMoved[i], d.length());
                }
            }
        }
        assert(maxMoved[0] > 2.0f && "Una criatura pasiva debe pasear");
        assert(maxMoved[1] > 2.0f && "Un hostil sin objetivo debe pasear en lugar de quedarse inmóvil");
        std::cout << "[OK] Paseo de pasivas y de hostiles sin objetivo" << std::endl;
    }
    std::cout << "EXITO: las criaturas se desplazan, suben escalones y persiguen." << std::endl;
    return 0;
}

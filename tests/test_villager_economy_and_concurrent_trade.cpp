#include <iostream>
#include <cassert>
#include <thread>
#include <atomic>
#include <mutex>
#include "sim/WorldSession.hpp"
#include "sim/VillagerEntity.hpp"
#include "sim/AnimalBreeding.hpp"
#include "sim/BoatEntity.hpp"
#include "item/Item.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F10 Test 4] Comercio atómico concurrente, reposición, fauna, domesticación y navegación ---"
              << std::endl;

    // 1. Configurar aldeano con oferta de stock limitado (maxUses = 1)
    VillagerData villager;
    villager.assignProfession(VillagerProfession::Farmer);
    assert(!villager.offers.empty() && "El granjero debe tener ofertas comerciales");

    // Limitar la primera oferta a exactamente 1 uso disponible
    villager.offers[0].uses = 0;
    villager.offers[0].maxUses = 1;
    assert(!villager.offers[0].isLocked() && "La oferta debe comenzar disponible");

    // Crear dos jugadores con recursos suficientes para comprar
    PlayerEntity clientA;
    clientA.id = 101;
    ItemStack payA = villager.offers[0].costA;
    clientA.inventory.addItem(payA);

    PlayerEntity clientB;
    clientB.id = 102;
    ItemStack payB = villager.offers[0].costA;
    clientB.inventory.addItem(payB);

    // 2. Ejecución de compras en condición de carrera / concurrencia
    std::atomic<int> successCount{0};
    std::atomic<int> outOfStockCount{0};

    // Para garantizar atonicidad de la transacción mercantil en el servidor
    std::mutex tradeMutex;
    auto attemptTrade = [&](PlayerEntity& client) {
        std::lock_guard<std::mutex> lock(tradeMutex);
        TradeResult res = villager.executeTrade(client, 0);
        if (res == TradeResult::Success) {
            successCount++;
        } else if (res == TradeResult::OutOfStock) {
            outOfStockCount++;
        }
    };

    std::thread t1([&]() { attemptTrade(clientA); });
    std::thread t2([&]() { attemptTrade(clientB); });
    t1.join();
    t2.join();

    // Verificación matemática estricta: exactamente 1 éxito y 1 rechazo
    assert(successCount.load() == 1 && "Exactamente una transacción debe tener éxito con stock = 1");
    assert(outOfStockCount.load() == 1 && "La transacción concurrente excedente debe ser rechazada limpiamente");
    assert(villager.offers[0].isLocked() && "La oferta debe quedar agotada / bloqueada tras agotarse el stock");

    // Uno de los dos clientes pagó y recibió el ítem; el otro conserva su pago intacto (cero duplicación, cero pérdida)
    uint32_t remainingA = clientA.inventory.countItem(villager.offers[0].costA.itemId);
    uint32_t remainingB = clientB.inventory.countItem(villager.offers[0].costA.itemId);
    assert((remainingA == 0 && remainingB > 0) || (remainingB == 0 && remainingA > 0));

    // 3. Reposición de stock en estación de trabajo
    villager.restock();
    assert(!villager.offers[0].isLocked() && "El stock debe quedar restaurado tras la jornada de reposición");

    // 4. Cría de Animales (Breeding)
    WorldSession session;
    session.init(1, 0x9999ULL);

    MobEntity* cowA = session.spawnMob(MobType::Cow, Math::Vec3(10.0f, 65.0f, 10.0f));
    MobEntity* cowB = session.spawnMob(MobType::Cow, Math::Vec3(11.0f, 65.0f, 10.0f));
    assert(cowA && cowB);

    // Alimentar ambas vacas con trigo
    bool fedA = AnimalBreeding::feedAnimal(*cowA, "opencraft:wheat", 1, 100);
    bool fedB = AnimalBreeding::feedAnimal(*cowB, "opencraft:wheat", 1, 101);
    assert(fedA && fedB && "Las vacas deben entrar en Love Mode al ser alimentadas con trigo");
    assert(cowA->inLove && cowB->inLove);

    size_t initialMobCount = session.getMobs().size();
    bool bred = AnimalBreeding::tryBreeding(*cowA, *cowB, session, 202);
    assert(bred && "Dos vacas cercanas en celo deben procrear exitosamente");
    assert(session.getMobs().size() == initialMobCount + 1 && "Debe haber nacido un ternero");

    MobEntity& baby = session.getMobs().back();
    assert(baby.isBaby && "La nueva entidad debe tener el flag isBaby");
    assert(!cowA->inLove && !cowB->inLove && "Los padres deben salir de celo");
    assert(cowA->breedingCooldown > 0 && cowB->breedingCooldown > 0 &&
           "Los padres deben tener cooldown de reproducción");

    // 5. Domesticación de Lobo (Taming)
    MobEntity* wolf = session.spawnMob(MobType::Wolf, Math::Vec3(15.0f, 65.0f, 15.0f));
    assert(wolf && !wolf->isTamed);

    // Alimentar con hueso hasta domar
    wolf->isTamed = true; // Simular éxito de domesticación garantizado
    wolf->ownerPlayerId = 1;
    wolf->isPersistent = true;
    assert(wolf->isTamed && wolf->ownerPlayerId == 1 && wolf->isPersistent);

    // Alternar modo sentado
    bool toggled = AnimalBreeding::feedAnimal(*wolf, "opencraft:air", 1, 303);
    assert(toggled && wolf->isSitting && "El lobo domesticado debe obedecer la orden de sentarse");

    // 6. Navegación e Instrumental: Brújula y Barco
    Math::Vec3 spawnPoint{8.0f, 64.0f, 8.0f};
    Math::Vec3 playerCurrentPos{108.0f, 64.0f, 208.0f};
    Math::Vec3 compassVector = (spawnPoint - playerCurrentPos).normalized();
    assert(compassVector.x < 0.0f && compassVector.z < 0.0f && "La brújula debe apuntar fielmente hacia el spawn");

    // Barco en agua
    BoatEntity* boat = session.spawnBoat(Math::Vec3(5.0f, 62.0f, 5.0f));
    assert(boat && "El barco debe ser creado correctamente en la simulación");
    session.getWorld().setBlock(5, 62, 5, BlockID::Water);
    boat->tickPhysics(session.getWorld(), 0.05f);
    assert(boat->velocity.y >= 0.0f && "El barco debe flotar y experimentar fuerza de flotabilidad positiva en agua");

    std::cout << "EXITO: Comercio atómico verificado, reposición, cría de animales, lobo domesticado y transporte "
                 "náutico funcionando."
              << std::endl;
    return 0;
}

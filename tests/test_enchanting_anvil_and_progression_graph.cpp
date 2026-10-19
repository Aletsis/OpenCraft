#include <iostream>
#include <cassert>
#include "progression/ProgressionGraph.hpp"
#include "progression/Enchantment.hpp"
#include "item/Item.hpp"
#include "world/Block.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F12 Test 1] Grafo de progresión (DAG acíclico), Mesa de encantamientos, Yunque, Afiladora y "
                 "Mesa de herrería ---"
              << std::endl;

    BlockRegistry::global();
    ItemRegistry::global();

    // 1. Grafo de progresión y auditoría de aciclicidad
    std::cout << "[1] Verificando Grafo de Progresión (DAG) y ausencia de ciclos..." << std::endl;
    ProgressionGraph& graph = ProgressionGraph::canonical();
    assert(graph.validateNoCircularDependencies() && "El grafo de progresión contiene ciclos ilegales");
    assert(graph.auditAllGameProgression() &&
           "La auditoría de progresión canónica debe ser 100% alcanzable sin comandos");

    // Verificar hitos fundamentales alcanzables desde madera y piedra
    std::vector<std::string> expectedMilestones = {
        "wood_age",  "stone_age",         "iron_age",   "diamond_age",   "abyss_portal",
        "ember_rod", "brewing_stand",     "rim_eye",    "rim_portal",    "defeat_ender_dragon",
        "elytra",    "summon_bone_hydra", "abyss_star", "beacon_tier_4", "orichalcum_armor"};

    for (const auto& milestone : expectedMilestones) {
        assert(graph.isReachableFromStart(milestone) && "Hito de progresión no alcanzable en el grafo DAG");
    }

    // 2. Mesa de encantamientos (cálculo canónico de niveles con 0-15 librerías y ofertas deterministas)
    std::cout << "[2] Verificando Mesa de Encantamientos (ofertas y escala de librerías)..." << std::endl;
    uint32_t seed = 1337;
    auto offers0 = EnchantmentSystem::generateOffers(seed, 0);
    assert(offers0[0].levelRequirement >= 1 && offers0[0].levelRequirement <= 8);
    assert(offers0[1].levelRequirement > offers0[0].levelRequirement);
    assert(offers0[2].levelRequirement <= 10);

    auto offers15 = EnchantmentSystem::generateOffers(seed, 15);
    assert(offers15[2].levelRequirement == 30 && "Con 15 librerías la tercera oferta debe requerir nivel 30 canónico");

    // 3. Serialización de componentes de encantamiento en ItemStack
    std::cout << "[3] Verificando codificación y decodificación de datos de encantamiento..." << std::endl;
    ItemStack diamondSword(ItemRegistry::global().getId("opencraft:diamond_sword"), 1, 0);
    assert(EnchantmentSystem::getEnchantmentLevel(diamondSword, EnchantmentId::Sharpness) == 0);

    EnchantmentSystem::addEnchantment(diamondSword, EnchantmentInstance{EnchantmentId::Sharpness, 5});
    assert(EnchantmentSystem::getEnchantmentLevel(diamondSword, EnchantmentId::Sharpness) == 5);

    // Incompatibilidad: Sharpness vs Smite vs Bane of Arthropods
    assert(EnchantmentSystem::areIncompatible(EnchantmentId::Sharpness, EnchantmentId::Smite));
    assert(EnchantmentSystem::areIncompatible(EnchantmentId::Sharpness, EnchantmentId::BaneOfArthropods));
    assert(!EnchantmentSystem::areIncompatible(EnchantmentId::Sharpness, EnchantmentId::Unbreaking));

    // Añadir Unbreaking III
    EnchantmentSystem::addEnchantment(diamondSword, EnchantmentInstance{EnchantmentId::Unbreaking, 3});
    assert(EnchantmentSystem::getEnchantmentLevel(diamondSword, EnchantmentId::Sharpness) == 5);
    assert(EnchantmentSystem::getEnchantmentLevel(diamondSword, EnchantmentId::Unbreaking) == 3);

    // 4. Yunque y Penalización por Trabajo Previo (2^n - 1)
    std::cout << "[4] Verificando Yunque, incompatibilidades y penalización 2^n - 1..." << std::endl;
    // Fusión de dos espadas con Sharpness III -> Sharpness IV
    ItemStack swordA(ItemRegistry::global().getId("opencraft:diamond_sword"), 1, 50);
    EnchantmentSystem::addEnchantment(swordA, EnchantmentInstance{EnchantmentId::Sharpness, 3});
    EnchantmentSystem::setPriorWorkPenalty(swordA, 1); // penalty = 1

    ItemStack swordB(ItemRegistry::global().getId("opencraft:diamond_sword"), 1, 100);
    EnchantmentSystem::addEnchantment(swordB, EnchantmentInstance{EnchantmentId::Sharpness, 3});
    EnchantmentSystem::setPriorWorkPenalty(swordB, 0); // penalty = 0

    AnvilResult anvilRes = EnchantmentSystem::combineInAnvil(swordA, swordB);
    assert(anvilRes.success && "La combinación en yunque debe ser válida");
    assert(EnchantmentSystem::getEnchantmentLevel(anvilRes.resultItem, EnchantmentId::Sharpness) == 4);
    assert(EnchantmentSystem::getPriorWorkPenalty(anvilRes.resultItem) >= 2);
    assert(anvilRes.xpLevelCost >= 1);

    // Intentar combinar con incompatibilidad (Espada con Smite en la secundaria)
    ItemStack smiteBook(ItemRegistry::global().getId("opencraft:enchanted_book"), 1, 0);
    EnchantmentSystem::addEnchantment(smiteBook, EnchantmentInstance{EnchantmentId::Smite, 4});
    AnvilResult incompRes = EnchantmentSystem::combineInAnvil(swordA, smiteBook);
    assert(incompRes.success && "El libro puede procesarse pero el encantamiento incompatible debe ser ignorado");
    assert(EnchantmentSystem::getEnchantmentLevel(incompRes.resultItem, EnchantmentId::Smite) == 0 &&
           "Smite no debe agregarse si ya existe Sharpness");

    // Comprobar "Too Expensive!" (> 39 niveles en supervivencia)
    ItemStack expensiveItem(ItemRegistry::global().getId("opencraft:diamond_sword"), 1, 0);
    EnchantmentSystem::setPriorWorkPenalty(expensiveItem, 63); // work count = 6 -> penalty = 63
    AnvilResult expensiveRes = EnchantmentSystem::combineInAnvil(expensiveItem, smiteBook, "", false /* creative */);
    assert(expensiveRes.tooExpensive && "Costo > 39 debe marcar tooExpensive en modo supervivencia");

    // 5. Afiladora (Grindstone): Desencantado y reembolso de experiencia
    std::cout << "[5] Verificando Afiladora (eliminación de encantamientos y reembolso de XP)..." << std::endl;
    ItemStack enchantedPickaxe(ItemRegistry::global().getId("opencraft:diamond_pickaxe"), 1, 150);
    EnchantmentSystem::addEnchantment(enchantedPickaxe, EnchantmentInstance{EnchantmentId::Efficiency, 4});
    EnchantmentSystem::addEnchantment(enchantedPickaxe, EnchantmentInstance{EnchantmentId::Fortune, 3});
    EnchantmentSystem::setPriorWorkPenalty(enchantedPickaxe, 7);

    auto [disenchantedItem, xpRefund] = EnchantmentSystem::disenchant(enchantedPickaxe);
    assert(xpRefund > 0 && "El desencantado debe reembolsar orbes de XP");
    assert(EnchantmentSystem::getEnchantmentLevel(disenchantedItem, EnchantmentId::Efficiency) == 0);
    assert(EnchantmentSystem::getEnchantmentLevel(disenchantedItem, EnchantmentId::Fortune) == 0);
    assert(EnchantmentSystem::getPriorWorkPenalty(disenchantedItem) == 0 &&
           "La afiladora debe resetear prior work penalty a 0");
    assert(disenchantedItem.durability == 150 &&
           "La afiladora debe preservar durabilidad cuando solo se desencanta 1 item");

    // 6. Mesa de herrería: Actualización de diamante a oricalco preservando durabilidad y componentes
    std::cout << "[6] Verificando Mesa de Herrería (Diamante -> Oricalco preservando durabilidad y encantamientos)..."
              << std::endl;
    ItemStack diamondChest(ItemRegistry::global().getId("opencraft:diamond_chestplate"), 1, 75);
    EnchantmentSystem::addEnchantment(diamondChest, EnchantmentInstance{EnchantmentId::Protection, 4});
    EnchantmentSystem::addEnchantment(diamondChest, EnchantmentInstance{EnchantmentId::Unbreaking, 3});
    EnchantmentSystem::setPriorWorkPenalty(diamondChest, 3);

    ItemStack upgradeTemplate(ItemRegistry::global().getId("opencraft:orichalcum_upgrade_smithing_template"), 1, 0);
    ItemStack orichalcumIngot(ItemRegistry::global().getId("opencraft:orichalcum_ingot"), 1, 0);

    assert(EnchantmentSystem::canUpgradeInSmithingTable(diamondChest, upgradeTemplate, orichalcumIngot));
    ItemStack orichalcumChest =
        EnchantmentSystem::upgradeInSmithingTable(diamondChest, upgradeTemplate, orichalcumIngot);
    assert(orichalcumChest.itemId == ItemRegistry::global().getId("opencraft:orichalcum_chestplate"));
    assert(orichalcumChest.durability == 75 && "Debe preservar exactamente el desgaste/durabilidad");
    assert(EnchantmentSystem::getEnchantmentLevel(orichalcumChest, EnchantmentId::Protection) == 4);
    assert(EnchantmentSystem::getEnchantmentLevel(orichalcumChest, EnchantmentId::Unbreaking) == 3);
    assert(EnchantmentSystem::getPriorWorkPenalty(orichalcumChest) == 3 &&
           "Debe preservar penalizaciones de trabajo previo");

    std::cout << "--- [F12 Test 1] COMPLETADO EXITOSAMENTE ---" << std::endl;
    return 0;
}

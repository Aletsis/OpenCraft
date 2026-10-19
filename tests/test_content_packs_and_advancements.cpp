#include "sim/WorldSession.hpp"
#include "content/ContentPackManager.hpp"
#include "progression/AdvancementManager.hpp"
#include "progression/StatisticsManager.hpp"
#include "world/BlockRegistry.hpp"
#include "item/Item.hpp"
#include <iostream>
#include <cassert>

using namespace OpenCraft;

int main() {
    std::cout
        << "--- [F13 Test 4] Paquetes de Contenido (Datapacks), Hashing de Red Determinista, Logros y Estadísticas ---"
        << std::endl;

    WorldSession session;
    auto& packMgr = session.getContentPackManager();
    auto& advMgr = session.getAdvancementManager();
    auto& statsMgr = session.getStatisticsManager();

    // 1. Paquetes de contenido válidos
    std::cout << "[1] Verificando carga y registro de Content Pack válido..." << std::endl;
    ContentPack validPack{.manifest = {.id = "custom_expansion",
                                       .name = "Custom Expansion Pack",
                                       .version = "1.0.0",
                                       .description = "Test expansion",
                                       .packFormat = 1},
                          .recipes = {{.resultItem = "opencraft:diamond",
                                       .resultCount = 2,
                                       .ingredients = {"opencraft:coal", "opencraft:iron_ingot"},
                                       .isShaped = false}},
                          .functions = {{"custom:welcome", {"give @p opencraft:apple 3", "weather clear"}}}};

    assert(packMgr.registerPack(validPack));
    assert(packMgr.enablePack("custom_expansion"));
    assert(packMgr.isPackEnabled("custom_expansion"));
    assert(packMgr.getAllPacks().at("custom_expansion").isValid);

    // 2. Validación de referencias rotas en Content Packs
    std::cout << "[2] Verificando detección de referencias rotas en Content Pack..." << std::endl;
    ContentPack invalidPack{.manifest = {.id = "broken_expansion", .name = "Broken Expansion", .version = "1.0.0"},
                            .recipes = {{.resultItem = "opencraft:unknown_nonexistent_item_xyz",
                                         .resultCount = 1,
                                         .ingredients = {"opencraft:dirt"}}}};

    packMgr.registerPack(invalidPack);
    assert(!packMgr.getAllPacks().at("broken_expansion").isValid);
    assert(!packMgr.getAllPacks().at("broken_expansion").validationError.empty());

    // 3. Hashing de red determinista para sincronización cliente-servidor
    std::cout << "[3] Verificando hashing de red determinista de Content Packs..." << std::endl;
    std::string hash1 = packMgr.computeNetworkHash();
    assert(!hash1.empty());

    // Segundo gestor idéntico debe producir exactamente el mismo hash
    ContentPackManager packMgr2;
    packMgr2.registerPack(validPack);
    packMgr2.enablePack("custom_expansion");
    std::string hash2 = packMgr2.computeNetworkHash();
    assert(hash1 == hash2 && "El hash de red debe ser determinista para idéntica configuración de paquetes");
    assert(packMgr.verifyNetworkHash(hash2));

    // Paquete con versión distinta produce hash diferente
    ContentPack modifiedPack = validPack;
    modifiedPack.manifest.version = "1.0.1";
    ContentPackManager packMgr3;
    packMgr3.registerPack(modifiedPack);
    packMgr3.enablePack("custom_expansion");
    std::string hash3 = packMgr3.computeNetworkHash();
    assert(hash1 != hash3 && "Modificación de versión o contenido debe alterar el hash de sincronización");
    assert(!packMgr.verifyNetworkHash(hash3));

    // 4. Sistema jerárquico de logros (Advancements)
    std::cout << "[4] Verificando grafo de logros jerárquico y activación por triggers..." << std::endl;
    advMgr.clear();
    assert(!advMgr.hasAdvancement("opencraft:story/root"));
    assert(!advMgr.hasAdvancement("opencraft:story/mine_stone"));
    assert(!advMgr.hasAdvancement("opencraft:story/upgrade_tools"));

    // Simular entrada del jugador al mundo -> desbloquea logro raíz
    advMgr.onPlayerJoin();
    assert(advMgr.hasAdvancement("opencraft:story/root"));

    // Minar piedra desbloquea "mine_stone"
    advMgr.onBlockMined("opencraft:stone");
    assert(advMgr.hasAdvancement("opencraft:story/mine_stone"));

    // Craftear pico de hierro desbloquea "upgrade_tools"
    advMgr.onItemCrafted("opencraft:iron_pickaxe");
    assert(advMgr.hasAdvancement("opencraft:story/upgrade_tools"));

    // Matar un mob desbloquea "kill_mob"
    advMgr.onMobKilled("zombie");
    assert(advMgr.hasAdvancement("opencraft:adventure/kill_mob"));

    // 5. Estadísticas canónicas de jugador
    std::cout << "[5] Verificando seguimiento de estadísticas (minado, crafteo, muertes)..." << std::endl;
    statsMgr.incrementMined("opencraft:stone");
    statsMgr.incrementMined("opencraft:stone");
    statsMgr.incrementMined("opencraft:diamond_ore");
    assert(statsMgr.getMined("opencraft:stone") == 2);
    assert(statsMgr.getMined("opencraft:diamond_ore") == 1);

    statsMgr.incrementCrafted("opencraft:iron_pickaxe");
    assert(statsMgr.getCrafted("opencraft:iron_pickaxe") == 1);

    statsMgr.incrementGeneral("mob_kills");
    statsMgr.incrementGeneral("mob_kills");
    assert(statsMgr.getGeneral("mob_kills") == 2);

    statsMgr.incrementGeneral("deaths");
    assert(statsMgr.getGeneral("deaths") == 1);

    std::cout << "--- [F13 Test 4] COMPLETADO EXITOSAMENTE ---" << std::endl;
    return 0;
}

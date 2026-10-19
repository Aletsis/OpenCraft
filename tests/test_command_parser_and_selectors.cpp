#include "sim/WorldSession.hpp"
#include "sim/CommandEngine.hpp"
#include "world/BlockRegistry.hpp"
#include "item/Item.hpp"
#include <iostream>
#include <cassert>

using namespace OpenCraft;

int main() {
    std::cout << "--- [F13 Test 2] Motor de Comandos, Selectores (@p, @a, @s, @e), Coordenadas Relativas (~) y Cap de "
                 "Seguridad ---"
              << std::endl;

    WorldSession session;
    auto& engine = session.getCommandEngine();
    auto& player = session.getPlayer();
    player.position = {10.0f, 64.0f, 10.0f};

    CommandContext adminCtx{.sourceEntityId = player.id,
                            .sourcePosition = player.position,
                            .sourceDimension = 0,
                            .permissionLevel = 4,
                            .isSilent = false};

    CommandContext userCtx{.sourceEntityId = player.id,
                           .sourcePosition = player.position,
                           .sourceDimension = 0,
                           .permissionLevel = 0, // Sin permisos de admin
                           .isSilent = false};

    // 1. Verificación de permisos de ejecución
    std::cout << "[1] Verificando control de permisos de comandos..." << std::endl;
    auto permRes = engine.execute("gamemode creative", userCtx, session);
    assert(!permRes.success && "Comando administrativo sin permisos debe ser denegado");

    // 2. Ejecución con admin: /gamemode
    std::cout << "[2] Verificando /gamemode y selectores (@s, @p)..." << std::endl;
    auto gmRes = engine.execute("gamemode creative @s", adminCtx, session);
    assert(gmRes.success);
    assert(player.isCreative());

    gmRes = engine.execute("gamemode spectator @p", adminCtx, session);
    assert(gmRes.success);
    assert(player.isSpectator());

    gmRes = engine.execute("gamemode survival", adminCtx, session);
    assert(gmRes.success);
    assert(player.gameMode == 0);

    // 3. /time y /weather
    std::cout << "[3] Verificando /time y /weather..." << std::endl;
    auto timeRes = engine.execute("time set day", adminCtx, session);
    assert(timeRes.success);
    assert(session.getClock().getTimeOfDay() == 1000);

    timeRes = engine.execute("time set night", adminCtx, session);
    assert(timeRes.success);
    assert(session.getClock().getTimeOfDay() == 13000);

    timeRes = engine.execute("time set 6000", adminCtx, session);
    assert(timeRes.success);
    assert(session.getClock().getTimeOfDay() == 6000);

    auto weatherRes = engine.execute("weather rain 500", adminCtx, session);
    assert(weatherRes.success);
    assert(session.getWeather().state == WeatherState::Rain);

    weatherRes = engine.execute("weather clear", adminCtx, session);
    assert(weatherRes.success);
    assert(session.getWeather().state == WeatherState::Clear);

    // 4. /tp con coordenadas relativas (~)
    std::cout << "[4] Verificando /tp con coordenadas relativas (~)..." << std::endl;
    player.position = {10.0f, 64.0f, 10.0f};
    adminCtx.sourcePosition = player.position;
    auto tpRes = engine.execute("tp ~5 ~10 ~-2", adminCtx, session);
    assert(tpRes.success);
    assert(std::abs(player.position.x - 15.0f) < 0.01f);
    assert(std::abs(player.position.y - 74.0f) < 0.01f);
    assert(std::abs(player.position.z - 8.0f) < 0.01f);

    // 5. /give
    std::cout << "[5] Verificando /give..." << std::endl;
    auto giveRes = engine.execute("give @p opencraft:diamond 5", adminCtx, session);
    assert(giveRes.success);
    ItemId diaId = ItemRegistry::global().getId("opencraft:diamond");
    assert(player.inventory.countItem(diaId) == 5);

    // 6. /summon
    std::cout << "[6] Verificando /summon..." << std::endl;
    size_t prevMobs = session.getMobs().size();
    auto summonRes = engine.execute("summon zombie 15 74 8", adminCtx, session);
    assert(summonRes.success);
    assert(session.getMobs().size() == prevMobs + 1);
    assert(session.getMobs().back().mobType == MobType::Zombie);

    // 7. /setblock
    std::cout << "[7] Verificando /setblock..." << std::endl;
    auto setblockRes = engine.execute("setblock 15 70 8 opencraft:obsidian", adminCtx, session);
    assert(setblockRes.success);
    assert(session.getWorld().getBlock(15, 70, 8) == BlockID::Obsidian);

    // 8. /fill con safety cap (<= 32768 bloques)
    std::cout << "[8] Verificando /fill y límite estricto de seguridad de volumen..." << std::endl;
    // Bounding box seguro: 10x10x10 = 1000 bloques <= 32768
    auto fillSafeRes = engine.execute("fill 0 60 0 9 69 9 opencraft:stone", adminCtx, session);
    assert(fillSafeRes.success);
    assert(fillSafeRes.affectedCount == 1000);
    assert(session.getWorld().getBlock(0, 60, 0) == BlockID::Stone);
    assert(session.getWorld().getBlock(9, 69, 9) == BlockID::Stone);

    // Bounding box inseguro: 40x40x30 = 48,000 bloques > 32768
    auto fillUnsafeRes = engine.execute("fill 0 0 0 39 39 29 opencraft:stone", adminCtx, session);
    assert(!fillUnsafeRes.success && "Fill con volumen > 32768 debe ser rechazado por seguridad");
    assert(fillUnsafeRes.message.find("Límite") != std::string::npos ||
           fillUnsafeRes.message.find("32768") != std::string::npos);

    // 9. /gamerule
    std::cout << "[9] Verificando /gamerule..." << std::endl;
    assert(engine.getGameRules().keepInventory == false);
    auto grRes = engine.execute("gamerule keepInventory true", adminCtx, session);
    assert(grRes.success);
    assert(engine.getGameRules().keepInventory == true);

    grRes = engine.execute("gamerule doDaylightCycle false", adminCtx, session);
    assert(grRes.success);
    assert(engine.getGameRules().doDaylightCycle == false);

    std::cout << "--- [F13 Test 2] COMPLETADO EXITOSAMENTE ---" << std::endl;
    return 0;
}

#include "sim/WorldSession.hpp"
#include "world/CommandBlockEntity.hpp"
#include "sim/FunctionRunner.hpp"
#include "world/BlockRegistry.hpp"
#include "item/Item.hpp"
#include <iostream>
#include <cassert>

using namespace OpenCraft;

int main() {
    std::cout << "--- [F13 Test 3] Bloques de Comandos, Funciones, Límite de Recursión (16) y Presupuesto por Tick "
                 "(10000) ---"
              << std::endl;

    WorldSession session;
    auto& world = session.getWorld();
    auto& runner = session.getFunctionRunner();

    // 1. Bloque de comandos Impulse con cinabrio
    std::cout << "[1] Verificando bloque de comandos Impulse con señal de cinabrio..." << std::endl;
    world.setBlock(10, 64, 10, BlockID::CommandBlock);
    auto cb = std::make_shared<CommandBlockEntity>(BlockPos{10, 64, 10});
    cb->setCommand("time set 12000");
    cb->setMode(CommandBlockMode::Impulse);
    cb->setAlwaysActive(false); // Requiere cinabrio
    world.setBlockEntity(10, 64, 10, cb);

    // Sin cinabrio: tick() no debe ejecutarlo
    session.tick();
    assert(session.getClock().getTimeOfDay() != 12000);

    // Activar cinabrio contiguo
    session.getCinnabarEngine().setSignal({10, 64, 11}, 15);
    world.setBlock(10, 64, 11, BlockID::CinnabarBlock);
    session.tick();

    assert(session.getClock().getTimeOfDay() == 12001);
    assert(cb->getSuccessCount() == 1);

    // 2. Bloque de comandos Repeating AlwaysActive
    std::cout << "[2] Verificando bloque de comandos Repeating AlwaysActive..." << std::endl;
    world.setBlock(12, 64, 12, BlockID::RepeatingCommandBlock);
    auto repCb = std::make_shared<CommandBlockEntity>(BlockPos{12, 64, 12});
    repCb->setCommand("time set day");
    repCb->setMode(CommandBlockMode::Repeating);
    repCb->setAlwaysActive(true);
    world.setBlockEntity(12, 64, 12, repCb);

    session.getClock().setTickCount(5000);
    session.tick();
    assert(session.getClock().getTimeOfDay() == 1001);

    // 3. Ejecución secuencial de .mcfunction
    std::cout << "[3] Verificando ejecución secuencial de funciones (.mcfunction)..." << std::endl;
    runner.registerFunction("test:setup", {"time set night", "weather clear", "setblock 0 64 0 opencraft:stone"});

    CommandContext ctx{.sourceEntityId = session.getPlayer().id,
                       .sourcePosition = {0.0f, 64.0f, 0.0f},
                       .sourceDimension = 0,
                       .permissionLevel = 4,
                       .isSilent = true};

    auto fnRes = runner.runFunction("test:setup", ctx, session);
    assert(fnRes.success);
    assert(fnRes.affectedCount == 3);
    assert(session.getClock().getTimeOfDay() == 13000);
    assert(session.getWeather().state == WeatherState::Clear);
    assert(world.getBlock(0, 64, 0) == BlockID::Stone);

    // 4. Límite estricto de profundidad de recursión (<= 16)
    std::cout << "[4] Verificando límite de recursión (profundidad máxima = 16)..." << std::endl;
    runner.registerFunction("test:infinite_recursion", {"function test:infinite_recursion"});

    auto recurRes = runner.runFunction("test:infinite_recursion", ctx, session);
    assert(!recurRes.success && "Recursión infinita debe ser abortada");
    assert(recurRes.message.find("Recursión") != std::string::npos || recurRes.message.find("16") != std::string::npos);

    // 5. Presupuesto de instrucciones por tick (<= 10,000 comandos/tick)
    std::cout << "[5] Verificando presupuesto de comandos por tick (límite = 10,000)..." << std::endl;
    // Creamos una función con bucle desenrollado o recursión acotada que intente superar 10,000
    std::vector<std::string> largeBatch;
    for (int i = 0; i < 5000; ++i) {
        largeBatch.push_back("time set day");
    }
    runner.registerFunction("test:batch_5k", largeBatch);

    runner.onTickStart();
    auto run1 = runner.runFunction("test:batch_5k", ctx, session);
    assert(run1.success);
    assert(runner.getCommandsExecutedThisTick() == 5000);

    auto run2 = runner.runFunction("test:batch_5k", ctx, session);
    assert(run2.success);
    assert(runner.getCommandsExecutedThisTick() == 10000);

    // El siguiente comando dentro del mismo tick excede el presupuesto de 10,000
    std::vector<std::string> oneMore = {"time set night"};
    runner.registerFunction("test:one_more", oneMore);
    auto run3 = runner.runFunction("test:one_more", ctx, session);
    assert(!run3.success && "Ejecución que exceda 10,000 comandos/tick debe ser abortada");
    assert(run3.message.find("Presupuesto") != std::string::npos || run3.message.find("10000") != std::string::npos);

    // Al comenzar nuevo tick, el contador debe reiniciarse
    runner.onTickStart();
    assert(runner.getCommandsExecutedThisTick() == 0);
    auto run4 = runner.runFunction("test:one_more", ctx, session);
    assert(run4.success);
    assert(runner.getCommandsExecutedThisTick() == 1);

    std::cout << "--- [F13 Test 3] COMPLETADO EXITOSAMENTE ---" << std::endl;
    return 0;
}

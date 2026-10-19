#include <cassert>
#include <iostream>
#include <filesystem>
#include "platform/LifecycleManager.hpp"
#include "sim/WorldSession.hpp"
#include "world/Block.hpp"
#include "storage/WorldStorage.hpp"
#include "audio/SpatialAudio.hpp"
#include "input/TouchController.hpp"

using namespace OpenCraft;

void testTwentySuspendResumeCycles() {
    std::filesystem::path testDir = std::filesystem::current_path() / "test_scratch_lifecycle";
    std::filesystem::create_directories(testDir);

    WorldStorage storage(testDir / "lifecycle_world");
    WorldSession session;
    session.init(3, 4242);

    // Colocar al jugador en una posición y darle objetos
    auto& player = session.getPlayer();
    player.position = Math::Vec3{100.5f, 65.0f, -200.5f};
    player.prevPosition = player.position;
    ItemStack ironStack(static_cast<uint16_t>(BlockID::IronOre), 15);
    player.inventory.addItem(ironStack);
    ItemStack diamondStack(static_cast<uint16_t>(BlockID::DiamondOre), 3);
    player.inventory.addItem(diamondStack);

    SpatialAudioEngine audio;
    TouchController touch;
    touch.updateScreenDimensions(1280, 720);

    LifecycleManager lifecycle;

    // Ejecutar exactamente 20 ciclos continuos de suspensión a segundo plano y recuperación
    for (int cycle = 1; cycle <= 20; ++cycle) {
        // En cada ciclo simular toques en pantalla
        touch.handleFingerDown(cycle, 120.0f, 600.0f);
        touch.handleFingerMotion(cycle, 120.0f, 570.0f, 0.0f, -30.0f);
        assert(touch.getActiveFingerCount() > 0);

        bool cycleSuccess = lifecycle.runSuspendResumeCycle(session, storage, audio, touch);
        assert(cycleSuccess);

        // Verificar que tras reanudar, no hay dedos pegados
        assert(touch.getActiveFingerCount() == 0);
        assert(touch.getMovementVector().length() == 0.0f);
        assert(!audio.isMuted());

        // Verificar que los datos del jugador se mantienen íntegros
        const auto& reloadedPlayer = session.getPlayer();
        assert(std::abs(reloadedPlayer.position.x - 100.5f) < 0.01f);
        assert(std::abs(reloadedPlayer.position.z - (-200.5f)) < 0.01f);
        assert(reloadedPlayer.inventory.hasItem(static_cast<uint16_t>(BlockID::IronOre), 15));
        assert(reloadedPlayer.inventory.hasItem(static_cast<uint16_t>(BlockID::DiamondOre), 3));
    }

    assert(lifecycle.getStats().suspendCount == 20);
    assert(lifecycle.getStats().resumeCount == 20);

    std::filesystem::remove_all(testDir);
    std::cout << "[PASS] 20 ciclos de suspensión/reanudación superados con 100% integridad y cero controles pegados."
              << std::endl;
}

void testLowMemoryAndContextLoss() {
    LifecycleManager lifecycle;
    assert(lifecycle.getState() == AppLifecycleState::Active);

    // Simular evento de baja memoria del sistema operativo móvil
    lifecycle.onLowMemory();
    assert(lifecycle.getStats().lowMemoryCount == 1);

    // Simular pérdida y restauración de contexto OpenGL ES
    lifecycle.onContextLost();
    assert(lifecycle.getStats().contextLossCount == 1);

    lifecycle.onContextRestored();
    assert(lifecycle.getState() == AppLifecycleState::Active);

    std::cout << "[PASS] Gestión de baja memoria y pérdida de contexto gráfico móvil verificada." << std::endl;
}

int main() {
    std::cout << "=== Ejecutando Suite de Pruebas: LifecycleAndBackgroundRecoveryTest ===" << std::endl;
    testTwentySuspendResumeCycles();
    testLowMemoryAndContextLoss();
    std::cout << "=== Todas las pruebas de Ciclo de Vida y Recuperación Móvil SUPERADAS ===" << std::endl;
    return 0;
}

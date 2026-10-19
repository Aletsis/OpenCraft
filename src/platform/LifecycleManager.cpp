#include "platform/LifecycleManager.hpp"
#include "sim/WorldSession.hpp"
#include "storage/WorldStorage.hpp"
#include "audio/SpatialAudio.hpp"
#include "input/TouchController.hpp"

namespace OpenCraft {

LifecycleManager::LifecycleManager() = default;

void LifecycleManager::onWillEnterBackground() {
    m_state = AppLifecycleState::WillSuspend;
    if (m_saveCallback) {
        m_stats.autoSavedOnLastSuspend = m_saveCallback();
    }
    if (m_audioMuteCallback) {
        m_audioMuteCallback(true);
    }
    if (m_touchResetCallback) {
        m_touchResetCallback();
    }
}

void LifecycleManager::onDidEnterBackground() {
    m_state = AppLifecycleState::Suspended;
    m_stats.suspendCount++;
}

void LifecycleManager::onWillEnterForeground() {
    m_state = AppLifecycleState::WillResume;
    if (m_touchResetCallback) {
        m_touchResetCallback();
    }
}

void LifecycleManager::onDidEnterForeground() {
    m_state = AppLifecycleState::Active;
    m_stats.resumeCount++;
    if (m_audioMuteCallback) {
        m_audioMuteCallback(false);
    }
}

void LifecycleManager::onLowMemory() { m_stats.lowMemoryCount++; }

void LifecycleManager::onContextLost() { m_stats.contextLossCount++; }

void LifecycleManager::onContextRestored() {
    if (m_touchResetCallback) {
        m_touchResetCallback();
    }
}

bool LifecycleManager::runSuspendResumeCycle(WorldSession& session, WorldStorage& storage, SpatialAudioEngine& audio,
                                             TouchController& touch) {
    // 1. Simular inicio de suspensión
    m_state = AppLifecycleState::WillSuspend;
    touch.cancelAllFingers();
    audio.setMuted(true);

    // 2. Autoguardado coherente al pasar a segundo plano
    StorageError saveErr = session.saveToStorage(storage);
    if (saveErr != StorageError::None) {
        return false;
    }
    m_stats.autoSavedOnLastSuspend = true;

    // 3. Confirmar suspensión
    m_state = AppLifecycleState::Suspended;
    m_stats.suspendCount++;

    // 4. Simular recuperación y vuelta a primer plano
    m_state = AppLifecycleState::WillResume;
    touch.cancelAllFingers();

    // 5. Cargar y verificar integridad desde disco
    StorageError loadErr = session.loadFromStorage(storage);
    if (loadErr != StorageError::None) {
        return false;
    }

    // 6. Restaurar contexto y audio
    audio.setMuted(false);
    m_state = AppLifecycleState::Active;
    m_stats.resumeCount++;

    // Verificar que los controles táctiles no quedaron pegados
    if (touch.getActiveFingerCount() > 0 || touch.isJumpPressed() || touch.isAttackPressed()) {
        return false;
    }

    return true;
}

} // namespace OpenCraft

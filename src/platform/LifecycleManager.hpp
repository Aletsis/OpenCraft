#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace OpenCraft {

class WorldSession;
class WorldStorage;
class SpatialAudioEngine;
class TouchController;

enum class AppLifecycleState {
    Active = 0,
    WillSuspend,
    Suspended,
    WillResume
};

struct LifecycleStats {
    uint32_t suspendCount = 0;
    uint32_t resumeCount = 0;
    uint32_t lowMemoryCount = 0;
    uint32_t contextLossCount = 0;
    bool autoSavedOnLastSuspend = false;
};

class LifecycleManager {
public:
    LifecycleManager();

    AppLifecycleState getState() const { return m_state; }
    const LifecycleStats& getStats() const { return m_stats; }

    // Procesamiento de eventos nativos SDL3
    void onWillEnterBackground();
    void onDidEnterBackground();
    void onWillEnterForeground();
    void onDidEnterForeground();
    void onLowMemory();
    void onContextLost();
    void onContextRestored();

    // Callbacks configurables
    void setSaveCallback(std::function<bool()> callback) { m_saveCallback = callback; }
    void setAudioMuteCallback(std::function<void(bool)> callback) { m_audioMuteCallback = callback; }
    void setTouchResetCallback(std::function<void()> callback) { m_touchResetCallback = callback; }

    // Simulador de ciclo para tests automatizados de estrés (ej. 20 ciclos continuos)
    bool runSuspendResumeCycle(WorldSession& session, WorldStorage& storage, SpatialAudioEngine& audio,
                               TouchController& touch);

    void resetStats() { m_stats = LifecycleStats{}; }

private:
    AppLifecycleState m_state = AppLifecycleState::Active;
    LifecycleStats m_stats;

    std::function<bool()> m_saveCallback;
    std::function<void(bool)> m_audioMuteCallback;
    std::function<void()> m_touchResetCallback;
};

} // namespace OpenCraft

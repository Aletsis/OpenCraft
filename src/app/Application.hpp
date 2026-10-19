#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>
#include <SDL3/SDL.h>
#include "core/Window.hpp"
#include "sim/WorldSession.hpp"
#include "render/WorldRenderer.hpp"
#include "render/EntityRenderer.hpp"
#include "render/TextureAtlas.hpp"
#include "render/Shader.hpp"
#include "render/Camera.hpp"
#include "ui/UIBatch.hpp"
#include "ui/HUD.hpp"
#include "ui/MenuSystem.hpp"
#include "network/NetworkManager.hpp"
#include "storage/WorldManager.hpp"
#include "world/ChunkWorkerPool.hpp"
#include "world/ChunkManager.hpp"
#include "render/Frustum.hpp"
#include "physics/Raycast.hpp"

#include "platform/DeviceSettings.hpp"
#include "platform/PlatformPaths.hpp"
#include "platform/LifecycleManager.hpp"
#include "audio/SpatialAudio.hpp"
#include "audio/AudioSubtitles.hpp"
#include "audio/AudioOutput.hpp"
#include "input/TouchController.hpp"
#include "input/GamepadController.hpp"
#include "render/ParticleSystem.hpp"
#include "game/TutorialSystem.hpp"
#include "ui/LocalizationManager.hpp"
#include "benchmark/BenchmarkHarness.hpp"

namespace OpenCraft {

struct AppConfig {
    std::string title = "OpenCraft";
    int width = 1280;
    int height = 720;
    bool vsync = true;
    bool fullscreen = false;
    std::string glBackend = "auto"; // auto | native | angle (--gl / OPENCRAFT_GL)
    std::string worldId;            // abrir directamente este mundo
    std::string createWorldName;    // crear y abrir un mundo nuevo
    std::string createWorldSeed;
    uint32_t createWorldMode = 0;
    bool listWorldsOnly = false;
    float autoSaveInterval = 30.0f;
    std::string scriptPath; // automatización de QA (--script)
};

class Application {
public:
    explicit Application(const AppConfig& config = {});
    ~Application();

    bool init();
    int run();

private:
    // Partida en curso: se crea al entrar a un mundo y se destruye al salir al título
    struct EventCollector : IGameEventListener {
        std::vector<GameEvent> events;
        void onEvent(const GameEvent& event) override { events.push_back(event); }
    };

    struct Game {
        std::string worldId;
        EventCollector events;
        std::unique_ptr<WorldStorage> storage;
        WorldSession session;
        ChunkWorkerPool pool;
        ChunkManager chunks;
        WorldRenderer renderer;
        NetworkManager net;
        bool ready = false;
        bool freshWorld = false;
        float autoSaveTimer = 0.0f;
        float loadTimer = 0.0f;
        uint64_t lastDigTick = 0;
        bool digging = false;
        BlockPos digPos{0, 0, 0};
        float useCooldown = 0.0f;
        float breakCooldown = 0.0f;
        bool deathHandled = false;
        bool lanOpen = false;
        bool placedOnSurface = false;

        explicit Game(size_t threads);
    };

    AppConfig m_config;
    Window m_window;
    TextureAtlas m_atlas;
    Shader m_voxelShader;
    UIBatch m_ui;
    HUD m_hud;
    HudState m_hudState;
    MenuSystem m_menu;
    EntityRenderer m_entityRenderer;
    Camera m_camera;
    DeviceSettings m_settings;
    std::unique_ptr<WorldManager> m_worldManager;
    std::vector<WorldSummary> m_worlds;
    std::unique_ptr<Game> m_game;
    Net::LANDiscovery m_lanBrowser;

    TouchController m_touch;
    GamepadController m_gamepad;
    SpatialAudioEngine m_audio;
    AudioOutput m_audioOut;
    float m_stepDistance = 0.0f;
    AudioSubtitleManager m_subtitles;
    ParticleSystem m_particles;
    TutorialSystem m_tutorial;
    LifecycleManager m_lifecycle;
    BenchmarkHarness m_benchmark;

    // Estado de entrada
    bool m_inventoryOpen = false;
    bool m_mouseCaptured = false;
    bool m_touchSeen = false;
    bool m_hideHud = false;
    float m_mouseX = -1.0f;
    float m_mouseY = -1.0f;
    bool m_leftDown = false;
    bool m_rightDown = false;
    std::unordered_set<SDL_Keycode> m_keysDown;
    MenuInput m_menuInput;
    uint64_t m_lastClickMs = 0;
    uint64_t m_lastJumpMs = 0;
    bool m_jumpQueued = false;
    float m_swing = 0.0f;
    float m_bob = 0.0f;
    size_t m_lastSelectedSlot = 9999;
    uint16_t m_lastHeldItem = 0xFFFF;
    double m_time = 0.0;
    float m_loadingProgress = 0.0f;
    int m_fps = 0;
    bool m_screenshotRequested = false;
    std::string m_screenshotPath;
    SDL_Gamepad* m_sdlGamepad = nullptr;

    // Automatización de QA
    struct ScriptStep {
        std::string op;
        std::vector<std::string> args;
    };
    std::deque<ScriptStep> m_script;
    double m_scriptWaitUntil = 0.0;
    std::deque<std::string> m_scriptTextStorage;
    std::vector<std::pair<double, SDL_Keycode>> m_scriptReleases;

    // Flujo de pantallas y partida
    void refreshWorldList();
    bool startWorld(const std::string& worldId);
    void createAndStartWorld(const std::string& name, const std::string& seedText, uint32_t mode, bool hardcore);
    void leaveWorld();
    void saveGame();
    void updateLoading(float dt);
    void enterGameplay();
    void handleMenuEvents(const std::vector<MenuEvent>& events);
    void applySettings();
    void setMouseCaptured(bool captured);
    void openInventory(bool open);
    void pauseGame();
    bool attackEntityInSight();

    // Entrada y simulación
    void handleEvent(const SDL_Event& e);
    void handleGameKey(SDL_Keycode key, bool repeat);
    void handleInventoryClick(bool right, bool shift);
    void updateGameplay(float dt);
    void continuousActions(float dt, const std::optional<RaycastHit>& hit);
    void useItem(const RaycastHit& hit);
    std::optional<RaycastHit> pickBlock() const;

    // Render
    void render();
    void renderWorld(float alpha, const WorldSnapshot& snap, const std::optional<RaycastHit>& hit);
    Math::Vec3 skyColor() const;
    DebugInfo buildDebugInfo(const WorldSnapshot& snap, const std::optional<RaycastHit>& hit) const;
    void takeScreenshot(const std::string& path);
    // Efecto de sonido: posicional si se indica posición, si no centrado (interfaz)
    void playSfx(const std::string& name, AudioCategory category, const Math::Vec3* position, float pitch = 1.0f);

    // Script
    bool loadScript(const std::string& path);
    void runScript();
    void pushKey(SDL_Keycode key, bool down);
};

} // namespace OpenCraft

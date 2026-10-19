#include "app/Application.hpp"
#include "core/Log.hpp"
#include "core/PngWriter.hpp"
#include "ui/ItemNames.hpp"
#include "sim/MobEntity.hpp"
#include <GLES3/gl3.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <format>
#include <fstream>
#include <random>
#include <sstream>
#include <thread>

namespace OpenCraft {

namespace {

constexpr int SPAWN_RADIUS = 2; // chunks que deben existir antes de soltar al jugador
constexpr float MAX_LOAD_SECONDS = 20.0f;
constexpr uint16_t LAN_PORT = 35565;

uint64_t nowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

int floorDiv16(float v) { return static_cast<int>(std::floor(v / 16.0f)); }

// Semilla a partir del texto del jugador: vacío = aleatoria, número = literal y texto = hash al
// estilo del juego original (String.hashCode de Java) para que las semillas compartidas coincidan
uint64_t parseSeed(const std::string& text) {
    std::string t = text;
    t.erase(0, t.find_first_not_of(' '));
    while (!t.empty() && t.back() == ' ') t.pop_back();
    if (t.empty()) {
        std::random_device rd;
        return (static_cast<uint64_t>(rd()) << 32) ^ rd() ^ static_cast<uint64_t>(std::time(nullptr));
    }
    bool numeric = std::all_of(t.begin() + (t[0] == '-' ? 1 : 0), t.end(), [](char c) { return c >= '0' && c <= '9'; });
    if (numeric && t != "-") {
        try {
            return static_cast<uint64_t>(std::stoll(t));
        } catch (...) {}
    }
    int32_t h = 0;
    for (unsigned char c : t) h = static_cast<int32_t>(31u * static_cast<uint32_t>(h) + c);
    return static_cast<uint64_t>(static_cast<int64_t>(h));
}

bool isInteractiveBlock(const BlockProperties& p) {
    static const char* IDS[] = {"chest",           "crafting_table", "furnace",  "bed",           "lever",
                                "button",          "door",           "repeater", "comparator",    "hopper",
                                "dropper",         "dispenser",      "crafter",  "brewing_stand", "beacon",
                                "command_block",   "vault",          "lectern",  "composter",     "daylight_detector",
                                "rim_portal_frame"};
    for (const char* id : IDS) {
        if (p.namespacedId.find(id) != std::string::npos) return true;
    }
    return false;
}

bool opensContainer(const BlockProperties& p) {
    return p.legacyId == BlockID::Chest || p.legacyId == BlockID::CraftingTable || p.legacyId == BlockID::Furnace;
}

SDL_Keycode keyFromName(const std::string& n) {
    if (n.size() == 1) {
        char c = n[0];
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) return static_cast<SDL_Keycode>(c);
    }
    if (n == "space") return SDLK_SPACE;
    if (n == "shift") return SDLK_LSHIFT;
    if (n == "ctrl") return SDLK_LCTRL;
    if (n == "escape" || n == "esc") return SDLK_ESCAPE;
    if (n == "enter") return SDLK_RETURN;
    if (n == "tab") return SDLK_TAB;
    if (n == "backspace") return SDLK_BACKSPACE;
    if (n == "up") return SDLK_UP;
    if (n == "down") return SDLK_DOWN;
    if (n == "f1") return SDLK_F1;
    if (n == "f2") return SDLK_F2;
    if (n == "f3") return SDLK_F3;
    if (n == "f11") return SDLK_F11;
    return SDLK_UNKNOWN;
}

} // namespace

Application::Game::Game(size_t threads) : pool(threads), chunks(session.getWorld(), pool) {
    session.addEventListener(&events);
}

Application::Application(const AppConfig& config)
    : m_config(config),
      m_window(WindowConfig{.title = config.title,
                            .width = config.width,
                            .height = config.height,
                            .vsync = config.vsync,
                            .fullscreen = config.fullscreen}) {}

Application::~Application() {
    if (m_game) {
        leaveWorld();
    }
    if (m_sdlGamepad) {
        SDL_CloseGamepad(m_sdlGamepad);
    }
}

// =================================================================================================
// Inicialización
// =================================================================================================

// Las versiones anteriores guardaban los mundos en ./saves (relativo al directorio de ejecución).
// Se copian (sin mover ni borrar los originales) a la carpeta de datos del usuario si aún no están.
static void importLegacyWorlds(const std::filesystem::path& savesDir) {
    std::error_code ec;
    std::vector<std::filesystem::path> candidates = {std::filesystem::current_path(ec) / "saves"};
    if (auto exe = PlatformPaths::getExecutableDir(); !exe.empty()) {
        candidates.push_back(exe / "saves");
        candidates.push_back(exe.parent_path() / "saves");
    }
    for (const auto& legacy : candidates) {
        if (legacy.empty() || !std::filesystem::is_directory(legacy, ec)) continue;
        if (std::filesystem::equivalent(legacy, savesDir, ec)) continue;
        for (const auto& entry : std::filesystem::directory_iterator(legacy, ec)) {
            if (!entry.is_directory() || !std::filesystem::exists(entry.path() / "world.meta", ec)) continue;
            auto target = savesDir / entry.path().filename();
            if (std::filesystem::exists(target, ec)) continue;
            std::filesystem::create_directories(savesDir, ec);
            std::filesystem::copy(entry.path(), target, std::filesystem::copy_options::recursive, ec);
            std::filesystem::remove(target / "session.lock", ec);
            if (!ec)
                Log::info("Mundo importado de la versión anterior: {} -> {}", entry.path().string(), target.string());
        }
    }
}

bool Application::init() {
    importLegacyWorlds(PlatformPaths::getSavesDir());
    m_worldManager = std::make_unique<WorldManager>(PlatformPaths::getSavesDir(), PlatformPaths::getBackupsDir());
    m_settings.loadDefault();

    if (m_config.listWorldsOnly) {
        auto worlds = m_worldManager->listWorlds();
        Log::info("=== Mundos en {} ===", m_worldManager->getSavesDir().string());
        if (worlds.empty()) Log::info("  (No hay mundos guardados)");
        for (const auto& w : worlds) {
            Log::info("  - ID: '{}' | Nombre: '{}' | Semilla: {} | Esquema: v{} | Tamaño: {} bytes", w.id, w.name,
                      w.seed, w.schemaVersion, w.sizeOnDisk);
        }
        return false;
    }

    if (!m_window.init()) {
        Log::error("Fallo crítico al inicializar la ventana y el contexto gráfico");
        return false;
    }
    m_window.setVsync(m_settings.vsync);
    if (m_audioOut.init()) {
        m_audioOut.setMasterVolume(static_cast<float>(m_settings.masterVolume) / 100.0f);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    m_atlas.generateProceduralAtlas();
    if (!m_voxelShader.loadFromFiles("assets/shaders/voxel.vert", "assets/shaders/voxel.frag") || !m_ui.init() ||
        !m_entityRenderer.init()) {
        Log::error("No se encontraron los recursos del juego (shaders). Ejecute desde el paquete instalado "
                   "o defina OPENCRAFT_ASSETS_DIR con la ruta de la carpeta assets/");
        return false;
    }
    m_camera.fov = static_cast<float>(m_settings.fov);

    m_touch.updateScreenDimensions(m_window.getWidth(), m_window.getHeight());
    m_particles.setMaxParticles(static_cast<size_t>(m_settings.maxParticles));
    m_lifecycle.setSaveCallback([this]() {
        saveGame();
        return true;
    });
    m_lifecycle.setAudioMuteCallback([this](bool muted) {
        m_audio.setMuted(muted);
        m_audioOut.setPaused(muted);
    });
    m_lifecycle.setTouchResetCallback([this]() { m_touch.cancelAllFingers(); });

#if defined(__ANDROID__) || defined(__APPLE__)
    m_touchSeen = true;
#endif
    if (PlatformPaths::getPlatformType() == PlatformType::LinuxMobile) m_touchSeen = true;

    if (!m_config.scriptPath.empty() && !loadScript(m_config.scriptPath)) {
        return false;
    }

    refreshWorldList();
    m_menu.open(MenuScreen::Title);

    if (!m_config.createWorldName.empty()) {
        createAndStartWorld(m_config.createWorldName, m_config.createWorldSeed, m_config.createWorldMode, false);
    } else if (!m_config.worldId.empty()) {
        startWorld(m_config.worldId);
    }
    return true;
}

void Application::refreshWorldList() { m_worlds = m_worldManager->listWorlds(); }

// =================================================================================================
// Ciclo de vida de una partida
// =================================================================================================

bool Application::startWorld(const std::string& worldId) {
    if (m_game) leaveWorld();

    // Un mundo de un esquema anterior se lee con compatibilidad hacia atrás y se reescribe con el
    // esquema actual al guardar: antes se conserva una copia completa por si hubiera que volver atrás
    for (const auto& w : m_worlds) {
        if (w.id == worldId && w.schemaVersion < CURRENT_SCHEMA_VERSION) {
            auto backup = m_worldManager->backupWorld(worldId);
            if (backup.empty()) {
                m_menu.showMessage("No se puede abrir el mundo",
                                   "El mundo es de una versión anterior y no se pudo crear la copia de seguridad "
                                   "previa a su actualización.",
                                   MenuScreen::SelectWorld);
                return false;
            }
            Log::info("Mundo '{}' (esquema v{}) respaldado en '{}' antes de actualizarlo a v{}", worldId,
                      w.schemaVersion, backup.string(), CURRENT_SCHEMA_VERSION);
        }
    }

    StorageError err = StorageError::None;
    auto storage = m_worldManager->openWorld(worldId, err);
    if (!storage || err != StorageError::None) {
        std::string why = (err == StorageError::Locked)
                              ? "El mundo está abierto en otra ventana de OpenCraft."
                              : std::format("No se pudo abrir el mundo ({}).", storageErrorToString(err));
        m_menu.showMessage("No se puede abrir el mundo", why, MenuScreen::SelectWorld);
        return false;
    }

    unsigned hw = std::thread::hardware_concurrency();
    size_t threads = std::clamp<size_t>(hw > 2 ? hw - 1 : 1, 1, 4);
    m_game = std::make_unique<Game>(threads);
    Game& g = *m_game;
    g.worldId = worldId;
    g.storage = std::move(storage);
    g.session.init(m_settings.renderDistance);

    if (g.storage->exists()) {
        StorageError loadErr = g.session.loadFromStorage(*g.storage);
        if (loadErr != StorageError::None) {
            m_worldManager->closeWorld(worldId);
            m_game.reset();
            m_menu.showMessage("No se puede cargar el mundo",
                               std::format("Los datos del mundo están dañados o son de una versión no compatible ({}). "
                                           "Puede restaurar una copia de la carpeta de copias de seguridad.",
                                           storageErrorToString(loadErr)),
                               MenuScreen::SelectWorld);
            return false;
        }
    }
    g.session.getWorld().setRenderDistance(m_settings.renderDistance);
    g.freshWorld = g.session.getClock().getTickCount() == 0;
    g.chunks.setStorage(g.storage.get());
    g.chunks.setDimension(static_cast<DimensionType>(g.session.getCurrentDimension()));
    g.session.attachStorage(g.storage.get());

    const auto& p = g.session.getPlayer();
    m_camera.yaw = p.yaw;
    m_camera.pitch = p.pitch;
    m_camera.processMouseMovement(0.0f, 0.0f);
    m_inventoryOpen = false;
    m_hudState.creativePage = 0;
    m_lastSelectedSlot = 9999;
    m_tutorial = TutorialSystem();
    m_menu.open(MenuScreen::Loading);
    Log::info("Abriendo mundo '{}'", worldId);
    return true;
}

void Application::createAndStartWorld(const std::string& name, const std::string& seedText, uint32_t mode,
                                      bool hardcore) {
    uint64_t seed = parseSeed(seedText);
    std::string id = m_worldManager->createWorld(name, seed, mode, CURRENT_SCHEMA_VERSION, hardcore);
    if (id.empty()) {
        m_menu.showMessage("No se pudo crear el mundo",
                           "Compruebe el espacio libre y los permisos de la carpeta de datos.",
                           MenuScreen::SelectWorld);
        return;
    }
    refreshWorldList();
    if (startWorld(id) && m_game) {
        // Supervivencia empieza con las manos vacías; en creativo se usa la paleta del inventario
        auto& player = m_game->session.getPlayer();
        for (size_t i = 0; i < player.inventory.getSize(); ++i) player.inventory.getSlot(i).clear();
        player.syncHotbar();
        // El modo elegido se aplica al jugador del mundo nuevo
        m_game->session.queueCommand(SetGameModeCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .gameMode = mode});
        m_game->session.setHardcore(hardcore);
        m_game->freshWorld = true;
    }
}

void Application::saveGame() {
    if (!m_game || !m_game->storage) return;
    Game& g = *m_game;
    StorageError chunkErr = StorageError::None;
    g.chunks.saveAllDirtyChunks(&chunkErr);
    StorageError err = g.session.saveToStorage(*g.storage);
    if (err != StorageError::None || chunkErr != StorageError::None) {
        Log::error("Error al guardar el mundo '{}': {}", g.worldId,
                   storageErrorToString(err != StorageError::None ? err : chunkErr));
        m_hudState.actionMessage = "¡No se pudo guardar el mundo!";
        m_hudState.actionMessageAlpha = 4.0f;
    } else {
        Log::info("Mundo '{}' guardado", g.worldId);
        m_tutorial.onGameSaved();
    }
}

void Application::leaveWorld() {
    if (!m_game) return;
    if (m_game->ready) saveGame();
    m_game->net.disconnect();
    m_worldManager->closeWorld(m_game->worldId);
    m_game.reset();
    m_inventoryOpen = false;
    setMouseCaptured(false);
    refreshWorldList();
}

void Application::updateLoading(float dt) {
    Game& g = *m_game;
    g.loadTimer += dt;
    const auto& p = g.session.getPlayer();
    g.chunks.update(p.position.x, p.position.z);

    int pcx = floorDiv16(p.position.x), pcz = floorDiv16(p.position.z);
    int needed = 0, have = 0;
    for (int dx = -SPAWN_RADIUS; dx <= SPAWN_RADIUS; ++dx) {
        for (int dz = -SPAWN_RADIUS; dz <= SPAWN_RADIUS; ++dz) {
            ++needed;
            if (g.chunks.isChunkReady(pcx + dx, pcz + dz) || g.chunks.isChunkFailed(pcx + dx, pcz + dz)) ++have;
        }
    }
    int meshed = 0;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz)
            if (g.renderer.isChunkMeshed(pcx + dx, pcz + dz)) ++meshed;

    m_loadingProgress =
        0.8f * static_cast<float>(have) / static_cast<float>(needed) + 0.2f * static_cast<float>(meshed) / 9.0f;
    if (have == needed && g.session.ensurePlayerAboveGround(g.freshWorld)) {
        const auto& np = g.session.getPlayer().position;
        Log::info("Jugador colocado sobre la superficie en ({:.1f}, {:.1f}, {:.1f})", np.x, np.y, np.z);
        g.freshWorld = false;
        g.placedOnSurface = true;
    }
    if ((have == needed && meshed == 9) || g.loadTimer > MAX_LOAD_SECONDS) {
        enterGameplay();
    }
}

void Application::enterGameplay() {
    Game& g = *m_game;
    g.ready = true;
    g.deathHandled = false;
    m_menu.open(MenuScreen::None);
    setMouseCaptured(true);
    if (g.placedOnSurface) {
        // Guardado inicial: deja persistida la posición segura de aparición
        saveGame();
        g.placedOnSurface = false;
    }
}

// =================================================================================================
// Bucle principal
// =================================================================================================

int Application::run() {
    uint64_t last = SDL_GetPerformanceCounter();
    const double freq = static_cast<double>(SDL_GetPerformanceFrequency());
    int frames = 0;
    double fpsTimer = 0.0;

    while (!m_window.shouldClose()) {
        uint64_t now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(static_cast<double>(now - last) / freq);
        last = now;
        dt = std::min(dt, 0.25f); // pausas largas (depurador, suspensión) no disparan ráfagas de ticks
        m_time += dt;
        fpsTimer += dt;
        ++frames;
        if (fpsTimer >= 1.0) {
            m_fps = frames;
            frames = 0;
            fpsTimer = 0.0;
        }

        m_menuInput = MenuInput{};
        m_menuInput.mouseX = m_mouseX;
        m_menuInput.mouseY = m_mouseY;
        runScript();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            handleEvent(e);
        }

        if (m_menu.wantsTextInput() != SDL_TextInputActive(m_window.getSDLWindow())) {
            if (m_menu.wantsTextInput())
                SDL_StartTextInput(m_window.getSDLWindow());
            else
                SDL_StopTextInput(m_window.getSDLWindow());
        }

        if (m_game) {
            if (!m_game->ready)
                updateLoading(dt);
            else
                updateGameplay(dt);
        } else if (m_menu.current() == MenuScreen::Multiplayer) {
            m_lanBrowser.update();
        }

        m_hudState.heldItemNameAlpha = std::max(0.0f, m_hudState.heldItemNameAlpha - dt * 0.5f);
        m_hudState.actionMessageAlpha = std::max(0.0f, m_hudState.actionMessageAlpha - dt * 0.5f);
        render();
        m_window.swapBuffers();
    }

    if (m_game) leaveWorld();
    m_settings.saveDefault();
    Log::info("Saliendo limpiamente de OpenCraft...");
    return 0;
}

// =================================================================================================
// Entrada
// =================================================================================================

void Application::setMouseCaptured(bool captured) {
    m_mouseCaptured = captured;
    SDL_SetWindowRelativeMouseMode(m_window.getSDLWindow(), captured);
}

void Application::openInventory(bool open) {
    if (!m_game || m_inventoryOpen == open) return;
    m_inventoryOpen = open;
    if (!open) {
        m_game->session.queueCommand(CloseContainerCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
    }
    setMouseCaptured(!open);
    m_leftDown = m_rightDown = false;
    if (open) {
        // Centrar el puntero al abrir, como en el juego original
        m_mouseX = static_cast<float>(m_window.getWidth()) * 0.5f;
        m_mouseY = static_cast<float>(m_window.getHeight()) * 0.5f;
        SDL_WarpMouseInWindow(m_window.getSDLWindow(), m_mouseX, m_mouseY);
    }
}

void Application::pauseGame() {
    if (m_inventoryOpen) openInventory(false);
    m_menu.open(MenuScreen::Pause);
    setMouseCaptured(false);
    m_leftDown = m_rightDown = false;
}

void Application::handleEvent(const SDL_Event& e) {
    m_window.onEvent(e);
    const bool playing = m_game && m_game->ready && !m_menu.isBlockingGame();

    switch (e.type) {
        case SDL_EVENT_WILL_ENTER_BACKGROUND:
            m_lifecycle.onWillEnterBackground();
            if (playing) pauseGame();
            return;
        case SDL_EVENT_DID_ENTER_BACKGROUND: m_lifecycle.onDidEnterBackground(); return;
        case SDL_EVENT_WILL_ENTER_FOREGROUND: m_lifecycle.onWillEnterForeground(); return;
        case SDL_EVENT_DID_ENTER_FOREGROUND: m_lifecycle.onDidEnterForeground(); return;
        case SDL_EVENT_LOW_MEMORY: m_lifecycle.onLowMemory(); return;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            m_touch.updateScreenDimensions(m_window.getWidth(), m_window.getHeight());
            return;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            m_keysDown.clear();
            m_leftDown = m_rightDown = false;
            if (playing && !m_game->lanOpen && m_config.scriptPath.empty()) pauseGame();
            return;
        case SDL_EVENT_GAMEPAD_ADDED:
            if (!m_sdlGamepad) m_sdlGamepad = SDL_OpenGamepad(e.gdevice.which);
            return;
        case SDL_EVENT_GAMEPAD_REMOVED:
            if (m_sdlGamepad && SDL_GetGamepadID(m_sdlGamepad) == e.gdevice.which) {
                SDL_CloseGamepad(m_sdlGamepad);
                m_sdlGamepad = nullptr;
                m_gamepad.reset();
            }
            return;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            m_gamepad.updateAxis(e.gaxis.axis, static_cast<float>(e.gaxis.value) / 32767.0f);
            return;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            m_gamepad.updateButton(e.gbutton.button, e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
            if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && m_menu.isBlockingGame()) {
                if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) m_menuInput.enter = true;
                if (e.gbutton.button == SDL_GAMEPAD_BUTTON_EAST) m_menuInput.escape = true;
                if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP) m_menuInput.up = true;
                if (e.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) m_menuInput.down = true;
            }
            return;
        default: break;
    }

    // Toques: alimentan los controles táctiles en juego; en menús SDL los convierte en ratón
    if (e.type == SDL_EVENT_FINGER_DOWN || e.type == SDL_EVENT_FINGER_UP || e.type == SDL_EVENT_FINGER_MOTION ||
        e.type == SDL_EVENT_FINGER_CANCELED) {
        m_touchSeen = true;
        if (!playing || m_inventoryOpen || !m_settings.touchControlsEnabled) return;
        float w = static_cast<float>(m_window.getWidth()), h = static_cast<float>(m_window.getHeight());
        float fx = e.tfinger.x * w, fy = e.tfinger.y * h;
        auto id = static_cast<int64_t>(e.tfinger.fingerID);
        if (e.type == SDL_EVENT_FINGER_DOWN)
            m_touch.handleFingerDown(id, fx, fy);
        else if (e.type == SDL_EVENT_FINGER_UP)
            m_touch.handleFingerUp(id, fx, fy);
        else if (e.type == SDL_EVENT_FINGER_MOTION)
            m_touch.handleFingerMotion(id, fx, fy, e.tfinger.dx * w, e.tfinger.dy * h);
        else
            m_touch.handleFingerCanceled(id);
        return;
    }

    if (e.type == SDL_EVENT_MOUSE_MOTION) {
        m_mouseX = e.motion.x;
        m_mouseY = e.motion.y;
        m_menuInput.mouseX = m_mouseX;
        m_menuInput.mouseY = m_mouseY;
        if (playing && m_mouseCaptured && !m_inventoryOpen && e.motion.which != SDL_TOUCH_MOUSEID) {
            float sens = m_settings.mouseSensitivity;
            float ySign = m_settings.invertY ? 1.0f : -1.0f;
            m_camera.processMouseMovement(e.motion.xrel * sens, e.motion.yrel * sens * ySign);
            m_tutorial.onPlayerRotated(std::abs(e.motion.xrel) + std::abs(e.motion.yrel));
        }
        return;
    }

    if (e.type == SDL_EVENT_TEXT_INPUT) {
        m_menuInput.text += e.text.text;
        return;
    }

    if (e.type == SDL_EVENT_MOUSE_WHEEL) {
        m_menuInput.wheel += e.wheel.y;
        if (playing && !m_inventoryOpen && e.wheel.y != 0.0f) {
            size_t slot = m_game->session.getPlayer().selectedSlot;
            int delta = e.wheel.y > 0 ? -1 : 1;
            slot = static_cast<size_t>((static_cast<int>(slot) + delta + 9) % 9);
            m_game->session.queueCommand(SelectSlotCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .slot = slot});
        }
        return;
    }

    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        bool down = e.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
        bool fromTouch = e.button.which == SDL_TOUCH_MOUSEID;
        m_mouseX = e.button.x;
        m_mouseY = e.button.y;
        m_menuInput.mouseX = m_mouseX;
        m_menuInput.mouseY = m_mouseY;
        if (m_menu.isBlockingGame()) {
            if (down && e.button.button == SDL_BUTTON_LEFT) {
                uint64_t t = nowMs();
                m_menuInput.clicked = true;
                m_menuInput.doubleClicked = (t - m_lastClickMs) < 400;
                m_lastClickMs = t;
            } else if (down && e.button.button == SDL_BUTTON_RIGHT) {
                m_menuInput.rightClicked = true;
            }
            return;
        }
        if (!playing) return;
        if (m_inventoryOpen) {
            if (down) {
                bool shift = m_keysDown.contains(SDLK_LSHIFT) || m_keysDown.contains(SDLK_RSHIFT);
                handleInventoryClick(!fromTouch && e.button.button == SDL_BUTTON_RIGHT, shift && !fromTouch);
            }
            return;
        }
        if (fromTouch) return; // en juego los toques los gestiona TouchController
        if (down && !m_mouseCaptured) {
            setMouseCaptured(true);
            return;
        }
        if (e.button.button == SDL_BUTTON_LEFT) {
            m_leftDown = down;
            if (down) {
                m_swing = 0.001f;
                if (attackEntityInSight()) {
                    m_leftDown = false;
                } else if (m_game->session.getPlayer().gameMode == 1) {
                    // En creativo el bloque se rompe al instante con la pulsación
                    if (auto hit = pickBlock()) {
                        m_game->session.queueCommand(BreakBlockCommand{
                            .entityId = LOCAL_PLAYER_ENTITY_ID, .x = hit->blockX, .y = hit->blockY, .z = hit->blockZ});
                        m_game->breakCooldown = 0.25f;
                    }
                }
            }
        } else if (e.button.button == SDL_BUTTON_RIGHT) {
            m_rightDown = down;
            // La acción ocurre al pulsar (un clic más corto que un fotograma no se pierde);
            // mantener pulsado la repite con el intervalo del juego original
            if (down) {
                if (auto hit = pickBlock())
                    useItem(*hit);
                else
                    continuousActions(0.0f, std::nullopt);
                m_game->useCooldown = 0.25f;
            }
        } else if (e.button.button == SDL_BUTTON_MIDDLE && down) {
            // Elegir bloque: en creativo añade una pila del bloque apuntado al inventario
            if (auto hit = pickBlock(); hit && m_game->session.getPlayer().gameMode == 1) {
                ItemId item = ItemRegistry::global().getItemForBlock(hit->blockState);
                if (item != EMPTY_ITEM_ID) {
                    m_game->session.queueCommand(CreativePickCommand{
                        .entityId = LOCAL_PLAYER_ENTITY_ID, .itemId = item, .count = 64, .toInventory = true});
                }
            }
        }
        return;
    }

    if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) {
        SDL_Keycode key = e.key.key;
        bool down = e.type == SDL_EVENT_KEY_DOWN;
        if (down)
            m_keysDown.insert(key);
        else
            m_keysDown.erase(key);
        if (!down) return;

        if (key == SDLK_F2) {
            m_screenshotRequested = true;
            return;
        }
        if (key == SDLK_F11) {
            m_window.setFullscreen(!m_window.isFullscreen());
            return;
        }
        if (m_menu.isBlockingGame()) {
            if (key == SDLK_ESCAPE) m_menuInput.escape = true;
            if (key == SDLK_RETURN || key == SDLK_KP_ENTER) m_menuInput.enter = true;
            if (key == SDLK_BACKSPACE) ++m_menuInput.backspace;
            if (key == SDLK_TAB) m_menuInput.tab = true;
            if (key == SDLK_UP) m_menuInput.up = true;
            if (key == SDLK_DOWN) m_menuInput.down = true;
            return;
        }
        if (playing) handleGameKey(key, e.key.repeat);
    }
}

bool Application::attackEntityInSight() {
    const auto snap = m_game->session.getSnapshot();
    Math::Vec3 eye = m_camera.position, dir = m_camera.getFront();
    EntityId best = INVALID_ENTITY_ID;
    float bestT = 3.5f;
    for (const auto& mob : snap.mobs) {
        Math::Vec3 p = mob.transform.currentPosition;
        const float mn[3] = {p.x - 0.4f, p.y, p.z - 0.4f}, mx[3] = {p.x + 0.4f, p.y + 1.9f, p.z + 0.4f};
        const float o[3] = {eye.x, eye.y, eye.z}, d[3] = {dir.x, dir.y, dir.z};
        float tmin = 0.0f, tmax = bestT;
        bool ok = true;
        for (int a = 0; a < 3 && ok; ++a) {
            if (std::abs(d[a]) < 1e-6f) {
                ok = o[a] >= mn[a] && o[a] <= mx[a];
            } else {
                float t1 = (mn[a] - o[a]) / d[a], t2 = (mx[a] - o[a]) / d[a];
                if (t1 > t2) std::swap(t1, t2);
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
                ok = tmin <= tmax;
            }
        }
        if (ok && tmin < bestT) {
            bestT = tmin;
            best = mob.id;
        }
    }
    if (best == INVALID_ENTITY_ID) return false;
    if (auto hit = pickBlock()) {
        Math::Vec3 c(hit->blockX + 0.5f, hit->blockY + 0.5f, hit->blockZ + 0.5f);
        if (std::sqrt((c - eye).dot(c - eye)) < bestT) return false; // un bloque tapa a la criatura
    }
    m_game->session.queueCommand(AttackEntityCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .targetEntityId = best});
    return true;
}

void Application::handleGameKey(SDL_Keycode key, bool repeat) {
    auto& session = m_game->session;
    if (key == SDLK_ESCAPE) {
        if (m_inventoryOpen)
            openInventory(false);
        else
            pauseGame();
        return;
    }
    if (key == SDLK_E) {
        openInventory(!m_inventoryOpen);
        return;
    }
    if (m_inventoryOpen) return;
    if (key >= SDLK_1 && key <= SDLK_9) {
        session.queueCommand(
            SelectSlotCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .slot = static_cast<size_t>(key - SDLK_1)});
    } else if (key == SDLK_Q) {
        bool ctrl = m_keysDown.contains(SDLK_LCTRL) || m_keysDown.contains(SDLK_RCTRL);
        session.queueCommand(DropItemCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                             .fromCursor = false,
                                             .slot = session.getPlayer().selectedSlot,
                                             .count = static_cast<uint16_t>(ctrl ? 0 : 1)});
    } else if (key == SDLK_F1) {
        m_hideHud = !m_hideHud;
    } else if (key == SDLK_F3) {
        m_hudState.showDebug = !m_hudState.showDebug;
    } else if (key == SDLK_SPACE && !repeat) {
        if (!session.getPlayer().isAlive) return;
        uint64_t t = nowMs();
        if (session.getPlayer().gameMode == 1 && t - m_lastJumpMs < 300) {
            session.queueCommand(ToggleFlightCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
            m_lastJumpMs = 0;
        } else {
            m_lastJumpMs = t;
        }
        m_jumpQueued = true;
    }
}

void Application::handleInventoryClick(bool right, bool shift) {
    Game& g = *m_game;
    WorldSnapshot snap = g.session.getSnapshot();
    const int W = m_window.getWidth(), H = m_window.getHeight();

    int page = m_hud.getCreativePageButtonAt(W, H, m_hudState, m_mouseX, m_mouseY);
    if (page != 0 && snap.gameMode == 1 && !snap.isContainerOpen) {
        m_hudState.creativePage = std::clamp(m_hudState.creativePage + page, 0, HUD::creativePageCount() - 1);
        return;
    }

    SlotHit hit = m_hud.getSlotAt(W, H, snap, m_hudState, m_mouseX, m_mouseY);
    if (!hit.hit) {
        // Clic fuera del panel con un objeto en el cursor: se tira al suelo
        if (!snap.cursorItem.isEmpty() && !m_hud.isInsidePanel(W, H, m_hudState, m_mouseX, m_mouseY)) {
            g.session.queueCommand(DropItemCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                   .fromCursor = true,
                                                   .slot = 0,
                                                   .count = static_cast<uint16_t>(right ? 1 : 0)});
        }
        return;
    }
    if (hit.creativePalette) {
        if (!snap.cursorItem.isEmpty()) {
            g.session.queueCommand(CreativePickCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .itemId = 0, .count = 0});
        } else if (hit.paletteItem != EMPTY_ITEM_ID) {
            uint16_t full = ItemRegistry::global().get(hit.paletteItem).maxStackSize;
            g.session.queueCommand(CreativePickCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                       .itemId = hit.paletteItem,
                                                       .count = right ? uint16_t{1} : full,
                                                       .toInventory = shift});
        }
        return;
    }

    if (hit.container == InventoryContainerType::CraftingOutput) {
        if (snap.craftingOutput.isEmpty()) return;
        if (snap.cursorItem.isEmpty() ||
            (snap.cursorItem.canStackWith(snap.craftingOutput) &&
             snap.cursorItem.count + snap.craftingOutput.count <= snap.cursorItem.getMaxStack())) {
            g.session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                               .action = InventoryAction::Move,
                                                               .fromContainer = InventoryContainerType::CraftingOutput,
                                                               .fromSlot = 0,
                                                               .toContainer = InventoryContainerType::Cursor,
                                                               .toSlot = 0,
                                                               .amount = 0,
                                                               .expectedRevision = 0});
            m_tutorial.onItemCrafted(1);
        }
        return;
    }

    if (shift && snap.cursorItem.isEmpty()) {
        // Transferencia rápida entre el contenedor abierto y el inventario
        InventoryContainerType target = InventoryContainerType::PlayerInventory;
        if (hit.container == InventoryContainerType::PlayerInventory && snap.isContainerOpen) {
            if (snap.openContainerKind == OpenContainerKind::Chest)
                target = InventoryContainerType::ChestContainer;
            else if (snap.openContainerKind == OpenContainerKind::Furnace)
                target = InventoryContainerType::FurnaceContainer;
        }
        if (target != hit.container) {
            g.session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                               .action = InventoryAction::QuickTransfer,
                                                               .fromContainer = hit.container,
                                                               .fromSlot = hit.slotIndex,
                                                               .toContainer = target,
                                                               .toSlot = 0,
                                                               .amount = 0,
                                                               .expectedRevision = 0});
            return;
        }
        // Dentro del inventario: entre la barra rápida (0-8) y la mochila (9-35)
        const auto& inv = snap.inventorySlots;
        if (hit.slotIndex >= inv.size() || inv[hit.slotIndex].isEmpty()) return;
        size_t from = hit.slotIndex;
        size_t lo = from < 9 ? 9 : 0, hi = from < 9 ? inv.size() : 9;
        size_t dest = SIZE_MAX;
        for (size_t i = lo; i < hi && dest == SIZE_MAX; ++i)
            if (!inv[i].isEmpty() && inv[i].canStackWith(inv[from]) && inv[i].count < inv[i].getMaxStack()) dest = i;
        for (size_t i = lo; i < hi && dest == SIZE_MAX; ++i)
            if (inv[i].isEmpty()) dest = i;
        if (dest != SIZE_MAX) {
            g.session.queueCommand(InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                               .action = InventoryAction::Move,
                                                               .fromContainer = InventoryContainerType::PlayerInventory,
                                                               .fromSlot = from,
                                                               .toContainer = InventoryContainerType::PlayerInventory,
                                                               .toSlot = dest,
                                                               .amount = 0,
                                                               .expectedRevision = 0});
        }
        return;
    }

    bool cursorEmpty = snap.cursorItem.isEmpty();
    uint16_t amount = 0;
    if (right) {
        if (cursorEmpty) {
            ItemStack src;
            if (hit.container == InventoryContainerType::ChestContainer ||
                hit.container == InventoryContainerType::FurnaceContainer) {
                if (hit.slotIndex < snap.containerSlots.size()) src = snap.containerSlots[hit.slotIndex];
            } else if (hit.container == InventoryContainerType::CraftingGrid) {
                if (hit.slotIndex < snap.craftingGrid.size()) src = snap.craftingGrid[hit.slotIndex];
            } else if (hit.slotIndex < snap.inventorySlots.size()) {
                src = snap.inventorySlots[hit.slotIndex];
            }
            amount = static_cast<uint16_t>((src.count + 1) / 2);
        } else {
            amount = 1;
        }
    }
    g.session.queueCommand(
        InventoryTransactionCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                    .action = InventoryAction::Move,
                                    .fromContainer = cursorEmpty ? hit.container : InventoryContainerType::Cursor,
                                    .fromSlot = cursorEmpty ? hit.slotIndex : 0,
                                    .toContainer = cursorEmpty ? InventoryContainerType::Cursor : hit.container,
                                    .toSlot = cursorEmpty ? 0 : hit.slotIndex,
                                    .amount = amount,
                                    .expectedRevision = 0});
}

std::optional<RaycastHit> Application::pickBlock() const {
    if (!m_game) return std::nullopt;
    float reach = m_game->session.getPlayer().gameMode == 1 ? 5.0f : 4.5f;
    RaycastHit hit = VoxelRaycast::cast(m_game->session.getWorld(), m_camera.position, m_camera.getFront(), reach);
    if (!hit.hit) return std::nullopt;
    return hit;
}

void Application::useItem(const RaycastHit& hit) {
    auto& session = m_game->session;
    const auto& player = session.getPlayer();
    const auto& reg = BlockRegistry::global();
    const auto& props = reg.get(session.getWorld().getBlockState(hit.blockX, hit.blockY, hit.blockZ));
    bool sneaking = m_keysDown.contains(SDLK_LSHIFT) || m_keysDown.contains(SDLK_RSHIFT) || m_touch.isSneakActive();
    ItemStack held =
        player.selectedSlot < player.inventory.getSize() ? player.inventory.getSlot(player.selectedSlot) : ItemStack();
    const auto& def = ItemRegistry::global().get(held.itemId);
    m_swing = 0.001f;

    if (isInteractiveBlock(props) && !(sneaking && !held.isEmpty())) {
        session.queueCommand(InteractBlockCommand{
            .entityId = LOCAL_PLAYER_ENTITY_ID, .x = hit.blockX, .y = hit.blockY, .z = hit.blockZ});
        if (opensContainer(props)) openInventory(true);
        return;
    }
    if (held.isEmpty()) return;
    if (def.isFood && (player.hunger < 20.0f || player.gameMode == 1)) {
        session.queueCommand(EatFoodCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
        return;
    }
    if (def.placeableBlockState == AIR_BLOCK_STATE) {
        // Cubos, mechero, semillas, Ojo del Confín...: interacción contra el bloque apuntado
        session.queueCommand(InteractBlockCommand{
            .entityId = LOCAL_PLAYER_ENTITY_ID, .x = hit.blockX, .y = hit.blockY, .z = hit.blockZ});
        return;
    }
    int tx = hit.blockX + static_cast<int>(hit.normal.x);
    int ty = hit.blockY + static_cast<int>(hit.normal.y);
    int tz = hit.blockZ + static_cast<int>(hit.normal.z);
    session.queueCommand(PlaceBlockCommand{
        .entityId = LOCAL_PLAYER_ENTITY_ID, .x = tx, .y = ty, .z = tz, .blockState = def.placeableBlockState});
    m_tutorial.onBlockPlaced(static_cast<uint16_t>(reg.get(def.placeableBlockState).legacyId));
}

void Application::continuousActions(float dt, const std::optional<RaycastHit>& hit) {
    Game& g = *m_game;
    auto& session = g.session;
    const auto& player = session.getPlayer();
    bool creative = player.gameMode == 1;
    bool attack = (m_leftDown && m_mouseCaptured) || m_touch.isAttackPressed() || m_gamepad.isAttackPressed();
    bool use = (m_rightDown && m_mouseCaptured) || m_touch.isUsePressed() || m_gamepad.isUsePressed();

    g.breakCooldown = std::max(0.0f, g.breakCooldown - dt);
    g.useCooldown = std::max(0.0f, g.useCooldown - dt);

    if (attack && hit && player.gameMode != 3) {
        if (m_swing <= 0.0f) m_swing = 0.001f;
        BlockPos pos{hit->blockX, hit->blockY, hit->blockZ};
        if (creative) {
            if (g.breakCooldown <= 0.0f) {
                session.queueCommand(
                    BreakBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .x = pos.x, .y = pos.y, .z = pos.z});
                g.breakCooldown = 0.25f;
            }
        } else {
            uint64_t tick = session.getClock().getTickCount();
            if (!g.digging || g.digPos != pos) {
                session.queueCommand(DigBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                     .x = pos.x,
                                                     .y = pos.y,
                                                     .z = pos.z,
                                                     .action = DigAction::Start});
                g.digging = true;
                g.digPos = pos;
                g.lastDigTick = tick;
            } else if (tick != g.lastDigTick) {
                // Un avance de minado por tick de simulación transcurrido
                for (uint64_t t = g.lastDigTick; t < tick && t < g.lastDigTick + 4; ++t) {
                    session.queueCommand(DigBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                                         .x = pos.x,
                                                         .y = pos.y,
                                                         .z = pos.z,
                                                         .action = DigAction::Progress});
                }
                g.lastDigTick = tick;
            }
        }
    } else if (g.digging) {
        session.queueCommand(DigBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                             .x = g.digPos.x,
                                             .y = g.digPos.y,
                                             .z = g.digPos.z,
                                             .action = DigAction::Cancel});
        g.digging = false;
    }
    if (g.digging && session.getWorld().getBlockState(g.digPos.x, g.digPos.y, g.digPos.z) == AIR_BLOCK_STATE) {
        g.digging = false;
    }

    if (use && g.useCooldown <= 0.0f) {
        if (hit) {
            useItem(*hit);
        } else {
            ItemStack held = player.selectedSlot < player.inventory.getSize()
                                 ? player.inventory.getSlot(player.selectedSlot)
                                 : ItemStack();
            if (!held.isEmpty() && ItemRegistry::global().get(held.itemId).isFood && player.hunger < 20.0f) {
                session.queueCommand(EatFoodCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
            }
        }
        g.useCooldown = 0.25f;
    }
}

void Application::updateGameplay(float dt) {
    Game& g = *m_game;
    auto& session = g.session;
    const bool menuOpen = m_menu.isBlockingGame();

    // Acciones de táctil y mando
    if (!menuOpen) {
        if (m_touch.isInventoryTriggered() || m_gamepad.consumeInventoryTrigger()) openInventory(!m_inventoryOpen);
        if (m_touch.isPauseTriggered() || m_gamepad.consumePauseTrigger()) pauseGame();
        int touchSlot = m_touch.getSelectedHotbarSlot();
        if (touchSlot >= 0)
            session.queueCommand(
                SelectSlotCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .slot = static_cast<size_t>(touchSlot)});
        int padDelta = m_gamepad.consumeHotbarDelta();
        if (padDelta != 0) {
            size_t slot = static_cast<size_t>((static_cast<int>(session.getPlayer().selectedSlot) + padDelta + 9) % 9);
            session.queueCommand(SelectSlotCommand{.entityId = LOCAL_PLAYER_ENTITY_ID, .slot = slot});
        }
    }

    // Movimiento
    bool controlling = !m_menu.isBlockingGame() && !m_inventoryOpen;
    Math::Vec3 move{0.0f, 0.0f, 0.0f};
    bool jump = false, sprint = false, sneak = false;
    if (controlling) {
        if (m_keysDown.contains(SDLK_W)) move.z += 1.0f;
        if (m_keysDown.contains(SDLK_S)) move.z -= 1.0f;
        if (m_keysDown.contains(SDLK_D)) move.x += 1.0f;
        if (m_keysDown.contains(SDLK_A)) move.x -= 1.0f;
        Math::Vec2 tm = m_touch.getMovementVector();
        Math::Vec2 pm = m_gamepad.getMovementVector();
        move.x += tm.x + pm.x;
        move.z += tm.y + pm.y; // ambos con +1 = adelante (el mando ya invierte su eje Y)
        float ySign = m_settings.invertY ? 1.0f : -1.0f;
        Math::Vec2 look = m_touch.getLookDelta();
        if (look.x != 0.0f || look.y != 0.0f) {
            float k = m_settings.touchSensitivity / 100.0f;
            m_camera.processMouseMovement(look.x * k, look.y * k * ySign);
        }
        Math::Vec2 padLook = m_gamepad.getLookVector(180.0f * dt);
        if (padLook.x != 0.0f || padLook.y != 0.0f) m_camera.processMouseMovement(padLook.x, padLook.y * ySign);

        jump = m_keysDown.contains(SDLK_SPACE) || m_jumpQueued || m_touch.isJumpPressed() || m_gamepad.isJumpPressed();
        sprint = m_keysDown.contains(SDLK_LCTRL) || m_keysDown.contains(SDLK_RCTRL);
        sneak = m_keysDown.contains(SDLK_LSHIFT) || m_keysDown.contains(SDLK_RSHIFT) || m_touch.isSneakActive() ||
                m_gamepad.isSneakPressed();

        // Salto automático (opción de accesibilidad, útil en pantallas táctiles)
        if (m_settings.autoJump && move.z > 0.1f && session.getPlayer().isGrounded) {
            const auto& p = session.getPlayer();
            float yr = Math::radians(m_camera.yaw);
            int fx = static_cast<int>(std::floor(p.position.x + std::cos(yr) * 0.7f));
            int fz = static_cast<int>(std::floor(p.position.z + std::sin(yr) * 0.7f));
            int fy = static_cast<int>(std::floor(p.position.y + 0.01f));
            const auto& reg = BlockRegistry::global();
            const auto& w = session.getWorld();
            if (reg.get(w.getBlockState(fx, fy, fz)).isSolid && !reg.get(w.getBlockState(fx, fy + 1, fz)).isSolid &&
                !reg.get(w.getBlockState(fx, fy + 2, fz)).isSolid) {
                jump = true;
            }
        }
        if (move.dot(move) > 0.01f) m_tutorial.onPlayerMoved(std::sqrt(move.dot(move)) * dt * 4.3f);
    }
    m_jumpQueued = false;
    session.queueCommand(MoveCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                     .moveInput = move,
                                     .yaw = m_camera.yaw,
                                     .pitch = m_camera.pitch,
                                     .jump = jump,
                                     .sprint = sprint,
                                     .sneak = sneak});

    std::optional<RaycastHit> hit = controlling ? pickBlock() : std::nullopt;
    if (controlling) {
        continuousActions(dt, hit);
    } else if (g.digging) {
        session.queueCommand(DigBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                             .x = g.digPos.x,
                                             .y = g.digPos.y,
                                             .z = g.digPos.z,
                                             .action = DigAction::Cancel});
        g.digging = false;
    }

    // La partida solo se detiene en el menú de pausa si nadie más juega en ella
    bool paused = m_menu.current() == MenuScreen::Pause && !g.lanOpen;
    if (m_menu.current() == MenuScreen::Options || m_menu.current() == MenuScreen::Message) paused = !g.lanOpen;
    if (!paused) {
        session.update(dt);
    }
    g.net.update(session);

    std::vector<GameEvent> events;
    events.swap(g.events.events);
    for (const auto& ev : events) {
        if (const auto* bc = std::get_if<BlockChangedEvent>(&ev)) {
            // Solo los cambios del jugador suenan (el agua que fluye o los cultivos no)
            bool byPlayer = bc->causeEntityId == session.getPlayer().id || bc->causeEntityId == LOCAL_PLAYER_ENTITY_ID;
            if (!byPlayer) continue;
            Math::Vec3 pos(bc->x + 0.5f, bc->y + 0.5f, bc->z + 0.5f);
            if (bc->newState == AIR_BLOCK_STATE && bc->oldState != AIR_BLOCK_STATE) {
                m_particles.spawnBlockBreakParticles(pos, 0.55f, 0.45f, 0.35f, 12);
                playSfx("block.break", AudioCategory::Blocks, &pos);
                m_subtitles.addSubtitle(LocalizationManager::global().get("subtitles.break"), pos,
                                        AudioCategory::Blocks);
                m_tutorial.onBlockMined(1);
            } else if (bc->oldState == AIR_BLOCK_STATE && bc->newState != AIR_BLOCK_STATE) {
                playSfx("block.place", AudioCategory::Blocks, &pos, 0.9f);
            }
        } else if (std::get_if<PlayerHurtEvent>(&ev)) {
            playSfx("entity.player.hurt", AudioCategory::Players, nullptr);
        } else if (std::get_if<ItemPickedUpEvent>(&ev)) {
            playSfx("entity.item.pickup", AudioCategory::Players, nullptr,
                    1.0f + static_cast<float>(std::fmod(m_time * 7.0, 0.4)));
        } else if (const auto* ex = std::get_if<ExplosionEvent>(&ev)) {
            playSfx("entity.generic.explode", AudioCategory::Hostile, &ex->position);
        } else if (std::get_if<PlayerAteEvent>(&ev)) {
            playSfx("entity.player.eat", AudioCategory::Players, nullptr);
        } else if (const auto* mh = std::get_if<MobHurtEvent>(&ev)) {
            for (const auto& mob : session.getMobs()) {
                if (mob.id == mh->entityId) {
                    playSfx("entity.hurt", AudioCategory::Hostile, &mob.position, 1.2f);
                    break;
                }
            }
        }
    }

    // Pasos: un sonido cada ~1,7 bloques recorridos sobre el suelo
    {
        const auto& pl = session.getPlayer();
        float horizSpeed = std::sqrt(pl.velocity.x * pl.velocity.x + pl.velocity.z * pl.velocity.z);
        if (pl.isGrounded && !pl.isFlying && horizSpeed > 0.5f && !paused) {
            m_stepDistance += horizSpeed * dt;
            if (m_stepDistance > 1.7f) {
                m_stepDistance = 0.0f;
                Math::Vec3 feet = pl.position;
                playSfx("step", AudioCategory::Players, &feet,
                        0.9f + static_cast<float>(std::fmod(m_time * 13.0, 0.2)));
            }
        }
    }

    // Muerte
    const auto& player = session.getPlayer();
    if (!player.isAlive && !g.deathHandled) {
        g.deathHandled = true;
        if (m_inventoryOpen) openInventory(false);
        m_menu.open(MenuScreen::Death);
        setMouseCaptured(false);
        m_leftDown = m_rightDown = false;
    }

    // Streaming de terreno
    g.chunks.update(player.position.x, player.position.z);
    auto dim = static_cast<DimensionType>(session.getCurrentDimension());
    if (dim != g.chunks.getDimension()) {
        g.chunks.setDimension(dim);
        g.renderer.clear();
    }

    // Nombre del objeto en la mano al cambiar de ranura
    if (player.selectedSlot < player.inventory.getSize()) {
        uint16_t item = player.inventory.getSlot(player.selectedSlot).itemId;
        if (player.selectedSlot != m_lastSelectedSlot || item != m_lastHeldItem) {
            m_lastSelectedSlot = player.selectedSlot;
            m_lastHeldItem = item;
            m_hudState.heldItemName = item != EMPTY_ITEM_ID ? localizedItemName(item) : "";
            m_hudState.heldItemNameAlpha = item != EMPTY_ITEM_ID ? 2.5f : 0.0f;
        }
    }

    m_audio.setListener(m_camera.position, m_camera.getFront(), m_camera.getUp());
    m_subtitles.update(dt, m_camera.position, m_camera.getFront(), m_camera.getUp());
    m_particles.update(dt, &session.getWorld());
    m_benchmark.recordFrameTimeMs(static_cast<double>(dt) * 1000.0);

    if (m_swing > 0.0f) {
        m_swing += dt * 3.3f;
        if (m_swing >= 1.0f) m_swing = (m_leftDown && m_mouseCaptured && hit) ? 0.001f : 0.0f;
    }
    float horiz = std::sqrt(player.velocity.x * player.velocity.x + player.velocity.z * player.velocity.z);
    if (player.isGrounded) m_bob += dt * horiz * 0.35f;

    g.autoSaveTimer += dt;
    if (g.autoSaveTimer >= m_config.autoSaveInterval) {
        g.autoSaveTimer = 0.0f;
        saveGame();
    }
}

// =================================================================================================
// Menús
// =================================================================================================

void Application::applySettings() {
    m_window.setVsync(m_settings.vsync);
    m_audioOut.setMasterVolume(static_cast<float>(m_settings.masterVolume) / 100.0f);
    m_camera.fov = static_cast<float>(m_settings.fov);
    if (m_game) {
        m_game->session.getWorld().setRenderDistance(m_settings.renderDistance);
    }
    m_settings.saveDefault();
}

void Application::handleMenuEvents(const std::vector<MenuEvent>& events) {
    for (const auto& ev : events) {
        switch (ev.type) {
            case MenuEvent::Type::Quit: m_window.setShouldClose(true); break;
            case MenuEvent::Type::PlayWorld: startWorld(ev.worldId); break;
            case MenuEvent::Type::CreateWorld:
                createAndStartWorld(ev.name, ev.seedText, ev.gameMode, ev.hardcore);
                break;
            case MenuEvent::Type::DeleteWorld:
                if (!m_worldManager->deleteWorld(ev.worldId)) {
                    m_menu.showMessage("No se pudo eliminar el mundo",
                                       "El mundo está en uso o no se pudo crear su copia de seguridad.",
                                       MenuScreen::SelectWorld);
                }
                refreshWorldList();
                break;
            case MenuEvent::Type::Resume:
                m_menu.open(MenuScreen::None);
                setMouseCaptured(true);
                break;
            case MenuEvent::Type::SaveAndQuit:
            case MenuEvent::Type::ReturnToTitle:
                leaveWorld();
                m_menu.open(MenuScreen::Title);
                break;
            case MenuEvent::Type::Respawn:
                if (m_game) {
                    m_game->session.queueCommand(RespawnCommand{.entityId = LOCAL_PLAYER_ENTITY_ID});
                    m_game->session.update(SimulationClock::TICK_DURATION);
                    // Recargar el terreno del punto de reaparición antes de devolver el control
                    m_game->ready = false;
                    m_game->loadTimer = 0.0f;
                    m_game->freshWorld = false;
                    m_menu.open(MenuScreen::Loading);
                }
                break;
            case MenuEvent::Type::SettingsChanged: applySettings(); break;
            case MenuEvent::Type::OpenToLan:
                if (m_game && m_game->net.startHost(LAN_PORT, m_game->worldId)) {
                    m_game->lanOpen = true;
                    m_hudState.actionMessage = std::format("Partida abierta en la red local (puerto {})", LAN_PORT);
                    m_hudState.actionMessageAlpha = 4.0f;
                } else {
                    m_menu.showMessage("No se pudo abrir en LAN", "El puerto está ocupado o la red no está disponible.",
                                       MenuScreen::Pause);
                }
                break;
            case MenuEvent::Type::RefreshServers:
                m_lanBrowser.stop();
                m_lanBrowser.startListening();
                break;
            case MenuEvent::Type::JoinServer:
                m_menu.showMessage(
                    "Multijugador como cliente no disponible",
                    std::format("Unirse a {}:{} aún no sincroniza el mundo del anfitrión en esta versión "
                                "candidata (limitación conocida). El servidor dedicado y la apertura en LAN "
                                "sí están disponibles.",
                                ev.host, ev.port),
                    MenuScreen::Multiplayer);
                break;
        }
    }
}

// =================================================================================================
// Render
// =================================================================================================

Math::Vec3 Application::skyColor() const {
    if (!m_game) return Math::Vec3(0.53f, 0.75f, 0.92f);
    const auto& s = m_game->session;
    if (s.getCurrentDimension() == 1) return Math::Vec3(0.2f, 0.04f, 0.03f);
    if (s.getCurrentDimension() == 2) return Math::Vec3(0.06f, 0.04f, 0.09f);
    float rain = s.getWeather().state != WeatherState::Clear ? 1.0f : 0.0f;
    float sun = s.getClock().getSunlightFactor(rain);
    float t = std::clamp((sun - 0.1f) / 0.9f, 0.0f, 1.0f);
    Math::Vec3 day = rain > 0.0f ? Math::Vec3(0.45f, 0.52f, 0.6f) : Math::Vec3(0.53f, 0.75f, 0.92f);
    Math::Vec3 night(0.01f, 0.015f, 0.04f);
    return night * (1.0f - t) + day * t;
}

DebugInfo Application::buildDebugInfo(const WorldSnapshot& snap, const std::optional<RaycastHit>& hit) const {
    DebugInfo d;
    d.fps = m_fps;
    const Game& g = *m_game;
    d.loadedChunks = g.session.getWorld().getLoadedChunkCount();
    auto rs = g.renderer.getStats();
    d.renderedSections = rs.renderedSections;
    d.pendingChunks = rs.pendingChunks;
    float yaw = std::fmod(std::fmod(m_camera.yaw, 360.0f) + 360.0f, 360.0f);
    // yaw 0 = +X (este), 90 = +Z (sur)
    const char* dirs[] = {"este (+X)", "sur (+Z)", "oeste (-X)", "norte (-Z)"};
    d.facing = std::format("{} ({:.1f} / {:.1f})", dirs[static_cast<int>((yaw + 45.0f) / 90.0f) % 4], m_camera.yaw,
                           m_camera.pitch);
    auto p = snap.player.currentPosition;
    int bx = static_cast<int>(std::floor(p.x)), by = static_cast<int>(std::floor(p.y)),
        bz = static_cast<int>(std::floor(p.z));
    d.skyLight = g.session.getWorld().getSkyLight(bx, by, bz);
    d.blockLight = g.session.getWorld().getBlockLight(bx, by, bz);
    d.timeOfDay = g.session.getClock().getTimeOfDay();
    if (hit) {
        const auto& bp = BlockRegistry::global().get(hit->blockState);
        d.targetBlock = std::format("{} {} {} ({})", hit->blockX, hit->blockY, hit->blockZ, bp.namespacedId);
    }
    return d;
}

void Application::renderWorld(float alpha, const WorldSnapshot& snap, const std::optional<RaycastHit>& hit) {
    Game& g = *m_game;
    const int rd = m_settings.renderDistance;
    m_camera.fov = static_cast<float>(m_settings.fov) * (snap.isSprinting ? 1.1f : 1.0f);
    m_camera.farPlane = static_cast<float>(rd * 16 + 48);
    Math::Vec3 eye = snap.player.getInterpolatedEyePosition(alpha);
    if (snap.isSneaking) eye.y -= 0.08f;
    if (m_settings.viewBobbing && !snap.isFlying) eye.y += std::abs(std::sin(m_bob * 3.14159f)) * 0.05f;
    m_camera.position = eye;

    Math::Mat4 view = m_camera.getViewMatrix();
    Math::Mat4 proj = m_camera.getProjectionMatrix(m_window.getAspectRatio());
    Frustum frustum;
    frustum.updateFromViewProj(proj * view);

    int pcx = floorDiv16(snap.player.currentPosition.x), pcz = floorDiv16(snap.player.currentPosition.z);
    g.renderer.update(g.session.getWorld(), g.pool, pcx, pcz);
    std::vector<ChunkMeshBuildResult> meshes;
    g.pool.pollMeshes(meshes);
    g.renderer.applyCompletedMeshes(meshes, g.session.getWorld());

    Math::Vec3 fog = skyColor();
    float fogStart = static_cast<float>(rd * 16) * 0.6f, fogEnd = static_cast<float>(rd * 16) * 0.95f;
    if (snap.isHeadInWater) {
        fog = Math::Vec3(0.05f, 0.12f, 0.35f);
        fogStart = 1.0f;
        fogEnd = 24.0f;
    } else if (snap.isHeadInLava) {
        fog = Math::Vec3(0.6f, 0.15f, 0.0f);
        fogStart = 0.0f;
        fogEnd = 3.0f;
    }
    float rain = g.session.getWeather().state != WeatherState::Clear ? 1.0f : 0.0f;
    // La luz nocturna equivale a la luna (nivel ~4 de 15): de noche el terreno sigue intuyéndose
    float simSun = g.session.getClock().getSunlightFactor(rain);
    float sun =
        g.session.getCurrentDimension() == 0 ? 0.3f + 0.7f * std::clamp((simSun - 0.15f) / 0.85f, 0.0f, 1.0f) : 0.3f;
    float gamma = static_cast<float>(m_settings.brightness) / 100.0f;

    m_voxelShader.use();
    m_voxelShader.setMat4("uView", view);
    m_voxelShader.setMat4("uProjection", proj);
    m_voxelShader.setInt("uTextureAtlas", 0);
    m_voxelShader.setFloat("uSunlightFactor", sun);
    m_voxelShader.setFloat("uGamma", gamma);
    m_voxelShader.setVec3("uFogColor", fog);
    m_voxelShader.setFloat("uFogStart", fogStart);
    m_voxelShader.setFloat("uFogEnd", fogEnd);
    m_voxelShader.setFloat("uAlpha", 1.0f);
    m_atlas.bind(0);
    g.renderer.render(m_voxelShader, &frustum, eye);

    EntityLighting lighting{.sunlight = sun, .gamma = gamma, .fogColor = fog, .fogStart = fogStart, .fogEnd = fogEnd};
    m_entityRenderer.renderWorldEntities(snap, alpha, g.session.getWorld(), m_atlas, view, proj, lighting, m_time);
    if (hit && !m_inventoryOpen && !m_hideHud) {
        float progress =
            (snap.miningBlockPos == BlockPos{hit->blockX, hit->blockY, hit->blockZ}) ? snap.miningProgress : 0.0f;
        m_entityRenderer.renderBlockSelection(g.session.getWorld(), hit->blockX, hit->blockY, hit->blockZ, progress,
                                              m_atlas, view, proj, lighting);
    }
    if (!m_hideHud && snap.gameMode != 3) {
        uint16_t held =
            snap.selectedSlot < snap.inventorySlots.size() ? snap.inventorySlots[snap.selectedSlot].itemId : 0;
        int bx = static_cast<int>(std::floor(eye.x)), by = static_cast<int>(std::floor(eye.y)),
            bz = static_cast<int>(std::floor(eye.z));
        m_entityRenderer.renderFirstPerson(
            held, m_swing, m_bob * 3.14159f, static_cast<float>(g.session.getWorld().getSkyLight(bx, by, bz)),
            static_cast<float>(g.session.getWorld().getBlockLight(bx, by, bz)), m_atlas,
            Math::Mat4::perspective(Math::radians(70.0f), m_window.getAspectRatio(), 0.05f, 10.0f), lighting);
    }
}

void Application::render() {
    glViewport(0, 0, m_window.getFramebufferWidth(), m_window.getFramebufferHeight());
    bool worldVisible = m_game && m_game->ready;
    WorldSnapshot snap;
    if (worldVisible) snap = m_game->session.getSnapshot();

    Math::Vec3 sky = skyColor();
    if (worldVisible && snap.isHeadInWater) sky = Math::Vec3(0.05f, 0.12f, 0.35f);
    glClearColor(sky.x, sky.y, sky.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const int W = m_window.getWidth(), H = m_window.getHeight();
    m_hudState.guiScale = HUD::computeGuiScale(W, H, m_settings.uiScale);
    m_hudState.highContrast = m_settings.highContrast;

    std::optional<RaycastHit> hit;
    if (worldVisible) {
        if (!m_menu.isBlockingGame() && !m_inventoryOpen) hit = pickBlock();
        renderWorld(m_game->session.getClock().getInterpolationAlpha(), snap, hit);
    } else if (m_game) {
        // Pantalla de carga: el renderer malla en segundo plano los chunks que van llegando
        const auto& p = m_game->session.getPlayer();
        m_game->renderer.update(m_game->session.getWorld(), m_game->pool, floorDiv16(p.position.x),
                                floorDiv16(p.position.z));
        std::vector<ChunkMeshBuildResult> meshes;
        m_game->pool.pollMeshes(meshes);
        m_game->renderer.applyCompletedMeshes(meshes, m_game->session.getWorld());
    }

    m_ui.begin(W, H);
    if (worldVisible && !m_hideHud && m_menu.current() != MenuScreen::Death) {
        if (m_hudState.showDebug) m_hudState.debug = buildDebugInfo(snap, hit);
        if (m_inventoryOpen) {
            m_hud.renderInventory(m_ui, snap, m_hudState, m_mouseX, m_mouseY);
        } else {
            m_hud.renderInGame(m_ui, snap, m_hudState);
            if (m_settings.touchControlsEnabled && m_touchSeen && !m_menu.isBlockingGame()) {
                m_hud.renderTouchControls(m_ui, m_touch);
            }
            if (!m_menu.isBlockingGame() && !m_hudState.showDebug)
                m_hud.renderTutorialBanner(m_ui, m_tutorial, m_hudState.guiScale);
        }
        if (m_settings.subtitlesEnabled) m_hud.renderSubtitles(m_ui, m_subtitles, m_hudState.guiScale);
    }

    if (m_menu.isBlockingGame()) {
        MenuContext ctx;
        ctx.worlds = &m_worlds;
        ctx.settings = &m_settings;
        ctx.servers = &m_lanBrowser.getDiscoveredServers();
        ctx.inGame = worldVisible;
        ctx.lanOpen = m_game && m_game->lanOpen;
        ctx.lanPort = LAN_PORT;
        ctx.guiScale = m_hudState.guiScale;
        ctx.timeSeconds = m_time;
        ctx.loadingProgress = m_loadingProgress;
        ctx.loadingMessage = "Generando terreno...";
        if (worldVisible) {
            ctx.hardcoreDeath = snap.isHardcore;
            ctx.deathScore = snap.xpLevel;
        }
        handleMenuEvents(m_menu.frame(m_ui, m_menuInput, ctx));
        if (m_menu.consumeClickSound()) playSfx("ui.click", AudioCategory::Master, nullptr, 1.2f);
    }
    m_ui.end(&m_atlas);

    if (m_screenshotRequested) {
        m_screenshotRequested = false;
        takeScreenshot(m_screenshotPath);
        m_screenshotPath.clear();
    }
}

void Application::playSfx(const std::string& name, AudioCategory category, const Math::Vec3* position, float pitch) {
    SoundEvent ev{.name = name,
                  .category = category,
                  .position = position ? *position : m_camera.position,
                  .isSpatial = position != nullptr,
                  .volume = 1.0f,
                  .pitch = pitch};
    m_audio.playSound(ev, m_settings);
    if (!m_audioOut.isAvailable()) return;
    float left = 0.7f, right = 0.7f;
    if (position) {
        auto sp = m_audio.calculateSpatial(*position, ev.maxDistance);
        if (!sp.audible) return;
        left = sp.leftGain;
        right = sp.rightGain;
    }
    int categoryVolume = 100;
    switch (category) {
        case AudioCategory::Blocks: categoryVolume = m_settings.blocksVolume; break;
        case AudioCategory::Hostile: categoryVolume = m_settings.hostileVolume; break;
        case AudioCategory::Friendly: categoryVolume = m_settings.friendlyVolume; break;
        case AudioCategory::Players: categoryVolume = m_settings.playersVolume; break;
        case AudioCategory::Ambient: categoryVolume = m_settings.ambientVolume; break;
        case AudioCategory::Weather: categoryVolume = m_settings.weatherVolume; break;
        default: break;
    }
    float k = static_cast<float>(categoryVolume) / 100.0f;
    m_audioOut.play(name, left * k, right * k, pitch);
}

void Application::takeScreenshot(const std::string& path) {
    int w = m_window.getFramebufferWidth(), h = m_window.getFramebufferHeight();
    std::vector<uint8_t> pixels(static_cast<size_t>(w) * static_cast<size_t>(h) * 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    std::vector<uint8_t> flipped(pixels.size());
    const size_t row = static_cast<size_t>(w) * 4;
    for (int y = 0; y < h; ++y) {
        std::copy_n(pixels.begin() + static_cast<std::ptrdiff_t>(static_cast<size_t>(h - 1 - y) * row), row,
                    flipped.begin() + static_cast<std::ptrdiff_t>(static_cast<size_t>(y) * row));
    }
    for (size_t i = 3; i < flipped.size(); i += 4) flipped[i] = 255;

    std::filesystem::path out = path;
    if (out.empty()) {
        std::time_t t = std::time(nullptr);
        std::tm tm{};
        localtime_r(&t, &tm);
        char name[64];
        std::strftime(name, sizeof(name), "%Y-%m-%d_%H.%M.%S.png", &tm);
        out = PlatformPaths::getUserDataDir() / "screenshots" / name;
    }
    if (PngWriter::writeRGBA(out, flipped, static_cast<uint32_t>(w), static_cast<uint32_t>(h))) {
        Log::info("Captura guardada en {}", out.string());
        m_hudState.actionMessage = "Captura guardada: " + out.filename().string();
        m_hudState.actionMessageAlpha = 3.0f;
    } else {
        Log::error("No se pudo guardar la captura en {}", out.string());
    }
}

// =================================================================================================
// Automatización (--script)
// =================================================================================================

bool Application::loadScript(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        Log::error("No se pudo abrir el guion de pruebas: {}", path);
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        auto hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        std::istringstream ss(line);
        ScriptStep step;
        if (!(ss >> step.op)) continue;
        if (step.op == "type" || step.op == "log") {
            std::string rest;
            std::getline(ss, rest);
            if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            step.args.push_back(rest);
        } else {
            std::string a;
            while (ss >> a) step.args.push_back(a);
        }
        m_script.push_back(step);
    }
    Log::info("Guion de pruebas cargado: {} pasos", m_script.size());
    return true;
}

void Application::pushKey(SDL_Keycode key, bool down) {
    SDL_Event ev{};
    ev.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    ev.key.key = key;
    ev.key.scancode = SDL_GetScancodeFromKey(key, nullptr);
    ev.key.down = down;
    ev.key.windowID = SDL_GetWindowID(m_window.getSDLWindow());
    SDL_PushEvent(&ev);
}

void Application::runScript() {
    for (auto it = m_scriptReleases.begin(); it != m_scriptReleases.end();) {
        if (m_time >= it->first) {
            pushKey(it->second, false);
            it = m_scriptReleases.erase(it);
        } else {
            ++it;
        }
    }
    auto coord = [&](const std::string& v, float extent) {
        if (!v.empty() && v.back() == '%') return std::stof(v.substr(0, v.size() - 1)) / 100.0f * extent;
        return std::stof(v);
    };
    while (!m_script.empty() && m_time >= m_scriptWaitUntil) {
        ScriptStep step = m_script.front();
        m_script.pop_front();
        const auto& a = step.args;
        try {
            if (step.op == "wait" && !a.empty()) {
                m_scriptWaitUntil = m_time + std::stod(a[0]);
                return;
            } else if (step.op == "waitready") {
                // Espera a que termine la carga del mundo
                if (m_game && !m_game->ready) {
                    m_script.push_front(step);
                    return;
                }
            } else if ((step.op == "key" || step.op == "down" || step.op == "up" || step.op == "hold") && !a.empty()) {
                SDL_Keycode k = keyFromName(a[0]);
                if (step.op != "up") pushKey(k, true);
                if (step.op == "key") m_scriptReleases.emplace_back(m_time + 0.05, k);
                if (step.op == "up") pushKey(k, false);
                if (step.op == "hold")
                    m_scriptReleases.emplace_back(m_time + (a.size() > 1 ? std::stod(a[1]) : 0.5), k);
            } else if ((step.op == "click" || step.op == "rclick" || step.op == "press" || step.op == "release") &&
                       a.size() >= 2) {
                float x = coord(a[0], static_cast<float>(m_window.getWidth()));
                float y = coord(a[1], static_cast<float>(m_window.getHeight()));
                Uint8 button =
                    (step.op == "rclick" || (a.size() > 2 && a[2] == "right")) ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
                SDL_WindowID wid = SDL_GetWindowID(m_window.getSDLWindow());
                SDL_Event mv{};
                mv.type = SDL_EVENT_MOUSE_MOTION;
                mv.motion.x = x;
                mv.motion.y = y;
                mv.motion.windowID = wid;
                SDL_PushEvent(&mv);
                if (step.op != "release") {
                    SDL_Event d{};
                    d.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                    d.button.button = button;
                    d.button.down = true;
                    d.button.clicks = 1;
                    d.button.x = x;
                    d.button.y = y;
                    d.button.windowID = wid;
                    SDL_PushEvent(&d);
                }
                if (step.op == "click" || step.op == "rclick" || step.op == "release") {
                    SDL_Event u{};
                    u.type = SDL_EVENT_MOUSE_BUTTON_UP;
                    u.button.button = button;
                    u.button.down = false;
                    u.button.x = x;
                    u.button.y = y;
                    u.button.windowID = wid;
                    SDL_PushEvent(&u);
                }
            } else if (step.op == "move" && a.size() >= 2) {
                SDL_Event mv{};
                mv.type = SDL_EVENT_MOUSE_MOTION;
                mv.motion.x = coord(a[0], static_cast<float>(m_window.getWidth()));
                mv.motion.y = coord(a[1], static_cast<float>(m_window.getHeight()));
                mv.motion.windowID = SDL_GetWindowID(m_window.getSDLWindow());
                SDL_PushEvent(&mv);
            } else if (step.op == "look" && a.size() >= 2) {
                m_camera.processMouseMovement(std::stof(a[0]), std::stof(a[1]));
            } else if (step.op == "type" && !a.empty()) {
                m_scriptTextStorage.push_back(a[0]);
                SDL_Event t{};
                t.type = SDL_EVENT_TEXT_INPUT;
                t.text.text = m_scriptTextStorage.back().c_str();
                t.text.windowID = SDL_GetWindowID(m_window.getSDLWindow());
                SDL_PushEvent(&t);
            } else if (step.op == "shot") {
                m_screenshotRequested = true;
                m_screenshotPath = a.empty() ? "" : a[0];
                m_scriptWaitUntil = m_time + 0.05;
                return;
            } else if (step.op == "log" && !a.empty()) {
                Log::info("[guion] {}", a[0]);
            } else if (step.op == "time" && !a.empty() && m_game) {
                m_game->session.getClock().setTickCount(std::stoull(a[0]));
            } else if (step.op == "summon" && a.size() >= 3 && m_game) {
                // Criatura delante del jugador para revisar modelos (guiones de QA)
                const auto& p = m_game->session.getPlayer();
                Math::Vec3 pos = p.position + Math::Vec3(std::stof(a[1]), 0.0f, std::stof(a[2]));
                pos.y = static_cast<float>(
                    m_game->session
                        .findSurfaceSpawn(static_cast<int>(std::floor(pos.x)), static_cast<int>(std::floor(pos.z)), 2)
                        .value_or(pos)
                        .y);
                static EntityId nextId = 900000;
                m_game->session.getMobs().emplace_back(nextId++, mobTypeFromString(a[0]), pos);
            } else if (step.op == "quit") {
                m_window.setShouldClose(true);
                return;
            } else {
                Log::warn("[guion] instrucción no reconocida: {}", step.op);
            }
        } catch (const std::exception& ex) {
            Log::warn("[guion] argumento no válido en '{}': {}", step.op, ex.what());
        }
    }
}

} // namespace OpenCraft

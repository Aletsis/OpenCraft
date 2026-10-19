#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "ui/UIBatch.hpp"
#include "render/TextureAtlas.hpp"
#include "storage/WorldManager.hpp"
#include "platform/DeviceSettings.hpp"
#include "network/LANDiscovery.hpp"

namespace OpenCraft {

enum class MenuScreen {
    None, // en juego, sin menú
    Title,
    SelectWorld,
    CreateWorld,
    ConfirmDelete,
    Options,
    Pause,
    Death,
    Loading,
    Message,
    Multiplayer
};

// Entrada de un fotograma para la interfaz de menús
struct MenuInput {
    float mouseX = -1.0f;
    float mouseY = -1.0f;
    bool clicked = false; // botón primario pulsado en este fotograma
    bool rightClicked = false;
    bool doubleClicked = false;
    float wheel = 0.0f;
    std::string text;  // texto UTF-8 tecleado en este fotograma
    int backspace = 0; // pulsaciones de retroceso en este fotograma
    bool enter = false;
    bool escape = false;
    bool tab = false;
    bool up = false;
    bool down = false;
};

struct MenuEvent {
    enum class Type {
        Quit,
        PlayWorld,
        CreateWorld,
        DeleteWorld,
        Resume,
        SaveAndQuit,
        Respawn,
        SettingsChanged,
        OpenToLan,
        JoinServer,
        RefreshServers,
        ReturnToTitle
    };
    Type type = Type::Quit;
    std::string worldId;
    std::string name;
    std::string seedText;
    uint32_t gameMode = 0;
    bool hardcore = false;
    std::string host;
    uint16_t port = 0;
};

// Datos que la aplicación expone a los menús en cada fotograma
struct MenuContext {
    const std::vector<WorldSummary>* worlds = nullptr;
    DeviceSettings* settings = nullptr;
    const std::vector<Net::DiscoveredServer>* servers = nullptr;
    bool inGame = false;
    bool lanOpen = false;
    uint16_t lanPort = 0;
    float loadingProgress = 0.0f;
    std::string loadingMessage;
    bool hardcoreDeath = false;
    uint32_t deathScore = 0;
    std::string deathMessage;
    int guiScale = 2;
    double timeSeconds = 0.0;
};

class MenuSystem {
public:
    MenuScreen current() const noexcept { return m_screen; }
    void open(MenuScreen screen);
    void showMessage(const std::string& title, const std::string& message, MenuScreen returnTo);
    bool isBlockingGame() const noexcept { return m_screen != MenuScreen::None; }
    bool wantsTextInput() const noexcept { return m_focusedField >= 0; }
    bool consumeClickSound() noexcept {
        bool c = m_clickSound;
        m_clickSound = false;
        return c;
    }

    // Dibuja la pantalla actual y devuelve las acciones elegidas por el jugador
    std::vector<MenuEvent> frame(UIBatch& ui, const MenuInput& in, const MenuContext& ctx);

private:
    MenuScreen m_screen = MenuScreen::Title;
    MenuScreen m_optionsReturn = MenuScreen::Title;
    MenuScreen m_messageReturn = MenuScreen::Title;
    std::string m_messageTitle;
    std::string m_messageBody;

    // Selección de mundos
    int m_selectedWorld = -1;
    float m_listScroll = 0.0f;

    // Creación de mundo
    std::string m_newWorldName = "Nuevo mundo";
    std::string m_newWorldSeed;
    int m_newWorldMode = 0; // 0 supervivencia, 1 creativo, 2 extremo
    int m_focusedField = -1;
    std::string m_pendingDeleteId;
    std::string m_pendingDeleteName;

    // Multijugador
    std::string m_directAddress;
    int m_selectedServer = -1;

    // Estado de widgets por fotograma
    const MenuInput* m_in = nullptr;
    UIBatch* m_ui = nullptr;
    float m_s = 2.0f;
    std::vector<MenuEvent> m_events;
    double m_time = 0.0;
    bool m_clickSound = false;

    bool button(const std::string& label, float x, float y, float w, float h = 20.0f, bool enabled = true);
    // Igual que button() pero informa el sentido del clic (+1 primario, -1 secundario)
    int cycleButton(const std::string& label, float x, float y, float w, bool enabled = true);
    void textField(int id, std::string& value, float x, float y, float w, const std::string& placeholder,
                   size_t maxLen);
    void drawBackground(const MenuContext& ctx);
    void title(const std::string& text, float y);

    void titleScreen(const MenuContext& ctx);
    void selectWorldScreen(const MenuContext& ctx);
    void createWorldScreen(const MenuContext& ctx);
    void confirmDeleteScreen(const MenuContext& ctx);
    void optionsScreen(const MenuContext& ctx);
    void pauseScreen(const MenuContext& ctx);
    void deathScreen(const MenuContext& ctx);
    void loadingScreen(const MenuContext& ctx);
    void messageScreen(const MenuContext& ctx);
    void multiplayerScreen(const MenuContext& ctx);

    float W() const;
    float H() const;
};

} // namespace OpenCraft

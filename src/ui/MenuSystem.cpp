#include "ui/MenuSystem.hpp"

// Los eventos se construyen con inicializadores designados: los campos omitidos toman su valor por
// defecto a propósito
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "ui/BitmapFont.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <format>

namespace OpenCraft {

namespace {

constexpr UIColor BTN_BG{0.44f, 0.44f, 0.44f, 1.0f};
constexpr UIColor BTN_HOVER{0.49f, 0.53f, 0.75f, 1.0f};
constexpr UIColor BTN_DISABLED{0.2f, 0.2f, 0.2f, 1.0f};
constexpr UIColor TEXT_HOVER{1.0f, 1.0f, 0.63f, 1.0f};
constexpr UIColor TEXT_DISABLED{0.63f, 0.63f, 0.63f, 1.0f};
constexpr UIColor TEXT_GRAY{0.63f, 0.63f, 0.63f, 1.0f};

const char* gameModeName(uint32_t mode, bool hardcore) {
    if (hardcore) return "Extremo";
    switch (mode) {
        case 1: return "Creativo";
        case 2: return "Aventura";
        case 3: return "Espectador";
        default: return "Supervivencia";
    }
}

std::string formatDate(int64_t epoch) {
    if (epoch <= 0) return "nunca";
    std::time_t t = static_cast<std::time_t>(epoch);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M", &tm);
    return buf;
}

std::string formatSize(uint64_t bytes) {
    if (bytes >= 1024ull * 1024ull) return std::format("{:.1f} MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    return std::format("{} KB", std::max<uint64_t>(1, bytes / 1024));
}

// Elimina el último carácter UTF-8 completo
void popUtf8(std::string& s) {
    if (s.empty()) return;
    size_t i = s.size() - 1;
    while (i > 0 && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) --i;
    s.erase(i);
}

size_t utf8Length(const std::string& s) { return BitmapFont::decodeUtf8(s).size(); }

} // namespace

float MenuSystem::W() const { return static_cast<float>(m_ui->width()) / m_s; }
float MenuSystem::H() const { return static_cast<float>(m_ui->height()) / m_s; }

void MenuSystem::open(MenuScreen screen) {
    if (screen == MenuScreen::Options) {
        m_optionsReturn = (m_screen == MenuScreen::Options) ? m_optionsReturn : m_screen;
    }
    if (screen == MenuScreen::CreateWorld) {
        m_newWorldName = "Nuevo mundo";
        m_newWorldSeed.clear();
        m_newWorldMode = 0;
        m_focusedField = 0;
    } else {
        m_focusedField = -1;
    }
    m_screen = screen;
}

void MenuSystem::showMessage(const std::string& title, const std::string& message, MenuScreen returnTo) {
    m_messageTitle = title;
    m_messageBody = message;
    m_messageReturn = returnTo;
    m_screen = MenuScreen::Message;
    m_focusedField = -1;
}

std::vector<MenuEvent> MenuSystem::frame(UIBatch& ui, const MenuInput& in, const MenuContext& ctx) {
    m_events.clear();
    if (m_screen == MenuScreen::None) return m_events;
    m_ui = &ui;
    m_in = &in;
    m_s = static_cast<float>(std::max(1, ctx.guiScale));
    m_time = ctx.timeSeconds;

    drawBackground(ctx);
    switch (m_screen) {
        case MenuScreen::Title: titleScreen(ctx); break;
        case MenuScreen::SelectWorld: selectWorldScreen(ctx); break;
        case MenuScreen::CreateWorld: createWorldScreen(ctx); break;
        case MenuScreen::ConfirmDelete: confirmDeleteScreen(ctx); break;
        case MenuScreen::Options: optionsScreen(ctx); break;
        case MenuScreen::Pause: pauseScreen(ctx); break;
        case MenuScreen::Death: deathScreen(ctx); break;
        case MenuScreen::Loading: loadingScreen(ctx); break;
        case MenuScreen::Message: messageScreen(ctx); break;
        case MenuScreen::Multiplayer: multiplayerScreen(ctx); break;
        case MenuScreen::None: break;
    }
    return m_events;
}

// ---------------------------------------------------------------------------------------------
// Widgets
// ---------------------------------------------------------------------------------------------

bool MenuSystem::button(const std::string& label, float x, float y, float w, float h, bool enabled) {
    return cycleButton(label, x, y, w, enabled) == 1 && h > 0.0f;
}

int MenuSystem::cycleButton(const std::string& label, float x, float y, float w, bool enabled) {
    const float s = m_s, h = 20.0f;
    float px = std::round(x * s), py = std::round(y * s), pw = w * s, ph = h * s;
    bool hover =
        enabled && m_in->mouseX >= px && m_in->mouseX < px + pw && m_in->mouseY >= py && m_in->mouseY < py + ph;
    UIColor bg = !enabled ? BTN_DISABLED : (hover ? BTN_HOVER : BTN_BG);
    m_ui->rect(px, py, pw, ph, UIColors::Black);
    m_ui->rect(px + s, py + s, pw - 2 * s, ph - 2 * s, bg);
    if (enabled) {
        m_ui->rect(px + s, py + s, pw - 2 * s, s, UIColor{1, 1, 1, 0.35f});
        m_ui->rect(px + s, py + ph - 3 * s, pw - 2 * s, 2 * s, UIColor{0, 0, 0, 0.3f});
    }
    if (hover) m_ui->outline(px, py, pw, ph, s, UIColors::White);
    UIColor tc = !enabled ? TEXT_DISABLED : (hover ? TEXT_HOVER : UIColors::White);
    m_ui->textCentered(label, px + pw * 0.5f, py + 6 * s, s, tc);
    if (!hover) return 0;
    if (m_in->clicked || m_in->rightClicked) m_clickSound = true;
    if (m_in->clicked) return 1;
    if (m_in->rightClicked) return -1;
    return 0;
}

void MenuSystem::textField(int id, std::string& value, float x, float y, float w, const std::string& placeholder,
                           size_t maxLen) {
    const float s = m_s, h = 20.0f;
    float px = std::round(x * s), py = std::round(y * s), pw = w * s, ph = h * s;
    bool hover = m_in->mouseX >= px && m_in->mouseX < px + pw && m_in->mouseY >= py && m_in->mouseY < py + ph;
    if (m_in->clicked) {
        if (hover)
            m_focusedField = id;
        else if (m_focusedField == id)
            m_focusedField = -1;
    }
    bool focused = m_focusedField == id;
    if (focused) {
        // El retroceso se aplica antes que el texto nuevo del mismo fotograma
        for (int i = 0; i < m_in->backspace; ++i) popUtf8(value);
        if (!m_in->text.empty()) {
            for (char32_t cp : BitmapFont::decodeUtf8(m_in->text)) {
                if (utf8Length(value) >= maxLen) break;
                if (cp < 0x20) continue;
                // Reinsertar el punto de código como UTF-8
                if (cp < 0x80)
                    value.push_back(static_cast<char>(cp));
                else if (cp < 0x800) {
                    value.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                    value.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                } else {
                    value.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                    value.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                    value.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                }
            }
        }
    }
    m_ui->rect(px, py, pw, ph, focused ? UIColors::White : TEXT_GRAY);
    m_ui->rect(px + s, py + s, pw - 2 * s, ph - 2 * s, UIColors::Black);
    float tx = px + 4 * s, ty = py + 6 * s;
    if (value.empty() && !focused) {
        m_ui->text(placeholder, tx, ty, s, UIColor{0.45f, 0.45f, 0.45f, 1.0f}, false);
    } else {
        // Desplazar hacia la izquierda si el texto no cabe
        std::string shown = value;
        while (!shown.empty() && UIBatch::textWidth(shown, s) > pw - 12 * s) {
            auto cps = BitmapFont::decodeUtf8(shown);
            size_t first = 1;
            size_t bytes = 0;
            for (size_t i = 0; i < first && i < cps.size(); ++i) {
                char32_t c = cps[i];
                bytes += c < 0x80 ? 1 : (c < 0x800 ? 2 : (c < 0x10000 ? 3 : 4));
            }
            shown.erase(0, bytes);
        }
        m_ui->text(shown, tx, ty, s, UIColor{0.88f, 0.88f, 0.88f, 1.0f});
        if (focused && std::fmod(m_time, 1.0) < 0.5) {
            m_ui->text("_", tx + UIBatch::textWidth(shown, s) + s, ty, s, UIColor{0.88f, 0.88f, 0.88f, 1.0f});
        }
    }
}

void MenuSystem::drawBackground(const MenuContext& ctx) {
    const float W = static_cast<float>(m_ui->width()), H = static_cast<float>(m_ui->height());
    bool overWorld = ctx.inGame && (m_screen == MenuScreen::Pause || m_screen == MenuScreen::Death ||
                                    m_screen == MenuScreen::Options || m_screen == MenuScreen::Message);
    if (m_screen == MenuScreen::Death) {
        m_ui->rect(0, 0, W, H, UIColor{0.45f, 0.0f, 0.0f, 0.55f});
        return;
    }
    if (overWorld) {
        m_ui->rect(0, 0, W, H, UIColor{0.06f, 0.06f, 0.06f, 0.6f});
        return;
    }
    // Fondo clásico de tierra oscurecida
    float tile = 32.0f * m_s;
    for (float y = 0; y < H; y += tile) {
        for (float x = 0; x < W; x += tile) {
            m_ui->atlasTile(x, y, tile, 2, UIColor{0.25f, 0.25f, 0.25f, 1.0f});
        }
    }
}

void MenuSystem::title(const std::string& text, float y) {
    m_ui->textCentered(text, W() * 0.5f * m_s, y * m_s, m_s, UIColors::White);
}

// ---------------------------------------------------------------------------------------------
// Pantallas
// ---------------------------------------------------------------------------------------------

void MenuSystem::titleScreen(const MenuContext& ctx) {
    const float s = m_s;
    float cx = W() * 0.5f;
    float top = H() * 0.25f;

    // Logotipo
    float logoScale = std::max(2.0f, std::floor(s * 3.0f));
    std::string logo = "OpenCraft";
    float lw = UIBatch::textWidth(logo, logoScale);
    float lx = std::round(cx * s - lw * 0.5f), ly = std::round((top - 30) * s);
    m_ui->text(logo, lx + logoScale, ly + logoScale, logoScale, UIColor{0.15f, 0.15f, 0.15f, 1.0f}, false);
    m_ui->text(logo, lx, ly, logoScale, UIColor{0.85f, 0.85f, 0.85f, 1.0f}, false);
    // Frase amarilla pulsante
    float pulse = 1.0f + 0.06f * static_cast<float>(std::sin(ctx.timeSeconds * 6.0));
    float ss = std::max(1.0f, std::round(s * pulse));
    m_ui->text("¡Ahora en tu móvil!", lx + lw - UIBatch::textWidth("¡Ahora en tu móvil!", ss) * 0.6f,
               ly + 8 * logoScale + 2 * s, ss, UIColors::Yellow);

    float y = top + 18;
    if (button("Un jugador", cx - 100, y, 200)) open(MenuScreen::SelectWorld);
    // El cliente multijugador aún no sincroniza el mundo del anfitrión (limitación conocida de la RC)
    button("Multijugador (próximamente)", cx - 100, y + 24, 200, 20, false);
    if (button("Opciones...", cx - 100, y + 60, 98)) open(MenuScreen::Options);
    if (button("Salir del juego", cx + 2, y + 60, 98)) m_events.push_back({.type = MenuEvent::Type::Quit});

    m_ui->text("OpenCraft 1.0.0-rc1", 2 * s, (H() - 10) * s, s, UIColors::White);
    std::string right = "C++20 - SDL3 - OpenGL ES 3";
    m_ui->text(right, W() * s - UIBatch::textWidth(right, s) - 2 * s, (H() - 10) * s, s, UIColors::White);
    if (m_in->escape) m_events.push_back({.type = MenuEvent::Type::Quit});
}

void MenuSystem::selectWorldScreen(const MenuContext& ctx) {
    const float s = m_s;
    float cx = W() * 0.5f;
    title("Seleccionar mundo", 12);
    const auto& worlds = *ctx.worlds;
    if (m_selectedWorld >= static_cast<int>(worlds.size())) m_selectedWorld = -1;

    float listTop = 32, listBottom = H() - 64, entryH = 36, listW = 270;
    float listX = cx - listW * 0.5f;
    m_ui->rect(0, listTop * s, W() * s, (listBottom - listTop) * s, UIColor{0, 0, 0, 0.45f});

    float contentH = static_cast<float>(worlds.size()) * entryH;
    float maxScroll = std::max(0.0f, contentH - (listBottom - listTop - 4));
    m_listScroll = std::clamp(m_listScroll - m_in->wheel * entryH, 0.0f, maxScroll);

    if (worlds.empty()) {
        m_ui->textCentered("No hay mundos guardados todavía.", cx * s, (listTop + 20) * s, s, TEXT_GRAY);
        m_ui->textCentered("Crea uno nuevo para empezar a jugar.", cx * s, (listTop + 32) * s, s, TEXT_GRAY);
    }
    for (size_t i = 0; i < worlds.size(); ++i) {
        float y = listTop + 4 + static_cast<float>(i) * entryH - m_listScroll;
        if (y < listTop || y + entryH - 4 > listBottom) continue;
        const auto& w = worlds[i];
        float px = listX * s, py = y * s, pw = listW * s, ph = (entryH - 4) * s;
        bool hover = m_in->mouseX >= px && m_in->mouseX < px + pw && m_in->mouseY >= py && m_in->mouseY < py + ph;
        bool selected = m_selectedWorld == static_cast<int>(i);
        if (selected) {
            m_ui->rect(px - s, py - s, pw + 2 * s, ph + 2 * s, TEXT_GRAY);
            m_ui->rect(px, py, pw, ph, UIColors::Black);
        } else if (hover) {
            m_ui->rect(px, py, pw, ph, UIColor{1, 1, 1, 0.08f});
        }
        // Miniatura: bloque de hierba
        m_ui->atlasTile(px + 2 * s, py + 2 * s, 28 * s, w.hardcore ? 22 : 0);
        float tx = px + 34 * s;
        m_ui->text(w.name, tx, py + 2 * s, s, UIColors::White);
        m_ui->text(std::format("{} ({})", w.id, formatDate(w.lastPlayed)), tx, py + 12 * s, s, TEXT_GRAY, false);
        std::string info = std::format("Modo {}, {}", gameModeName(w.gameMode, w.hardcore), formatSize(w.sizeOnDisk));
        if (w.locked) info += " - en uso";
        m_ui->text(info, tx, py + 22 * s, s, w.hardcore ? UIColors::Red : TEXT_GRAY, false);
        if (hover && m_in->clicked) {
            if (selected && m_in->doubleClicked && !w.locked) {
                m_events.push_back({.type = MenuEvent::Type::PlayWorld, .worldId = w.id});
            }
            m_selectedWorld = static_cast<int>(i);
        }
    }
    if (m_in->up && !worlds.empty()) m_selectedWorld = std::max(0, m_selectedWorld - 1);
    if (m_in->down && !worlds.empty())
        m_selectedWorld = std::min(static_cast<int>(worlds.size()) - 1, m_selectedWorld + 1);

    bool hasSel = m_selectedWorld >= 0 && m_selectedWorld < static_cast<int>(worlds.size());
    bool canPlay = hasSel && !worlds[static_cast<size_t>(m_selectedWorld)].locked;
    float by = H() - 52;
    if (button("Jugar mundo seleccionado", cx - 154, by, 150, 20, canPlay) || (m_in->enter && canPlay)) {
        m_events.push_back(
            {.type = MenuEvent::Type::PlayWorld, .worldId = worlds[static_cast<size_t>(m_selectedWorld)].id});
    }
    if (button("Crear mundo nuevo", cx + 4, by, 150)) open(MenuScreen::CreateWorld);
    if (button("Eliminar", cx - 154, by + 24, 72, 20, canPlay)) {
        m_pendingDeleteId = worlds[static_cast<size_t>(m_selectedWorld)].id;
        m_pendingDeleteName = worlds[static_cast<size_t>(m_selectedWorld)].name;
        open(MenuScreen::ConfirmDelete);
    }
    if (button("Cancelar", cx + 82, by + 24, 72) || m_in->escape) open(MenuScreen::Title);
}

void MenuSystem::createWorldScreen(const MenuContext& ctx) {
    (void)ctx;
    float cx = W() * 0.5f;
    title("Crear mundo nuevo", 16);
    const float s = m_s;

    float y = 44;
    m_ui->text("Nombre del mundo", (cx - 100) * s, y * s, s, TEXT_GRAY);
    textField(0, m_newWorldName, cx - 100, y + 10, 200, "Nuevo mundo", 32);

    static const char* MODES[] = {"Supervivencia", "Creativo", "Extremo"};
    static const char* DESCRIPTIONS[][2] = {
        {"Busca recursos, fabrica herramientas,", "gana niveles, salud y hambre"},
        {"Recursos ilimitados, vuelo libre", "y destrucción instantánea de bloques"},
        {"Igual que supervivencia, con la máxima", "dificultad y una sola vida"},
    };
    int dir = cycleButton(std::format("Modo de juego: {}", MODES[m_newWorldMode]), cx - 100, y + 40, 200);
    if (dir != 0) m_newWorldMode = (m_newWorldMode + dir + 3) % 3;
    m_ui->textCentered(DESCRIPTIONS[m_newWorldMode][0], cx * s, (y + 64) * s, s, TEXT_GRAY);
    m_ui->textCentered(DESCRIPTIONS[m_newWorldMode][1], cx * s, (y + 74) * s, s, TEXT_GRAY);

    m_ui->text("Semilla para el generador de mundos", (cx - 100) * s, (y + 92) * s, s, TEXT_GRAY);
    textField(1, m_newWorldSeed, cx - 100, y + 102, 200, "Vacío = semilla aleatoria", 32);

    if (m_in->tab) m_focusedField = (m_focusedField + 1) % 2;

    std::string trimmed = m_newWorldName;
    trimmed.erase(0, trimmed.find_first_not_of(' '));
    bool valid = !trimmed.empty();
    float by = H() - 28;
    if (button("Crear mundo nuevo", cx - 154, by, 150, 20, valid) || (m_in->enter && valid)) {
        m_events.push_back({.type = MenuEvent::Type::CreateWorld,
                            .name = trimmed,
                            .seedText = m_newWorldSeed,
                            .gameMode = m_newWorldMode == 1 ? 1u : 0u,
                            .hardcore = m_newWorldMode == 2});
        m_focusedField = -1;
    }
    if (button("Cancelar", cx + 4, by, 150) || m_in->escape) open(MenuScreen::SelectWorld);
}

void MenuSystem::confirmDeleteScreen(const MenuContext& ctx) {
    (void)ctx;
    float cx = W() * 0.5f;
    const float s = m_s;
    title("¿Seguro que quieres eliminar este mundo?", H() * 0.3f);
    m_ui->textCentered(std::format("«{}» se perderá para siempre.", m_pendingDeleteName), cx * s, (H() * 0.3f + 16) * s,
                       s, TEXT_GRAY);
    m_ui->textCentered("Antes se guardará una copia de seguridad.", cx * s, (H() * 0.3f + 28) * s, s, TEXT_GRAY);
    float by = H() * 0.3f + 52;
    if (button("Eliminar", cx - 154, by, 150)) {
        m_events.push_back({.type = MenuEvent::Type::DeleteWorld, .worldId = m_pendingDeleteId});
        m_selectedWorld = -1;
        open(MenuScreen::SelectWorld);
    }
    if (button("Cancelar", cx + 4, by, 150) || m_in->escape) open(MenuScreen::SelectWorld);
}

void MenuSystem::optionsScreen(const MenuContext& ctx) {
    float cx = W() * 0.5f;
    title("Opciones", 14);
    DeviceSettings& st = *ctx.settings;
    bool changed = false;
    auto onOff = [](bool v) { return v ? "Sí" : "No"; };

    float y = 36, lx = cx - 155, rx = cx + 5;
    auto cycleInt = [&](const std::string& label, float x, float yy, int& value, int minV, int maxV, int step) {
        int d = cycleButton(label, x, yy, 150);
        if (d != 0) {
            value += d * step;
            if (value > maxV) value = minV;
            if (value < minV) value = maxV;
            changed = true;
        }
    };
    auto toggle = [&](const std::string& label, float x, float yy, bool& value) {
        if (cycleButton(label, x, yy, 150) != 0) {
            value = !value;
            changed = true;
        }
    };

    cycleInt(std::format("Distancia: {} chunks", st.renderDistance), lx, y, st.renderDistance, 2, 16, 1);
    cycleInt(std::format("Campo de visión: {}", st.fov), rx, y, st.fov, 50, 110, 5);
    std::string bright =
        st.brightness <= 0 ? "Sombrío" : (st.brightness >= 100 ? "¡Brillante!" : std::format("{}%", st.brightness));
    cycleInt("Brillo: " + bright, lx, y + 24, st.brightness, 0, 100, 25);
    int sens = static_cast<int>(std::round(st.mouseSensitivity / 0.15f * 100.0f));
    int sensNew = sens;
    cycleInt(std::format("Sensibilidad: {}%", sens), rx, y + 24, sensNew, 25, 300, 25);
    if (sensNew != sens) st.mouseSensitivity = 0.15f * static_cast<float>(sensNew) / 100.0f;
    std::string gui = st.uiScale == 0 ? "Auto" : std::to_string(st.uiScale);
    cycleInt("Escala de interfaz: " + gui, lx, y + 48, st.uiScale, 0, 4, 1);
    toggle(std::format("Invertir ratón: {}", onOff(st.invertY)), rx, y + 48, st.invertY);
    toggle(std::format("Controles táctiles: {}", onOff(st.touchControlsEnabled)), lx, y + 72, st.touchControlsEnabled);
    toggle(std::format("Salto automático: {}", onOff(st.autoJump)), rx, y + 72, st.autoJump);
    toggle(std::format("Subtítulos: {}", onOff(st.subtitlesEnabled)), lx, y + 96, st.subtitlesEnabled);
    toggle(std::format("Alto contraste: {}", onOff(st.highContrast)), rx, y + 96, st.highContrast);
    toggle(std::format("Sincronía vertical: {}", onOff(st.vsync)), lx, y + 120, st.vsync);
    cycleInt(std::format("Volumen general: {}%", st.masterVolume), rx, y + 120, st.masterVolume, 0, 100, 10);

    if (changed) m_events.push_back({.type = MenuEvent::Type::SettingsChanged});
    if (button("Listo", cx - 100, H() - 28, 200) || m_in->escape) {
        m_events.push_back({.type = MenuEvent::Type::SettingsChanged});
        m_screen = m_optionsReturn;
    }
}

void MenuSystem::pauseScreen(const MenuContext& ctx) {
    float cx = W() * 0.5f;
    const float s = m_s;
    title("Menú del juego", 40);
    float y = H() * 0.25f + 8;
    if (button("Volver al juego", cx - 100, y, 200) || m_in->escape) {
        m_events.push_back({.type = MenuEvent::Type::Resume});
    }
    if (button("Opciones...", cx - 100, y + 24, 98)) open(MenuScreen::Options);
    (void)ctx.lanOpen;
    button("LAN (pronto)", cx + 2, y + 24, 98, 20, false);
    if (button("Guardar y salir al título", cx - 100, y + 60, 200)) {
        m_events.push_back({.type = MenuEvent::Type::SaveAndQuit});
    }
    m_ui->textCentered("Pulsa F2 para guardar una captura, F3 para depuración", cx * s, (H() - 14) * s, s, TEXT_GRAY);
}

void MenuSystem::deathScreen(const MenuContext& ctx) {
    float cx = W() * 0.5f;
    const float s = m_s;
    float big = s * 2;
    std::string t = ctx.hardcoreDeath ? "¡Fin de la partida!" : "¡Has muerto!";
    m_ui->textCentered(t, cx * s, H() * 0.25f * s, big, UIColors::White);
    if (!ctx.deathMessage.empty()) {
        m_ui->textCentered(ctx.deathMessage, cx * s, (H() * 0.25f + 24) * s, s, UIColors::White);
    }
    m_ui->textCentered(std::format("Puntuación: {}", ctx.deathScore), cx * s, (H() * 0.25f + 36) * s, s,
                       UIColors::Yellow);
    float y = H() * 0.25f + 64;
    if (ctx.hardcoreDeath) {
        if (button("Observar el mundo", cx - 100, y, 200)) m_events.push_back({.type = MenuEvent::Type::Respawn});
    } else if (button("Reaparecer", cx - 100, y, 200) || m_in->enter) {
        m_events.push_back({.type = MenuEvent::Type::Respawn});
    }
    if (button("Pantalla de título", cx - 100, y + 24, 200)) {
        m_events.push_back({.type = MenuEvent::Type::SaveAndQuit});
    }
}

void MenuSystem::loadingScreen(const MenuContext& ctx) {
    float cx = W() * 0.5f;
    const float s = m_s;
    title("Cargando mundo", H() * 0.4f);
    m_ui->textCentered(ctx.loadingMessage, cx * s, (H() * 0.4f + 14) * s, s, TEXT_GRAY);
    float bw = 200, bx = cx - bw * 0.5f, by = H() * 0.4f + 30;
    m_ui->rect(bx * s, by * s, bw * s, 4 * s, UIColor{0.3f, 0.3f, 0.3f, 1});
    m_ui->rect(bx * s, by * s, bw * std::clamp(ctx.loadingProgress, 0.0f, 1.0f) * s, 4 * s,
               UIColor{0.5f, 1.0f, 0.4f, 1});
}

void MenuSystem::messageScreen(const MenuContext& ctx) {
    (void)ctx;
    float cx = W() * 0.5f;
    const float s = m_s;
    title(m_messageTitle, H() * 0.3f);
    // Ajuste de línea simple por palabras
    std::vector<std::string> lines;
    std::string line, word;
    float maxW = std::min(W() - 20, 300.0f) * s;
    auto flushWord = [&]() {
        std::string cand = line.empty() ? word : line + " " + word;
        if (UIBatch::textWidth(cand, s) > maxW && !line.empty()) {
            lines.push_back(line);
            line = word;
        } else {
            line = cand;
        }
        word.clear();
    };
    for (char c : m_messageBody) {
        if (c == ' ')
            flushWord();
        else if (c == '\n') {
            flushWord();
            lines.push_back(line);
            line.clear();
        } else
            word.push_back(c);
    }
    flushWord();
    if (!line.empty()) lines.push_back(line);
    float y = H() * 0.3f + 18;
    for (const auto& l : lines) {
        m_ui->textCentered(l, cx * s, y * s, s, TEXT_GRAY);
        y += 11;
    }
    if (button("Volver", cx - 100, y + 14, 200) || m_in->escape || m_in->enter) {
        m_screen = m_messageReturn;
    }
}

void MenuSystem::multiplayerScreen(const MenuContext& ctx) {
    const float s = m_s;
    float cx = W() * 0.5f;
    title("Jugar en red local", 12);
    const auto& servers = *ctx.servers;
    float listTop = 32, listBottom = H() - 90, listW = 270, entryH = 26;
    m_ui->rect(0, listTop * s, W() * s, (listBottom - listTop) * s, UIColor{0, 0, 0, 0.45f});
    if (servers.empty()) {
        m_ui->textCentered("Buscando partidas en tu red local...", cx * s, (listTop + 20) * s, s, TEXT_GRAY);
        m_ui->textCentered("En el otro equipo: Menú del juego > Abrir en LAN", cx * s, (listTop + 32) * s, s,
                           TEXT_GRAY);
    }
    if (m_selectedServer >= static_cast<int>(servers.size())) m_selectedServer = -1;
    for (size_t i = 0; i < servers.size(); ++i) {
        float y = listTop + 4 + static_cast<float>(i) * entryH;
        if (y + entryH > listBottom) break;
        float px = (cx - listW * 0.5f) * s, py = y * s, pw = listW * s, ph = (entryH - 4) * s;
        bool hover = m_in->mouseX >= px && m_in->mouseX < px + pw && m_in->mouseY >= py && m_in->mouseY < py + ph;
        if (m_selectedServer == static_cast<int>(i)) {
            m_ui->rect(px - s, py - s, pw + 2 * s, ph + 2 * s, TEXT_GRAY);
            m_ui->rect(px, py, pw, ph, UIColors::Black);
        }
        m_ui->text(servers[i].serverName, px + 4 * s, py + 2 * s, s, UIColors::White);
        m_ui->text(std::format("{}:{}", servers[i].ip, servers[i].port), px + 4 * s, py + 12 * s, s, TEXT_GRAY, false);
        if (hover && m_in->clicked) {
            if (m_selectedServer == static_cast<int>(i) && m_in->doubleClicked) {
                m_events.push_back(
                    {.type = MenuEvent::Type::JoinServer, .host = servers[i].ip, .port = servers[i].port});
            }
            m_selectedServer = static_cast<int>(i);
        }
    }

    m_ui->text("Conexión directa (IP:puerto)", (cx - 154) * s, (H() - 84) * s, s, TEXT_GRAY);
    textField(10, m_directAddress, cx - 154, H() - 74, 230, "192.168.1.20:35565", 64);
    bool directValid = !m_directAddress.empty();
    if (button("Conectar", cx + 80, H() - 74, 74, 20, directValid) ||
        (m_in->enter && directValid && m_focusedField == 10)) {
        std::string host = m_directAddress;
        uint16_t port = 35565;
        if (auto colon = host.rfind(':'); colon != std::string::npos) {
            try {
                port = static_cast<uint16_t>(std::stoi(host.substr(colon + 1)));
            } catch (...) {
                port = 35565;
            }
            host = host.substr(0, colon);
        }
        m_events.push_back({.type = MenuEvent::Type::JoinServer, .host = host, .port = port});
    }
    bool hasSel = m_selectedServer >= 0;
    float by = H() - 48;
    if (button("Unirse a la partida", cx - 154, by, 150, 20, hasSel)) {
        const auto& sv = servers[static_cast<size_t>(m_selectedServer)];
        m_events.push_back({.type = MenuEvent::Type::JoinServer, .host = sv.ip, .port = sv.port});
    }
    if (button("Actualizar", cx + 4, by, 150)) m_events.push_back({.type = MenuEvent::Type::RefreshServers});
    if (button("Cancelar", cx - 100, by + 24, 200) || m_in->escape) open(MenuScreen::Title);
}

} // namespace OpenCraft

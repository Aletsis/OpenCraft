#include "ui/LocalizationManager.hpp"
#include <algorithm>

namespace OpenCraft {

LocalizationManager& LocalizationManager::global() {
    static LocalizationManager instance;
    return instance;
}

LocalizationManager::LocalizationManager() { initDictionary(); }

void LocalizationManager::setLanguage(const std::string& langCode) {
    if (m_tables.contains(langCode)) {
        m_currentLanguage = langCode;
    } else {
        m_currentLanguage = "en_US";
    }
}

std::string LocalizationManager::get(const std::string& key) const {
    auto langIt = m_tables.find(m_currentLanguage);
    if (langIt != m_tables.end()) {
        auto keyIt = langIt->second.find(key);
        if (keyIt != langIt->second.end()) {
            return keyIt->second;
        }
    }

    // Fallback a inglés
    auto fallbackIt = m_tables.find("en_US");
    if (fallbackIt != m_tables.end()) {
        auto keyIt = fallbackIt->second.find(key);
        if (keyIt != fallbackIt->second.end()) {
            return keyIt->second;
        }
    }

    return key;
}

bool LocalizationManager::has(const std::string& key) const {
    auto langIt = m_tables.find(m_currentLanguage);
    if (langIt != m_tables.end()) {
        return langIt->second.contains(key);
    }
    return false;
}

int LocalizationManager::calculateAutoUIScale(int screenWidth, int screenHeight) {
    int minDim = std::min(screenWidth, screenHeight);
    if (minDim >= 1440) return 4;
    if (minDim >= 900) return 3;
    if (minDim >= 540) return 2;
    return 1;
}

float LocalizationManager::getUIScaleFactor(int uiScaleSetting, int screenWidth, int screenHeight) {
    int scale =
        (uiScaleSetting <= 0) ? calculateAutoUIScale(screenWidth, screenHeight) : std::clamp(uiScaleSetting, 1, 4);
    return static_cast<float>(scale);
}

void LocalizationManager::initDictionary() {
    // --- Español (es_ES) ---
    auto& es = m_tables["es_ES"];
    es["menu.play"] = "Jugar";
    es["menu.worlds"] = "Seleccionar Mundo";
    es["menu.create_world"] = "Crear Nuevo Mundo";
    es["menu.multiplayer"] = "Multijugador LAN";
    es["menu.options"] = "Opciones";
    es["menu.quit"] = "Salir del Juego";
    es["menu.resume"] = "Volver al Juego";
    es["menu.save_quit"] = "Guardar y Salir";
    es["menu.back"] = "Volver";
    es["menu.done"] = "Hecho";

    es["options.title"] = "Configuración";
    es["options.video"] = "Gráficos y Rendimiento";
    es["options.audio"] = "Sonido y Subtítulos";
    es["options.controls"] = "Controles y Entrada";
    es["options.accessibility"] = "Accesibilidad";
    es["options.language"] = "Idioma";
    es["options.render_distance"] = "Distancia de Renderizado";
    es["options.simulation_distance"] = "Distancia de Simulación";
    es["options.vsync"] = "Sincronización Vertical (VSync)";
    es["options.fov"] = "Campo de Visión (FOV)";
    es["options.master_volume"] = "Volumen Principal";
    es["options.subtitles"] = "Mostrar Subtítulos";
    es["options.high_contrast"] = "Modo Alto Contraste";
    es["options.ui_scale"] = "Escala de Interfaz";

    es["subtitles.step"] = "Pisadas";
    es["subtitles.break"] = "Bloque roto";
    es["subtitles.place"] = "Bloque colocado";
    es["subtitles.hurt"] = "Daño recibido";
    es["subtitles.explode"] = "Explosión";
    es["subtitles.splash"] = "Salpicadura";
    es["subtitles.zombie"] = "Zombi gime";
    es["subtitles.click"] = "Clic";

    es["tutorial.step1.title"] = "Paso 1: Exploración Básica";
    es["tutorial.step1.desc"] = "Muévete con WASD / Joystick y mira con el ratón / toque.";
    es["tutorial.step2.title"] = "Paso 2: Recolectar Madera";
    es["tutorial.step2.desc"] = "Mina un bloque de tronco de madera.";
    es["tutorial.step3.title"] = "Paso 3: Fabricación Inicial";
    es["tutorial.step3.desc"] = "Abre el inventario (E) y fabrica tablas y una mesa de trabajo.";
    es["tutorial.step4.title"] = "Paso 4: Construir Refugio";
    es["tutorial.step4.desc"] = "Coloca tu mesa de crafteo y construye muros para pasar la noche.";
    es["tutorial.step5.title"] = "Paso 5: Guardar Progreso";
    es["tutorial.step5.desc"] = "Presiona ESC / Pausa y guarda tu mundo con éxito.";
    es["tutorial.completed"] = "¡Tutorial básico completado con éxito!";

    // --- Inglés (en_US) ---
    auto& en = m_tables["en_US"];
    en["menu.play"] = "Play Game";
    en["menu.worlds"] = "Select World";
    en["menu.create_world"] = "Create New World";
    en["menu.multiplayer"] = "LAN Multiplayer";
    en["menu.options"] = "Options";
    en["menu.quit"] = "Quit Game";
    en["menu.resume"] = "Back to Game";
    en["menu.save_quit"] = "Save and Quit to Title";
    en["menu.back"] = "Back";
    en["menu.done"] = "Done";

    en["options.title"] = "Settings";
    en["options.video"] = "Video & Performance";
    en["options.audio"] = "Sound & Subtitles";
    en["options.controls"] = "Controls & Input";
    en["options.accessibility"] = "Accessibility";
    en["options.language"] = "Language";
    en["options.render_distance"] = "Render Distance";
    en["options.simulation_distance"] = "Simulation Distance";
    en["options.vsync"] = "Vertical Sync (VSync)";
    en["options.fov"] = "Field of View (FOV)";
    en["options.master_volume"] = "Master Volume";
    en["options.subtitles"] = "Show Subtitles";
    en["options.high_contrast"] = "High Contrast Mode";
    en["options.ui_scale"] = "UI Scale";

    en["subtitles.step"] = "Footsteps";
    en["subtitles.break"] = "Block broken";
    en["subtitles.place"] = "Block placed";
    en["subtitles.hurt"] = "Damage taken";
    en["subtitles.explode"] = "Explosion";
    en["subtitles.splash"] = "Splash";
    en["subtitles.zombie"] = "Zombie groans";
    en["subtitles.click"] = "Click";

    en["tutorial.step1.title"] = "Step 1: Basic Movement";
    en["tutorial.step1.desc"] = "Move using WASD / Joystick and look with Mouse / Touch.";
    en["tutorial.step2.title"] = "Step 2: Gathering Wood";
    en["tutorial.step2.desc"] = "Mine a wood log block.";
    en["tutorial.step3.title"] = "Step 3: Crafting Basics";
    en["tutorial.step3.desc"] = "Open inventory (E) and craft wooden planks and a crafting table.";
    en["tutorial.step4.title"] = "Step 4: Building Shelter";
    en["tutorial.step4.desc"] = "Place down your crafting table and build walls for the night.";
    en["tutorial.step5.title"] = "Step 5: Saving Progress";
    en["tutorial.step5.desc"] = "Press ESC / Pause and save your world successfully.";
    en["tutorial.completed"] = "Basic tutorial completed successfully!";
}

} // namespace OpenCraft

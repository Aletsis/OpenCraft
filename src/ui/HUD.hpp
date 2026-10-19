#pragma once

#include <string>
#include <vector>
#include "ui/UIBatch.hpp"
#include "render/TextureAtlas.hpp"
#include "sim/Snapshot.hpp"
#include "sim/GameCommand.hpp"

namespace OpenCraft {

class TouchController;
class AudioSubtitleManager;
class TutorialSystem;

struct SlotHit {
    bool hit = false;
    InventoryContainerType container = InventoryContainerType::PlayerInventory;
    size_t slotIndex = 0;
    // Paleta creativa: ítem bajo el cursor (EMPTY_ITEM_ID si es la papelera)
    bool creativePalette = false;
    uint16_t paletteItem = 0;
};

// Información del panel de depuración (F3)
struct DebugInfo {
    int fps = 0;
    size_t loadedChunks = 0;
    size_t renderedSections = 0;
    size_t pendingChunks = 0;
    std::string facing;
    std::string targetBlock;
    uint8_t skyLight = 0;
    uint8_t blockLight = 0;
    uint64_t timeOfDay = 0;
    std::string biome;
};

struct HudState {
    int guiScale = 2;
    bool showDebug = false;
    DebugInfo debug;
    std::string heldItemName;
    float heldItemNameAlpha = 0.0f;
    std::string actionMessage; // mensajes breves (p. ej. "Captura guardada")
    float actionMessageAlpha = 0.0f;
    int creativePage = 0;
    bool highContrast = false;
};

class HUD {
public:
    static int computeGuiScale(int screenWidth, int screenHeight, int preferred);

    void renderInGame(UIBatch& ui, const WorldSnapshot& snap, const HudState& state);
    void renderInventory(UIBatch& ui, const WorldSnapshot& snap, const HudState& state, float mouseX, float mouseY);
    void renderTouchControls(UIBatch& ui, const TouchController& touch);
    void renderSubtitles(UIBatch& ui, const AudioSubtitleManager& subtitles, int guiScale);
    void renderTutorialBanner(UIBatch& ui, const TutorialSystem& tutorial, int guiScale);

    SlotHit getSlotAt(int screenWidth, int screenHeight, const WorldSnapshot& snap, const HudState& state,
                      float screenX, float screenY) const;
    // Botones de página de la paleta creativa: -1 anterior, +1 siguiente, 0 ninguno
    int getCreativePageButtonAt(int screenWidth, int screenHeight, const HudState& state, float x, float y) const;
    static int creativePageCount();
    bool isInsidePanel(int screenWidth, int screenHeight, const HudState& state, float x, float y) const;

    // Dibuja el ícono de un ítem: cubo isométrico para bloques, tesela plana para objetos
    static void drawItemIcon(UIBatch& ui, uint16_t itemId, float x, float y, float size);
    static void drawItemStack(UIBatch& ui, const ItemStack& stack, float x, float y, float s);

private:
    struct SlotRect {
        float x, y; // esquina del área interior de 16x16 (en píxeles de pantalla)
        InventoryContainerType container;
        size_t index;
    };
    struct InventoryLayout {
        float s = 2.0f;
        float panelX = 0, panelY = 0, panelW = 176, panelH = 166;
        std::string title;
        float titleX = 8, titleY = 6;
        bool showInventoryLabel = true;
        std::vector<SlotRect> slots;
        bool creative = false;
    };

    InventoryLayout computeLayout(int w, int h, const WorldSnapshot& snap, const HudState& state) const;
    static const std::vector<uint16_t>& creativeItems();
};

} // namespace OpenCraft

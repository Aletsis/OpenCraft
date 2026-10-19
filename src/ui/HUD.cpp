#include "ui/HUD.hpp"
#include "ui/BitmapFont.hpp"
#include "ui/ItemNames.hpp"
#include "item/Item.hpp"
#include "world/BlockRegistry.hpp"
#include "input/TouchController.hpp"
#include "audio/AudioSubtitles.hpp"
#include "game/TutorialSystem.hpp"
#include <algorithm>
#include <cmath>
#include <format>

namespace OpenCraft {

namespace {

constexpr UIColor PANEL_BG{0.776f, 0.776f, 0.776f, 1.0f};
constexpr UIColor PANEL_LIGHT{1.0f, 1.0f, 1.0f, 1.0f};
constexpr UIColor PANEL_DARK{0.333f, 0.333f, 0.333f, 1.0f};
constexpr UIColor SLOT_BG{0.545f, 0.545f, 0.545f, 1.0f};
constexpr UIColor SLOT_DARK{0.216f, 0.216f, 0.216f, 1.0f};
constexpr UIColor LABEL{0.25f, 0.25f, 0.25f, 1.0f};

// Iconos de estado 9x9 dibujados píxel a píxel
const char* HEART_OUTLINE[9] = {".##...##.", "#..#.#..#", "#...#...#", "#.......#", ".#.....#.",
                                "..#...#..", "...#.#...", "....#....", "........."};
const char* HEART_FILL[9] = {".........", ".##...##.", ".###.###.", ".#######.", "..#####..",
                             "...###...", "....#....", ".........", "........."};
const char* SHANK_OUTLINE[9] = {"....###..", "...#...#.", "...#...#.", "..#...#..", ".##..#...",
                                "#..##....", "#..#.....", ".##......", "........."};
const char* SHANK_MEAT[9] = {".........", "....###..", "....###..", "...###...", "..##.....",
                             ".........", ".........", ".........", "........."};
const char* SHANK_BONE[9] = {".........", ".........", ".........", ".........", ".........",
                             ".##......", ".##......", ".........", "........."};
const char* BUBBLE[9] = {"..####...", ".#....#..", "#.##...#.", "#.#....#.", "#......#.",
                         "#......#.", ".#....#..", "..####...", "........."};
const char* ARMOR[9] = {"##.....##", "#########", "#########", ".#######.", ".#######.",
                        ".#######.", "..#####..", "...###...", "........."};

void drawPattern(UIBatch& ui, const char* const rows[9], float x, float y, float s, UIColor c, int maxCol = 9) {
    for (int r = 0; r < 9; ++r) {
        int col = 0;
        while (col < maxCol) {
            if (rows[r][col] != '#') {
                ++col;
                continue;
            }
            int start = col;
            while (col < maxCol && rows[r][col] == '#') ++col;
            ui.rect(x + start * s, y + r * s, (col - start) * s, s, c);
        }
    }
}

void drawSlotBackground(UIBatch& ui, float x, float y, float s) {
    ui.rect(x - s, y - s, 18 * s, 18 * s, SLOT_BG);
    ui.rect(x - s, y - s, 17 * s, s, SLOT_DARK);
    ui.rect(x - s, y - s, s, 17 * s, SLOT_DARK);
    ui.rect(x, y + 16 * s, 17 * s, s, PANEL_LIGHT);
    ui.rect(x + 16 * s, y, s, 17 * s, PANEL_LIGHT);
}

void drawPanel(UIBatch& ui, float x, float y, float w, float h, float s) {
    ui.rect(x + s, y, w - 2 * s, h, UIColors::Black);
    ui.rect(x, y + s, w, h - 2 * s, UIColors::Black);
    ui.rect(x + s, y + s, w - 2 * s, h - 2 * s, PANEL_BG);
    ui.rect(x + s, y + s, w - 3 * s, 2 * s, PANEL_LIGHT);
    ui.rect(x + s, y + s, 2 * s, h - 3 * s, PANEL_LIGHT);
    ui.rect(x + 2 * s, y + h - 3 * s, w - 3 * s, 2 * s, PANEL_DARK);
    ui.rect(x + w - 3 * s, y + 2 * s, 2 * s, h - 3 * s, PANEL_DARK);
}

bool isFlatIconBlock(const BlockProperties& p) {
    static const char* FLAT[] = {
        "torch", "rail",    "cinnabar_wire", "pressure_plate", "button", "lever",    "door",       "sign",
        "wheat", "carrots", "potatoes",      "fire",           "portal", "repeater", "comparator", "bed"};
    for (const char* f : FLAT) {
        if (p.namespacedId.find(f) != std::string::npos) return true;
    }
    return false;
}

const BlockProperties* iconBlockFor(uint16_t itemId) {
    const auto& def = ItemRegistry::global().get(itemId);
    if (def.placeableBlockState == AIR_BLOCK_STATE) return nullptr;
    const auto& p = BlockRegistry::global().get(def.placeableBlockState);
    if (isFlatIconBlock(p) || p.legacyId == BlockID::Water || p.legacyId == BlockID::Lava) return nullptr;
    return &p;
}

void tileUV(int tile, float& u0, float& v0, float& u1, float& v1) {
    constexpr float t = 1.0f / 16.0f;
    u0 = static_cast<float>(tile % 16) * t;
    v0 = static_cast<float>(tile / 16) * t;
    u1 = u0 + t;
    v1 = v0 + t;
}

} // namespace

int HUD::computeGuiScale(int w, int h, int preferred) {
    int autoScale = std::max(1, std::min(w / 320, h / 240));
    if (preferred <= 0) return autoScale;
    return std::clamp(preferred, 1, std::max(1, autoScale));
}

void HUD::drawItemIcon(UIBatch& ui, uint16_t itemId, float x, float y, float size) {
    if (itemId == EMPTY_ITEM_ID) return;
    if (const BlockProperties* bp = iconBlockFor(itemId)) {
        // Cubo isométrico: cara superior, frontal izquierda (sur) y frontal derecha (este)
        float u0, v0, u1, v1;
        auto P = [&](float px, float py, float out[2]) {
            out[0] = x + size * (0.06f + px * 0.88f);
            out[1] = y + size * (0.02f + py * 0.96f);
        };
        float top[4][2], left[4][2], right[4][2];
        P(0.5f, 0.0f, top[0]);
        P(1.0f, 0.25f, top[1]);
        P(0.5f, 0.5f, top[2]);
        P(0.0f, 0.25f, top[3]);
        P(0.0f, 0.25f, left[0]);
        P(0.5f, 0.5f, left[1]);
        P(0.5f, 1.0f, left[2]);
        P(0.0f, 0.75f, left[3]);
        P(0.5f, 0.5f, right[0]);
        P(1.0f, 0.25f, right[1]);
        P(1.0f, 0.75f, right[2]);
        P(0.5f, 1.0f, right[3]);

        tileUV(bp->textureIndices[0], u0, v0, u1, v1);
        float uvTop[4][2] = {{u0, v0}, {u1, v0}, {u1, v1}, {u0, v1}};
        ui.imageQuad(top, uvTop, UIColors::White);
        tileUV(bp->textureIndices[3], u0, v0, u1, v1);
        float uvSide[4][2] = {{u0, v0}, {u1, v0}, {u1, v1}, {u0, v1}};
        ui.imageQuad(left, uvSide, UIColor{0.8f, 0.8f, 0.8f, 1.0f});
        tileUV(bp->textureIndices[5], u0, v0, u1, v1);
        float uvSide2[4][2] = {{u0, v0}, {u1, v0}, {u1, v1}, {u0, v1}};
        ui.imageQuad(right, uvSide2, UIColor{0.6f, 0.6f, 0.6f, 1.0f});
        return;
    }
    const auto& def = ItemRegistry::global().get(itemId);
    int tile = def.iconTileIndex;
    if (def.placeableBlockState != AIR_BLOCK_STATE && def.iconTileIndex < 96) {
        tile = BlockRegistry::global().get(def.placeableBlockState).textureIndices[2];
    }
    ui.atlasTile(x, y, size, tile);
}

void HUD::drawItemStack(UIBatch& ui, const ItemStack& stack, float x, float y, float s) {
    if (stack.isEmpty()) return;
    drawItemIcon(ui, stack.itemId, x, y, 16 * s);
    const auto& def = ItemRegistry::global().get(stack.itemId);
    if (def.maxDurability > 0 && stack.durability > 0) {
        float frac = 1.0f - static_cast<float>(stack.durability) / static_cast<float>(def.maxDurability);
        frac = std::clamp(frac, 0.0f, 1.0f);
        ui.rect(x + 2 * s, y + 13 * s, 13 * s, 2 * s, UIColors::Black);
        ui.rect(x + 2 * s, y + 13 * s, std::round(13 * frac) * s, s, UIColor{1.0f - frac, frac, 0.0f, 1.0f});
    }
    if (stack.count > 1) {
        std::string n = std::to_string(stack.count);
        float w = UIBatch::textWidth(n, s);
        ui.text(n, x + 17 * s - w, y + 9 * s, s, UIColors::White);
    }
}

// ---------------------------------------------------------------------------------------------
// HUD de juego
// ---------------------------------------------------------------------------------------------

void HUD::renderInGame(UIBatch& ui, const WorldSnapshot& snap, const HudState& st) {
    const float W = static_cast<float>(ui.width());
    const float H = static_cast<float>(ui.height());
    const float s = static_cast<float>(st.guiScale);
    const bool survivalLike = snap.gameMode == 0 || snap.gameMode == 2;

    // Viñeta de daño y de inmersión
    if (snap.hurtTime > 0) {
        ui.rect(0, 0, W, H, UIColor{0.7f, 0.0f, 0.0f, 0.04f * static_cast<float>(snap.hurtTime)});
    }
    if (snap.isHeadInWater) {
        ui.rect(0, 0, W, H, UIColor{0.05f, 0.15f, 0.45f, 0.35f});
    } else if (snap.isHeadInLava) {
        ui.rect(0, 0, W, H, UIColor{0.9f, 0.35f, 0.0f, 0.75f});
    }

    // Retícula
    if (!st.showDebug) {
        float cx = std::round(W * 0.5f), cy = std::round(H * 0.5f);
        UIColor c{1.0f, 1.0f, 1.0f, 0.85f};
        ui.rect(cx - 5 * s, cy - s * 0.5f, 10 * s, s, c);
        ui.rect(cx - s * 0.5f, cy - 5 * s, s, 4.5f * s, c);
        ui.rect(cx - s * 0.5f, cy + s * 0.5f, s, 4.5f * s, c);
    }

    // Barra rápida
    float hx = std::round((W - 182 * s) * 0.5f);
    float hy = H - 22 * s;
    ui.rect(hx, hy, 182 * s, 22 * s, UIColor{0.0f, 0.0f, 0.0f, 0.55f});
    ui.outline(hx, hy, 182 * s, 22 * s, s, UIColor{0.45f, 0.45f, 0.45f, 0.9f});
    for (int i = 0; i < 9; ++i) {
        float sx = hx + (3 + 20 * i) * s;
        ui.outline(sx - 2 * s, hy + s, 20 * s, 20 * s, s, UIColor{0.3f, 0.3f, 0.3f, 0.9f});
    }
    if (snap.selectedSlot < 9) {
        float sx = hx + (20 * static_cast<float>(snap.selectedSlot) - 1) * s;
        ui.outline(sx, hy - s, 24 * s, 24 * s, 2 * s, UIColors::White);
        ui.outline(sx + 2 * s, hy + s, 20 * s, 20 * s, s, UIColor{0.2f, 0.2f, 0.2f, 1.0f});
    }
    for (size_t i = 0; i < 9 && i < snap.inventorySlots.size(); ++i) {
        drawItemStack(ui, snap.inventorySlots[i], hx + (3 + 20 * static_cast<float>(i)) * s, hy + 3 * s, s);
    }

    // Nombre del objeto seleccionado
    if (st.heldItemNameAlpha > 0.01f && !st.heldItemName.empty()) {
        float y = hy - (survivalLike ? 37 : 14) * s;
        ui.textCentered(st.heldItemName, W * 0.5f, y, s, UIColor{1, 1, 1, st.heldItemNameAlpha});
    }

    if (survivalLike) {
        // Experiencia
        float xpY = H - 29 * s;
        ui.rect(hx, xpY, 182 * s, 5 * s, UIColor{0.0f, 0.0f, 0.0f, 0.7f});
        ui.rect(hx + s, xpY + s, std::round(180 * std::clamp(snap.xpProgress, 0.0f, 1.0f)) * s, 3 * s,
                UIColor{0.5f, 1.0f, 0.13f, 1.0f});
        if (snap.xpLevel > 0) {
            std::string lvl = std::to_string(snap.xpLevel);
            float lw = UIBatch::textWidth(lvl, s);
            ui.text(lvl, W * 0.5f - lw * 0.5f, xpY - 7 * s, s, UIColor{0.5f, 1.0f, 0.13f, 1.0f});
        }

        // Corazones (fila izquierda) y hambre (fila derecha)
        float rowY = H - 39 * s;
        UIColor heartRed = snap.isHardcore ? UIColor{0.75f, 0.1f, 0.1f, 1.0f} : UIColor{0.9f, 0.1f, 0.1f, 1.0f};
        for (int i = 0; i < 10; ++i) {
            float x = hx + 8 * i * s;
            drawPattern(ui, HEART_OUTLINE, x, rowY, s, UIColors::Black);
            drawPattern(ui, HEART_FILL, x, rowY, s, UIColor{0.18f, 0.05f, 0.05f, 1.0f});
            float hp = snap.health - static_cast<float>(i * 2);
            if (hp >= 2.0f) {
                drawPattern(ui, HEART_FILL, x, rowY, s, heartRed);
                ui.rect(x + 2 * s, rowY + 2 * s, s, s, UIColor{1.0f, 0.7f, 0.7f, 1.0f});
            } else if (hp >= 1.0f) {
                drawPattern(ui, HEART_FILL, x, rowY, s, heartRed, 5);
            }
        }
        for (int i = 0; i < 10; ++i) {
            float x = hx + 182 * s - 9 * s - 8 * i * s;
            drawPattern(ui, SHANK_OUTLINE, x, rowY, s, UIColors::Black);
            float food = snap.hunger - static_cast<float>(i * 2);
            if (food >= 1.0f) {
                drawPattern(ui, SHANK_MEAT, x, rowY, s, UIColor{0.72f, 0.38f, 0.12f, 1.0f}, food >= 2.0f ? 9 : 5);
                drawPattern(ui, SHANK_BONE, x, rowY, s, UIColor{0.93f, 0.9f, 0.82f, 1.0f});
            } else {
                drawPattern(ui, SHANK_MEAT, x, rowY, s, UIColor{0.2f, 0.12f, 0.05f, 1.0f});
            }
        }
        // Armadura sobre los corazones, oxígeno sobre el hambre
        if (snap.armorPoints > 0.0f) {
            for (int i = 0; i < 10; ++i) {
                float x = hx + 8 * i * s;
                float a = snap.armorPoints - static_cast<float>(i * 2);
                drawPattern(ui, ARMOR, x, rowY - 10 * s, s,
                            a >= 1.0f ? UIColor{0.85f, 0.85f, 0.9f, 1.0f} : UIColor{0.2f, 0.2f, 0.2f, 0.8f});
            }
        }
        if (snap.air < 300) {
            int bubbles = static_cast<int>(std::ceil(static_cast<float>(snap.air) / 30.0f));
            for (int i = 0; i < bubbles && i < 10; ++i) {
                float x = hx + 182 * s - 9 * s - 8 * i * s;
                drawPattern(ui, BUBBLE, x, rowY - 10 * s, s, UIColor{0.6f, 0.85f, 1.0f, 1.0f});
            }
        }
    }

    if (st.actionMessageAlpha > 0.01f && !st.actionMessage.empty()) {
        ui.textCentered(st.actionMessage, W * 0.5f, H - 72 * s, s, UIColor{1, 1, 1, st.actionMessageAlpha});
    }

    // Panel de depuración (F3)
    if (st.showDebug) {
        const auto& d = st.debug;
        auto pos = snap.player.currentPosition;
        int bx = static_cast<int>(std::floor(pos.x)), by = static_cast<int>(std::floor(pos.y)),
            bz = static_cast<int>(std::floor(pos.z));
        std::vector<std::string> lines = {
            std::format("OpenCraft 1.0.0-rc1 ({} fps)", d.fps),
            std::format("Chunks: {} cargados, {} secciones visibles, {} pendientes", d.loadedChunks, d.renderedSections,
                        d.pendingChunks),
            std::format("XYZ: {:.3f} / {:.3f} / {:.3f}", pos.x, pos.y, pos.z),
            std::format("Bloque: {} {} {}  Chunk: {} {}", bx, by, bz, static_cast<int>(std::floor(pos.x / 16.0f)),
                        static_cast<int>(std::floor(pos.z / 16.0f))),
            std::format("Mirando: {}", d.facing),
            std::format("Luz: cielo {} / bloque {}", d.skyLight, d.blockLight),
            std::format("Hora: {}  Dimensión: {}", d.timeOfDay,
                        snap.dimension == 0 ? "Superficie" : (snap.dimension == 1 ? "Abismo" : "El Confín")),
        };
        if (!d.biome.empty()) lines.push_back("Bioma: " + d.biome);
        if (!d.targetBlock.empty()) lines.push_back("Objetivo: " + d.targetBlock);
        float y = 2 * s;
        for (const auto& l : lines) {
            float w = UIBatch::textWidth(l, s);
            ui.rect(s, y - s, w + 3 * s, 10 * s, UIColor{0.31f, 0.31f, 0.31f, 0.56f});
            ui.text(l, 2 * s, y, s, UIColor{0.88f, 0.88f, 0.88f, 1.0f}, false);
            y += 10 * s;
        }
        // Ejes en lugar de la retícula
        float cx = W * 0.5f, cy = H * 0.5f;
        ui.rect(cx, cy - s * 0.5f, 10 * s, s, UIColor{1, 0.2f, 0.2f, 1});
        ui.rect(cx - s * 0.5f, cy - 10 * s, s, 10 * s, UIColor{0.2f, 1, 0.2f, 1});
        ui.rect(cx - 7 * s, cy + 3 * s, 7 * s, s, UIColor{0.3f, 0.5f, 1, 1});
    }
}

// ---------------------------------------------------------------------------------------------
// Inventario y contenedores
// ---------------------------------------------------------------------------------------------

const std::vector<uint16_t>& HUD::creativeItems() {
    static const std::vector<uint16_t> items = [] {
        std::vector<uint16_t> list;
        const auto& reg = ItemRegistry::global();
        for (size_t i = 1; i < reg.getItemCount(); ++i) {
            const auto& def = reg.get(static_cast<ItemId>(i));
            if (def.id == EMPTY_ITEM_ID) continue;
            // Bloques técnicos que no deben obtenerse desde la paleta
            static const char* HIDDEN[] = {"opencraft:air",    "flowing_",       "piston_head",
                                           "abyss_portal",     "opencraft:fire", "unlit_cinnabar_torch",
                                           "lit_cinnabar_lamp"};
            bool hidden = false;
            for (const char* h : HIDDEN) {
                if (def.namespacedId.find(h) != std::string::npos) hidden = true;
            }
            if (def.namespacedId == "opencraft:rim_portal") hidden = true;
            if (!hidden) list.push_back(static_cast<uint16_t>(i));
        }
        return list;
    }();
    return items;
}

int HUD::creativePageCount() {
    constexpr size_t PER_PAGE = 45;
    return std::max(1, static_cast<int>((creativeItems().size() + PER_PAGE - 1) / PER_PAGE));
}

HUD::InventoryLayout HUD::computeLayout(int w, int h, const WorldSnapshot& snap, const HudState& st) const {
    InventoryLayout L;
    L.s = static_cast<float>(st.guiScale);
    const float s = L.s;
    L.panelX = std::round((static_cast<float>(w) - 176 * s) * 0.5f);
    L.panelY = std::round((static_cast<float>(h) - 166 * s) * 0.5f);
    auto add = [&](float gx, float gy, InventoryContainerType c, size_t idx) {
        L.slots.push_back({L.panelX + gx * s, L.panelY + gy * s, c, idx});
    };

    bool creative = snap.gameMode == 1 && !snap.isContainerOpen;
    L.creative = creative;
    if (creative) {
        L.title = "Inventario creativo";
        L.showInventoryLabel = false;
    } else if (snap.isContainerOpen && snap.openContainerKind == OpenContainerKind::Chest) {
        L.title = "Cofre";
        for (size_t r = 0; r < 3; ++r)
            for (size_t c = 0; c < 9; ++c)
                add(8 + 18 * static_cast<float>(c), 18 + 18 * static_cast<float>(r),
                    InventoryContainerType::ChestContainer, r * 9 + c);
    } else if (snap.isContainerOpen && snap.openContainerKind == OpenContainerKind::CraftingTable) {
        L.title = "Mesa de trabajo";
        L.titleX = 28;
        for (size_t r = 0; r < 3; ++r)
            for (size_t c = 0; c < 3; ++c)
                add(30 + 18 * static_cast<float>(c), 17 + 18 * static_cast<float>(r),
                    InventoryContainerType::CraftingGrid, r * 3 + c);
        add(124, 35, InventoryContainerType::CraftingOutput, 0);
    } else if (snap.isContainerOpen && snap.openContainerKind == OpenContainerKind::Furnace) {
        L.title = "Horno";
        L.titleX = 88 - UIBatch::textWidth("Horno", 1.0f) * 0.5f;
        add(56, 17, InventoryContainerType::FurnaceContainer, 0);
        add(56, 53, InventoryContainerType::FurnaceContainer, 1);
        add(116, 35, InventoryContainerType::FurnaceContainer, 2);
    } else {
        L.title = "Fabricación";
        L.titleX = 97;
        L.showInventoryLabel = false; // en el inventario personal la figura del jugador ocupa ese espacio
        for (size_t r = 0; r < 2; ++r)
            for (size_t c = 0; c < 2; ++c)
                add(98 + 18 * static_cast<float>(c), 18 + 18 * static_cast<float>(r),
                    InventoryContainerType::CraftingGrid, r * 2 + c);
        add(154, 28, InventoryContainerType::CraftingOutput, 0);
    }

    if (!creative) {
        for (size_t r = 0; r < 3; ++r)
            for (size_t c = 0; c < 9; ++c)
                add(8 + 18 * static_cast<float>(c), 84 + 18 * static_cast<float>(r),
                    InventoryContainerType::PlayerInventory, 9 + r * 9 + c);
    }
    for (size_t c = 0; c < 9; ++c) {
        add(8 + 18 * static_cast<float>(c), 142, InventoryContainerType::PlayerInventory, c);
    }
    return L;
}

SlotHit HUD::getSlotAt(int w, int h, const WorldSnapshot& snap, const HudState& st, float mx, float my) const {
    SlotHit hit;
    InventoryLayout L = computeLayout(w, h, snap, st);
    const float s = L.s;
    for (const auto& sl : L.slots) {
        if (mx >= sl.x - s && mx < sl.x + 17 * s && my >= sl.y - s && my < sl.y + 17 * s) {
            hit.hit = true;
            hit.container = sl.container;
            hit.slotIndex = sl.index;
            return hit;
        }
    }
    if (L.creative) {
        const auto& items = creativeItems();
        size_t base = static_cast<size_t>(std::clamp(st.creativePage, 0, creativePageCount() - 1)) * 45;
        for (int r = 0; r < 5; ++r) {
            for (int c = 0; c < 9; ++c) {
                float x = L.panelX + (8 + 18 * c) * s, y = L.panelY + (18 + 18 * r) * s;
                if (mx >= x - s && mx < x + 17 * s && my >= y - s && my < y + 17 * s) {
                    size_t idx = base + static_cast<size_t>(r * 9 + c);
                    hit.hit = true;
                    hit.creativePalette = true;
                    hit.paletteItem = idx < items.size() ? items[idx] : EMPTY_ITEM_ID;
                    return hit;
                }
            }
        }
    }
    return hit;
}

bool HUD::isInsidePanel(int w, int h, const HudState& st, float mx, float my) const {
    const float s = static_cast<float>(st.guiScale);
    float px = std::round((static_cast<float>(w) - 176 * s) * 0.5f);
    float py = std::round((static_cast<float>(h) - 166 * s) * 0.5f);
    return mx >= px && mx < px + 176 * s && my >= py && my < py + 166 * s;
}

int HUD::getCreativePageButtonAt(int w, int h, const HudState& st, float mx, float my) const {
    const float s = static_cast<float>(st.guiScale);
    float px = std::round((static_cast<float>(w) - 176 * s) * 0.5f);
    float py = std::round((static_cast<float>(h) - 166 * s) * 0.5f);
    float by = py + 112 * s;
    if (my < by || my > by + 12 * s) return 0;
    if (mx >= px + 7 * s && mx < px + 27 * s) return -1;
    if (mx >= px + 149 * s && mx < px + 169 * s) return 1;
    return 0;
}

void HUD::renderInventory(UIBatch& ui, const WorldSnapshot& snap, const HudState& st, float mx, float my) {
    const float W = static_cast<float>(ui.width());
    const float H = static_cast<float>(ui.height());
    InventoryLayout L = computeLayout(ui.width(), ui.height(), snap, st);
    const float s = L.s;

    ui.rect(0, 0, W, H, UIColor{0.06f, 0.06f, 0.06f, 0.75f});
    drawPanel(ui, L.panelX, L.panelY, 176 * s, 166 * s, s);
    ui.text(L.title, L.panelX + L.titleX * s, L.panelY + L.titleY * s, s, LABEL, false);
    if (L.showInventoryLabel) {
        ui.text("Inventario", L.panelX + 8 * s, L.panelY + 73 * s, s, LABEL, false);
    }

    // Figura del jugador en el inventario personal
    if (!snap.isContainerOpen && !L.creative) {
        float fx = L.panelX + 26 * s, fy = L.panelY + 8 * s;
        ui.rect(fx, fy, 49 * s, 70 * s, UIColors::Black);
        ui.rect(fx + s, fy + s, 47 * s, 68 * s, UIColor{0.12f, 0.12f, 0.14f, 1.0f});
        float cx = fx + 24.5f * s;
        ui.rect(cx - 4 * s, fy + 8 * s, 8 * s, 8 * s, UIColor{0.78f, 0.58f, 0.45f, 1}); // cabeza
        ui.rect(cx - 4 * s, fy + 8 * s, 8 * s, 3 * s, UIColor{0.27f, 0.17f, 0.1f, 1});  // pelo
        ui.rect(cx - 2 * s, fy + 12 * s, s, s, UIColors::White);
        ui.rect(cx + s, fy + 12 * s, s, s, UIColors::White);
        ui.rect(cx - 4 * s, fy + 16 * s, 8 * s, 12 * s, UIColor{0.0f, 0.68f, 0.68f, 1}); // torso
        ui.rect(cx - 8 * s, fy + 16 * s, 4 * s, 12 * s, UIColor{0.0f, 0.6f, 0.6f, 1});   // brazos
        ui.rect(cx + 4 * s, fy + 16 * s, 4 * s, 12 * s, UIColor{0.0f, 0.6f, 0.6f, 1});
        ui.rect(cx - 8 * s, fy + 26 * s, 4 * s, 2 * s, UIColor{0.78f, 0.58f, 0.45f, 1});
        ui.rect(cx + 4 * s, fy + 26 * s, 4 * s, 2 * s, UIColor{0.78f, 0.58f, 0.45f, 1});
        ui.rect(cx - 4 * s, fy + 28 * s, 8 * s, 12 * s, UIColor{0.25f, 0.22f, 0.6f, 1}); // piernas
        ui.rect(cx - 4 * s, fy + 40 * s, 8 * s, 2 * s, UIColor{0.3f, 0.3f, 0.3f, 1});
    }

    // Flechas de fabricación / horno
    auto arrow = [&](float gx, float gy, float progress) {
        float x = L.panelX + gx * s, y = L.panelY + gy * s;
        ui.rect(x, y + 6 * s, 16 * s, 4 * s, SLOT_BG);
        for (int i = 0; i < 6; ++i)
            ui.rect(x + static_cast<float>(16 + i) * s, y + static_cast<float>(2 + i) * s, s,
                    static_cast<float>(12 - 2 * i) * s, SLOT_BG);
        if (progress > 0.0f) {
            float pw = std::round(22 * std::clamp(progress, 0.0f, 1.0f));
            ui.rect(x, y + 6 * s, std::min(16.0f, pw) * s, 4 * s, UIColors::White);
            for (int i = 0; i < 6 && static_cast<float>(16 + i) < pw; ++i) {
                ui.rect(x + static_cast<float>(16 + i) * s, y + static_cast<float>(2 + i) * s, s,
                        static_cast<float>(12 - 2 * i) * s, UIColors::White);
            }
        }
    };
    if (snap.isContainerOpen && snap.openContainerKind == OpenContainerKind::Furnace) {
        float cook = snap.furnaceCookTotal > 0
                         ? static_cast<float>(snap.furnaceCookProgress) / static_cast<float>(snap.furnaceCookTotal)
                         : 0.0f;
        arrow(79, 34, cook);
        float fx = L.panelX + 57 * s, fy = L.panelY + 37 * s;
        ui.rect(fx, fy, 13 * s, 13 * s, SLOT_BG);
        if (snap.furnaceLitTime > 0 && snap.furnaceLitDuration > 0) {
            float f = std::clamp(static_cast<float>(snap.furnaceLitTime) / static_cast<float>(snap.furnaceLitDuration),
                                 0.0f, 1.0f);
            float fh = std::round(13 * f);
            ui.rect(fx + 2 * s, fy + (13 - fh) * s, 9 * s, fh * s, UIColor{1.0f, 0.55f, 0.1f, 1});
            ui.rect(fx + 4 * s, fy + (13 - fh * 0.6f) * s, 5 * s, fh * 0.6f * s, UIColor{1.0f, 0.9f, 0.3f, 1});
        }
    } else if (snap.isContainerOpen && snap.openContainerKind == OpenContainerKind::CraftingTable) {
        arrow(90, 35, 0.0f);
    } else if (!L.creative) {
        arrow(135, 28, 0.0f);
    }

    // Ranuras
    SlotHit hover = getSlotAt(ui.width(), ui.height(), snap, st, mx, my);
    std::string tooltip;
    auto stackFor = [&](const SlotRect& sl) -> ItemStack {
        switch (sl.container) {
            case InventoryContainerType::ChestContainer:
            case InventoryContainerType::FurnaceContainer:
                return sl.index < snap.containerSlots.size() ? snap.containerSlots[sl.index] : ItemStack();
            case InventoryContainerType::CraftingGrid:
                return sl.index < snap.craftingGrid.size() ? snap.craftingGrid[sl.index] : ItemStack();
            case InventoryContainerType::CraftingOutput: return snap.craftingOutput;
            default: return sl.index < snap.inventorySlots.size() ? snap.inventorySlots[sl.index] : ItemStack();
        }
    };
    for (const auto& sl : L.slots) {
        if (sl.container == InventoryContainerType::CraftingOutput) {
            ui.rect(sl.x - 4 * s, sl.y - 4 * s, 24 * s, 24 * s, SLOT_DARK);
            ui.rect(sl.x - 3 * s, sl.y - 3 * s, 23 * s, 23 * s, PANEL_LIGHT);
            ui.rect(sl.x - 3 * s, sl.y - 3 * s, 22 * s, 22 * s, SLOT_BG);
        } else {
            drawSlotBackground(ui, sl.x, sl.y, s);
        }
        ItemStack stack = stackFor(sl);
        drawItemStack(ui, stack, sl.x, sl.y, s);
        if (hover.hit && !hover.creativePalette && hover.container == sl.container && hover.slotIndex == sl.index) {
            ui.rect(sl.x, sl.y, 16 * s, 16 * s, UIColor{1, 1, 1, 0.45f});
            if (!stack.isEmpty()) tooltip = localizedItemName(stack.itemId);
        }
    }

    if (L.creative) {
        const auto& items = creativeItems();
        int page = std::clamp(st.creativePage, 0, creativePageCount() - 1);
        size_t base = static_cast<size_t>(page) * 45;
        for (int r = 0; r < 5; ++r) {
            for (int c = 0; c < 9; ++c) {
                float x = L.panelX + static_cast<float>(8 + 18 * c) * s,
                      y = L.panelY + static_cast<float>(18 + 18 * r) * s;
                drawSlotBackground(ui, x, y, s);
                size_t idx = base + static_cast<size_t>(r * 9 + c);
                if (idx < items.size()) {
                    drawItemIcon(ui, items[idx], x, y, 16 * s);
                    if (hover.creativePalette && hover.paletteItem == items[idx]) {
                        ui.rect(x, y, 16 * s, 16 * s, UIColor{1, 1, 1, 0.45f});
                        tooltip = localizedItemName(items[idx]);
                    }
                }
            }
        }
        float by = L.panelY + 112 * s;
        std::string pageText = std::format("Página {} de {}", page + 1, creativePageCount());
        ui.textCentered(pageText, L.panelX + 88 * s, by + 2 * s, s, LABEL, false);
        for (int dir : {-1, 1}) {
            float bx = L.panelX + static_cast<float>(dir < 0 ? 7 : 149) * s;
            bool hot = getCreativePageButtonAt(ui.width(), ui.height(), st, mx, my) == dir;
            ui.rect(bx, by, 20 * s, 12 * s, hot ? UIColor{0.55f, 0.6f, 0.85f, 1} : SLOT_BG);
            ui.outline(bx, by, 20 * s, 12 * s, s, UIColors::Black);
            ui.textCentered(dir < 0 ? "<" : ">", bx + 10 * s, by + 2 * s, s, UIColors::White);
        }
        ui.text("Barra rápida", L.panelX + 8 * s, L.panelY + 130 * s, s, LABEL, false);
    }

    // Objeto en el cursor y descripción emergente
    if (!snap.cursorItem.isEmpty()) {
        drawItemStack(ui, snap.cursorItem, mx - 8 * s, my - 8 * s, s);
    } else if (!tooltip.empty()) {
        float tw = UIBatch::textWidth(tooltip, s);
        float tx = mx + 12 * s, ty = my - 12 * s;
        if (tx + tw + 6 * s > W) tx = W - tw - 6 * s;
        ui.rect(tx - 3 * s, ty - 3 * s, tw + 6 * s, 14 * s, UIColor{0.06f, 0.0f, 0.06f, 0.94f});
        ui.outline(tx - 2 * s, ty - 2 * s, tw + 4 * s, 12 * s, s, UIColor{0.31f, 0.0f, 1.0f, 0.5f});
        ui.text(tooltip, tx, ty, s, UIColors::White);
    }
}

// ---------------------------------------------------------------------------------------------
// Capas auxiliares
// ---------------------------------------------------------------------------------------------

void HUD::renderTouchControls(UIBatch& ui, const TouchController& touch) {
    Math::Vec2 c = touch.getJoystickCenter();
    Math::Vec2 k = touch.getJoystickKnob();
    float r = touch.getJoystickRadius();
    ui.rect(c.x - r, c.y - r, r * 2, r * 2, UIColor{0.2f, 0.2f, 0.2f, 0.35f});
    ui.outline(c.x - r, c.y - r, r * 2, r * 2, 2, UIColor{1, 1, 1, 0.4f});
    float kr = r * 0.45f;
    ui.rect(k.x - kr, k.y - kr, kr * 2, kr * 2, UIColor{0.9f, 0.9f, 0.9f, 0.6f});

    for (const auto& btn : touch.getButtons()) {
        if (btn.id == TouchButtonID::Hotbar) continue;
        float a = btn.isPressed ? 0.75f : 0.4f;
        UIColor col{0.2f, 0.2f, 0.2f, a};
        const char* label = "";
        switch (btn.id) {
            case TouchButtonID::Jump:
                col = {0.3f, 0.6f, 0.9f, a};
                label = "Saltar";
                break;
            case TouchButtonID::Attack:
                col = {0.9f, 0.3f, 0.3f, a};
                label = "Romper";
                break;
            case TouchButtonID::Use:
                col = {0.3f, 0.8f, 0.3f, a};
                label = "Usar";
                break;
            case TouchButtonID::Sneak:
                if (btn.isToggledOn) col = {0.8f, 0.8f, 0.2f, 0.8f};
                label = "Agachar";
                break;
            case TouchButtonID::Inventory: label = "Inv."; break;
            case TouchButtonID::Pause: label = "II"; break;
            default: break;
        }
        ui.rect(btn.x, btn.y, btn.w, btn.h, col);
        ui.outline(btn.x, btn.y, btn.w, btn.h, 2, UIColor{1, 1, 1, 0.6f});
        float ts = std::max(1.0f, std::floor(btn.h / 24.0f));
        ui.textCentered(label, btn.x + btn.w * 0.5f, btn.y + (btn.h - 8 * ts) * 0.5f, ts, UIColor{1, 1, 1, 0.9f});
    }
}

void HUD::renderSubtitles(UIBatch& ui, const AudioSubtitleManager& subtitles, int guiScale) {
    const auto& items = subtitles.getItems();
    if (items.empty()) return;
    const float s = static_cast<float>(guiScale);
    float y = static_cast<float>(ui.height()) - 40 * s;
    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[items.size() - 1 - i];
        std::string text = item.getFormattedText();
        float a = item.getAlpha();
        float w = UIBatch::textWidth(text, s);
        float x = static_cast<float>(ui.width()) - w - 6 * s;
        ui.rect(x - 2 * s, y - 2 * s, w + 4 * s, 11 * s, UIColor{0, 0, 0, 0.6f * a});
        ui.text(text, x, y, s, UIColor{1, 1, 1, a}, false);
        y -= 11 * s;
    }
}

void HUD::renderTutorialBanner(UIBatch& ui, const TutorialSystem& tutorial, int guiScale) {
    if (tutorial.isCompleted()) return;
    const float s = static_cast<float>(guiScale);
    std::string title = tutorial.getCurrentTitle();
    std::string desc = tutorial.getCurrentDescription();
    float w = std::max(UIBatch::textWidth(title, s), UIBatch::textWidth(desc, s)) + 12 * s;
    float x = std::round((static_cast<float>(ui.width()) - w) * 0.5f);
    float y = 4 * s;
    ui.rect(x, y, w, 26 * s, UIColor{0.08f, 0.08f, 0.12f, 0.8f});
    ui.outline(x, y, w, 26 * s, s, UIColor{0.9f, 0.8f, 0.2f, 1.0f});
    ui.text(title, x + 6 * s, y + 4 * s, s, UIColor{1.0f, 0.9f, 0.3f, 1.0f});
    ui.text(desc, x + 6 * s, y + 14 * s, s, UIColor{0.92f, 0.92f, 0.92f, 1.0f});
}

} // namespace OpenCraft

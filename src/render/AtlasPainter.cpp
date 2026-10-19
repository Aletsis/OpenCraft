#include "render/AtlasPainter.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <unordered_map>

namespace OpenCraft::AtlasPainter {

namespace {

struct RGBA {
    int r = 0, g = 0, b = 0, a = 255;
};

constexpr RGBA rgb(int r, int g, int b, int a = 255) { return {r, g, b, a}; }

uint32_t hash3(uint32_t x, uint32_t y, uint32_t seed) {
    uint32_t h = x * 374761393u + y * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

class Canvas {
public:
    explicit Canvas(std::vector<uint8_t>& px) : m_px(px) {}

    void set(int tile, int x, int y, RGBA c) {
        if (x < 0 || y < 0 || x >= TILE_SIZE || y >= TILE_SIZE) return;
        int gx = (tile % TILES_PER_ROW) * TILE_SIZE + x;
        int gy = (tile / TILES_PER_ROW) * TILE_SIZE + y;
        size_t i = (static_cast<size_t>(gy) * ATLAS_SIZE + static_cast<size_t>(gx)) * 4;
        m_px[i + 0] = static_cast<uint8_t>(std::clamp(c.r, 0, 255));
        m_px[i + 1] = static_cast<uint8_t>(std::clamp(c.g, 0, 255));
        m_px[i + 2] = static_cast<uint8_t>(std::clamp(c.b, 0, 255));
        m_px[i + 3] = static_cast<uint8_t>(std::clamp(c.a, 0, 255));
    }

    RGBA get(int tile, int x, int y) const {
        int gx = (tile % TILES_PER_ROW) * TILE_SIZE + x;
        int gy = (tile / TILES_PER_ROW) * TILE_SIZE + y;
        size_t i = (static_cast<size_t>(gy) * ATLAS_SIZE + static_cast<size_t>(gx)) * 4;
        return {m_px[i], m_px[i + 1], m_px[i + 2], m_px[i + 3]};
    }

    void copy(int dst, int src) {
        for (int y = 0; y < TILE_SIZE; ++y)
            for (int x = 0; x < TILE_SIZE; ++x) set(dst, x, y, get(src, x, y));
    }

    // Relleno con variación de luminosidad por píxel
    void noise(int tile, RGBA base, int variance, uint32_t seed) {
        for (int y = 0; y < TILE_SIZE; ++y) {
            for (int x = 0; x < TILE_SIZE; ++x) {
                int v = static_cast<int>(hash3(static_cast<uint32_t>(x), static_cast<uint32_t>(y), seed) %
                                         static_cast<uint32_t>(variance * 2 + 1)) -
                        variance;
                set(tile, x, y, {base.r + v, base.g + v, base.b + v, base.a});
            }
        }
    }

    // Vetas de mineral sobre una base ya pintada
    void specks(int tile, RGBA color, RGBA shade, int clusters, uint32_t seed) {
        for (int c = 0; c < clusters; ++c) {
            int cx = static_cast<int>(hash3(static_cast<uint32_t>(c), 1, seed) % 13) + 1;
            int cy = static_cast<int>(hash3(static_cast<uint32_t>(c), 2, seed) % 13) + 1;
            for (int i = 0; i < 4; ++i) {
                int dx = static_cast<int>(hash3(static_cast<uint32_t>(c), static_cast<uint32_t>(i + 3), seed) % 3) - 1;
                int dy = static_cast<int>(hash3(static_cast<uint32_t>(c), static_cast<uint32_t>(i + 9), seed) % 3) - 1;
                set(tile, cx + dx, cy + dy, i == 0 ? shade : color);
            }
        }
    }

    void bricks(int tile, RGBA brick, RGBA mortar, int bw, int bh, uint32_t seed) {
        for (int y = 0; y < TILE_SIZE; ++y) {
            int row = y / bh;
            int offset = (row % 2) * (bw / 2);
            for (int x = 0; x < TILE_SIZE; ++x) {
                bool m = (y % bh == bh - 1) || ((x + offset) % bw == bw - 1);
                int v = static_cast<int>(hash3(static_cast<uint32_t>(x), static_cast<uint32_t>(y), seed) % 13) - 6;
                RGBA c = m ? mortar : RGBA{brick.r + v, brick.g + v, brick.b + v, 255};
                set(tile, x, y, c);
            }
        }
    }

    void frame(int tile, RGBA c, int thickness = 1) {
        for (int i = 0; i < TILE_SIZE; ++i) {
            for (int t = 0; t < thickness; ++t) {
                set(tile, i, t, c);
                set(tile, i, TILE_SIZE - 1 - t, c);
                set(tile, t, i, c);
                set(tile, TILE_SIZE - 1 - t, i, c);
            }
        }
    }

    void rect(int tile, int x0, int y0, int x1, int y1, RGBA c) {
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x) set(tile, x, y, c);
    }

    void clear(int tile) {
        for (int y = 0; y < TILE_SIZE; ++y)
            for (int x = 0; x < TILE_SIZE; ++x) set(tile, x, y, {0, 0, 0, 0});
    }

    // Sprite de 16 filas: cada carácter se traduce mediante la paleta ('.' = transparente)
    void sprite(int tile, const std::array<const char*, 16>& rows, const std::unordered_map<char, RGBA>& palette) {
        clear(tile);
        for (int y = 0; y < 16; ++y) {
            const char* row = rows[static_cast<size_t>(y)];
            for (int x = 0; x < 16 && row[x]; ++x) {
                auto it = palette.find(row[x]);
                if (it != palette.end()) set(tile, x, y, it->second);
            }
        }
    }

private:
    std::vector<uint8_t>& m_px;
};

using Palette = std::unordered_map<char, RGBA>;
using Rows = std::array<const char*, 16>;

Palette material(RGBA main, RGBA light, RGBA dark) {
    return {{'M', main},           {'L', light}, {'D', dark}, {'h', rgb(105, 75, 38)}, {'H', rgb(140, 100, 55)},
            {'k', rgb(40, 30, 20)}};
}

// --- Plantillas de herramientas y armadura (M = material, L = brillo, D = sombra, h = mango) ---
const Rows SWORD = {"................", ".............LL.", "............LML.", "...........LMD..",
                    "..........LMD...", ".........LMD....", "........LMD.....", ".......LMD......",
                    "..kk..LMD.......", "..kMkLMD........", "...kMMD.........", "....hkk.........",
                    "...hH.kk........", "..hH............", ".kk.............", "................"};
const Rows PICKAXE = {"................", ".....DMMMMMD....", "...DMLLLLLLMD...", "..DML......LMD..",
                      "..ML.......hLMD.", "..D.......hH.LD.", "..........hH..M.", ".........hH...D.",
                      "........hH......", ".......hH.......", "......hH........", ".....hH.........",
                      "....hH..........", "...hH...........", "..kk............", "................"};
const Rows AXE = {"................", "........DMM.....", ".......DMLLM....", "......DMLLLLhH..",
                  "......DMLLLhH...", ".......DMMhH....", ".........hH.....", "........hH......",
                  ".......hH.......", "......hH........", ".....hH.........", "....hH..........",
                  "...hH...........", "..hH............", ".kk.............", "................"};
const Rows SHOVEL = {"................", "...........DMM..", "..........DMLLM.", ".........DMLLLLM",
                     ".........DMLLLM.", "..........DMMM..", ".........hH.....", "........hH......",
                     ".......hH.......", "......hH........", ".....hH.........", "....hH..........",
                     "...hH...........", "..hH............", ".kk.............", "................"};
const Rows HOE = {"................", ".......DMMMM....", ".......DDLMMhH..", "...........hH...",
                  "..........hH....", ".........hH.....", "........hH......", ".......hH.......",
                  "......hH........", ".....hH.........", "....hH..........", "...hH...........",
                  "..hH............", ".kk.............", "................", "................"};
const Rows HELMET = {"................", "................", "................", "....DMMMMMMD....",
                     "...DMLLLLLLMD...", "...ML......LM...", "...MD......DM...", "...M........M...",
                     "................", "................", "................", "................",
                     "................", "................", "................", "................"};
const Rows CHESTPLATE = {"................", "...DMM....MMD...", "..DMLLM..MLLMD..", "..MLLLMMMMLLLM..",
                         "..MMLLLLLLLLMM..", "....MLLLLLLM....", "....MLLLLLLM....", "....MLLLLLLM....",
                         "....MLLLLLLM....", "....MDDDDDDM....", "....MMMMMMMM....", "................",
                         "................", "................", "................", "................"};
const Rows LEGGINGS = {"................", "................", "...DMMMMMMMMD...", "...MLLLLLLLLM...",
                       "...MLLLMMLLLM...", "...MLLM..MLLM...", "...MLLM..MLLM...", "...MLLM..MLLM...",
                       "...MLLM..MLLM...", "...MDDM..MDDM...", "...MMMM..MMMM...", "................",
                       "................", "................", "................", "................"};
const Rows BOOTS = {"................", "................", "................", "................",
                    "................", "................", "...DMM....MMD...", "...MLM....MLM...",
                    "...MLM....MLM...", "..MMLM...MMLM...", ".MLLLM..MLLLM...", ".MDDDM..MDDDM...",
                    "................", "................", "................", "................"};

// --- Formas genéricas para objetos recoloreables (a = principal, b = brillo, c = sombra) ---
const Rows INGOT = {"................", "................", "................", "................",
                    "................", "......bbbbbb....", ".....babbbbac...", "....baaaaaaacc..",
                    "...baaaaaaaac...", "..cccccccccc....", "................", "................",
                    "................", "................", "................", "................"};
const Rows GEM = {"................", "................", ".....cccccc.....", "....cbbbbbbc....",
                  "...cbbaaaaabc...", "..cbaaaaaaaaac..", "..caaaaaaaaaac..", "...caaaaaaaac...",
                  "....caaaaaac....", ".....caaaac.....", "......caac......", ".......cc.......",
                  "................", "................", "................", "................"};
const Rows DUST = {"................", "................", "................", "................",
                   "................", "................", ".......b........", "......bab.......",
                   ".....aab.a......", "....a.aaaab.....", "...aabaaacaa....", "..acaaacaaaaa...",
                   "..cccccccccc....", "................", "................", "................"};
const Rows ROD = {"................", "............cb..", "...........cab..", "..........cab...",
                  ".........cab....", "........cab.....", ".......cab......", "......cab.......",
                  ".....cab........", "....cab.........", "...cab..........", "..cab...........",
                  "..cb............", "................", "................", "................"};
const Rows ORB = {"................", "................", "................", "......cccc......",
                  ".....cbbaac.....", "....cbbaaaac....", "....cbaaaaac....", "....caaaaaac....",
                  "....caaaaacc....", ".....caaacc.....", "......cccc......", "................",
                  "................", "................", "................", "................"};
const Rows LUMP = {"................", "................", "................", "................",
                   "......ccc.......", "....ccbaacc.....", "...cbaaaaaac....", "...caaacaaaac...",
                   "..caaaaaaacac...", "..cacaaaaaaac...", "...caaacaaac....", "....ccaaacc.....",
                   "......ccc.......", "................", "................", "................"};
const Rows BOTTLE = {"................", "......kkkk......", "......kwwk......", ".......gg.......",
                     ".......gg.......", "......g..g......", ".....g....g.....", "....gaaaaaag....",
                     "....gaaaabag....", "....gaaaaaag....", "....gaaaaaag....", ".....gaaaag.....",
                     "......gggg......", "................", "................", "................"};
const Rows BOOK = {"................", "................", "...ccccccccc....", "..cbbbbbbbbbc...",
                   "..caaaaaaaaac...", "..caaaaaaaaacw..", "..caaaaaaaaacw..", "..caaaaaaaaacw..",
                   "..caaaaaaaaacw..", "..caaaaaaaaacw..", "..caaaaaaaaacw..", "..cccccccccccw..",
                   "...wwwwwwwwwww..", "................", "................", "................"};
const Rows KEY = {"................", "................", "..........bbb...", ".........baaab..",
                  ".........a...a..", ".........a...a..", ".........caaac..", "........cac.....",
                  ".......cac......", "......cac.......", ".....cac........", "....cacc........",
                  "...cac.c........", "....c...........", "................", "................"};

struct SpriteDef {
    std::string_view id;
    int tile;
};

// Asignación objeto -> tesela de sprite
const SpriteDef ITEM_SPRITES[] = {
    {"opencraft:stick", 96},
    {"opencraft:coal", 97},
    {"opencraft:raw_iron", 98},
    {"opencraft:iron_ingot", 99},
    {"opencraft:diamond", 100},
    {"opencraft:bucket", 101},
    {"opencraft:water_bucket", 102},
    {"opencraft:lava_bucket", 103},
    {"opencraft:raw_beef", 104},
    {"opencraft:cooked_beef", 105},
    {"opencraft:apple", 106},
    {"opencraft:bread", 107},
    {"opencraft:bed", 108},
    {"opencraft:wooden_pickaxe", 109},
    {"opencraft:wooden_axe", 110},
    {"opencraft:wooden_shovel", 111},
    {"opencraft:wooden_sword", 112},
    {"opencraft:wooden_hoe", 113},
    {"opencraft:stone_pickaxe", 114},
    {"opencraft:stone_axe", 115},
    {"opencraft:stone_shovel", 116},
    {"opencraft:stone_sword", 117},
    {"opencraft:stone_hoe", 118},
    {"opencraft:iron_pickaxe", 119},
    {"opencraft:iron_axe", 120},
    {"opencraft:iron_shovel", 121},
    {"opencraft:iron_sword", 122},
    {"opencraft:iron_hoe", 123},
    {"opencraft:diamond_pickaxe", 124},
    {"opencraft:diamond_axe", 125},
    {"opencraft:diamond_shovel", 126},
    {"opencraft:diamond_sword", 127},
    {"opencraft:diamond_hoe", 128},
    {"opencraft:rotten_flesh", 129},
    {"opencraft:bone", 130},
    {"opencraft:arrow", 131},
    {"opencraft:gunpowder", 132},
    {"opencraft:leather", 133},
    {"opencraft:bow", 134},
    {"opencraft:shield", 135},
    {"opencraft:iron_helmet", 136},
    {"opencraft:iron_chestplate", 137},
    {"opencraft:iron_leggings", 138},
    {"opencraft:iron_boots", 139},
    {"opencraft:diamond_helmet", 140},
    {"opencraft:diamond_chestplate", 141},
    {"opencraft:diamond_leggings", 142},
    {"opencraft:diamond_boots", 143},
    {"opencraft:wheat_seeds", 144},
    {"opencraft:wheat", 145},
    {"opencraft:carrot", 146},
    {"opencraft:potato", 147},
    {"opencraft:baked_potato", 148},
    {"opencraft:emerald", 149},
    {"opencraft:gold_ingot", 150},
    {"opencraft:fishing_rod", 151},
    {"opencraft:cod", 152},
    {"opencraft:cooked_cod", 153},
    {"opencraft:salmon", 154},
    {"opencraft:saddle", 155},
    {"opencraft:boat", 156},
    {"opencraft:compass", 157},
    {"opencraft:clock", 158},
    {"opencraft:cinnabar", 159},
    {"opencraft:minecart", 160},
    {"opencraft:chest_minecart", 161},
    {"opencraft:hopper_minecart", 162},
    {"opencraft:enchanted_book", 163},
    {"opencraft:orichalcum_upgrade_smithing_template", 164},
    {"opencraft:orichalcum_ingot", 165},
    {"opencraft:orichalcum_scrap", 166},
    {"opencraft:orichalcum_sword", 167},
    {"opencraft:orichalcum_pickaxe", 168},
    {"opencraft:orichalcum_chestplate", 169},
    {"opencraft:ember_rod", 170},
    {"opencraft:ember_powder", 171},
    {"opencraft:crimson_fungus", 172},
    {"opencraft:potion", 173},
    {"opencraft:glass_bottle", 174},
    {"opencraft:rim_pearl", 175},
    {"opencraft:rim_eye", 176},
    {"opencraft:elytra", 177},
    {"opencraft:abyss_star", 178},
    {"opencraft:mace", 179},
    {"opencraft:whirl_rod", 180},
    {"opencraft:wind_charge", 181},
    {"opencraft:trial_key", 182},
    {"opencraft:ominous_trial_key", 183},
    {"opencraft:heavy_core", 184},
    {"opencraft:red_dye", 185},
    {"opencraft:blue_dye", 186},
    {"opencraft:writable_book", 187},
    {"opencraft:map", 188},
    {"opencraft:brush", 189},
    {"opencraft:flint", 190},
    {"opencraft:flint_and_steel", 191},
    {"opencraft:water_bottle", 192},
    {"opencraft:iron_door", 193},
    {"opencraft:oak_door", 194},
    {"opencraft:repeater", 195},
    {"opencraft:comparator", 196},
    {"opencraft:oak_sign", 197},
};

void paintBlocks(Canvas& c) {
    const RGBA dirt = rgb(134, 96, 67);
    // 0-2 hierba y tierra
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            int v = static_cast<int>(hash3(static_cast<uint32_t>(x), static_cast<uint32_t>(y), 11) % 31) - 15;
            c.set(0, x, y, {98 + v / 2, 160 + v, 60 + v / 3, 255});
        }
    c.noise(2, dirt, 12, 22);
    c.copy(1, 2);
    for (int x = 0; x < 16; ++x) {
        int depth = 3 + static_cast<int>(hash3(static_cast<uint32_t>(x), 0, 33) % 3);
        for (int y = 0; y < depth; ++y) c.set(1, x, y, c.get(0, x, y));
    }
    // 3 piedra, 4 adoquín
    c.noise(3, rgb(125, 125, 125), 10, 44);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            uint32_t h = hash3(static_cast<uint32_t>(x / 4 + (y / 4) * 7), 5, 55);
            int base = 100 + static_cast<int>(h % 40);
            bool mortar = (x % 4 == 0 && (y + (x / 4) * 2) % 3 == 0) || (y % 4 == 0 && (x + y) % 3 == 0);
            int v = static_cast<int>(hash3(static_cast<uint32_t>(x), static_cast<uint32_t>(y), 56) % 11) - 5;
            int g = mortar ? 70 : base + v;
            c.set(4, x, y, {g, g, g, 255});
        }
    // 5 corteza, 6 anillos del tronco, 7 tablones
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            int v = static_cast<int>(hash3(static_cast<uint32_t>(x), static_cast<uint32_t>(y / 3), 66) % 25) - 12;
            bool groove = (x % 4 == 1) && (hash3(static_cast<uint32_t>(x), static_cast<uint32_t>(y), 67) % 4 != 0);
            c.set(5, x, y, groove ? rgb(70 + v / 2, 52 + v / 2, 30) : rgb(104 + v, 80 + v, 48 + v / 2));
            float d = std::sqrt(static_cast<float>((x - 7.5f) * (x - 7.5f) + (y - 7.5f) * (y - 7.5f)));
            bool ring = static_cast<int>(d) % 3 == 0;
            bool bark = x == 0 || y == 0 || x == 15 || y == 15;
            c.set(6, x, y, bark ? rgb(104, 80, 48) : (ring ? rgb(150, 115, 68) : rgb(176, 138, 84)));
            bool plankLine = (y % 4 == 3);
            bool joint = (x == ((y / 4) % 2 ? 4 : 11)) && !plankLine;
            int pv = static_cast<int>(hash3(static_cast<uint32_t>(x), static_cast<uint32_t>(y), 77) % 15) - 7;
            c.set(7, x, y, (plankLine || joint) ? rgb(120, 90, 52) : rgb(176 + pv, 138 + pv, 84 + pv / 2));
        }
    // 8 hojas (recortadas)
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            uint32_t h = hash3(static_cast<uint32_t>(x), static_cast<uint32_t>(y), 88);
            if (h % 6 == 0) {
                c.set(8, x, y, {0, 0, 0, 0});
            } else {
                int v = static_cast<int>(h % 40) - 20;
                c.set(8, x, y, {48 + v / 3, 130 + v, 40 + v / 3, 255});
            }
        }
    // 9 arena, 10 cristal, 11 lecho de roca, 12 agua
    c.noise(9, rgb(219, 207, 160), 9, 99);
    c.clear(10);
    c.frame(10, rgb(220, 240, 250, 230));
    for (int i = 2; i < 6; ++i) c.set(10, i, 7 - i, rgb(240, 250, 255, 200));
    for (int i = 9; i < 12; ++i) c.set(10, i, 20 - i, rgb(240, 250, 255, 200));
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            uint32_t h = hash3(static_cast<uint32_t>(x), static_cast<uint32_t>(y), 111);
            int g = 40 + static_cast<int>(h % 70);
            c.set(11, x, y, {g, g, g, 255});
            int v = static_cast<int>(hash3(static_cast<uint32_t>(x + y), static_cast<uint32_t>(y), 120) % 18) - 9;
            c.set(12, x, y, {40 + v, 90 + v, 210 + v, 175});
        }
    // 13 mesa de trabajo (superior), 14 horno (frente)
    c.copy(13, 7);
    c.frame(13, rgb(110, 80, 45));
    for (int i = 1; i < 15; ++i) {
        c.set(13, i, 7, rgb(110, 80, 45));
        c.set(13, 7, i, rgb(110, 80, 45));
    }
    c.copy(14, 4);
    c.rect(14, 3, 8, 12, 13, rgb(30, 28, 26));
    c.rect(14, 4, 9, 11, 12, rgb(55, 45, 40));
    c.rect(14, 2, 2, 13, 3, rgb(90, 90, 90));
    // 15-16 menas de carbón y hierro
    c.copy(15, 3);
    c.specks(15, rgb(35, 35, 35), rgb(20, 20, 20), 6, 151);
    c.copy(16, 3);
    c.specks(16, rgb(216, 175, 147), rgb(170, 125, 100), 5, 161);
    // 17 cama (superior)
    c.noise(17, rgb(170, 30, 30), 8, 171);
    c.rect(17, 2, 1, 13, 5, rgb(235, 235, 235));
    c.rect(17, 0, 0, 15, 0, rgb(120, 85, 50));
    // 18 nieve, 19 trigo (cultivo)
    c.noise(18, rgb(245, 250, 255), 4, 181);
    c.clear(19);
    for (int x = 1; x < 16; x += 3) {
        int top = 3 + static_cast<int>(hash3(static_cast<uint32_t>(x), 0, 191) % 4);
        for (int y = top; y < 16; ++y) c.set(19, x, y, y < top + 3 ? rgb(200, 180, 70) : rgb(110, 150, 40));
        c.set(19, x - 1, top + 1, rgb(220, 200, 90));
        c.set(19, x + 1, top + 2, rgb(220, 200, 90));
    }
    // 20 lava, 21 obsidiana
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            float w = std::sin(static_cast<float>(x) * 0.8f + static_cast<float>(y) * 0.5f) +
                      std::sin(static_cast<float>(y) * 1.1f - static_cast<float>(x) * 0.3f);
            int t = static_cast<int>((w + 2.0f) * 30.0f);
            c.set(20, x, y, {220 + t / 4, 70 + t, 10 + t / 6, 255});
            uint32_t h = hash3(static_cast<uint32_t>(x), static_cast<uint32_t>(y), 211);
            c.set(21, x, y,
                  h % 9 == 0 ? rgb(70, 45, 100)
                             : rgb(20 + static_cast<int>(h % 12), 14, 30 + static_cast<int>(h % 14)));
        }
    // 22 antorcha, 23 fuego
    c.clear(22);
    c.rect(22, 7, 6, 8, 15, rgb(110, 80, 40));
    c.rect(22, 7, 4, 8, 5, rgb(255, 200, 60));
    c.set(22, 7, 3, rgb(255, 240, 150));
    c.set(22, 8, 3, rgb(255, 150, 30));
    c.clear(23);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            uint32_t h = hash3(static_cast<uint32_t>(x), static_cast<uint32_t>(y), 231);
            int height = 6 + static_cast<int>(hash3(static_cast<uint32_t>(x), 0, 232) % 8);
            if (16 - y <= height && h % 5 != 0) {
                int heat = (16 - y) * 255 / std::max(1, height);
                c.set(23, x, y, {255, 230 - heat / 2, 40, 230});
            }
        }
    // 24 arenisca, 25 grava, 26 mena de oro, 27 diamante, 28 esmeralda, 29 adoquín musgoso
    c.noise(24, rgb(216, 203, 155), 6, 241);
    for (int x = 0; x < 16; ++x) {
        c.set(24, x, 3, rgb(190, 175, 125));
        c.set(24, x, 11, rgb(190, 175, 125));
    }
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            uint32_t h = hash3(static_cast<uint32_t>(x / 2), static_cast<uint32_t>(y / 2), 251);
            int g = 100 + static_cast<int>(h % 60);
            c.set(25, x, y, {g + 10, g, g - 5, 255});
        }
    c.copy(26, 3);
    c.specks(26, rgb(250, 220, 70), rgb(200, 160, 30), 5, 261);
    c.copy(27, 3);
    c.specks(27, rgb(90, 230, 220), rgb(40, 160, 160), 5, 271);
    c.copy(28, 3);
    c.specks(28, rgb(60, 220, 110), rgb(20, 150, 60), 4, 281);
    c.copy(29, 4);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
            if (hash3(static_cast<uint32_t>(x), static_cast<uint32_t>(y), 291) % 3 == 0)
                c.set(29, x, y, rgb(80, 120, 60));
    // 30 generador de monstruos (jaula), 31 compostador, 32 atril
    c.clear(30);
    for (int i = 0; i < 16; ++i)
        for (int j = 0; j < 16; ++j)
            if (i % 4 == 0 || j % 4 == 0 || i == 15 || j == 15) c.set(30, i, j, rgb(40, 50, 60));
    c.copy(31, 7);
    c.rect(31, 3, 3, 12, 12, rgb(90, 60, 30));
    c.copy(32, 7);
    c.rect(32, 2, 2, 13, 6, rgb(230, 225, 200));
    // 33 polvo de cinabrio, 34 bloque, 35/36 antorchas de cinabrio
    c.clear(33);
    for (int i = 0; i < 16; ++i) {
        c.set(33, i, 7, rgb(200, 20, 20));
        c.set(33, i, 8, rgb(140, 10, 10));
        c.set(33, 7, i, rgb(200, 20, 20));
        c.set(33, 8, i, rgb(140, 10, 10));
    }
    c.noise(34, rgb(175, 25, 15), 15, 341);
    c.frame(34, rgb(120, 15, 10));
    c.copy(35, 22);
    c.rect(35, 7, 3, 8, 5, rgb(255, 40, 30));
    c.copy(36, 22);
    c.rect(36, 7, 3, 8, 5, rgb(90, 30, 30));
    // 37 repetidor, 38 comparador (losa de piedra con antorchas)
    c.noise(37, rgb(160, 160, 160), 6, 371);
    c.rect(37, 2, 7, 13, 8, rgb(150, 30, 20));
    c.rect(37, 4, 6, 5, 9, rgb(255, 50, 30));
    c.rect(37, 10, 6, 11, 9, rgb(255, 50, 30));
    c.copy(38, 37);
    c.rect(38, 7, 2, 8, 4, rgb(255, 50, 30));
    // 39 palanca, 40 botón de piedra, 41 botón de madera, 42 placa de presión
    c.clear(39);
    c.rect(39, 4, 11, 11, 15, rgb(125, 125, 125));
    for (int i = 0; i < 7; ++i) c.set(39, 6 + i / 2, 10 - i, rgb(110, 80, 40));
    c.copy(40, 3);
    c.copy(41, 7);
    c.copy(42, 3);
    c.frame(42, rgb(95, 95, 95));
    // 43 sensor de luz solar, 44/45 lámparas de cinabrio
    c.copy(43, 7);
    c.rect(43, 1, 1, 14, 14, rgb(200, 200, 160));
    for (int i = 1; i < 15; i += 3) c.rect(43, i, 1, i, 14, rgb(120, 110, 80));
    c.noise(44, rgb(110, 70, 40), 10, 441);
    c.frame(44, rgb(70, 45, 25));
    c.noise(45, rgb(250, 210, 120), 15, 451);
    c.frame(45, rgb(180, 120, 60));
    // 46 puerta de hierro, 47 puerta de roble
    c.noise(46, rgb(200, 200, 200), 6, 461);
    c.frame(46, rgb(130, 130, 130));
    c.rect(46, 4, 3, 11, 6, rgb(140, 140, 140));
    c.copy(47, 7);
    c.frame(47, rgb(100, 70, 35));
    c.rect(47, 4, 3, 11, 6, rgb(120, 90, 50));
    // 48 pistón, 49 pistón pegajoso, 50 cabeza de pistón
    c.copy(48, 4);
    c.rect(48, 0, 0, 15, 3, rgb(176, 138, 84));
    c.rect(48, 6, 4, 9, 15, rgb(160, 160, 160));
    c.copy(49, 48);
    c.rect(49, 0, 0, 15, 3, rgb(120, 190, 90));
    c.copy(50, 7);
    c.rect(50, 6, 6, 9, 9, rgb(160, 160, 160));
    // 51 tolva, 52 soltador, 53 dispensador, 54 fabricador
    c.noise(51, rgb(70, 70, 75), 6, 511);
    c.frame(51, rgb(45, 45, 48), 2);
    c.copy(52, 4);
    c.rect(52, 5, 5, 10, 10, rgb(30, 30, 30));
    c.copy(53, 4);
    c.rect(53, 5, 5, 10, 10, rgb(30, 30, 30));
    c.rect(53, 7, 7, 8, 8, rgb(120, 120, 120));
    c.copy(54, 7);
    c.rect(54, 3, 3, 12, 12, rgb(90, 90, 95));
    for (int i = 3; i <= 12; i += 3) {
        c.rect(54, i, 3, i, 12, rgb(60, 60, 64));
        c.rect(54, 3, i, 12, i, rgb(60, 60, 64));
    }
    // 55-58 raíles
    auto rail = [&](int tile, RGBA accent) {
        c.clear(tile);
        for (int y = 0; y < 16; y += 3) c.rect(tile, 1, y, 14, y + 1, rgb(110, 80, 45));
        for (int y = 0; y < 16; ++y) {
            c.set(tile, 3, y, rgb(170, 170, 170));
            c.set(tile, 12, y, rgb(170, 170, 170));
        }
        if (accent.a > 0)
            for (int y = 0; y < 16; y += 2) {
                c.set(tile, 4, y, accent);
                c.set(tile, 11, y, accent);
            }
    };
    rail(55, {0, 0, 0, 0});
    rail(56, rgb(230, 190, 50));
    rail(57, rgb(150, 30, 20));
    rail(58, rgb(200, 60, 60));
    // 59-65 Abismo y Fin
    c.noise(59, rgb(110, 50, 50), 18, 591);
    c.noise(60, rgb(85, 64, 50), 12, 601);
    for (int i = 0; i < 4; ++i) {
        int fx = 2 + i * 4;
        c.set(60, fx, 6, rgb(40, 28, 20));
        c.set(60, fx + 1, 6, rgb(40, 28, 20));
        c.set(60, fx, 9, rgb(40, 28, 20));
    }
    c.noise(61, rgb(75, 58, 46), 10, 611);
    c.copy(62, 59);
    c.specks(62, rgb(235, 230, 220), rgb(200, 190, 180), 5, 621);
    c.noise(63, rgb(220, 180, 100), 30, 631);
    c.noise(64, rgb(95, 70, 60), 10, 641);
    c.frame(64, rgb(70, 55, 50));
    c.rect(64, 5, 5, 10, 10, rgb(130, 100, 85));
    c.noise(65, rgb(220, 222, 160), 10, 651);
    // 66 soporte de pociones, 67 faro, 68 portal del Abismo, 69 marco del portal del Confín, 70 portal del Confín, 71
    // huevo de dragón
    c.clear(66);
    c.rect(66, 7, 2, 8, 14, rgb(150, 120, 60));
    c.rect(66, 3, 13, 12, 14, rgb(110, 110, 110));
    c.noise(67, rgb(120, 220, 230), 15, 671);
    c.frame(67, rgb(220, 240, 245));
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            float w = std::sin(static_cast<float>(x + y) * 0.7f) * 0.5f + 0.5f;
            c.set(68, x, y, {110 + static_cast<int>(w * 60), 30, 200 + static_cast<int>(w * 40), 200});
            int v = static_cast<int>(hash3(static_cast<uint32_t>(x), static_cast<uint32_t>(y), 701) % 30);
            c.set(70, x, y, {10 + v / 3, 20 + v / 2, 30 + v, 255});
        }
    c.copy(69, 65);
    c.rect(69, 4, 4, 11, 11, rgb(50, 110, 90));
    c.rect(69, 6, 6, 9, 9, rgb(30, 70, 60));
    c.noise(71, rgb(25, 10, 35), 8, 711);
    c.specks(71, rgb(130, 40, 160), rgb(90, 20, 120), 5, 712);
    // 72-74 bloques de comandos
    auto cmd = [&](int tile, RGBA base) {
        c.noise(tile, base, 8, static_cast<uint32_t>(tile * 13));
        c.frame(tile, {base.r - 40, base.g - 40, base.b - 40, 255});
        c.rect(tile, 5, 5, 10, 10, rgb(60, 60, 60));
        c.rect(tile, 6, 6, 9, 9, rgb(200, 200, 200));
    };
    cmd(72, rgb(190, 130, 90));
    cmd(73, rgb(120, 90, 200));
    cmd(74, rgb(110, 170, 140));
    // 75-79 cobre
    c.noise(75, rgb(195, 108, 76), 10, 751);
    c.bricks(76, rgb(190, 104, 74), rgb(150, 80, 55), 8, 8, 761);
    c.copy(77, 75);
    c.frame(77, rgb(150, 80, 55), 2);
    c.rect(77, 5, 5, 10, 10, rgb(150, 80, 55));
    c.clear(78);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
            if (x % 4 == 0 || y % 4 == 0) c.set(78, x, y, rgb(195, 108, 76));
    c.copy(79, 75);
    c.rect(79, 4, 4, 11, 11, rgb(250, 220, 140));
    // 80-82 toba
    c.noise(80, rgb(108, 109, 102), 8, 801);
    c.bricks(81, rgb(110, 112, 104), rgb(80, 82, 76), 8, 8, 811);
    c.copy(82, 80);
    c.frame(82, rgb(80, 82, 76), 2);
    c.rect(82, 6, 6, 9, 9, rgb(80, 82, 76));
    // 83 generador de desafío, 84 bóveda, 85 cartel, 86 vasija decorada
    c.clear(83);
    for (int i = 0; i < 16; ++i)
        for (int j = 0; j < 16; ++j)
            if (i % 5 == 0 || j % 5 == 0 || i == 15 || j == 15) c.set(83, i, j, rgb(160, 90, 60));
    c.rect(83, 6, 6, 9, 9, rgb(255, 160, 60));
    c.noise(84, rgb(60, 70, 80), 6, 841);
    c.frame(84, rgb(190, 110, 70), 2);
    c.rect(84, 6, 6, 9, 9, rgb(120, 200, 220));
    c.copy(85, 7);
    for (int y = 4; y < 12; y += 2) c.rect(85, 3, y, 12, y, rgb(60, 40, 20));
    c.noise(86, rgb(150, 85, 60), 8, 861);
    c.rect(86, 3, 4, 12, 5, rgb(110, 60, 40));
    c.rect(86, 3, 10, 12, 11, rgb(110, 60, 40));

    // Teselas nuevas 87-95
    c.noise(TILE_ICE, rgb(150, 190, 245, 200), 10, 871);
    for (int i = 2; i < 14; i += 5) c.set(TILE_ICE, i, i / 2 + 3, rgb(230, 245, 255, 220));
    c.noise(TILE_FARMLAND_TOP, rgb(92, 60, 38), 8, 881);
    for (int y = 1; y < 16; y += 4) c.rect(TILE_FARMLAND_TOP, 0, y, 15, y, rgb(60, 38, 22));
    c.clear(TILE_CARROTS);
    c.clear(TILE_POTATOES);
    for (int x = 2; x < 16; x += 4) {
        for (int y = 6; y < 16; ++y) {
            c.set(TILE_CARROTS, x, y, rgb(60, 150, 40));
            c.set(TILE_POTATOES, x, y, rgb(70, 140, 50));
        }
        c.set(TILE_CARROTS, x - 1, 7, rgb(80, 180, 50));
        c.set(TILE_CARROTS, x + 1, 9, rgb(80, 180, 50));
        c.rect(TILE_CARROTS, x - 1, 13, x, 15, rgb(240, 130, 30));
        c.set(TILE_POTATOES, x - 1, 8, rgb(90, 160, 60));
        c.rect(TILE_POTATOES, x, 13, x + 1, 15, rgb(200, 170, 90));
    }
    c.copy(TILE_CRAFTING_SIDE, 7);
    c.rect(TILE_CRAFTING_SIDE, 0, 0, 15, 2, rgb(110, 80, 45));
    c.rect(TILE_CRAFTING_SIDE, 3, 5, 5, 11, rgb(140, 140, 140)); // sierra
    c.rect(TILE_CRAFTING_SIDE, 9, 5, 12, 6, rgb(140, 140, 140)); // martillo
    c.rect(TILE_CRAFTING_SIDE, 10, 7, 10, 12, rgb(100, 70, 40));
    c.copy(TILE_CHEST_TOP, 7);
    c.frame(TILE_CHEST_TOP, rgb(80, 55, 25));
    c.copy(TILE_CHEST_FRONT, 7);
    c.frame(TILE_CHEST_FRONT, rgb(80, 55, 25));
    c.rect(TILE_CHEST_FRONT, 0, 5, 15, 5, rgb(80, 55, 25));
    c.rect(TILE_CHEST_FRONT, 7, 4, 8, 7, rgb(200, 200, 200));
    c.copy(TILE_BED_SIDE, 7);
    c.rect(TILE_BED_SIDE, 0, 0, 15, 6, rgb(170, 30, 30));
    c.rect(TILE_BED_SIDE, 0, 7, 15, 8, rgb(235, 235, 235));
    c.copy(TILE_FURNACE_SIDE, 4);
    c.frame(TILE_FURNACE_SIDE, rgb(90, 90, 90));
}

void paintItems(Canvas& c) {
    const RGBA wood = rgb(150, 110, 60), woodL = rgb(190, 145, 85), woodD = rgb(105, 75, 40);
    const RGBA stone = rgb(128, 128, 128), stoneL = rgb(165, 165, 165), stoneD = rgb(85, 85, 85);
    const RGBA iron = rgb(205, 205, 205), ironL = rgb(245, 245, 245), ironD = rgb(140, 140, 140);
    const RGBA gold = rgb(240, 205, 60), goldL = rgb(255, 245, 140), goldD = rgb(185, 140, 30);
    const RGBA dia = rgb(70, 220, 210), diaL = rgb(170, 255, 245), diaD = rgb(30, 150, 145);
    const RGBA neth = rgb(75, 65, 70), nethL = rgb(115, 105, 110), nethD = rgb(40, 35, 38);

    auto tools = [&](int base, const Palette& p) {
        c.sprite(base, PICKAXE, p);
        c.sprite(base + 1, AXE, p);
        c.sprite(base + 2, SHOVEL, p);
        c.sprite(base + 3, SWORD, p);
        c.sprite(base + 4, HOE, p);
    };
    tools(109, material(wood, woodL, woodD));
    tools(114, material(stone, stoneL, stoneD));
    tools(119, material(iron, ironL, ironD));
    tools(124, material(dia, diaL, diaD));
    auto armor = [&](int base, const Palette& p) {
        c.sprite(base, HELMET, p);
        c.sprite(base + 1, CHESTPLATE, p);
        c.sprite(base + 2, LEGGINGS, p);
        c.sprite(base + 3, BOOTS, p);
    };
    armor(136, material(iron, ironL, ironD));
    armor(140, material(dia, diaL, diaD));
    c.sprite(167, SWORD, material(neth, nethL, nethD));
    c.sprite(168, PICKAXE, material(neth, nethL, nethD));
    c.sprite(169, CHESTPLATE, material(neth, nethL, nethD));

    auto recolor = [&](int tile, const Rows& shape, RGBA a, RGBA b, RGBA cc) {
        c.sprite(tile, shape,
                 {{'a', a},
                  {'b', b},
                  {'c', cc},
                  {'k', rgb(60, 45, 30)},
                  {'w', rgb(235, 230, 215)},
                  {'g', rgb(200, 225, 240, 220)}});
    };
    recolor(96, ROD, rgb(140, 100, 55), rgb(175, 130, 75), rgb(90, 65, 35));
    recolor(97, LUMP, rgb(45, 45, 45), rgb(85, 85, 85), rgb(20, 20, 20));
    recolor(98, LUMP, rgb(200, 160, 130), rgb(230, 200, 175), rgb(140, 105, 80));
    recolor(99, INGOT, iron, ironL, ironD);
    recolor(100, GEM, dia, diaL, diaD);
    recolor(129, LUMP, rgb(150, 90, 60), rgb(190, 120, 80), rgb(90, 120, 50));
    recolor(132, DUST, rgb(110, 110, 110), rgb(160, 160, 160), rgb(60, 60, 60));
    recolor(133, LUMP, rgb(160, 90, 50), rgb(195, 120, 70), rgb(110, 60, 30));
    recolor(149, GEM, rgb(60, 200, 100), rgb(150, 250, 170), rgb(20, 130, 60));
    recolor(150, INGOT, gold, goldL, goldD);
    recolor(159, DUST, rgb(200, 20, 15), rgb(255, 80, 60), rgb(120, 10, 5));
    recolor(165, INGOT, neth, nethL, nethD);
    recolor(166, LUMP, rgb(95, 70, 60), rgb(130, 100, 85), rgb(60, 45, 40));
    recolor(170, ROD, rgb(240, 180, 40), rgb(255, 240, 120), rgb(190, 110, 20));
    recolor(171, DUST, rgb(240, 160, 30), rgb(255, 230, 100), rgb(180, 90, 10));
    recolor(172, LUMP, rgb(170, 30, 40), rgb(210, 70, 80), rgb(110, 15, 25));
    recolor(173, BOTTLE, rgb(220, 40, 90), rgb(255, 140, 180), rgb(150, 20, 60));
    c.sprite(174, BOTTLE, {{'g', rgb(200, 225, 240, 220)}, {'k', rgb(110, 80, 50)}, {'w', rgb(200, 225, 240, 220)}});
    recolor(175, ORB, rgb(30, 110, 100), rgb(80, 190, 170), rgb(10, 50, 45));
    c.sprite(176,
             {"................", "................", ".......ii.......", "......iaai......", ".....iabbai.....",
              "....iabbbcai....", "...iabbbbccai...", "...iabbbccccai..", "....iabccccai...", ".....iaccai.....",
              "......iaai......", ".......ii.......", "................", "................", "................",
              "................"},
             {{'i', rgb(150, 110, 60)}, {'a', rgb(70, 40, 120)}, {'b', rgb(190, 150, 255)}, {'c', rgb(120, 80, 200)}});
    recolor(178, GEM, rgb(240, 240, 220), rgb(255, 255, 255), rgb(190, 190, 160));
    recolor(180, ROD, rgb(120, 200, 230), rgb(200, 240, 255), rgb(60, 120, 160));
    recolor(181, ORB, rgb(170, 220, 240), rgb(240, 250, 255), rgb(100, 150, 190));
    recolor(182, KEY, rgb(200, 120, 70), rgb(240, 170, 110), rgb(130, 70, 40));
    recolor(183, KEY, rgb(60, 90, 110), rgb(120, 170, 190), rgb(30, 50, 60));
    recolor(184, ORB, rgb(70, 75, 85), rgb(120, 125, 135), rgb(35, 38, 45));
    recolor(185, DUST, rgb(200, 40, 40), rgb(255, 110, 100), rgb(130, 20, 20));
    recolor(186, DUST, rgb(50, 70, 200), rgb(120, 140, 255), rgb(25, 35, 130));
    recolor(163, BOOK, rgb(130, 60, 160), rgb(200, 140, 230), rgb(80, 30, 110));
    recolor(187, BOOK, rgb(130, 80, 40), rgb(170, 110, 60), rgb(80, 50, 25));
    recolor(190, LUMP, rgb(60, 60, 65), rgb(110, 110, 115), rgb(30, 30, 32));
    recolor(192, BOTTLE, rgb(60, 110, 220), rgb(140, 180, 255), rgb(30, 60, 150));
    recolor(164, INGOT, rgb(80, 75, 90), rgb(140, 120, 160), rgb(40, 35, 45));

    // Sprites específicos
    Palette bucketP{{'i', iron}, {'l', ironL}, {'d', ironD}, {'w', rgb(50, 100, 220)}, {'v', rgb(240, 100, 20)}};
    Rows bucket = {"................", "................", "................", "....dddddddd....",
                   "...d........d...", "...dllllllllld..", "...diiiiiiiiid..", "....diiiiiiid...",
                   "....diiiiiiid...", "....diiiiiiid...", ".....diiiiid....", ".....dddddd.....",
                   "................", "................", "................", "................"};
    c.sprite(101, bucket, bucketP);
    Rows waterBucket = bucket;
    waterBucket[4] = "...dwwwwwwwwd...";
    c.sprite(102, waterBucket, bucketP);
    Rows lavaBucket = bucket;
    lavaBucket[4] = "...dvvvvvvvvd...";
    c.sprite(103, lavaBucket, bucketP);

    Palette meat{
        {'r', rgb(200, 60, 60)}, {'p', rgb(240, 140, 140)}, {'w', rgb(240, 230, 220)}, {'d', rgb(130, 30, 30)}};
    Rows beef = {"................", "................", "................", ".....dddd.......",
                 "....drrrrdd.....", "...drrprrrrd....", "...drprrrrrrd...", "..drrrrrrprrd...",
                 "..drrrrrrrrrd...", "..drrprrrrrd....", "...drrrrrrd.....", "....ddrrrdww....",
                 "......ddd.ww....", "................", "................", "................"};
    c.sprite(104, beef, meat);
    c.sprite(105, beef,
             {{'r', rgb(130, 75, 45)}, {'p', rgb(170, 110, 70)}, {'w', rgb(240, 230, 220)}, {'d', rgb(70, 40, 20)}});
    c.sprite(106,
             {"................", ".......k........", "........k.......", "......gkgg......", "....rrrkrrrr....",
              "...rrwrrrrrrr...", "...rwrrrrrrrr...", "...rrrrrrrrrr...", "...rrrrrrrrrr...", "...rrrrrrrrrr...",
              "....rrrrrrrr....", ".....rr..rr.....", "................", "................", "................",
              "................"},
             {{'r', rgb(210, 25, 35)}, {'w', rgb(255, 160, 160)}, {'k', rgb(90, 60, 30)}, {'g', rgb(60, 160, 40)}});
    c.sprite(107,
             {"................", "................", "................", "................", "......bbbbb.....",
              "....bbaaaaabb...", "...baaacaaacab..", "..baacaaacaaab..", "..baaaaaaaaab...", "...bbbbbbbbb....",
              "................", "................", "................", "................", "................",
              "................"},
             {{'a', rgb(200, 150, 70)}, {'b', rgb(150, 100, 40)}, {'c', rgb(230, 190, 110)}});
    c.sprite(108,
             {"................", "................", "................", "................", "................",
              "..wwwrrrrrrrrrr.", ".wwwwrrrrrrrrrr.", ".pppppppppppppp.", ".p............p.", ".p............p.",
              "................", "................", "................", "................", "................",
              "................"},
             {{'w', rgb(235, 235, 235)}, {'r', rgb(180, 30, 30)}, {'p', rgb(150, 110, 60)}});
    c.sprite(130,
             {"................", "............ww..", "...........wwww.", "..........wwww..", ".........ww.....",
              "........ww......", ".......ww.......", "......ww........", ".....ww.........", "....ww..........",
              "..wwww..........", ".wwww...........", "..ww............", "................", "................",
              "................"},
             {{'w', rgb(235, 230, 210)}});
    c.sprite(131,
             {"................", "...........fff..", "............ff..", "...........f.f..", "..........k.....",
              ".........k......", "........k.......", ".......k........", "......k.........", ".....k..........",
              "....k...........", "..ww............", "..www...........", "...w............", "................",
              "................"},
             {{'f', rgb(200, 200, 200)}, {'k', rgb(120, 90, 50)}, {'w', rgb(240, 240, 240)}});
    c.sprite(134,
             {"................", "........kkkk....", "......kk...s....", ".....k....s.....", "....k....s......",
              "....k...s.......", "...k...s........", "...k..s.........", "...k.s..........", "....s...........",
              "...s.k..........", "..s..k..........", ".s...k..........", "....k...........", "..kk............",
              "................"},
             {{'k', rgb(130, 90, 45)}, {'s', rgb(230, 230, 230)}});
    c.sprite(135,
             {"................", "...dddddddddd...", "...dwwwwwwwwd...", "...dwwwwwwwwd...", "...dwwiiiwwwd...",
              "...dwwiiiiwwd...", "...dwwwiiwwwd...", "...dwwwwwwwwd...", "...dwwwwwwwwd...", "....dwwwwwwd....",
              ".....dwwwwd.....", "......dddd......", "................", "................", "................",
              "................"},
             {{'d', rgb(120, 120, 120)}, {'w', rgb(150, 110, 60)}, {'i', rgb(200, 200, 200)}});
    c.sprite(144,
             {"................", "................", "................", "................", "......g...g.....",
              "....g...g...g...", "......g..g......", "...g...g....g...", ".....g...g......", "...g..g..g..g...",
              "................", "................", "................", "................", "................",
              "................"},
             {{'g', rgb(70, 160, 40)}});
    c.sprite(145,
             {"................", "........y.......", ".......yy.y.....", "......y.yy......", ".....yyy.y......",
              "....y.yy........", "...yyy..........", "..g.y...........", ".g..............", "g...............",
              "................", "................", "................", "................", "................",
              "................"},
             {{'y', rgb(220, 190, 80)}, {'g', rgb(120, 150, 40)}});
    c.sprite(146,
             {"................", "...........gg...", "..........ggg.g.", "...........gg...", "..........oo....",
              ".........ooo....", "........ooo.....", ".......ooo......", "......ooo.......", ".....ooo........",
              "....oo..........", "...oo...........", "................", "................", "................",
              "................"},
             {{'o', rgb(240, 130, 30)}, {'g', rgb(60, 160, 40)}});
    Palette potatoP{{'p', rgb(200, 165, 90)}, {'d', rgb(150, 115, 55)}};
    Rows potato = {"................", "................", "................", "................",
                   ".....dddd.......", "....dppppdd.....", "...dpppdppppd...", "...dppppppppd...",
                   "...dpppppdppd...", "....dppppppd....", ".....ddddd......", "................",
                   "................", "................", "................", "................"};
    c.sprite(147, potato, potatoP);
    c.sprite(148, potato, {{'p', rgb(215, 160, 70)}, {'d', rgb(140, 90, 35)}});
    Palette fishP{{'f', rgb(190, 160, 110)}, {'d', rgb(130, 100, 60)}, {'e', rgb(20, 20, 20)}};
    Rows fish = {"................", "................", "................", "................",
                 ".......dd.......", "....dddffd....d.", "...dfffffffd.dd.", "..dfefffffffdfd.",
                 "...dfffffffd.dd.", "....dddddfd...d.", "................", "................",
                 "................", "................", "................", "................"};
    c.sprite(152, fish, fishP);
    c.sprite(153, fish, {{'f', rgb(220, 190, 140)}, {'d', rgb(150, 110, 70)}, {'e', rgb(20, 20, 20)}});
    c.sprite(154, fish, {{'f', rgb(200, 90, 70)}, {'d', rgb(120, 50, 40)}, {'e', rgb(20, 20, 20)}});
    c.sprite(151,
             {"................", "...........kk...", "..........k..s..", ".........k...s..", "........k....s..",
              ".......k.....s..", "......k......s..", ".....k.......s..", "....k........s..", "...k.........r..",
              "..k.............", ".k..............", "................", "................", "................",
              "................"},
             {{'k', rgb(130, 90, 45)}, {'s', rgb(220, 220, 220)}, {'r', rgb(200, 30, 30)}});
    c.sprite(155,
             {"................", "................", "................", "......bbbb......", "....bbbbbbbb....",
              "...bbbbbbbbbb...", "...bb.bbbb.bb...", "...b..bbbb..b...", "......bbbb......", "......i..i......",
              "................", "................", "................", "................", "................",
              "................"},
             {{'b', rgb(120, 70, 35)}, {'i', rgb(190, 190, 190)}});
    c.sprite(156,
             {"................", "................", "................", "................", "................",
              ".k............k.", ".kk..........kk.", ".kwwwwwwwwwwwwk.", "..kwwwwwwwwwwk..", "...kkkkkkkkkk...",
              "................", "................", "................", "................", "................",
              "................"},
             {{'k', rgb(100, 70, 35)}, {'w', rgb(160, 120, 70)}});
    c.sprite(157,
             {"................", "................", ".....iiiiii.....", "....iwwwwwwi....", "...iwwwrwwwwi...",
              "...iwwwrwwwwi...", "...iwwwrwwwwi...", "...iwwwkwwwwi...", "...iwwwwwwwwi...", "...iwwwwwwwwi...",
              "....iwwwwwwi....", ".....iiiiii.....", "................", "................", "................",
              "................"},
             {{'i', rgb(150, 150, 150)}, {'w', rgb(220, 220, 220)}, {'r', rgb(200, 30, 30)}, {'k', rgb(60, 60, 60)}});
    c.sprite(158,
             {"................", "................", ".....gggggg.....", "....gyyyyyyg....", "...gyyyyyyyyg...",
              "...gyyykyyyyg...", "...gyyykyyyyg...", "...gyyykkkyyg...", "...gyyyyyyyyg...", "...gbbbbbbbbg...",
              "....gbbbbbbg....", ".....gggggg.....", "................", "................", "................",
              "................"},
             {{'g', rgb(200, 160, 40)}, {'y', rgb(250, 240, 180)}, {'b', rgb(40, 50, 90)}, {'k', rgb(40, 40, 40)}});
    Palette cartP{{'i', rgb(140, 140, 145)},
                  {'d', rgb(80, 80, 85)},
                  {'w', rgb(40, 40, 40)},
                  {'c', rgb(150, 110, 60)},
                  {'h', rgb(70, 70, 75)}};
    Rows cart = {"................", "................", "................", "................",
                 "................", ".iiiiiiiiiiiiii.", ".i............i.", ".i............i.",
                 ".dddddddddddddd.", "..dddddddddddd..", "...ww......ww...", "...ww......ww...",
                 "................", "................", "................", "................"};
    c.sprite(160, cart, cartP);
    Rows chestCart = cart;
    chestCart[2] = "....cccccccc....";
    chestCart[3] = "....cccccccc....";
    chestCart[4] = "....cccccccc....";
    c.sprite(161, chestCart, cartP);
    Rows hopperCart = cart;
    hopperCart[3] = "....hhhhhhhh....";
    hopperCart[4] = ".....hhhhhh.....";
    c.sprite(162, hopperCart, cartP);
    c.sprite(177,
             {"................", "..ppp......ppp..", ".pggpp....ppggp.", ".pgggp....pgggp.", ".pgggpp..ppgggp.",
              ".pggggp..pggggp.", ".pggggp..pggggp.", "..pgggp..pgggp..", "..pgggp..pgggp..", "...pggp..pggp...",
              "....pp....pp....", "................", "................", "................", "................",
              "................"},
             {{'p', rgb(90, 90, 110)}, {'g', rgb(150, 150, 175)}});
    c.sprite(179,
             {"................", "........dddd....", ".......dmmmmd...", ".......dmlmmd...", ".......dmmmmd...",
              "........ddddk...", "...........k....", "..........k.....", ".........k......", "........k.......",
              ".......k........", "......k.........", ".....k..........", "....kk..........", "................",
              "................"},
             {{'d', rgb(60, 60, 70)}, {'m', rgb(120, 120, 130)}, {'l', rgb(200, 200, 210)}, {'k', rgb(110, 80, 40)}});
    c.sprite(188,
             {"................", "................", "...pppppppppp...", "...pwwwwwwwwp...", "...pwgggwwwwp...",
              "...pwgggbbwwp...", "...pwwwgbbbwp...", "...pwwwwbbwwp...", "...pwwwwwwwwp...", "...pppppppppp...",
              "................", "................", "................", "................", "................",
              "................"},
             {{'p', rgb(170, 140, 90)}, {'w', rgb(235, 225, 190)}, {'g', rgb(90, 160, 70)}, {'b', rgb(70, 110, 200)}});
    c.sprite(189,
             {"................", "...........www..", "..........wwww..", "..........www...", ".........kk.....",
              "........kk......", ".......kk.......", "......kk........", ".....kk.........", "....kk..........",
              "...kk...........", "................", "................", "................", "................",
              "................"},
             {{'w', rgb(220, 200, 150)}, {'k', rgb(170, 110, 70)}});
    c.sprite(191,
             {"................", "................", "...ii...........", "..i..i..........", "..i..i..........",
              "...ii.i.........", "......i.........", ".......i........", "........kk......", ".......kkkk.....",
              "........kk......", "................", "................", "................", "................",
              "................"},
             {{'i', rgb(170, 170, 170)}, {'k', rgb(50, 50, 55)}});
    // Objetos de bloques planos (puertas, repetidores y cartel)
    Rows door = {"................", ".....dddddd.....", ".....dmmmmd.....", ".....dmllmd.....",
                 ".....dmmmmd.....", ".....dmmmmd.....", ".....dmmmmd.....", ".....dmmmkd.....",
                 ".....dmmmmd.....", ".....dmmmmd.....", ".....dmmmmd.....", ".....dmmmmd.....",
                 ".....dmmmmd.....", ".....dddddd.....", "................", "................"};
    c.sprite(193, door,
             {{'d', rgb(130, 130, 130)}, {'m', rgb(205, 205, 205)}, {'l', rgb(150, 150, 150)}, {'k', rgb(80, 80, 80)}});
    c.sprite(194, door,
             {{'d', rgb(100, 70, 35)}, {'m', rgb(176, 138, 84)}, {'l', rgb(120, 90, 50)}, {'k', rgb(60, 60, 60)}});
    Rows diode = {"................", "................", "................", "................",
                  "................", "................", "................", "...r......r.....",
                  "...k......k.....", "..ssssssssssss..", "..sddddddddddds.", "..ssssssssssss..",
                  "................", "................", "................", "................"};
    c.sprite(195, diode,
             {{'r', rgb(255, 50, 30)}, {'k', rgb(110, 80, 40)}, {'s', rgb(150, 150, 150)}, {'d', rgb(150, 30, 20)}});
    Rows diode2 = diode;
    diode2[6] = "......r.........";
    diode2[7] = "...r..k...r.....";
    c.sprite(196, diode2,
             {{'r', rgb(255, 50, 30)}, {'k', rgb(110, 80, 40)}, {'s', rgb(150, 150, 150)}, {'d', rgb(150, 30, 20)}});
    c.sprite(197,
             {"................", "..pppppppppppp..", "..pwwwwwwwwwwp..", "..pwkkkwkkkwwp..", "..pwwwwwwwwwwp..",
              "..pwkkwkkkkwwp..", "..pwwwwwwwwwwp..", "..pppppppppppp..", ".......pp.......", ".......pp.......",
              ".......pp.......", ".......pp.......", "................", "................", "................",
              "................"},
             {{'p', rgb(120, 90, 50)}, {'w', rgb(176, 138, 84)}, {'k', rgb(60, 40, 20)}});
}

} // namespace

std::vector<uint8_t> paint() {
    std::vector<uint8_t> pixels(static_cast<size_t>(ATLAS_SIZE) * ATLAS_SIZE * 4, 0);
    Canvas c(pixels);
    paintBlocks(c);
    paintItems(c);
    for (int y = 0; y < TILE_SIZE; ++y)
        for (int x = 0; x < TILE_SIZE; ++x) c.set(TILE_WHITE, x, y, rgb(255, 255, 255));

    // Grietas de rotura: 10 etapas acumulativas (cada etapa añade ramas a la anterior)
    c.clear(TILE_CRACK_FIRST);
    for (int stage = 0; stage < 10; ++stage) {
        int tile = TILE_CRACK_FIRST + stage;
        if (stage > 0) c.copy(tile, tile - 1);
        for (int branch = 0; branch < 2; ++branch) {
            uint32_t seed = static_cast<uint32_t>(stage * 7 + branch * 3 + 901);
            int x = 8 + static_cast<int>(hash3(seed, 1, 5) % 5) - 2;
            int y = 8 + static_cast<int>(hash3(seed, 2, 5) % 5) - 2;
            int len = 3 + stage / 2;
            for (int i = 0; i < len; ++i) {
                c.set(tile, x, y, rgb(0, 0, 0, 190));
                uint32_t d = hash3(seed, static_cast<uint32_t>(i + 3), 7) % 8;
                x += (d % 3) - 1;
                y += (d / 3 % 3) - 1;
                x = std::clamp(x, 0, 15);
                y = std::clamp(y, 0, 15);
            }
        }
    }
    return pixels;
}

int itemIconTile(std::string_view namespacedId) {
    for (const auto& s : ITEM_SPRITES) {
        if (s.id == namespacedId) return s.tile;
    }
    return -1;
}

} // namespace OpenCraft::AtlasPainter

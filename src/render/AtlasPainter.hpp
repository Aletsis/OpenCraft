#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace OpenCraft {

// Generación procedural del atlas de 16x16 teselas (256x256 RGBA) sin dependencias gráficas.
// Las teselas 0-95 son texturas de bloque (índices de BlockRegistry::textureIndices) y las 96-255
// son sprites de objetos (ItemDefinition::iconTileIndex).
namespace AtlasPainter {

constexpr int TILES_PER_ROW = 16;
constexpr int TILE_SIZE = 16;
constexpr int ATLAS_SIZE = TILES_PER_ROW * TILE_SIZE;

// Teselas de bloque añadidas para separar texturas que compartían índice
constexpr int TILE_ICE = 87;
constexpr int TILE_FARMLAND_TOP = 88;
constexpr int TILE_CARROTS = 89;
constexpr int TILE_POTATOES = 90;
constexpr int TILE_CRAFTING_SIDE = 91;
constexpr int TILE_CHEST_TOP = 92;
constexpr int TILE_CHEST_FRONT = 93;
constexpr int TILE_BED_SIDE = 94;
constexpr int TILE_FURNACE_SIDE = 95;
constexpr int TILE_CRACK_FIRST = 240; // 240-249: etapas de rotura
constexpr int TILE_WHITE = 255;       // blanco liso para geometría coloreada (entidades)

std::vector<uint8_t> paint();

// Tesela de sprite para un objeto que no es un bloque; -1 si no tiene sprite propio
int itemIconTile(std::string_view namespacedId);

} // namespace AtlasPainter

} // namespace OpenCraft

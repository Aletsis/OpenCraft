#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace OpenCraft {

// Fuente de mapa de bits 5x7 (más una fila de descendente) con ASCII imprimible y los caracteres
// del español. Cada glifo son 8 filas de 5 bits (bit 4 = columna izquierda).
struct Glyph {
    std::array<uint8_t, 8> rows{};
    uint8_t advance = 6; // ancho del glifo más un píxel de separación
};

namespace BitmapFont {

constexpr int GLYPH_HEIGHT = 8;
constexpr int LINE_HEIGHT = 10;

// Decodifica UTF-8 a puntos de código; los bytes inválidos se sustituyen por '?'
std::vector<char32_t> decodeUtf8(std::string_view text);

const Glyph& glyphFor(char32_t codepoint);

// Ancho en píxeles de fuente (multiplicar por la escala para obtener píxeles de pantalla)
int measure(std::string_view text);

} // namespace BitmapFont

} // namespace OpenCraft

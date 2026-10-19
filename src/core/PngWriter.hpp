#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace OpenCraft {

// Codificador PNG mínimo (RGBA 8 bits, deflate sin compresión) sin dependencias externas.
// Suficiente para capturas de pantalla y evidencias de pruebas visuales.
namespace PngWriter {

// Codifica una imagen RGBA con filas de arriba hacia abajo
std::vector<uint8_t> encodeRGBA(std::span<const uint8_t> rgba, uint32_t width, uint32_t height);

// Escribe la imagen en disco; crea los directorios intermedios si faltan
bool writeRGBA(const std::filesystem::path& path, std::span<const uint8_t> rgba, uint32_t width, uint32_t height);

} // namespace PngWriter

} // namespace OpenCraft

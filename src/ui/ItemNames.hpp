#pragma once

#include <string>
#include <cstdint>

namespace OpenCraft {

// Nombre visible en español de un objeto o bloque (cae al nombre del registro si no hay traducción)
std::string localizedItemName(uint16_t itemId);

} // namespace OpenCraft

#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace OpenCraft {

// Identificadores renombrados en 1.0.0-rc1 (nombres propios en lugar de términos de terceros).
// Los mundos, comandos y paquetes que usan el identificador anterior se resuelven al nuevo.
namespace IdAliases {

const std::vector<std::pair<std::string_view, std::string_view>>& table();

// Devuelve el identificador vigente (el propio si no está renombrado)
std::string canonical(std::string_view namespacedId);

} // namespace IdAliases

} // namespace OpenCraft

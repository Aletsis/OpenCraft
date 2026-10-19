#pragma once

#include "math/Math.hpp"
#include "world/World.hpp"
#include "world/Dimension.hpp"
#include "sim/Entity.hpp"

namespace OpenCraft {

class PortalSystem {
public:
    // Ticks que el jugador debe permanecer en un portal del Abismo (modo Supervivencia/Aventura)
    static constexpr uint32_t ABYSS_PORTAL_WAIT_TICKS = 80;
    // Enfriamiento tras una transferencia para evitar rebotes entre portales
    static constexpr uint32_t PORTAL_COOLDOWN_TICKS = 100;

    // Detección y activación del marco de portal del Abismo
    static bool detectAndActivateAbyssPortal(World& world, const BlockPos& ignitionPos);

    // Mapeo canónico de coordenadas entre Overworld y Abismo (escala 8:1)
    static Math::Vec3 mapCoordinates(DimensionType from, DimensionType to, const Math::Vec3& pos);

    // Búsqueda de portal de destino seguro en radio de 128 bloques, o construcción de nuevo portal
    static Math::Vec3 findOrCreateAbyssPortalDestination(World& targetWorld, const Math::Vec3& targetPos);

    // Inserción de Ojo del Confín y activación del Portal del Confín
    static bool insertRimEye(World& world, const BlockPos& framePos);

    // Genera la plataforma fija de obsidiana del Confín en (100, 48, 0)
    static Math::Vec3 setupEndSpawnPlatform(World& endWorld);

    // Actualiza dimensión, posición y estado de portal del jugador. El intercambio del mundo activo
    // lo realiza WorldSession::changeDimension(), que es la ruta que deben usar los llamadores.
    static bool transferPlayer(PlayerEntity& player, DimensionType targetDim, const Math::Vec3& targetPos);
};

} // namespace OpenCraft

#include "world/PortalSystem.hpp"
#include <cmath>
#include <algorithm>

namespace OpenCraft {

bool PortalSystem::detectAndActivateAbyssPortal(World& world, const BlockPos& ignitionPos) {
    // Marco estándar orientado en eje X o Z: interior de 2x3 libre rodeado de obsidiana
    // (las esquinas del marco son opcionales). ignitionPos debe ser una celda del interior.
    constexpr int INNER_W = 2;
    constexpr int INNER_H = 3;

    auto isOpen = [&](int x, int y, int z) {
        BlockID b = world.getBlock(x, y, z);
        return b == BlockID::Air || b == BlockID::Fire;
    };
    auto isObsidian = [&](int x, int y, int z) { return world.getBlock(x, y, z) == BlockID::Obsidian; };

    auto tryFrame = [&](int dx, int dz) -> bool {
        for (int wOff = 0; wOff < INNER_W; ++wOff) {
            for (int hOff = 0; hOff < INNER_H; ++hOff) {
                int ox = ignitionPos.x - wOff * dx;
                int oz = ignitionPos.z - wOff * dz;
                int oy = ignitionPos.y - hOff;

                bool valid = true;
                for (int w = 0; w < INNER_W && valid; ++w) {
                    int px = ox + w * dx;
                    int pz = oz + w * dz;
                    valid = isObsidian(px, oy - 1, pz) && isObsidian(px, oy + INNER_H, pz);
                    for (int h = 0; h < INNER_H && valid; ++h) {
                        valid = isOpen(px, oy + h, pz);
                    }
                }
                for (int h = 0; h < INNER_H && valid; ++h) {
                    valid = isObsidian(ox - dx, oy + h, oz - dz) &&
                            isObsidian(ox + INNER_W * dx, oy + h, oz + INNER_W * dz);
                }
                if (!valid) continue;

                for (int h = 0; h < INNER_H; ++h) {
                    for (int w = 0; w < INNER_W; ++w) {
                        world.setBlock(ox + w * dx, oy + h, oz + w * dz, BlockID::AbyssPortal);
                    }
                }
                return true;
            }
        }
        return false;
    };

    if (!isOpen(ignitionPos.x, ignitionPos.y, ignitionPos.z)) return false;
    return tryFrame(1, 0) || tryFrame(0, 1);
}

Math::Vec3 PortalSystem::mapCoordinates(DimensionType from, DimensionType to, const Math::Vec3& pos) {
    if (from == DimensionType::Overworld && to == DimensionType::Abyss) {
        return Math::Vec3(pos.x / 8.0f, pos.y, pos.z / 8.0f);
    }
    if (from == DimensionType::Abyss && to == DimensionType::Overworld) {
        return Math::Vec3(pos.x * 8.0f, pos.y, pos.z * 8.0f);
    }
    return pos;
}

Math::Vec3 PortalSystem::findOrCreateAbyssPortalDestination(World& targetWorld, const Math::Vec3& targetPos) {
    int targetX = static_cast<int>(std::floor(targetPos.x));
    int targetY = static_cast<int>(std::floor(targetPos.y));
    int targetZ = static_cast<int>(std::floor(targetPos.z));

    // 1. Buscar portal existente en radio de 128 bloques
    int searchRadius = 128;
    int bestDistSq = 128 * 128 + 1;
    Math::Vec3 foundPos{0, 0, 0};
    bool found = false;

    for (int dz = -searchRadius; dz <= searchRadius; dz += 4) {
        for (int dx = -searchRadius; dx <= searchRadius; dx += 4) {
            int cx = (targetX + dx) >> 4;
            int cz = (targetZ + dz) >> 4;
            if (!targetWorld.isChunkLoaded(cx, cz)) {
                // Solo chunks cargados o disponibles
                continue;
            }
            for (int y = 30; y < 110; ++y) {
                if (targetWorld.getBlock(targetX + dx, y, targetZ + dz) == BlockID::AbyssPortal) {
                    int distSq = dx * dx + dz * dz;
                    if (distSq < bestDistSq) {
                        bestDistSq = distSq;
                        foundPos = Math::Vec3(static_cast<float>(targetX + dx) + 0.5f, static_cast<float>(y),
                                              static_cast<float>(targetZ + dz) + 0.5f);
                        found = true;
                    }
                }
            }
        }
    }

    if (found) {
        return foundPos;
    }

    // 2. Si no se encontró ningún portal, construir uno seguro en targetPos
    int spawnY = std::clamp(targetY, 35, 100);
    // Asegurar chunks para el portal
    int pcx = targetX >> 4;
    int pcz = targetZ >> 4;
    if (!targetWorld.isChunkLoaded(pcx, pcz)) {
        auto chk = std::make_unique<Chunk>(pcx, pcz);
        chk->generateTerrain(targetWorld.getSeed(), 3);
        targetWorld.setChunk(pcx, pcz, std::move(chk));
    }

    // Plataforma de 4x4 obsidiana en spawnY - 1
    for (int dz = -1; dz <= 2; ++dz) {
        for (int dx = -1; dx <= 2; ++dx) {
            targetWorld.setBlock(targetX + dx, spawnY - 1, targetZ + dz, BlockID::Obsidian);
        }
    }

    // Marco de portal de obsidiana 4x5 en el eje X
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 4; ++x) {
            bool isBorder = (x == 0 || x == 3 || y == 0 || y == 4);
            if (isBorder) {
                targetWorld.setBlock(targetX + x, spawnY + y, targetZ, BlockID::Obsidian);
            } else {
                targetWorld.setBlock(targetX + x, spawnY + y, targetZ, BlockID::AbyssPortal);
            }
        }
    }

    // Espacio de aire despejado alrededor
    for (int y = 0; y < 4; ++y) {
        for (int x = 1; x <= 2; ++x) {
            targetWorld.setBlock(targetX + x, spawnY + y, targetZ + 1, BlockID::Air);
            targetWorld.setBlock(targetX + x, spawnY + y, targetZ - 1, BlockID::Air);
        }
    }

    // Pies sobre la base del marco (la fila spawnY es obsidiana), dentro del interior del portal
    return Math::Vec3(static_cast<float>(targetX) + 1.5f, static_cast<float>(spawnY + 1),
                      static_cast<float>(targetZ) + 0.5f);
}

bool PortalSystem::insertRimEye(World& world, const BlockPos& framePos) {
    if (world.getBlock(framePos.x, framePos.y, framePos.z) != BlockID::RimPortalFrame) {
        return false;
    }

    // Comprobar si forma un anillo 5x5 completo para activación
    bool fullPortalActivated = false;
    for (int offZ = -2; offZ <= 2; ++offZ) {
        for (int offX = -2; offX <= 2; ++offX) {
            int centerX = framePos.x + offX;
            int centerZ = framePos.z + offZ;
            int y = framePos.y;

            // Comprobar 12 marcos en anillo 5x5
            int frameCount = 0;
            for (int rz = -2; rz <= 2; ++rz) {
                for (int rx = -2; rx <= 2; ++rx) {
                    bool isCorner = (std::abs(rx) == 2 && std::abs(rz) == 2);
                    bool isRing = (std::abs(rx) == 2 || std::abs(rz) == 2) && !isCorner;

                    if (isRing) {
                        if (world.getBlock(centerX + rx, y, centerZ + rz) == BlockID::RimPortalFrame) {
                            frameCount++;
                        }
                    }
                }
            }

            if (frameCount == 12) {
                // Activar portal: llenar interior de 3x3 con EndPortal
                for (int rz = -1; rz <= 1; ++rz) {
                    for (int rx = -1; rx <= 1; ++rx) {
                        world.setBlock(centerX + rx, y, centerZ + rz, BlockID::RimPortal);
                    }
                }
                fullPortalActivated = true;
                break;
            }
        }
        if (fullPortalActivated) break;
    }

    // Insertar el ojo en el marco siempre devuelve éxito si el bloque es un marco
    return true;
}

Math::Vec3 PortalSystem::setupEndSpawnPlatform(World& endWorld) {
    int px = 100;
    int py = 48;
    int pz = 0;

    int cx = px >> 4;
    int cz = pz >> 4;
    if (!endWorld.isChunkLoaded(cx, cz)) {
        auto chk = std::make_unique<Chunk>(cx, cz);
        chk->generateTerrain(endWorld.getSeed(), 3);
        endWorld.setChunk(cx, cz, std::move(chk));
    }

    // Plataforma fija de 5x5 obsidiana en Y=48
    for (int dz = -2; dz <= 2; ++dz) {
        for (int dx = -2; dx <= 2; ++dx) {
            endWorld.setBlock(px + dx, py, pz + dz, BlockID::Obsidian);
            for (int h = 1; h <= 3; ++h) {
                endWorld.setBlock(px + dx, py + h, pz + dz, BlockID::Air);
            }
        }
    }

    return Math::Vec3(100.0f, 49.0f, 0.0f);
}

bool PortalSystem::transferPlayer(PlayerEntity& player, DimensionType targetDim, const Math::Vec3& targetPos) {
    player.dimension = static_cast<uint8_t>(targetDim);
    player.position = targetPos;
    player.prevPosition = targetPos;
    player.velocity = Math::Vec3(0.0f, 0.0f, 0.0f);
    player.fallDistance = 0.0f;
    player.isGrounded = true;
    player.isGliding = false;
    player.portalTicks = 0;
    player.portalCooldown = PORTAL_COOLDOWN_TICKS;
    return true;
}

} // namespace OpenCraft

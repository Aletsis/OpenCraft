#include "world/BeaconBlockEntity.hpp"
#include "world/World.hpp"
#include "sim/Entity.hpp"
#include <cmath>

namespace OpenCraft {

BeaconBlockEntity::BeaconBlockEntity(BlockPos pos) : BlockEntity(pos) {}

bool BeaconBlockEntity::isMineralBlock(BlockID id) noexcept {
    return id == BlockID::CinnabarBlock || id == BlockID::Obsidian || id == BlockID::GoldOre ||
           id == BlockID::DiamondOre || id == BlockID::EmeraldOre || id == BlockID::IronOre;
}

uint32_t BeaconBlockEntity::checkPyramid(const World& world) {
    // 1. Comprobar que no haya bloques opacos bloqueando el cielo sobre la baliza
    for (int y = m_pos.y + 1; y < 320; ++y) {
        BlockID b = world.getBlock(m_pos.x, y, m_pos.z);
        if (b != BlockID::Air && b != BlockID::Glass) {
            m_pyramidLevel = 0;
            m_isActive = false;
            return 0;
        }
    }

    // 2. Comprobar hasta 4 niveles de pirámide hacia abajo
    uint32_t level = 0;
    for (int lvl = 1; lvl <= 4; ++lvl) {
        int py = m_pos.y - lvl;
        int radius = lvl; // radio lvl = capa de (2*lvl + 1) x (2*lvl + 1)
        bool layerComplete = true;

        for (int dz = -radius; dz <= radius && layerComplete; ++dz) {
            for (int dx = -radius; dx <= radius; ++dx) {
                BlockID b = world.getBlock(m_pos.x + dx, py, m_pos.z + dz);
                // Si es un bloque sólido mineral o equivalente de construcción (o oricalco)
                if (b == BlockID::Air) {
                    layerComplete = false;
                    break;
                }
            }
        }

        if (layerComplete) {
            level = lvl;
        } else {
            break;
        }
    }

    m_pyramidLevel = level;
    m_isActive = (level > 0);
    return level;
}

void BeaconBlockEntity::stepBeacon(World& world, std::vector<PlayerEntity*>& players) {
    if (world.isChunkLoaded(m_pos.x >> 4, m_pos.z >> 4)) {
        checkPyramid(world);
    }
    if (!m_isActive || m_pyramidLevel == 0) return;

    ++m_pulseTicks;
    if (m_pulseTicks >= 80) { // Cada 4 segundos (80 ticks)
        m_pulseTicks = 0;
        float radius = static_cast<float>(m_pyramidLevel * 10 + 10);
        uint32_t effectDuration = static_cast<uint32_t>(9 * 20); // 9 segundos

        Math::Vec3 beaconCenter(static_cast<float>(m_pos.x) + 0.5f, static_cast<float>(m_pos.y) + 0.5f,
                                static_cast<float>(m_pos.z) + 0.5f);

        for (auto* player : players) {
            if (!player || !player->isAlive) continue;

            float dist = (player->position - beaconCenter).length();
            if (dist <= radius) {
                // Efecto primario
                player->statusEffects.addEffect(ActiveEffect{
                    .type = m_primaryEffect,
                    .durationTicks = effectDuration,
                    .amplifier =
                        (m_pyramidLevel == 4 && m_secondaryEffect == m_primaryEffect) ? (uint8_t)1 : (uint8_t)0,
                    .isAmbient = true});

                // Efecto secundario si la pirámide es de nivel 4
                if (m_pyramidLevel == 4 && m_secondaryEffect != m_primaryEffect) {
                    player->statusEffects.addEffect(ActiveEffect{
                        .type = m_secondaryEffect, .durationTicks = effectDuration, .amplifier = 0, .isAmbient = true});
                }
            }
        }
    }
}

} // namespace OpenCraft

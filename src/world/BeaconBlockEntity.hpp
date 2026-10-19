#pragma once

#include "world/BlockEntity.hpp"
#include "world/Block.hpp"
#include "progression/StatusEffect.hpp"
#include <vector>

namespace OpenCraft {

class World;
struct PlayerEntity;

class BeaconBlockEntity : public BlockEntity {
public:
    explicit BeaconBlockEntity(BlockPos pos);
    ~BeaconBlockEntity() override = default;

    BlockEntityType getType() const noexcept override { return BlockEntityType::Beacon; }

    uint32_t getRevision() const noexcept { return m_revision; }
    void incrementRevision() noexcept { ++m_revision; }

    uint32_t getPyramidLevel() const noexcept { return m_pyramidLevel; }
    void setPyramidLevel(uint32_t lvl) noexcept {
        m_pyramidLevel = lvl;
        m_isActive = (lvl > 0);
    }
    bool isActive() const noexcept { return m_isActive; }

    EffectType getPrimaryEffect() const noexcept { return m_primaryEffect; }
    void setPrimaryEffect(EffectType eff) noexcept { m_primaryEffect = eff; }

    EffectType getSecondaryEffect() const noexcept { return m_secondaryEffect; }
    void setSecondaryEffect(EffectType eff) noexcept { m_secondaryEffect = eff; }

    // Reevalúa la base piramidal (1 a 4 capas de bloques minerales) y el haz de luz
    uint32_t checkPyramid(const World& world);

    // Irradia los efectos de estado a los jugadores en el radio de acción
    void stepBeacon(World& world, std::vector<PlayerEntity*>& players);

private:
    uint32_t m_revision = 0;
    uint32_t m_pyramidLevel = 0;
    bool m_isActive = false;
    EffectType m_primaryEffect = EffectType::Speed;
    EffectType m_secondaryEffect = EffectType::Regeneration;
    uint32_t m_pulseTicks = 0;

    static bool isMineralBlock(BlockID id) noexcept;
};

} // namespace OpenCraft

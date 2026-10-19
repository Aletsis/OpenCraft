#pragma once

#include "world/BlockEntity.hpp"
#include "item/ItemStack.hpp"
#include "math/Math.hpp"
#include <vector>
#include <cstdint>
#include <unordered_set>

namespace OpenCraft {

enum class TrialSpawnerState : uint8_t {
    Inactive = 0,
    ActiveSpawning = 1,
    WaitingForWaveDefeat = 2,
    EjectingRewards = 3,
    Cooldown = 4
};

class TrialSpawnerBlockEntity : public BlockEntity {
public:
    explicit TrialSpawnerBlockEntity(BlockPos pos) : BlockEntity(pos) {}

    BlockEntityType getType() const noexcept override { return BlockEntityType::TrialSpawner; }

    TrialSpawnerState getState() const noexcept { return m_state; }
    void setState(TrialSpawnerState state) noexcept { m_state = state; }

    bool isOminous() const noexcept { return m_isOminous; }
    void setOminous(bool ominous) noexcept { m_isOminous = ominous; }

    uint32_t getCooldownTicks() const noexcept { return m_cooldownTicks; }
    void setCooldownTicks(uint32_t ticks) noexcept { m_cooldownTicks = ticks; }

    uint32_t getCurrentWave() const noexcept { return m_currentWave; }
    uint32_t getTotalWaves() const noexcept { return m_totalWaves; }
    void setTotalWaves(uint32_t waves) noexcept { m_totalWaves = waves; }

    uint32_t getAliveMobsCount() const noexcept { return static_cast<uint32_t>(m_activeMobIds.size()); }
    void addActiveMob(uint32_t entityId) { m_activeMobIds.insert(entityId); }
    const std::unordered_set<uint32_t>& getActiveMobs() const noexcept { return m_activeMobIds; }
    void onMobDefeated(uint32_t entityId) {
        m_activeMobIds.erase(entityId);
        if (m_activeMobIds.empty() && m_state == TrialSpawnerState::WaitingForWaveDefeat) {
            if (m_currentWave + 1 >= m_totalWaves) {
                m_state = TrialSpawnerState::EjectingRewards;
            } else {
                m_currentWave++;
                m_state = TrialSpawnerState::ActiveSpawning;
            }
        }
    }

    bool isPlayerInRange(const Math::Vec3& playerPos) const noexcept {
        float dx = playerPos.x - (static_cast<float>(m_pos.x) + 0.5f);
        float dy = playerPos.y - (static_cast<float>(m_pos.y) + 0.5f);
        float dz = playerPos.z - (static_cast<float>(m_pos.z) + 0.5f);
        return (dx * dx + dy * dy + dz * dz) <= (14.0f * 14.0f);
    }

    void activate() {
        if (m_state == TrialSpawnerState::Inactive) {
            m_state = TrialSpawnerState::ActiveSpawning;
            m_currentWave = 0;
            m_activeMobIds.clear();
        }
    }

    void resetToCooldown(uint32_t ticks = 36000) {
        m_state = TrialSpawnerState::Cooldown;
        m_cooldownTicks = ticks;
        m_activeMobIds.clear();
        m_currentWave = 0;
    }

    void tick() {
        if (m_state == TrialSpawnerState::Cooldown) {
            if (m_cooldownTicks > 0) {
                m_cooldownTicks--;
                if (m_cooldownTicks == 0) {
                    m_state = TrialSpawnerState::Inactive;
                }
            }
        }
    }

    std::vector<ItemStack> generateRewards() const;

private:
    TrialSpawnerState m_state = TrialSpawnerState::Inactive;
    bool m_isOminous = false;
    uint32_t m_currentWave = 0;
    uint32_t m_totalWaves = 2;
    uint32_t m_cooldownTicks = 0;
    std::unordered_set<uint32_t> m_activeMobIds;
};

} // namespace OpenCraft

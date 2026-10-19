#pragma once

#include "world/BlockEntity.hpp"
#include "item/ItemStack.hpp"
#include <vector>
#include <cstdint>
#include <unordered_set>
#include <string>

namespace OpenCraft {

class VaultBlockEntity : public BlockEntity {
public:
    explicit VaultBlockEntity(BlockPos pos) : BlockEntity(pos) {}

    BlockEntityType getType() const noexcept override { return BlockEntityType::Vault; }

    bool isOminous() const noexcept { return m_isOminous; }
    void setOminous(bool ominous) noexcept { m_isOminous = ominous; }

    bool hasPlayerUnlocked(uint32_t playerId) const noexcept {
        return m_unlockedPlayers.find(playerId) != m_unlockedPlayers.end();
    }

    void markPlayerUnlocked(uint32_t playerId) { m_unlockedPlayers.insert(playerId); }

    const std::unordered_set<uint32_t>& getUnlockedPlayers() const noexcept { return m_unlockedPlayers; }

    void setUnlockedPlayers(std::unordered_set<uint32_t> players) { m_unlockedPlayers = std::move(players); }

    bool tryUnlock(uint32_t playerId, const std::string& keyNamespacedId, std::vector<ItemStack>& outLoot);

private:
    bool m_isOminous = false;
    std::unordered_set<uint32_t> m_unlockedPlayers;
};

} // namespace OpenCraft

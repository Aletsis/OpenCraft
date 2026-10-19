#include "progression/StatisticsManager.hpp"

namespace OpenCraft {

void StatisticsManager::incrementGeneral(const std::string& stat, uint64_t delta) { m_generalStats[stat] += delta; }

uint64_t StatisticsManager::getGeneral(const std::string& stat) const {
    auto it = m_generalStats.find(stat);
    if (it != m_generalStats.end()) {
        return it->second;
    }
    return 0;
}

void StatisticsManager::incrementMined(const std::string& blockId, uint32_t delta) { m_minedStats[blockId] += delta; }

uint32_t StatisticsManager::getMined(const std::string& blockId) const {
    auto it = m_minedStats.find(blockId);
    if (it != m_minedStats.end()) {
        return it->second;
    }
    return 0;
}

void StatisticsManager::incrementCrafted(const std::string& itemId, uint32_t delta) { m_craftedStats[itemId] += delta; }

uint32_t StatisticsManager::getCrafted(const std::string& itemId) const {
    auto it = m_craftedStats.find(itemId);
    if (it != m_craftedStats.end()) {
        return it->second;
    }
    return 0;
}

void StatisticsManager::incrementUsed(const std::string& itemId, uint32_t delta) { m_usedStats[itemId] += delta; }

uint32_t StatisticsManager::getUsed(const std::string& itemId) const {
    auto it = m_usedStats.find(itemId);
    if (it != m_usedStats.end()) {
        return it->second;
    }
    return 0;
}

} // namespace OpenCraft

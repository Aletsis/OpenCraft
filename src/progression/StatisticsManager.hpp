#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

namespace OpenCraft {

class StatisticsManager {
public:
    StatisticsManager() = default;
    ~StatisticsManager() = default;

    void incrementGeneral(const std::string& stat, uint64_t delta = 1);
    uint64_t getGeneral(const std::string& stat) const;

    void incrementMined(const std::string& blockId, uint32_t delta = 1);
    uint32_t getMined(const std::string& blockId) const;

    void incrementCrafted(const std::string& itemId, uint32_t delta = 1);
    uint32_t getCrafted(const std::string& itemId) const;

    void incrementUsed(const std::string& itemId, uint32_t delta = 1);
    uint32_t getUsed(const std::string& itemId) const;

    const std::unordered_map<std::string, uint64_t>& getAllGeneral() const noexcept { return m_generalStats; }
    const std::unordered_map<std::string, uint32_t>& getAllMined() const noexcept { return m_minedStats; }
    const std::unordered_map<std::string, uint32_t>& getAllCrafted() const noexcept { return m_craftedStats; }
    const std::unordered_map<std::string, uint32_t>& getAllUsed() const noexcept { return m_usedStats; }

    void clear() {
        m_generalStats.clear();
        m_minedStats.clear();
        m_craftedStats.clear();
        m_usedStats.clear();
    }

private:
    std::unordered_map<std::string, uint64_t> m_generalStats;
    std::unordered_map<std::string, uint32_t> m_minedStats;
    std::unordered_map<std::string, uint32_t> m_craftedStats;
    std::unordered_map<std::string, uint32_t> m_usedStats;
};

} // namespace OpenCraft

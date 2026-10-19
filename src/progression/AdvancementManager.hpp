#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

namespace OpenCraft {

enum class AdvancementFrame : uint8_t {
    Task = 0,
    Goal = 1,
    Challenge = 2
};

struct Advancement {
    std::string id;
    std::string parent;
    std::string title;
    std::string description;
    std::string iconItem;
    AdvancementFrame frame = AdvancementFrame::Task;
    std::vector<std::string> criteria;
};

class AdvancementManager {
public:
    AdvancementManager();
    ~AdvancementManager() = default;

    void registerAdvancement(Advancement adv);
    bool grantAdvancement(const std::string& id);
    bool revokeAdvancement(const std::string& id);
    bool hasAdvancement(const std::string& id) const;

    const std::unordered_map<std::string, Advancement>& getAllAdvancements() const noexcept { return m_advancements; }

    const std::unordered_set<std::string>& getCompletedAdvancements() const noexcept { return m_completedAdvancements; }

    void loadCompletedAdvancements(const std::vector<std::string>& list);
    std::vector<std::string> exportCompletedAdvancements() const;

    // Disparadores canónicos de simulación
    void onPlayerJoin();
    void onBlockMined(const std::string& blockId);
    void onItemCrafted(const std::string& itemId);
    void onDimensionChanged(uint8_t dimension);
    void onMobKilled(const std::string& mobType);

    void clear() { m_completedAdvancements.clear(); }

private:
    std::unordered_map<std::string, Advancement> m_advancements;
    std::unordered_set<std::string> m_completedAdvancements;

    void initDefaults();
};

} // namespace OpenCraft

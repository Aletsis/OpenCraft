#pragma once

#include <vector>
#include <queue>
#include <cstdint>
#include <algorithm>
#include <unordered_set>
#include "math/Math.hpp"
#include "world/Block.hpp"
#include "world/BlockEntity.hpp"

namespace OpenCraft {

struct ScheduledBlockUpdate {
    BlockPos pos;
    BlockID blockId = BlockID::Air;
    uint64_t targetTick = 0;
    int priority = 0;
    uint64_t sequence = 0;

    // Comparador estricto para orden determinista
    bool operator>(const ScheduledBlockUpdate& o) const noexcept {
        if (targetTick != o.targetTick) return targetTick > o.targetTick;
        if (priority != o.priority) return priority < o.priority;
        if (sequence != o.sequence) return sequence > o.sequence;
        if (pos.y != o.pos.y) return pos.y > o.pos.y;
        if (pos.x != o.pos.x) return pos.x > o.pos.x;
        return pos.z > o.pos.z;
    }
};

class ScheduledUpdateQueue {
public:
    static constexpr size_t DEFAULT_TICK_BUDGET = 500; // Presupuesto máximo por tick

    ScheduledUpdateQueue() = default;
    ~ScheduledUpdateQueue() = default;

    void schedule(BlockPos pos, BlockID blockId, uint64_t targetTick, int priority = 0) {
        // Evitar duplicados idénticos en la misma posición y tick
        uint64_t key = (static_cast<uint64_t>(pos.x) * 73856093ULL) ^ (static_cast<uint64_t>(pos.y) * 19349663ULL) ^
                       (static_cast<uint64_t>(pos.z) * 83492791ULL) ^ (targetTick * 31ULL);
        if (m_scheduledKeys.contains(key)) {
            return;
        }
        m_scheduledKeys.insert(key);

        ScheduledBlockUpdate update{.pos = pos,
                                    .blockId = blockId,
                                    .targetTick = targetTick,
                                    .priority = priority,
                                    .sequence = ++m_sequenceCounter};
        m_queue.push(update);
    }

    size_t size() const noexcept { return m_queue.size(); }

    bool empty() const noexcept { return m_queue.empty(); }

    void clear() noexcept {
        while (!m_queue.empty()) m_queue.pop();
        m_scheduledKeys.clear();
        m_sequenceCounter = 0;
    }

    // Extrae hasta maxBudget actualizaciones listas para ejecutarse (targetTick <= currentTick)
    void pollReady(uint64_t currentTick, size_t maxBudget, std::vector<ScheduledBlockUpdate>& outReady) {
        outReady.clear();
        size_t processed = 0;

        while (!m_queue.empty() && processed < maxBudget) {
            const auto& top = m_queue.top();
            if (top.targetTick > currentTick) {
                break; // Aún no es hora de ejecutar
            }

            outReady.push_back(top);
            uint64_t key = (static_cast<uint64_t>(top.pos.x) * 73856093ULL) ^
                           (static_cast<uint64_t>(top.pos.y) * 19349663ULL) ^
                           (static_cast<uint64_t>(top.pos.z) * 83492791ULL) ^ (top.targetTick * 31ULL);
            m_scheduledKeys.erase(key);
            m_queue.pop();
            ++processed;
        }
    }

    // Exportación e importación para persistencia
    std::vector<ScheduledBlockUpdate> getAllPending() const {
        std::vector<ScheduledBlockUpdate> result;
        auto copy = m_queue;
        while (!copy.empty()) {
            result.push_back(copy.top());
            copy.pop();
        }
        return result;
    }

    void restorePending(const std::vector<ScheduledBlockUpdate>& updates) {
        clear();
        for (const auto& u : updates) {
            schedule(u.pos, u.blockId, u.targetTick, u.priority);
        }
    }

private:
    std::priority_queue<ScheduledBlockUpdate, std::vector<ScheduledBlockUpdate>, std::greater<ScheduledBlockUpdate>>
        m_queue;
    std::unordered_set<uint64_t> m_scheduledKeys;
    uint64_t m_sequenceCounter = 0;
};

} // namespace OpenCraft

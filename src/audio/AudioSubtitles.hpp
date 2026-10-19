#pragma once

#include <string>
#include <vector>
#include "math/Math.hpp"
#include "audio/SpatialAudio.hpp"

namespace OpenCraft {

enum class SubtitleDirection {
    Left,
    Right,
    Center
};

struct SubtitleItem {
    std::string text;
    Math::Vec3 position;
    SubtitleDirection direction = SubtitleDirection::Center;
    float timeRemaining = 3.0f;
    float maxTime = 3.0f;
    AudioCategory category = AudioCategory::Blocks;

    std::string getFormattedText() const {
        if (direction == SubtitleDirection::Left) {
            return "< " + text;
        } else if (direction == SubtitleDirection::Right) {
            return text + " >";
        }
        return "· " + text + " ·";
    }

    float getAlpha() const {
        if (timeRemaining < 0.5f) {
            return timeRemaining / 0.5f;
        }
        return 1.0f;
    }
};

class AudioSubtitleManager {
public:
    AudioSubtitleManager();

    void addSubtitle(const std::string& text, const Math::Vec3& pos, AudioCategory category = AudioCategory::Blocks,
                     float duration = 3.0f);

    void update(float dt, const Math::Vec3& listenerPos, const Math::Vec3& listenerFront, const Math::Vec3& listenerUp);

    const std::vector<SubtitleItem>& getItems() const { return m_items; }
    void clear() { m_items.clear(); }

    void setMaxDisplayCount(size_t count) { m_maxDisplayCount = count; }

private:
    std::vector<SubtitleItem> m_items;
    size_t m_maxDisplayCount = 6;
};

} // namespace OpenCraft

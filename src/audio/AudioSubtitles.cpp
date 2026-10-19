#include "audio/AudioSubtitles.hpp"
#include <algorithm>

namespace OpenCraft {

AudioSubtitleManager::AudioSubtitleManager() = default;

void AudioSubtitleManager::addSubtitle(const std::string& text, const Math::Vec3& pos, AudioCategory category,
                                       float duration) {
    // Si ya existe un subtítulo idéntico muy reciente, reiniciar su duración
    for (auto& item : m_items) {
        if (item.text == text && (item.position - pos).length() < 2.0f) {
            item.timeRemaining = duration;
            item.maxTime = duration;
            item.position = pos;
            return;
        }
    }

    SubtitleItem item;
    item.text = text;
    item.position = pos;
    item.timeRemaining = duration;
    item.maxTime = duration;
    item.category = category;
    item.direction = SubtitleDirection::Center;

    m_items.push_back(item);

    if (m_items.size() > m_maxDisplayCount) {
        m_items.erase(m_items.begin());
    }
}

void AudioSubtitleManager::update(float dt, const Math::Vec3& listenerPos, const Math::Vec3& listenerFront,
                                  const Math::Vec3& listenerUp) {
    Math::Vec3 right = listenerFront.cross(listenerUp).normalized();

    for (auto it = m_items.begin(); it != m_items.end();) {
        it->timeRemaining -= dt;
        if (it->timeRemaining <= 0.0f) {
            it = m_items.erase(it);
            continue;
        }

        // Recalcular dirección angular respecto al oyente
        Math::Vec3 diff = it->position - listenerPos;
        float dist = diff.length();
        if (dist > 0.1f) {
            Math::Vec3 dir = diff.normalized();
            float panDot = dir.dot(right);
            if (panDot < -0.25f) {
                it->direction = SubtitleDirection::Left;
            } else if (panDot > 0.25f) {
                it->direction = SubtitleDirection::Right;
            } else {
                it->direction = SubtitleDirection::Center;
            }
        } else {
            it->direction = SubtitleDirection::Center;
        }

        ++it;
    }
}

} // namespace OpenCraft

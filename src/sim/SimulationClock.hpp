#pragma once

#include <cstdint>
#include <algorithm>

namespace OpenCraft {

class SimulationClock {
public:
    static constexpr double TICK_RATE = 20.0;
    static constexpr double TICK_DURATION = 1.0 / TICK_RATE; // 0.05s = 50ms
    static constexpr double MAX_ACCUMULATOR = 0.25;          // 5 ticks máximo por frame

    SimulationClock() = default;

    void reset() noexcept {
        m_accumulator = 0.0;
        m_tickCount = 0;
    }

    void addTime(double dt) noexcept {
        if (dt < 0.0) return;
        m_accumulator += dt;
        if (m_accumulator > MAX_ACCUMULATOR) {
            m_accumulator = MAX_ACCUMULATOR; // Limita la espiral de muerte
        }
    }

    bool shouldTick() noexcept {
        if (m_accumulator >= TICK_DURATION) {
            m_accumulator -= TICK_DURATION;
            return true;
        }
        return false;
    }

    void advanceTick() noexcept { ++m_tickCount; }

    // Factor de interpolación alfa en [0.0, 1.0) para renderizado suave
    float getInterpolationAlpha() const noexcept {
        return static_cast<float>(std::clamp(m_accumulator / TICK_DURATION, 0.0, 1.0));
    }

    uint64_t getTickCount() const noexcept { return m_tickCount; }
    void setTickCount(uint64_t tick) noexcept { m_tickCount = tick; }
    double getAccumulator() const noexcept { return m_accumulator; }

    // Ciclo día/noche (24000 ticks = 1 día en el juego de referencia)
    uint64_t getTimeOfDay() const noexcept { return m_tickCount % 24000ULL; }

    float getSunlightFactor(float rainFactor = 0.0f) const noexcept {
        uint64_t tod = getTimeOfDay();
        float factor = 1.0f;

        if (tod < 12000) {
            factor = 1.0f; // Día pleno
        } else if (tod < 13800) {
            // Atardecer (12000 a 13800): transición suave de 1.0 a 0.15
            float t = static_cast<float>(tod - 12000) / 1800.0f;
            factor = 1.0f - t * (1.0f - 0.15f);
        } else if (tod < 22200) {
            factor = 0.15f; // Noche
        } else {
            // Amanecer (22200 a 24000): transición suave de 0.15 a 1.0
            float t = static_cast<float>(tod - 22200) / 1800.0f;
            factor = 0.15f + t * (1.0f - 0.15f);
        }

        if (rainFactor > 0.0f) {
            factor *= (1.0f - 0.25f * std::clamp(rainFactor, 0.0f, 1.0f));
        }

        return std::clamp(factor, 0.10f, 1.0f);
    }

private:
    double m_accumulator = 0.0;
    uint64_t m_tickCount = 0;
};

} // namespace OpenCraft

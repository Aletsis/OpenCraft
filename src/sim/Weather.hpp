#pragma once

#include <cstdint>

namespace OpenCraft {

enum class WeatherState : uint8_t {
    Clear = 0,
    Rain,
    Thunder
};

struct WeatherStatus {
    WeatherState state = WeatherState::Clear;
    uint32_t ticksRemaining = 24000;
    float rainStrength = 0.0f; // 0.0f = Despejado, 1.0f = Lluvia/Tormenta activa
};

} // namespace OpenCraft

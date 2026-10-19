#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include "world/Block.hpp"

namespace OpenCraft {

enum class BiomeType : uint8_t {
    Plains = 0,
    Forest,
    Desert,
    Snowy,
    Mountain,
    Ocean,
    Caves,
    Count
};

struct BiomeProperties {
    BiomeType type = BiomeType::Plains;
    std::string name = "Plains";
    float minTemperature = 0.0f;
    float maxTemperature = 1.0f;
    float minHumidity = 0.0f;
    float maxHumidity = 1.0f;
    BlockID surfaceBlock = BlockID::Grass;
    BlockID subSurfaceBlock = BlockID::Dirt;
    BlockID underwaterBlock = BlockID::Sand;
    bool hasPrecipitation = true;
    bool isCold = false;
    float treeDensity = 0.01f;
};

class BiomeRegistry {
public:
    static const BiomeProperties& getProperties(BiomeType type) noexcept;
    static std::string_view getBiomeName(BiomeType type) noexcept;

    // Determina el bioma superficial según parámetros climáticos normalizados [-1.0f, 1.0f]
    static BiomeType determineBiome(float temperature, float humidity, float continentalness, float height) noexcept;
};

} // namespace OpenCraft

#include "world/Biome.hpp"
#include <array>

namespace OpenCraft {

static const std::array<BiomeProperties, static_cast<size_t>(BiomeType::Count)> s_biomeProperties = []() constexpr {
    std::array<BiomeProperties, static_cast<size_t>(BiomeType::Count)> props{};

    props[static_cast<size_t>(BiomeType::Plains)] = BiomeProperties{.type = BiomeType::Plains,
                                                                    .name = "Plains",
                                                                    .minTemperature = -0.2f,
                                                                    .maxTemperature = 0.6f,
                                                                    .minHumidity = -0.3f,
                                                                    .maxHumidity = 0.3f,
                                                                    .surfaceBlock = BlockID::Grass,
                                                                    .subSurfaceBlock = BlockID::Dirt,
                                                                    .underwaterBlock = BlockID::Sand,
                                                                    .hasPrecipitation = true,
                                                                    .isCold = false,
                                                                    .treeDensity = 0.008f};

    props[static_cast<size_t>(BiomeType::Forest)] = BiomeProperties{.type = BiomeType::Forest,
                                                                    .name = "Forest",
                                                                    .minTemperature = 0.0f,
                                                                    .maxTemperature = 0.7f,
                                                                    .minHumidity = 0.2f,
                                                                    .maxHumidity = 1.0f,
                                                                    .surfaceBlock = BlockID::Grass,
                                                                    .subSurfaceBlock = BlockID::Dirt,
                                                                    .underwaterBlock = BlockID::Sand,
                                                                    .hasPrecipitation = true,
                                                                    .isCold = false,
                                                                    .treeDensity = 0.035f};

    props[static_cast<size_t>(BiomeType::Desert)] = BiomeProperties{.type = BiomeType::Desert,
                                                                    .name = "Desert",
                                                                    .minTemperature = 0.45f,
                                                                    .maxTemperature = 1.0f,
                                                                    .minHumidity = -1.0f,
                                                                    .maxHumidity = -0.1f,
                                                                    .surfaceBlock = BlockID::Sand,
                                                                    .subSurfaceBlock = BlockID::Sandstone,
                                                                    .underwaterBlock = BlockID::Sand,
                                                                    .hasPrecipitation = false,
                                                                    .isCold = false,
                                                                    .treeDensity = 0.0f};

    props[static_cast<size_t>(BiomeType::Snowy)] = BiomeProperties{.type = BiomeType::Snowy,
                                                                   .name = "Snowy",
                                                                   .minTemperature = -1.0f,
                                                                   .maxTemperature = -0.2f,
                                                                   .minHumidity = -0.5f,
                                                                   .maxHumidity = 0.8f,
                                                                   .surfaceBlock = BlockID::SnowBlock,
                                                                   .subSurfaceBlock = BlockID::Dirt,
                                                                   .underwaterBlock = BlockID::Gravel,
                                                                   .hasPrecipitation = true,
                                                                   .isCold = true,
                                                                   .treeDensity = 0.015f};

    props[static_cast<size_t>(BiomeType::Mountain)] = BiomeProperties{.type = BiomeType::Mountain,
                                                                      .name = "Mountain",
                                                                      .minTemperature = -0.8f,
                                                                      .maxTemperature = 0.3f,
                                                                      .minHumidity = -0.5f,
                                                                      .maxHumidity = 0.5f,
                                                                      .surfaceBlock = BlockID::Stone,
                                                                      .subSurfaceBlock = BlockID::Stone,
                                                                      .underwaterBlock = BlockID::Gravel,
                                                                      .hasPrecipitation = true,
                                                                      .isCold = true,
                                                                      .treeDensity = 0.004f};

    props[static_cast<size_t>(BiomeType::Ocean)] = BiomeProperties{.type = BiomeType::Ocean,
                                                                   .name = "Ocean",
                                                                   .minTemperature = -0.5f,
                                                                   .maxTemperature = 0.8f,
                                                                   .minHumidity = -0.5f,
                                                                   .maxHumidity = 1.0f,
                                                                   .surfaceBlock = BlockID::Gravel,
                                                                   .subSurfaceBlock = BlockID::Gravel,
                                                                   .underwaterBlock = BlockID::Gravel,
                                                                   .hasPrecipitation = true,
                                                                   .isCold = false,
                                                                   .treeDensity = 0.0f};

    props[static_cast<size_t>(BiomeType::Caves)] = BiomeProperties{.type = BiomeType::Caves,
                                                                   .name = "Caves",
                                                                   .minTemperature = 0.0f,
                                                                   .maxTemperature = 0.0f,
                                                                   .minHumidity = 0.0f,
                                                                   .maxHumidity = 0.0f,
                                                                   .surfaceBlock = BlockID::Stone,
                                                                   .subSurfaceBlock = BlockID::Stone,
                                                                   .underwaterBlock = BlockID::Gravel,
                                                                   .hasPrecipitation = false,
                                                                   .isCold = false,
                                                                   .treeDensity = 0.0f};

    return props;
}();

const BiomeProperties& BiomeRegistry::getProperties(BiomeType type) noexcept {
    size_t idx = static_cast<size_t>(type);
    if (idx >= static_cast<size_t>(BiomeType::Count)) {
        return s_biomeProperties[0];
    }
    return s_biomeProperties[idx];
}

std::string_view BiomeRegistry::getBiomeName(BiomeType type) noexcept { return getProperties(type).name; }

BiomeType BiomeRegistry::determineBiome(float temperature, float humidity, float continentalness,
                                        float height) noexcept {
    if (continentalness < -0.22f && height < 58.0f) {
        return BiomeType::Ocean;
    }
    if (height > 92.0f) {
        return BiomeType::Mountain;
    }
    if (temperature < -0.2f) {
        return BiomeType::Snowy;
    }
    if (temperature > 0.40f && humidity < -0.1f) {
        return BiomeType::Desert;
    }
    if (humidity > 0.22f) {
        return BiomeType::Forest;
    }
    return BiomeType::Plains;
}

} // namespace OpenCraft

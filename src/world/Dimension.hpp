#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include "world/World.hpp"

namespace OpenCraft {

enum class DimensionType : uint8_t {
    Overworld = 0,
    Abyss = 1,
    Rim = 2
};

const char* dimensionTypeToString(DimensionType dim) noexcept;
DimensionType dimensionTypeFromString(std::string_view name) noexcept;

struct ChunkKey {
    DimensionType dimension = DimensionType::Overworld;
    int x = 0;
    int z = 0;

    bool operator==(const ChunkKey& o) const noexcept { return dimension == o.dimension && x == o.x && z == o.z; }
};

struct ChunkKeyHash {
    size_t operator()(const ChunkKey& k) const noexcept {
        return (static_cast<size_t>(k.dimension) * 1000003) ^ (static_cast<size_t>(k.x) * 73856093) ^
               (static_cast<size_t>(k.z) * 19349663);
    }
};

class Dimension {
public:
    Dimension(DimensionType type, uint64_t seed);
    ~Dimension() = default;

    DimensionType getType() const noexcept { return m_type; }
    World& getWorld() noexcept { return m_world; }
    const World& getWorld() const noexcept { return m_world; }

    // Genera terreno procedural canónico para la dimensión
    void generateChunk(int cx, int cz);

    // Genera el terreno de un chunk según el generador canónico de la dimensión indicada
    static void generateTerrain(DimensionType type, Chunk& chunk, uint64_t seed, uint32_t generatorVersion);

private:
    DimensionType m_type;
    uint64_t m_seed;
    World m_world;

    static void generateAbyssChunk(Chunk& chunk, int cx, int cz);
    static void generateEndChunk(Chunk& chunk, int cx, int cz);
};

} // namespace OpenCraft

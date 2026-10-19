#pragma once

#include <cstdint>

namespace OpenCraft {

class Chunk;

class WorldGenerator {
public:
    static constexpr uint32_t CURRENT_GENERATOR_VERSION = 3;

    WorldGenerator() = default;
    ~WorldGenerator() = default;

    // Genera el contenido completo de un chunk de forma estrictamente determinista
    static void generateChunk(int chunkX, int chunkZ, Chunk& chunk, uint64_t seed,
                              uint32_t version = CURRENT_GENERATOR_VERSION);

    // Funciones matemáticas y de ruido deterministas sin dependencias de plataforma
    static uint64_t hashCoords(int64_t x, int64_t z, uint64_t seed) noexcept;
    static uint64_t hash3D(int64_t x, int64_t y, int64_t z, uint64_t seed) noexcept;

    // Ruido 2D normalizado [-1.0f, 1.0f] con gradientes enteros deterministas
    static float gradientNoise2D(float x, float z, uint64_t seed) noexcept;

    // Ruido Fractal Browniano (FBM) multi-octava
    static float octaveNoise2D(float x, float z, uint64_t seed, int octaves, float persistence,
                               float lacunarity) noexcept;

private:
    static float smoothstep(float t) noexcept { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

    static float dotGradient(int ix, int iz, float x, float z, uint64_t seed) noexcept;
};

} // namespace OpenCraft

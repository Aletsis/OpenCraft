#pragma once

#include <cstdint>

namespace OpenCraft {

// Números pseudoaleatorios para la simulación. A diferencia de rand(), producen la misma secuencia en
// todas las plataformas y bibliotecas estándar (glibc, macOS, MSVC), de modo que una partida con la misma
// semilla y las mismas órdenes evoluciona igual en cualquier sistema.
class DeterministicRandom {
public:
    explicit DeterministicRandom(uint64_t seed = 0x9E3779B97F4A7C15ULL) noexcept : m_state(seed) {}

    void seed(uint64_t s) noexcept { m_state = s; }

    // SplitMix64
    uint64_t next() noexcept { return mix(m_state += 0x9E3779B97F4A7C15ULL); }

    // Entero uniforme en [0, bound) (bound > 0)
    uint32_t nextBelow(uint32_t bound) noexcept { return static_cast<uint32_t>(next() % bound); }

    static constexpr uint64_t mix(uint64_t z) noexcept {
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    // Valor sin estado a partir de varios enteros (p. ej. id de entidad, tick y propósito)
    static constexpr uint32_t hashBelow(uint32_t bound, uint64_t a, uint64_t b = 0, uint64_t c = 0) noexcept {
        uint64_t h = mix(a + 0x9E3779B97F4A7C15ULL);
        h = mix(h ^ (b + 0x632BE59BD9B4E019ULL));
        h = mix(h ^ (c + 0x8CB92BA72F3D8DD7ULL));
        return static_cast<uint32_t>(h % bound);
    }

private:
    uint64_t m_state;
};

} // namespace OpenCraft

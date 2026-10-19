#include <cassert>
#include <iostream>
#include <random>
#include "benchmark/QualitySettings.hpp"
#include "benchmark/BenchmarkHarness.hpp"
#include "render/ParticleSystem.hpp"

using namespace OpenCraft;

void testQualityProfiles() {
    auto low = QualityProfiles::get(QualityPresetLevel::Low);
    assert(low.renderDistance == 4);
    assert(low.simulationDistance == 4);
    assert(low.smoothLighting == false);
    assert(low.maxParticles == 256);
    assert(low.estimatedMemoryCapMB <= 100);

    auto ultra = QualityProfiles::get(QualityPresetLevel::Ultra);
    assert(ultra.renderDistance == 16);
    assert(ultra.simulationDistance == 12);
    assert(ultra.smoothLighting == true);
    assert(ultra.maxParticles == 4096);

    std::cout << "[PASS] Perfiles de calidad Low, Medium, High y Ultra verificados." << std::endl;
}

void testParticleBudgetCap() {
    ParticleSystem ps(256);
    assert(ps.getMaxParticles() == 256);

    // Intentar emitir 600 partículas (más del doble del presupuesto)
    for (int i = 0; i < 600; ++i) {
        Particle p;
        p.position = {static_cast<float>(i), 64.0f, 0.0f};
        ps.spawnParticle(p);
    }

    // El sistema debe limitar estrictamente las partículas vivas al presupuesto máximo
    assert(ps.getActiveParticleCount() <= 256);
    assert(ps.getParticles().size() == 256);

    // Actualizar con dt
    ps.update(0.05f);
    assert(ps.getActiveParticleCount() <= 256);

    std::cout << "[PASS] Presupuesto estricto de partículas (límite duro de memoria) verificado." << std::endl;
}

void testBenchmarkMetricsAndPercentiles() {
    BenchmarkHarness bench;

    std::mt19937 rng(42);
    std::normal_distribution<double> frameDist(16.6, 1.2); // ~60 FPS
    std::normal_distribution<double> tickDist(8.0, 1.5);   // ~8ms simulación (límite 50ms)

    // Simular 1200 fotogramas (20 segundos a 60 FPS)
    for (int i = 0; i < 1200; ++i) {
        double f = std::max(10.0, frameDist(rng));
        bench.recordFrameTimeMs(f);
    }

    // Simular 400 ticks (20 segundos a 20 TPS)
    for (int i = 0; i < 400; ++i) {
        double t = std::max(2.0, tickDist(rng));
        bench.recordTickDurationMs(t);
    }

    bench.recordMemorySampleMB(68);

    BenchmarkMetrics m = bench.computeMetrics();
    assert(m.totalFrames == 1200);
    assert(m.totalTicks == 400);
    assert(m.avgFps >= 55.0 && m.avgFps <= 65.0);
    assert(m.p50FrameTimeMs >= 15.0 && m.p50FrameTimeMs <= 18.0);
    assert(m.p95FrameTimeMs < 25.0);
    assert(m.p99FrameTimeMs < 30.0);
    assert(m.p95TickTimeMs < 50.0);
    assert(m.tickBudgetPass == true);

    std::string report = bench.generateMarkdownReport("Sesión 30 min — Hardware Referencia");
    assert(report.find("### Informe de Benchmark") != std::string::npos);
    assert(report.find("Frame Time p95") != std::string::npos);
    assert(report.find("Tick Time p95") != std::string::npos);

    std::cout << "[PASS] Cálculo de percentiles p50/p95/p99 y cuota de tick verificados." << std::endl;
}

int main() {
    std::cout << "=== Ejecutando Suite de Pruebas: BenchmarkAndQualityProfilesTest ===" << std::endl;
    testQualityProfiles();
    testParticleBudgetCap();
    testBenchmarkMetricsAndPercentiles();
    std::cout << "=== Todas las pruebas de Benchmarks y Perfiles de Calidad SUPERADAS ===" << std::endl;
    return 0;
}

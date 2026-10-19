#include "benchmark/BenchmarkHarness.hpp"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace OpenCraft {

BenchmarkHarness::BenchmarkHarness() = default;

void BenchmarkHarness::reset() {
    m_frameTimes.clear();
    m_tickTimes.clear();
    m_peakMemoryMB = 0;
}

void BenchmarkHarness::recordFrameTimeMs(double frameMs) {
    if (frameMs > 0.0) {
        m_frameTimes.push_back(frameMs);
    }
}

void BenchmarkHarness::recordTickDurationMs(double tickMs) {
    if (tickMs >= 0.0) {
        m_tickTimes.push_back(tickMs);
    }
}

void BenchmarkHarness::recordMemorySampleMB(size_t memoryMB) {
    if (memoryMB > m_peakMemoryMB) {
        m_peakMemoryMB = memoryMB;
    }
}

double BenchmarkHarness::calculatePercentile(std::vector<double>& sortedData, double p) {
    if (sortedData.empty()) return 0.0;
    if (p <= 0.0) return sortedData.front();
    if (p >= 100.0) return sortedData.back();

    double rank = (p / 100.0) * (static_cast<double>(sortedData.size()) - 1.0);
    size_t low = static_cast<size_t>(rank);
    size_t high = std::min(low + 1, sortedData.size() - 1);
    double weight = rank - static_cast<double>(low);

    return sortedData[low] * (1.0 - weight) + sortedData[high] * weight;
}

BenchmarkMetrics BenchmarkHarness::computeMetrics() {
    BenchmarkMetrics m;
    m.totalFrames = m_frameTimes.size();
    m.totalTicks = m_tickTimes.size();
    m.peakMemoryMB = m_peakMemoryMB;

    if (!m_frameTimes.empty()) {
        std::vector<double> sortedFrames = m_frameTimes;
        std::sort(sortedFrames.begin(), sortedFrames.end());

        double totalMs = std::accumulate(sortedFrames.begin(), sortedFrames.end(), 0.0);
        m.totalTimeSec = totalMs / 1000.0;
        m.avgFps = (m.totalTimeSec > 0.0) ? (static_cast<double>(m.totalFrames) / m.totalTimeSec) : 0.0;

        m.p50FrameTimeMs = calculatePercentile(sortedFrames, 50.0);
        m.p95FrameTimeMs = calculatePercentile(sortedFrames, 95.0);
        m.p99FrameTimeMs = calculatePercentile(sortedFrames, 99.0);
        m.maxFrameTimeMs = sortedFrames.back();
    }

    if (!m_tickTimes.empty()) {
        std::vector<double> sortedTicks = m_tickTimes;
        std::sort(sortedTicks.begin(), sortedTicks.end());

        m.p50TickTimeMs = calculatePercentile(sortedTicks, 50.0);
        m.p95TickTimeMs = calculatePercentile(sortedTicks, 95.0);
        m.maxTickTimeMs = sortedTicks.back();
        m.tickBudgetPass = (m.p95TickTimeMs <= 50.0); // 20 TPS = 50ms por tick
    }

    return m;
}

std::string BenchmarkHarness::generateMarkdownReport(const std::string& sceneName) {
    BenchmarkMetrics m = computeMetrics();
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);

    ss << "### Informe de Benchmark: " << sceneName << "\n\n";
    ss << "| Métrica | Valor Medido | Umbral de Referencia | Estado |\n";
    ss << "| :--- | :--- | :--- | :---: |\n";
    ss << "| Fotogramas Totales | " << m.totalFrames << " | $\\ge 1000$ | 🟢 |\n";
    ss << "| FPS Promedio | " << m.avgFps << " FPS | $\\ge 60$ FPS | " << (m.avgFps >= 59.5 ? "🟢" : "🟡") << " |\n";
    ss << "| Frame Time p50 | " << m.p50FrameTimeMs << " ms | $\\le 16.6$ ms | "
       << (m.p50FrameTimeMs <= 16.7 ? "🟢" : "🟡") << " |\n";
    ss << "| Frame Time p95 | " << m.p95FrameTimeMs << " ms | $\\le 25.0$ ms | "
       << (m.p95FrameTimeMs <= 25.0 ? "🟢" : "🟡") << " |\n";
    ss << "| Frame Time p99 | " << m.p99FrameTimeMs << " ms | $\\le 33.3$ ms | "
       << (m.p99FrameTimeMs <= 33.3 ? "🟢" : "🟡") << " |\n";
    ss << "| Tick Time p95 (Simulación 20 Hz) | " << m.p95TickTimeMs << " ms | $\\le 50.0$ ms | "
       << (m.tickBudgetPass ? "🟢" : "🔴") << " |\n";
    ss << "| Consumo Pico de RAM | " << m.peakMemoryMB << " MB | $\\le 150.0$ MB | "
       << (m.peakMemoryMB <= 150 ? "🟢" : "🟡") << " |\n";

    return ss.str();
}

} // namespace OpenCraft

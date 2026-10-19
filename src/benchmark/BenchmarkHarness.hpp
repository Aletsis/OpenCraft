#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace OpenCraft {

struct BenchmarkMetrics {
    size_t totalFrames = 0;
    double totalTimeSec = 0.0;
    double avgFps = 0.0;
    double p50FrameTimeMs = 0.0;
    double p95FrameTimeMs = 0.0;
    double p99FrameTimeMs = 0.0;
    double maxFrameTimeMs = 0.0;

    size_t totalTicks = 0;
    double p50TickTimeMs = 0.0;
    double p95TickTimeMs = 0.0;
    double maxTickTimeMs = 0.0;
    bool tickBudgetPass = true; // <= 50.0 ms para 20 TPS

    size_t peakMemoryMB = 0;
};

class BenchmarkHarness {
public:
    BenchmarkHarness();

    void recordFrameTimeMs(double frameMs);
    void recordTickDurationMs(double tickMs);
    void recordMemorySampleMB(size_t memoryMB);

    BenchmarkMetrics computeMetrics();
    std::string generateMarkdownReport(const std::string& sceneName = "Escena F00 Ampliada");

    void reset();

private:
    std::vector<double> m_frameTimes;
    std::vector<double> m_tickTimes;
    size_t m_peakMemoryMB = 0;

    static double calculatePercentile(std::vector<double>& sortedData, double p);
};

} // namespace OpenCraft

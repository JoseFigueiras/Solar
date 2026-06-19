#include "profiling.h"

#include <cstdio>

namespace profiling {

double physicsMs = 0.0;
double renderMs = 0.0;
double presentMs = 0.0;

double physicsSampleMs = 0.0;
double renderSampleMs = 0.0;
double presentSampleMs = 0.0;

double avgFrameMs = 0.0;
double avgPhysicsMs = 0.0;
double avgRenderMs = 0.0;
double avgPresentMs = 0.0;

namespace {

// Running totals for the current one-second window.
double frameSum = 0.0;
double physicsSum = 0.0;
double renderSum = 0.0;
double presentSum = 0.0;
double elapsedMs = 0.0;
int sampleCount = 0;

// Ring buffer holding the most recent completed one-second averages, used to
// compute the 10-second average reported on shutdown.
constexpr int kHistorySeconds = 10;
double frameHistory[kHistorySeconds] = {};
double physicsHistory[kHistorySeconds] = {};
double renderHistory[kHistorySeconds] = {};
double presentHistory[kHistorySeconds] = {};
int historyNext = 0;   // index of the next slot to write
int historyCount = 0;  // number of valid entries (capped at kHistorySeconds)

} // namespace

void tick(double frameMs)
{
    frameSum += frameMs;
    physicsSum += physicsSampleMs;
    renderSum += renderSampleMs;
    presentSum += presentSampleMs;
    elapsedMs += frameMs;
    ++sampleCount;

    if (elapsedMs >= 1000.0 && sampleCount > 0)
    {
        avgFrameMs = frameSum / sampleCount;
        avgPhysicsMs = physicsSum / sampleCount;
        avgRenderMs = renderSum / sampleCount;
        avgPresentMs = presentSum / sampleCount;

        frameHistory[historyNext] = avgFrameMs;
        physicsHistory[historyNext] = avgPhysicsMs;
        renderHistory[historyNext] = avgRenderMs;
        presentHistory[historyNext] = avgPresentMs;
        historyNext = (historyNext + 1) % kHistorySeconds;
        if (historyCount < kHistorySeconds)
        {
            ++historyCount;
        }

        frameSum = 0.0;
        physicsSum = 0.0;
        renderSum = 0.0;
        presentSum = 0.0;
        elapsedMs = 0.0;
        sampleCount = 0;
    }
}

void report()
{
    if (historyCount == 0)
    {
        std::printf("\n=== Profiling (10s averages) ===\nno data captured\n");
        return;
    }

    double frameTotal = 0.0;
    double physicsTotal = 0.0;
    double renderTotal = 0.0;
    double presentTotal = 0.0;
    for (int i = 0; i < historyCount; ++i)
    {
        frameTotal += frameHistory[i];
        physicsTotal += physicsHistory[i];
        renderTotal += renderHistory[i];
        presentTotal += presentHistory[i];
    }

    std::printf(
        "\n=== Profiling (10s averages) ===\n"
        "frame avg:   %6.2f ms\n"
        "physics avg: %6.2f ms\n"
        "render avg:  %6.2f ms\n"
        "present avg: %6.2f ms\n",
        frameTotal / historyCount,
        physicsTotal / historyCount,
        renderTotal / historyCount,
        presentTotal / historyCount);

    if (historyCount < kHistorySeconds)
    {
        std::printf("(captured %d of %d seconds)\n", historyCount, kHistorySeconds);
    }
}

} // namespace profiling

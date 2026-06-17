#include "profiling.h"

namespace profiling {

double physicsMs = 0.0;
double renderMs = 0.0;

double physicsSampleMs = 0.0;
double renderSampleMs = 0.0;

double avgFrameMs = 0.0;
double avgPhysicsMs = 0.0;
double avgRenderMs = 0.0;

namespace {

// Running totals for the current one-second window.
double frameSum = 0.0;
double physicsSum = 0.0;
double renderSum = 0.0;
double elapsedMs = 0.0;
int sampleCount = 0;

} // namespace

void tick(double frameMs)
{
    frameSum += frameMs;
    physicsSum += physicsSampleMs;
    renderSum += renderSampleMs;
    elapsedMs += frameMs;
    ++sampleCount;

    if (elapsedMs >= 1000.0 && sampleCount > 0)
    {
        avgFrameMs = frameSum / sampleCount;
        avgPhysicsMs = physicsSum / sampleCount;
        avgRenderMs = renderSum / sampleCount;

        frameSum = 0.0;
        physicsSum = 0.0;
        renderSum = 0.0;
        elapsedMs = 0.0;
        sampleCount = 0;
    }
}

} // namespace profiling

#pragma once

// Lightweight shared profiling state. Each instrumented function measures its
// own CPU time and stores a smoothed result here so the renderer can display
// all timings together as an on-screen overlay.
namespace profiling {

// Smoothed CPU time (in milliseconds) spent in the most recent calls.
extern double physicsMs;
extern double renderMs;

// Raw (unsmoothed) time of the most recent call, fed into the per-second
// averages below.
extern double physicsSampleMs;
extern double renderSampleMs;

// Averages over the last full second, refreshed once per second.
extern double avgFrameMs;
extern double avgPhysicsMs;
extern double avgRenderMs;

// Fold a fresh sample into a smoothed value using an exponential moving
// average, so on-screen numbers stay readable instead of flickering.
inline void accumulate(double &smoothed, double sampleMs)
{
    constexpr double kSmoothing = 0.1;
    smoothed += kSmoothing * (sampleMs - smoothed);
}

// Called once per frame from the main loop with the independently measured
// total frame time. Accumulates the frame/physics/render samples and refreshes
// the per-second averages when a full second has elapsed.
void tick(double frameMs);

} // namespace profiling

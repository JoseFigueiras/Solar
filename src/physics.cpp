#include "physics.h"

#include "profiling.h"

#include <chrono>
#include <cmath>
#include <vector>

namespace {

// Softening length to avoid singular forces when bodies get very close.
constexpr float kSoftening = 0.05f;
// Number of integration substeps run every frame. The count is fixed, so the
// per-frame physics cost stays constant regardless of frame rate.
constexpr int kSubstepsPerFrame = 4;

std::vector<Vector3> compute_accelerations(const std::vector<CelestialBody> &bodies)
{
    std::vector<Vector3> accelerations(bodies.size(), {0.0f, 0.0f, 0.0f});

    // Only bodies with mass act as gravity sources. Asteroids are massless test
    // particles: they are pulled by the massive bodies but exert no force of
    // their own and do not interact with each other. This keeps the cost at
    // O(bodies * sources) instead of O(bodies^2) when there are thousands of
    // asteroids.
    for (std::size_t j = 0; j < bodies.size(); ++j)
    {
        const float sourceMass = bodies[j].mass;
        if (sourceMass <= 0.0f)
        {
            continue;
        }

        for (std::size_t i = 0; i < bodies.size(); ++i)
        {
            if (i == j)
            {
                continue;
            }

            const Vector3 delta = {
                bodies[j].position.x - bodies[i].position.x,
                bodies[j].position.y - bodies[i].position.y,
                bodies[j].position.z - bodies[i].position.z,
            };

            const float distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z + kSoftening * kSoftening;
            const float invDist = 1.0f / std::sqrt(distSq);
            const float invDist3 = invDist * invDist * invDist;

            // Acceleration on i from j is G * m_j / r^2 along the unit vector to j.
            const float scale = physics::kGravity * sourceMass * invDist3;
            accelerations[i].x += delta.x * scale;
            accelerations[i].y += delta.y * scale;
            accelerations[i].z += delta.z * scale;
        }
    }

    return accelerations;
}

void step_physics(std::vector<CelestialBody> &bodies, float dt)
{
    const std::vector<Vector3> accelOld = compute_accelerations(bodies);

    // Velocity Verlet: advance positions using the current acceleration.
    for (std::size_t i = 0; i < bodies.size(); ++i)
    {
        bodies[i].position.x += bodies[i].velocity.x * dt + 0.5f * accelOld[i].x * dt * dt;
        bodies[i].position.y += bodies[i].velocity.y * dt + 0.5f * accelOld[i].y * dt * dt;
        bodies[i].position.z += bodies[i].velocity.z * dt + 0.5f * accelOld[i].z * dt * dt;
    }

    const std::vector<Vector3> accelNew = compute_accelerations(bodies);

    // Update velocities with the average of old and new accelerations.
    for (std::size_t i = 0; i < bodies.size(); ++i)
    {
        bodies[i].velocity.x += 0.5f * (accelOld[i].x + accelNew[i].x) * dt;
        bodies[i].velocity.y += 0.5f * (accelOld[i].y + accelNew[i].y) * dt;
        bodies[i].velocity.z += 0.5f * (accelOld[i].z + accelNew[i].z) * dt;
    }
}

} // namespace

namespace physics {

void update(std::vector<CelestialBody> &bodies, float frameTime)
{
    // Real-time motion with a constant per-frame physics cost: run a fixed
    // number of substeps, but size each one to cover an equal share of the real
    // elapsed frame time. Simulated time therefore tracks wall-clock time while
    // the amount of computation stays the same every frame. The trade-off is
    // that on a slow frame each substep is large, so integration accuracy
    // degrades (and may go unstable) rather than slowing down.
    const float dt = frameTime / kSubstepsPerFrame;

    const auto profilingStart = std::chrono::steady_clock::now();

    for (int step = 0; step < kSubstepsPerFrame; ++step)
    {
        step_physics(bodies, dt);
    }

    const auto profilingEnd = std::chrono::steady_clock::now();
    const double sampleMs = std::chrono::duration<double, std::milli>(profilingEnd - profilingStart).count();
    profiling::physicsSampleMs = sampleMs;
    profiling::accumulate(profiling::physicsMs, sampleMs);
}

} // namespace physics

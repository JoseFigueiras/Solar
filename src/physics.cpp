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

std::vector<Vector3> compute_massive_accelerations(const std::vector<MassiveBody> &bodies)
{
    std::vector<Vector3> accelerations(bodies.size(), {0.0f, 0.0f, 0.0f});

    // Every massive body both feels and exerts gravity, so this is the full
    // O(massive^2) pairwise sum. The massive set is tiny compared to the
    // asteroid belts, so this loop stays cheap.
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

std::vector<Vector3> compute_massless_accelerations(
    const MasslessBodies &masslessBodies,
    const std::vector<MassiveBody> &massiveBodies)
{
    std::vector<Vector3> accelerations(masslessBodies.size(), {0.0f, 0.0f, 0.0f});

    // Asteroids are massless test particles: they are pulled by the massive
    // bodies but exert no force of their own and do not interact with each
    // other. Cost is O(massless * massive). Looping with the source body in the
    // outer loop keeps each massive body's data hot in cache across the whole
    // inner sweep.
    for (std::size_t j = 0; j < massiveBodies.size(); ++j)
    {
        const float sourceMass = massiveBodies[j].mass;
        if (sourceMass <= 0.0f)
        {
            continue;
        }

        const Vector3 sourcePos = massiveBodies[j].position;

        for (std::size_t i = 0; i < masslessBodies.size(); ++i)
        {
            const Vector3 delta = {
                sourcePos.x - masslessBodies[i].position.x,
                sourcePos.y - masslessBodies[i].position.y,
                sourcePos.z - masslessBodies[i].position.z,
            };

            const float distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z + kSoftening * kSoftening;
            const float invDist = 1.0f / std::sqrt(distSq);
            const float invDist3 = invDist * invDist * invDist;

            const float scale = physics::kGravity * sourceMass * invDist3;
            accelerations[i].x += delta.x * scale;
            accelerations[i].y += delta.y * scale;
            accelerations[i].z += delta.z * scale;
        }
    }

    return accelerations;
}

void step_physics(std::vector<MassiveBody> &massiveBodies, MasslessBodies &masslessBodies, float dt)
{
    const std::vector<Vector3> massiveAccelOld = compute_massive_accelerations(massiveBodies);
    const std::vector<Vector3> masslessAccelOld =
        compute_massless_accelerations(masslessBodies, massiveBodies);

    // Velocity Verlet: advance positions using the current acceleration.
    for (std::size_t i = 0; i < massiveBodies.size(); ++i)
    {
        massiveBodies[i].position.x += massiveBodies[i].velocity.x * dt + 0.5f * massiveAccelOld[i].x * dt * dt;
        massiveBodies[i].position.y += massiveBodies[i].velocity.y * dt + 0.5f * massiveAccelOld[i].y * dt * dt;
        massiveBodies[i].position.z += massiveBodies[i].velocity.z * dt + 0.5f * massiveAccelOld[i].z * dt * dt;
    }

    for (std::size_t i = 0; i < masslessBodies.size(); ++i)
    {
        masslessBodies[i].position.x += masslessBodies[i].velocity.x * dt + 0.5f * masslessAccelOld[i].x * dt * dt;
        masslessBodies[i].position.y += masslessBodies[i].velocity.y * dt + 0.5f * masslessAccelOld[i].y * dt * dt;
        masslessBodies[i].position.z += masslessBodies[i].velocity.z * dt + 0.5f * masslessAccelOld[i].z * dt * dt;
    }

    // Recompute accelerations at the new positions. Massless bodies are never
    // sources, so the massive set's new accelerations depend only on the
    // massive positions just advanced above.
    const std::vector<Vector3> massiveAccelNew = compute_massive_accelerations(massiveBodies);
    const std::vector<Vector3> masslessAccelNew =
        compute_massless_accelerations(masslessBodies, massiveBodies);

    // Update velocities with the average of old and new accelerations.
    for (std::size_t i = 0; i < massiveBodies.size(); ++i)
    {
        massiveBodies[i].velocity.x += 0.5f * (massiveAccelOld[i].x + massiveAccelNew[i].x) * dt;
        massiveBodies[i].velocity.y += 0.5f * (massiveAccelOld[i].y + massiveAccelNew[i].y) * dt;
        massiveBodies[i].velocity.z += 0.5f * (massiveAccelOld[i].z + massiveAccelNew[i].z) * dt;
    }

    for (std::size_t i = 0; i < masslessBodies.size(); ++i)
    {
        masslessBodies[i].velocity.x += 0.5f * (masslessAccelOld[i].x + masslessAccelNew[i].x) * dt;
        masslessBodies[i].velocity.y += 0.5f * (masslessAccelOld[i].y + masslessAccelNew[i].y) * dt;
        masslessBodies[i].velocity.z += 0.5f * (masslessAccelOld[i].z + masslessAccelNew[i].z) * dt;
    }
}

} // namespace

namespace physics {

void update(std::vector<MassiveBody> &massiveBodies, MasslessBodies &masslessBodies, float frameTime)
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
        step_physics(massiveBodies, masslessBodies, dt);
    }

    const auto profilingEnd = std::chrono::steady_clock::now();
    const double sampleMs = std::chrono::duration<double, std::milli>(profilingEnd - profilingStart).count();
    profiling::physicsSampleMs = sampleMs;
    profiling::accumulate(profiling::physicsMs, sampleMs);
}

} // namespace physics

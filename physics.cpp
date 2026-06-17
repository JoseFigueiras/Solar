#include "physics.h"

#include "profiling.h"

#include <chrono>
#include <cmath>
#include <vector>

namespace {

// Gravitational constant in simulation units (tuned for a visually pleasant scale).
constexpr float kGravity = 1.0f;
// Softening length to avoid singular forces when bodies get very close.
constexpr float kSoftening = 0.05f;
// Controls how fast simulated time advances relative to real time.
constexpr float kTimeScale = 1.0f;
// Fixed physics timestep for numerical stability, independent of frame rate.
constexpr float kFixedStep = 1.0f / 240.0f;

std::vector<Vector3> compute_accelerations(const std::vector<CelestialBody> &bodies)
{
    std::vector<Vector3> accelerations(bodies.size(), {0.0f, 0.0f, 0.0f});

    for (std::size_t i = 0; i < bodies.size(); ++i)
    {
        for (std::size_t j = i + 1; j < bodies.size(); ++j)
        {
            const Vector3 delta = {
                bodies[j].position.x - bodies[i].position.x,
                bodies[j].position.y - bodies[i].position.y,
                bodies[j].position.z - bodies[i].position.z,
            };

            const float distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z + kSoftening * kSoftening;
            const float invDist = 1.0f / std::sqrt(distSq);
            const float invDist3 = invDist * invDist * invDist;

            // Acceleration on i from j is G * m_j / r^2 along the unit vector to j.
            const float scaleI = kGravity * bodies[j].mass * invDist3;
            accelerations[i].x += delta.x * scaleI;
            accelerations[i].y += delta.y * scaleI;
            accelerations[i].z += delta.z * scaleI;

            // Equal and opposite reaction on j from i.
            const float scaleJ = kGravity * bodies[i].mass * invDist3;
            accelerations[j].x -= delta.x * scaleJ;
            accelerations[j].y -= delta.y * scaleJ;
            accelerations[j].z -= delta.z * scaleJ;
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

// Remove net momentum so the system's barycenter stays fixed in space.
void zero_net_momentum(std::vector<CelestialBody> &bodies)
{
    Vector3 totalMomentum = {0.0f, 0.0f, 0.0f};
    float totalMass = 0.0f;

    for (const CelestialBody &body : bodies)
    {
        totalMomentum.x += body.velocity.x * body.mass;
        totalMomentum.y += body.velocity.y * body.mass;
        totalMomentum.z += body.velocity.z * body.mass;
        totalMass += body.mass;
    }

    if (totalMass <= 0.0f)
    {
        return;
    }

    const Vector3 driftVelocity = {
        totalMomentum.x / totalMass,
        totalMomentum.y / totalMass,
        totalMomentum.z / totalMass,
    };

    for (CelestialBody &body : bodies)
    {
        body.velocity.x -= driftVelocity.x;
        body.velocity.y -= driftVelocity.y;
        body.velocity.z -= driftVelocity.z;
    }
}

// Place a body on a circular orbit around a central mass at the given radius.
CelestialBody make_orbiting_body(
    const CelestialBody &center,
    float orbitRadius,
    float mass,
    float radius,
    float phase,
    float verticalOffset)
{
    const Vector3 position = {
        center.position.x + std::cos(phase) * orbitRadius,
        center.position.y + verticalOffset,
        center.position.z + std::sin(phase) * orbitRadius,
    };

    // Circular orbital speed for a two-body approximation: v = sqrt(G * M / r).
    const float speed = std::sqrt(kGravity * center.mass / orbitRadius);

    // Velocity is perpendicular to the radius, in the orbital plane.
    const Vector3 velocity = {
        -std::sin(phase) * speed,
        0.0f,
        std::cos(phase) * speed,
    };

    return {position, velocity, mass, radius};
}

} // namespace

namespace physics {

std::vector<CelestialBody> create_solar_system()
{
    const CelestialBody sun = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1000.0f, 1.35f};

    std::vector<CelestialBody> bodies = {
        sun,
        make_orbiting_body(sun, 2.4f, 0.8f, 0.24f, 0.0f, 0.0f),
        make_orbiting_body(sun, 3.5f, 1.6f, 0.38f, 1.8f, 0.08f),
        make_orbiting_body(sun, 4.8f, 3.2f, 0.55f, 3.4f, -0.06f),
        make_orbiting_body(sun, 6.2f, 1.1f, 0.31f, 4.7f, 0.16f),
        make_orbiting_body(sun, 7.8f, 5.0f, 0.68f, 2.9f, -0.12f),
        make_orbiting_body(sun, 9.4f, 2.4f, 0.46f, 5.3f, 0.24f),
    };

    // Start with no net momentum so the barycenter does not drift away.
    zero_net_momentum(bodies);

    return bodies;
}

void update(std::vector<CelestialBody> &bodies, float frameTime)
{
    const auto profilingStart = std::chrono::steady_clock::now();

    // Carry leftover simulated time between frames so motion stays smooth and
    // frame-rate independent.
    static float accumulator = 0.0f;
    accumulator += frameTime * kTimeScale;

    // Advance the simulation in fixed-size steps for stable integration.
    while (accumulator >= kFixedStep)
    {
        step_physics(bodies, kFixedStep);
        accumulator -= kFixedStep;
    }

    const auto profilingEnd = std::chrono::steady_clock::now();
    const double sampleMs = std::chrono::duration<double, std::milli>(profilingEnd - profilingStart).count();
    profiling::physicsSampleMs = sampleMs;
    profiling::accumulate(profiling::physicsMs, sampleMs);
}

} // namespace physics

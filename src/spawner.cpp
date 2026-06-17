#include "spawner.h"

#include "physics.h"

#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace {

// --- Tunable scene parameters -------------------------------------------------

// Global multiplier on every orbital distance from the sun (planets and belts).
// Spreading orbits out keeps the massive giants from flinging the asteroid
// belts apart while still letting the camera frame everything.
constexpr float kOrbitScale = 10.0f;

// Converts a body's mass into a render radius assuming constant density, so
// radius grows with the cube root of mass (r = k * cbrt(m)). The constant is
// tuned for visibility against the wide orbital distances.
constexpr float kRadiusPerCbrtMass = 2.6f;

// The sun keeps an explicit radius: constant-density scaling from its huge mass
// would make it dwarf everything else.
constexpr float kSunMass = 1000.0f;
constexpr float kSunRadius = 14.0f;

// Asteroids are massless test particles, so they have no mass to derive a size
// from; they share one small fixed render radius instead.
constexpr float kAsteroidRadius = 0.5f;

// Total asteroids per belt. The actual count used at runtime is passed into
// create_solar_system; this constant only documents the historical default and
// is exposed via spawner::kDefaultAsteroidsPerBelt in the header.

// Inner "main" belt sits between the rocky inner planets and the gas giants.
// Radii are pre-scale; the global kOrbitScale is applied when the belt is built.
constexpr float kInnerBeltMinRadius = 11.0f;
constexpr float kInnerBeltMaxRadius = 13.5f;
constexpr float kInnerBeltThickness = 0.35f;

// Outer belt lies beyond the outermost planet.
constexpr float kOuterBeltMinRadius = 30.0f;
constexpr float kOuterBeltMaxRadius = 34.0f;
constexpr float kOuterBeltThickness = 0.6f;

// Fixed seed keeps the generated belts identical from run to run.
constexpr std::uint32_t kAsteroidSeed = 0x50C1A12Eu;

// --- Helpers ------------------------------------------------------------------

float radius_from_mass(float mass)
{
    return kRadiusPerCbrtMass * std::cbrt(mass);
}

// Place a body on a circular orbit around a central mass at the given radius.
// The orbit is computed relative to the center's current position *and*
// velocity, so moons correctly inherit the motion of the planet they circle.
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
    const float speed = std::sqrt(physics::kGravity * center.mass / orbitRadius);

    // Velocity is perpendicular to the radius, in the orbital plane, added on
    // top of whatever motion the central body already has.
    const Vector3 velocity = {
        center.velocity.x + -std::sin(phase) * speed,
        center.velocity.y,
        center.velocity.z + std::cos(phase) * speed,
    };

    return {position, velocity, mass, radius};
}

// Convenience overload: a planet or moon whose render radius is derived from its
// mass via the constant-density relationship.
CelestialBody make_massive_body(
    const CelestialBody &center,
    float orbitRadius,
    float mass,
    float phase,
    float verticalOffset)
{
    return make_orbiting_body(center, orbitRadius, mass, radius_from_mass(mass), phase, verticalOffset);
}

// Append `count` moons orbiting `planet` into `bodies`. Moons are spread around
// the planet with deterministic-but-varied spacing and small vertical spread so
// dense systems do not look like a single flat ring.
void add_moons(
    std::vector<CelestialBody> &bodies,
    const CelestialBody &planet,
    int count,
    std::mt19937 &rng)
{
    if (count <= 0)
    {
        return;
    }

    const float planetRadius = planet.radius;
    // Keep moons comfortably outside the planet surface and inside its Hill-ish
    // neighborhood so they stay gravitationally bound and visually distinct.
    const float minOrbit = planetRadius * 2.0f;
    const float maxOrbit = planetRadius * 6.0f;

    std::uniform_real_distribution<float> orbitDist(minOrbit, maxOrbit);
    std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * static_cast<float>(M_PI));
    std::uniform_real_distribution<float> tiltDist(-planetRadius * 0.5f, planetRadius * 0.5f);
    // Tiny moon masses: enough to participate in the N-body sum without
    // meaningfully perturbing their parent planet.
    std::uniform_real_distribution<float> massDist(0.0005f, 0.004f);

    for (int i = 0; i < count; ++i)
    {
        const float orbit = orbitDist(rng);
        const float phase = phaseDist(rng);
        const float tilt = tiltDist(rng);
        const float mass = massDist(rng);
        bodies.push_back(make_massive_body(planet, orbit, mass, phase, tilt));
    }
}

// Generate one asteroid belt as a band of massless test particles orbiting the
// sun. Positions, phases, vertical spread, and slight speed jitter are all
// sampled randomly, so the belt is described by a handful of parameters rather
// than thousands of hardcoded values.
void add_asteroid_belt(
    std::vector<CelestialBody> &bodies,
    const CelestialBody &sun,
    int count,
    float minRadius,
    float maxRadius,
    float thickness,
    std::mt19937 &rng)
{
    std::uniform_real_distribution<float> radiusDist(minRadius, maxRadius);
    std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * static_cast<float>(M_PI));
    std::uniform_real_distribution<float> heightDist(-thickness, thickness);
    // +/-3% speed variation gives the belt mild eccentricity instead of a
    // perfectly uniform ring.
    std::uniform_real_distribution<float> speedJitter(0.97f, 1.03f);

    bodies.reserve(bodies.size() + static_cast<std::size_t>(count));

    for (int i = 0; i < count; ++i)
    {
        const float orbitRadius = radiusDist(rng);
        const float phase = phaseDist(rng);
        const float height = heightDist(rng);

        CelestialBody asteroid =
            make_orbiting_body(sun, orbitRadius, 0.0f, kAsteroidRadius, phase, height);

        const float jitter = speedJitter(rng);
        asteroid.velocity.x *= jitter;
        asteroid.velocity.z *= jitter;

        bodies.push_back(asteroid);
    }
}

// Remove net momentum so the system's barycenter stays fixed in space. Massless
// asteroids contribute nothing here, so the belts cannot bias the drift.
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

} // namespace

namespace spawner {

std::vector<CelestialBody> create_solar_system(int asteroidsPerBelt)
{
    if (asteroidsPerBelt < 0)
    {
        asteroidsPerBelt = 0;
    }

    const CelestialBody sun = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, kSunMass, kSunRadius};

    std::mt19937 rng(kAsteroidSeed);

    std::vector<CelestialBody> bodies;
    bodies.push_back(sun);

    // Ten planets. The four inner worlds are small and rocky; the outer six are
    // progressively more massive gas/ice giants. Each entry is
    // {orbitRadius, mass, phase, verticalOffset, moonCount}.
    struct PlanetSpec {
        float orbitRadius;
        float mass;
        float phase;
        float verticalOffset;
        int moonCount;
    };

    const PlanetSpec planets[] = {
        {2.4f, 0.4f, 0.0f, 0.00f, 0},
        {3.6f, 0.9f, 1.8f, 0.06f, 1},
        {5.0f, 1.1f, 3.4f, -0.05f, 2},
        {6.6f, 0.6f, 4.7f, 0.12f, 2},
        {20.0f, 18.0f, 2.9f, -0.10f, 62},
        {26.0f, 14.0f, 5.3f, 0.18f, 70},
        {42.0f, 8.0f, 0.7f, -0.14f, 27},
        {50.0f, 7.0f, 2.1f, 0.20f, 14},
        {60.0f, 3.0f, 3.9f, -0.22f, 5},
        {70.0f, 2.5f, 5.6f, 0.24f, 3},
    };

    for (const PlanetSpec &spec : planets)
    {
        const CelestialBody planet =
            make_massive_body(sun, spec.orbitRadius * kOrbitScale, spec.mass, spec.phase, spec.verticalOffset);
        bodies.push_back(planet);
        add_moons(bodies, planet, spec.moonCount, rng);
    }

    // Two asteroid belts of massless test particles.
    add_asteroid_belt(
        bodies, sun, asteroidsPerBelt, kInnerBeltMinRadius * kOrbitScale, kInnerBeltMaxRadius * kOrbitScale,
        kInnerBeltThickness * kOrbitScale, rng);
    add_asteroid_belt(
        bodies, sun, asteroidsPerBelt, kOuterBeltMinRadius * kOrbitScale, kOuterBeltMaxRadius * kOrbitScale,
        kOuterBeltThickness * kOrbitScale, rng);

    // Start with no net momentum so the barycenter does not drift away.
    zero_net_momentum(bodies);

    return bodies;
}

} // namespace spawner

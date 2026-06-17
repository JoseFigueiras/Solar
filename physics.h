#pragma once

#include "solar.h"

#include <vector>

namespace physics {

// Build the initial set of bodies (sun + planets) with stable orbital velocities.
std::vector<CelestialBody> create_solar_system();

// Advance the simulation to cover the elapsed frame time, using fixed internal
// substeps for numerical stability.
void update(std::vector<CelestialBody> &bodies, float frameTime);

} // namespace physics

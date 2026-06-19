#pragma once

#include "solar.h"

namespace spawner {

// Default number of asteroids generated in each belt when no explicit count is
// requested.
constexpr int kDefaultAsteroidsPerBelt = 500;

// Build the initial set of bodies: the sun, ten planets with their moons, and
// two procedurally generated asteroid belts. Orbital velocities are chosen so
// every body starts on a stable circular orbit, and the whole system begins
// with no net momentum so the barycenter does not drift.
//
// `asteroidsPerBelt` controls how many massless test particles populate each of
// the two belts; pass a different value to trade visual density for
// performance.
GameObjects create_solar_system(int asteroidsPerBelt = kDefaultAsteroidsPerBelt);

} // namespace spawner

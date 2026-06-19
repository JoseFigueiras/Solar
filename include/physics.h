#pragma once

#include "solar.h"

#include <vector>

namespace physics {

// Gravitational constant in simulation units (tuned for a visually pleasant
// scale). Exposed so spawning code can derive stable circular orbital speeds.
inline constexpr float kGravity = 1.0f;

// Advance the simulation to cover the elapsed frame time, using fixed internal
// substeps for numerical stability. Massive bodies gravitate each other and the
// massless test particles; the massless bodies are only ever pulled, never
// sources, so they are integrated against the massive set.
void update(std::vector<MassiveBody> &massiveBodies, MasslessBodies &masslessBodies, float frameTime);

} // namespace physics

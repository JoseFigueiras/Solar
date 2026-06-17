#pragma once

#include "raylib.h"

// Shared data type used by both the physics and render modules.
struct CelestialBody {
    Vector3 position;
    Vector3 velocity;
    float mass;
    float radius;
};

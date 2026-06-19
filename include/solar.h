#pragma once

#include "raylib.h"

#include <vector>

// Physics state for a body that has mass and therefore acts as a gravity
// source. Kept deliberately small (position, velocity, mass) so the N-body
// inner loop streams through cache without dragging along render-only data.
struct MassiveBody {
    Vector3 position;
    Vector3 velocity;
    float mass;
};

// Physics state for a massless test particle (asteroid). It is pulled by the
// massive bodies but exerts no force, so it needs no mass and no render size
// of its own.
struct MasslessBody {
    Vector3 position;
    Vector3 velocity;
};

// Render-only data for a massive body, stored in a separate parallel array so
// it never pollutes the physics cache lines. Index i here corresponds to index
// i in MassiveBodies::bodies.
struct MassObjectsRenderInfo {
    float radius;
};

// A massive body's physics array and its render-info array, kept side by side.
// The two vectors are always the same length and index-aligned.
struct MassiveBodies {
    std::vector<MassiveBody> bodies;
    std::vector<MassObjectsRenderInfo> renderInfo;
};

// Massless bodies need only a single physics array.
using MasslessBodies = std::vector<MasslessBody>;

// The whole simulated scene: massive bodies (sun, planets, moons) and massless
// test particles (asteroid belts).
struct GameObjects {
    MassiveBodies massive;
    MasslessBodies massless;
};

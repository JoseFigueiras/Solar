#pragma once

#include "solar.h"

#include <vector>

// Owns the GPU assets needed to draw the scene. Assets are loaded in the
// constructor and released in the destructor (RAII), so a window must already
// be initialized before a Renderer is created.
class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    // Draw the bodies from a heliocentric viewpoint (sun kept centered),
    // observed through the supplied camera.
    void draw(const Camera3D &camera, const std::vector<CelestialBody> &bodies) const;

private:
    Model sphere_;
};

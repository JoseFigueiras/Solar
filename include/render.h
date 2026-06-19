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

    // Push the far clip plane well past the camera's maximum zoom-out distance
    // (input's maxRadius) plus the radius of the outermost orbits, so nothing in
    // the scene is culled when the view is pulled all the way back. raylib's
    // default far plane of 1000 is far too close for this orbital scale. A
    // window must already be initialized before calling this.
    static void configure_clip_planes();

    // Draw the bodies from a heliocentric viewpoint (sun kept centered),
    // observed through the supplied camera. Massive bodies and their parallel
    // render-info array describe the spheres; the massless bodies are drawn as
    // billboards.
    void draw(
        const Camera3D &camera,
        const std::vector<MassiveBody> &massiveBodies,
        const std::vector<MassObjectsRenderInfo> &massiveRenderInfo,
        const MasslessBodies &masslessBodies) const;

private:
    Model sphere_;
    Texture2D asteroidSprite_;
};

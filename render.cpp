#include "render.h"

#include "profiling.h"

#include <chrono>
#include <cstdio>
#include <vector>

namespace {

void draw_body(
    const CelestialBody &body,
    const Vector3 &renderPosition,
    const Model &sphere)
{
    DrawModelEx(
        sphere,
        renderPosition,
        {0.0f, 1.0f, 0.0f},
        0.0f,
        {body.radius, body.radius, body.radius},
        WHITE);
}

// Draw the profiling overlay: live per-frame timings in the top-left corner
// and per-second averages right-aligned in the top-right corner.
void draw_profiling_overlay()
{
    const int fontSize = 20;

    char overlay[64];
    std::snprintf(
        overlay,
        sizeof(overlay),
        "physics: %5.2f ms\nrender:  %5.2f ms",
        profiling::physicsMs,
        profiling::renderMs);
    DrawText(overlay, 10, 10, fontSize, RAYWHITE);

    // Each line is right-aligned so the numbers stay anchored to the edge
    // regardless of width.
    const int right = GetScreenWidth() - 10;
    char line[48];

    std::snprintf(line, sizeof(line), "frame avg:   %5.2f ms", profiling::avgFrameMs);
    DrawText(line, right - MeasureText(line, fontSize), 10, fontSize, RAYWHITE);

    std::snprintf(line, sizeof(line), "physics avg: %5.2f ms", profiling::avgPhysicsMs);
    DrawText(line, right - MeasureText(line, fontSize), 34, fontSize, RAYWHITE);

    std::snprintf(line, sizeof(line), "render avg:  %5.2f ms", profiling::avgRenderMs);
    DrawText(line, right - MeasureText(line, fontSize), 58, fontSize, RAYWHITE);
}

} // namespace

Renderer::Renderer()
    : sphere_(LoadModelFromMesh(GenMeshSphere(1.0f, 64, 32)))
{
}

Renderer::~Renderer()
{
    UnloadModel(sphere_);
}

void Renderer::draw(
    const Camera3D &camera,
    const std::vector<CelestialBody> &bodies) const
{
    // Heliocentric view: draw everything relative to the sun so it stays centered,
    // while the underlying physics remains a full N-body simulation.
    const Vector3 origin = bodies.empty() ? Vector3{0.0f, 0.0f, 0.0f} : bodies[0].position;

    const auto profilingStart = std::chrono::steady_clock::now();

    BeginDrawing();
    ClearBackground({3, 5, 12, 255});

    BeginMode3D(camera);
    DrawGrid(24, 1.0f);

    for (const CelestialBody &body : bodies)
    {
        const Vector3 renderPosition = {
            body.position.x - origin.x,
            body.position.y - origin.y,
            body.position.z - origin.z,
        };
        draw_body(body, renderPosition, sphere_);
    }
    EndMode3D();

    // Measure only the CPU command-recording work above. EndDrawing() below
    // blocks on vsync / the 60 FPS cap, so keeping it outside this region gives
    // the renderer's real cost instead of a flat ~16.66 ms frame time.
    const auto profilingEnd = std::chrono::steady_clock::now();
    const double sampleMs = std::chrono::duration<double, std::milli>(profilingEnd - profilingStart).count();
    profiling::renderSampleMs = sampleMs;
    profiling::accumulate(profiling::renderMs, sampleMs);

    draw_profiling_overlay();

    EndDrawing();
}

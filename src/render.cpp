#include "render.h"

#include "profiling.h"

#include "rlgl.h"

#include <chrono>
#include <cstdio>
#include <vector>

namespace {

// Draw the profiling overlay: live per-frame timings in the top-left corner
// and per-second averages right-aligned in the top-right corner.
void draw_profiling_overlay()
{
    const int fontSize = 20;

    char overlay[96];
    std::snprintf(
        overlay,
        sizeof(overlay),
        "physics: %5.2f ms\nrender:  %5.2f ms\npresent: %5.2f ms",
        profiling::physicsMs,
        profiling::renderMs,
        profiling::presentMs);
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

    std::snprintf(line, sizeof(line), "present avg: %5.2f ms", profiling::avgPresentMs);
    DrawText(line, right - MeasureText(line, fontSize), 82, fontSize, RAYWHITE);
}

// Build the tiny texture used for asteroid billboards. A single white pixel is
// enough: each asteroid is drawn as one camera-facing quad tinted to taste, so
// no detailed image is needed and the GPU keeps the whole belt in one cheap
// texture binding.
Texture2D load_asteroid_sprite()
{
    Image image = GenImageColor(1, 1, WHITE);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

} // namespace

Renderer::Renderer()
    : sphere_(LoadModelFromMesh(GenMeshSphere(1.0f, 12, 12))),
      asteroidSprite_(load_asteroid_sprite())
{
}

Renderer::~Renderer()
{
    UnloadModel(sphere_);
    UnloadTexture(asteroidSprite_);
}

void Renderer::configure_clip_planes()
{
    rlSetClipPlanes(0.1, 10000.0);
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
    //DrawGrid(200, 20.0f);

    for (const CelestialBody &body : bodies)
    {
        const Vector3 renderPosition = {
            body.position.x - origin.x,
            body.position.y - origin.y,
            body.position.z - origin.z,
        };

        // Asteroids are massless test particles drawn as a single camera-facing
        // billboard quad: one polygon per asteroid is dramatically cheaper than a
        // mesh across a belt of thousands. Massive bodies (sun, planets, moons)
        // use an actual low-poly sphere mesh so they keep real 3D volume.
        if (body.mass == 0.0f)
        {
            DrawBillboard(camera, asteroidSprite_, renderPosition, body.radius * 2.0f, WHITE);
        }
        else
        {
            DrawModelEx(
                sphere_,
                renderPosition,
                {0.0f, 1.0f, 0.0f},
                0.0f,
                {body.radius, body.radius, body.radius},
                WHITE);
        }
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

    // EndDrawing() does the buffer swap and the SetTargetFPS wait. Under
    // software OpenGL it also performs the actual pixel rasterization on the
    // CPU, so this "present" phase captures the real drawing cost that the
    // render timer above deliberately excludes.
    const auto presentStart = std::chrono::steady_clock::now();
    EndDrawing();
    const auto presentEnd = std::chrono::steady_clock::now();
    const double presentSampleMs = std::chrono::duration<double, std::milli>(presentEnd - presentStart).count();
    profiling::presentSampleMs = presentSampleMs;
    profiling::accumulate(profiling::presentMs, presentSampleMs);
}

#include "camera.h"
#include "input.h"
#include "physics.h"
#include "profiling.h"
#include "render.h"
#include "spawner.h"

#include "raylib.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <thread>

namespace {

// Parse the optional asteroids-per-belt count from the command line. Accepts a
// single non-negative integer argument; on missing or invalid input the default
// from the spawner is used so the program always starts with a sensible scene.
int parse_asteroids_per_belt(int argc, char **argv)
{
    if (argc < 2)
    {
        return spawner::kDefaultAsteroidsPerBelt;
    }

    char *end = nullptr;
    const long value = std::strtol(argv[1], &end, 10);

    if (end == argv[1] || *end != '\0' || value < 0)
    {
        std::fprintf(
            stderr, "Invalid asteroid count '%s'; using default of %d per belt.\n",
            argv[1], spawner::kDefaultAsteroidsPerBelt);
        return spawner::kDefaultAsteroidsPerBelt;
    }

    return static_cast<int>(value);
}

} // namespace

int main(int argc, char **argv)
{
    const int asteroidsPerBelt = parse_asteroids_per_belt(argc, argv);

    const int screenWidth = 800;
    const int screenHeight = 450;

    if (std::getenv("DISPLAY") == nullptr)
    {
        std::fprintf(stderr, "Failed to create raylib window: DISPLAY is not set.\n");
        std::fprintf(stderr, "Run this from a graphical desktop session, or set up X forwarding/Xvfb.\n");
        return 1;
    }

    InitWindow(screenWidth, screenHeight, "Solar");
    if (!IsWindowReady())
    {
        std::fprintf(stderr, "Failed to create raylib window. Make sure a graphical display is available.\n");
        return 1;
    }

    SetTargetFPS(60);

    Renderer::configure_clip_planes();

    // Scope the renderer so its destructor (which unloads GPU assets) runs
    // while the window is still alive, before CloseWindow below.
    {
        Renderer renderer;

        std::vector<CelestialBody> bodies = spawner::create_solar_system(asteroidsPerBelt);
        Camera3D camera = camera::create();

        while (!WindowShouldClose())
        {
            const auto frameStart = std::chrono::steady_clock::now();

            auto frameTime = GetFrameTime();
            physics::update(bodies, frameTime);
            input::update_camera(camera, frameTime);
            renderer.draw(camera, bodies);

            // Total frame time, measured independently of the per-function
            // timers. Includes the vsync / 60 FPS wait inside EndDrawing.
            const auto frameEnd = std::chrono::steady_clock::now();
            profiling::tick(
                std::chrono::duration<double, std::milli>(frameEnd - frameStart).count());
        }
    }

    CloseWindow();

    // Window is gone, but the process is still alive: emit the final timings so
    // they remain visible in the terminal after the program exits.
    profiling::report();

    return 0;
}

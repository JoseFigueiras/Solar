#include "camera.h"
#include "input.h"
#include "physics.h"
#include "profiling.h"
#include "render.h"

#include "raylib.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

int main()
{
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

    // Scope the renderer so its destructor (which unloads GPU assets) runs
    // while the window is still alive, before CloseWindow below.
    {
        Renderer renderer;

        std::vector<CelestialBody> bodies = physics::create_solar_system();
        Camera3D camera = camera::create();

        while (!WindowShouldClose())
        {
            const auto frameStart = std::chrono::steady_clock::now();

            physics::update(bodies, GetFrameTime());
            input::update_camera(camera, GetFrameTime());
            renderer.draw(camera, bodies);

            // Total frame time, measured independently of the per-function
            // timers. Includes the vsync / 60 FPS wait inside EndDrawing.
            const auto frameEnd = std::chrono::steady_clock::now();
            profiling::tick(
                std::chrono::duration<double, std::milli>(frameEnd - frameStart).count());
        }
    }

    CloseWindow();

    return 0;
}

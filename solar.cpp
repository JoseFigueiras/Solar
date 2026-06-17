#include "physics.h"
#include "render.h"

#include "raylib.h"

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

        while (!WindowShouldClose())
        {
            physics::update(bodies, GetFrameTime());
            renderer.draw(bodies);
        }
    }

    CloseWindow();

    return 0;
}

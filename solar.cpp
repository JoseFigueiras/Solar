#include "raylib.h"
#include "vec2.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

struct CelestialBody {
    Vec2 position;
    float radius;
    Color color;
};

struct Camera2DWorld {
    Vec2 position;
    float pixelsPerWorldUnit;
    int screenWidth;
    int screenHeight;
};

static Vector2 world_to_screen(const Camera2DWorld &camera, Vec2 point)
{
    return {
        (point.x - camera.position.x) * camera.pixelsPerWorldUnit + camera.screenWidth / 2.0f,
        camera.screenHeight / 2.0f - (point.y - camera.position.y) * camera.pixelsPerWorldUnit,
    };
}

static void update_orbit(CelestialBody &first, CelestialBody &second, float time)
{
    constexpr float orbitRadius = 3.0f;
    constexpr float orbitSpeed = 1.2f;

    const float angle = time * orbitSpeed;
    const float x = std::cos(angle) * orbitRadius;
    const float y = std::sin(angle) * orbitRadius;

    first.position = {x, y};
    second.position = {-x, -y};
}

static void draw_body(const CelestialBody &body, const Camera2DWorld &camera)
{
    const Vector2 screenPosition = world_to_screen(camera, body.position);
    DrawCircleV(screenPosition, body.radius * camera.pixelsPerWorldUnit, body.color);
}

static void draw_orbit_path(const Camera2DWorld &camera, float radius)
{
    const Vector2 center = world_to_screen(camera, {0.0f, 0.0f});
    DrawCircleLines(
        static_cast<int>(center.x),
        static_cast<int>(center.y),
        radius * camera.pixelsPerWorldUnit,
        Fade(LIGHTGRAY, 0.45f));
}

static void render(const CelestialBody &first, const CelestialBody &second, const Camera2DWorld &camera)
{
    BeginDrawing();
    ClearBackground(BLACK);

    draw_orbit_path(camera, 3.0f);
    draw_body(first, camera);
    draw_body(second, camera);

    EndDrawing();
}

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

    CelestialBody first = {{0.0f, 0.0f}, 0.35f, GOLD};
    CelestialBody second = {{0.0f, 0.0f}, 0.35f, SKYBLUE};
    Camera2DWorld camera = {{0.0f, 0.0f}, 60.0f, screenWidth, screenHeight};

    while (!WindowShouldClose())
    {
        update_orbit(first, second, static_cast<float>(GetTime()));
        render(first, second, camera);
    }

    CloseWindow();

    return 0;
}

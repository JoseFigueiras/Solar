#include "raylib.h"

static void render()
{
    BeginDrawing();
    ClearBackground(PINK);
    EndDrawing();
}

int main()
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Solar");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        render();
    }

    CloseWindow();

    return 0;
}

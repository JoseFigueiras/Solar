#include "render.h"

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

} // namespace

Renderer::Renderer()
    : sphere_(LoadModelFromMesh(GenMeshSphere(1.0f, 64, 32)))
    , camera_{}
{
    camera_.position = {0.0f, 8.5f, 15.5f};
    camera_.target = {0.0f, 0.0f, 0.0f};
    camera_.up = {0.0f, 1.0f, 0.0f};
    camera_.fovy = 45.0f;
    camera_.projection = CAMERA_PERSPECTIVE;
}

Renderer::~Renderer()
{
    UnloadModel(sphere_);
}

void Renderer::draw(
    const std::vector<CelestialBody> &bodies) const
{
    // Heliocentric view: draw everything relative to the sun so it stays centered,
    // while the underlying physics remains a full N-body simulation.
    const Vector3 origin = bodies.empty() ? Vector3{0.0f, 0.0f, 0.0f} : bodies[0].position;

    BeginDrawing();
    ClearBackground({3, 5, 12, 255});

    BeginMode3D(camera_);
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

    EndDrawing();
}

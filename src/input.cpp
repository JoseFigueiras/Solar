#include "input.h"

#include <cmath>

namespace input {

void update_camera(Camera3D &camera, float deltaSeconds)
{
    constexpr float rotationSpeed = 1.5f; // radians per second
    constexpr float minPitch = -1.5533f;  // clamp just shy of +/- 90 degrees
    constexpr float maxPitch = 1.5533f;    // to avoid flipping over the poles
    constexpr float zoomSpeed = 150.0f;    // units per second
    constexpr float minRadius = 2.0f;      // don't pass through the target
    constexpr float maxRadius = 2000.0f;   // don't fly off into the void

    // Current camera offset from the target, expressed in spherical coords.
    Vector3 offset = {
        camera.position.x - camera.target.x,
        camera.position.y - camera.target.y,
        camera.position.z - camera.target.z,
    };

    float radius = std::sqrt(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
    float yaw = std::atan2(offset.x, offset.z);
    float pitch = std::asin(offset.y / radius);

    const float step = rotationSpeed * deltaSeconds;
    if (IsKeyDown(KEY_LEFT))  yaw -= step;
    if (IsKeyDown(KEY_RIGHT)) yaw += step;
    if (IsKeyDown(KEY_UP))    pitch += step;
    if (IsKeyDown(KEY_DOWN))  pitch -= step;

    const float zoomStep = zoomSpeed * deltaSeconds;
    if (IsKeyDown(KEY_W)) radius -= zoomStep;
    if (IsKeyDown(KEY_S)) radius += zoomStep;

    if (pitch < minPitch) pitch = minPitch;
    if (pitch > maxPitch) pitch = maxPitch;
    if (radius < minRadius) radius = minRadius;
    if (radius > maxRadius) radius = maxRadius;

    const float cosPitch = std::cos(pitch);
    camera.position = {
        camera.target.x + radius * cosPitch * std::sin(yaw),
        camera.target.y + radius * std::sin(pitch),
        camera.target.z + radius * cosPitch * std::cos(yaw),
    };
}

} // namespace input

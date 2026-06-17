#include "input.h"

#include <cmath>

namespace input {

void update_camera(Camera3D &camera, float deltaSeconds)
{
    constexpr float rotationSpeed = 1.5f; // radians per second
    constexpr float minPitch = -1.5533f;  // clamp just shy of +/- 90 degrees
    constexpr float maxPitch = 1.5533f;    // to avoid flipping over the poles

    // Current camera offset from the target, expressed in spherical coords.
    Vector3 offset = {
        camera.position.x - camera.target.x,
        camera.position.y - camera.target.y,
        camera.position.z - camera.target.z,
    };

    const float radius = std::sqrt(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
    float yaw = std::atan2(offset.x, offset.z);
    float pitch = std::asin(offset.y / radius);

    const float step = rotationSpeed * deltaSeconds;
    if (IsKeyDown(KEY_LEFT))  yaw -= step;
    if (IsKeyDown(KEY_RIGHT)) yaw += step;
    if (IsKeyDown(KEY_UP))    pitch += step;
    if (IsKeyDown(KEY_DOWN))  pitch -= step;

    if (pitch < minPitch) pitch = minPitch;
    if (pitch > maxPitch) pitch = maxPitch;

    const float cosPitch = std::cos(pitch);
    camera.position = {
        camera.target.x + radius * cosPitch * std::sin(yaw),
        camera.target.y + radius * std::sin(pitch),
        camera.target.z + radius * cosPitch * std::cos(yaw),
    };
}

} // namespace input

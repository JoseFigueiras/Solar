#pragma once

#include "raylib.h"

namespace input {

// Orbit the camera around its target using the arrow keys. Left/Right change
// the yaw, Up/Down change the pitch. W/S move the camera closer to or further
// from the target. `deltaSeconds` keeps the movement speed frame-rate
// independent.
void update_camera(Camera3D &camera, float deltaSeconds);

} // namespace input

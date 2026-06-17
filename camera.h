#pragma once

#include "raylib.h"

namespace camera {

// Create the scene camera: a perspective view positioned to look at the
// heliocentric origin (the sun stays centered). This is the camera's initial
// state before any input moves it.
Camera3D create();

} // namespace camera

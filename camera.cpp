#include "camera.h"

namespace camera {

Camera3D create()
{
    Camera3D camera{};
    camera.position = {0.0f, 8.5f, 15.5f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

} // namespace camera

#include "raylib.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

constexpr int MaxLights = 4;

struct CelestialBody {
    Vector3 position;
    float radius;
    Color baseColor;
    float roughness;
    float brightness;
    bool emissive;
    float emissionStrength;
    Vector3 orbitCenter;
    float orbitRadius;
    float orbitSpeed;
    float orbitPhase;
    float verticalOffset;
};

struct ShaderLocations {
    int viewPosition;
    int baseColor;
    int roughness;
    int brightness;
    int emissive;
    int emissionStrength;
    int lightCount;
    int lightPositions;
    int lightColors;
    int lightStrengths;
};

static constexpr const char *LightingVertexShader = R"(
#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;

uniform mat4 mvp;
uniform mat4 matModel;

out vec3 fragPosition;
out vec3 fragNormal;

void main()
{
    vec4 worldPosition = matModel * vec4(vertexPosition, 1.0);
    fragPosition = worldPosition.xyz;
    fragNormal = normalize(mat3(transpose(inverse(matModel))) * vertexNormal);
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

static constexpr const char *LightingFragmentShader = R"(
#version 330

#define MAX_LIGHTS 4

in vec3 fragPosition;
in vec3 fragNormal;

out vec4 finalColor;

uniform vec3 viewPosition;
uniform vec3 baseColor;
uniform float roughness;
uniform float brightness;
uniform float emissive;
uniform float emissionStrength;
uniform int lightCount;
uniform vec3 lightPositions[MAX_LIGHTS];
uniform vec3 lightColors[MAX_LIGHTS];
uniform float lightStrengths[MAX_LIGHTS];

void main()
{
    vec3 color = baseColor * brightness;

    if (emissive > 0.5)
    {
        vec3 glow = color * emissionStrength;
        finalColor = vec4(glow, 1.0);
        return;
    }

    vec3 normal = normalize(fragNormal);
    vec3 viewDirection = normalize(viewPosition - fragPosition);
    vec3 litColor = color * 0.055;
    float clampedRoughness = clamp(roughness, 0.05, 1.0);
    float shininess = mix(96.0, 8.0, clampedRoughness);

    for (int i = 0; i < lightCount; i++)
    {
        vec3 lightOffset = lightPositions[i] - fragPosition;
        float distanceToLight = max(length(lightOffset), 0.001);
        vec3 lightDirection = lightOffset / distanceToLight;
        float attenuation = lightStrengths[i] / (1.0 + distanceToLight * distanceToLight * 0.12);
        float diffuse = max(dot(normal, lightDirection), 0.0);

        vec3 halfwayDirection = normalize(lightDirection + viewDirection);
        float specular = pow(max(dot(normal, halfwayDirection), 0.0), shininess);
        specular *= (1.0 - clampedRoughness) * 0.65;

        vec3 lightColor = lightColors[i] * attenuation;
        litColor += color * lightColor * diffuse;
        litColor += lightColor * specular;
    }

    finalColor = vec4(min(litColor, vec3(1.0)), 1.0);
}
)";

static Vector3 color_to_vector(Color color)
{
    return {
        static_cast<float>(color.r) / 255.0f,
        static_cast<float>(color.g) / 255.0f,
        static_cast<float>(color.b) / 255.0f,
    };
}

static Shader load_lighting_shader()
{
    Shader shader = LoadShaderFromMemory(LightingVertexShader, LightingFragmentShader);
    shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(shader, "mvp");
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");
    return shader;
}

static ShaderLocations get_shader_locations(const Shader &shader)
{
    return {
        GetShaderLocation(shader, "viewPosition"),
        GetShaderLocation(shader, "baseColor"),
        GetShaderLocation(shader, "roughness"),
        GetShaderLocation(shader, "brightness"),
        GetShaderLocation(shader, "emissive"),
        GetShaderLocation(shader, "emissionStrength"),
        GetShaderLocation(shader, "lightCount"),
        GetShaderLocation(shader, "lightPositions[0]"),
        GetShaderLocation(shader, "lightColors[0]"),
        GetShaderLocation(shader, "lightStrengths[0]"),
    };
}

static void update_orbits(std::vector<CelestialBody> &bodies, float time)
{
    for (CelestialBody &body : bodies)
    {
        if (body.orbitRadius <= 0.0f)
        {
            body.position = body.orbitCenter;
            continue;
        }

        const float angle = body.orbitPhase + time * body.orbitSpeed;
        body.position = {
            body.orbitCenter.x + std::cos(angle) * body.orbitRadius,
            body.orbitCenter.y + body.verticalOffset,
            body.orbitCenter.z + std::sin(angle) * body.orbitRadius,
        };
    }
}

static void draw_orbits(const std::vector<CelestialBody> &bodies)
{
    for (const CelestialBody &body : bodies)
    {
        if (body.orbitRadius <= 0.0f)
        {
            continue;
        }

        const Vector3 center = {
            body.orbitCenter.x,
            body.orbitCenter.y + body.verticalOffset,
            body.orbitCenter.z,
        };
        const Color orbitColor = body.emissive ? Fade(YELLOW, 0.32f) : Fade(LIGHTGRAY, 0.22f);
        DrawCircle3D(center, body.orbitRadius, {1.0f, 0.0f, 0.0f}, 90.0f, orbitColor);
    }
}

static void update_light_uniforms(
    const Shader &shader,
    const ShaderLocations &locations,
    const std::vector<CelestialBody> &bodies)
{
    std::array<Vector3, MaxLights> lightPositions = {};
    std::array<Vector3, MaxLights> lightColors = {};
    std::array<float, MaxLights> lightStrengths = {};
    int lightCount = 0;

    for (const CelestialBody &body : bodies)
    {
        if (!body.emissive || lightCount >= MaxLights)
        {
            continue;
        }

        lightPositions[lightCount] = body.position;
        lightColors[lightCount] = color_to_vector(body.baseColor);
        lightStrengths[lightCount] = body.emissionStrength;
        lightCount++;
    }

    SetShaderValue(shader, locations.lightCount, &lightCount, SHADER_UNIFORM_INT);
    SetShaderValueV(shader, locations.lightPositions, lightPositions.data(), SHADER_UNIFORM_VEC3, MaxLights);
    SetShaderValueV(shader, locations.lightColors, lightColors.data(), SHADER_UNIFORM_VEC3, MaxLights);
    SetShaderValueV(shader, locations.lightStrengths, lightStrengths.data(), SHADER_UNIFORM_FLOAT, MaxLights);
}

static void draw_body(
    const CelestialBody &body,
    const Model &sphere,
    const Shader &shader,
    const ShaderLocations &locations)
{
    const Vector3 bodyColor = color_to_vector(body.baseColor);
    const float roughness = std::clamp(body.roughness, 0.0f, 1.0f);
    const float brightness = std::max(body.brightness, 0.0f);
    const float emissive = body.emissive ? 1.0f : 0.0f;
    const float emissionStrength = std::max(body.emissionStrength, 0.0f);

    SetShaderValue(shader, locations.baseColor, &bodyColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, locations.roughness, &roughness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, locations.brightness, &brightness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, locations.emissive, &emissive, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, locations.emissionStrength, &emissionStrength, SHADER_UNIFORM_FLOAT);

    DrawModelEx(
        sphere,
        body.position,
        {0.0f, 1.0f, 0.0f},
        0.0f,
        {body.radius, body.radius, body.radius},
        WHITE);
}

static void render(
    const std::vector<CelestialBody> &bodies,
    const Camera3D &camera,
    const Model &sphere,
    const Shader &shader,
    const ShaderLocations &locations)
{
    BeginDrawing();
    ClearBackground({3, 5, 12, 255});

    BeginMode3D(camera);
    DrawGrid(24, 1.0f);
    draw_orbits(bodies);
    update_light_uniforms(shader, locations, bodies);

    for (const CelestialBody &body : bodies)
    {
        draw_body(body, sphere, shader, locations);
    }
    EndMode3D();

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

    Shader shader = load_lighting_shader();
    const ShaderLocations shaderLocations = get_shader_locations(shader);

    Mesh sphereMesh = GenMeshSphere(1.0f, 64, 32);
    Model sphere = LoadModelFromMesh(sphereMesh);
    sphere.materials[0].shader = shader;

    std::vector<CelestialBody> bodies = {
        {{0.0f, 0.0f, 0.0f}, 1.35f, GOLD, 0.18f, 1.15f, true, 3.9f, {0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f, 0.0f},
        {{0.0f, 0.0f, 0.0f}, 0.24f, ORANGE, 0.42f, 0.72f, false, 0.0f, {0.0f, 0.0f, 0.0f}, 2.4f, 1.18f, 0.2f, 0.0f},
        {{0.0f, 0.0f, 0.0f}, 0.38f, SKYBLUE, 0.28f, 0.86f, false, 0.0f, {0.0f, 0.0f, 0.0f}, 3.5f, 0.78f, 1.8f, 0.08f},
        {{0.0f, 0.0f, 0.0f}, 0.55f, GREEN, 0.64f, 0.68f, false, 0.0f, {0.0f, 0.0f, 0.0f}, 4.8f, 0.49f, 3.4f, -0.06f},
        {{0.0f, 0.0f, 0.0f}, 0.31f, VIOLET, 0.82f, 0.58f, false, 0.0f, {0.0f, 0.0f, 0.0f}, 6.2f, 0.34f, 4.7f, 0.16f},
        {{0.0f, 0.0f, 0.0f}, 0.68f, BROWN, 0.72f, 0.62f, false, 0.0f, {0.0f, 0.0f, 0.0f}, 7.8f, 0.23f, 2.9f, -0.12f},
        {{0.0f, 0.0f, 0.0f}, 0.46f, {185, 220, 255, 255}, 0.12f, 1.05f, true, 1.45f, {0.0f, 0.0f, 0.0f}, 9.4f, 0.16f, 5.3f, 0.24f},
    };

    Camera3D camera = {};
    camera.position = {0.0f, 8.5f, 15.5f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose())
    {
        update_orbits(bodies, static_cast<float>(GetTime()));

        SetShaderValue(shader, shaderLocations.viewPosition, &camera.position, SHADER_UNIFORM_VEC3);
        render(bodies, camera, sphere, shader, shaderLocations);
    }

    UnloadModel(sphere);
    UnloadShader(shader);
    CloseWindow();

    return 0;
}

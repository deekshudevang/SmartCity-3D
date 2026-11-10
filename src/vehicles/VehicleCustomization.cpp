#include "smartcity/vehicles/VehicleCustomization.h"
#include "scene/ResourceManager.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>

namespace SmartCity::vehicles {

VehicleCustomizationRenderer::VehicleCustomizationRenderer() = default;
VehicleCustomizationRenderer::~VehicleCustomizationRenderer() = default;

void VehicleCustomizationRenderer::EnsureMeshes() {
    if (m_cubeMesh && m_cylinderMesh && m_quadMesh) return;

    // 1. Cube Mesh (unit box centered at origin)
    std::vector<gfx::Vertex> cubeVertices = {
        // Front face (+Z)
        { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f) },
        { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec4(1.0f) },
        // Back face (-Z)
        { glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f) },
        { glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec4(1.0f) },
        // Left face (-X)
        { glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f) },
        { glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec4(1.0f) },
        // Right face (+X)
        { glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f) },
        { glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec4(1.0f) },
        // Top face (+Y)
        { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f) },
        { glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec4(1.0f) },
        // Bottom face (-Y)
        { glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f) },
        { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec4(1.0f) }
    };

    std::vector<unsigned int> cubeIndices;
    for (unsigned int i = 0; i < 6; ++i) {
        unsigned int b = i * 4;
        cubeIndices.push_back(b + 0); cubeIndices.push_back(b + 1); cubeIndices.push_back(b + 2);
        cubeIndices.push_back(b + 2); cubeIndices.push_back(b + 3); cubeIndices.push_back(b + 0);
    }
    m_cubeMesh = std::make_unique<gfx::Mesh>(cubeVertices, cubeIndices);

    // 2. Cylinder Mesh for wheels (axis along X, radius 0.5, length 1.0)
    const int segments = 16;
    std::vector<gfx::Vertex> cylVertices;
    std::vector<unsigned int> cylIndices;

    for (int i = 0; i <= segments; ++i) {
        float theta = static_cast<float>(i) * glm::two_pi<float>() / static_cast<float>(segments);
        float y = std::cos(theta) * 0.5f;
        float z = std::sin(theta) * 0.5f;
        glm::vec3 normal(0.0f, std::cos(theta), std::sin(theta));

        // Left rim vertex (-X)
        cylVertices.push_back({ glm::vec3(-0.5f, y, z), normal, glm::vec2(static_cast<float>(i) / segments, 0.0f), glm::vec4(1.0f) });
        // Right rim vertex (+X)
        cylVertices.push_back({ glm::vec3( 0.5f, y, z), normal, glm::vec2(static_cast<float>(i) / segments, 1.0f), glm::vec4(1.0f) });
    }

    for (unsigned int i = 0; i < segments; ++i) {
        unsigned int b = i * 2;
        cylIndices.push_back(b + 0); cylIndices.push_back(b + 2); cylIndices.push_back(b + 1);
        cylIndices.push_back(b + 1); cylIndices.push_back(b + 2); cylIndices.push_back(b + 3);
    }
    m_cylinderMesh = std::make_unique<gfx::Mesh>(cylVertices, cylIndices);

    // 3. Horizontal Quad Mesh (on XZ plane, y = 0)
    std::vector<gfx::Vertex> quadVertices = {
        { glm::vec3(-0.5f, 0.0f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3( 0.5f, 0.0f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec4(1.0f) },
        { glm::vec3( 0.5f, 0.0f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f) },
        { glm::vec3(-0.5f, 0.0f,  0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec4(1.0f) }
    };
    std::vector<unsigned int> quadIndices = { 0, 1, 2, 2, 3, 0 };
    m_quadMesh = std::make_unique<gfx::Mesh>(quadVertices, quadIndices);
}

void VehicleCustomizationRenderer::RenderVehicle(const VehiclePreset& preset,
                                                const glm::vec3& position,
                                                float headingRad,
                                                float wheelRotationAngle,
                                                float steeringAngleRad,
                                                float acceleration,
                                                bool isTurningLeft,
                                                bool isTurningRight,
                                                gfx::Shader* sceneShader,
                                                gfx::Shader* emissiveShader) {
    if (!sceneShader) return;
    EnsureMeshes();

    glm::mat4 base = glm::translate(glm::mat4(1.0f), position);
    base = glm::rotate(base, headingRad, glm::vec3(0.0f, 1.0f, 0.0f));

    // 1. Neon Underglow (rendered first so chassis doesn't clip)
    if (preset.neon.hasNeon && (preset.neon.left || preset.neon.right || preset.neon.front || preset.neon.back)) {
        if (emissiveShader) {
            RenderNeonUnderglow(preset, base, emissiveShader);
        }
    }

    // 2. 3D Body Mesh
    if (preset.IsModelAvailable()) {
        auto model = scene::ResourceManager::Get().GetModel(preset.modelAssetPath);
        if (model) {
            glm::vec3 priRgb = preset.GetEffectivePrimaryColorRgb();
            sceneShader->Bind();
            sceneShader->SetInt("u_UseColorOverride", 1);
            sceneShader->SetVec4("u_ColorOverride", glm::vec4(priRgb, 1.0f));
            sceneShader->SetMat4("u_Model", base);
            model->Draw(sceneShader);
            sceneShader->SetInt("u_UseColorOverride", 0);
        } else {
            RenderPlaceholderChassis(preset, base, sceneShader, emissiveShader);
        }
    } else {
        RenderPlaceholderChassis(preset, base, sceneShader, emissiveShader);
    }

    // 3. Wheels
    RenderWheels(preset, base, wheelRotationAngle, steeringAngleRad, sceneShader);

    // 4. Lights (Headlights, Brake lights, Turn signals)
    if (emissiveShader) {
        RenderLights(preset, base, acceleration, isTurningLeft, isTurningRight, emissiveShader);
    }
}

void VehicleCustomizationRenderer::RenderPlaceholderChassis(const VehiclePreset& preset,
                                                            const glm::mat4& baseTransform,
                                                            gfx::Shader* sceneShader,
                                                            gfx::Shader* emissiveShader) {
    sceneShader->Bind();
    sceneShader->SetInt("u_UseColorOverride", 1);

    const auto& dims = preset.simProfile.dimensions; // length (X), width (Y), height (Z) in dims vector: x=length, y=width, z=height
    float length = dims.x;
    float width = dims.y;
    float height = dims.z;

    glm::vec3 priColor = preset.GetEffectivePrimaryColorRgb();
    glm::vec3 secColor = preset.GetEffectiveSecondaryColorRgb();

    // Lower Chassis (Primary color)
    float chassisY = height * 0.28f;
    float chassisH = height * 0.44f;
    glm::mat4 chassisMat = glm::translate(baseTransform, glm::vec3(0.0f, chassisY, 0.0f));
    chassisMat = glm::scale(chassisMat, glm::vec3(width, chassisH, length));

    sceneShader->SetVec4("u_ColorOverride", glm::vec4(priColor, 1.0f));
    sceneShader->SetMat4("u_Model", chassisMat);
    m_cubeMesh->Draw();

    // Cabin / Greenhouses (Secondary color / tint)
    float cabinH = height * 0.42f;
    float cabinY = chassisY + chassisH * 0.5f + cabinH * 0.5f;
    float cabinLen = (preset.category == VehicleCategory::Supercar) ? (length * 0.45f) : (length * 0.55f);
    float cabinZ = -length * 0.06f;

    glm::mat4 cabinMat = glm::translate(baseTransform, glm::vec3(0.0f, cabinY, cabinZ));
    cabinMat = glm::scale(cabinMat, glm::vec3(width * 0.84f, cabinH, cabinLen));

    // Tinted window shade
    glm::vec3 windowColor = secColor * 0.6f + glm::vec3(0.1f, 0.12f, 0.15f);
    sceneShader->SetVec4("u_ColorOverride", glm::vec4(windowColor, 1.0f));
    sceneShader->SetMat4("u_Model", cabinMat);
    m_cubeMesh->Draw();

    // Rear Spoiler (if mod0 spoiler is equipped or if supercar)
    bool hasSpoiler = (preset.category == VehicleCategory::Supercar);
    auto itMod = preset.modifications.find(0);
    if (itMod != preset.modifications.end() && itMod->second.present && itMod->second.index > 0) {
        hasSpoiler = true;
    }
    if (hasSpoiler) {
        float spoilerY = chassisY + chassisH * 0.5f + 0.22f;
        float spoilerZ = -length * 0.44f;
        glm::mat4 wingMat = glm::translate(baseTransform, glm::vec3(0.0f, spoilerY, spoilerZ));
        wingMat = glm::scale(wingMat, glm::vec3(width * 0.90f, 0.05f, 0.35f));
        sceneShader->SetVec4("u_ColorOverride", glm::vec4(secColor * 0.4f, 1.0f));
        sceneShader->SetMat4("u_Model", wingMat);
        m_cubeMesh->Draw();
    }

    sceneShader->SetInt("u_UseColorOverride", 0);
}

void VehicleCustomizationRenderer::RenderWheels(const VehiclePreset& preset,
                                               const glm::mat4& baseTransform,
                                               float wheelRotationAngle,
                                               float steeringAngleRad,
                                               gfx::Shader* sceneShader) {
    sceneShader->Bind();
    sceneShader->SetInt("u_UseColorOverride", 1);
    // Dark tire rubber
    sceneShader->SetVec4("u_ColorOverride", glm::vec4(0.12f, 0.12f, 0.12f, 1.0f));

    float axleZ = preset.simProfile.axleZ > 0.0f ? preset.simProfile.axleZ : (preset.simProfile.dimensions.x * 0.32f);
    float halfW = preset.simProfile.halfTrackWidth > 0.0f ? preset.simProfile.halfTrackWidth : (preset.simProfile.dimensions.y * 0.46f);
    float radius = preset.simProfile.wheelRadius > 0.0f ? preset.simProfile.wheelRadius : 0.35f;
    float wheelWidth = 0.26f;

    auto drawOneWheel = [&](float x, float z, bool steer) {
        glm::mat4 w = glm::translate(baseTransform, glm::vec3(x, radius, z));
        if (steer) {
            w = glm::rotate(w, steeringAngleRad, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        w = glm::rotate(w, wheelRotationAngle, glm::vec3(1.0f, 0.0f, 0.0f));
        w = glm::scale(w, glm::vec3(wheelWidth, radius * 2.0f, radius * 2.0f));

        sceneShader->SetMat4("u_Model", w);
        m_cylinderMesh->Draw();
    };

    // Front Wheels (Steering active)
    drawOneWheel(-halfW,  axleZ, true);
    drawOneWheel( halfW,  axleZ, true);

    // Rear Wheels (Fixed)
    drawOneWheel(-halfW, -axleZ, false);
    drawOneWheel( halfW, -axleZ, false);

    sceneShader->SetInt("u_UseColorOverride", 0);
}

void VehicleCustomizationRenderer::RenderNeonUnderglow(const VehiclePreset& preset,
                                                       const glm::mat4& baseTransform,
                                                       gfx::Shader* emissiveShader) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for neon glow
    glDepthMask(GL_FALSE);

    emissiveShader->Bind();
    glm::vec3 col = glm::vec3(
        static_cast<float>(preset.neon.color.r) / 255.0f,
        static_cast<float>(preset.neon.color.g) / 255.0f,
        static_cast<float>(preset.neon.color.b) / 255.0f
    );

    float length = preset.simProfile.dimensions.x * 0.95f;
    float width = preset.simProfile.dimensions.y * 1.25f;

    glm::mat4 neonMat = glm::translate(baseTransform, glm::vec3(0.0f, 0.03f, 0.0f));
    neonMat = glm::scale(neonMat, glm::vec3(width, 1.0f, length));

    emissiveShader->SetVec4("u_EmissiveColor", glm::vec4(col, 0.65f));
    emissiveShader->SetMat4("u_Model", neonMat);
    m_quadMesh->Draw();

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void VehicleCustomizationRenderer::RenderLights(const VehiclePreset& preset,
                                               const glm::mat4& baseTransform,
                                               float acceleration,
                                               bool isTurningLeft,
                                               bool isTurningRight,
                                               gfx::Shader* emissiveShader) {
    emissiveShader->Bind();

    float length = preset.simProfile.dimensions.x;
    float width = preset.simProfile.dimensions.y;
    float height = preset.simProfile.dimensions.z;

    float lightY = height * 0.28f;
    float frontZ = length * 0.505f;
    float rearZ = -length * 0.505f;
    float spacing = width * 0.36f;

    // 1. Front Headlights (Bright warm white)
    glm::vec4 headColor(1.0f, 0.98f, 0.90f, 1.0f);
    emissiveShader->SetVec4("u_EmissiveColor", headColor);

    glm::mat4 hlLeft = glm::translate(baseTransform, glm::vec3(-spacing, lightY, frontZ));
    hlLeft = glm::scale(hlLeft, glm::vec3(0.24f, 0.12f, 0.05f));
    emissiveShader->SetMat4("u_Model", hlLeft);
    m_cubeMesh->Draw();

    glm::mat4 hlRight = glm::translate(baseTransform, glm::vec3(spacing, lightY, frontZ));
    hlRight = glm::scale(hlRight, glm::vec3(0.24f, 0.12f, 0.05f));
    emissiveShader->SetMat4("u_Model", hlRight);
    m_cubeMesh->Draw();

    // 2. Rear Taillights (Red: bright red if braking/decelerating)
    bool isBraking = (acceleration < -0.2f);
    glm::vec4 tailColor = isBraking ? glm::vec4(1.0f, 0.05f, 0.05f, 1.0f) : glm::vec4(0.55f, 0.03f, 0.03f, 1.0f);
    emissiveShader->SetVec4("u_EmissiveColor", tailColor);

    glm::mat4 tlLeft = glm::translate(baseTransform, glm::vec3(-spacing, lightY, rearZ));
    tlLeft = glm::scale(tlLeft, glm::vec3(0.24f, 0.12f, 0.05f));
    emissiveShader->SetMat4("u_Model", tlLeft);
    m_cubeMesh->Draw();

    glm::mat4 tlRight = glm::translate(baseTransform, glm::vec3(spacing, lightY, rearZ));
    tlRight = glm::scale(tlRight, glm::vec3(0.24f, 0.12f, 0.05f));
    emissiveShader->SetMat4("u_Model", tlRight);
    m_cubeMesh->Draw();
}

void VehicleCustomizationRenderer::RenderShadowDepth(const VehiclePreset& preset,
                                                     const glm::vec3& position,
                                                     float headingRad,
                                                     gfx::Shader* shadowShader) {
    if (!shadowShader) return;
    EnsureMeshes();

    const auto& dims = preset.simProfile.dimensions;
    glm::mat4 base = glm::translate(glm::mat4(1.0f), position);
    base = glm::rotate(base, headingRad, glm::vec3(0.0f, 1.0f, 0.0f));

    // Shadow chassis
    float chassisY = dims.z * 0.28f;
    glm::mat4 chassis = glm::translate(base, glm::vec3(0.0f, chassisY, 0.0f));
    chassis = glm::scale(chassis, glm::vec3(dims.y, dims.z * 0.44f, dims.x));
    shadowShader->SetMat4("u_Model", chassis);
    m_cubeMesh->Draw();

    // Shadow cabin
    float cabinY = chassisY + dims.z * 0.43f;
    glm::mat4 cabin = glm::translate(base, glm::vec3(0.0f, cabinY, -dims.x * 0.06f));
    cabin = glm::scale(cabin, glm::vec3(dims.y * 0.84f, dims.z * 0.42f, dims.x * 0.50f));
    shadowShader->SetMat4("u_Model", cabin);
    m_cubeMesh->Draw();
}

void VehicleCustomizationRenderer::RenderPreview(const VehiclePreset& preset,
                                                 float turntableAngleRad,
                                                 float wheelRotationAngle,
                                                 float steeringAngleRad,
                                                 gfx::Shader* sceneShader,
                                                 gfx::Shader* emissiveShader) {
    RenderVehicle(preset, glm::vec3(0.0f), turntableAngleRad,
                  wheelRotationAngle, steeringAngleRad,
                  0.0f, false, false,
                  sceneShader, emissiveShader);
}

} // namespace SmartCity::vehicles


#pragma once

#include "smartcity/vehicles/VehiclePreset.h"
#include "gfx/Shader.h"
#include "gfx/Mesh.h"
#include "scene/Model.h"
#include <glm/glm.hpp>
#include <memory>

namespace SmartCity::vehicles {

class VehicleCustomizationRenderer {
public:
    VehicleCustomizationRenderer();
    ~VehicleCustomizationRenderer();

    // Renders a vehicle preset in world space at given position, heading, velocity, and steering angle
    void RenderVehicle(const VehiclePreset& preset,
                       const glm::vec3& position,
                       float headingRad,
                       float wheelRotationAngle,
                       float steeringAngleRad,
                       float acceleration,
                       bool isTurningLeft,
                       bool isTurningRight,
                       gfx::Shader* sceneShader,
                       gfx::Shader* emissiveShader);

    // Renders shadow depth for shadow mapping
    void RenderShadowDepth(const VehiclePreset& preset,
                           const glm::vec3& position,
                           float headingRad,
                           gfx::Shader* shadowShader);

    // Renders the vehicle inside the interactive preview panel (orbit camera, turntable rotation)
    void RenderPreview(const VehiclePreset& preset,
                       float turntableAngleRad,
                       float wheelRotationAngle,
                       float steeringAngleRad,
                       gfx::Shader* sceneShader,
                       gfx::Shader* emissiveShader);

private:
    void EnsureMeshes();
    void RenderPlaceholderChassis(const VehiclePreset& preset,
                                 const glm::mat4& baseTransform,
                                 gfx::Shader* sceneShader,
                                 gfx::Shader* emissiveShader);
    void RenderWheels(const VehiclePreset& preset,
                     const glm::mat4& baseTransform,
                     float wheelRotationAngle,
                     float steeringAngleRad,
                     gfx::Shader* sceneShader);
    void RenderNeonUnderglow(const VehiclePreset& preset,
                            const glm::mat4& baseTransform,
                            gfx::Shader* emissiveShader);
    void RenderLights(const VehiclePreset& preset,
                     const glm::mat4& baseTransform,
                     float acceleration,
                     bool isTurningLeft,
                     bool isTurningRight,
                     gfx::Shader* emissiveShader);

    std::unique_ptr<gfx::Mesh> m_cubeMesh;
    std::unique_ptr<gfx::Mesh> m_cylinderMesh;
    std::unique_ptr<gfx::Mesh> m_quadMesh;

    float m_indicatorBlinkTimer{ 0.0f };
    bool m_indicatorBlinkState{ false };
};

} // namespace SmartCity::vehicles


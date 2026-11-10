#include <iostream>
#include <cassert>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gfx/Camera.h"
#include "core/Logger.h"

int main() {
    SmartCity::core::Logger::Init("logs/test_camera.log");
    SmartCity::core::Logger::Info("Running unit test_camera...");

    // Radians internally: 45 deg = 0.785398 rad
    SmartCity::gfx::Camera camera(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
    assert(std::abs(camera.GetFOVDegrees() - 45.0f) < 1e-4f);
    assert(camera.GetMode() == SmartCity::gfx::CameraMode::Orbit);
    assert(camera.GetCurrentPreset() == SmartCity::gfx::CameraPreset::Overview);

    const glm::mat4& viewMat = camera.GetViewMatrix();
    float det = glm::determinant(glm::mat3(viewMat));
    assert(std::abs(det - 1.0f) < 1e-4f);
    SmartCity::core::Logger::Info("[PASS] View matrix orthogonality (det = 1.0)");

    const glm::mat4& projMat = camera.GetProjectionMatrix();
    assert(std::abs(projMat[2][3] - (-1.0f)) < 1e-4f);
    SmartCity::core::Logger::Info("[PASS] Perspective projection matrix properties");

    camera.ApplyPreset(SmartCity::gfx::CameraPreset::TopDown);
    assert(camera.GetCurrentPreset() == SmartCity::gfx::CameraPreset::TopDown);
    assert(std::abs(camera.GetPitchDegrees() - (-89.0f)) < 1e-4f);
    SmartCity::core::Logger::Info("[PASS] Top-down camera preset");

    camera.ApplyPreset(SmartCity::gfx::CameraPreset::PedestrianView);
    assert(camera.GetCurrentPreset() == SmartCity::gfx::CameraPreset::PedestrianView);
    assert(camera.GetOrbitDistance() == 14.0f);
    SmartCity::core::Logger::Info("[PASS] Pedestrian view preset");

    camera.SetOrbitDistance(500.0f);
    assert(camera.GetOrbitDistance() <= 250.0f);
    camera.SetOrbitDistance(1.0f);
    assert(camera.GetOrbitDistance() >= 5.0f);
    SmartCity::core::Logger::Info("[PASS] Orbit distance clamping");

    SmartCity::core::Logger::Info("All test_camera unit tests PASSED successfully!");
    SmartCity::core::Logger::Shutdown();
    return 0;
}

#include <iostream>
#include <cassert>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "graphics/Camera.h"
#include "core/Logger.h"

int main() {
    SmartCity::Logger::Info("Running test_camera...");

    // Test 1: Camera initialization & View Matrix
    SmartCity::Camera camera(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    assert(camera.GetFOV() == 45.0f);
    assert(camera.GetMode() == SmartCity::CameraMode::Orbit);
    assert(camera.GetCurrentPreset() == SmartCity::CameraPreset::Overview);

    const glm::mat4& viewMat = camera.GetViewMatrix();
    // Determinant of rigid-body view matrix should be +1 (orthogonal rotation)
    float det = glm::determinant(glm::mat3(viewMat));
    assert(std::abs(det - 1.0f) < 1e-4f);
    SmartCity::Logger::Info("[PASS] View matrix orthogonality");

    // Test 2: Projection Matrix properties
    const glm::mat4& projMat = camera.GetProjectionMatrix();
    // Perspective projection matrix element [3][2] is -1.0 for standard OpenGL perspective
    assert(std::abs(projMat[2][3] - (-1.0f)) < 1e-4f);
    SmartCity::Logger::Info("[PASS] Perspective projection matrix properties");

    // Test 3: Preset transitions
    camera.ApplyPreset(SmartCity::CameraPreset::TopDown);
    assert(camera.GetCurrentPreset() == SmartCity::CameraPreset::TopDown);
    assert(camera.GetPitch() == -89.0f);
    SmartCity::Logger::Info("[PASS] Top-down camera preset");

    camera.ApplyPreset(SmartCity::CameraPreset::PedestrianView);
    assert(camera.GetCurrentPreset() == SmartCity::CameraPreset::PedestrianView);
    assert(camera.GetOrbitDistance() == 14.0f);
    SmartCity::Logger::Info("[PASS] Pedestrian view preset");

    // Test 4: Distance clamp
    camera.SetOrbitDistance(500.0f);
    assert(camera.GetOrbitDistance() <= 250.0f); // clamped to max
    camera.SetOrbitDistance(1.0f);
    assert(camera.GetOrbitDistance() >= 5.0f);   // clamped to min
    SmartCity::Logger::Info("[PASS] Orbit distance clamping");

    SmartCity::Logger::Info("All test_camera tests PASSED successfully!");
    return 0;
}

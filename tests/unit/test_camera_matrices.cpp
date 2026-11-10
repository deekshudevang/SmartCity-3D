#include "gfx/Camera.h"
#include "core/Logger.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace SmartCity::gfx;
using namespace SmartCity::core;

int main() {
    Logger::Init("logs/test_camera_matrices.log");
    Logger::Info("Running unit test_camera_matrices...");

    // 1. Perspective Matrix verification against analytical hand-computed values
    // FOV = 90 deg (PI/2), aspect = 1.0, near = 1.0, far = 10.0
    float fovRad = glm::radians(90.0f);
    float aspect = 1.0f;
    float nearClip = 1.0f;
    float farClip = 10.0f;

    Camera camPersp(fovRad, aspect, nearClip, farClip);
    camPersp.SetProjectionType(ProjectionType::Perspective);

    const glm::mat4& pMat = camPersp.GetProjectionMatrix();

    // Hand calculations:
    // cot(45 deg) = 1.0
    // M[0][0] = cot / aspect = 1.0
    // M[1][1] = cot = 1.0
    // M[2][2] = -(far + near) / (far - near) = -(11) / 9 = -1.222222...
    // M[2][3] = -1.0
    // M[3][2] = -(2 * far * near) / (far - near) = -(20) / 9 = -2.222222...
    assert(std::abs(pMat[0][0] - 1.0f) < 1e-4f);
    assert(std::abs(pMat[1][1] - 1.0f) < 1e-4f);
    assert(std::abs(pMat[2][2] - (-11.0f / 9.0f)) < 1e-4f);
    assert(std::abs(pMat[2][3] - (-1.0f)) < 1e-4f);
    assert(std::abs(pMat[3][2] - (-20.0f / 9.0f)) < 1e-4f);
    assert(std::abs(pMat[3][3] - 0.0f) < 1e-4f);
    Logger::Info("[PASS] Perspective projection matrix matches analytical values.");

    // 2. Orthographic Matrix verification against analytical hand-computed values
    // orthoSize = 10.0, aspect = 1.0, near = 1.0, far = 10.0
    Camera camOrtho(fovRad, aspect, nearClip, farClip);
    camOrtho.SetProjectionType(ProjectionType::Orthographic);
    camOrtho.SetOrthoSize(10.0f);

    const glm::mat4& oMat = camOrtho.GetProjectionMatrix();

    // Hand calculations:
    // width = 2 * orthoSize * aspect = 20.0 -> M[0][0] = 2 / 20 = 0.1
    // height = 2 * orthoSize = 20.0        -> M[1][1] = 2 / 20 = 0.1
    // depth = far - near = 9.0             -> M[2][2] = -2 / 9 = -0.222222...
    // M[3][2] = -(far + near) / (far - near) = -11 / 9 = -1.222222...
    // M[3][3] = 1.0
    assert(std::abs(oMat[0][0] - 0.1f) < 1e-4f);
    assert(std::abs(oMat[1][1] - 0.1f) < 1e-4f);
    assert(std::abs(oMat[2][2] - (-2.0f / 9.0f)) < 1e-4f);
    assert(std::abs(oMat[3][2] - (-11.0f / 9.0f)) < 1e-4f);
    assert(std::abs(oMat[3][3] - 1.0f) < 1e-4f);
    Logger::Info("[PASS] Orthographic projection matrix matches analytical values.");

    // 3. Runtime Toggle between Perspective and Orthographic
    Camera toggleCam;
    assert(toggleCam.GetProjectionType() == ProjectionType::Perspective);
    toggleCam.ToggleProjectionType();
    assert(toggleCam.GetProjectionType() == ProjectionType::Orthographic);
    // Ortho matrix has M[3][3] == 1.0
    assert(std::abs(toggleCam.GetProjectionMatrix()[3][3] - 1.0f) < 1e-4f);

    toggleCam.ToggleProjectionType();
    assert(toggleCam.GetProjectionType() == ProjectionType::Perspective);
    // Perspective matrix has M[2][3] == -1.0
    assert(std::abs(toggleCam.GetProjectionMatrix()[2][3] - (-1.0f)) < 1e-4f);
    Logger::Info("[PASS] Runtime projection toggle verified.");

    Logger::Info("test_camera_matrices PASSED successfully.");
    Logger::Shutdown();
    return 0;
}


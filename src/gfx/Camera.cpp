#include "gfx/Camera.h"
#include "core/Input.h"
#include <GLFW/glfw3.h>
#include <algorithm>

namespace SmartCity::gfx {

Camera::Camera(float fovRad, float aspectRatio, float nearClip, float farClip)
    : m_mode(CameraMode::Orbit)
    , m_currentPreset(CameraPreset::Overview)
    , m_position(0.0f, 45.0f, 55.0f)
    , m_target(0.0f, 0.0f, 0.0f)
    , m_forward(0.0f, -0.6f, -0.8f)
    , m_up(0.0f, 1.0f, 0.0f)
    , m_right(1.0f, 0.0f, 0.0f)
    , m_fovRad(fovRad)
    , m_aspectRatio(aspectRatio)
    , m_nearClip(nearClip)
    , m_farClip(farClip)
    , m_orbitDistance(70.0f)
    , m_yawRad(glm::radians(-45.0f))
    , m_pitchRad(glm::radians(-38.0f))
    , m_moveSpeed(25.0f)
    , m_mouseSensitivity(0.003f) // in radians per pixel
{
    ApplyPreset(CameraPreset::Overview);
}

void Camera::SetViewportSize(int width, int height) {
    if (height > 0) {
        m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        RecalculateProjectionMatrix();
    }
}

void Camera::ApplyPreset(CameraPreset preset) {
    m_currentPreset = preset;
    switch (preset) {
        case CameraPreset::Overview:
            m_mode = CameraMode::Orbit;
            m_target = glm::vec3(0.0f, 0.0f, 0.0f);
            m_orbitDistance = 75.0f;
            m_yawRad = glm::radians(-45.0f);
            m_pitchRad = glm::radians(-38.0f);
            break;
        case CameraPreset::TopDown:
            m_mode = CameraMode::Orbit;
            m_target = glm::vec3(0.0f, 0.0f, 0.0f);
            m_orbitDistance = 85.0f;
            m_yawRad = glm::radians(-90.0f);
            m_pitchRad = glm::radians(-89.0f);
            break;
        case CameraPreset::NorthTrafficCam:
            m_mode = CameraMode::Orbit;
            m_target = glm::vec3(0.0f, 1.0f, 0.0f);
            m_orbitDistance = 45.0f;
            m_yawRad = glm::radians(-90.0f); // looking South towards +Z
            m_pitchRad = glm::radians(-22.0f);
            break;
        case CameraPreset::EastTrafficCam:
            m_mode = CameraMode::Orbit;
            m_target = glm::vec3(0.0f, 1.0f, 0.0f);
            m_orbitDistance = 45.0f;
            m_yawRad = glm::radians(180.0f); // looking West towards -X
            m_pitchRad = glm::radians(-22.0f);
            break;
        case CameraPreset::PedestrianView:
            m_mode = CameraMode::Orbit;
            m_target = glm::vec3(8.0f, 0.5f, 8.0f);
            m_orbitDistance = 14.0f;
            m_yawRad = glm::radians(-135.0f);
            m_pitchRad = glm::radians(-8.0f);
            break;
        case CameraPreset::Custom:
            break;
    }
    RecalculateVectors();
}

void Camera::SetOrbitTarget(const glm::vec3& target) {
    m_target = target;
    RecalculateVectors();
}

void Camera::SetOrbitDistance(float distance) {
    m_orbitDistance = std::clamp(distance, 5.0f, 250.0f);
    RecalculateVectors();
}

void Camera::Update(float dt) {
    glm::vec2 mouseDelta = core::Input::GetMouseDelta();
    float scrollDelta = core::Input::GetMouseScrollDelta();

    if (scrollDelta != 0.0f) {
        if (m_mode == CameraMode::Orbit) {
            SetOrbitDistance(m_orbitDistance - scrollDelta * 3.5f);
            m_currentPreset = CameraPreset::Custom;
        } else {
            m_moveSpeed = std::clamp(m_moveSpeed + scrollDelta * 5.0f, 5.0f, 100.0f);
        }
    }

    if (core::Input::GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)) {
        m_yawRad += mouseDelta.x * m_mouseSensitivity;
        m_pitchRad -= mouseDelta.y * m_mouseSensitivity;
        float maxPitch = glm::radians(89.0f);
        m_pitchRad = std::clamp(m_pitchRad, -maxPitch, maxPitch);
        m_currentPreset = CameraPreset::Custom;
        RecalculateVectors();
    }

    if (m_mode == CameraMode::Orbit && core::Input::GetMouseButton(GLFW_MOUSE_BUTTON_MIDDLE)) {
        float panSpeed = 0.035f * (m_orbitDistance / 50.0f);
        m_target -= m_right * (mouseDelta.x * panSpeed);
        m_target += m_up * (mouseDelta.y * panSpeed);
        m_currentPreset = CameraPreset::Custom;
        RecalculateVectors();
    }

    if (m_mode == CameraMode::FreeFly) {
        float velocity = m_moveSpeed * dt;
        if (core::Input::GetKey(GLFW_KEY_LEFT_SHIFT)) velocity *= 2.0f;

        if (core::Input::GetKey(GLFW_KEY_W)) m_position += m_forward * velocity;
        if (core::Input::GetKey(GLFW_KEY_S)) m_position -= m_forward * velocity;
        if (core::Input::GetKey(GLFW_KEY_A)) m_position -= m_right * velocity;
        if (core::Input::GetKey(GLFW_KEY_D)) m_position += m_right * velocity;
        if (core::Input::GetKey(GLFW_KEY_E) || core::Input::GetKey(GLFW_KEY_SPACE)) m_position += glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
        if (core::Input::GetKey(GLFW_KEY_Q) || core::Input::GetKey(GLFW_KEY_LEFT_CONTROL)) m_position -= glm::vec3(0.0f, 1.0f, 0.0f) * velocity;

        RecalculateViewMatrix();
    }
}

void Camera::RecalculateVectors() {
    glm::vec3 front;
    front.x = std::cos(m_pitchRad) * std::cos(m_yawRad);
    front.y = std::sin(m_pitchRad);
    front.z = std::cos(m_pitchRad) * std::sin(m_yawRad);
    m_forward = glm::normalize(front);

    m_right = glm::normalize(glm::cross(m_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_up = glm::normalize(glm::cross(m_right, m_forward));

    if (m_mode == CameraMode::Orbit) {
        m_position = m_target - m_forward * m_orbitDistance;
    }

    RecalculateViewMatrix();
}

void Camera::RecalculateViewMatrix() {
    m_viewMatrix = glm::lookAt(m_position, m_position + m_forward, m_up);
}

void Camera::RecalculateProjectionMatrix() {
    m_projectionMatrix = glm::perspective(m_fovRad, m_aspectRatio, m_nearClip, m_farClip);
}

} // namespace SmartCity::gfx

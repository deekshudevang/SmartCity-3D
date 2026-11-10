#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace SmartCity::gfx {

enum class CameraMode {
    Orbit,
    FreeFly
};

enum class CameraPreset {
    Overview,
    TopDown,
    NorthTrafficCam,
    EastTrafficCam,
    PedestrianView,
    Custom
};

class Camera {
public:
    // Units: radians internally for FOV, yaw, pitch
    Camera(float fovRad = glm::radians(45.0f), float aspectRatio = 16.0f / 9.0f, float nearClip = 0.1f, float farClip = 1000.0f);

    void Update(float dt);
    void SetViewportSize(int width, int height);

    const glm::mat4& GetViewMatrix() const { return m_viewMatrix; }
    const glm::mat4& GetProjectionMatrix() const { return m_projectionMatrix; }
    glm::mat4 GetViewProjectionMatrix() const { return m_projectionMatrix * m_viewMatrix; }

    const glm::vec3& GetPosition() const { return m_position; }
    const glm::vec3& GetForward() const { return m_forward; }
    const glm::vec3& GetTarget() const { return m_target; }

    void SetMode(CameraMode mode) { m_mode = mode; }
    CameraMode GetMode() const { return m_mode; }
    void ApplyPreset(CameraPreset preset);
    CameraPreset GetCurrentPreset() const { return m_currentPreset; }

    void SetOrbitTarget(const glm::vec3& target);
    void SetOrbitDistance(float distance);
    float GetOrbitDistance() const { return m_orbitDistance; }

    void SetPosition(const glm::vec3& pos) { m_position = pos; RecalculateViewMatrix(); }
    void SetYawPitchRad(float yawRad, float pitchRad) { m_yawRad = yawRad; m_pitchRad = pitchRad; RecalculateVectors(); }

    // Internal radians
    float GetYawRad() const { return m_yawRad; }
    float GetPitchRad() const { return m_pitchRad; }
    float GetFOVRad() const { return m_fovRad; }
    void SetFOVRad(float fovRad) { m_fovRad = fovRad; RecalculateProjectionMatrix(); }

    // UI helpers (degrees)
    float GetYawDegrees() const { return glm::degrees(m_yawRad); }
    float GetPitchDegrees() const { return glm::degrees(m_pitchRad); }
    float GetFOVDegrees() const { return glm::degrees(m_fovRad); }
    void SetFOVDegrees(float fovDeg) { SetFOVRad(glm::radians(fovDeg)); }

private:
    void RecalculateProjectionMatrix();
    void RecalculateViewMatrix();
    void RecalculateVectors();

    CameraMode m_mode;
    CameraPreset m_currentPreset;

    glm::vec3 m_position;
    glm::vec3 m_target;
    glm::vec3 m_forward;
    glm::vec3 m_up;
    glm::vec3 m_right;

    float m_fovRad;
    float m_aspectRatio;
    float m_nearClip;
    float m_farClip;

    float m_orbitDistance;
    float m_yawRad;   // radians internally
    float m_pitchRad; // radians internally

    float m_moveSpeed;
    float m_mouseSensitivity;

    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
};

} // namespace SmartCity::gfx

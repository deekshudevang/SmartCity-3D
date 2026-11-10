#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace SmartCity {

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
    Camera(float fov = 45.0f, float aspectRatio = 16.0f / 9.0f, float nearClip = 0.1f, float farClip = 1000.0f);

    void Update(float dt);
    void SetViewportSize(int width, int height);

    // Matrix accessors
    const glm::mat4& GetViewMatrix() const { return m_viewMatrix; }
    const glm::mat4& GetProjectionMatrix() const { return m_projectionMatrix; }
    glm::mat4 GetViewProjectionMatrix() const { return m_projectionMatrix * m_viewMatrix; }

    const glm::vec3& GetPosition() const { return m_position; }
    const glm::vec3& GetForward() const { return m_forward; }
    const glm::vec3& GetTarget() const { return m_target; }

    // Mode & Presets
    void SetMode(CameraMode mode) { m_mode = mode; }
    CameraMode GetMode() const { return m_mode; }
    void ApplyPreset(CameraPreset preset);
    CameraPreset GetCurrentPreset() const { return m_currentPreset; }

    // Orbit controls
    void SetOrbitTarget(const glm::vec3& target);
    void SetOrbitDistance(float distance);
    float GetOrbitDistance() const { return m_orbitDistance; }

    // Direct orientation setters
    void SetPosition(const glm::vec3& pos) { m_position = pos; RecalculateViewMatrix(); }
    void SetYawPitch(float yaw, float pitch) { m_yaw = yaw; m_pitch = pitch; RecalculateVectors(); }

    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }
    float GetFOV() const { return m_fov; }
    void SetFOV(float fov) { m_fov = fov; RecalculateProjectionMatrix(); }

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

    float m_fov;
    float m_aspectRatio;
    float m_nearClip;
    float m_farClip;

    // Orbit variables
    float m_orbitDistance;
    float m_yaw;   // in degrees
    float m_pitch; // in degrees

    // Speed configuration
    float m_moveSpeed;
    float m_mouseSensitivity;

    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
};

} // namespace SmartCity

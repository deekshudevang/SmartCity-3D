#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace SmartCity::scene {

class Transform {
public:
    Transform(const glm::vec3& position = glm::vec3(0.0f),
              const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
              const glm::vec3& scale = glm::vec3(1.0f));

    void SetPosition(const glm::vec3& pos);
    const glm::vec3& GetPosition() const { return m_position; }

    void SetRotation(const glm::quat& rot);
    const glm::quat& GetRotation() const { return m_rotation; }

    void SetScale(const glm::vec3& scale);
    const glm::vec3& GetScale() const { return m_scale; }

    // Euler angle helpers (in radians)
    void SetEulerAngles(const glm::vec3& eulerRad);
    glm::vec3 GetEulerAngles() const;

    // Euler angle helpers (in degrees for UI)
    void SetEulerDegrees(const glm::vec3& eulerDeg);
    glm::vec3 GetEulerDegrees() const;

    void Translate(const glm::vec3& delta);
    void Rotate(float angleRad, const glm::vec3& axis);
    void RotateEuler(const glm::vec3& deltaRad);

    void SetParent(Transform* parent);
    Transform* GetParent() const { return m_parent; }

    const glm::mat4& GetLocalMatrix() const;
    glm::mat4 GetWorldMatrix() const;
    glm::mat3 GetNormalMatrix() const;

    bool IsDirty() const { return m_isDirty; }
    void MarkDirty();

private:
    void UpdateLocalMatrix() const;

    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_scale;

    mutable glm::mat4 m_localMatrix;
    mutable bool m_isDirty;

    Transform* m_parent;
};

} // namespace SmartCity::scene


#include "scene/Transform.h"
#include <glm/gtc/matrix_transform.hpp>

namespace SmartCity::scene {

Transform::Transform(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
    : m_position(position)
    , m_rotation(rotation)
    , m_scale(scale)
    , m_localMatrix(1.0f)
    , m_isDirty(true)
    , m_parent(nullptr)
{
}

void Transform::SetPosition(const glm::vec3& pos) {
    m_position = pos;
    MarkDirty();
}

void Transform::SetRotation(const glm::quat& rot) {
    m_rotation = glm::normalize(rot);
    MarkDirty();
}

void Transform::SetScale(const glm::vec3& scale) {
    m_scale = scale;
    MarkDirty();
}

void Transform::SetEulerAngles(const glm::vec3& eulerRad) {
    m_rotation = glm::quat(eulerRad);
    MarkDirty();
}

glm::vec3 Transform::GetEulerAngles() const {
    return glm::eulerAngles(m_rotation);
}

void Transform::SetEulerDegrees(const glm::vec3& eulerDeg) {
    SetEulerAngles(glm::radians(eulerDeg));
}

glm::vec3 Transform::GetEulerDegrees() const {
    return glm::degrees(GetEulerAngles());
}

void Transform::Translate(const glm::vec3& delta) {
    m_position += delta;
    MarkDirty();
}

void Transform::Rotate(float angleRad, const glm::vec3& axis) {
    glm::quat q = glm::angleAxis(angleRad, glm::normalize(axis));
    m_rotation = glm::normalize(q * m_rotation);
    MarkDirty();
}

void Transform::RotateEuler(const glm::vec3& deltaRad) {
    glm::quat q = glm::quat(deltaRad);
    m_rotation = glm::normalize(q * m_rotation);
    MarkDirty();
}

void Transform::SetParent(Transform* parent) {
    m_parent = parent;
    MarkDirty();
}

void Transform::MarkDirty() {
    m_isDirty = true;
}

void Transform::UpdateLocalMatrix() const {
    if (m_isDirty) {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), m_position);
        glm::mat4 r = glm::toMat4(m_rotation);
        glm::mat4 s = glm::scale(glm::mat4(1.0f), m_scale);
        m_localMatrix = t * r * s;
        m_isDirty = false;
    }
}

const glm::mat4& Transform::GetLocalMatrix() const {
    UpdateLocalMatrix();
    return m_localMatrix;
}

glm::mat4 Transform::GetWorldMatrix() const {
    UpdateLocalMatrix();
    if (m_parent) {
        return m_parent->GetWorldMatrix() * m_localMatrix;
    }
    return m_localMatrix;
}

glm::mat3 Transform::GetNormalMatrix() const {
    glm::mat4 world = GetWorldMatrix();
    return glm::transpose(glm::inverse(glm::mat3(world)));
}

} // namespace SmartCity::scene


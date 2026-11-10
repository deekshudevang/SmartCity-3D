#include "scene/SceneObject.h"

namespace SmartCity::scene {

SceneObject::SceneObject(uint32_t id, const std::string& name)
    : m_id(id)
    , m_name(name)
    , m_localAABB(glm::vec3(-0.5f), glm::vec3(0.5f))
    , m_isVisible(true)
{
}

void SceneObject::SetModel(std::shared_ptr<Model> model) {
    m_model = model;
    if (m_model) {
        m_localAABB = m_model->GetLocalBounds();
    }
}

void SceneObject::SetMesh(std::shared_ptr<gfx::Mesh> mesh) {
    m_mesh = mesh;
}

void SceneObject::SetMaterial(std::shared_ptr<gfx::Material> material) {
    m_material = material;
}

gfx::AABB SceneObject::GetWorldAABB() const {
    return m_localAABB.Transform(m_transform.GetWorldMatrix());
}

void SceneObject::AddChild(std::shared_ptr<SceneObject> child) {
    if (child) {
        child->GetTransform().SetParent(&m_transform);
        m_children.push_back(child);
    }
}

} // namespace SmartCity::scene


#pragma once

#include <string>
#include <memory>
#include <vector>
#include "scene/Transform.h"
#include "scene/Model.h"
#include "gfx/Mesh.h"
#include "gfx/Material.h"
#include "gfx/Frustum.h"

namespace SmartCity::scene {

class SceneObject {
public:
    SceneObject(uint32_t id, const std::string& name);

    uint32_t GetId() const { return m_id; }
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    Transform& GetTransform() { return m_transform; }
    const Transform& GetTransform() const { return m_transform; }

    void SetModel(std::shared_ptr<Model> model);
    std::shared_ptr<Model> GetModel() const { return m_model; }

    void SetMesh(std::shared_ptr<gfx::Mesh> mesh);
    std::shared_ptr<gfx::Mesh> GetMesh() const { return m_mesh; }

    void SetMaterial(std::shared_ptr<gfx::Material> material);
    std::shared_ptr<gfx::Material> GetMaterial() const { return m_material; }

    void SetVisible(bool visible) { m_isVisible = visible; }
    bool IsVisible() const { return m_isVisible; }

    void SetLocalAABB(const gfx::AABB& aabb) { m_localAABB = aabb; }
    const gfx::AABB& GetLocalAABB() const { return m_localAABB; }
    gfx::AABB GetWorldAABB() const;

    void AddChild(std::shared_ptr<SceneObject> child);
    const std::vector<std::shared_ptr<SceneObject>>& GetChildren() const { return m_children; }

private:
    uint32_t m_id;
    std::string m_name;
    Transform m_transform;

    std::shared_ptr<Model> m_model;
    std::shared_ptr<gfx::Mesh> m_mesh;
    std::shared_ptr<gfx::Material> m_material;

    gfx::AABB m_localAABB;
    bool m_isVisible;

    std::vector<std::shared_ptr<SceneObject>> m_children;
};

} // namespace SmartCity::scene


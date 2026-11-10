#pragma once

#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "gfx/Mesh.h"
#include "gfx/Material.h"
#include "gfx/Frustum.h"

namespace SmartCity::scene {

struct ModelNode {
    std::string name;
    glm::mat4 transform{ 1.0f };
    std::vector<uint32_t> meshIndices;
    std::vector<ModelNode> children;
};

class Model {
public:
    Model(const std::string& filepath);
    // Explicit constructor to build model from in-memory mesh & material
    Model(std::shared_ptr<gfx::Mesh> mesh, std::shared_ptr<gfx::Material> material);
    ~Model() = default;

    void Draw(gfx::Shader* shader) const;

    const std::vector<std::shared_ptr<gfx::Mesh>>& GetMeshes() const { return m_meshes; }
    const std::vector<std::shared_ptr<gfx::Material>>& GetMaterials() const { return m_materials; }
    const std::vector<glm::mat4>& GetMeshTransforms() const { return m_meshTransforms; }

    const gfx::AABB& GetLocalBounds() const { return m_localBounds; }
    const std::string& GetFilepath() const { return m_filepath; }
    bool IsPlaceholder() const { return m_isPlaceholder; }

    static std::shared_ptr<Model> CreatePlaceholderModel();

private:
    void LoadModel(const std::string& filepath);
    void CreateFallbackPlaceholder();
    void CalculateBounds();

    std::string m_filepath;
    std::vector<std::shared_ptr<gfx::Mesh>> m_meshes;
    std::vector<std::shared_ptr<gfx::Material>> m_materials;
    std::vector<glm::mat4> m_meshTransforms;
    gfx::AABB m_localBounds;
    bool m_isPlaceholder;
};

} // namespace SmartCity::scene


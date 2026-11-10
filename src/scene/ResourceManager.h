#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include "gfx/Shader.h"
#include "gfx/Texture2D.h"
#include "gfx/Mesh.h"
#include "gfx/Material.h"
#include "scene/Model.h"

namespace SmartCity::scene {

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    // Singleton or shared instance accessor
    static ResourceManager& Get();

    std::shared_ptr<gfx::Shader> GetShader(const std::string& name, const std::string& vertPath = "", const std::string& fragPath = "");
    std::shared_ptr<gfx::Texture2D> GetTexture(const std::string& path);
    std::shared_ptr<Model> GetModel(const std::string& path);
    std::shared_ptr<gfx::Material> GetMaterial(const std::string& name);

    void RegisterMesh(const std::string& name, std::shared_ptr<gfx::Mesh> mesh);
    std::shared_ptr<gfx::Mesh> GetMesh(const std::string& name);

    void RegisterMaterial(const std::string& name, std::shared_ptr<gfx::Material> material);

    uint32_t GetDiskLoadCount() const { return m_diskLoadCount; }
    uint32_t GetCacheHitCount() const { return m_cacheHitCount; }

    size_t GetShaderCount() const { return m_shaders.size(); }
    size_t GetTextureCount() const { return m_textures.size(); }
    size_t GetMeshCount() const { return m_meshes.size(); }
    size_t GetModelCount() const { return m_models.size(); }
    size_t GetMaterialCount() const { return m_materials.size(); }

    void Shutdown();

private:
    std::string CanonicalizePath(const std::string& path) const;

    std::unordered_map<std::string, std::shared_ptr<gfx::Shader>> m_shaders;
    std::unordered_map<std::string, std::shared_ptr<gfx::Texture2D>> m_textures;
    std::unordered_map<std::string, std::shared_ptr<gfx::Mesh>> m_meshes;
    std::unordered_map<std::string, std::shared_ptr<Model>> m_models;
    std::unordered_map<std::string, std::shared_ptr<gfx::Material>> m_materials;

    uint32_t m_diskLoadCount;
    uint32_t m_cacheHitCount;
};

} // namespace SmartCity::scene


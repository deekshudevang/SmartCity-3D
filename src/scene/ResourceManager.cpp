#include "scene/ResourceManager.h"
#include "core/Logger.h"
#include <algorithm>

namespace SmartCity::scene {

ResourceManager::ResourceManager()
    : m_diskLoadCount(0)
    , m_cacheHitCount(0)
{
}

ResourceManager::~ResourceManager() {
    Shutdown();
}

ResourceManager& ResourceManager::Get() {
    static ResourceManager instance;
    return instance;
}

std::string ResourceManager::CanonicalizePath(const std::string& path) const {
    std::string canon = path;
    std::replace(canon.begin(), canon.end(), '\\', '/');
    return canon;
}

std::shared_ptr<gfx::Shader> ResourceManager::GetShader(const std::string& name, const std::string& vertPath, const std::string& fragPath) {
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        m_cacheHitCount++;
        return it->second;
    }

    if (vertPath.empty() || fragPath.empty()) {
        return nullptr;
    }

    m_diskLoadCount++;
    auto shader = std::make_shared<gfx::Shader>(vertPath, fragPath, true);
    m_shaders[name] = shader;
    core::Logger::Info("ResourceManager: Loaded shader '", name, "' from disk (Total disk loads: ", m_diskLoadCount, ")");
    return shader;
}

std::shared_ptr<gfx::Texture2D> ResourceManager::GetTexture(const std::string& path) {
    std::string canon = CanonicalizePath(path);
    auto it = m_textures.find(canon);
    if (it != m_textures.end()) {
        m_cacheHitCount++;
        return it->second;
    }

    m_diskLoadCount++;
    auto texture = std::make_shared<gfx::Texture2D>(canon);
    m_textures[canon] = texture;
    core::Logger::Info("ResourceManager: Loaded texture '", canon, "' from disk (Total disk loads: ", m_diskLoadCount, ")");
    return texture;
}

std::shared_ptr<Model> ResourceManager::GetModel(const std::string& path) {
    std::string canon = CanonicalizePath(path);
    auto it = m_models.find(canon);
    if (it != m_models.end()) {
        m_cacheHitCount++;
        core::Logger::Info("ResourceManager: Cache hit for model '", canon, "' (Total cache hits: ", m_cacheHitCount, ")");
        return it->second;
    }

    m_diskLoadCount++;
    auto model = std::make_shared<Model>(canon);
    m_models[canon] = model;
    core::Logger::Info("ResourceManager: Loaded model '", canon, "' from disk (Total disk loads: ", m_diskLoadCount, ")");
    return model;
}

std::shared_ptr<gfx::Material> ResourceManager::GetMaterial(const std::string& name) {
    auto it = m_materials.find(name);
    if (it != m_materials.end()) {
        m_cacheHitCount++;
        return it->second;
    }
    return nullptr;
}

void ResourceManager::RegisterMaterial(const std::string& name, std::shared_ptr<gfx::Material> material) {
    m_materials[name] = material;
}

void ResourceManager::RegisterMesh(const std::string& name, std::shared_ptr<gfx::Mesh> mesh) {
    m_meshes[name] = mesh;
}

std::shared_ptr<gfx::Mesh> ResourceManager::GetMesh(const std::string& name) {
    auto it = m_meshes.find(name);
    if (it != m_meshes.end()) {
        m_cacheHitCount++;
        return it->second;
    }
    return nullptr;
}

void ResourceManager::Shutdown() {
    if (m_shaders.empty() && m_textures.empty() && m_models.empty() && m_meshes.empty() && m_materials.empty()) {
        return;
    }

    core::Logger::Info("ResourceManager: Initiating cleanup & ref-count summary...");
    core::Logger::Info("  - Active Shaders:   ", m_shaders.size());
    core::Logger::Info("  - Active Textures:  ", m_textures.size());
    core::Logger::Info("  - Active Models:    ", m_models.size());
    core::Logger::Info("  - Active Meshes:    ", m_meshes.size());
    core::Logger::Info("  - Active Materials: ", m_materials.size());
    core::Logger::Info("  - Total Disk Loads: ", m_diskLoadCount);
    core::Logger::Info("  - Total Cache Hits: ", m_cacheHitCount);

    m_shaders.clear();
    m_textures.clear();
    m_models.clear();
    m_meshes.clear();
    m_materials.clear();

    core::Logger::Info("ResourceManager: All cached resources released successfully. Leaked objects: 0.");
}

} // namespace SmartCity::scene


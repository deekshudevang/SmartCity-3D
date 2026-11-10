#include "gfx/Material.h"

namespace SmartCity::gfx {

uint32_t Material::s_nextMaterialId = 1;

Material::Material(const std::string& name, std::shared_ptr<Shader> shader)
    : m_id(s_nextMaterialId++)
    , m_name(name)
    , m_shader(shader)
    , m_ambient(0.2f, 0.2f, 0.2f)
    , m_diffuse(0.8f, 0.8f, 0.8f)
    , m_specular(0.5f, 0.5f, 0.5f)
    , m_shininess(32.0f)
    , m_diffuseMap(nullptr)
    , m_specularMap(nullptr)
    , m_normalMap(nullptr)
{
}

uint64_t Material::GetSortKey() const {
    uint64_t shaderId = m_shader ? static_cast<uint64_t>(m_shader->GetRendererId()) : 0ULL;
    return (shaderId << 32) | static_cast<uint64_t>(m_id);
}

void Material::Bind() const {
    if (!m_shader) return;

    m_shader->Bind();
    m_shader->SetVec3("u_Material.ambient", m_ambient);
    m_shader->SetVec3("u_Material.diffuse", m_diffuse);
    m_shader->SetVec3("u_Material.specular", m_specular);
    m_shader->SetFloat("u_Material.shininess", m_shininess);

    if (m_diffuseMap) {
        m_diffuseMap->Bind(0);
        m_shader->SetInt("u_Material.diffuseMap", 0);
        m_shader->SetInt("u_Material.hasDiffuseMap", 1);
    } else {
        m_shader->SetInt("u_Material.hasDiffuseMap", 0);
    }

    if (m_specularMap) {
        m_specularMap->Bind(1);
        m_shader->SetInt("u_Material.specularMap", 1);
        m_shader->SetInt("u_Material.hasSpecularMap", 1);
    } else {
        m_shader->SetInt("u_Material.hasSpecularMap", 0);
    }

    if (m_normalMap) {
        m_normalMap->Bind(2);
        m_shader->SetInt("u_Material.normalMap", 2);
        m_shader->SetInt("u_Material.hasNormalMap", 1);
    } else {
        m_shader->SetInt("u_Material.hasNormalMap", 0);
    }
}

} // namespace SmartCity::gfx


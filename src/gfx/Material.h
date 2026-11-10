#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "gfx/Shader.h"
#include "gfx/Texture2D.h"

namespace SmartCity::gfx {

class Material {
public:
    Material(const std::string& name, std::shared_ptr<Shader> shader);

    void SetAmbient(const glm::vec3& ambient) { m_ambient = ambient; }
    const glm::vec3& GetAmbient() const { return m_ambient; }

    void SetDiffuse(const glm::vec3& diffuse) { m_diffuse = diffuse; }
    const glm::vec3& GetDiffuse() const { return m_diffuse; }

    void SetSpecular(const glm::vec3& specular) { m_specular = specular; }
    const glm::vec3& GetSpecular() const { return m_specular; }

    void SetShininess(float shininess) { m_shininess = shininess; }
    float GetShininess() const { return m_shininess; }

    void SetDiffuseMap(std::shared_ptr<Texture2D> texture) { m_diffuseMap = texture; }
    std::shared_ptr<Texture2D> GetDiffuseMap() const { return m_diffuseMap; }

    void SetSpecularMap(std::shared_ptr<Texture2D> texture) { m_specularMap = texture; }
    std::shared_ptr<Texture2D> GetSpecularMap() const { return m_specularMap; }

    void SetNormalMap(std::shared_ptr<Texture2D> texture) { m_normalMap = texture; }
    std::shared_ptr<Texture2D> GetNormalMap() const { return m_normalMap; }

    void SetShader(std::shared_ptr<Shader> shader) { m_shader = shader; }
    std::shared_ptr<Shader> GetShader() const { return m_shader; }

    uint32_t GetId() const { return m_id; }
    const std::string& GetName() const { return m_name; }

    // Generates a 64-bit sort key (upper 32 bits: Shader program ID, lower 32 bits: Material ID)
    uint64_t GetSortKey() const;

    void Bind() const;

private:
    static uint32_t s_nextMaterialId;

    uint32_t m_id;
    std::string m_name;
    std::shared_ptr<Shader> m_shader;

    glm::vec3 m_ambient;
    glm::vec3 m_diffuse;
    glm::vec3 m_specular;
    float m_shininess;

    std::shared_ptr<Texture2D> m_diffuseMap;
    std::shared_ptr<Texture2D> m_specularMap;
    std::shared_ptr<Texture2D> m_normalMap;
};

} // namespace SmartCity::gfx


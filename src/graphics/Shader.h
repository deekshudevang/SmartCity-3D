#pragma once

#include <string>
#include <unordered_map>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace SmartCity {

class Shader {
public:
    Shader(const std::string& vertexSrc, const std::string& fragmentSrc, bool fromFile = false);
    ~Shader();

    void Bind() const;
    void Unbind() const;

    // Uniform setters
    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetVec2(const std::string& name, const glm::vec2& value);
    void SetVec3(const std::string& name, const glm::vec3& value);
    void SetVec4(const std::string& name, const glm::vec4& value);
    void SetMat4(const std::string& name, const glm::mat4& value);

    uint32_t GetRendererId() const { return m_rendererId; }
    bool IsValid() const { return m_rendererId != 0; }

    static std::string ReadFile(const std::string& filepath);

private:
    bool Compile(const std::string& vertexSource, const std::string& fragmentSource);
    GLint GetUniformLocation(const std::string& name);

    uint32_t m_rendererId;
    std::unordered_map<std::string, GLint> m_uniformLocationCache;
};

} // namespace SmartCity

#include "gfx/Shader.h"
#include "core/Logger.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

namespace SmartCity::gfx {

Shader::Shader(const std::string& vertexInput, const std::string& fragmentInput, bool fromFile)
    : m_rendererId(0)
{
    std::string vSource = vertexInput;
    std::string fSource = fragmentInput;

    if (fromFile) {
        vSource = ReadFile(vertexInput);
        fSource = ReadFile(fragmentInput);
    }

    if (vSource.empty() || fSource.empty()) {
        core::Logger::Warn("Shader source was empty! Loading default fallback shaders.");
        vSource = 
            "#version 330 core\n"
            "layout(location = 0) in vec3 a_Position;\n"
            "layout(location = 1) in vec3 a_Normal;\n"
            "layout(location = 2) in vec2 a_TexCoord;\n"
            "layout(location = 3) in vec4 a_Color;\n"
            "uniform mat4 u_ViewProjection;\n"
            "uniform mat4 u_Model;\n"
            "out vec4 v_Color;\n"
            "void main() {\n"
            "    v_Color = a_Color;\n"
            "    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);\n"
            "}\n";
        fSource = 
            "#version 330 core\n"
            "in vec4 v_Color;\n"
            "out vec4 o_Color;\n"
            "void main() {\n"
            "    o_Color = v_Color;\n"
            "}\n";
    }

    Compile(vSource, fSource);
}

Shader::~Shader() {
    if (m_rendererId != 0) {
        glDeleteProgram(m_rendererId);
        m_rendererId = 0;
    }
}

std::string Shader::ReadFile(const std::string& filepath) {
    std::ifstream in(filepath, std::ios::in | std::ios::binary);
    if (!in) {
        in.clear();
        in.open("../" + filepath, std::ios::in | std::ios::binary);
    }
    if (!in) {
        core::Logger::Error("Could not open shader file: ", filepath);
        return "";
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool Shader::Compile(const std::string& vertexSource, const std::string& fragmentSource) {
    GLuint program = glCreateProgram();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const GLchar* vSourceC = vertexSource.c_str();
    glShaderSource(vs, 1, &vSourceC, nullptr);
    glCompileShader(vs);

    GLint isCompiled = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &maxLength);
        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(vs, maxLength, &maxLength, &infoLog[0]);
        glDeleteShader(vs);
        glDeleteProgram(program);
        core::Logger::Error("Vertex Shader Compilation Failed:\n", infoLog.data());
        return false;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* fSourceC = fragmentSource.c_str();
    glShaderSource(fs, 1, &fSourceC, nullptr);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &maxLength);
        std::vector<GLchar> infoLog(maxLength);
        glGetShaderInfoLog(fs, maxLength, &maxLength, &infoLog[0]);
        glDeleteShader(vs);
        glDeleteShader(fs);
        glDeleteProgram(program);
        core::Logger::Error("Fragment Shader Compilation Failed:\n", infoLog.data());
        return false;
    }

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE) {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
        glDeleteProgram(program);
        glDeleteShader(vs);
        glDeleteShader(fs);
        core::Logger::Error("Shader Program Linking Failed:\n", infoLog.data());
        return false;
    }

    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    m_rendererId = program;
    return true;
}

void Shader::Bind() const {
    glUseProgram(m_rendererId);
}

void Shader::Unbind() const {
    glUseProgram(0);
}

GLint Shader::GetUniformLocation(const std::string& name) {
    if (m_uniformLocationCache.find(name) != m_uniformLocationCache.end()) {
        return m_uniformLocationCache[name];
    }
    GLint location = glGetUniformLocation(m_rendererId, name.c_str());
    m_uniformLocationCache[name] = location;
    return location;
}

void Shader::SetInt(const std::string& name, int value) {
    GLint loc = GetUniformLocation(name);
    if (loc != -1) glUniform1i(loc, value);
}

void Shader::SetFloat(const std::string& name, float value) {
    GLint loc = GetUniformLocation(name);
    if (loc != -1) glUniform1f(loc, value);
}

void Shader::SetVec2(const std::string& name, const glm::vec2& value) {
    GLint loc = GetUniformLocation(name);
    if (loc != -1) glUniform2f(loc, value.x, value.y);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) {
    GLint loc = GetUniformLocation(name);
    if (loc != -1) glUniform3f(loc, value.x, value.y, value.z);
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) {
    GLint loc = GetUniformLocation(name);
    if (loc != -1) glUniform4f(loc, value.x, value.y, value.z, value.w);
}

void Shader::SetMat3(const std::string& name, const glm::mat3& value) {
    GLint loc = GetUniformLocation(name);
    if (loc != -1) glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetMat4(const std::string& name, const glm::mat4& value) {
    GLint loc = GetUniformLocation(name);
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

bool Shader::ValidateAllShaders(std::vector<std::string>& failedShaders) {
    failedShaders.clear();

    const std::vector<std::pair<std::string, std::string>> shaderPairs = {
        { "shaders/scene.vert", "shaders/scene.frag" },
        { "shaders/shadow_depth.vert", "shaders/shadow_depth.frag" },
        { "shaders/skybox.vert", "shaders/skybox.frag" },
        { "shaders/light_indicator.vert", "shaders/light_indicator.frag" },
        { "shaders/rain.vert", "shaders/rain.frag" }
    };

    core::Logger::Info("--- Validating All Project Shaders ---");
    for (const auto& pair : shaderPairs) {
        std::string vSrc = ReadFile(pair.first);
        std::string fSrc = ReadFile(pair.second);

        if (vSrc.empty()) {
            core::Logger::Error("Missing or unreadable vertex shader: ", pair.first);
            failedShaders.push_back(pair.first);
            continue;
        }
        if (fSrc.empty()) {
            core::Logger::Error("Missing or unreadable fragment shader: ", pair.second);
            failedShaders.push_back(pair.second);
            continue;
        }

        if (glad_glCreateShader != nullptr) {
            Shader testShader(pair.first, pair.second, true);
            if (!testShader.IsValid()) {
                core::Logger::Error("Failed to compile shader pair: ", pair.first, " & ", pair.second);
                failedShaders.push_back(pair.first + " + " + pair.second);
            } else {
                core::Logger::Info("Shader successfully validated: ", pair.first, " & ", pair.second);
            }
        } else {
            core::Logger::Info("Shader file present (OpenGL headless bypass): ", pair.first, " & ", pair.second);
        }
    }

    return failedShaders.empty();
}

} // namespace SmartCity::gfx

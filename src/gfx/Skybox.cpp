#include "gfx/Skybox.h"
#include "core/Logger.h"
#include <stb_image.h>

namespace SmartCity::gfx {

static const float s_skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

Skybox::Skybox(const std::vector<std::string>& facePaths)
    : m_cubemapId(0)
    , m_hasCubemap(false)
    , m_vao(0)
    , m_vbo(0)
    , m_topColor(0.25f, 0.52f, 0.85f)
    , m_horizonColor(0.68f, 0.78f, 0.90f)
    , m_groundColor(0.18f, 0.20f, 0.22f)
{
    InitCubeMesh();

    // Load skybox shader
    m_shader = std::make_unique<Shader>("shaders/skybox.vert", "shaders/skybox.frag", true);

    if (facePaths.size() == 6) {
        LoadCubemap(facePaths);
    } else {
        core::Logger::Info("No cubemap images provided. Utilizing procedural atmospheric skybox fallback.");
    }
}

Skybox::~Skybox() {
    if (m_cubemapId != 0) {
        glDeleteTextures(1, &m_cubemapId);
        m_cubemapId = 0;
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        m_vao = 0;
        m_vbo = 0;
    }
}

void Skybox::InitCubeMesh() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_skyboxVertices), s_skyboxVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

void Skybox::LoadCubemap(const std::vector<std::string>& facePaths) {
    glGenTextures(1, &m_cubemapId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapId);

    stbi_set_flip_vertically_on_load(0); // Cubemaps expect unflipped images

    bool allLoaded = true;
    for (unsigned int i = 0; i < facePaths.size(); ++i) {
        int width, height, channels;
        unsigned char* data = stbi_load(facePaths[i].c_str(), &width, &height, &channels, 0);
        if (data) {
            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            core::Logger::Warn("Failed to load skybox face '", facePaths[i], "'. Reverting to procedural skybox.");
            allLoaded = false;
            break;
        }
    }

    if (allLoaded) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        m_hasCubemap = true;
        core::Logger::Info("Cubemap skybox loaded successfully.");
    } else {
        glDeleteTextures(1, &m_cubemapId);
        m_cubemapId = 0;
        m_hasCubemap = false;
    }
}

void Skybox::Draw(const Camera& camera) {
    if (!m_shader) return;

    // Draw skybox with depth func LEQUAL so depth test passes at far plane 1.0
    glDepthFunc(GL_LEQUAL);
    m_shader->Bind();

    // Strip translation from view matrix so skybox stays centered around camera
    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(camera.GetViewMatrix()));
    glm::mat4 viewProj = camera.GetProjectionMatrix() * viewNoTranslation;
    m_shader->SetMat4("u_ViewProjection", viewProj);

    m_shader->SetInt("u_HasCubemap", m_hasCubemap ? 1 : 0);
    if (m_hasCubemap) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapId);
        m_shader->SetInt("u_Skybox", 0);
    } else {
        m_shader->SetVec3("u_TopColor", m_topColor);
        m_shader->SetVec3("u_HorizonColor", m_horizonColor);
        m_shader->SetVec3("u_GroundColor", m_groundColor);
    }

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // Restore standard depth function
    glDepthFunc(GL_LESS);
}

} // namespace SmartCity::gfx


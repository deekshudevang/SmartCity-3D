#pragma once

#include <vector>
#include <string>
#include <memory>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "gfx/Camera.h"
#include "gfx/Shader.h"

namespace SmartCity::gfx {

class Skybox {
public:
    // Takes paths to 6 cubemap face textures (right, left, top, bottom, front, back)
    // If files are empty or missing, enables procedural gradient fallback automatically
    Skybox(const std::vector<std::string>& facePaths = {});
    ~Skybox();

    void Draw(const Camera& camera);

    void SetTopColor(const glm::vec3& col) { m_topColor = col; }
    void SetHorizonColor(const glm::vec3& col) { m_horizonColor = col; }
    void SetGroundColor(const glm::vec3& col) { m_groundColor = col; }

    bool HasCubemap() const { return m_hasCubemap; }
    uint32_t GetCubemapId() const { return m_cubemapId; }

private:
    void InitCubeMesh();
    void LoadCubemap(const std::vector<std::string>& facePaths);

    uint32_t m_cubemapId;
    bool m_hasCubemap;

    uint32_t m_vao;
    uint32_t m_vbo;

    std::unique_ptr<Shader> m_shader;

    glm::vec3 m_topColor;
    glm::vec3 m_horizonColor;
    glm::vec3 m_groundColor;
};

} // namespace SmartCity::gfx


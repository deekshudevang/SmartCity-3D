#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace SmartCity {

class ShadowMap {
public:
    ShadowMap(uint32_t width = 2048, uint32_t height = 2048);
    ~ShadowMap();

    void BindForWriting();
    void Unbind(int viewportWidth, int viewportHeight);
    void BindTexture(uint32_t textureSlot = 1) const;

    glm::mat4 CalculateLightSpaceMatrix(const glm::vec3& lightDir, const glm::vec3& targetCenter = glm::vec3(0.0f)) const;

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    uint32_t GetDepthTexture() const { return m_depthTexture; }

private:
    uint32_t m_fbo;
    uint32_t m_depthTexture;
    uint32_t m_width;
    uint32_t m_height;
};

} // namespace SmartCity

#include "gfx/ShadowMap.h"
#include "core/Logger.h"
#include <glm/gtc/matrix_transform.hpp>

namespace SmartCity::gfx {

ShadowMap::ShadowMap(uint32_t width, uint32_t height)
    : m_fbo(0), m_depthTexture(0), m_width(width), m_height(height)
{
    if (glad_glGenFramebuffers == nullptr) {
        return; // Headless environment guard (null GLAD pointers in CLI unit tests)
    }

    glGenFramebuffers(1, &m_fbo);

    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        core::Logger::Error("Shadow Map Framebuffer is incomplete!");
    } else {
        core::Logger::Info("Shadow Map FBO created (", width, "x", height, ")");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

ShadowMap::~ShadowMap() {
    if (m_fbo != 0 && glad_glDeleteFramebuffers != nullptr) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_depthTexture != 0 && glad_glDeleteTextures != nullptr) {
        glDeleteTextures(1, &m_depthTexture);
        m_depthTexture = 0;
    }
}

void ShadowMap::BindForWriting() {
    if (glad_glBindFramebuffer == nullptr) return;
    glViewport(0, 0, m_width, m_height);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);
}

void ShadowMap::Unbind(int viewportWidth, int viewportHeight) {
    if (glad_glBindFramebuffer == nullptr) return;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewportWidth, viewportHeight);
    glCullFace(GL_BACK);
}

void ShadowMap::BindTexture(uint32_t textureSlot) const {
    if (glad_glBindTexture == nullptr) return;
    glActiveTexture(GL_TEXTURE0 + textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
}

glm::mat4 ShadowMap::CalculateLightSpaceMatrix(const glm::vec3& lightDir, const glm::vec3& targetCenter) const {
    float orthoSize = 95.0f;
    glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 240.0f);

    glm::vec3 normalizedLightDir = glm::normalize(lightDir);
    glm::vec3 up = (std::abs(normalizedLightDir.y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 lightPos = targetCenter - normalizedLightDir * 110.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, targetCenter, up);

    // Texel Snapping (Shadow Stabilization to eliminate camera shimmering)
    glm::vec4 shadowOrigin = lightProjection * lightView * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    shadowOrigin *= (static_cast<float>(m_width) * 0.5f);
    glm::vec4 roundedOrigin = glm::round(shadowOrigin);
    glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
    roundOffset *= (2.0f / static_cast<float>(m_width));
    roundOffset.z = 0.0f;
    roundOffset.w = 0.0f;

    lightProjection[3] += roundOffset;

    return lightProjection * lightView;
}

} // namespace SmartCity::gfx

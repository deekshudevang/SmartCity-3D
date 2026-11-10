#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "gfx/Shader.h"

namespace SmartCity::fx {

struct RainParticle {
    glm::vec3 position;
    float length;
    float speed;
};

struct RainVertex {
    glm::vec3 position;
    glm::vec4 color;
};

class RainSystem {
public:
    explicit RainSystem(size_t maxParticles = 10000);
    ~RainSystem();

    void Update(float dt, const glm::vec3& cameraPos);
    void Render(const glm::mat4& viewProjection);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetIntensity(float intensity) { m_intensity = glm::clamp(intensity, 0.0f, 1.0f); }
    float GetIntensity() const { return m_intensity; }

    size_t GetParticleCount() const { return m_particles.size(); }

private:
    void InitGL();

    size_t m_maxParticles;
    bool m_enabled{ false };
    float m_intensity{ 0.75f };

    std::vector<RainParticle> m_particles;
    std::vector<RainVertex> m_vertexData;

    uint32_t m_vao{ 0 };
    uint32_t m_vbo{ 0 };
    std::unique_ptr<gfx::Shader> m_shader;
    bool m_glInitialized{ false };
};

} // namespace SmartCity::fx

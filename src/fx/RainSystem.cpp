#include "fx/RainSystem.h"
#include <glad/glad.h>
#include "core/Logger.h"
#include <random>

namespace SmartCity::fx {

RainSystem::RainSystem(size_t maxParticles)
    : m_maxParticles(maxParticles)
    , m_enabled(false)
    , m_intensity(0.75f)
    , m_glInitialized(false)
{
    m_particles.resize(m_maxParticles);
    m_vertexData.resize(m_maxParticles * 2);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> distX(-35.0f, 35.0f);
    std::uniform_real_distribution<float> distY(0.0f, 25.0f);
    std::uniform_real_distribution<float> distZ(-35.0f, 35.0f);
    std::uniform_real_distribution<float> distLen(0.40f, 0.75f);
    std::uniform_real_distribution<float> distSpeed(18.0f, 26.0f);

    for (size_t i = 0; i < m_maxParticles; ++i) {
        m_particles[i].position = glm::vec3(distX(rng), distY(rng), distZ(rng));
        m_particles[i].length = distLen(rng);
        m_particles[i].speed = distSpeed(rng);
    }

    InitGL();
}

RainSystem::~RainSystem() {
    if (m_glInitialized && glad_glDeleteVertexArrays != nullptr) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
    }
}

void RainSystem::InitGL() {
    if (glad_glGenVertexArrays == nullptr) {
        return; // Headless environment guard
    }

    m_shader = std::make_unique<gfx::Shader>("shaders/rain.vert", "shaders/rain.frag", true);

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Allocate buffer for 2 vertices per streak (top and bottom)
    glBufferData(GL_ARRAY_BUFFER, sizeof(RainVertex) * m_maxParticles * 2, nullptr, GL_DYNAMIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RainVertex), (void*)offsetof(RainVertex, position));

    // Color attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(RainVertex), (void*)offsetof(RainVertex, color));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_glInitialized = true;
}

void RainSystem::Update(float dt, const glm::vec3& cameraPos) {
    if (!m_enabled || m_intensity <= 0.001f) {
        return;
    }

    // Rain velocity: vertical fall with slight wind tilt
    glm::vec3 rainVelocity(-2.2f, -22.0f, -1.2f);
    const float volumeRadius = 35.0f;
    const float volumeHeight = 25.0f;

    size_t activeCount = static_cast<size_t>(m_maxParticles * m_intensity);
    if (activeCount > m_maxParticles) activeCount = m_maxParticles;

    for (size_t i = 0; i < activeCount; ++i) {
        auto& p = m_particles[i];
        p.position += rainVelocity * (p.speed / 20.0f) * dt;

        // Wrap around camera bounding box
        if (p.position.y < cameraPos.y - 1.0f) {
            p.position.y += volumeHeight;
            p.position.x = cameraPos.x + ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * volumeRadius;
            p.position.z = cameraPos.z + ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * volumeRadius;
        }

        // Horizontal camera wrap
        if (std::abs(p.position.x - cameraPos.x) > volumeRadius) {
            p.position.x = cameraPos.x + (p.position.x > cameraPos.x ? -volumeRadius : volumeRadius);
        }
        if (std::abs(p.position.z - cameraPos.z) > volumeRadius) {
            p.position.z = cameraPos.z + (p.position.z > cameraPos.z ? -volumeRadius : volumeRadius);
        }

        // Build line segment (tail to head)
        glm::vec3 tail = p.position - glm::normalize(rainVelocity) * p.length;
        glm::vec4 headColor(0.85f, 0.90f, 1.0f, 0.55f * m_intensity);
        glm::vec4 tailColor(0.70f, 0.80f, 0.95f, 0.05f * m_intensity);

        m_vertexData[i * 2 + 0] = { tail, tailColor };
        m_vertexData[i * 2 + 1] = { p.position, headColor };
    }
}

void RainSystem::Render(const glm::mat4& viewProjection) {
    if (!m_enabled || m_intensity <= 0.001f || !m_glInitialized || !m_shader) {
        return;
    }

    size_t activeCount = static_cast<size_t>(m_maxParticles * m_intensity);
    if (activeCount == 0) return;

    // Upload active particle vertices to dynamic VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(RainVertex) * activeCount * 2, m_vertexData.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Alpha blending with depth test but without depth writes
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    m_shader->Bind();
    m_shader->SetMat4("u_ViewProjection", viewProjection);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(activeCount * 2));
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

} // namespace SmartCity::fx


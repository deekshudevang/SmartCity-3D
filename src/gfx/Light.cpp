#include "gfx/Light.h"
#include <cmath>
#include <algorithm>
#include <string>
#include <glm/gtc/matrix_transform.hpp>

namespace SmartCity::gfx {

LightingEnvironment::LightingEnvironment()
    : m_useBlinnPhong(true)
{
}

void LightingEnvironment::AddPointLight(const PointLight& light) {
    if (m_pointLights.size() < 16) {
        m_pointLights.push_back(light);
    }
}

void LightingEnvironment::AddSpotLight(const SpotLight& light) {
    if (m_spotLights.size() < 8) {
        m_spotLights.push_back(light);
    }
}

void LightingEnvironment::ApplyToShader(Shader* shader, const glm::vec3& cameraPos) const {
    if (!shader) return;

    shader->SetVec3("u_CameraPos", cameraPos);
    shader->SetInt("u_UseBlinnPhong", m_useBlinnPhong ? 1 : 0);

    // 1. Directional Light
    shader->SetVec3("u_DirLight.direction", m_dirLight.direction);
    shader->SetVec3("u_DirLight.color", m_dirLight.color);
    shader->SetFloat("u_DirLight.intensity", m_dirLight.intensity);
    shader->SetVec3("u_DirLight.ambient", m_dirLight.ambient);

    // 2. Point Lights (up to 16)
    int activePointCount = 0;
    for (size_t i = 0; i < m_pointLights.size() && i < 16; ++i) {
        if (!m_pointLights[i].active) continue;
        std::string prefix = "u_PointLights[" + std::to_string(activePointCount) + "].";
        shader->SetVec3(prefix + "position", m_pointLights[i].position);
        shader->SetVec3(prefix + "color", m_pointLights[i].color);
        shader->SetFloat(prefix + "intensity", m_pointLights[i].intensity);
        shader->SetFloat(prefix + "radius", m_pointLights[i].radius);
        shader->SetFloat(prefix + "constant", m_pointLights[i].constant);
        shader->SetFloat(prefix + "linear", m_pointLights[i].linear);
        shader->SetFloat(prefix + "quadratic", m_pointLights[i].quadratic);
        activePointCount++;
    }
    shader->SetInt("u_NumPointLights", activePointCount);

    // 3. Spot Lights (up to 8)
    int activeSpotCount = 0;
    for (size_t i = 0; i < m_spotLights.size() && i < 8; ++i) {
        if (!m_spotLights[i].active) continue;
        std::string prefix = "u_SpotLights[" + std::to_string(activeSpotCount) + "].";
        shader->SetVec3(prefix + "position", m_spotLights[i].position);
        shader->SetVec3(prefix + "direction", glm::normalize(m_spotLights[i].direction));
        shader->SetVec3(prefix + "color", m_spotLights[i].color);
        shader->SetFloat(prefix + "intensity", m_spotLights[i].intensity);
        shader->SetFloat(prefix + "innerCutoff", m_spotLights[i].innerCutoff);
        shader->SetFloat(prefix + "outerCutoff", m_spotLights[i].outerCutoff);
        shader->SetFloat(prefix + "constant", m_spotLights[i].constant);
        shader->SetFloat(prefix + "linear", m_spotLights[i].linear);
        shader->SetFloat(prefix + "quadratic", m_spotLights[i].quadratic);
        activeSpotCount++;
    }
    shader->SetInt("u_NumSpotLights", activeSpotCount);
}

// ----------------- DayNightCycle -----------------
DayNightCycle::DayNightCycle(float initialTimeHours)
    : m_timeOfDay(initialTimeHours)
    , m_cycleSpeed(0.05f)
    , m_isPaused(true)
    , m_streetlightsOn(false)
{
    CalculateLighting();
}

void DayNightCycle::SetTimeOfDay(float hours) {
    m_timeOfDay = std::fmod(hours, 24.0f);
    if (m_timeOfDay < 0.0f) m_timeOfDay += 24.0f;
    CalculateLighting();
}

void DayNightCycle::Update(float dt) {
    if (!m_isPaused) {
        m_timeOfDay += m_cycleSpeed * dt;
        if (m_timeOfDay >= 24.0f) m_timeOfDay -= 24.0f;
        CalculateLighting();
    }
}

void DayNightCycle::CalculateLighting() {
    float sunAngle = (m_timeOfDay - 6.0f) / 24.0f * 2.0f * 3.14159265359f;
    float sunHeight = std::sin(sunAngle);

    glm::vec3 sunPos;
    sunPos.x = std::cos(sunAngle) * 0.7f;
    sunPos.y = sunHeight;
    sunPos.z = 0.35f;
    m_sunLight.direction = -glm::normalize(sunPos);

    if (sunHeight > 0.15f) {
        float t = std::clamp((sunHeight - 0.15f) / 0.85f, 0.0f, 1.0f);
        m_sunLight.color = glm::mix(glm::vec3(1.0f, 0.8f, 0.6f), glm::vec3(1.0f, 0.98f, 0.92f), t);
        m_sunLight.intensity = glm::mix(0.85f, 1.15f, t);
        m_sunLight.ambient = glm::mix(glm::vec3(0.25f, 0.28f, 0.32f), glm::vec3(0.35f, 0.38f, 0.42f), t);
        m_skyColor = glm::mix(glm::vec3(0.5f, 0.6f, 0.75f), glm::vec3(0.42f, 0.64f, 0.88f), t);
        m_streetlightsOn = false;
    } else if (sunHeight > -0.1f) {
        float t = (sunHeight + 0.1f) / 0.25f;
        m_sunLight.color = glm::mix(glm::vec3(0.7f, 0.35f, 0.2f), glm::vec3(1.0f, 0.7f, 0.4f), t);
        m_sunLight.intensity = glm::mix(0.2f, 0.85f, t);
        m_sunLight.ambient = glm::mix(glm::vec3(0.12f, 0.12f, 0.18f), glm::vec3(0.25f, 0.28f, 0.32f), t);
        m_skyColor = glm::mix(glm::vec3(0.18f, 0.15f, 0.28f), glm::vec3(0.5f, 0.45f, 0.65f), t);
        m_streetlightsOn = (sunHeight < 0.05f);
    } else {
        // Night (Moon illumination)
        m_sunLight.direction = glm::normalize(glm::vec3(0.2f, -1.0f, 0.4f));
        m_sunLight.color = glm::vec3(0.35f, 0.45f, 0.65f);
        m_sunLight.intensity = 0.18f;
        m_sunLight.ambient = glm::vec3(0.06f, 0.07f, 0.10f);
        m_skyColor = glm::vec3(0.03f, 0.04f, 0.07f);
        m_streetlightsOn = true;
    }
}

} // namespace SmartCity::gfx

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

static inline float SmoothStep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

void DayNightCycle::CalculateLighting() {
    // 1. Solar Arc: Elevation and Azimuth
    // Solar time angle: 0h = midnight, 6h = dawn, 12h = noon, 18h = dusk
    float sunAngle = (m_timeOfDay - 6.0f) / 24.0f * 2.0f * 3.14159265359f;
    m_sunElevation = std::sin(sunAngle); // in [-1.0, 1.0]
    m_sunAzimuth = (m_timeOfDay - 6.0f) / 12.0f * 3.14159265359f;

    // Physical sun position on celestial sphere
    float cosElev = std::sqrt(std::max(0.0f, 1.0f - m_sunElevation * m_sunElevation));
    glm::vec3 sunPos(
        cosElev * std::sin(m_sunAzimuth) * 0.85f,
        m_sunElevation,
        cosElev * std::cos(m_sunAzimuth) * 0.45f
    );
    m_sunDirection = -glm::normalize(sunPos);

    // 2. Smooth piecewise Hermite curves through Dawn, Day, Dusk, Night
    if (m_sunElevation > 0.15f) {
        // High Day
        float dayFactor = SmoothStep(0.15f, 0.85f, m_sunElevation);

        m_sunLight.direction = m_sunDirection;
        m_sunLight.color = glm::mix(glm::vec3(1.0f, 0.88f, 0.72f), glm::vec3(1.0f, 0.98f, 0.92f), dayFactor);
        m_sunLight.intensity = glm::mix(0.85f, 1.20f, dayFactor);
        m_sunLight.ambient = glm::mix(glm::vec3(0.20f, 0.22f, 0.28f), glm::vec3(0.32f, 0.35f, 0.40f), dayFactor);

        m_skyTopColor = glm::mix(glm::vec3(0.25f, 0.45f, 0.75f), glm::vec3(0.18f, 0.42f, 0.88f), dayFactor);
        m_horizonColor = glm::mix(glm::vec3(0.75f, 0.65f, 0.55f), glm::vec3(0.68f, 0.78f, 0.90f), dayFactor);
        m_groundColor = glm::vec3(0.18f, 0.22f, 0.19f);
        m_skyColor = m_horizonColor;

        m_shadowFade = 1.0f;
        m_streetlightFade = 0.0f;
        m_windowEmissiveFade = 0.0f;
        m_streetlightsOn = false;

    } else if (m_sunElevation > -0.15f) {
        // Dawn / Dusk Transition Interval: smooth blend between Day, Twilight, and Night
        float u = SmoothStep(-0.15f, 0.15f, m_sunElevation); // 0 at deep night, 1 at day

        glm::vec3 daySunColor(1.0f, 0.88f, 0.72f);
        glm::vec3 duskSunColor(1.0f, 0.52f, 0.22f);
        glm::vec3 nightMoonColor(0.42f, 0.55f, 0.82f);

        glm::vec3 moonDir = glm::normalize(glm::vec3(0.25f, -0.92f, 0.30f));
        glm::vec3 dayTop(0.25f, 0.45f, 0.75f);
        glm::vec3 duskTop(0.10f, 0.12f, 0.28f);
        glm::vec3 nightTop(0.015f, 0.020f, 0.045f);

        glm::vec3 dayHorizon(0.75f, 0.65f, 0.55f);
        glm::vec3 duskHorizon(0.85f, 0.42f, 0.20f);
        glm::vec3 nightHorizon(0.035f, 0.045f, 0.085f);

        if (u >= 0.5f) {
            float t = (u - 0.5f) * 2.0f; // 0 at sunset/dusk, 1 at full day
            m_sunLight.direction = m_sunDirection;
            m_sunLight.color = glm::mix(duskSunColor, daySunColor, t);
            m_sunLight.intensity = glm::mix(0.35f, 0.85f, t);
            m_sunLight.ambient = glm::mix(glm::vec3(0.08f, 0.09f, 0.14f), glm::vec3(0.20f, 0.22f, 0.28f), t);

            m_skyTopColor = glm::mix(duskTop, dayTop, t);
            m_horizonColor = glm::mix(duskHorizon, dayHorizon, t);
            m_groundColor = glm::mix(glm::vec3(0.08f, 0.10f, 0.10f), glm::vec3(0.18f, 0.22f, 0.19f), t);
        } else {
            float t = u * 2.0f; // 0 at deep night, 1 at sunset/dusk
            m_sunLight.direction = glm::normalize(glm::mix(moonDir, m_sunDirection, t));
            m_sunLight.color = glm::mix(nightMoonColor, duskSunColor, t);
            m_sunLight.intensity = glm::mix(0.12f, 0.35f, t);
            m_sunLight.ambient = glm::mix(glm::vec3(0.04f, 0.05f, 0.08f), glm::vec3(0.08f, 0.09f, 0.14f), t);

            m_skyTopColor = glm::mix(nightTop, duskTop, t);
            m_horizonColor = glm::mix(nightHorizon, duskHorizon, t);
            m_groundColor = glm::mix(glm::vec3(0.010f, 0.012f, 0.018f), glm::vec3(0.08f, 0.10f, 0.10f), t);
        }
        m_skyColor = m_horizonColor;

        // Shadow fades out smoothly as sun reaches horizon
        m_shadowFade = SmoothStep(0.0f, 0.15f, m_sunElevation);

        // Streetlights and window lights fade in smoothly
        m_streetlightFade = 1.0f - u;
        m_windowEmissiveFade = 1.0f - u;
        m_streetlightsOn = (m_streetlightFade > 0.1f);

    } else {
        // Deep Night: Sun is below horizon, soft Moon illumination
        m_sunLight.direction = glm::normalize(glm::vec3(0.25f, -0.92f, 0.30f));
        m_sunLight.color = glm::vec3(0.42f, 0.55f, 0.82f); // Cool moonlight
        m_sunLight.intensity = 0.12f;
        m_sunLight.ambient = glm::vec3(0.04f, 0.05f, 0.08f);

        m_skyTopColor = glm::vec3(0.015f, 0.020f, 0.045f);
        m_horizonColor = glm::vec3(0.035f, 0.045f, 0.085f);
        m_groundColor = glm::vec3(0.010f, 0.012f, 0.018f);
        m_skyColor = m_horizonColor;

        m_shadowFade = 0.0f; // No sun shadow when sun is below horizon
        m_streetlightFade = 1.0f;
        m_windowEmissiveFade = 1.0f;
        m_streetlightsOn = true;
    }
}

} // namespace SmartCity::gfx

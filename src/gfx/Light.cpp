#include "gfx/Light.h"
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace SmartCity::gfx {

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
        m_sunLight.ambient = glm::mix(glm::vec3(0.1f, 0.1f, 0.16f), glm::vec3(0.25f, 0.22f, 0.28f), t);
        m_skyColor = glm::mix(glm::vec3(0.12f, 0.12f, 0.22f), glm::vec3(0.78f, 0.42f, 0.3f), t);
        m_streetlightsOn = true;
    } else {
        m_sunLight.color = glm::vec3(0.25f, 0.32f, 0.5f);
        m_sunLight.intensity = 0.25f;
        m_sunLight.ambient = glm::vec3(0.06f, 0.08f, 0.12f);
        m_skyColor = glm::vec3(0.05f, 0.06f, 0.10f);
        m_streetlightsOn = true;
    }
}

} // namespace SmartCity::gfx

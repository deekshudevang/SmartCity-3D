#include "graphics/Light.h"
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace SmartCity {

DayNightCycle::DayNightCycle(float initialTimeHours)
    : m_timeOfDay(initialTimeHours)
    , m_cycleSpeed(0.05f) // 1 hour every 20 seconds by default
    , m_isPaused(true)    // Start paused at 2:00 PM for predictable lighting
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
    // 0.0 = 00:00 (Midnight)
    // 6.0 = 06:00 (Sunrise/Dawn)
    // 12.0 = 12:00 (Noon)
    // 18.0 = 18:00 (Sunset/Dusk)

    // Calculate solar angle in radians (0 at 06:00, PI/2 at 12:00, PI at 18:00)
    float sunAngle = (m_timeOfDay - 6.0f) / 24.0f * 2.0f * 3.14159265359f;
    float sunHeight = std::sin(sunAngle); // > 0 is day, < 0 is night

    // Direction vector points FROM the sun towards the scene (for shader)
    glm::vec3 sunPos;
    sunPos.x = std::cos(sunAngle) * 0.7f;
    sunPos.y = sunHeight;
    sunPos.z = 0.35f;
    m_sunLight.direction = -glm::normalize(sunPos);

    if (sunHeight > 0.15f) {
        // Broad daylight (Noon - afternoon)
        float t = std::clamp((sunHeight - 0.15f) / 0.85f, 0.0f, 1.0f);
        m_sunLight.color = glm::mix(glm::vec3(1.0f, 0.8f, 0.6f), glm::vec3(1.0f, 0.98f, 0.92f), t);
        m_sunLight.intensity = glm::mix(0.85f, 1.15f, t);
        m_sunLight.ambient = glm::mix(glm::vec3(0.25f, 0.28f, 0.32f), glm::vec3(0.35f, 0.38f, 0.42f), t);
        m_skyColor = glm::mix(glm::vec3(0.5f, 0.6f, 0.75f), glm::vec3(0.42f, 0.64f, 0.88f), t);
        m_streetlightsOn = false;
    } else if (sunHeight > -0.1f) {
        // Dawn or Dusk transition (Golden / Twilight hour)
        float t = (sunHeight + 0.1f) / 0.25f; // 0.0 at night edge, 1.0 at day edge
        m_sunLight.color = glm::mix(glm::vec3(0.7f, 0.35f, 0.2f), glm::vec3(1.0f, 0.7f, 0.4f), t);
        m_sunLight.intensity = glm::mix(0.2f, 0.85f, t);
        m_sunLight.ambient = glm::mix(glm::vec3(0.1f, 0.1f, 0.16f), glm::vec3(0.25f, 0.22f, 0.28f), t);
        m_skyColor = glm::mix(glm::vec3(0.12f, 0.12f, 0.22f), glm::vec3(0.78f, 0.42f, 0.3f), t);
        m_streetlightsOn = true;
    } else {
        // Night (Moonlight)
        m_sunLight.color = glm::vec3(0.25f, 0.32f, 0.5f); // Soft blue moonlight
        m_sunLight.intensity = 0.25f;
        m_sunLight.ambient = glm::vec3(0.06f, 0.08f, 0.12f);
        m_skyColor = glm::vec3(0.05f, 0.06f, 0.10f); // Dark night sky
        m_streetlightsOn = true;
    }
}

} // namespace SmartCity

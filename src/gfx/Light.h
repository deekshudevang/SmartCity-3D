#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace SmartCity::gfx {

struct DirectionalLight {
    glm::vec3 direction{ -0.3f, -1.0f, -0.4f };
    glm::vec3 color{ 1.0f, 0.98f, 0.90f };
    float intensity{ 1.0f };
    glm::vec3 ambient{ 0.3f, 0.32f, 0.35f };
};

struct PointLight {
    glm::vec3 position{ 0.0f };
    glm::vec3 color{ 1.0f, 0.85f, 0.5f };
    float intensity{ 1.0f };
    float radius{ 25.0f };
    bool active{ true };
};

class DayNightCycle {
public:
    DayNightCycle(float initialTimeHours = 14.0f);

    void Update(float dt);

    void SetTimeOfDay(float hours);
    float GetTimeOfDay() const { return m_timeOfDay; }

    void SetCycleSpeed(float speed) { m_cycleSpeed = speed; }
    float GetCycleSpeed() const { return m_cycleSpeed; }
    void SetPaused(bool paused) { m_isPaused = paused; }
    bool IsPaused() const { return m_isPaused; }

    const DirectionalLight& GetLight() const { return m_sunLight; }
    glm::vec3 GetClearColor() const { return m_skyColor; }
    bool AreStreetlightsActive() const { return m_streetlightsOn; }

private:
    void CalculateLighting();

    float m_timeOfDay;
    float m_cycleSpeed;
    bool m_isPaused;

    DirectionalLight m_sunLight;
    glm::vec3 m_skyColor;
    bool m_streetlightsOn;
};

} // namespace SmartCity::gfx

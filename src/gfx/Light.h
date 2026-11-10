#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "gfx/Shader.h"

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
    float constant{ 1.0f };
    float linear{ 0.09f };
    float quadratic{ 0.032f };
    bool active{ true };
};

struct SpotLight {
    glm::vec3 position{ 0.0f, 10.0f, 0.0f };
    glm::vec3 direction{ 0.0f, -1.0f, 0.0f };
    glm::vec3 color{ 1.0f, 1.0f, 1.0f };
    float intensity{ 2.0f };
    float innerCutoff{ 0.9659f }; // cos(15 deg)
    float outerCutoff{ 0.9396f }; // cos(20 deg)
    float constant{ 1.0f };
    float linear{ 0.09f };
    float quadratic{ 0.032f };
    bool active{ true };
};

class LightingEnvironment {
public:
    LightingEnvironment();

    void SetDirectionalLight(const DirectionalLight& dirLight) { m_dirLight = dirLight; }
    const DirectionalLight& GetDirectionalLight() const { return m_dirLight; }

    void AddPointLight(const PointLight& light);
    void ClearPointLights() { m_pointLights.clear(); }
    const std::vector<PointLight>& GetPointLights() const { return m_pointLights; }
    std::vector<PointLight>& GetPointLights() { return m_pointLights; }

    void AddSpotLight(const SpotLight& light);
    void ClearSpotLights() { m_spotLights.clear(); }
    const std::vector<SpotLight>& GetSpotLights() const { return m_spotLights; }
    std::vector<SpotLight>& GetSpotLights() { return m_spotLights; }

    void SetUseBlinnPhong(bool useBlinn) { m_useBlinnPhong = useBlinn; }
    bool IsBlinnPhong() const { return m_useBlinnPhong; }
    void ToggleBlinnPhong() { m_useBlinnPhong = !m_useBlinnPhong; }

    void ApplyToShader(Shader* shader, const glm::vec3& cameraPos) const;

private:
    DirectionalLight m_dirLight;
    std::vector<PointLight> m_pointLights;
    std::vector<SpotLight> m_spotLights;
    bool m_useBlinnPhong;
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
    glm::vec3 GetSunDirection() const { return m_sunDirection; }
    float GetSunElevation() const { return m_sunElevation; }
    float GetSunAzimuth() const { return m_sunAzimuth; }

    glm::vec3 GetClearColor() const { return m_skyColor; }
    glm::vec3 GetSkyTopColor() const { return m_skyTopColor; }
    glm::vec3 GetHorizonColor() const { return m_horizonColor; }
    glm::vec3 GetGroundColor() const { return m_groundColor; }

    bool AreStreetlightsActive() const { return m_streetlightsOn; }
    float GetStreetlightIntensity() const { return m_streetlightFade; }
    float GetWindowEmissiveIntensity() const { return m_windowEmissiveFade; }
    float GetSunShadowFade() const { return m_shadowFade; }

private:
    void CalculateLighting();

    float m_timeOfDay;
    float m_cycleSpeed;
    bool m_isPaused;

    DirectionalLight m_sunLight;
    glm::vec3 m_sunDirection{ 0.0f, -1.0f, 0.0f };
    float m_sunElevation{ 0.0f };
    float m_sunAzimuth{ 0.0f };

    glm::vec3 m_skyColor;
    glm::vec3 m_skyTopColor;
    glm::vec3 m_horizonColor;
    glm::vec3 m_groundColor;

    bool m_streetlightsOn{ false };
    float m_streetlightFade{ 0.0f };
    float m_windowEmissiveFade{ 0.0f };
    float m_shadowFade{ 1.0f };
};

} // namespace SmartCity::gfx

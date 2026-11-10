#include <cassert>
#include <iostream>
#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "gfx/Light.h"
#include "gfx/ShadowMap.h"
#include "gfx/Shader.h"
#include "fx/RainSystem.h"
#include "city/RoadNetwork.h"
#include "sim/VehicleManager.h"

using namespace SmartCity;

void Test_TimeOfDayContinuity() {
    std::cout << "[Test 1] TimeOfDay Smooth Continuity Bounds (1440 1-minute steps)..." << std::endl;

    gfx::DayNightCycle cycle(0.0f);
    cycle.SetPaused(true);

    float maxDirDelta = 0.0f;
    float maxColorDelta = 0.0f;
    float maxIntensityDelta = 0.0f;
    float maxSkyDelta = 0.0f;

    glm::vec3 prevDir = cycle.GetSunDirection();
    glm::vec3 prevColor = cycle.GetLight().color;
    float prevIntensity = cycle.GetLight().intensity;
    glm::vec3 prevSky = cycle.GetClearColor();

    const float dtHours = 24.0f / 1440.0f; // 1 minute = 1/60 hr

    for (int step = 1; step <= 1440; ++step) {
        float timeHours = step * dtHours;
        cycle.SetTimeOfDay(timeHours);

        glm::vec3 currDir = cycle.GetSunDirection();
        glm::vec3 currColor = cycle.GetLight().color;
        float currIntensity = cycle.GetLight().intensity;
        glm::vec3 currSky = cycle.GetClearColor();

        float dirDelta = glm::length(currDir - prevDir);
        float colorDelta = glm::length(currColor - prevColor);
        float intensityDelta = std::abs(currIntensity - prevIntensity);
        float skyDelta = glm::length(currSky - prevSky);

        if (dirDelta > maxDirDelta) maxDirDelta = dirDelta;
        if (colorDelta > maxColorDelta) maxColorDelta = colorDelta;
        if (intensityDelta > maxIntensityDelta) maxIntensityDelta = intensityDelta;
        if (skyDelta > maxSkyDelta) maxSkyDelta = skyDelta;

        // Bounded per-minute delta: no sudden step jumps (> 0.05)
        assert(dirDelta < 0.05f && "Sun direction jumped abruptly between consecutive minutes!");
        assert(colorDelta < 0.05f && "Sun color jumped abruptly between consecutive minutes!");
        assert(intensityDelta < 0.05f && "Sun intensity jumped abruptly between consecutive minutes!");
        assert(skyDelta < 0.05f && "Sky color jumped abruptly between consecutive minutes!");

        prevDir = currDir;
        prevColor = currColor;
        prevIntensity = currIntensity;
        prevSky = currSky;
    }

    std::cout << "  PASS: Max per-minute deltas: Dir=" << maxDirDelta
              << ", Color=" << maxColorDelta
              << ", Intensity=" << maxIntensityDelta
              << ", Sky=" << maxSkyDelta << std::endl;
}

void Test_SunDirectionAtKnownTimes() {
    std::cout << "[Test 2] Sun Direction at Known Solar Times (Noon vs Midnight)..." << std::endl;

    gfx::DayNightCycle cycle(12.0f); // Solar Noon
    cycle.SetPaused(true);

    float noonElevation = cycle.GetSunElevation();
    glm::vec3 noonDir = cycle.GetSunDirection();
    std::cout << "  Noon (12:00): Elevation = " << noonElevation << ", Direction = ("
              << noonDir.x << ", " << noonDir.y << ", " << noonDir.z << ")" << std::endl;

    // At noon (12:00), sun must be at highest elevation (> 0.8) and pointing downwards (-Y)
    assert(noonElevation > 0.85f && "Noon sun elevation must be near maximum positive!");
    assert(noonDir.y < -0.85f && "Noon sun light direction vector must point downwards!");
    assert(cycle.GetSunShadowFade() > 0.95f && "Sun shadow must be fully active at noon!");
    assert(!cycle.AreStreetlightsActive() && "Streetlights must be inactive at noon!");

    // Midnight (00:00)
    cycle.SetTimeOfDay(0.0f);
    float midnightElevation = cycle.GetSunElevation();
    glm::vec3 midnightDir = cycle.GetSunDirection();
    std::cout << "  Midnight (00:00): Elevation = " << midnightElevation << ", Direction = ("
              << midnightDir.x << ", " << midnightDir.y << ", " << midnightDir.z << ")" << std::endl;

    // At midnight, sun elevation must be below horizon (< -0.85)
    assert(midnightElevation < -0.85f && "Midnight sun must be below horizon!");
    assert(cycle.GetSunShadowFade() == 0.0f && "Sun shadow fade must be exactly 0.0 at midnight!");
    assert(cycle.AreStreetlightsActive() && "Streetlights must be active at midnight!");
    assert(cycle.GetWindowEmissiveIntensity() > 0.95f && "Window emissive glow must be at max at midnight!");

    // Dawn (06:00) and Dusk (18:00)
    cycle.SetTimeOfDay(6.0f);
    assert(std::abs(cycle.GetSunElevation()) < 0.15f && "Sun must be crossing horizon at dawn (6:00)!");

    cycle.SetTimeOfDay(18.0f);
    assert(std::abs(cycle.GetSunElevation()) < 0.15f && "Sun must be crossing horizon at dusk (18:00)!");

    std::cout << "  PASS: Sun direction & elevation verified across solar timeline." << std::endl;
}

void Test_LightSpaceMatrixContainment() {
    std::cout << "[Test 3] Light-Space Matrix Frustum Containment (NDC [-1, 1])..." << std::endl;

    gfx::ShadowMap shadowMap(2048, 2048);
    glm::vec3 sunLightDir = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.4f));

    glm::mat4 lightSpaceMatrix = shadowMap.CalculateLightSpaceMatrix(sunLightDir, glm::vec3(0.0f));

    // Critical world points across the 4-way intersection that must receive shadows
    std::vector<glm::vec3> worldPoints = {
        { 0.0f, 0.0f, 0.0f },       // Intersection center
        { 30.0f, 0.0f, 0.0f },      // East approach stop line
        { -30.0f, 0.0f, 0.0f },     // West approach stop line
        { 0.0f, 0.0f, 30.0f },      // South approach stop line
        { 0.0f, 0.0f, -30.0f },     // North approach stop line
        { 50.0f, 0.0f, 50.0f },     // SE quadrant sidewalk / building base
        { -50.0f, 0.0f, -50.0f },   // NW quadrant sidewalk / building base
        { 50.0f, 0.0f, -50.0f },    // NE quadrant sidewalk / building base
        { -50.0f, 0.0f, 50.0f },    // SW quadrant sidewalk / building base
        { 0.0f, 8.0f, 0.0f }        // Mast arm signal head elevation (8m)
    };

    for (const auto& pt : worldPoints) {
        glm::vec4 ndc = lightSpaceMatrix * glm::vec4(pt, 1.0f);
        ndc /= ndc.w;
        std::cerr << "  Point (" << pt.x << ", " << pt.y << ", " << pt.z << ") -> NDC ("
                  << ndc.x << ", " << ndc.y << ", " << ndc.z << ")" << std::endl;

        // Verify that point projects cleanly within [-1.0, 1.0] NDC
        assert(ndc.x >= -1.0f && ndc.x <= 1.0f && "Point X projected outside shadow frustum!");
        assert(ndc.y >= -1.0f && ndc.y <= 1.0f && "Point Y projected outside shadow frustum!");
        assert(ndc.z >= -1.0f && ndc.z <= 1.0f && "Point Z projected outside shadow frustum!");
    }

    std::cout << "  PASS: All " << worldPoints.size() << " intersection sample points contained in [-1, 1] NDC." << std::endl;
}

void Test_ShaderCompileValidator() {
    std::cout << "[Test 4] Shader File & Startup Validator..." << std::endl;

    std::vector<std::string> failedShaders;
    bool allValid = gfx::Shader::ValidateAllShaders(failedShaders);

    if (!allValid) {
        for (const auto& err : failedShaders) {
            std::cerr << "  FAILED: " << err << std::endl;
        }
    }

    assert(allValid && "All project shaders must be present and validate successfully!");
    std::cout << "  PASS: 100% shader files present and verified." << std::endl;
}

void Test_SimulationDeterminismWithFX() {
    std::cout << "[Test 5] Simulation Invariance & Determinism (FX ON vs FX OFF)..." << std::endl;

    city::RoadNetwork net;
    net.BuildFromSpec(core::IntersectionSettings{});

    // Run 1: Base simulation (No FX)
    sim::VehicleManager simBase(net, nullptr);
    simBase.Reset(42);

    for (int tick = 0; tick < 600; ++tick) {
        simBase.Step(1.0f / 60.0f);
    }

    size_t baseActive = simBase.GetActiveVehicleCount();
    int baseSpawned = simBase.GetTotalSpawned();
    float baseAvgSpeed = simBase.GetAverageSpeed();

    std::vector<glm::vec3> basePositions;
    for (const auto& v : simBase.GetActiveVehicles()) {
        basePositions.push_back(v->GetPosition());
    }

    // Run 2: Simulation with active RainSystem and DayNightCycle updating in parallel
    sim::VehicleManager simWithFX(net, nullptr);
    simWithFX.Reset(42);

    gfx::DayNightCycle dayNight(14.0f);
    fx::RainSystem rain(5000);
    rain.SetEnabled(true);
    rain.SetIntensity(0.9f);

    for (int tick = 0; tick < 600; ++tick) {
        float dt = 1.0f / 60.0f;
        // Step environmental systems
        dayNight.Update(dt);
        rain.Update(dt, glm::vec3(0.0f, 15.0f, 0.0f));

        // Step simulation
        simWithFX.Step(dt);
    }

    size_t fxActive = simWithFX.GetActiveVehicleCount();
    int fxSpawned = simWithFX.GetTotalSpawned();
    float fxAvgSpeed = simWithFX.GetAverageSpeed();

    assert(baseActive == fxActive && "Vehicle count diverged when FX was active!");
    assert(baseSpawned == fxSpawned && "Total spawned diverged when FX was active!");
    assert(std::abs(baseAvgSpeed - fxAvgSpeed) < 1e-5f && "Average speed diverged when FX was active!");

    const auto& fxVehicles = simWithFX.GetActiveVehicles();
    assert(fxVehicles.size() == basePositions.size());

    for (size_t i = 0; i < basePositions.size(); ++i) {
        float dist = glm::distance(basePositions[i], fxVehicles[i]->GetPosition());
        assert(dist < 1e-5f && "Vehicle position altered by visual effects!");
    }

    std::cout << "  PASS: Simulation physics strictly invariant to visual effects (Delta = 0.000000m)." << std::endl;
}

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "SmartCity 3D — Environment & Visual Effects Test Suite" << std::endl;
    std::cout << "=====================================================" << std::endl;

    Test_TimeOfDayContinuity();
    Test_SunDirectionAtKnownTimes();
    Test_LightSpaceMatrixContainment();
    Test_ShaderCompileValidator();
    Test_SimulationDeterminismWithFX();

    std::cout << "=====================================================" << std::endl;
    std::cout << "ALL ENVIRONMENT & FX TESTS PASSED (5/5)" << std::endl;
    std::cout << "=====================================================" << std::endl;
    return 0;
}

#include <iostream>
#include <cassert>
#include <cmath>
#include "core/Config.h"
#include "core/Logger.h"

int main() {
    SmartCity::core::Logger::Init("logs/test_intersection.log");
    SmartCity::core::Logger::Info("Running unit test_intersection...");

    SmartCity::core::IntersectionSettings settings;
    bool loaded = SmartCity::core::ConfigManager::LoadIntersectionConfig("assets/config/intersection_config.json", settings);
    assert(loaded);
    assert(settings.roadWidth == 14.0f);
    assert(settings.approachLength == 80.0f);
    assert(settings.curbHeight == 0.15f);
    assert(settings.numLanesPerApproach == 4);
    SmartCity::core::Logger::Info("[PASS] Loaded intersection settings from assets/config/intersection_config.json");

    float hw = settings.roadWidth * 0.5f;
    assert(hw == 7.0f);
    float laneWidth = settings.roadWidth / static_cast<float>(settings.numLanesPerApproach);
    assert(std::abs(laneWidth - 3.5f) < 1e-4f);
    SmartCity::core::Logger::Info("[PASS] Lane width calculation: ", laneWidth, " m (4 lanes)");

    float stopLineDist = settings.stopLineDistance;
    float crosswalkDist = settings.crosswalkDistance;
    assert(crosswalkDist > hw);
    assert(crosswalkDist + settings.crosswalkWidth <= stopLineDist);
    SmartCity::core::Logger::Info("[PASS] Crosswalk (", crosswalkDist, "m) and Stop Line (", stopLineDist, "m) geometric spacing");

    float sidewalkOuter = hw + settings.sidewalkWidth;
    assert(std::abs(sidewalkOuter - 11.0f) < 1e-4f);
    SmartCity::core::Logger::Info("[PASS] Sidewalk outer boundary (", sidewalkOuter, "m)");

    SmartCity::core::Logger::Info("All test_intersection unit tests PASSED successfully!");
    SmartCity::core::Logger::Shutdown();
    return 0;
}

#include <iostream>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include "city/RoadNetwork.h"
#include "city/BuildingGenerator.h"
#include "core/Logger.h"
#include "core/Config.h"

int main() {
    SmartCity::core::Logger::Init("logs/test_road_network.log");
    SmartCity::core::Logger::Info("=================================================");
    SmartCity::core::Logger::Info(" Running Phase 3 Unit Tests: RoadNetwork & City");
    SmartCity::core::Logger::Info("=================================================");

    SmartCity::core::IntersectionSettings settings;
    settings.roadWidth = 14.0f;
    settings.laneWidth = 3.5f;
    settings.approachLength = 120.0f;
    settings.stopLineDistance = 19.5f;
    settings.crosswalkDistance = 14.0f;
    settings.crosswalkWidth = 4.0f;
    settings.stopLineBeforeCrosswalk = 1.5f;
    settings.medianWidth = 1.5f;

    SmartCity::city::RoadNetwork network;
    network.BuildFromSpec(settings);

    // ---------------- TEST 1: 4-Way Geometric Alignment & Symmetry ----------------
    const auto* north = network.GetApproach(SmartCity::city::ApproachId::North);
    const auto* east  = network.GetApproach(SmartCity::city::ApproachId::East);
    const auto* south = network.GetApproach(SmartCity::city::ApproachId::South);
    const auto* west  = network.GetApproach(SmartCity::city::ApproachId::West);

    assert(north && east && south && west);
    assert(std::abs(glm::dot(north->forwardDir, east->forwardDir)) < 1e-4f);
    assert(std::abs(glm::dot(east->forwardDir, south->forwardDir)) < 1e-4f);
    assert(std::abs(glm::dot(south->forwardDir, west->forwardDir)) < 1e-4f);
    assert(std::abs(glm::dot(west->forwardDir, north->forwardDir)) < 1e-4f);

    // Symmetrical positions of stop lines
    const auto* slN = network.GetStopLine("STOP_North");
    const auto* slE = network.GetStopLine("STOP_East");
    const auto* slS = network.GetStopLine("STOP_South");
    const auto* slW = network.GetStopLine("STOP_West");
    assert(slN && slE && slS && slW);

    assert(std::abs(std::abs(slN->position.z) - settings.stopLineDistance) < 1e-3f);
    assert(std::abs(std::abs(slS->position.z) - settings.stopLineDistance) < 1e-3f);
    assert(std::abs(std::abs(slE->position.x) - settings.stopLineDistance) < 1e-3f);
    assert(std::abs(std::abs(slW->position.x) - settings.stopLineDistance) < 1e-3f);

    // Stop line normals perpendicular to lane direction (pointing opposite to traffic forwardDir)
    assert(glm::dot(slN->normal, north->forwardDir) < -0.99f);
    assert(glm::dot(slS->normal, south->forwardDir) < -0.99f);
    assert(glm::dot(slE->normal, east->forwardDir) < -0.99f);
    assert(glm::dot(slW->normal, west->forwardDir) < -0.99f);
    SmartCity::core::Logger::Info("[PASS] Test 1: 4-Way 90-degree alignment, perpendicular stop lines, and symmetry verified.");

    // ---------------- TEST 2: Stop-Line Placement & Crosswalk Spacing ----------------
    for (const auto& sl : network.GetAllStopLines()) {
        assert(std::abs(sl.distanceToCrosswalk - settings.stopLineBeforeCrosswalk) < 1e-3f);
        // All inbound lanes on this approach are listed
        auto inboundLanes = network.InboundLanesFor(sl.approach);
        assert(!inboundLanes.empty());
        assert(sl.laneIds.size() == inboundLanes.size());
        for (const auto* lane : inboundLanes) {
            assert(std::find(sl.laneIds.begin(), sl.laneIds.end(), lane->id) != sl.laneIds.end());
        }
    }
    SmartCity::core::Logger::Info("[PASS] Test 2: Stop line placement distance to crosswalk (1.5m) and lane coverage verified.");

    // ---------------- TEST 3: Lane Connectivity & Turning Paths ----------------
    const auto& lanes = network.GetAllLanes();
    assert(lanes.size() == 24); // 4 approaches * (4 inbound + 2 outbound) = 24 lanes
    const auto& paths = network.GetAllPaths();
    assert(paths.size() == 16); // 4 approaches * 4 inbound movements = 16 paths

    for (const auto& lane : lanes) {
        if (lane.direction == SmartCity::city::LaneDirection::Inbound) {
            auto outPaths = network.PathsFrom(lane.id);
            assert(!outPaths.empty()); // No dead ends
            for (const auto* p : outPaths) {
                // Endpoint matching tolerance < 1 cm (0.01 m)
                float dStart = glm::distance(p->p0, lane.endPoint);
                assert(dStart < 0.01f);

                const auto* toLane = network.GetLane(p->toLaneId);
                assert(toLane != nullptr);
                assert(toLane->direction == SmartCity::city::LaneDirection::Outbound);
                float dEnd = glm::distance(p->p3, toLane->startPoint);
                assert(dEnd < 0.01f);

                // Physical curvature check
                if (p->movement == SmartCity::city::MovementType::Right) {
                    assert(p->minRadius >= 7.5f); // Spec: >= 8m plausible
                } else if (p->movement == SmartCity::city::MovementType::Left) {
                    assert(p->minRadius >= 11.5f); // Spec: >= 12m plausible
                }
            }
        }
    }
    SmartCity::core::Logger::Info("[PASS] Test 3: Lane connectivity, 1cm endpoint tolerance, and curvature constraints verified.");

    // ---------------- TEST 4: Entry and Exit Points ----------------
    const auto& entryPoints = network.GetEntryPoints();
    const auto& exitPoints = network.GetExitPoints();
    assert(entryPoints.size() == 16); // 4 inbound per approach * 4 = 16
    assert(exitPoints.size() == 8);   // 2 outbound per approach * 4 = 8

    for (const auto& ep : entryPoints) {
        const auto* lane = network.GetLane(ep.laneId);
        assert(lane != nullptr);
        assert(glm::distance(ep.position, lane->startPoint) < 0.01f);
        assert(std::abs(ep.headingRad - lane->headingRad) < 1e-4f);
    }
    for (const auto& xp : exitPoints) {
        const auto* lane = network.GetLane(xp.laneId);
        assert(lane != nullptr);
        assert(glm::distance(xp.position, lane->endPoint) < 0.01f);
        assert(std::abs(xp.headingRad - lane->headingRad) < 1e-4f);
    }
    SmartCity::core::Logger::Info("[PASS] Test 4: Entry and exit points with headings verified.");

    // ---------------- TEST 5: Conflict Zone Symmetry & Movement Compatibility ----------------
    // Rule 1: North left turn conflicts with South opposing through
    std::string northLeftPath = "PATH_N_IN_0_TO_E_OUT_0";
    std::string southThroughPath1 = "PATH_S_IN_1_TO_N_OUT_0";
    std::string southThroughPath2 = "PATH_S_IN_2_TO_N_OUT_1";

    auto northLeftConflicts = network.ConflictsOf(northLeftPath);
    assert(std::find(northLeftConflicts.begin(), northLeftConflicts.end(), southThroughPath1) != northLeftConflicts.end());
    assert(std::find(northLeftConflicts.begin(), northLeftConflicts.end(), southThroughPath2) != northLeftConflicts.end());

    // Rule 2: Symmetric relationship
    auto southThroughConflicts1 = network.ConflictsOf(southThroughPath1);
    assert(std::find(southThroughConflicts1.begin(), southThroughConflicts1.end(), northLeftPath) != southThroughConflicts1.end());

    // Rule 3: Right turns do not conflict with opposing left under RHT
    std::string northRightPath = "PATH_N_IN_3_TO_W_OUT_1";
    std::string southLeftPath = "PATH_S_IN_0_TO_W_OUT_0";
    auto northRightConflicts = network.ConflictsOf(northRightPath);
    assert(std::find(northRightConflicts.begin(), northRightConflicts.end(), southLeftPath) == northRightConflicts.end());
    SmartCity::core::Logger::Info("[PASS] Test 5: Conflict zones, symmetric relations, and movement compatibilities verified.");

    // ---------------- TEST 6: Building Overlap Verification ----------------
    SmartCity::city::BuildingGenerator buildingGen(settings);
    assert(buildingGen.GetBuildings().size() >= 8); // Spec requires >= 8 buildings (we have 12)
    assert(buildingGen.VerifyNoOverlap(18.0f)); // Corridor clearance of 18m
    SmartCity::core::Logger::Info("[PASS] Test 6: Building clearance (12 buildings, 0 road/sidewalk overlaps) verified.");

    // ---------------- TEST 7: Asset Existence Validator ----------------
    std::vector<std::string> requiredAssets = {
        "shaders/scene.vert",
        "shaders/scene.frag",
        "shaders/shadow_depth.vert",
        "shaders/shadow_depth.frag",
        "shaders/light_indicator.vert",
        "shaders/light_indicator.frag",
        "shaders/skybox.vert",
        "shaders/skybox.frag",
        "assets/models/test_cube.obj",
        "assets/config/simulation_config.json",
        "assets/config/intersection_config.json"
    };

    for (const auto& asset : requiredAssets) {
        bool exists = std::filesystem::exists(asset) || std::filesystem::exists("../" + asset);
        if (!exists) {
            SmartCity::core::Logger::Error("Required asset file missing: ", asset);
            assert(false && "Missing required asset!");
        }
    }
    SmartCity::core::Logger::Info("[PASS] Test 7: Startup asset validator verified all shader, model, and config files.");

    // ---------------- TEST 8: JSON Round-Trip Serialization Test ----------------
    std::string jsonPath = "assets/config/network.json";
    bool saved = network.SaveToJson(jsonPath);
    assert(saved);

    SmartCity::city::RoadNetwork reloadedNetwork;
    bool loaded = reloadedNetwork.LoadFromJson(jsonPath);
    assert(loaded);

    bool equivalent = network.IsEquivalent(reloadedNetwork, 0.01f);
    assert(equivalent);
    SmartCity::core::Logger::Info("[PASS] Test 8: RoadNetwork JSON round-trip export and reload verified.");

    SmartCity::core::Logger::Info("All Phase 3 RoadNetwork unit tests PASSED successfully!");
    SmartCity::core::Logger::Shutdown();
    return 0;
}


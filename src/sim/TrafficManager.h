#pragma once

#include <vector>
#include <memory>
#include <random>
#include <glm/glm.hpp>
#include "core/Config.h"
#include "gfx/Shader.h"
#include "gfx/Mesh.h"
#include "sim/LaneGraph.h"
#include "sim/Vehicle.h"
#include "sim/IDMController.h"

namespace SmartCity::sim {

class TrafficManager {
public:
    TrafficManager(const core::SimulationSettings& simSettings, const core::IntersectionSettings& interSettings);
    ~TrafficManager() = default;

    void Update(float dt, int nsSignalState, int ewSignalState, int nsLeftState, int ewLeftState);

    void Render(gfx::Shader* sceneShader, gfx::Shader* emissiveShader);
    void RenderShadowDepth(gfx::Shader* shadowShader);

    // Manual spawner for tests & UI
    Vehicle* SpawnVehicle(VehicleType type, ApproachDirection approach, TurnManeuver maneuver);

    size_t GetActiveVehicleCount() const { return m_vehicles.size(); }
    int GetTotalSpawned() const { return m_totalSpawned; }
    int GetTotalCompleted() const { return m_totalCompleted; }
    float GetAverageSpeed() const;
    bool IsEmergencyActive() const { return m_emergencyActive; }
    int GetEmergencyApproach() const { return m_emergencyApproach; }

    const std::vector<std::unique_ptr<Vehicle>>& GetVehicles() const { return m_vehicles; }
    const LaneGraph& GetLaneGraph() const { return *m_laneGraph; }

    void Clear();

private:
    void ProcessSpawning(float dt);
    void UpdateVehiclePhysics(Vehicle* vehicle, float dt, int nsSignal, int ewSignal, int nsLeft, int ewLeft);
    void EnsureRenderingMeshes();
    void DrawVehicle3D(const Vehicle* vehicle, gfx::Shader* sceneShader, gfx::Shader* emissiveShader);
    void DrawVehicleShadow(const Vehicle* vehicle, gfx::Shader* shadowShader);

    core::SimulationSettings m_simSettings;
    core::IntersectionSettings m_interSettings;

    std::unique_ptr<LaneGraph> m_laneGraph;
    std::vector<std::unique_ptr<Vehicle>> m_vehicles;

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist01;

    float m_northSpawnTimer;
    float m_southSpawnTimer;
    float m_eastSpawnTimer;
    float m_westSpawnTimer;

    int m_nextVehicleId;
    int m_totalSpawned;
    int m_totalCompleted;

    bool m_emergencyActive;
    int m_emergencyApproach; // -1: none, 0: North, 1: South, 2: East, 3: West

    // Procedural rendering meshes
    std::unique_ptr<gfx::Mesh> m_cubeMesh;
    std::unique_ptr<gfx::Mesh> m_wheelMesh;
};

} // namespace SmartCity::sim

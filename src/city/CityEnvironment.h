#pragma once

#include <memory>
#include <vector>
#include "city/IntersectionGeometry.h"
#include "city/RoadMarkings.h"
#include "city/BuildingGenerator.h"
#include "city/StreetFurniture.h"
#include "city/RoadNetwork.h"
#include "gfx/Shader.h"
#include "gfx/Light.h"
#include "core/Config.h"

namespace SmartCity::city {

class CityEnvironment {
public:
    CityEnvironment(const core::IntersectionSettings& settings = core::IntersectionSettings());
    ~CityEnvironment() = default;

    void Render(gfx::Shader* sceneShader, gfx::Shader* emissiveShader,
                int nsSignalState, int ewSignalState,
                int nsLeftSignalState, int ewLeftSignalState,
                bool nsPedWalk, bool ewPedWalk);

    void RenderShadowDepth(gfx::Shader* shadowShader);

    const core::IntersectionSettings& GetSettings() const { return m_settings; }
    const std::vector<gfx::PointLight>& GetStreetlights() const { return m_streetFurniture->GetStreetlightSources(); }
    const RoadNetwork& GetRoadNetwork() const { return *m_roadNetwork; }

    void RenderDebugOverlay(gfx::Shader* debugShader,
                            bool showLaneCenters,
                            bool showTurningBeziers,
                            bool showConflictZones,
                            bool showStopLines,
                            bool showEntryExit);

private:
    void RenderEmissiveSignalHeads(gfx::Shader* emissiveShader,
                                  int nsSignalState, int ewSignalState,
                                  int nsLeftSignalState, int ewLeftSignalState,
                                  bool nsPedWalk, bool ewPedWalk);

    void GenerateDebugOverlayMeshes();

    core::IntersectionSettings m_settings;

    std::unique_ptr<RoadNetwork> m_roadNetwork;
    std::unique_ptr<IntersectionGeometry> m_geometry;
    std::unique_ptr<RoadMarkings> m_markings;
    std::unique_ptr<BuildingGenerator> m_buildings;
    std::unique_ptr<StreetFurniture> m_streetFurniture;

    std::unique_ptr<gfx::Mesh> m_lampBulbMesh;
    std::unique_ptr<gfx::Mesh> m_debugLaneMesh;
    std::unique_ptr<gfx::Mesh> m_debugBezierMesh;
    std::unique_ptr<gfx::Mesh> m_debugConflictMesh;
    std::unique_ptr<gfx::Mesh> m_debugStopLineMesh;
    std::unique_ptr<gfx::Mesh> m_debugEntryExitMesh;
};

} // namespace SmartCity::city

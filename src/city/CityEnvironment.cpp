#include "city/CityEnvironment.h"
#include <glm/gtc/matrix_transform.hpp>

namespace SmartCity::city {

CityEnvironment::CityEnvironment(const core::IntersectionSettings& settings)
    : m_settings(settings)
{
    m_roadNetwork = std::make_unique<RoadNetwork>();
    m_roadNetwork->BuildFromSpec(settings);

    m_geometry = std::make_unique<IntersectionGeometry>(settings);
    m_markings = std::make_unique<RoadMarkings>(settings);
    m_buildings = std::make_unique<BuildingGenerator>(settings);
    m_streetFurniture = std::make_unique<StreetFurniture>(settings);

    m_lampBulbMesh = gfx::Mesh::CreateSphere(0.18f, 10, 14, glm::vec4(1.0f));
    GenerateDebugOverlayMeshes();
}

void CityEnvironment::RenderShadowDepth(gfx::Shader* shadowShader) {
    if (!shadowShader) return;

    glm::mat4 identity(1.0f);
    shadowShader->SetMat4("u_Model", identity);

    m_geometry->DrawRoads();
    m_geometry->DrawSidewalks();
    m_geometry->DrawCurbs();
    m_buildings->Draw();
    m_streetFurniture->DrawStructures();
}

void CityEnvironment::Render(gfx::Shader* sceneShader, gfx::Shader* emissiveShader,
                             int nsSignalState, int ewSignalState,
                             int nsLeftSignalState, int ewLeftSignalState,
                             bool nsPedWalk, bool ewPedWalk)
{
    if (!sceneShader) return;

    glm::mat4 identity(1.0f);
    sceneShader->SetMat4("u_Model", identity);

    m_geometry->DrawRoads();
    m_geometry->DrawSidewalks();
    m_geometry->DrawCurbs();
    m_geometry->DrawGrass();

    m_markings->Draw();
    m_buildings->Draw();
    m_streetFurniture->DrawStructures();

    if (emissiveShader) {
        RenderEmissiveSignalHeads(emissiveShader, nsSignalState, ewSignalState, nsLeftSignalState, ewLeftSignalState, nsPedWalk, ewPedWalk);
    }
}

void CityEnvironment::RenderEmissiveSignalHeads(gfx::Shader* emissiveShader,
                                               int nsSignalState, int ewSignalState,
                                               int nsLeftSignalState, int ewLeftSignalState,
                                               bool nsPedWalk, bool ewPedWalk)
{
    emissiveShader->Bind();

    float hw = m_settings.roadWidth * 0.5f;
    float curb = hw + 1.2f;

    glm::vec4 brightRed(1.0f, 0.08f, 0.08f, 1.0f);
    glm::vec4 dimRed(0.22f, 0.02f, 0.02f, 1.0f);

    glm::vec4 brightYellow(1.0f, 0.85f, 0.12f, 1.0f);
    glm::vec4 dimYellow(0.20f, 0.16f, 0.02f, 1.0f);

    glm::vec4 brightGreen(0.12f, 1.0f, 0.25f, 1.0f);
    glm::vec4 dimGreen(0.02f, 0.20f, 0.04f, 1.0f);

    glm::vec4 brightWhite(0.95f, 0.95f, 0.95f, 1.0f);
    glm::vec4 dimWhite(0.15f, 0.15f, 0.15f, 1.0f);

    auto render3AspectHead = [&](const glm::vec3& headCenter, int state) {
        glm::mat4 mRed = glm::translate(glm::mat4(1.0f), headCenter + glm::vec3(0.0f, 0.45f, 0.0f));
        emissiveShader->SetMat4("u_Model", mRed);
        emissiveShader->SetVec4("u_EmissiveColor", (state == 0) ? brightRed : dimRed);
        m_lampBulbMesh->Draw();

        glm::mat4 mYel = glm::translate(glm::mat4(1.0f), headCenter);
        emissiveShader->SetMat4("u_Model", mYel);
        emissiveShader->SetVec4("u_EmissiveColor", (state == 1) ? brightYellow : dimYellow);
        m_lampBulbMesh->Draw();

        glm::mat4 mGrn = glm::translate(glm::mat4(1.0f), headCenter + glm::vec3(0.0f, -0.45f, 0.0f));
        emissiveShader->SetMat4("u_Model", mGrn);
        emissiveShader->SetVec4("u_EmissiveColor", (state == 2) ? brightGreen : dimGreen);
        m_lampBulbMesh->Draw();
    };

    auto renderPedestrianHead = [&](const glm::vec3& headCenter, bool walk) {
        glm::mat4 mStop = glm::translate(glm::mat4(1.0f), headCenter + glm::vec3(0.0f, 0.18f, 0.0f));
        emissiveShader->SetMat4("u_Model", mStop);
        emissiveShader->SetVec4("u_EmissiveColor", (!walk) ? brightRed : dimRed);
        m_lampBulbMesh->Draw();

        glm::mat4 mWalk = glm::translate(glm::mat4(1.0f), headCenter + glm::vec3(0.0f, -0.18f, 0.0f));
        emissiveShader->SetMat4("u_Model", mWalk);
        emissiveShader->SetVec4("u_EmissiveColor", walk ? brightWhite : dimWhite);
        m_lampBulbMesh->Draw();
    };

    // North Approach
    render3AspectHead(glm::vec3(curb - 6.0f, 5.0f, -curb), nsSignalState);
    render3AspectHead(glm::vec3(curb - 8.0f, 5.0f, -curb), nsLeftSignalState);
    renderPedestrianHead(glm::vec3(curb, 2.4f, -curb), nsPedWalk);

    // South Approach
    render3AspectHead(glm::vec3(-curb + 6.0f, 5.0f, curb), nsSignalState);
    render3AspectHead(glm::vec3(-curb + 8.0f, 5.0f, curb), nsLeftSignalState);
    renderPedestrianHead(glm::vec3(-curb, 2.4f, curb), nsPedWalk);

    // West Approach
    render3AspectHead(glm::vec3(-curb, 5.0f, -curb + 6.0f), ewSignalState);
    render3AspectHead(glm::vec3(-curb, 5.0f, -curb + 8.0f), ewLeftSignalState);
    renderPedestrianHead(glm::vec3(-curb, 2.4f, -curb), ewPedWalk);

    // East Approach
    render3AspectHead(glm::vec3(curb, 5.0f, curb - 6.0f), ewSignalState);
    render3AspectHead(glm::vec3(curb, 5.0f, curb - 8.0f), ewLeftSignalState);
    renderPedestrianHead(glm::vec3(curb, 2.4f, curb), ewPedWalk);
}

void CityEnvironment::GenerateDebugOverlayMeshes() {
    if (!m_roadNetwork) return;

    glm::vec3 up(0.0f, 1.0f, 0.0f);

    auto addLineQuad = [](std::vector<gfx::Vertex>& verts, std::vector<uint32_t>& inds,
                          const glm::vec3& p0, const glm::vec3& p1, float thickness, const glm::vec4& col) {
        glm::vec3 dir = glm::normalize(p1 - p0);
        glm::vec3 perp(-dir.z * thickness * 0.5f, 0.0f, dir.x * thickness * 0.5f);
        uint32_t b = static_cast<uint32_t>(verts.size());

        verts.push_back({ p0 - perp, {0,1,0}, {0,0}, col });
        verts.push_back({ p0 + perp, {0,1,0}, {1,0}, col });
        verts.push_back({ p1 + perp, {0,1,0}, {1,1}, col });
        verts.push_back({ p1 - perp, {0,1,0}, {0,1}, col });

        inds.push_back(b); inds.push_back(b+1); inds.push_back(b+2);
        inds.push_back(b+2); inds.push_back(b+3); inds.push_back(b);
    };

    // 1. Lane Centrelines Mesh (Cyan)
    std::vector<gfx::Vertex> laneVerts;
    std::vector<uint32_t> laneInds;
    glm::vec4 laneColor(0.1f, 0.85f, 0.95f, 1.0f);

    for (const auto& lane : m_roadNetwork->GetAllLanes()) {
        glm::vec3 p0 = lane.startPoint + glm::vec3(0.0f, 0.06f, 0.0f);
        glm::vec3 p1 = lane.endPoint + glm::vec3(0.0f, 0.06f, 0.0f);
        addLineQuad(laneVerts, laneInds, p0, p1, 0.22f, laneColor);
    }
    m_debugLaneMesh = std::make_unique<gfx::Mesh>(laneVerts, laneInds);

    // 2. Turning-Path Béziers (Magenta)
    std::vector<gfx::Vertex> bezierVerts;
    std::vector<uint32_t> bezierInds;
    glm::vec4 bezierColor(0.95f, 0.2f, 0.85f, 1.0f);

    for (const auto& path : m_roadNetwork->GetAllPaths()) {
        constexpr int segments = 24;
        glm::vec3 prev = path.Evaluate(0.0f) + glm::vec3(0.0f, 0.08f, 0.0f);
        for (int i = 1; i <= segments; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(segments);
            glm::vec3 curr = path.Evaluate(t) + glm::vec3(0.0f, 0.08f, 0.0f);
            addLineQuad(bezierVerts, bezierInds, prev, curr, 0.24f, bezierColor);
            prev = curr;
        }
    }
    m_debugBezierMesh = std::make_unique<gfx::Mesh>(bezierVerts, bezierInds);

    // 3. Conflict Zones (Translucent Red AABB quads)
    std::vector<gfx::Vertex> conflictVerts;
    std::vector<uint32_t> conflictInds;
    glm::vec4 conflictColor(0.95f, 0.15f, 0.15f, 0.85f);

    for (const auto& cz : m_roadNetwork->GetAllConflictZones()) {
        uint32_t b = static_cast<uint32_t>(conflictVerts.size());
        float y = 0.10f;
        conflictVerts.push_back({ { cz.aabbMin.x, y, cz.aabbMax.z }, up, {0,0}, conflictColor });
        conflictVerts.push_back({ { cz.aabbMax.x, y, cz.aabbMax.z }, up, {1,0}, conflictColor });
        conflictVerts.push_back({ { cz.aabbMax.x, y, cz.aabbMin.z }, up, {1,1}, conflictColor });
        conflictVerts.push_back({ { cz.aabbMin.x, y, cz.aabbMin.z }, up, {0,1}, conflictColor });

        conflictInds.push_back(b); conflictInds.push_back(b+1); conflictInds.push_back(b+2);
        conflictInds.push_back(b+2); conflictInds.push_back(b+3); conflictInds.push_back(b);
    }
    m_debugConflictMesh = std::make_unique<gfx::Mesh>(conflictVerts, conflictInds);

    // 4. Stop Lines (Bright Orange-Red)
    std::vector<gfx::Vertex> stopVerts;
    std::vector<uint32_t> stopInds;
    glm::vec4 stopColor(1.0f, 0.35f, 0.1f, 1.0f);

    for (const auto& sl : m_roadNetwork->GetAllStopLines()) {
        glm::vec3 perp(-sl.normal.z * sl.width * 0.5f, 0.0f, sl.normal.x * sl.width * 0.5f);
        glm::vec3 p0 = sl.position - perp + glm::vec3(0.0f, 0.07f, 0.0f);
        glm::vec3 p1 = sl.position + perp + glm::vec3(0.0f, 0.07f, 0.0f);
        addLineQuad(stopVerts, stopInds, p0, p1, 0.45f, stopColor);
    }
    m_debugStopLineMesh = std::make_unique<gfx::Mesh>(stopVerts, stopInds);

    // 5. Entry / Exit Points (Green Entry, Orange Exit)
    std::vector<gfx::Vertex> eeVerts;
    std::vector<uint32_t> eeInds;
    glm::vec4 entryCol(0.2f, 0.95f, 0.3f, 1.0f);
    glm::vec4 exitCol(0.95f, 0.65f, 0.1f, 1.0f);

    auto addDiamond = [](std::vector<gfx::Vertex>& verts, std::vector<uint32_t>& inds,
                         const glm::vec3& c, float sz, const glm::vec4& col) {
        uint32_t b = static_cast<uint32_t>(verts.size());
        float y = c.y + 0.12f;
        verts.push_back({ { c.x, y, c.z + sz }, {0,1,0}, {0,0}, col });
        verts.push_back({ { c.x + sz, y, c.z }, {0,1,0}, {1,0}, col });
        verts.push_back({ { c.x, y, c.z - sz }, {0,1,0}, {1,1}, col });
        verts.push_back({ { c.x - sz, y, c.z }, {0,1,0}, {0,1}, col });

        inds.push_back(b); inds.push_back(b+1); inds.push_back(b+2);
        inds.push_back(b+2); inds.push_back(b+3); inds.push_back(b);
    };

    for (const auto& ep : m_roadNetwork->GetEntryPoints()) {
        addDiamond(eeVerts, eeInds, ep.position, 0.7f, entryCol);
    }
    for (const auto& xp : m_roadNetwork->GetExitPoints()) {
        addDiamond(eeVerts, eeInds, xp.position, 0.7f, exitCol);
    }
    m_debugEntryExitMesh = std::make_unique<gfx::Mesh>(eeVerts, eeInds);
}

void CityEnvironment::RenderDebugOverlay(gfx::Shader* debugShader,
                                        bool showLaneCenters,
                                        bool showTurningBeziers,
                                        bool showConflictZones,
                                        bool showStopLines,
                                        bool showEntryExit)
{
    if (!debugShader) return;

    glm::mat4 identity(1.0f);
    debugShader->SetMat4("u_Model", identity);

    if (showLaneCenters && m_debugLaneMesh) {
        m_debugLaneMesh->Draw();
    }
    if (showTurningBeziers && m_debugBezierMesh) {
        m_debugBezierMesh->Draw();
    }
    if (showConflictZones && m_debugConflictMesh) {
        m_debugConflictMesh->Draw();
    }
    if (showStopLines && m_debugStopLineMesh) {
        m_debugStopLineMesh->Draw();
    }
    if (showEntryExit && m_debugEntryExitMesh) {
        m_debugEntryExitMesh->Draw();
    }
}

} // namespace SmartCity::city

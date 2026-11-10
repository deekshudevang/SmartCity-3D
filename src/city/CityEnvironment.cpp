#include "city/CityEnvironment.h"
#include <glm/gtc/matrix_transform.hpp>

namespace SmartCity::city {

CityEnvironment::CityEnvironment(const core::IntersectionSettings& settings)
    : m_settings(settings)
{
    m_geometry = std::make_unique<IntersectionGeometry>(settings);
    m_markings = std::make_unique<RoadMarkings>(settings);
    m_buildings = std::make_unique<BuildingGenerator>(settings);
    m_streetFurniture = std::make_unique<StreetFurniture>(settings);

    m_lampBulbMesh = gfx::Mesh::CreateSphere(0.18f, 10, 14, glm::vec4(1.0f));
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

} // namespace SmartCity::city

#include "city/StreetFurniture.h"
#include <cmath>

namespace SmartCity::city {

StreetFurniture::StreetFurniture(const core::IntersectionSettings& settings)
    : m_settings(settings)
{
    GenerateStructures();
}

void StreetFurniture::GenerateStructures() {
    float hw = m_settings.roadWidth * 0.5f;
    float curb = hw + 1.2f;

    std::vector<gfx::Vertex> poleVerts;
    std::vector<uint32_t> poleInds;

    std::vector<gfx::Vertex> housingVerts;
    std::vector<uint32_t> housingInds;

    std::vector<gfx::Vertex> treeVerts;
    std::vector<uint32_t> treeInds;

    glm::vec4 darkSteel(0.25f, 0.27f, 0.30f, 1.0f);
    glm::vec4 housingBlack(0.12f, 0.12f, 0.14f, 1.0f);
    glm::vec4 woodBrown(0.35f, 0.22f, 0.15f, 1.0f);
    glm::vec4 foliageGreen(0.18f, 0.38f, 0.16f, 1.0f);

    auto addBox = [](std::vector<gfx::Vertex>& verts, std::vector<uint32_t>& inds,
                     const glm::vec3& center, const glm::vec3& size, const glm::vec4& color) {
        glm::vec3 h = size * 0.5f;
        uint32_t base = static_cast<uint32_t>(verts.size());

        // Front
        verts.push_back({ center + glm::vec3(-h.x, -h.y,  h.z), { 0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f }, color });
        verts.push_back({ center + glm::vec3( h.x, -h.y,  h.z), { 0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f }, color });
        verts.push_back({ center + glm::vec3( h.x,  h.y,  h.z), { 0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f }, color });
        verts.push_back({ center + glm::vec3(-h.x,  h.y,  h.z), { 0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f }, color });

        // Back
        verts.push_back({ center + glm::vec3( h.x, -h.y, -h.z), { 0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f }, color });
        verts.push_back({ center + glm::vec3(-h.x, -h.y, -h.z), { 0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f }, color });
        verts.push_back({ center + glm::vec3(-h.x,  h.y, -h.z), { 0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f }, color });
        verts.push_back({ center + glm::vec3( h.x,  h.y, -h.z), { 0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f }, color });

        // Top
        verts.push_back({ center + glm::vec3(-h.x,  h.y,  h.z), { 0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f }, color });
        verts.push_back({ center + glm::vec3( h.x,  h.y,  h.z), { 0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f }, color });
        verts.push_back({ center + glm::vec3( h.x,  h.y, -h.z), { 0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f }, color });
        verts.push_back({ center + glm::vec3(-h.x,  h.y, -h.z), { 0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f }, color });

        // Bottom
        verts.push_back({ center + glm::vec3(-h.x, -h.y, -h.z), { 0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f }, color });
        verts.push_back({ center + glm::vec3( h.x, -h.y, -h.z), { 0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f }, color });
        verts.push_back({ center + glm::vec3( h.x, -h.y,  h.z), { 0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f }, color });
        verts.push_back({ center + glm::vec3(-h.x, -h.y,  h.z), { 0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f }, color });

        // Right
        verts.push_back({ center + glm::vec3( h.x, -h.y,  h.z), { 1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f }, color });
        verts.push_back({ center + glm::vec3( h.x, -h.y, -h.z), { 1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f }, color });
        verts.push_back({ center + glm::vec3( h.x,  h.y, -h.z), { 1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f }, color });
        verts.push_back({ center + glm::vec3( h.x,  h.y,  h.z), { 1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f }, color });

        // Left
        verts.push_back({ center + glm::vec3(-h.x, -h.y, -h.z), { -1.0f, 0.0f,  0.0f }, { 0.0f, 0.0f }, color });
        verts.push_back({ center + glm::vec3(-h.x, -h.y,  h.z), { -1.0f, 0.0f,  0.0f }, { 1.0f, 0.0f }, color });
        verts.push_back({ center + glm::vec3(-h.x,  h.y,  h.z), { -1.0f, 0.0f,  0.0f }, { 1.0f, 1.0f }, color });
        verts.push_back({ center + glm::vec3(-h.x,  h.y, -h.z), { -1.0f, 0.0f,  0.0f }, { 0.0f, 1.0f }, color });

        for (uint32_t i = 0; i < 6; ++i) {
            uint32_t f = base + i * 4;
            inds.push_back(f);
            inds.push_back(f + 1);
            inds.push_back(f + 2);
            inds.push_back(f + 2);
            inds.push_back(f + 3);
            inds.push_back(f);
        }
    };

    struct CornerPole {
        glm::vec3 pos;
        glm::vec3 mastExt;
    };
    std::vector<CornerPole> cornerPoles = {
        { {  curb, 0.0f, -curb }, { -8.5f, 0.0f, 0.0f } }, // NE corner
        { { -curb, 0.0f, -curb }, { 0.0f, 0.0f, 8.5f } },  // NW corner
        { { -curb, 0.0f,  curb }, { 8.5f, 0.0f, 0.0f } },  // SW corner
        { {  curb, 0.0f,  curb }, { 0.0f, 0.0f, -8.5f } }  // SE corner
    };

    for (const auto& cp : cornerPoles) {
        addBox(poleVerts, poleInds, cp.pos + glm::vec3(0.0f, 3.25f, 0.0f), glm::vec3(0.35f, 6.5f, 0.35f), darkSteel);
        glm::vec3 armCenter = cp.pos + glm::vec3(cp.mastExt.x * 0.5f, 6.0f, cp.mastExt.z * 0.5f);
        glm::vec3 armSize(
            std::abs(cp.mastExt.x) > 0.0f ? std::abs(cp.mastExt.x) : 0.25f,
            0.25f,
            std::abs(cp.mastExt.z) > 0.0f ? std::abs(cp.mastExt.z) : 0.25f
        );
        addBox(poleVerts, poleInds, armCenter, armSize, darkSteel);

        glm::vec3 headCenter = cp.pos + glm::vec3(cp.mastExt.x * 0.7f, 5.0f, cp.mastExt.z * 0.7f);
        addBox(housingVerts, housingInds, headCenter, glm::vec3(0.6f, 1.8f, 0.5f), housingBlack);

        glm::vec3 leftHeadCenter = cp.pos + glm::vec3(cp.mastExt.x * 0.9f, 5.0f, cp.mastExt.z * 0.9f);
        addBox(housingVerts, housingInds, leftHeadCenter, glm::vec3(0.6f, 1.8f, 0.5f), housingBlack);

        glm::vec3 pedHeadCenter = cp.pos + glm::vec3(0.0f, 2.4f, 0.0f);
        addBox(housingVerts, housingInds, pedHeadCenter, glm::vec3(0.45f, 0.85f, 0.4f), housingBlack);
    }

    m_streetlightSources.clear();
    std::vector<glm::vec3> streetlightPositions = {
        {  curb, 0.0f, -35.0f }, {  curb, 0.0f, -65.0f },
        { -curb, 0.0f, -35.0f }, { -curb, 0.0f, -65.0f },
        {  curb, 0.0f,  35.0f }, {  curb, 0.0f,  65.0f },
        { -curb, 0.0f,  35.0f }, { -curb, 0.0f,  65.0f },
        {  35.0f, 0.0f,  curb }, {  65.0f, 0.0f,  curb },
        {  35.0f, 0.0f, -curb }, {  65.0f, 0.0f, -curb },
        { -35.0f, 0.0f,  curb }, { -65.0f, 0.0f,  curb },
        { -35.0f, 0.0f, -curb }, { -65.0f, 0.0f, -curb }
    };

    for (const auto& sp : streetlightPositions) {
        addBox(poleVerts, poleInds, sp + glm::vec3(0.0f, 4.5f, 0.0f), glm::vec3(0.25f, 9.0f, 0.25f), darkSteel);
        glm::vec3 armDir = (std::abs(sp.x) == curb) ? glm::vec3((sp.x > 0 ? -1.5f : 1.5f), 0.0f, 0.0f)
                                                    : glm::vec3(0.0f, 0.0f, (sp.z > 0 ? -1.5f : 1.5f));
        addBox(poleVerts, poleInds, sp + glm::vec3(armDir.x * 0.5f, 8.8f, armDir.z * 0.5f), glm::vec3(std::abs(armDir.x) > 0 ? 1.5f : 0.2f, 0.15f, std::abs(armDir.z) > 0 ? 1.5f : 0.2f), darkSteel);

        glm::vec3 fixturePos = sp + glm::vec3(armDir.x, 8.6f, armDir.z);
        addBox(housingVerts, housingInds, fixturePos, glm::vec3(0.6f, 0.25f, 0.6f), housingBlack);

        gfx::PointLight pl;
        pl.position = fixturePos + glm::vec3(0.0f, -0.3f, 0.0f);
        pl.color = glm::vec3(1.0f, 0.88f, 0.68f);
        pl.intensity = 1.4f;
        pl.radius = 28.0f;
        m_streetlightSources.push_back(pl);
    }

    std::vector<glm::vec3> treePositions = {
        {  curb + 2.5f, 0.0f, -25.0f }, {  curb + 2.5f, 0.0f, -50.0f },
        { -curb - 2.5f, 0.0f, -25.0f }, { -curb - 2.5f, 0.0f, -50.0f },
        {  curb + 2.5f, 0.0f,  25.0f }, {  curb + 2.5f, 0.0f,  50.0f },
        { -curb - 2.5f, 0.0f,  25.0f }, { -curb - 2.5f, 0.0f,  50.0f },
        {  25.0f, 0.0f,  curb + 2.5f }, {  50.0f, 0.0f,  curb + 2.5f },
        {  25.0f, 0.0f, -curb - 2.5f }, {  50.0f, 0.0f, -curb - 2.5f },
        { -25.0f, 0.0f,  curb + 2.5f }, { -50.0f, 0.0f,  curb + 2.5f },
        { -25.0f, 0.0f, -curb - 2.5f }, { -50.0f, 0.0f, -curb - 2.5f }
    };

    for (const auto& tp : treePositions) {
        addBox(treeVerts, treeInds, tp + glm::vec3(0.0f, 1.75f, 0.0f), glm::vec3(0.45f, 3.5f, 0.45f), woodBrown);
        addBox(treeVerts, treeInds, tp + glm::vec3(0.0f, 4.5f, 0.0f), glm::vec3(3.2f, 2.5f, 3.2f), foliageGreen);
        addBox(treeVerts, treeInds, tp + glm::vec3(0.0f, 6.2f, 0.0f), glm::vec3(2.2f, 2.0f, 2.2f), foliageGreen * 1.15f);
    }

    m_metalPolesMesh = std::make_unique<gfx::Mesh>(poleVerts, poleInds);
    m_signalHousingsMesh = std::make_unique<gfx::Mesh>(housingVerts, housingInds);
    m_treesMesh = std::make_unique<gfx::Mesh>(treeVerts, treeInds);
}

void StreetFurniture::DrawStructures() const {
    if (m_metalPolesMesh) m_metalPolesMesh->Draw();
    if (m_signalHousingsMesh) m_signalHousingsMesh->Draw();
    if (m_treesMesh) m_treesMesh->Draw();
}

} // namespace SmartCity::city

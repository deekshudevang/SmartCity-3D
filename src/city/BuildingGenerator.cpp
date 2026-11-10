#include "city/BuildingGenerator.h"

namespace SmartCity::city {

BuildingGenerator::BuildingGenerator(const core::IntersectionSettings& settings)
    : m_settings(settings)
{
    GenerateCityBlocks();
}

void BuildingGenerator::GenerateCityBlocks() {
    float setback = m_settings.roadWidth * 0.5f + m_settings.sidewalkWidth + 4.0f;

    m_buildings = {
        // North-East Quadrant (+X, -Z)
        { {  setback + 10.0f, 0.0f, -setback - 10.0f }, { 18.0f, 38.0f, 18.0f }, { 0.28f, 0.32f, 0.38f, 1.0f }, { 0.65f, 0.75f, 0.85f, 1.0f }, 11 },
        { {  setback + 32.0f, 0.0f, -setback - 10.0f }, { 16.0f, 26.0f, 16.0f }, { 0.58f, 0.35f, 0.28f, 1.0f }, { 0.82f, 0.82f, 0.80f, 1.0f }, 7 },
        { {  setback + 10.0f, 0.0f, -setback - 32.0f }, { 18.0f, 48.0f, 18.0f }, { 0.35f, 0.38f, 0.42f, 1.0f }, { 0.60f, 0.72f, 0.85f, 1.0f }, 14 },

        // North-West Quadrant (-X, -Z)
        { { -setback - 10.0f, 0.0f, -setback - 10.0f }, { 18.0f, 52.0f, 18.0f }, { 0.22f, 0.25f, 0.30f, 1.0f }, { 0.70f, 0.80f, 0.90f, 1.0f }, 16 },
        { { -setback - 32.0f, 0.0f, -setback - 10.0f }, { 18.0f, 32.0f, 16.0f }, { 0.62f, 0.58f, 0.52f, 1.0f }, { 0.75f, 0.78f, 0.82f, 1.0f }, 9 },
        { { -setback - 10.0f, 0.0f, -setback - 32.0f }, { 16.0f, 22.0f, 18.0f }, { 0.52f, 0.30f, 0.25f, 1.0f }, { 0.85f, 0.85f, 0.80f, 1.0f }, 6 },

        // South-East Quadrant (+X, +Z)
        { {  setback + 10.0f, 0.0f,  setback + 10.0f }, { 18.0f, 44.0f, 18.0f }, { 0.32f, 0.35f, 0.38f, 1.0f }, { 0.65f, 0.78f, 0.90f, 1.0f }, 13 },
        { {  setback + 32.0f, 0.0f,  setback + 10.0f }, { 18.0f, 28.0f, 18.0f }, { 0.55f, 0.45f, 0.38f, 1.0f }, { 0.80f, 0.82f, 0.85f, 1.0f }, 8 },
        { {  setback + 10.0f, 0.0f,  setback + 32.0f }, { 16.0f, 36.0f, 18.0f }, { 0.25f, 0.28f, 0.35f, 1.0f }, { 0.70f, 0.80f, 0.88f, 1.0f }, 10 },

        // South-West Quadrant (-X, +Z)
        { { -setback - 10.0f, 0.0f,  setback + 10.0f }, { 18.0f, 34.0f, 18.0f }, { 0.48f, 0.32f, 0.26f, 1.0f }, { 0.82f, 0.82f, 0.82f, 1.0f }, 10 },
        { { -setback - 32.0f, 0.0f,  setback + 10.0f }, { 18.0f, 56.0f, 18.0f }, { 0.20f, 0.22f, 0.28f, 1.0f }, { 0.68f, 0.78f, 0.92f, 1.0f }, 17 },
        { { -setback - 10.0f, 0.0f,  setback + 32.0f }, { 16.0f, 24.0f, 18.0f }, { 0.58f, 0.55f, 0.50f, 1.0f }, { 0.80f, 0.80f, 0.78f, 1.0f }, 7 }
    };

    std::vector<gfx::Vertex> vertices;
    std::vector<uint32_t> indices;

    auto addBox = [&](const glm::vec3& center, const glm::vec3& size, const glm::vec4& color) {
        glm::vec3 h = size * 0.5f;
        uint32_t base = static_cast<uint32_t>(vertices.size());

        // Front (+Z)
        vertices.push_back({ center + glm::vec3(-h.x, -h.y,  h.z), { 0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f }, color });
        vertices.push_back({ center + glm::vec3( h.x, -h.y,  h.z), { 0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f }, color });
        vertices.push_back({ center + glm::vec3( h.x,  h.y,  h.z), { 0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f }, color });
        vertices.push_back({ center + glm::vec3(-h.x,  h.y,  h.z), { 0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f }, color });

        // Back (-Z)
        vertices.push_back({ center + glm::vec3( h.x, -h.y, -h.z), { 0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f }, color });
        vertices.push_back({ center + glm::vec3(-h.x, -h.y, -h.z), { 0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f }, color });
        vertices.push_back({ center + glm::vec3(-h.x,  h.y, -h.z), { 0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f }, color });
        vertices.push_back({ center + glm::vec3( h.x,  h.y, -h.z), { 0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f }, color });

        // Top (+Y)
        vertices.push_back({ center + glm::vec3(-h.x,  h.y,  h.z), { 0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f }, color * 0.9f });
        vertices.push_back({ center + glm::vec3( h.x,  h.y,  h.z), { 0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f }, color * 0.9f });
        vertices.push_back({ center + glm::vec3( h.x,  h.y, -h.z), { 0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f }, color * 0.9f });
        vertices.push_back({ center + glm::vec3(-h.x,  h.y, -h.z), { 0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f }, color * 0.9f });

        // Right (+X)
        vertices.push_back({ center + glm::vec3( h.x, -h.y,  h.z), { 1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f }, color });
        vertices.push_back({ center + glm::vec3( h.x, -h.y, -h.z), { 1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f }, color });
        vertices.push_back({ center + glm::vec3( h.x,  h.y, -h.z), { 1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f }, color });
        vertices.push_back({ center + glm::vec3( h.x,  h.y,  h.z), { 1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f }, color });

        // Left (-X)
        vertices.push_back({ center + glm::vec3(-h.x, -h.y, -h.z), { -1.0f, 0.0f,  0.0f }, { 0.0f, 0.0f }, color });
        vertices.push_back({ center + glm::vec3(-h.x, -h.y,  h.z), { -1.0f, 0.0f,  0.0f }, { 1.0f, 0.0f }, color });
        vertices.push_back({ center + glm::vec3(-h.x,  h.y,  h.z), { -1.0f, 0.0f,  0.0f }, { 1.0f, 1.0f }, color });
        vertices.push_back({ center + glm::vec3(-h.x,  h.y, -h.z), { -1.0f, 0.0f,  0.0f }, { 0.0f, 1.0f }, color });

        for (uint32_t i = 0; i < 5; ++i) {
            uint32_t f = base + i * 4;
            indices.push_back(f);
            indices.push_back(f + 1);
            indices.push_back(f + 2);
            indices.push_back(f + 2);
            indices.push_back(f + 3);
            indices.push_back(f);
        }
    };

    for (const auto& b : m_buildings) {
        glm::vec3 center = b.position + glm::vec3(0.0f, b.size.y * 0.5f, 0.0f);
        addBox(center, b.size, b.primaryColor);

        float floorHeight = b.size.y / static_cast<float>(b.numStories);
        for (int s = 1; s < b.numStories; ++s) {
            float y = b.position.y + s * floorHeight;
            glm::vec3 bandCenter(b.position.x, y, b.position.z);
            glm::vec3 bandSize(b.size.x + 0.08f, floorHeight * 0.55f, b.size.z + 0.08f);
            addBox(bandCenter, bandSize, b.windowColor);
        }

        glm::vec3 roofBoxCenter(b.position.x, b.position.y + b.size.y + 1.25f, b.position.z);
        glm::vec3 roofBoxSize(b.size.x * 0.45f, 2.5f, b.size.z * 0.45f);
        addBox(roofBoxCenter, roofBoxSize, glm::vec4(0.35f, 0.36f, 0.38f, 1.0f));
    }

    m_combinedBuildingsMesh = std::make_unique<gfx::Mesh>(vertices, indices);
}

void BuildingGenerator::Draw() const {
    if (m_combinedBuildingsMesh) {
        m_combinedBuildingsMesh->Draw();
    }
}

} // namespace SmartCity::city

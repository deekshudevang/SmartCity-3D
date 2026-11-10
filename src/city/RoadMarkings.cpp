#include "city/RoadMarkings.h"

namespace SmartCity::city {

RoadMarkings::RoadMarkings(const core::IntersectionSettings& settings)
    : m_settings(settings)
{
    GenerateMarkings();
}

void RoadMarkings::GenerateMarkings() {
    std::vector<gfx::Vertex> vertices;
    std::vector<uint32_t> indices;

    float hw = m_settings.roadWidth * 0.5f;
    float len = m_settings.approachLength;
    float yOffset = 0.015f;

    glm::vec4 white(0.95f, 0.95f, 0.95f, 1.0f);
    glm::vec4 yellow(0.92f, 0.78f, 0.15f, 1.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    auto addStripe = [&](float xMin, float xMax, float zMin, float zMax, const glm::vec4& color) {
        uint32_t base = static_cast<uint32_t>(vertices.size());
        vertices.push_back({ { xMin, yOffset, zMax }, up, { 0.0f, 0.0f }, color });
        vertices.push_back({ { xMax, yOffset, zMax }, up, { 1.0f, 0.0f }, color });
        vertices.push_back({ { xMax, yOffset, zMin }, up, { 1.0f, 1.0f }, color });
        vertices.push_back({ { xMin, yOffset, zMin }, up, { 0.0f, 1.0f }, color });

        indices.push_back(base);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
        indices.push_back(base);
    };

    float stopLineDist = m_settings.stopLineDistance;
    float crosswalkDist = m_settings.crosswalkDistance;
    float crosswalkWidth = m_settings.crosswalkWidth;

    // 1. Double Solid Yellow Centerline
    float dyWidth = 0.15f;
    float dyGap = 0.15f;

    // North (North = -Z)
    addStripe(-dyGap - dyWidth, -dyGap, -len, -stopLineDist, yellow);
    addStripe( dyGap,  dyGap + dyWidth, -len, -stopLineDist, yellow);

    // South (South = +Z)
    addStripe(-dyGap - dyWidth, -dyGap, stopLineDist, len, yellow);
    addStripe( dyGap,  dyGap + dyWidth, stopLineDist, len, yellow);

    // East (East = +X)
    addStripe(stopLineDist, len, -dyGap - dyWidth, -dyGap, yellow);
    addStripe(stopLineDist, len,  dyGap,  dyGap + dyWidth, yellow);

    // West (West = -X)
    addStripe(-len, -stopLineDist, -dyGap - dyWidth, -dyGap, yellow);
    addStripe(-len, -stopLineDist,  dyGap,  dyGap + dyWidth, yellow);

    // 2. Stop Lines
    float stopLineWidth = 0.5f;
    addStripe(0.2f, hw - 0.2f, -stopLineDist, -stopLineDist + stopLineWidth, white);
    addStripe(-hw + 0.2f, -0.2f, stopLineDist - stopLineWidth, stopLineDist, white);
    addStripe(stopLineDist - stopLineWidth, stopLineDist, 0.2f, hw - 0.2f, white);
    addStripe(-stopLineDist, -stopLineDist + stopLineWidth, -hw + 0.2f, -0.2f, white);

    // 3. Pedestrian Zebra Crossings
    float barWidth = 0.5f;
    float barGap = 0.5f;

    // North Zebra Crosswalk
    for (float x = -hw + 0.3f; x <= hw - 0.3f - barWidth; x += (barWidth + barGap)) {
        addStripe(x, x + barWidth, -crosswalkDist - crosswalkWidth, -crosswalkDist, white);
    }

    // South Zebra Crosswalk
    for (float x = -hw + 0.3f; x <= hw - 0.3f - barWidth; x += (barWidth + barGap)) {
        addStripe(x, x + barWidth, crosswalkDist, crosswalkDist + crosswalkWidth, white);
    }

    // East Zebra Crosswalk
    for (float z = -hw + 0.3f; z <= hw - 0.3f - barWidth; z += (barWidth + barGap)) {
        addStripe(crosswalkDist, crosswalkDist + crosswalkWidth, z, z + barWidth, white);
    }

    // West Zebra Crosswalk
    for (float z = -hw + 0.3f; z <= hw - 0.3f - barWidth; z += (barWidth + barGap)) {
        addStripe(-crosswalkDist - crosswalkWidth, -crosswalkDist, z, z + barWidth, white);
    }

    // 4. Dashed White Lane Dividers
    float laneDivX = hw * 0.5f;
    float dashLen = 3.0f;
    float dashGap = 4.0f;

    for (float z = -len; z <= -stopLineDist - dashLen; z += (dashLen + dashGap)) {
        addStripe(-laneDivX - 0.08f, -laneDivX + 0.08f, z, z + dashLen, white);
        addStripe( laneDivX - 0.08f,  laneDivX + 0.08f, z, z + dashLen, white);
    }

    for (float z = stopLineDist; z <= len - dashLen; z += (dashLen + dashGap)) {
        addStripe(-laneDivX - 0.08f, -laneDivX + 0.08f, z, z + dashLen, white);
        addStripe( laneDivX - 0.08f,  laneDivX + 0.08f, z, z + dashLen, white);
    }

    for (float x = stopLineDist; x <= len - dashLen; x += (dashLen + dashGap)) {
        addStripe(x, x + dashLen, -laneDivX - 0.08f, -laneDivX + 0.08f, white);
        addStripe(x, x + dashLen,  laneDivX - 0.08f,  laneDivX + 0.08f, white);
    }

    for (float x = -len; x <= -stopLineDist - dashLen; x += (dashLen + dashGap)) {
        addStripe(x, x + dashLen, -laneDivX - 0.08f, -laneDivX + 0.08f, white);
        addStripe(x, x + dashLen,  laneDivX - 0.08f,  laneDivX + 0.08f, white);
    }

    m_markingsMesh = std::make_unique<gfx::Mesh>(vertices, indices);
}

void RoadMarkings::Draw() const {
    if (m_markingsMesh) {
        m_markingsMesh->Draw();
    }
}

} // namespace SmartCity::city

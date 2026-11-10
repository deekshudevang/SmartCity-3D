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

    // 5. Turn Arrows Painted per Lane
    auto addArrow = [&](float cx, float cz, float angleRad, int type) {
        // type: 0 = Through, 1 = Left, 2 = Right
        float cosA = std::cos(angleRad);
        float sinA = std::sin(angleRad);
        auto rotPt = [cx, cz, cosA, sinA](float lx, float lz) -> glm::vec3 {
            float rx = lx * cosA - lz * sinA;
            float rz = lx * sinA + lz * cosA;
            return glm::vec3(cx + rx, 0.015f, cz + rz);
        };

        // Arrow stem
        uint32_t b = static_cast<uint32_t>(vertices.size());
        glm::vec3 s0 = rotPt(-0.15f, -1.8f);
        glm::vec3 s1 = rotPt( 0.15f, -1.8f);
        glm::vec3 s2 = rotPt( 0.15f,  0.5f);
        glm::vec3 s3 = rotPt(-0.15f,  0.5f);

        vertices.push_back({ s0, up, {0,0}, white });
        vertices.push_back({ s1, up, {1,0}, white });
        vertices.push_back({ s2, up, {1,1}, white });
        vertices.push_back({ s3, up, {0,1}, white });

        indices.push_back(b); indices.push_back(b+1); indices.push_back(b+2);
        indices.push_back(b+2); indices.push_back(b+3); indices.push_back(b);

        // Arrow head
        uint32_t h = static_cast<uint32_t>(vertices.size());
        glm::vec3 h0, h1, h2;
        if (type == 0) { // Straight
            h0 = rotPt(-0.6f, 0.4f);
            h1 = rotPt( 0.6f, 0.4f);
            h2 = rotPt( 0.0f, 1.8f);
        } else if (type == 1) { // Left
            h0 = rotPt(-0.6f, 0.0f);
            h1 = rotPt(-0.6f, 1.2f);
            h2 = rotPt(-1.6f, 0.6f);
        } else { // Right
            h0 = rotPt(0.6f, 0.0f);
            h1 = rotPt(0.6f, 1.2f);
            h2 = rotPt(1.6f, 0.6f);
        }

        vertices.push_back({ h0, up, {0,0}, white });
        vertices.push_back({ h1, up, {1,0}, white });
        vertices.push_back({ h2, up, {0.5f,1.0f}, white });

        indices.push_back(h); indices.push_back(h+1); indices.push_back(h+2);
    };

    float arrowDist = stopLineDist + 8.0f;
    float l0 = hw * 0.25f * 0.5f; // lane 0
    float l1 = hw * 0.25f * 1.5f; // lane 1
    float l2 = hw * 0.25f * 2.5f; // lane 2
    float l3 = hw * 0.25f * 3.5f; // lane 3

    // North Inbound (+Z forward)
    addArrow( l0, -arrowDist, 0.0f, 1); // Left
    addArrow( l1, -arrowDist, 0.0f, 0); // Straight
    addArrow( l2, -arrowDist, 0.0f, 0); // Straight
    addArrow( l3, -arrowDist, 0.0f, 2); // Right

    // South Inbound (-Z forward)
    addArrow(-l0,  arrowDist, glm::radians(180.0f), 1); // Left
    addArrow(-l1,  arrowDist, glm::radians(180.0f), 0); // Straight
    addArrow(-l2,  arrowDist, glm::radians(180.0f), 0); // Straight
    addArrow(-l3,  arrowDist, glm::radians(180.0f), 2); // Right

    // East Inbound (-X forward)
    addArrow( arrowDist,  l0, glm::radians(90.0f), 1); // Left
    addArrow( arrowDist,  l1, glm::radians(90.0f), 0); // Straight
    addArrow( arrowDist,  l2, glm::radians(90.0f), 0); // Straight
    addArrow( arrowDist,  l3, glm::radians(90.0f), 2); // Right

    // West Inbound (+X forward)
    addArrow(-arrowDist, -l0, glm::radians(270.0f), 1); // Left
    addArrow(-arrowDist, -l1, glm::radians(270.0f), 0); // Straight
    addArrow(-arrowDist, -l2, glm::radians(270.0f), 0); // Straight
    addArrow(-arrowDist, -l3, glm::radians(270.0f), 2); // Right

    m_markingsMesh = std::make_unique<gfx::Mesh>(vertices, indices);
}

void RoadMarkings::Draw() const {
    if (m_markingsMesh) {
        m_markingsMesh->Draw();
    }
}

} // namespace SmartCity::city

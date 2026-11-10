#include "city/IntersectionGeometry.h"

namespace SmartCity::city {

IntersectionGeometry::IntersectionGeometry(const core::IntersectionSettings& settings)
    : m_settings(settings)
{
    GenerateGeometry();
}

void IntersectionGeometry::GenerateGeometry() {
    float hw = m_settings.roadWidth * 0.5f;
    float len = m_settings.approachLength;
    float sw = m_settings.sidewalkWidth;
    float ch = m_settings.curbHeight;

    glm::vec4 asphaltColor(0.18f, 0.19f, 0.21f, 1.0f);
    glm::vec4 concreteColor(0.68f, 0.69f, 0.70f, 1.0f);
    glm::vec4 curbColor(0.52f, 0.53f, 0.55f, 1.0f);
    glm::vec4 grassColor(0.24f, 0.44f, 0.22f, 1.0f);

    std::vector<gfx::Vertex> roadVertices;
    std::vector<uint32_t> roadIndices;

    auto addQuad = [](std::vector<gfx::Vertex>& verts, std::vector<uint32_t>& inds,
                      const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
                      const glm::vec3& norm, const glm::vec4& col) {
        uint32_t base = static_cast<uint32_t>(verts.size());
        verts.push_back({ p0, norm, { 0.0f, 0.0f }, col });
        verts.push_back({ p1, norm, { 1.0f, 0.0f }, col });
        verts.push_back({ p2, norm, { 1.0f, 1.0f }, col });
        verts.push_back({ p3, norm, { 0.0f, 1.0f }, col });

        inds.push_back(base);
        inds.push_back(base + 1);
        inds.push_back(base + 2);
        inds.push_back(base + 2);
        inds.push_back(base + 3);
        inds.push_back(base);
    };

    glm::vec3 up(0.0f, 1.0f, 0.0f);

    // North-South road (North = -Z, South = +Z)
    addQuad(roadVertices, roadIndices,
        { -hw, 0.0f,  len }, {  hw, 0.0f,  len },
        {  hw, 0.0f, -len }, { -hw, 0.0f, -len },
        up, asphaltColor);

    // East arm (East = +X)
    addQuad(roadVertices, roadIndices,
        {  hw, 0.0f,  hw }, {  len, 0.0f,  hw },
        {  len, 0.0f, -hw }, {  hw, 0.0f, -hw },
        up, asphaltColor);

    // West arm (West = -X)
    addQuad(roadVertices, roadIndices,
        { -len, 0.0f,  hw }, { -hw, 0.0f,  hw },
        { -hw, 0.0f, -hw }, { -len, 0.0f, -hw },
        up, asphaltColor);

    m_roadSurfaceMesh = std::make_unique<gfx::Mesh>(roadVertices, roadIndices);

    // Sidewalks & Curbs
    std::vector<gfx::Vertex> sidewalkVerts;
    std::vector<uint32_t> sidewalkInds;

    std::vector<gfx::Vertex> curbVerts;
    std::vector<uint32_t> curbInds;

    auto generateQuadrantSidewalk = [&](float xMin, float xMax, float zMin, float zMax) {
        addQuad(sidewalkVerts, sidewalkInds,
            { xMin, ch, zMax }, { xMax, ch, zMax },
            { xMax, ch, zMin }, { xMin, ch, zMin },
            up, concreteColor);
    };

    generateQuadrantSidewalk(hw, hw + sw, -len, -hw);
    generateQuadrantSidewalk(hw + sw, len, -hw - sw, -hw);

    generateQuadrantSidewalk(-hw - sw, -hw, -len, -hw);
    generateQuadrantSidewalk(-len, -hw - sw, -hw - sw, -hw);

    generateQuadrantSidewalk(hw, hw + sw, hw, len);
    generateQuadrantSidewalk(hw + sw, len, hw, hw + sw);

    generateQuadrantSidewalk(-hw - sw, -hw, hw, len);
    generateQuadrantSidewalk(-len, -hw - sw, hw, hw + sw);

    // North approach curbs
    addQuad(curbVerts, curbInds, { -hw, 0.0f, -len }, { -hw, 0.0f, -hw }, { -hw, ch, -hw }, { -hw, ch, -len }, { 1.0f, 0.0f, 0.0f }, curbColor);
    addQuad(curbVerts, curbInds, {  hw, 0.0f, -hw }, {  hw, 0.0f, -len }, {  hw, ch, -len }, {  hw, ch, -hw }, { -1.0f, 0.0f, 0.0f }, curbColor);

    // South approach curbs
    addQuad(curbVerts, curbInds, { -hw, 0.0f,  hw }, { -hw, 0.0f,  len }, { -hw, ch,  len }, { -hw, ch,  hw }, { 1.0f, 0.0f, 0.0f }, curbColor);
    addQuad(curbVerts, curbInds, {  hw, 0.0f,  len }, {  hw, 0.0f,  hw }, {  hw, ch,  hw }, {  hw, ch,  len }, { -1.0f, 0.0f, 0.0f }, curbColor);

    // East approach curbs
    addQuad(curbVerts, curbInds, {  hw, 0.0f, -hw }, {  len, 0.0f, -hw }, {  len, ch, -hw }, {  hw, ch, -hw }, { 0.0f, 0.0f,  1.0f }, curbColor);
    addQuad(curbVerts, curbInds, {  len, 0.0f,  hw }, {  hw, 0.0f,  hw }, {  hw, ch,  hw }, {  len, ch,  hw }, { 0.0f, 0.0f, -1.0f }, curbColor);

    // West approach curbs
    addQuad(curbVerts, curbInds, { -len, 0.0f, -hw }, { -hw, 0.0f, -hw }, { -hw, ch, -hw }, { -len, ch, -hw }, { 0.0f, 0.0f,  1.0f }, curbColor);
    addQuad(curbVerts, curbInds, { -hw, 0.0f,  hw }, { -len, 0.0f,  hw }, { -len, ch,  hw }, { -hw, ch,  hw }, { 0.0f, 0.0f, -1.0f }, curbColor);

    m_sidewalkMesh = std::make_unique<gfx::Mesh>(sidewalkVerts, sidewalkInds);
    m_curbMesh = std::make_unique<gfx::Mesh>(curbVerts, curbInds);

    // Grass
    std::vector<gfx::Vertex> grassVerts;
    std::vector<uint32_t> grassInds;

    float outer = len + 10.0f;
    float gw = hw + sw;

    addQuad(grassVerts, grassInds, {  gw, ch * 0.8f, -outer }, {  outer, ch * 0.8f, -outer }, {  outer, ch * 0.8f, -gw }, {  gw, ch * 0.8f, -gw }, up, grassColor);
    addQuad(grassVerts, grassInds, { -outer, ch * 0.8f, -outer }, { -gw, ch * 0.8f, -outer }, { -gw, ch * 0.8f, -gw }, { -outer, ch * 0.8f, -gw }, up, grassColor);
    addQuad(grassVerts, grassInds, {  gw, ch * 0.8f,  gw }, {  outer, ch * 0.8f,  gw }, {  outer, ch * 0.8f,  outer }, {  gw, ch * 0.8f,  outer }, up, grassColor);
    addQuad(grassVerts, grassInds, { -outer, ch * 0.8f,  gw }, { -gw, ch * 0.8f,  gw }, { -gw, ch * 0.8f,  outer }, { -outer, ch * 0.8f,  outer }, up, grassColor);

    m_grassMesh = std::make_unique<gfx::Mesh>(grassVerts, grassInds);
}

void IntersectionGeometry::DrawRoads() const {
    if (m_roadSurfaceMesh) m_roadSurfaceMesh->Draw();
}

void IntersectionGeometry::DrawSidewalks() const {
    if (m_sidewalkMesh) m_sidewalkMesh->Draw();
}

void IntersectionGeometry::DrawCurbs() const {
    if (m_curbMesh) m_curbMesh->Draw();
}

void IntersectionGeometry::DrawGrass() const {
    if (m_grassMesh) m_grassMesh->Draw();
}

} // namespace SmartCity::city

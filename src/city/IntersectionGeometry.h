#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "gfx/Mesh.h"
#include "core/Config.h"

namespace SmartCity::city {

class IntersectionGeometry {
public:
    IntersectionGeometry(const core::IntersectionSettings& settings = core::IntersectionSettings());
    ~IntersectionGeometry() = default;

    void DrawRoads() const;
    void DrawSidewalks() const;
    void DrawCurbs() const;
    void DrawGrass() const;

    const core::IntersectionSettings& GetSettings() const { return m_settings; }

private:
    void GenerateGeometry();

    core::IntersectionSettings m_settings;

    std::unique_ptr<gfx::Mesh> m_roadSurfaceMesh;
    std::unique_ptr<gfx::Mesh> m_sidewalkMesh;
    std::unique_ptr<gfx::Mesh> m_curbMesh;
    std::unique_ptr<gfx::Mesh> m_grassMesh;
};

} // namespace SmartCity::city

#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "gfx/Mesh.h"
#include "core/Config.h"

namespace SmartCity::city {

class RoadMarkings {
public:
    RoadMarkings(const core::IntersectionSettings& settings);
    ~RoadMarkings() = default;

    void Draw() const;

private:
    void GenerateMarkings();

    core::IntersectionSettings m_settings;
    std::unique_ptr<gfx::Mesh> m_markingsMesh;
};

} // namespace SmartCity::city

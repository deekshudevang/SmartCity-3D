#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "gfx/Mesh.h"
#include "gfx/Light.h"
#include "core/Config.h"

namespace SmartCity::city {

class StreetFurniture {
public:
    StreetFurniture(const core::IntersectionSettings& settings);
    ~StreetFurniture() = default;

    void DrawStructures() const;
    const std::vector<gfx::PointLight>& GetStreetlightSources() const { return m_streetlightSources; }

private:
    void GenerateStructures();

    core::IntersectionSettings m_settings;

    std::unique_ptr<gfx::Mesh> m_metalPolesMesh;
    std::unique_ptr<gfx::Mesh> m_signalHousingsMesh;
    std::unique_ptr<gfx::Mesh> m_treesMesh;

    std::vector<gfx::PointLight> m_streetlightSources;
};

} // namespace SmartCity::city

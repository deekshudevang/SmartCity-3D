#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "gfx/Mesh.h"
#include "core/Config.h"

namespace SmartCity::city {

struct BuildingProps {
    glm::vec3 position;
    glm::vec3 size;
    glm::vec4 primaryColor;
    glm::vec4 windowColor;
    int numStories;
};

class BuildingGenerator {
public:
    BuildingGenerator(const core::IntersectionSettings& settings);
    ~BuildingGenerator() = default;

    void Draw() const;

private:
    void GenerateCityBlocks();

    core::IntersectionSettings m_settings;
    std::vector<BuildingProps> m_buildings;
    std::unique_ptr<gfx::Mesh> m_combinedBuildingsMesh;
};

} // namespace SmartCity::city

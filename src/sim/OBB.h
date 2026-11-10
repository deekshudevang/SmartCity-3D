#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace SmartCity::sim {

struct OrientedBoundingBox {
    glm::vec2 center{ 0.0f };       // (X, Z) center position in world space
    glm::vec2 halfExtents{ 0.0f };  // (halfLength, halfWidth)
    glm::vec2 axisX{ 1.0f, 0.0f };  // Unit vector along longitudinal forward direction
    glm::vec2 axisY{ 0.0f, 1.0f };  // Unit vector along lateral right direction

    OrientedBoundingBox() = default;
    OrientedBoundingBox(const glm::vec2& c, const glm::vec2& halfExt, float headingRad);

    std::vector<glm::vec2> GetCorners() const;

    // Evaluates intersection using Separating Axis Theorem (SAT)
    bool Intersects(const OrientedBoundingBox& other) const;

    // Checks if a 2D point lies inside the OBB
    bool Contains(const glm::vec2& point) const;
};

// Separating Axis Theorem test between two 2D OBBs
bool CheckSATOverlap(const OrientedBoundingBox& a, const OrientedBoundingBox& b);

} // namespace SmartCity::sim

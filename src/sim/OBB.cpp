#include "sim/OBB.h"
#include <cmath>
#include <algorithm>

namespace SmartCity::sim {

OrientedBoundingBox::OrientedBoundingBox(const glm::vec2& c, const glm::vec2& halfExt, float headingRad)
    : center(c)
    , halfExtents(halfExt)
{
    // In XZ plane (X = East, Z = South), forward heading theta:
    // theta = 0 points to +Z (South), theta = PI/2 points to +X (East)
    axisX = glm::vec2(std::sin(headingRad), std::cos(headingRad)); // Longitudinal forward
    axisY = glm::vec2(std::cos(headingRad), -std::sin(headingRad)); // Lateral right
}

std::vector<glm::vec2> OrientedBoundingBox::GetCorners() const {
    glm::vec2 extX = axisX * halfExtents.x;
    glm::vec2 extY = axisY * halfExtents.y;

    return {
        center + extX + extY,
        center - extX + extY,
        center - extX - extY,
        center + extX - extY
    };
}

bool OrientedBoundingBox::Contains(const glm::vec2& point) const {
    glm::vec2 d = point - center;
    float projX = std::abs(glm::dot(d, axisX));
    float projY = std::abs(glm::dot(d, axisY));
    return projX <= halfExtents.x && projY <= halfExtents.y;
}

bool OrientedBoundingBox::Intersects(const OrientedBoundingBox& other) const {
    return CheckSATOverlap(*this, other);
}

bool CheckSATOverlap(const OrientedBoundingBox& a, const OrientedBoundingBox& b) {
    glm::vec2 delta = b.center - a.center;

    // Test 4 candidate separating axes (2 from box A, 2 from box B)
    glm::vec2 axes[4] = { a.axisX, a.axisY, b.axisX, b.axisY };

    for (int i = 0; i < 4; ++i) {
        const glm::vec2& axis = axes[i];

        // Project box A onto axis
        float rA = a.halfExtents.x * std::abs(glm::dot(axis, a.axisX)) +
                   a.halfExtents.y * std::abs(glm::dot(axis, a.axisY));

        // Project box B onto axis
        float rB = b.halfExtents.x * std::abs(glm::dot(axis, b.axisX)) +
                   b.halfExtents.y * std::abs(glm::dot(axis, b.axisY));

        // Distance between centers projected on axis
        float dist = std::abs(glm::dot(delta, axis));

        // If centers distance exceeds sum of projected half extents, axis separates boxes
        if (dist > (rA + rB)) {
            return false;
        }
    }

    // No separating axis found -> boxes overlap
    return true;
}

} // namespace SmartCity::sim

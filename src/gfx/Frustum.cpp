#include "gfx/Frustum.h"
#include <cmath>
#include <algorithm>

namespace SmartCity::gfx {

AABB AABB::Transform(const glm::mat4& transform) const {
    glm::vec3 center = GetCenter();
    glm::vec3 extents = GetExtents();

    glm::vec3 newCenter = glm::vec3(transform * glm::vec4(center, 1.0f));

    // Arvo's method for tight bounding box of transformed AABB
    glm::mat3 absMat;
    for (int col = 0; col < 3; ++col) {
        for (int row = 0; row < 3; ++row) {
            absMat[col][row] = std::abs(transform[col][row]);
        }
    }

    glm::vec3 newExtents = absMat * extents;
    return AABB(newCenter - newExtents, newCenter + newExtents);
}

void Frustum::ExtractPlanes(const glm::mat4& m) {
    // Gribb-Hartmann method for row extraction from column-major matrix
    // Left plane: row 3 + row 0
    m_planes[0].normal.x = m[0][3] + m[0][0];
    m_planes[0].normal.y = m[1][3] + m[1][0];
    m_planes[0].normal.z = m[2][3] + m[2][0];
    m_planes[0].distance = m[3][3] + m[3][0];

    // Right plane: row 3 - row 0
    m_planes[1].normal.x = m[0][3] - m[0][0];
    m_planes[1].normal.y = m[1][3] - m[1][0];
    m_planes[1].normal.z = m[2][3] - m[2][0];
    m_planes[1].distance = m[3][3] - m[3][0];

    // Bottom plane: row 3 + row 1
    m_planes[2].normal.x = m[0][3] + m[0][1];
    m_planes[2].normal.y = m[1][3] + m[1][1];
    m_planes[2].normal.z = m[2][3] + m[2][1];
    m_planes[2].distance = m[3][3] + m[3][1];

    // Top plane: row 3 - row 1
    m_planes[3].normal.x = m[0][3] - m[0][1];
    m_planes[3].normal.y = m[1][3] - m[1][1];
    m_planes[3].normal.z = m[2][3] - m[2][1];
    m_planes[3].distance = m[3][3] - m[3][1];

    // Near plane: row 3 + row 2
    m_planes[4].normal.x = m[0][3] + m[0][2];
    m_planes[4].normal.y = m[1][3] + m[1][2];
    m_planes[4].normal.z = m[2][3] + m[2][2];
    m_planes[4].distance = m[3][3] + m[3][2];

    // Far plane: row 3 - row 2
    m_planes[5].normal.x = m[0][3] - m[0][2];
    m_planes[5].normal.y = m[1][3] - m[1][2];
    m_planes[5].normal.z = m[2][3] - m[2][2];
    m_planes[5].distance = m[3][3] - m[3][2];

    for (auto& plane : m_planes) {
        plane.Normalize();
    }
}

bool Frustum::IntersectsAABB(const AABB& aabb) const {
    glm::vec3 center = aabb.GetCenter();
    glm::vec3 extents = aabb.GetExtents();

    for (const auto& plane : m_planes) {
        // Project extents onto plane normal
        float r = extents.x * std::abs(plane.normal.x) +
                  extents.y * std::abs(plane.normal.y) +
                  extents.z * std::abs(plane.normal.z);

        float signedDistance = plane.GetSignedDistanceToPoint(center);
        if (signedDistance < -r) {
            // Entire box is on the negative (outside) side of this plane
            return false;
        }
    }
    return true;
}

bool Frustum::IntersectsSphere(const glm::vec3& center, float radius) const {
    for (const auto& plane : m_planes) {
        if (plane.GetSignedDistanceToPoint(center) < -radius) {
            return false;
        }
    }
    return true;
}

} // namespace SmartCity::gfx


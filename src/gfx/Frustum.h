#pragma once

#include <glm/glm.hpp>
#include <array>

namespace SmartCity::gfx {

struct AABB {
    glm::vec3 minPoint;
    glm::vec3 maxPoint;

    AABB()
        : minPoint(0.0f)
        , maxPoint(0.0f)
    {
    }

    AABB(const glm::vec3& minP, const glm::vec3& maxP)
        : minPoint(minP)
        , maxPoint(maxP)
    {
    }

    glm::vec3 GetCenter() const { return (minPoint + maxPoint) * 0.5f; }
    glm::vec3 GetExtents() const { return (maxPoint - minPoint) * 0.5f; }

    AABB Transform(const glm::mat4& transform) const;
};

struct Plane {
    glm::vec3 normal;
    float distance; // Distance from origin along normal: normal . P + distance = 0

    Plane() : normal(0.0f, 1.0f, 0.0f), distance(0.0f) {}
    Plane(const glm::vec3& norm, float dist) : normal(norm), distance(dist) {}

    void Normalize() {
        float len = glm::length(normal);
        if (len > 1e-6f) {
            normal /= len;
            distance /= len;
        }
    }

    float GetSignedDistanceToPoint(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }
};

enum class FrustumPlaneIndex {
    Left = 0,
    Right = 1,
    Bottom = 2,
    Top = 3,
    Near = 4,
    Far = 5
};

class Frustum {
public:
    Frustum() = default;

    // Extracts 6 normalized frustum planes from a View-Projection matrix using the Gribb-Hartmann method
    void ExtractPlanes(const glm::mat4& viewProjection);

    bool IntersectsAABB(const AABB& aabb) const;
    bool IntersectsSphere(const glm::vec3& center, float radius) const;

    const std::array<Plane, 6>& GetPlanes() const { return m_planes; }
    const Plane& GetPlane(FrustumPlaneIndex index) const { return m_planes[static_cast<size_t>(index)]; }

private:
    std::array<Plane, 6> m_planes;
};

} // namespace SmartCity::gfx


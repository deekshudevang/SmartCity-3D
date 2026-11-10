#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "sim/OBB.h"

namespace SmartCity::algos {

struct Ray {
    glm::vec3 origin{ 0.0f };
    glm::vec3 direction{ 0.0f, 0.0f, -1.0f };

    glm::vec3 At(float t) const { return origin + direction * t; }
};

struct RayHit {
    bool hit{ false };
    float distance{ 0.0f };
    glm::vec3 point{ 0.0f };
    glm::vec3 normal{ 0.0f };
    int objectId{ -1 };
    std::string objectType;
    std::string description;
};

class RayCaster {
public:
    // Unprojects 2D screen coordinates (pixels) into a 3D world-space ray
    // Correctly handles both Perspective and Orthographic projection matrices
    static Ray ScreenPointToRay(float screenX, float screenY,
                                float viewportWidth, float viewportHeight,
                                const glm::mat4& viewMatrix,
                                const glm::mat4& projectionMatrix);

    // Ray-AABB intersection test using Kay-Kajiya slab method
    static bool IntersectRayAABB(const Ray& ray, const glm::vec3& boxMin, const glm::vec3& boxMax,
                                 float& outTMin, float& outTMax, glm::vec3* outNormal = nullptr);

    // Ray-OBB intersection test (local space transform)
    static bool IntersectRayOBB(const Ray& ray,
                                const glm::vec3& boxCenter,
                                const glm::vec3& boxHalfExtents,
                                const glm::mat3& boxAxes,
                                float& outDistance, glm::vec3* outNormal = nullptr);

    // Ray-OBB intersection using simulation 2D/3D OrientedBoundingBox
    static bool IntersectRayOBB(const Ray& ray, const sim::OrientedBoundingBox& obb,
                                float height, float groundY,
                                float& outDistance);
};

} // namespace SmartCity::algos


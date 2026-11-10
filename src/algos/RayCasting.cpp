#include "algos/RayCasting.h"
#include <algorithm>
#include <limits>
#include <cmath>

namespace SmartCity::algos {

Ray RayCaster::ScreenPointToRay(float screenX, float screenY,
                                float viewportWidth, float viewportHeight,
                                const glm::mat4& viewMatrix,
                                const glm::mat4& projectionMatrix) {
    Ray ray;
    if (viewportWidth <= 0.0f || viewportHeight <= 0.0f) {
        return ray;
    }

    // Convert screen coordinates to Normalized Device Coordinates (NDC) [-1, 1]
    float xNdc = (2.0f * screenX) / viewportWidth - 1.0f;
    float yNdc = 1.0f - (2.0f * screenY) / viewportHeight; // Invert Y (top-down screen to bottom-up NDC)

    glm::mat4 invVP = glm::inverse(projectionMatrix * viewMatrix);

    glm::vec4 nearNdc(xNdc, yNdc, -1.0f, 1.0f);
    glm::vec4 farNdc(xNdc, yNdc, 1.0f, 1.0f);

    glm::vec4 nearWorld = invVP * nearNdc;
    glm::vec4 farWorld = invVP * farNdc;

    if (std::abs(nearWorld.w) > 1e-7f) nearWorld /= nearWorld.w;
    if (std::abs(farWorld.w) > 1e-7f) farWorld /= farWorld.w;

    ray.origin = glm::vec3(nearWorld);
    glm::vec3 dir = glm::vec3(farWorld) - glm::vec3(nearWorld);
    float len = glm::length(dir);
    ray.direction = (len > 1e-6f) ? (dir / len) : glm::vec3(0.0f, 0.0f, -1.0f);

    return ray;
}

bool RayCaster::IntersectRayAABB(const Ray& ray, const glm::vec3& boxMin, const glm::vec3& boxMax,
                                 float& outTMin, float& outTMax, glm::vec3* outNormal) {
    float tMin = -std::numeric_limits<float>::infinity();
    float tMax = std::numeric_limits<float>::infinity();
    glm::vec3 normal(0.0f);

    for (int i = 0; i < 3; ++i) {
        if (std::abs(ray.direction[i]) < 1e-8f) {
            // Ray is parallel to the slab
            if (ray.origin[i] < boxMin[i] || ray.origin[i] > boxMax[i]) {
                return false;
            }
        } else {
            float invD = 1.0f / ray.direction[i];
            float t1 = (boxMin[i] - ray.origin[i]) * invD;
            float t2 = (boxMax[i] - ray.origin[i]) * invD;
            glm::vec3 n1(0.0f), n2(0.0f);
            n1[i] = -1.0f;
            n2[i] = 1.0f;

            if (t1 > t2) {
                std::swap(t1, t2);
                std::swap(n1, n2);
            }

            if (t1 > tMin) {
                tMin = t1;
                normal = n1;
            }
            tMax = std::min(tMax, t2);

            if (tMin > tMax) {
                return false;
            }
        }
    }

    // If tMax < 0, the entire box is behind the ray origin
    if (tMax < 0.0f) {
        return false;
    }

    outTMin = tMin;
    outTMax = tMax;
    if (outNormal) {
        *outNormal = normal;
    }

    return true;
}

bool RayCaster::IntersectRayOBB(const Ray& ray,
                                const glm::vec3& boxCenter,
                                const glm::vec3& boxHalfExtents,
                                const glm::mat3& boxAxes,
                                float& outDistance, glm::vec3* outNormal) {
    // Transform ray into OBB's local coordinate system
    // Local box is an AABB centered at (0, 0, 0) with extents [-boxHalfExtents, +boxHalfExtents]
    glm::vec3 relOrigin = ray.origin - boxCenter;

    // Project origin and direction onto local axes (transpose of rotation matrix)
    Ray localRay;
    localRay.origin = glm::vec3(
        glm::dot(relOrigin, boxAxes[0]),
        glm::dot(relOrigin, boxAxes[1]),
        glm::dot(relOrigin, boxAxes[2])
    );
    localRay.direction = glm::vec3(
        glm::dot(ray.direction, boxAxes[0]),
        glm::dot(ray.direction, boxAxes[1]),
        glm::dot(ray.direction, boxAxes[2])
    );

    float tMin = 0.0f, tMax = 0.0f;
    glm::vec3 localNormal(0.0f);
    bool hit = IntersectRayAABB(localRay, -boxHalfExtents, boxHalfExtents, tMin, tMax, &localNormal);

    if (!hit) {
        return false;
    }

    // If ray starts inside the box (tMin < 0), the hit distance forward along ray is 0.0
    outDistance = (tMin >= 0.0f) ? tMin : 0.0f;

    if (outNormal) {
        // Transform normal back to world coordinates
        *outNormal = boxAxes[0] * localNormal.x +
                     boxAxes[1] * localNormal.y +
                     boxAxes[2] * localNormal.z;
    }

    return true;
}

bool RayCaster::IntersectRayOBB(const Ray& ray, const sim::OrientedBoundingBox& obb,
                                float height, float groundY,
                                float& outDistance) {
    glm::vec3 center(obb.center.x, groundY + height * 0.5f, obb.center.y);
    glm::vec3 halfExtents(obb.halfExtents.y, height * 0.5f, obb.halfExtents.x); // x=halfWidth, z=halfLength

    glm::mat3 axes(1.0f);
    axes[0] = glm::vec3(obb.axisY.x, 0.0f, obb.axisY.y); // lateral (local X)
    axes[1] = glm::vec3(0.0f, 1.0f, 0.0f);               // local Y (Up)
    axes[2] = glm::vec3(obb.axisX.x, 0.0f, obb.axisX.y); // longitudinal (local Z)

    return IntersectRayOBB(ray, center, halfExtents, axes, outDistance, nullptr);
}

} // namespace SmartCity::algos

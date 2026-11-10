#include "gfx/Frustum.h"
#include "core/Logger.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

using namespace SmartCity::gfx;
using namespace SmartCity::core;

int main() {
    Logger::Init("logs/test_frustum.log");
    Logger::Info("Running unit test_frustum...");

    // Standard camera looking down -Z from origin (0, 0, 0)
    // FOV = 90 degrees, aspect = 1.0, near = 1.0, far = 100.0
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 1.0f, 100.0f);
    glm::mat4 viewProj = proj * view;

    Frustum frustum;
    frustum.ExtractPlanes(viewProj);

    // 1. Known Inside Case: Object directly in front of camera
    AABB boxInside(glm::vec3(-2.0f, -2.0f, -20.0f), glm::vec3(2.0f, 2.0f, -15.0f));
    assert(frustum.IntersectsAABB(boxInside) == true);
    Logger::Info("[PASS] AABB inside frustum test passed.");

    // 2. Known Outside Behind Case: Object behind the camera (+Z)
    AABB boxBehind(glm::vec3(-2.0f, -2.0f, 5.0f), glm::vec3(2.0f, 2.0f, 10.0f));
    assert(frustum.IntersectsAABB(boxBehind) == false);
    Logger::Info("[PASS] AABB behind camera culled correctly.");

    // 3. Known Outside Left Case: Object far off to the left (-X)
    AABB boxLeft(glm::vec3(-150.0f, -2.0f, -20.0f), glm::vec3(-140.0f, 2.0f, -15.0f));
    assert(frustum.IntersectsAABB(boxLeft) == false);
    Logger::Info("[PASS] AABB outside left plane culled correctly.");

    // 4. Known Outside Beyond Far Plane: Object beyond 100m
    AABB boxBeyondFar(glm::vec3(-2.0f, -2.0f, -150.0f), glm::vec3(2.0f, 2.0f, -120.0f));
    assert(frustum.IntersectsAABB(boxBeyondFar) == false);
    Logger::Info("[PASS] AABB beyond far plane culled correctly.");

    // 5. Known Straddling Case: Object crossing the near plane (near = 1.0 -> Z = -1.0)
    AABB boxStraddleNear(glm::vec3(-1.0f, -1.0f, -2.0f), glm::vec3(1.0f, 1.0f, -0.5f));
    assert(frustum.IntersectsAABB(boxStraddleNear) == true);
    Logger::Info("[PASS] AABB straddling near plane intersects correctly.");

    // 6. Bounding Sphere Inside / Outside Tests
    assert(frustum.IntersectsSphere(glm::vec3(0.0f, 0.0f, -30.0f), 5.0f) == true);
    assert(frustum.IntersectsSphere(glm::vec3(0.0f, 0.0f, 20.0f), 2.0f) == false);
    Logger::Info("[PASS] Bounding sphere inside and outside tests passed.");

    Logger::Info("test_frustum PASSED successfully.");
    Logger::Shutdown();
    return 0;
}


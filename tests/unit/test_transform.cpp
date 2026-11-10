#include "scene/Transform.h"
#include "core/Logger.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace SmartCity::scene;
using namespace SmartCity::core;

int main() {
    Logger::Init("logs/test_transform.log");
    Logger::Info("Running unit test_transform...");

    // 1. Test Composition Order: M = T * R * S
    Transform transform;
    transform.SetPosition(glm::vec3(10.0f, 20.0f, 30.0f));
    transform.SetScale(glm::vec3(2.0f, 3.0f, 4.0f));
    // 90 degrees around Y axis: X -> -Z, Z -> X
    transform.Rotate(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 m = transform.GetLocalMatrix();

    // Point (0, 0, 0) -> Translation only
    glm::vec4 p0 = m * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    assert(std::abs(p0.x - 10.0f) < 1e-4f);
    assert(std::abs(p0.y - 20.0f) < 1e-4f);
    assert(std::abs(p0.z - 30.0f) < 1e-4f);
    Logger::Info("[PASS] Transform translation component verified.");

    // Vector (1, 0, 0): Scaled by 2 along X, rotated 90 deg around Y -> (0, 0, -2) + (10, 20, 30) = (10, 20, 28)
    glm::vec4 p1 = m * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    assert(std::abs(p1.x - 10.0f) < 1e-4f);
    assert(std::abs(p1.y - 20.0f) < 1e-4f);
    assert(std::abs(p1.z - 28.0f) < 1e-4f);
    Logger::Info("[PASS] Scale, rotation, and translation composition order verified.");

    // 2. Test Hierarchy (Parent - Child)
    Transform parent;
    parent.SetPosition(glm::vec3(5.0f, 0.0f, 0.0f));

    Transform child;
    child.SetPosition(glm::vec3(0.0f, 12.0f, 0.0f));
    child.SetParent(&parent);

    glm::mat4 childWorld = child.GetWorldMatrix();
    glm::vec4 childPosWorld = childWorld * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    assert(std::abs(childPosWorld.x - 5.0f) < 1e-4f);
    assert(std::abs(childPosWorld.y - 12.0f) < 1e-4f);
    assert(std::abs(childPosWorld.z - 0.0f) < 1e-4f);
    Logger::Info("[PASS] Transform parent-child hierarchical composition verified.");

    // 3. Test Dirty Flag Optimization
    assert(child.IsDirty() == false);
    child.Translate(glm::vec3(1.0f, 0.0f, 0.0f));
    assert(child.IsDirty() == true);
    // Calling GetLocalMatrix() clears dirty flag
    (void)child.GetLocalMatrix();
    assert(child.IsDirty() == false);
    Logger::Info("[PASS] Dirty flag caching behavior verified.");

    // 4. Test Normal Matrix calculation
    glm::mat3 normMat = transform.GetNormalMatrix();
    // Normal matrix must preserve orthogonality: det != 0
    assert(std::abs(glm::determinant(normMat)) > 1e-4f);
    Logger::Info("[PASS] CPU normal matrix calculation verified.");

    Logger::Info("test_transform PASSED successfully.");
    Logger::Shutdown();
    return 0;
}


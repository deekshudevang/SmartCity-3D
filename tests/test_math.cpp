#include <iostream>
#include <cassert>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/Logger.h"

// Helper function for cubic Bézier point evaluation
glm::vec3 EvaluateBezier(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    glm::vec3 p = uuu * p0; // (1-t)^3 * P0
    p += 3.0f * uu * t * p1; // 3*(1-t)^2 * t * P1
    p += 3.0f * u * tt * p2; // 3*(1-t) * t^2 * P2
    p += ttt * p3;          // t^3 * P3
    return p;
}

// Derivative of cubic Bézier curve for tangent/heading
glm::vec3 EvaluateBezierDerivative(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
    float u = 1.0f - t;
    glm::vec3 d = 3.0f * u * u * (p1 - p0) +
                  6.0f * u * t * (p2 - p1) +
                  3.0f * t * t * (p3 - p2);
    return d;
}

int main() {
    SmartCity::Logger::Info("Running test_math...");

    // Test 1: GLM Vector operations
    glm::vec3 a(1.0f, 2.0f, 3.0f);
    glm::vec3 b(4.0f, 5.0f, 6.0f);
    glm::vec3 sum = a + b;
    assert(sum.x == 5.0f && sum.y == 7.0f && sum.z == 9.0f);
    SmartCity::Logger::Info("[PASS] Vector addition");

    // Test 2: Matrix transformations
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(10.0f, 0.0f, -5.0f));
    glm::vec4 point(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 transformed = model * point;
    assert(std::abs(transformed.x - 10.0f) < 1e-5f);
    assert(std::abs(transformed.y - 0.0f) < 1e-5f);
    assert(std::abs(transformed.z - (-5.0f)) < 1e-5f);
    SmartCity::Logger::Info("[PASS] Matrix translation");

    // Test 3: Cubic Bézier Curve endpoints and midpoint
    glm::vec3 p0(0.0f, 0.0f, 0.0f);
    glm::vec3 p1(0.0f, 0.0f, 10.0f);
    glm::vec3 p2(10.0f, 0.0f, 20.0f);
    glm::vec3 p3(20.0f, 0.0f, 20.0f);

    glm::vec3 start = EvaluateBezier(p0, p1, p2, p3, 0.0f);
    glm::vec3 end = EvaluateBezier(p0, p1, p2, p3, 1.0f);
    glm::vec3 mid = EvaluateBezier(p0, p1, p2, p3, 0.5f);

    assert(glm::distance(start, p0) < 1e-5f);
    assert(glm::distance(end, p3) < 1e-5f);
    // Midpoint check: (1/8)*0 + (3/8)*10z + (3/8)*(10x+20z) + (1/8)*(20x+20z)
    // x = 3/8*10 + 1/8*20 = 30/8 + 20/8 = 50/8 = 6.25
    // z = 3/8*10 + 3/8*20 + 1/8*20 = 30/8 + 60/8 + 20/8 = 110/8 = 13.75
    assert(std::abs(mid.x - 6.25f) < 1e-4f);
    assert(std::abs(mid.z - 13.75f) < 1e-4f);
    SmartCity::Logger::Info("[PASS] Cubic Bézier geometry interpolation");

    // Test 4: Bézier Tangent normalization
    glm::vec3 tangentStart = glm::normalize(EvaluateBezierDerivative(p0, p1, p2, p3, 0.0f));
    assert(std::abs(tangentStart.x - 0.0f) < 1e-4f);
    assert(std::abs(tangentStart.z - 1.0f) < 1e-4f); // tangent at start points along +Z
    SmartCity::Logger::Info("[PASS] Bézier initial heading vector");

    SmartCity::Logger::Info("All test_math tests PASSED successfully!");
    return 0;
}

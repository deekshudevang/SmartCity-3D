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

    glm::vec3 p = uuu * p0;
    p += 3.0f * uu * t * p1;
    p += 3.0f * u * tt * p2;
    p += ttt * p3;
    return p;
}

glm::vec3 EvaluateBezierDerivative(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
    float u = 1.0f - t;
    glm::vec3 d = 3.0f * u * u * (p1 - p0) +
                  6.0f * u * t * (p2 - p1) +
                  3.0f * t * t * (p3 - p2);
    return d;
}

int main() {
    SmartCity::core::Logger::Init("logs/test_math.log");
    SmartCity::core::Logger::Info("Running unit test_math...");

    glm::vec3 a(1.0f, 2.0f, 3.0f);
    glm::vec3 b(4.0f, 5.0f, 6.0f);
    glm::vec3 sum = a + b;
    assert(sum.x == 5.0f && sum.y == 7.0f && sum.z == 9.0f);
    SmartCity::core::Logger::Info("[PASS] Vector addition");

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(10.0f, 0.0f, -5.0f));
    glm::vec4 point(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 transformed = model * point;
    assert(std::abs(transformed.x - 10.0f) < 1e-5f);
    assert(std::abs(transformed.y - 0.0f) < 1e-5f);
    assert(std::abs(transformed.z - (-5.0f)) < 1e-5f);
    SmartCity::core::Logger::Info("[PASS] Matrix translation");

    // Coordinates: North = -Z, East = +X
    glm::vec3 p0(0.0f, 0.0f, 0.0f);
    glm::vec3 p1(0.0f, 0.0f, 10.0f);
    glm::vec3 p2(10.0f, 0.0f, 20.0f);
    glm::vec3 p3(20.0f, 0.0f, 20.0f);

    glm::vec3 start = EvaluateBezier(p0, p1, p2, p3, 0.0f);
    glm::vec3 end = EvaluateBezier(p0, p1, p2, p3, 1.0f);
    glm::vec3 mid = EvaluateBezier(p0, p1, p2, p3, 0.5f);

    assert(glm::distance(start, p0) < 1e-5f);
    assert(glm::distance(end, p3) < 1e-5f);
    assert(std::abs(mid.x - 6.25f) < 1e-4f);
    assert(std::abs(mid.z - 13.75f) < 1e-4f);
    SmartCity::core::Logger::Info("[PASS] Cubic Bézier geometry interpolation");

    glm::vec3 tangentStart = glm::normalize(EvaluateBezierDerivative(p0, p1, p2, p3, 0.0f));
    assert(std::abs(tangentStart.x - 0.0f) < 1e-4f);
    assert(std::abs(tangentStart.z - 1.0f) < 1e-4f);
    SmartCity::core::Logger::Info("[PASS] Bézier initial heading vector");

    SmartCity::core::Logger::Info("All test_math unit tests PASSED successfully!");
    SmartCity::core::Logger::Shutdown();
    return 0;
}

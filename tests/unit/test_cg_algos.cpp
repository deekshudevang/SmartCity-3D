#include <cassert>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "algos/LineDrawing.h"
#include "algos/BezierCurve.h"
#include "algos/RayCasting.h"

using namespace SmartCity;

void Test_LineRasterizationAllOctants() {
    std::cout << "[Test 1] Line Rasterization (DDA vs Bresenham in all 8 octants)..." << std::endl;

    // Test cases covering:
    // Octant 0 (shallow +x, +y), Octant 1 (steep +x, +y), Octant 2 (steep -x, +y), Octant 3 (shallow -x, +y)
    // Octant 4 (shallow -x, -y), Octant 5 (steep -x, -y), Octant 6 (steep +x, -y), Octant 7 (shallow +x, -y)
    // Horizontal, Vertical, Degenerate (P0 == P1)
    struct LineCase {
        int x0, y0, x1, y1;
        int expectedOctant;
    };

    std::vector<LineCase> testCases = {
        { 0, 0, 8, 4, 0 },    // Octant 0
        { 0, 0, 4, 8, 1 },    // Octant 1
        { 0, 0, -4, 8, 2 },   // Octant 2
        { 0, 0, -8, 4, 3 },   // Octant 3
        { 0, 0, -8, -4, 4 },  // Octant 4
        { 0, 0, -4, -8, 5 },  // Octant 5
        { 0, 0, 4, -8, 6 },   // Octant 6
        { 0, 0, 8, -4, 7 },   // Octant 7
        { 2, 5, 12, 5, 0 },   // Horizontal
        { 3, 1, 3, 15, 1 },   // Vertical
        { 7, 7, 7, 7, 0 }     // Degenerate
    };

    for (const auto& tc : testCases) {
        auto ddaRes = algos::LineRasterizer::RasterizeDDA(tc.x0, tc.y0, tc.x1, tc.y1);
        auto bresRes = algos::LineRasterizer::RasterizeBresenham(tc.x0, tc.y0, tc.x1, tc.y1);

        assert(!ddaRes.pixels.empty() && !bresRes.pixels.empty());

        // Check exact endpoints
        assert(bresRes.pixels.front().x == tc.x0 && bresRes.pixels.front().y == tc.y0);
        assert(bresRes.pixels.back().x == tc.x1 && bresRes.pixels.back().y == tc.y1);

        assert(ddaRes.pixels.front().x == tc.x0 && ddaRes.pixels.front().y == tc.y0);
        assert(ddaRes.pixels.back().x == tc.x1 && ddaRes.pixels.back().y == tc.y1);

        // Verify that DDA and Bresenham rasterizations are visually consistent (distance <= 1 pixel)
        size_t count = std::min(ddaRes.pixels.size(), bresRes.pixels.size());
        for (size_t i = 0; i < count; ++i) {
            int dx = std::abs(ddaRes.pixels[i].x - bresRes.pixels[i].x);
            int dy = std::abs(ddaRes.pixels[i].y - bresRes.pixels[i].y);
            assert(dx <= 1 && dy <= 1 && "DDA and Bresenham pixels drifted apart!");
        }
    }

    std::cout << "  PASS: DDA and Bresenham rasterizers verified across all octants and degenerate cases." << std::endl;
}

void Test_BezierCurveEvaluations() {
    std::cout << "[Test 2] Cubic Bézier Evaluations (De Casteljau vs Polynomial)..." << std::endl;

    glm::vec3 p0(0.0f, 0.0f, 0.0f);
    glm::vec3 p1(10.0f, 0.0f, 0.0f);
    glm::vec3 p2(20.0f, 0.0f, 10.0f);
    glm::vec3 p3(20.0f, 0.0f, 20.0f);

    // 1. Verify that De Casteljau and Bernstein Polynomial formulations agree within machine epsilon
    bool match = algos::BezierCurveEvaluator::VerifyFormulationsAgreement(p0, p1, p2, p3, 200, 1e-5f);
    assert(match && "De Casteljau and Polynomial formulations must be mathematically equivalent!");

    // 2. Analytical tangent vs numerical central difference
    float testT = 0.45f;
    glm::vec3 analyticalTangent = algos::BezierCurveEvaluator::EvaluateDerivative(p0, p1, p2, p3, testT);

    float eps = 1e-3f;
    glm::vec3 pPlus = algos::BezierCurveEvaluator::EvaluatePolynomial(p0, p1, p2, p3, testT + eps);
    glm::vec3 pMinus = algos::BezierCurveEvaluator::EvaluatePolynomial(p0, p1, p2, p3, testT - eps);
    glm::vec3 numericalTangent = (pPlus - pMinus) / (2.0f * eps);

    float tangentDiff = glm::distance(analyticalTangent, numericalTangent);
    std::cout << "  Analytical tangent: (" << analyticalTangent.x << ", " << analyticalTangent.y << ", " << analyticalTangent.z << ")\n";
    std::cout << "  Numerical tangent:  (" << numericalTangent.x << ", " << numericalTangent.y << ", " << numericalTangent.z << ")\n";
    std::cout << "  Tangent diff: " << tangentDiff << std::endl;
    assert(tangentDiff < 1e-2f && "Analytical tangent must match numerical derivative!");

    // 3. Monotonic Arc-Length Table
    auto lut = algos::BezierCurveEvaluator::BuildArcLengthLUT(p0, p1, p2, p3, 100);
    assert(lut.size() == 101);
    assert(lut.front().arcLength == 0.0f);
    assert(lut.back().arcLength > 20.0f);

    for (size_t i = 1; i < lut.size(); ++i) {
        assert(lut[i].arcLength >= lut[i - 1].arcLength && "Arc length must be monotonically non-decreasing!");
    }

    std::cout << "  PASS: Bézier De Casteljau, Polynomial, Tangent, and Arc-Length LUT fully verified." << std::endl;
}

void Test_RayCastingIntersections() {
    std::cout << "[Test 3] Ray Casting (AABB, OBB & Camera Unprojection)..." << std::endl;

    // 1. Ray-AABB Direct Hit
    algos::Ray ray1;
    ray1.origin = glm::vec3(0.0f, 0.0f, 5.0f);
    ray1.direction = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::vec3 boxMin(-1.0f, -1.0f, -1.0f);
    glm::vec3 boxMax(1.0f, 1.0f, 1.0f);

    float tMin = 0.0f, tMax = 0.0f;
    glm::vec3 hitNormal(0.0f);
    bool hit1 = algos::RayCaster::IntersectRayAABB(ray1, boxMin, boxMax, tMin, tMax, &hitNormal);
    assert(hit1);
    assert(std::abs(tMin - 4.0f) < 1e-4f); // Origin at z=5, front face at z=1 -> dist = 4.0
    assert(std::abs(tMax - 6.0f) < 1e-4f); // Back face at z=-1 -> dist = 6.0
    assert(hitNormal == glm::vec3(0.0f, 0.0f, 1.0f));

    // 2. Ray-AABB Clear Miss
    algos::Ray rayMiss;
    rayMiss.origin = glm::vec3(5.0f, 5.0f, 5.0f);
    rayMiss.direction = glm::vec3(0.0f, 0.0f, -1.0f);
    bool hitMiss = algos::RayCaster::IntersectRayAABB(rayMiss, boxMin, boxMax, tMin, tMax);
    assert(!hitMiss && "Ray must miss the box!");

    // 3. Ray Starting Inside Box
    algos::Ray rayInside;
    rayInside.origin = glm::vec3(0.0f, 0.0f, 0.0f);
    rayInside.direction = glm::vec3(1.0f, 0.0f, 0.0f);
    bool hitInside = algos::RayCaster::IntersectRayAABB(rayInside, boxMin, boxMax, tMin, tMax);
    assert(hitInside);
    assert(tMin < 0.0f && tMax > 0.0f); // Starts inside

    // 4. Ray-OBB Hit on Rotated Box
    glm::vec3 obbCenter(10.0f, 0.0f, 0.0f);
    glm::vec3 halfExtents(2.0f, 1.0f, 1.0f);
    // Rotate 90 degrees around Y axis
    glm::mat3 rotY = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

    algos::Ray rayOBB;
    rayOBB.origin = glm::vec3(10.0f, 0.0f, 5.0f);
    rayOBB.direction = glm::vec3(0.0f, 0.0f, -1.0f);

    float obbDist = 0.0f;
    glm::vec3 obbNormal(0.0f);
    bool hitOBB = algos::RayCaster::IntersectRayOBB(rayOBB, obbCenter, halfExtents, rotY, obbDist, &obbNormal);
    assert(hitOBB);
    assert(obbDist > 0.0f);

    // 5. ScreenPointToRay Unprojection (Perspective & Orthographic)
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 10.0f, 20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projPersp = glm::perspective(glm::radians(60.0f), 1920.0f / 1080.0f, 0.1f, 1000.0f);
    glm::mat4 projOrtho = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, 0.1f, 1000.0f);

    // Center screen ray
    algos::Ray centerRayPersp = algos::RayCaster::ScreenPointToRay(960.0f, 540.0f, 1920.0f, 1080.0f, view, projPersp);
    glm::vec3 expectedDir = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - glm::vec3(0.0f, 10.0f, 20.0f));
    assert(glm::distance(centerRayPersp.direction, expectedDir) < 1e-3f);

    algos::Ray centerRayOrtho = algos::RayCaster::ScreenPointToRay(960.0f, 540.0f, 1920.0f, 1080.0f, view, projOrtho);
    assert(glm::distance(centerRayOrtho.direction, expectedDir) < 1e-3f);

    std::cout << "  PASS: Ray-AABB, Ray-OBB, and Screen-to-Ray unprojection fully verified." << std::endl;
}

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << "SmartCity 3D — Computer Graphics Algorithms Test Suite" << std::endl;
    std::cout << "=====================================================" << std::endl;

    Test_LineRasterizationAllOctants();
    Test_BezierCurveEvaluations();
    Test_RayCastingIntersections();

    std::cout << "=====================================================" << std::endl;
    std::cout << "ALL COMPUTER GRAPHICS ALGORITHMS TESTS PASSED (3/3)" << std::endl;
    std::cout << "=====================================================" << std::endl;
    return 0;
}

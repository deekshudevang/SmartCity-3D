#include <cassert>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "sim/OBB.h"

using namespace SmartCity::sim;

void TestAxisAlignedSeparation() {
    // Sedan dimensions: half-length = 2.25, half-width = 0.9
    OrientedBoundingBox boxA(glm::vec2(0.0f, 0.0f), glm::vec2(2.25f, 0.9f), 0.0f);
    // Box B separated ahead along Z by 10m
    OrientedBoundingBox boxB(glm::vec2(0.0f, 10.0f), glm::vec2(2.25f, 0.9f), 0.0f);
    assert(!boxA.Intersects(boxB));
    assert(!boxB.Intersects(boxA));

    // Box C separated sideways along X by 5m
    OrientedBoundingBox boxC(glm::vec2(5.0f, 0.0f), glm::vec2(2.25f, 0.9f), 0.0f);
    assert(!boxA.Intersects(boxC));
    assert(!boxC.Intersects(boxA));

    std::cout << "[PASS] TestAxisAlignedSeparation" << std::endl;
}

void TestAxisAlignedOverlap() {
    // Box A and Box B overlapping along Z
    OrientedBoundingBox boxA(glm::vec2(0.0f, 0.0f), glm::vec2(2.25f, 0.9f), 0.0f);
    OrientedBoundingBox boxB(glm::vec2(0.0f, 3.0f), glm::vec2(2.25f, 0.9f), 0.0f);
    // Center distance is 3.0, sum of half-lengths is 2.25 + 2.25 = 4.5 > 3.0 -> Overlap!
    assert(boxA.Intersects(boxB));
    assert(boxB.Intersects(boxA));

    // Identical boxes
    assert(boxA.Intersects(boxA));

    std::cout << "[PASS] TestAxisAlignedOverlap" << std::endl;
}

void TestRotatedOverlap() {
    // Box A at origin heading 0 (+Z)
    OrientedBoundingBox boxA(glm::vec2(0.0f, 0.0f), glm::vec2(2.25f, 0.9f), 0.0f);

    // Box B heading 90 degrees (+X) intersecting Box A at center
    float piOver2 = glm::half_pi<float>();
    OrientedBoundingBox boxB(glm::vec2(0.5f, 0.5f), glm::vec2(2.25f, 0.9f), piOver2);
    assert(boxA.Intersects(boxB));
    assert(boxB.Intersects(boxA));

    // Box C rotated 45 degrees
    float piOver4 = glm::quarter_pi<float>();
    OrientedBoundingBox boxC(glm::vec2(1.0f, 1.0f), glm::vec2(2.25f, 0.9f), piOver4);
    assert(boxA.Intersects(boxC));
    assert(boxC.Intersects(boxA));

    std::cout << "[PASS] TestRotatedOverlap" << std::endl;
}

void TestRotatedSeparationAABBFalsePositive() {
    float piOver4 = glm::quarter_pi<float>();
    // Box A rotated 45 degrees at origin (half-length 2.0, half-width 0.5)
    // AABB of Box A spans [-1.77, 1.77] in X and [-1.77, 1.77] in Z
    OrientedBoundingBox boxA(glm::vec2(0.0f, 0.0f), glm::vec2(2.0f, 0.5f), piOver4);

    // Box B axis-aligned (heading 0) centered at (2.0, 3.5) with (half-length 2.0, half-width 0.5)
    // AABB of Box B spans [1.5, 2.5] in X and [1.5, 5.5] in Z
    // The AABBs overlap in both X ([1.5, 1.77]) and Z ([1.5, 1.77])!
    OrientedBoundingBox boxB(glm::vec2(2.0f, 3.5f), glm::vec2(2.0f, 0.5f), 0.0f);

    // With true SAT, Box A's longitudinal axis separates them (proj dist = 3.89 > rA + rB = 3.77)
    assert(!boxA.Intersects(boxB));
    assert(!boxB.Intersects(boxA));

    std::cout << "[PASS] TestRotatedSeparationAABBFalsePositive" << std::endl;
}

void TestForcedCollisionDetection() {
    // Synthetic crash: Ambulance (5.5x2.0m) vs Bus (12.0x2.5m) crossing in intersection
    OrientedBoundingBox ambulance(glm::vec2(0.0f, 2.0f), glm::vec2(2.75f, 1.0f), 0.0f);
    OrientedBoundingBox bus(glm::vec2(1.5f, 2.0f), glm::vec2(6.0f, 1.25f), glm::half_pi<float>());

    // Distance in X is 1.5m, ambulance width is 2.0m (half-ext 1.0), bus length is 12m (half-ext 6.0)
    // Distance in Z is 0.0m. Overlap must be detected!
    assert(CheckSATOverlap(ambulance, bus));
    assert(CheckSATOverlap(bus, ambulance));

    std::cout << "[PASS] TestForcedCollisionDetection" << std::endl;
}

void TestPointContainment() {
    OrientedBoundingBox box(glm::vec2(5.0f, 5.0f), glm::vec2(2.0f, 1.0f), 0.0f);

    // Center point
    assert(box.Contains(glm::vec2(5.0f, 5.0f)));
    // Interior points
    assert(box.Contains(glm::vec2(5.5f, 6.0f)));
    assert(box.Contains(glm::vec2(4.5f, 4.0f)));
    // Exterior points
    assert(!box.Contains(glm::vec2(5.0f, 7.5f)));
    assert(!box.Contains(glm::vec2(7.5f, 5.0f)));
    assert(!box.Contains(glm::vec2(0.0f, 0.0f)));

    std::cout << "[PASS] TestPointContainment" << std::endl;
}

int main() {
    std::cout << "Running OBB SAT Unit Tests..." << std::endl;
    TestAxisAlignedSeparation();
    TestAxisAlignedOverlap();
    TestRotatedOverlap();
    TestRotatedSeparationAABBFalsePositive();
    TestForcedCollisionDetection();
    TestPointContainment();
    std::cout << "All OBB tests PASSED successfully." << std::endl;
    return 0;
}

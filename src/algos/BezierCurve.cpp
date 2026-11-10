#include "algos/BezierCurve.h"
#include <cmath>
#include <algorithm>

namespace SmartCity::algos {

glm::vec3 BezierCurveEvaluator::EvaluateDeCasteljau(const glm::vec3& p0, const glm::vec3& p1,
                                                    const glm::vec3& p2, const glm::vec3& p3, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    // Level 1 linear interpolations
    glm::vec3 q0 = glm::mix(p0, p1, t);
    glm::vec3 q1 = glm::mix(p1, p2, t);
    glm::vec3 q2 = glm::mix(p2, p3, t);

    // Level 2 linear interpolations
    glm::vec3 r0 = glm::mix(q0, q1, t);
    glm::vec3 r1 = glm::mix(q1, q2, t);

    // Level 3 final point on curve
    return glm::mix(r0, r1, t);
}

glm::vec3 BezierCurveEvaluator::EvaluatePolynomial(const glm::vec3& p0, const glm::vec3& p1,
                                                   const glm::vec3& p2, const glm::vec3& p3, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    return (uuu * p0) + (3.0f * uu * t * p1) + (3.0f * u * tt * p2) + (ttt * p3);
}

glm::vec3 BezierCurveEvaluator::EvaluateDerivative(const glm::vec3& p0, const glm::vec3& p1,
                                                   const glm::vec3& p2, const glm::vec3& p3, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    float u = 1.0f - t;
    return 3.0f * u * u * (p1 - p0) + 6.0f * u * t * (p2 - p1) + 3.0f * t * t * (p3 - p2);
}

glm::vec3 BezierCurveEvaluator::EvaluateSecondDerivative(const glm::vec3& p0, const glm::vec3& p1,
                                                         const glm::vec3& p2, const glm::vec3& p3, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    float u = 1.0f - t;
    return 6.0f * u * (p2 - 2.0f * p1 + p0) + 6.0f * t * (p3 - 2.0f * p2 + p1);
}

float BezierCurveEvaluator::EvaluateCurvature(const glm::vec3& p0, const glm::vec3& p1,
                                              const glm::vec3& p2, const glm::vec3& p3, float t) {
    glm::vec3 d1 = EvaluateDerivative(p0, p1, p2, p3, t);
    glm::vec3 d2 = EvaluateSecondDerivative(p0, p1, p2, p3, t);

    glm::vec3 crossProd = glm::cross(d1, d2);
    float crossNorm = glm::length(crossProd);
    float speed = glm::length(d1);

    if (speed < 1e-6f) return 0.0f;
    return crossNorm / (speed * speed * speed);
}

bool BezierCurveEvaluator::VerifyFormulationsAgreement(const glm::vec3& p0, const glm::vec3& p1,
                                                       const glm::vec3& p2, const glm::vec3& p3,
                                                       int sampleCount, float tolerance) {
    if (sampleCount <= 1) return true;

    for (int i = 0; i <= sampleCount; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sampleCount);
        glm::vec3 deCasteljauPt = EvaluateDeCasteljau(p0, p1, p2, p3, t);
        glm::vec3 polyPt = EvaluatePolynomial(p0, p1, p2, p3, t);

        float dist = glm::distance(deCasteljauPt, polyPt);
        if (dist > tolerance) {
            return false;
        }
    }
    return true;
}

std::vector<BezierArcSample> BezierCurveEvaluator::BuildArcLengthLUT(const glm::vec3& p0, const glm::vec3& p1,
                                                                     const glm::vec3& p2, const glm::vec3& p3,
                                                                     int subdivisions) {
    std::vector<BezierArcSample> lut;
    lut.reserve(subdivisions + 1);

    float accumulatedLength = 0.0f;
    glm::vec3 prevPoint = EvaluatePolynomial(p0, p1, p2, p3, 0.0f);

    BezierArcSample firstSample;
    firstSample.t = 0.0f;
    firstSample.arcLength = 0.0f;
    firstSample.position = prevPoint;
    firstSample.tangent = glm::normalize(EvaluateDerivative(p0, p1, p2, p3, 0.0f));
    lut.push_back(firstSample);

    for (int i = 1; i <= subdivisions; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(subdivisions);
        glm::vec3 currentPoint = EvaluatePolynomial(p0, p1, p2, p3, t);
        glm::vec3 tangent = EvaluateDerivative(p0, p1, p2, p3, t);
        float tangentLen = glm::length(tangent);

        accumulatedLength += glm::distance(currentPoint, prevPoint);

        BezierArcSample sample;
        sample.t = t;
        sample.arcLength = accumulatedLength;
        sample.position = currentPoint;
        sample.tangent = (tangentLen > 1e-6f) ? (tangent / tangentLen) : glm::vec3(0.0f, 0.0f, -1.0f);
        lut.push_back(sample);

        prevPoint = currentPoint;
    }

    return lut;
}

float BezierCurveEvaluator::ComputeTotalArcLength(const glm::vec3& p0, const glm::vec3& p1,
                                                  const glm::vec3& p2, const glm::vec3& p3,
                                                  int subdivisions) {
    auto lut = BuildArcLengthLUT(p0, p1, p2, p3, subdivisions);
    return lut.empty() ? 0.0f : lut.back().arcLength;
}

} // namespace SmartCity::algos


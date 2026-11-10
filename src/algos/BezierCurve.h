#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace SmartCity::algos {

struct BezierArcSample {
    float t{ 0.0f };
    float arcLength{ 0.0f };
    glm::vec3 position{ 0.0f };
    glm::vec3 tangent{ 0.0f };
};

class BezierCurveEvaluator {
public:
    // De Casteljau algorithm for cubic Bézier evaluation
    static glm::vec3 EvaluateDeCasteljau(const glm::vec3& p0, const glm::vec3& p1,
                                         const glm::vec3& p2, const glm::vec3& p3, float t);

    // Explicit Bernstein polynomial formulation
    static glm::vec3 EvaluatePolynomial(const glm::vec3& p0, const glm::vec3& p1,
                                        const glm::vec3& p2, const glm::vec3& p3, float t);

    // Analytical first derivative (tangent vector)
    static glm::vec3 EvaluateDerivative(const glm::vec3& p0, const glm::vec3& p1,
                                        const glm::vec3& p2, const glm::vec3& p3, float t);

    // Analytical second derivative
    static glm::vec3 EvaluateSecondDerivative(const glm::vec3& p0, const glm::vec3& p1,
                                              const glm::vec3& p2, const glm::vec3& p3, float t);

    // Curvature kappa = |r' x r''| / |r'|^3
    static float EvaluateCurvature(const glm::vec3& p0, const glm::vec3& p1,
                                   const glm::vec3& p2, const glm::vec3& p3, float t);

    // Verification check: ensures De Casteljau and Polynomial formulas agree within epsilon
    static bool VerifyFormulationsAgreement(const glm::vec3& p0, const glm::vec3& p1,
                                            const glm::vec3& p2, const glm::vec3& p3,
                                            int sampleCount = 100, float tolerance = 1e-5f);

    // Precompute Arc-Length Lookup Table via numerical integration
    static std::vector<BezierArcSample> BuildArcLengthLUT(const glm::vec3& p0, const glm::vec3& p1,
                                                          const glm::vec3& p2, const glm::vec3& p3,
                                                          int subdivisions = 200);

    // Total curve arc length
    static float ComputeTotalArcLength(const glm::vec3& p0, const glm::vec3& p1,
                                       const glm::vec3& p2, const glm::vec3& p3,
                                       int subdivisions = 200);
};

} // namespace SmartCity::algos


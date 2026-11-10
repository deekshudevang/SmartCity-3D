#include "sim/ArcLengthLUT.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace SmartCity::sim {

ArcLengthLUT::ArcLengthLUT(const city::TurningPath& path, int numSegments) {
    Build(path, numSegments);
}

void ArcLengthLUT::Build(const city::TurningPath& path, int numSegments) {
    numSegments = std::max(numSegments, 10);
    m_samples.clear();
    m_samples.reserve(numSegments + 1);

    m_maxCurvature = 0.0f;
    float accumulatedS = 0.0f;
    glm::vec3 prevPos = path.Evaluate(0.0f);

    ArcSample firstSample;
    firstSample.t = 0.0f;
    firstSample.s = 0.0f;
    firstSample.position = prevPos;
    firstSample.tangent = path.EvaluateTangent(0.0f);
    firstSample.headingRad = std::atan2(firstSample.tangent.x, firstSample.tangent.z);
    firstSample.curvature = path.CalculateCurvature(0.0f);
    m_samples.push_back(firstSample);

    for (int i = 1; i <= numSegments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(numSegments);
        glm::vec3 currPos = path.Evaluate(t);
        accumulatedS += glm::distance(prevPos, currPos);

        ArcSample s;
        s.t = t;
        s.s = accumulatedS;
        s.position = currPos;
        s.tangent = path.EvaluateTangent(t);
        s.headingRad = std::atan2(s.tangent.x, s.tangent.z);
        s.curvature = path.CalculateCurvature(t);

        if (s.curvature > m_maxCurvature) {
            m_maxCurvature = s.curvature;
        }

        m_samples.push_back(s);
        prevPos = currPos;
    }

    m_totalLength = accumulatedS;
}

void ArcLengthLUT::Sample(float s, glm::vec3& outPos, glm::vec3& outTangent, float& outHeadingRad, float& outCurvature) const {
    if (m_samples.empty()) {
        outPos = glm::vec3(0.0f);
        outTangent = glm::vec3(0.0f, 0.0f, 1.0f);
        outHeadingRad = 0.0f;
        outCurvature = 0.0f;
        return;
    }

    if (s <= 0.0f) {
        const auto& first = m_samples.front();
        outPos = first.position;
        outTangent = first.tangent;
        outHeadingRad = first.headingRad;
        outCurvature = first.curvature;
        return;
    }

    if (s >= m_totalLength) {
        const auto& last = m_samples.back();
        outPos = last.position;
        outTangent = last.tangent;
        outHeadingRad = last.headingRad;
        outCurvature = last.curvature;
        return;
    }

    // Binary search for interval
    auto it = std::lower_bound(m_samples.begin(), m_samples.end(), s,
        [](const ArcSample& sample, float val) {
            return sample.s < val;
        });

    if (it == m_samples.begin()) {
        const auto& first = m_samples.front();
        outPos = first.position;
        outTangent = first.tangent;
        outHeadingRad = first.headingRad;
        outCurvature = first.curvature;
        return;
    }

    const auto& b = *it;
    const auto& a = *(it - 1);

    float span = b.s - a.s;
    float alpha = (span > 1e-6f) ? ((s - a.s) / span) : 0.0f;
    alpha = std::clamp(alpha, 0.0f, 1.0f);

    outPos = glm::mix(a.position, b.position, alpha);
    outTangent = glm::normalize(glm::mix(a.tangent, b.tangent, alpha));

    // Angular lerp
    float diff = b.headingRad - a.headingRad;
    while (diff < -glm::pi<float>()) diff += glm::two_pi<float>();
    while (diff > glm::pi<float>()) diff -= glm::two_pi<float>();
    outHeadingRad = a.headingRad + diff * alpha;

    outCurvature = glm::mix(a.curvature, b.curvature, alpha);
}

} // namespace SmartCity::sim

#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "city/RoadNetwork.h"

namespace SmartCity::sim {

struct ArcSample {
    float t;
    float s; // cumulative arc length
    glm::vec3 position;
    glm::vec3 tangent;
    float headingRad;
    float curvature;
};

class ArcLengthLUT {
public:
    ArcLengthLUT() = default;
    explicit ArcLengthLUT(const city::TurningPath& path, int numSegments = 100);

    void Build(const city::TurningPath& path, int numSegments = 100);

    float GetTotalLength() const { return m_totalLength; }

    // Evaluates curve at physical arc length distance s in [0, totalLength]
    void Sample(float s, glm::vec3& outPos, glm::vec3& outTangent, float& outHeadingRad, float& outCurvature) const;

    float GetMaxCurvature() const { return m_maxCurvature; }

private:
    float m_totalLength{ 0.0f };
    float m_maxCurvature{ 0.0f };
    std::vector<ArcSample> m_samples;
};

} // namespace SmartCity::sim


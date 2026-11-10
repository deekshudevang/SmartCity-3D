#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace SmartCity::gfx {

struct GpuProfileResult {
    std::string name;
    float timeMs{ 0.0f };
};

class GpuProfiler {
public:
    GpuProfiler();
    ~GpuProfiler();

    void BeginFrame();
    void EndFrame();

    void BeginSection(const std::string& name);
    void EndSection(const std::string& name);

    float GetSectionTimeMs(const std::string& name) const;
    const std::vector<GpuProfileResult>& GetResults() const { return m_results; }

private:
    struct QueryData {
        uint32_t queries[2]{ 0, 0 }; // Ping-pong double buffer to prevent GPU stalls
        float lastTimeMs{ 0.0f };
        bool hasRecorded{ false };
    };

    void EnsureQuery(const std::string& name);

    uint32_t m_currentBufferIndex{ 0 };
    std::unordered_map<std::string, QueryData> m_queries;
    std::vector<GpuProfileResult> m_results;
    bool m_glInitialized{ false };
};

} // namespace SmartCity::gfx

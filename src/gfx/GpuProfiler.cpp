#include "gfx/GpuProfiler.h"
#include <glad/glad.h>
#include "core/Logger.h"

namespace SmartCity::gfx {

GpuProfiler::GpuProfiler()
    : m_currentBufferIndex(0)
    , m_glInitialized(false)
{
    if (glad_glGenQueries != nullptr) {
        m_glInitialized = true;
    }
}

GpuProfiler::~GpuProfiler() {
    if (m_glInitialized && glad_glDeleteQueries != nullptr) {
        for (auto& pair : m_queries) {
            glDeleteQueries(2, pair.second.queries);
        }
    }
}

void GpuProfiler::EnsureQuery(const std::string& name) {
    if (!m_glInitialized) return;

    if (m_queries.find(name) == m_queries.end()) {
        QueryData q;
        glGenQueries(2, q.queries);
        q.lastTimeMs = 0.0f;
        q.hasRecorded = false;
        m_queries[name] = q;
    }
}

void GpuProfiler::BeginFrame() {
    if (!m_glInitialized) return;

    // Swap ping-pong buffer
    m_currentBufferIndex = 1 - m_currentBufferIndex;
    uint32_t readBuffer = 1 - m_currentBufferIndex;

    m_results.clear();
    for (auto& pair : m_queries) {
        auto& q = pair.second;
        if (q.hasRecorded) {
            int available = 0;
            glGetQueryObjectiv(q.queries[readBuffer], GL_QUERY_RESULT_AVAILABLE, &available);
            if (available) {
                uint64_t timeElapsedNs = 0;
                glGetQueryObjectui64v(q.queries[readBuffer], GL_QUERY_RESULT, &timeElapsedNs);
                q.lastTimeMs = static_cast<float>(timeElapsedNs) / 1000000.0f;
            }
        }
        m_results.push_back({ pair.first, q.lastTimeMs });
    }
}

void GpuProfiler::EndFrame() {
    // Frame boundary marker
}

void GpuProfiler::BeginSection(const std::string& name) {
    if (!m_glInitialized) return;

    EnsureQuery(name);
    glBeginQuery(GL_TIME_ELAPSED, m_queries[name].queries[m_currentBufferIndex]);
}

void GpuProfiler::EndSection(const std::string& name) {
    if (!m_glInitialized) return;

    auto it = m_queries.find(name);
    if (it != m_queries.end()) {
        glEndQuery(GL_TIME_ELAPSED);
        it->second.hasRecorded = true;
    }
}

float GpuProfiler::GetSectionTimeMs(const std::string& name) const {
    auto it = m_queries.find(name);
    if (it != m_queries.end()) {
        return it->second.lastTimeMs;
    }
    return 0.0f;
}

} // namespace SmartCity::gfx

#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "gfx/Camera.h"
#include "gfx/Mesh.h"
#include "gfx/Material.h"
#include "gfx/Light.h"
#include "gfx/Frustum.h"

namespace SmartCity::scene {
    class SceneObject;
}

namespace SmartCity::gfx {

struct RenderStats {
    uint32_t drawCalls = 0;
    uint32_t triangleCount = 0;
    uint32_t visibleObjects = 0;
    uint32_t culledObjects = 0;
    float frameTimeMs = 0.0f;
};

struct RenderItem {
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
    glm::mat4 transform;
    glm::mat3 normalMatrix;
    uint64_t sortKey;
};

class Renderer {
public:
    Renderer();
    ~Renderer() = default;

    void Init();
    void SetViewport(int x, int y, int width, int height);

    void BeginFrame(const Camera& camera, const LightingEnvironment& lighting, const glm::vec4& clearColor = glm::vec4(0.12f, 0.14f, 0.18f, 1.0f));
    void Submit(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, const glm::mat4& transform, const glm::mat3& normalMatrix);
    void Submit(const scene::SceneObject& object);
    void EndFrame();

    const RenderStats& GetStats() const { return m_stats; }
    void ResetStats();

    void SetFrustumCullingEnabled(bool enabled) { m_enableFrustumCulling = enabled; }
    bool IsFrustumCullingEnabled() const { return m_enableFrustumCulling; }

    void SetDepthTest(bool enabled);
    void SetCullFace(bool enabled, bool backFace = true, bool ccw = true);

private:
    void FlushQueue();

    RenderStats m_stats;
    std::vector<RenderItem> m_renderQueue;

    glm::mat4 m_viewProjection;
    glm::vec3 m_cameraPos;
    Frustum m_cameraFrustum;
    LightingEnvironment m_lighting;

    bool m_enableFrustumCulling;
};

} // namespace SmartCity::gfx


#include "gfx/Renderer.h"
#include "scene/SceneObject.h"
#include "core/Logger.h"
#include <glad/glad.h>
#include <algorithm>

namespace SmartCity::gfx {

Renderer::Renderer()
    : m_viewProjection(1.0f)
    , m_cameraPos(0.0f)
    , m_enableFrustumCulling(true)
{
}

void Renderer::Init() {
    // 1. Depth test configuration
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // 2. Back-face culling with Counter-Clockwise winding order
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    core::Logger::Info("Renderer initialized: Depth test (GL_LESS) & Back-face culling (GL_CCW) active.");
}

void Renderer::SetViewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
}

void Renderer::SetDepthTest(bool enabled) {
    if (enabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void Renderer::SetCullFace(bool enabled, bool backFace, bool ccw) {
    if (enabled) {
        glEnable(GL_CULL_FACE);
        glCullFace(backFace ? GL_BACK : GL_FRONT);
        glFrontFace(ccw ? GL_CCW : GL_CW);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

void Renderer::ResetStats() {
    m_stats.drawCalls = 0;
    m_stats.triangleCount = 0;
    m_stats.visibleObjects = 0;
    m_stats.culledObjects = 0;
}

void Renderer::BeginFrame(const Camera& camera, const LightingEnvironment& lighting, const glm::vec4& clearColor) {
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_viewProjection = camera.GetViewProjectionMatrix();
    m_cameraPos = camera.GetPosition();
    m_cameraFrustum = camera.GetFrustum();
    m_lighting = lighting;

    ResetStats();
    m_renderQueue.clear();
}

void Renderer::Submit(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, const glm::mat4& transform, const glm::mat3& normalMatrix) {
    if (!mesh || !material) return;

    RenderItem item;
    item.mesh = mesh;
    item.material = material;
    item.transform = transform;
    item.normalMatrix = normalMatrix;
    item.sortKey = material->GetSortKey();

    m_renderQueue.push_back(item);
}

void Renderer::Submit(const scene::SceneObject& object) {
    if (!object.IsVisible()) return;

    // Frustum culling test against world AABB
    if (m_enableFrustumCulling) {
        if (!m_cameraFrustum.IntersectsAABB(object.GetWorldAABB())) {
            m_stats.culledObjects++;
            return;
        }
    }

    m_stats.visibleObjects++;

    glm::mat4 world = object.GetTransform().GetWorldMatrix();
    glm::mat3 normalMat = object.GetTransform().GetNormalMatrix();

    if (object.GetModel()) {
        const auto& model = object.GetModel();
        const auto& meshes = model->GetMeshes();
        const auto& materials = model->GetMaterials();
        const auto& transforms = model->GetMeshTransforms();

        for (size_t i = 0; i < meshes.size(); ++i) {
            glm::mat4 meshWorld = world * (i < transforms.size() ? transforms[i] : glm::mat4(1.0f));
            glm::mat3 meshNorm = glm::transpose(glm::inverse(glm::mat3(meshWorld)));
            auto mat = (i < materials.size() && materials[i]) ? materials[i] : object.GetMaterial();
            if (mat) {
                Submit(meshes[i], mat, meshWorld, meshNorm);
            }
        }
    } else if (object.GetMesh() && object.GetMaterial()) {
        Submit(object.GetMesh(), object.GetMaterial(), world, normalMat);
    }

    for (const auto& child : object.GetChildren()) {
        if (child) {
            Submit(*child);
        }
    }
}

void Renderer::FlushQueue() {
    if (m_renderQueue.empty()) return;

    // Sort render items by shader then material to minimize expensive GPU context switches
    std::sort(m_renderQueue.begin(), m_renderQueue.end(), [](const RenderItem& a, const RenderItem& b) {
        return a.sortKey < b.sortKey;
    });

    uint64_t lastShaderId = 0;
    uint32_t lastMaterialId = 0;
    Shader* currentShader = nullptr;

    for (const auto& item : m_renderQueue) {
        if (!item.mesh || !item.material) continue;

        auto shader = item.material->GetShader();
        if (!shader) continue;

        uint64_t shaderId = static_cast<uint64_t>(shader->GetRendererId());
        uint32_t materialId = item.material->GetId();

        // 1. Bind Shader only when it changes
        if (shaderId != lastShaderId || currentShader == nullptr) {
            currentShader = shader.get();
            currentShader->Bind();
            currentShader->SetMat4("u_ViewProjection", m_viewProjection);
            m_lighting.ApplyToShader(currentShader, m_cameraPos);
            lastShaderId = shaderId;
            lastMaterialId = 0; // Force material rebinding on shader switch
        }

        // 2. Bind Material only when it changes
        if (materialId != lastMaterialId) {
            item.material->Bind();
            currentShader->SetInt("u_UseMaterial", 1);
            lastMaterialId = materialId;
        }

        // 3. Set Per-Object Model & Normal Matrices
        currentShader->SetMat4("u_Model", item.transform);
        currentShader->SetMat3("u_NormalMatrix", item.normalMatrix);
        currentShader->SetInt("u_HasNormalMatrix", 1);

        // 4. Draw Mesh
        item.mesh->Draw();

        m_stats.drawCalls++;
        m_stats.triangleCount += item.mesh->GetIndexCount() / 3;
    }
}

void Renderer::EndFrame() {
    FlushQueue();
}

} // namespace SmartCity::gfx


#include "ui/VehiclePreviewPanel.h"
#include "smartcity/vehicles/VehicleLibrary.h"
#include "sim/VehicleManager.h"
#include "core/Logger.h"
#include <glad/glad.h>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace SmartCity::ui {

VehiclePreviewPanel::VehiclePreviewPanel() {
    InitFBO(m_fboWidth, m_fboHeight);
}

VehiclePreviewPanel::~VehiclePreviewPanel() {
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        glDeleteTextures(1, &m_colorTex);
        glDeleteRenderbuffers(1, &m_depthRbo);
    }
}

void VehiclePreviewPanel::InitFBO(int width, int height) {
    if (glad_glGenFramebuffers == nullptr) return; // Headless guard

    m_fboWidth = width;
    m_fboHeight = height;

    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        glDeleteTextures(1, &m_colorTex);
        glDeleteRenderbuffers(1, &m_depthRbo);
        m_fbo = 0;
    }

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Color texture attachment
    glGenTextures(1, &m_colorTex);
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);

    // Depth renderbuffer attachment
    glGenRenderbuffers(1, &m_depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        core::Logger::Error("VehiclePreviewPanel: Offscreen preview FBO is incomplete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VehiclePreviewPanel::RenderOffscreenFBO(const vehicles::VehiclePreset& preset,
                                             gfx::Shader* sceneShader,
                                             gfx::Shader* emissiveShader) {
    if (m_fbo == 0 || !sceneShader) return;

    // Save previous viewport
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_fboWidth, m_fboHeight);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glClearColor(0.12f, 0.13f, 0.16f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Compute Orbit Camera
    float camX = std::cos(m_cameraElevation) * std::sin(m_turntableAngle) * m_cameraDistance;
    float camY = std::sin(m_cameraElevation) * m_cameraDistance + 0.6f;
    float camZ = std::cos(m_cameraElevation) * std::cos(m_turntableAngle) * m_cameraDistance;
    glm::vec3 cameraPos(camX, camY, camZ);
    glm::vec3 targetPos(0.0f, 0.6f, 0.0f);

    glm::mat4 view = glm::lookAt(cameraPos, targetPos, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), static_cast<float>(m_fboWidth) / static_cast<float>(m_fboHeight), 0.1f, 100.0f);
    glm::mat4 viewProj = proj * view;

    // Bind preview uniforms
    sceneShader->Bind();
    sceneShader->SetMat4("u_ViewProjection", viewProj);
    sceneShader->SetVec3("u_CameraPos", cameraPos);
    sceneShader->SetVec3("u_DirLight.direction", glm::normalize(glm::vec3(0.5f, -1.0f, 0.7f)));
    sceneShader->SetVec3("u_DirLight.color", glm::vec3(1.0f, 0.98f, 0.92f));
    sceneShader->SetFloat("u_DirLight.intensity", 1.2f);
    sceneShader->SetVec3("u_AmbientColor", glm::vec3(0.35f, 0.38f, 0.44f));
    sceneShader->SetInt("u_FogEnabled", 0);
    sceneShader->SetInt("u_ShadowMap", 0);
    sceneShader->SetFloat("u_ShadowFade", 0.0f);
    sceneShader->SetFloat("u_RainWetness", 0.0f);
    sceneShader->SetFloat("u_WindowEmissive", 0.0f);
    sceneShader->SetInt("u_NumPointLights", 0);

    if (emissiveShader) {
        emissiveShader->Bind();
        emissiveShader->SetMat4("u_ViewProjection", viewProj);
    }

    // Render vehicle preset
    auto& renderer = vehicles::VehicleLibrary::Get().GetRenderer();
    renderer.RenderVehicle(preset, glm::vec3(0.0f), 0.0f,
                           m_wheelRotation, m_steeringAngle,
                           0.0f, false, false,
                           sceneShader, emissiveShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}

void VehiclePreviewPanel::Render(sim::VehicleManager* simManager,
                                 gfx::Shader* sceneShader,
                                 gfx::Shader* emissiveShader,
                                 bool* pOpen) {
    if (pOpen && !*pOpen) return;

    ImGui::SetNextWindowSize(ImVec2(560, 680), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Vehicle 3D Interactive Preview", pOpen, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }

    auto& lib = vehicles::VehicleLibrary::Get();
    const auto* preset = lib.GetSelectedPreset();

    if (!preset) {
        ImGui::TextDisabled("Select a vehicle from the Gallery to inspect and preview in 3D.");
        ImGui::End();
        return;
    }

    // Update turntable rotation if auto-rotate active
    if (m_autoRotate) {
        m_turntableAngle += ImGui::GetIO().DeltaTime * 0.45f;
        if (m_turntableAngle > glm::two_pi<float>()) {
            m_turntableAngle -= glm::two_pi<float>();
        }
    }

    // Render offscreen FBO
    RenderOffscreenFBO(*preset, sceneShader, emissiveShader);

    // 1. Display Embedded 3D Viewport Texture
    if (m_colorTex != 0) {
        ImVec2 previewSize(static_cast<float>(m_fboWidth), static_cast<float>(m_fboHeight));
        // Centering
        float availX = ImGui::GetContentRegionAvail().x;
        if (availX > previewSize.x) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availX - previewSize.x) * 0.5f);
        }
        ImGui::Image((ImTextureID)(intptr_t)m_colorTex, previewSize, ImVec2(0, 1), ImVec2(1, 0));
    }

    // 2. 3D Model Status Badge
    ImGui::Spacing();
    if (preset->IsModelAvailable()) {
        ImGui::TextColored(ImVec4(0.2f, 0.85f, 0.3f, 1.0f), "  [3D ASSET AVAILABLE: %s]", preset->modelAssetPath.c_str());
    } else {
        ImGui::TextColored(ImVec4(0.85f, 0.65f, 0.2f, 1.0f), "  [MISSING_MODEL: No matching 3D mesh. Displaying proportional CAD chassis]");
    }
    ImGui::Separator();

    // 3. Camera & Preview Controls
    if (ImGui::CollapsingHeader("Preview Camera & Turntable Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Auto-Rotate Turntable", &m_autoRotate);
        ImGui::SameLine();
        if (ImGui::Button("Reset Camera")) {
            m_turntableAngle = 0.0f;
            m_cameraElevation = 0.35f;
            m_cameraDistance = 7.5f;
            m_steeringAngle = 0.0f;
            m_wheelRotation = 0.0f;
        }

        ImGui::SliderAngle("Orbit Angle", &m_turntableAngle, 0.0f, 360.0f);
        ImGui::SliderAngle("Elevation Angle", &m_cameraElevation, -10.0f, 75.0f);
        ImGui::SliderFloat("Camera Distance", &m_cameraDistance, 3.5f, 15.0f, "%.1f m");

        ImGui::SliderAngle("Front Wheel Steering", &m_steeringAngle, -35.0f, 35.0f);
        ImGui::SliderAngle("Wheel Spin", &m_wheelRotation, 0.0f, 360.0f);
    }

    // 4. Traffic Simulation Spawning Section
    if (simManager && ImGui::CollapsingHeader("Spawn Into Traffic Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled("Spawn selected '%s' into the live 4-way intersection:", preset->displayName.c_str());

        const char* approaches[] = { "North Approach (-Z)", "South Approach (+Z)", "East Approach (+X)", "West Approach (-X)" };
        ImGui::SetNextItemWidth(200.0f);
        ImGui::Combo("Approach", &m_spawnApproachIndex, approaches, IM_ARRAYSIZE(approaches));

        ImGui::SameLine();
        const char* movements[] = { "Left Turn", "Through (Straight)", "Right Turn" };
        ImGui::SetNextItemWidth(180.0f);
        ImGui::Combo("Movement", &m_spawnMovementIndex, movements, IM_ARRAYSIZE(movements));

        ImGui::Spacing();
        if (ImGui::Button("Spawn Selected Vehicle Into Simulation", ImVec2(-1.0f, 32.0f))) {
            lib.SpawnSelectedVehicleIntoSim(*simManager, m_spawnApproachIndex, m_spawnMovementIndex);
        }
    }

    ImGui::End();
}

} // namespace SmartCity::ui


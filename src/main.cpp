#include <iostream>
#include <memory>
#include <vector>

#include "core/Logger.h"
#include "core/Window.h"
#include "core/Time.h"
#include "core/Input.h"

#include "graphics/Buffer.h"
#include "graphics/Mesh.h"
#include "graphics/Shader.h"
#include "graphics/Camera.h"
#include "graphics/Light.h"
#include "graphics/ShadowMap.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

int main(int argc, char* argv[]) {
    SmartCity::Logger::Init();
    SmartCity::Logger::Info("==================================================================");
    SmartCity::Logger::Info(" SmartCity 3D — Intelligent Traffic Intersection Simulation");
    SmartCity::Logger::Info(" Phase 2: 3D Graphics Pipeline, Shaders, Lighting & Camera");
    SmartCity::Logger::Info("==================================================================");

    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--headless") {
            headless = true;
        }
    }

    SmartCity::WindowProps props;
    props.title = "SmartCity 3D — Intelligent Traffic Intersection";
    props.width = 1600;
    props.height = 900;
    props.vsync = true;
    props.headless = headless;

    SmartCity::Window window(props);
    if (!window.Init()) {
        SmartCity::Logger::Error("Failed to initialize main window and OpenGL context!");
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Apply Unslop design standard: dark theme, restrained borders, no neon slop
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    ImGui_ImplGlfw_InitForOpenGL(window.GetNativeWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Camera setup
    SmartCity::Camera camera(45.0f, window.GetAspectRatio(), 0.1f, 1000.0f);
    window.SetResizeCallback([&camera](int w, int h) {
        camera.SetViewportSize(w, h);
    });

    // Shaders initialization
    auto sceneShader = std::make_unique<SmartCity::Shader>(
        "assets/shaders/scene.vert",
        "assets/shaders/scene.frag",
        true
    );

    auto shadowShader = std::make_unique<SmartCity::Shader>(
        "assets/shaders/shadow_depth.vert",
        "assets/shaders/shadow_depth.frag",
        true
    );

    auto emissiveShader = std::make_unique<SmartCity::Shader>(
        "assets/shaders/light_indicator.vert",
        "assets/shaders/light_indicator.frag",
        true
    );

    // Shadow Map (2048x2048)
    auto shadowMap = std::make_unique<SmartCity::ShadowMap>(2048, 2048);

    // Day/Night Cycle manager
    SmartCity::DayNightCycle dayNight(14.0f); // 2:00 PM afternoon

    // Sample demonstration geometry for Phase 2 validation
    auto groundPlane = SmartCity::Mesh::CreatePlane(120.0f, 120.0f, glm::vec4(0.18f, 0.20f, 0.22f, 1.0f), 20.0f, 20.0f);
    auto intersectionBlock = SmartCity::Mesh::CreateCube(glm::vec3(4.0f, 2.0f, 4.0f), glm::vec4(0.85f, 0.35f, 0.25f, 1.0f));
    auto streetlightPole = SmartCity::Mesh::CreateCylinder(0.2f, 8.0f, 16, glm::vec4(0.4f, 0.42f, 0.45f, 1.0f));
    auto streetlightHead = SmartCity::Mesh::CreateSphere(0.6f, 12, 16, glm::vec4(1.0f, 0.95f, 0.6f, 1.0f));

    // Sample Streetlight Point Light
    std::vector<SmartCity::PointLight> pointLights;
    SmartCity::PointLight pLight1;
    pLight1.position = glm::vec3(12.0f, 8.0f, 12.0f);
    pLight1.color = glm::vec3(1.0f, 0.88f, 0.65f);
    pLight1.intensity = 1.5f;
    pLight1.radius = 35.0f;
    pointLights.push_back(pLight1);

    SmartCity::Time timer(1.0 / 60.0);

    int frameLimit = headless ? 60 : -1;
    int framesRendered = 0;

    SmartCity::Logger::Info("Entering main graphics & simulation loop...");

    while (!window.ShouldClose()) {
        window.PollEvents();
        timer.Update();

        float dt = static_cast<float>(timer.GetFrameDeltaTime());

        // Process input only when ImGui is not actively capturing
        if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) {
            camera.Update(dt);
        }

        if (SmartCity::Input::GetKeyDown(GLFW_KEY_ESCAPE)) {
            window.SetShouldClose(true);
        }

        // Fixed-timestep simulation updates
        while (timer.ShouldSimulateStep()) {
            dayNight.Update(static_cast<float>(timer.GetFixedDeltaTime()));
            timer.ConsumeSimulateStep();
        }

        // Compute Matrices
        const auto& dirLight = dayNight.GetLight();
        glm::mat4 lightSpaceMatrix = shadowMap->CalculateLightSpaceMatrix(dirLight.direction);
        glm::mat4 viewProj = camera.GetViewProjectionMatrix();

        // ---------------- PASS 1: SHADOW MAP DEPTH PASS ----------------
        shadowMap->BindForWriting();
        shadowShader->Bind();
        shadowShader->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);

        // Ground
        glm::mat4 groundModel = glm::mat4(1.0f);
        shadowShader->SetMat4("u_Model", groundModel);
        groundPlane->Draw();

        // Center cube
        glm::mat4 cubeModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        shadowShader->SetMat4("u_Model", cubeModel);
        intersectionBlock->Draw();

        // Streetlight pole
        glm::mat4 poleModel = glm::translate(glm::mat4(1.0f), glm::vec3(12.0f, 4.0f, 12.0f));
        shadowShader->SetMat4("u_Model", poleModel);
        streetlightPole->Draw();

        shadowMap->Unbind(window.GetWidth(), window.GetHeight());

        // ---------------- PASS 2: MAIN SCENE RENDER PASS ----------------
        glm::vec3 clearColor = dayNight.GetClearColor();
        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneShader->Bind();
        sceneShader->SetMat4("u_ViewProjection", viewProj);
        sceneShader->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);
        sceneShader->SetVec3("u_CameraPos", camera.GetPosition());

        // Directional Sun/Moon uniforms
        sceneShader->SetVec3("u_DirLight.direction", dirLight.direction);
        sceneShader->SetVec3("u_DirLight.color", dirLight.color);
        sceneShader->SetFloat("u_DirLight.intensity", dirLight.intensity);
        sceneShader->SetVec3("u_DirLight.ambient", dirLight.ambient);

        // Point light uniforms
        bool streetlightsActive = dayNight.AreStreetlightsActive();
        sceneShader->SetInt("u_NumPointLights", streetlightsActive ? static_cast<int>(pointLights.size()) : 0);
        for (size_t i = 0; i < pointLights.size(); ++i) {
            std::string prefix = "u_PointLights[" + std::to_string(i) + "].";
            sceneShader->SetVec3(prefix + "position", pointLights[i].position);
            sceneShader->SetVec3(prefix + "color", pointLights[i].color);
            sceneShader->SetFloat(prefix + "intensity", pointLights[i].intensity);
            sceneShader->SetFloat(prefix + "radius", pointLights[i].radius);
        }

        // Material & Shadow Map sampler
        sceneShader->SetInt("u_ShadowMap", 1);
        shadowMap->BindTexture(1);
        sceneShader->SetInt("u_HasTexture", 0);
        sceneShader->SetFloat("u_SpecularStrength", 0.4f);
        sceneShader->SetFloat("u_Shininess", 32.0f);

        // Render Ground
        sceneShader->SetMat4("u_Model", groundModel);
        groundPlane->Draw();

        // Render Center Cube
        sceneShader->SetMat4("u_Model", cubeModel);
        intersectionBlock->Draw();

        // Render Streetlight Pole
        sceneShader->SetMat4("u_Model", poleModel);
        streetlightPole->Draw();

        // ---------------- PASS 3: EMISSIVE FIXTURES ----------------
        emissiveShader->Bind();
        emissiveShader->SetMat4("u_ViewProjection", viewProj);
        glm::mat4 lampHeadModel = glm::translate(glm::mat4(1.0f), glm::vec3(12.0f, 8.0f, 12.0f));
        emissiveShader->SetMat4("u_Model", lampHeadModel);
        glm::vec4 lampColor = streetlightsActive ? glm::vec4(1.0f, 0.95f, 0.7f, 1.0f) : glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
        emissiveShader->SetVec4("u_EmissiveColor", lampColor);
        streetlightHead->Draw();

        // ---------------- PASS 4: IMGUI UI OVERLAY ----------------
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Control Panel
        {
            ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(420.0f, 380.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("SmartCity 3D — Graphics & Environment Control");

            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.4f, 1.0f), "Pipeline: OpenGL 3.3 Core Profile");
            ImGui::Text("FPS: %.1f (%.2f ms)", timer.GetFPS(), timer.GetFrameDeltaTime() * 1000.0);
            ImGui::Separator();

            // Camera Controls
            ImGui::Text("Camera Preset:");
            if (ImGui::Button("Overview (45°)")) camera.ApplyPreset(SmartCity::CameraPreset::Overview);
            ImGui::SameLine();
            if (ImGui::Button("Top-Down")) camera.ApplyPreset(SmartCity::CameraPreset::TopDown);
            ImGui::SameLine();
            if (ImGui::Button("North Cam")) camera.ApplyPreset(SmartCity::CameraPreset::NorthTrafficCam);

            if (ImGui::Button("Pedestrian View")) camera.ApplyPreset(SmartCity::CameraPreset::PedestrianView);
            ImGui::SameLine();
            if (ImGui::Button("East Cam")) camera.ApplyPreset(SmartCity::CameraPreset::EastTrafficCam);

            float fov = camera.GetFOV();
            if (ImGui::SliderFloat("Camera FOV", &fov, 25.0f, 90.0f)) {
                camera.SetFOV(fov);
            }

            ImGui::Separator();
            // Day/Night Controls
            ImGui::Text("Atmosphere & Lighting:");
            float timeHours = dayNight.GetTimeOfDay();
            if (ImGui::SliderFloat("Time of Day", &timeHours, 0.0f, 24.0f, "%.1f hrs")) {
                dayNight.SetTimeOfDay(timeHours);
            }

            bool dayNightPaused = dayNight.IsPaused();
            if (ImGui::Checkbox("Pause Sun Cycle", &dayNightPaused)) {
                dayNight.SetPaused(dayNightPaused);
            }

            ImGui::Text("Sun Direction: (%.2f, %.2f, %.2f)", dirLight.direction.x, dirLight.direction.y, dirLight.direction.z);
            ImGui::Text("Streetlights: %s", streetlightsActive ? "ACTIVE (ON)" : "INACTIVE (OFF)");

            ImGui::Separator();
            ImGui::TextDisabled("Controls: Right-click+drag to orbit/look. Middle-click to pan. Scroll to zoom.");

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.SwapBuffers();

        framesRendered++;
        if (frameLimit > 0 && framesRendered >= frameLimit) {
            SmartCity::Logger::Info("Headless execution reached frame limit (", frameLimit, "). Exiting.");
            break;
        }
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    window.Shutdown();
    SmartCity::Logger::Info("SmartCity 3D Phase 2 terminated successfully.");
    return 0;
}

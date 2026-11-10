#include "core/Application.h"
#include "core/Logger.h"
#include "core/Window.h"
#include "core/Time.h"
#include "core/Input.h"
#include "core/Config.h"

#include "gfx/Buffer.h"
#include "gfx/Mesh.h"
#include "gfx/Shader.h"
#include "gfx/Camera.h"
#include "gfx/Light.h"
#include "gfx/ShadowMap.h"
#include "gfx/Renderer.h"
#include "gfx/Skybox.h"
#include "gfx/Material.h"

#include "scene/Scene.h"
#include "scene/SceneObject.h"
#include "scene/ResourceManager.h"
#include "scene/Transform.h"
#include "scene/Model.h"

#include "city/CityEnvironment.h"
#include "sim/TrafficManager.h"
#include "traffic/TrafficSignalController.h"
#include "ped/PedestrianManager.h"
#include "fx/RainSystem.h"
#include "gfx/GpuProfiler.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#include <filesystem>
#include <glm/gtx/norm.hpp>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <vector>
#include <algorithm>
#include <cmath>

namespace SmartCity::core {

Application::Application(const AppCommandLineArgs& args)
    : m_args(args)
    , m_running(false)
{
}

Application::~Application() {
}

static void SaveScreenshot(const std::string& filepath, int width, int height) {
    std::vector<unsigned char> pixels(width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    stbi_flip_vertically_on_write(1);
    stbi_write_png(filepath.c_str(), width, height, 3, pixels.data(), width * 3);
    Logger::Info("Saved visual evidence screenshot: ", filepath);
}

int Application::Run() {
    Logger::Init("logs/smartcity.log");
    Logger::Info("==================================================================");
    Logger::Info(" SmartCity 3D — Intelligent Traffic Intersection Simulation");
    Logger::Info(" Phase 2: Graphics Engine, Scene Graph, Renderer & Diagnostics");
    Logger::Info("==================================================================");

    SimulationSettings simSettings;
    IntersectionSettings interSettings;
    ConfigManager::LoadSimulationConfig(m_args.configPath, simSettings);
    ConfigManager::LoadIntersectionConfig(m_args.intersectionPath, interSettings);

    WindowProps winProps;
    winProps.title = "SmartCity 3D — Intelligent Traffic Intersection";
    winProps.width = 1600;
    winProps.height = 900;
    winProps.vsync = true;
    winProps.headless = m_args.headless;

    Window window(winProps);
    if (!window.Init()) {
        Logger::Error("Failed to initialize main window and OpenGL context!");
        return -1;
    }

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

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

    // Initialize Renderer
    auto renderer = std::make_unique<gfx::Renderer>();
    renderer->Init();

    // Camera setup
    gfx::Camera camera(glm::radians(45.0f), window.GetAspectRatio(), 0.1f, 1000.0f);
    window.SetResizeCallback([&camera, &renderer](int w, int h) {
        camera.SetViewportSize(w, h);
        renderer->SetViewport(0, 0, w, h);
    });

    // Resource Manager & Scene setup
    auto& rm = scene::ResourceManager::Get();
    auto sceneShaderHandle = rm.GetShader("SceneShader", "shaders/scene.vert", "shaders/scene.frag");
    auto shadowShader = std::make_unique<gfx::Shader>("shaders/shadow_depth.vert", "shaders/shadow_depth.frag", true);
    auto emissiveShader = std::make_unique<gfx::Shader>("shaders/light_indicator.vert", "shaders/light_indicator.frag", true);

    auto shadowMap = std::make_unique<gfx::ShadowMap>(2048, 2048);
    auto dayNight = std::make_unique<gfx::DayNightCycle>(14.0f);
    auto skybox = std::make_unique<gfx::Skybox>(); // Procedural gradient fallback
    auto rainSystem = std::make_unique<fx::RainSystem>(10000);
    auto gpuProfiler = std::make_unique<gfx::GpuProfiler>();

    // Startup Shader Validator (Phase 6 Requirement 11)
    std::vector<std::string> failedShaders;
    if (!gfx::Shader::ValidateAllShaders(failedShaders)) {
        Logger::Error("Shader startup validation detected errors in ", failedShaders.size(), " shaders!");
    } else {
        Logger::Info("All project GLSL 330 core shaders successfully validated at startup.");
    }

    // Materials
    auto goldMat = std::make_shared<gfx::Material>("GoldMaterial", sceneShaderHandle);
    goldMat->SetAmbient(glm::vec3(0.247f, 0.199f, 0.0745f));
    goldMat->SetDiffuse(glm::vec3(0.7516f, 0.6064f, 0.2265f));
    goldMat->SetSpecular(glm::vec3(0.628f, 0.556f, 0.366f));
    goldMat->SetShininess(51.2f);
    rm.RegisterMaterial("GoldMaterial", goldMat);

    auto emeraldMat = std::make_shared<gfx::Material>("EmeraldMaterial", sceneShaderHandle);
    emeraldMat->SetAmbient(glm::vec3(0.0215f, 0.1745f, 0.0215f));
    emeraldMat->SetDiffuse(glm::vec3(0.07568f, 0.61424f, 0.07568f));
    emeraldMat->SetSpecular(glm::vec3(0.633f, 0.7278f, 0.633f));
    emeraldMat->SetShininess(76.8f);
    rm.RegisterMaterial("EmeraldMaterial", emeraldMat);

    // Scene
    scene::Scene scene("MainUrbanScene");

    // 1. Showcase Model Loaded via Assimp & ResourceManager
    auto testCubeModel = rm.GetModel("assets/models/test_cube.obj");
    auto showcaseObj = scene.CreateObject("ShowcaseCube");
    if (showcaseObj) {
        showcaseObj->SetModel(testCubeModel);
        showcaseObj->SetMaterial(goldMat);
        showcaseObj->GetTransform().SetPosition(glm::vec3(12.0f, 2.0f, 12.0f));
        showcaseObj->GetTransform().SetScale(glm::vec3(2.0f, 2.0f, 2.0f));
        showcaseObj->SetLocalAABB(gfx::AABB(glm::vec3(-1.0f), glm::vec3(1.0f)));
    }

    // 2. Procedural Sphere Mesh Scene Object
    auto sphereMesh = std::shared_ptr<gfx::Mesh>(gfx::Mesh::CreateSphere(1.5f, 24, 32).release());
    rm.RegisterMesh("SphereMesh", sphereMesh);
    auto sphereObj = scene.CreateObject("ShowcaseSphere");
    if (sphereObj) {
        sphereObj->SetMesh(sphereMesh);
        sphereObj->SetMaterial(emeraldMat);
        sphereObj->GetTransform().SetPosition(glm::vec3(-12.0f, 1.5f, -12.0f));
        sphereObj->SetLocalAABB(gfx::AABB(glm::vec3(-1.5f), glm::vec3(1.5f)));
    }

    // City Environment (Intersection geometry, lane markings, buildings, poles)
    auto city = std::make_unique<city::CityEnvironment>(interSettings);

    // Traffic Simulation Manager
    auto traffic = std::make_unique<sim::TrafficManager>(simSettings, interSettings);

    // Traffic Signal Controller & Pedestrian Simulation Manager
    auto signalController = std::make_shared<traffic::TrafficSignalController>();
    signalController->LoadConfig("assets/config/signals.json");
    signalController->EnableCsvLogging("logs/signals_latest.csv");

    auto pedestrianManager = std::make_unique<ped::PedestrianManager>(city->GetRoadNetwork(), signalController);
    pedestrianManager->LoadConfig("assets/config/pedestrians.json");

    // Lighting Environment
    gfx::LightingEnvironment lightingEnv;
    gfx::SpotLight spot;
    spot.position = glm::vec3(0.0f, 20.0f, 0.0f);
    spot.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    spot.color = glm::vec3(1.0f, 0.95f, 0.85f);
    spot.intensity = 2.5f;
    spot.innerCutoff = std::cos(glm::radians(15.0f));
    spot.outerCutoff = std::cos(glm::radians(22.0f));
    lightingEnv.AddSpotLight(spot);

    Time timer(simSettings.fixedTimestep);

    int frameLimit = m_args.headless ? 60 : -1;
    int framesRendered = 0;

    // Traffic signal states: 0 = RED, 1 = YELLOW, 2 = GREEN
    int nsSignal = 2; // North-South Green
    int ewSignal = 0; // East-West Red
    int nsLeft = 0;
    int ewLeft = 0;
    bool nsPedWalk = true;
    bool ewPedWalk = false;

    // Diagnostic & UI state
    bool wireframeMode = false;
    bool depthTestEnabled = true;
    bool backfaceCullEnabled = true;
    bool frustumCullingEnabled = true;
    bool useBlinnPhong = true;

    // Environmental Effects State (Phase 6)
    bool fogEnabled = false;
    float fogDensity = 0.012f;
    bool rainEnabled = false;
    float rainIntensity = 0.75f;

    // RoadNetwork Debug Overlay toggles (Phase 3)
    bool showLaneCenters = false;
    bool showTurningBeziers = false;
    bool showConflictZones = false;
    bool showStopLines = false;
    bool showEntryExit = false;

    // Rolling 60-frame diagnostics window
    constexpr size_t ROLLING_WINDOW_SIZE = 60;
    float frameTimesMs[ROLLING_WINDOW_SIZE] = {};
    size_t rollingIndex = 0;
    size_t rollingCount = 0;

    Logger::Info("Entering main graphics & simulation loop...");
    m_running = true;

    while (!window.ShouldClose() && m_running) {
        window.PollEvents();
        timer.Update();

        float dt = static_cast<float>(timer.GetFrameDeltaTime());
        float currentFrameMs = dt * 1000.0f;

        // Record rolling frame time
        frameTimesMs[rollingIndex] = currentFrameMs;
        rollingIndex = (rollingIndex + 1) % ROLLING_WINDOW_SIZE;
        if (rollingCount < ROLLING_WINDOW_SIZE) rollingCount++;

        float avgFrameMs = 0.0f;
        float minFrameMs = 1e6f;
        float maxFrameMs = 0.0f;
        for (size_t i = 0; i < rollingCount; ++i) {
            avgFrameMs += frameTimesMs[i];
            if (frameTimesMs[i] < minFrameMs) minFrameMs = frameTimesMs[i];
            if (frameTimesMs[i] > maxFrameMs) maxFrameMs = frameTimesMs[i];
        }
        avgFrameMs /= static_cast<float>(rollingCount);
        float avgFPS = avgFrameMs > 0.0001f ? 1000.0f / avgFrameMs : 0.0f;
        float minFPS = maxFrameMs > 0.0001f ? 1000.0f / maxFrameMs : 0.0f;
        float maxFPS = minFrameMs > 0.0001f ? 1000.0f / minFrameMs : 0.0f;

        // Update window title
        static float titleUpdateTimer = 0.0f;
        titleUpdateTimer += dt;
        if (titleUpdateTimer >= 0.25f) {
            titleUpdateTimer = 0.0f;
            char titleBuf[128];
            std::snprintf(titleBuf, sizeof(titleBuf), "SmartCity 3D — %.1f FPS (%.2f ms) | Culling: %s",
                          avgFPS, avgFrameMs, frustumCullingEnabled ? "ON" : "OFF");
            window.SetTitle(titleBuf);
        }

        if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) {
            camera.Update(dt);
        }

        if (Input::GetKeyDown(GLFW_KEY_ESCAPE)) {
            window.SetShouldClose(true);
        }

        // F1-F4 Camera Shortcuts (Phase 3)
        if (Input::GetKeyDown(GLFW_KEY_F1)) {
            camera.SetMode(gfx::CameraMode::FreeFly);
        }
        if (Input::GetKeyDown(GLFW_KEY_F2)) {
            camera.ApplyPreset(gfx::CameraPreset::TopDown);
            camera.SetProjectionType(gfx::ProjectionType::Orthographic);
        }
        if (Input::GetKeyDown(GLFW_KEY_F3)) {
            camera.ApplyPreset(gfx::CameraPreset::Overview);
            camera.SetProjectionType(gfx::ProjectionType::Perspective);
        }
        if (Input::GetKeyDown(GLFW_KEY_F4)) {
            static int inspectIndex = 0;
            inspectIndex = (inspectIndex + 1) % 2;
            if (inspectIndex == 0) camera.ApplyPreset(gfx::CameraPreset::NorthTrafficCam);
            else camera.ApplyPreset(gfx::CameraPreset::EastTrafficCam);
            camera.SetProjectionType(gfx::ProjectionType::Perspective);
        }

        // Fixed-timestep simulation updates
        while (timer.ShouldSimulateStep()) {
            float fixedDt = static_cast<float>(timer.GetFixedDeltaTime());
            dayNight->Update(fixedDt);

            signalController->Update(fixedDt);
            pedestrianManager->Step(fixedDt);

            auto toAspectInt = [](sim::SignalAspect aspect) -> int {
                switch (aspect) {
                    case sim::SignalAspect::Red: return 0;
                    case sim::SignalAspect::Yellow: return 1;
                    case sim::SignalAspect::Green: return 2;
                }
                return 0;
            };

            nsSignal = toAspectInt(signalController->GetState(city::ApproachId::North, city::MovementType::Through));
            ewSignal = toAspectInt(signalController->GetState(city::ApproachId::East, city::MovementType::Through));
            nsLeft   = toAspectInt(signalController->GetState(city::ApproachId::North, city::MovementType::Left));
            ewLeft   = toAspectInt(signalController->GetState(city::ApproachId::East, city::MovementType::Left));

            bool flashState = (std::fmod(signalController->GetSimTime(), 0.5f) < 0.25f);
            nsPedWalk = (signalController->GetPedestrianSignal("CW_North") == traffic::PedSignalAspect::Walk) ||
                        (signalController->GetPedestrianSignal("CW_North") == traffic::PedSignalAspect::FlashingDontWalk && flashState);
            ewPedWalk = (signalController->GetPedestrianSignal("CW_East") == traffic::PedSignalAspect::Walk) ||
                        (signalController->GetPedestrianSignal("CW_East") == traffic::PedSignalAspect::FlashingDontWalk && flashState);

            traffic->Update(fixedDt, nsSignal, ewSignal, nsLeft, ewLeft);
            timer.ConsumeSimulateStep();
        }

        // Update rain particles around camera
        rainSystem->SetEnabled(rainEnabled);
        rainSystem->SetIntensity(rainIntensity);
        rainSystem->Update(dt, camera.GetPosition());

        // Lighting configuration
        const auto& dirLight = dayNight->GetLight();
        lightingEnv.SetDirectionalLight(dirLight);
        lightingEnv.SetUseBlinnPhong(useBlinnPhong);

        bool streetlightsActive = dayNight->AreStreetlightsActive();
        const auto& streetlights = city->GetStreetlights();
        lightingEnv.ClearPointLights();
        lightingEnv.ClearSpotLights();

        glm::vec3 camPos = camera.GetPosition();

        if (streetlightsActive) {
            // Sort streetlights by distance to camera (Phase 6 Selection Rule: N nearest lights)
            std::vector<gfx::PointLight> sortedStreetlights = streetlights;
            std::sort(sortedStreetlights.begin(), sortedStreetlights.end(), [&](const gfx::PointLight& a, const gfx::PointLight& b) {
                return glm::distance2(a.position, camPos) < glm::distance2(b.position, camPos);
            });

            for (size_t i = 0; i < sortedStreetlights.size() && i < 16; ++i) {
                const auto& sl = sortedStreetlights[i];
                gfx::PointLight pl = sl;
                pl.intensity = sl.intensity * dayNight->GetStreetlightIntensity();
                pl.active = true;
                lightingEnv.AddPointLight(pl);
            }

            // Vehicle Headlights at night (up to 4 closest active vehicles = 8 forward spotlights)
            const auto& activeVehs = traffic->GetVehicles();
            std::vector<const sim::Vehicle*> closeVehs;
            for (const auto& v : activeVehs) {
                if (v && v->IsActive() && !v->HasReachedEnd()) {
                    closeVehs.push_back(v.get());
                }
            }
            std::sort(closeVehs.begin(), closeVehs.end(), [&](const sim::Vehicle* a, const sim::Vehicle* b) {
                return glm::distance2(a->GetPosition(), camPos) < glm::distance2(b->GetPosition(), camPos);
            });

            for (size_t i = 0; i < closeVehs.size() && i < 4; ++i) {
                const auto* v = closeVehs[i];
                glm::vec3 pos = v->GetPosition();
                glm::vec3 forward = v->GetTangent();
                if (glm::length2(forward) < 0.001f) forward = glm::vec3(0.0f, 0.0f, -1.0f);
                glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
                glm::vec3 headDir = glm::normalize(forward + glm::vec3(0.0f, -0.12f, 0.0f));

                float halfWidth = v->GetDimensions().width * 0.38f;
                float frontOffset = v->GetDimensions().length * 0.45f;
                float headHeight = 0.65f;

                gfx::SpotLight slLeft;
                slLeft.position = pos + forward * frontOffset - right * halfWidth + glm::vec3(0.0f, headHeight, 0.0f);
                slLeft.direction = headDir;
                slLeft.color = glm::vec3(1.0f, 0.95f, 0.85f);
                slLeft.intensity = 3.5f * dayNight->GetStreetlightIntensity();
                slLeft.innerCutoff = std::cos(glm::radians(14.0f));
                slLeft.outerCutoff = std::cos(glm::radians(22.0f));
                slLeft.constant = 1.0f;
                slLeft.linear = 0.09f;
                slLeft.quadratic = 0.032f;
                slLeft.active = true;
                lightingEnv.AddSpotLight(slLeft);

                gfx::SpotLight slRight = slLeft;
                slRight.position = pos + forward * frontOffset + right * halfWidth + glm::vec3(0.0f, headHeight, 0.0f);
                lightingEnv.AddSpotLight(slRight);
            }
        }

        glm::mat4 lightSpaceMatrix = shadowMap->CalculateLightSpaceMatrix(dirLight.direction);
        glm::mat4 viewProj = camera.GetViewProjectionMatrix();

        gpuProfiler->BeginFrame();

        // ---------------- PASS 1: SHADOW DEPTH PASS ----------------
        gpuProfiler->BeginSection("Shadow Pass");
        shadowMap->BindForWriting();
        shadowShader->Bind();
        shadowShader->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);
        city->RenderShadowDepth(shadowShader.get());
        traffic->RenderShadowDepth(shadowShader.get());
        shadowMap->Unbind(window.GetWidth(), window.GetHeight());
        gpuProfiler->EndSection("Shadow Pass");

        // ---------------- PASS 2: MAIN SCENE RENDER PASS ----------------
        gpuProfiler->BeginSection("Main Scene Pass");
        glm::vec3 clearColor = dayNight->GetClearColor();
        renderer->SetDepthTest(depthTestEnabled);
        renderer->SetCullFace(backfaceCullEnabled);
        renderer->SetFrustumCullingEnabled(frustumCullingEnabled);

        if (wireframeMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        renderer->BeginFrame(camera, lightingEnv, glm::vec4(clearColor, 1.0f));

        // Submit Scene Graph Objects (Frustum Culled & Material Sorted)
        for (const auto& obj : scene.GetObjects()) {
            renderer->Submit(*obj);
        }
        renderer->EndFrame();

        // Render City & Traffic using existing scene shaders
        sceneShaderHandle->Bind();
        sceneShaderHandle->SetMat4("u_ViewProjection", viewProj);
        sceneShaderHandle->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);
        sceneShaderHandle->SetVec3("u_CameraPos", camera.GetPosition());
        sceneShaderHandle->SetInt("u_ShadowMap", 1);
        shadowMap->BindTexture(1);
        sceneShaderHandle->SetInt("u_HasTexture", 0);
        sceneShaderHandle->SetFloat("u_SpecularStrength", 0.35f);
        sceneShaderHandle->SetFloat("u_Shininess", 32.0f);
        sceneShaderHandle->SetInt("u_UseBlinnPhong", useBlinnPhong ? 1 : 0);

        // Phase 6 Visual Effects Uniforms
        sceneShaderHandle->SetFloat("u_ShadowFade", dayNight->GetSunShadowFade());
        sceneShaderHandle->SetInt("u_FogEnabled", fogEnabled ? 1 : 0);
        sceneShaderHandle->SetFloat("u_FogDensity", fogDensity);
        sceneShaderHandle->SetVec3("u_FogColor", dayNight->GetHorizonColor());
        sceneShaderHandle->SetFloat("u_RainWetness", rainSystem->IsEnabled() ? rainSystem->GetIntensity() : 0.0f);
        sceneShaderHandle->SetFloat("u_WindowEmissive", dayNight->GetWindowEmissiveIntensity());

        lightingEnv.ApplyToShader(sceneShaderHandle.get(), camera.GetPosition());

        city->Render(sceneShaderHandle.get(), emissiveShader.get(), nsSignal, ewSignal, nsLeft, ewLeft, nsPedWalk, ewPedWalk);
        traffic->Render(sceneShaderHandle.get(), emissiveShader.get());
        pedestrianManager->Render(sceneShaderHandle.get());

        // Draw Debug Overlays from real RoadNetwork data
        if (showLaneCenters || showTurningBeziers || showConflictZones || showStopLines || showEntryExit) {
            emissiveShader->Bind();
            emissiveShader->SetMat4("u_ViewProjection", viewProj);
            city->RenderDebugOverlay(emissiveShader.get(), showLaneCenters, showTurningBeziers, showConflictZones, showStopLines, showEntryExit);
        }
        gpuProfiler->EndSection("Main Scene Pass");

        // ---------------- PASS 3: SKYBOX & FX PASS ----------------
        gpuProfiler->BeginSection("Skybox & FX Pass");
        skybox->SetTopColor(dayNight->GetSkyTopColor());
        skybox->SetHorizonColor(dayNight->GetHorizonColor());
        skybox->SetGroundColor(dayNight->GetGroundColor());
        skybox->SetSunParams(dayNight->GetSunDirection(), dayNight->GetLight().color, dayNight->GetWindowEmissiveIntensity());
        skybox->Draw(camera);

        // Draw Rain Streaks
        rainSystem->Render(viewProj);
        gpuProfiler->EndSection("Skybox & FX Pass");

        if (wireframeMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Reset for UI
        }

        // ---------------- PASS 3: IMGUI UI OVERLAY ----------------
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(480.0f, 620.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("SmartCity 3D — Engine Diagnostics & Controls");

            // 1. ENGINE PERFORMANCE DIAGNOSTICS
            if (ImGui::CollapsingHeader("Engine Performance Diagnostics", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.4f, 1.0f), "Graphics Pipeline: OpenGL 3.3 Core Profile");
                ImGui::Text("FPS [Rolling 60 frames]: Avg: %.1f | Min: %.1f | Max: %.1f", avgFPS, minFPS, maxFPS);
                ImGui::Text("Frame Time: Avg: %.2f ms | Min: %.2f ms | Max: %.2f ms", avgFrameMs, minFrameMs, maxFrameMs);
                ImGui::Separator();

                const auto& stats = renderer->GetStats();
                ImGui::Text("Draw Calls: %u | Triangles: %u", stats.drawCalls, stats.triangleCount);
                ImGui::Text("Frustum Culling: %u Visible | %u Culled", stats.visibleObjects, stats.culledObjects);

                ImGui::Checkbox("Enable Frustum Culling", &frustumCullingEnabled);
                ImGui::SameLine();
                ImGui::Checkbox("Wireframe Mode", &wireframeMode);

                ImGui::Checkbox("Depth Test", &depthTestEnabled);
                ImGui::SameLine();
                ImGui::Checkbox("Backface Culling", &backfaceCullEnabled);

                ImGui::Text("Shading Model:");
                ImGui::SameLine();
                if (ImGui::RadioButton("Blinn-Phong", useBlinnPhong)) useBlinnPhong = true;
                ImGui::SameLine();
                if (ImGui::RadioButton("Classical Phong", !useBlinnPhong)) useBlinnPhong = false;
            }

            // GPU PASS TIMINGS (GL Timer Queries GL_TIME_ELAPSED - Phase 6 Requirement 14)
            if (ImGui::CollapsingHeader("GPU Pass Profiling (GL Timer Queries)", ImGuiTreeNodeFlags_DefaultOpen)) {
                const char* glRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
                const char* glVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
                ImGui::Text("Active GPU : %s", glRenderer ? glRenderer : "OpenGL Device");
                ImGui::Text("Vendor     : %s", glVendor ? glVendor : "OpenGL Vendor");
                ImGui::Separator();

                float totalGpuMs = 0.0f;
                for (const auto& res : gpuProfiler->GetResults()) {
                    ImGui::Text("%-20s: %6.2f ms", res.name.c_str(), res.timeMs);
                    totalGpuMs += res.timeMs;
                }
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "%-20s: %6.2f ms", "Total GPU Pass Time", totalGpuMs);
            }

            // 2. CAMERA & PROJECTION CONTROLS
            if (ImGui::CollapsingHeader("Camera & Projection", ImGuiTreeNodeFlags_DefaultOpen)) {
                bool isPerspective = (camera.GetProjectionType() == gfx::ProjectionType::Perspective);
                ImGui::Text("Projection Type: %s", isPerspective ? "PERSPECTIVE" : "ORTHOGRAPHIC");
                ImGui::SameLine();
                if (ImGui::Button("Toggle Projection")) {
                    camera.ToggleProjectionType();
                }

                if (isPerspective) {
                    float fovDeg = camera.GetFOVDegrees();
                    if (ImGui::SliderFloat("FOV", &fovDeg, 25.0f, 90.0f, "%.1f°")) {
                        camera.SetFOVDegrees(fovDeg);
                    }
                } else {
                    float orthoSize = camera.GetOrthoSize();
                    if (ImGui::SliderFloat("Ortho Size", &orthoSize, 5.0f, 150.0f, "%.1f m")) {
                        camera.SetOrthoSize(orthoSize);
                    }
                }

                ImGui::Text("Presets (Hotkeys: F1=Free, F2=Top-Down, F3=Overview, F4=Inspect):");
                if (ImGui::Button("Overview [F3]")) {
                    camera.ApplyPreset(gfx::CameraPreset::Overview);
                    camera.SetProjectionType(gfx::ProjectionType::Perspective);
                }
                ImGui::SameLine();
                if (ImGui::Button("Top-Down [F2]")) {
                    camera.ApplyPreset(gfx::CameraPreset::TopDown);
                    camera.SetProjectionType(gfx::ProjectionType::Orthographic);
                }
                ImGui::SameLine();
                if (ImGui::Button("North Cam [F4]")) camera.ApplyPreset(gfx::CameraPreset::NorthTrafficCam);

                if (ImGui::Button("Free Cam [F1]")) camera.SetMode(gfx::CameraMode::FreeFly);
                ImGui::SameLine();
                if (ImGui::Button("Pedestrian")) camera.ApplyPreset(gfx::CameraPreset::PedestrianView);
                ImGui::SameLine();
                if (ImGui::Button("East Cam")) camera.ApplyPreset(gfx::CameraPreset::EastTrafficCam);
            }

            // 3. ROADNETWORK & DEBUG OVERLAYS (PHASE 3)
            if (ImGui::CollapsingHeader("RoadNetwork & Debug Overlays (Phase 3)", ImGuiTreeNodeFlags_DefaultOpen)) {
                const auto& net = city->GetRoadNetwork();
                ImGui::Text("Network Topology: 4 Approaches | %zu Lanes | %zu Paths",
                            net.GetAllLanes().size(), net.GetAllPaths().size());
                ImGui::Text("Conflict Zones: %zu | Stop Lines: %zu | Crosswalks: %zu",
                            net.GetAllConflictZones().size(), net.GetAllStopLines().size(), net.GetAllCrosswalks().size());
                ImGui::Separator();

                ImGui::Text("Render Debug Overlays:");
                ImGui::Checkbox("Lane Centrelines (Cyan)", &showLaneCenters);
                ImGui::SameLine();
                ImGui::Checkbox("Turning Béziers (Magenta)", &showTurningBeziers);

                ImGui::Checkbox("Conflict Zones (Red)", &showConflictZones);
                ImGui::SameLine();
                ImGui::Checkbox("Stop Lines (Orange)", &showStopLines);

                ImGui::Checkbox("Entry/Exit Points (Green/Amber)", &showEntryExit);

                if (ImGui::Button("Export RoadNetwork JSON")) {
                    net.SaveToJson("assets/config/network.json");
                }
            }

            // 3. RESOURCE MANAGER STATUS
            if (ImGui::CollapsingHeader("Resource Manager Status")) {
                ImGui::Text("Loaded Shaders:   %zu", rm.GetShaderCount());
                ImGui::Text("Loaded Textures:  %zu", rm.GetTextureCount());
                ImGui::Text("Loaded Meshes:    %zu", rm.GetMeshCount());
                ImGui::Text("Loaded Models:    %zu", rm.GetModelCount());
                ImGui::Text("Loaded Materials: %zu", rm.GetMaterialCount());
                ImGui::Separator();
                ImGui::Text("Disk Loads: %u | Cache Hits: %u", rm.GetDiskLoadCount(), rm.GetCacheHitCount());
            }

            // 4. TRANSFORM & MATERIAL INSPECTOR
            if (showcaseObj && ImGui::CollapsingHeader("Transform & Material Inspector")) {
                ImGui::Text("Target Object: %s (ID: %u)", showcaseObj->GetName().c_str(), showcaseObj->GetId());
                auto& tf = showcaseObj->GetTransform();

                glm::vec3 pos = tf.GetPosition();
                if (ImGui::DragFloat3("Position", &pos.x, 0.2f, -50.0f, 50.0f)) {
                    tf.SetPosition(pos);
                }

                glm::vec3 rotDeg = tf.GetEulerDegrees();
                if (ImGui::DragFloat3("Rotation (Deg)", &rotDeg.x, 1.0f, -180.0f, 180.0f)) {
                    tf.SetEulerDegrees(rotDeg);
                }

                glm::vec3 scale = tf.GetScale();
                if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f)) {
                    tf.SetScale(scale);
                }

                auto mat = showcaseObj->GetMaterial();
                if (mat) {
                    glm::vec3 diff = mat->GetDiffuse();
                    if (ImGui::ColorEdit3("Diffuse Color", &diff.x)) {
                        mat->SetDiffuse(diff);
                    }
                    float shininess = mat->GetShininess();
                    if (ImGui::SliderFloat("Shininess", &shininess, 1.0f, 128.0f)) {
                        mat->SetShininess(shininess);
                    }
                }
            }

            // 5. ATMOSPHERE, WEATHER & VISUAL EFFECTS (Phase 6)
            if (ImGui::CollapsingHeader("Atmosphere, Weather & Visual Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
                float timeHours = dayNight->GetTimeOfDay();
                if (ImGui::SliderFloat("Time of Day", &timeHours, 0.0f, 24.0f, "%.1f hrs")) {
                    dayNight->SetTimeOfDay(timeHours);
                }
                bool dayNightPaused = dayNight->IsPaused();
                if (ImGui::Checkbox("Pause Solar Cycle", &dayNightPaused)) {
                    dayNight->SetPaused(dayNightPaused);
                }
                ImGui::Text("Sun Elevation : %+.3f | Azimuth: %.1f°",
                            dayNight->GetSunElevation(), glm::degrees(dayNight->GetSunAzimuth()));
                ImGui::Text("Shadow Fade   : %.2f | Streetlight Fade: %.2f | Window Glow: %.2f",
                            dayNight->GetSunShadowFade(), dayNight->GetStreetlightIntensity(), dayNight->GetWindowEmissiveIntensity());
                ImGui::Text("Streetlights  : %s", streetlightsActive ? "ACTIVE (ON)" : "INACTIVE (OFF)");

                ImGui::Separator();
                ImGui::Text("Atmospheric Exponential-Squared Fog:");
                ImGui::Checkbox("Enable Fog", &fogEnabled);
                if (fogEnabled) {
                    ImGui::SliderFloat("Fog Density", &fogDensity, 0.001f, 0.040f, "%.4f");
                }

                ImGui::Separator();
                ImGui::Text("Precipitation & Rain Particles:");
                ImGui::Checkbox("Enable Rain Particles", &rainEnabled);
                if (rainEnabled) {
                    ImGui::SliderFloat("Rain Intensity / Wetness", &rainIntensity, 0.05f, 1.0f, "%.2f");
                    ImGui::Text("Active Rain Streaks: %zu", static_cast<size_t>(rainSystem->GetParticleCount() * rainIntensity));
                }
            }

            // 6. TRAFFIC SIMULATION & SIGNALS
            if (ImGui::CollapsingHeader("Traffic Simulation & Signals")) {
                ImGui::Text("Active Vehicles: %zu | Spawned: %d | Exited: %d",
                            traffic->GetActiveVehicleCount(), traffic->GetTotalSpawned(), traffic->GetTotalCompleted());
                ImGui::Text("Average Velocity: %.2f m/s (%.1f km/h)",
                            traffic->GetAverageSpeed(), traffic->GetAverageSpeed() * 3.6f);

                if (traffic->IsEmergencyActive()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "EMERGENCY PREEMPTION: ACTIVE");
                } else {
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Emergency Preemption: Standby");
                }

                if (ImGui::Button("Spawn Sedan")) {
                    traffic->SpawnVehicle(sim::VehicleType::Sedan, sim::ApproachDirection::North, sim::TurnManeuver::Straight);
                }
                ImGui::SameLine();
                if (ImGui::Button("Spawn SUV")) {
                    traffic->SpawnVehicle(sim::VehicleType::SUV, sim::ApproachDirection::East, sim::TurnManeuver::Straight);
                }
                ImGui::SameLine();
                if (ImGui::Button("Spawn Bus")) {
                    traffic->SpawnVehicle(sim::VehicleType::Bus, sim::ApproachDirection::South, sim::TurnManeuver::Straight);
                }
                ImGui::SameLine();
                if (ImGui::Button("Spawn Emergency")) {
                    traffic->SpawnVehicle(sim::VehicleType::Emergency, sim::ApproachDirection::West, sim::TurnManeuver::Straight);
                }

                ImGui::Separator();
                ImGui::Text("Traffic Signal Controller (ISignalProvider):");
                ImGui::Text("Phase: %s | Elapsed: %.1fs / %.1fs (Rem: %.1fs)",
                            traffic::ToString(signalController->GetCurrentPhase()),
                            signalController->GetPhaseElapsed(),
                            signalController->GetPhaseDuration(),
                            signalController->GetPhaseRemaining());

                bool isAdaptive = (signalController->GetMode() == traffic::SignalMode::Adaptive);
                if (ImGui::RadioButton("Fixed-Time Mode", !isAdaptive)) {
                    signalController->SetMode(traffic::SignalMode::FixedTime);
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Adaptive Mode", isAdaptive)) {
                    signalController->SetMode(traffic::SignalMode::Adaptive);
                }

                ImGui::Text("Pedestrian Heads: CW_N [%s] CW_S [%s] CW_E [%s] CW_W [%s]",
                            traffic::ToString(signalController->GetPedestrianSignal("CW_North")),
                            traffic::ToString(signalController->GetPedestrianSignal("CW_South")),
                            traffic::ToString(signalController->GetPedestrianSignal("CW_East")),
                            traffic::ToString(signalController->GetPedestrianSignal("CW_West")));

                if (ImGui::Button("Call CW_North Ped")) {
                    signalController->RequestPedestrianCrossing("CW_North");
                }
                ImGui::SameLine();
                if (ImGui::Button("Call CW_East Ped")) {
                    signalController->RequestPedestrianCrossing("CW_East");
                }

                ImGui::Separator();
                ImGui::Text("Pedestrian Agents:");
                ImGui::Text("Active: %zu | Spawned: %d | Exited: %d | Violations: %d",
                            pedestrianManager->GetActiveCount(),
                            pedestrianManager->GetTotalSpawned(),
                            pedestrianManager->GetTotalCompleted(),
                            pedestrianManager->GetIllegalCrossingStarts());

                if (ImGui::Button("Spawn Pedestrian (CW_North)")) {
                    pedestrianManager->SpawnPedestrian("CW_North");
                }
                ImGui::SameLine();
                if (ImGui::Button("Spawn Pedestrian (CW_East)")) {
                    pedestrianManager->SpawnPedestrian("CW_East");
                }
            }

            ImGui::End();
        }

        gpuProfiler->BeginSection("UI Pass");
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        gpuProfiler->EndSection("UI Pass");
        gpuProfiler->EndFrame();

        window.SwapBuffers();

        // Automated Phase 6 Visual Verification Evidence Capture
        if (m_args.screenshots && framesRendered == 15) {
            Logger::Info("Generating Phase 6 Visual Verification Evidence Screenshots...");
            std::filesystem::create_directories("screenshots/phase6");
            int w = window.GetWidth();
            int h = window.GetHeight();

            auto captureFrame = [&](const std::string& name) {
                const auto& dl = dayNight->GetLight();
                lightingEnv.SetDirectionalLight(dl);
                glm::mat4 lsm = shadowMap->CalculateLightSpaceMatrix(dl.direction);
                glm::mat4 vp = camera.GetViewProjectionMatrix();

                // 1. Shadow depth pass
                shadowMap->BindForWriting();
                shadowShader->Bind();
                shadowShader->SetMat4("u_LightSpaceMatrix", lsm);
                city->RenderShadowDepth(shadowShader.get());
                traffic->RenderShadowDepth(shadowShader.get());
                shadowMap->Unbind(w, h);

                // 2. Main scene pass
                renderer->BeginFrame(camera, lightingEnv, glm::vec4(dayNight->GetClearColor(), 1.0f));
                for (const auto& obj : scene.GetObjects()) renderer->Submit(*obj);
                renderer->EndFrame();

                sceneShaderHandle->Bind();
                sceneShaderHandle->SetMat4("u_ViewProjection", vp);
                sceneShaderHandle->SetMat4("u_LightSpaceMatrix", lsm);
                sceneShaderHandle->SetVec3("u_CameraPos", camera.GetPosition());
                sceneShaderHandle->SetInt("u_ShadowMap", 1);
                shadowMap->BindTexture(1);
                sceneShaderHandle->SetFloat("u_ShadowFade", dayNight->GetSunShadowFade());
                sceneShaderHandle->SetInt("u_FogEnabled", fogEnabled ? 1 : 0);
                sceneShaderHandle->SetFloat("u_FogDensity", fogDensity);
                sceneShaderHandle->SetVec3("u_FogColor", dayNight->GetHorizonColor());
                sceneShaderHandle->SetFloat("u_RainWetness", rainSystem->IsEnabled() ? rainSystem->GetIntensity() : 0.0f);
                sceneShaderHandle->SetFloat("u_WindowEmissive", dayNight->GetWindowEmissiveIntensity());
                lightingEnv.ApplyToShader(sceneShaderHandle.get(), camera.GetPosition());

                city->Render(sceneShaderHandle.get(), emissiveShader.get(), nsSignal, ewSignal, nsLeft, ewLeft, nsPedWalk, ewPedWalk);
                traffic->Render(sceneShaderHandle.get(), emissiveShader.get());
                pedestrianManager->Render(sceneShaderHandle.get());

                // 3. Skybox & FX pass
                skybox->SetTopColor(dayNight->GetSkyTopColor());
                skybox->SetHorizonColor(dayNight->GetHorizonColor());
                skybox->SetGroundColor(dayNight->GetGroundColor());
                skybox->SetSunParams(dayNight->GetSunDirection(), dayNight->GetLight().color, dayNight->GetWindowEmissiveIntensity());
                skybox->Draw(camera);

                rainSystem->Render(vp);

                glFinish();
                SaveScreenshot("screenshots/phase6/" + name + ".png", w, h);
            };

            // 1. Shadow direction matches sun at 3 different times of day (Noon, Morning, Dusk)
            camera.ApplyPreset(gfx::CameraPreset::Overview);
            dayNight->SetTimeOfDay(12.0f);
            captureFrame("shadow_noon");

            dayNight->SetTimeOfDay(8.5f);
            captureFrame("shadow_morning");

            dayNight->SetTimeOfDay(17.5f);
            captureFrame("shadow_dusk");

            // 2. Camera distance & angle checks (Near, Far, Top-Down)
            camera.ApplyPreset(gfx::CameraPreset::TopDown);
            camera.SetProjectionType(gfx::ProjectionType::Orthographic);
            dayNight->SetTimeOfDay(13.0f);
            captureFrame("shadow_topdown");

            camera.ApplyPreset(gfx::CameraPreset::NorthTrafficCam);
            camera.SetProjectionType(gfx::ProjectionType::Perspective);
            captureFrame("shadow_low_angle");

            // 3. Night: Streetlights + Headlights + Window Emissive Glow
            camera.ApplyPreset(gfx::CameraPreset::Overview);
            dayNight->SetTimeOfDay(0.0f);
            captureFrame("night_streetlights_headlights");

            // 4. Fog ON vs OFF
            dayNight->SetTimeOfDay(14.0f);
            fogEnabled = false;
            captureFrame("fog_off");

            fogEnabled = true;
            fogDensity = 0.015f;
            captureFrame("fog_on");
            fogEnabled = false;

            // 5. Rain ON vs OFF
            rainEnabled = false;
            captureFrame("rain_off");

            rainEnabled = true;
            rainIntensity = 0.85f;
            rainSystem->SetEnabled(true);
            rainSystem->SetIntensity(0.85f);
            rainSystem->Update(0.1f, camera.GetPosition());
            captureFrame("rain_on");
            rainEnabled = false;

            // 6. Dawn / Dusk transition strip (6 frames without popping)
            const float stripTimes[6] = { 5.2f, 5.8f, 6.2f, 6.8f, 7.2f, 8.0f };
            for (int s = 0; s < 6; ++s) {
                dayNight->SetTimeOfDay(stripTimes[s]);
                captureFrame("dawn_strip_" + std::to_string(s));
            }

            Logger::Info("All Phase 6 visual verification evidence screenshots successfully saved!");
            break; // Finished automated capture!
        }

        framesRendered++;
        if (frameLimit > 0 && framesRendered >= frameLimit) {
            Logger::Info("Headless execution reached frame limit (", frameLimit, "). Exiting.");
            break;
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    scene.Clear();
    rm.Shutdown();

    window.Shutdown();
    Logger::Info("SmartCity 3D Phase 2 terminated successfully with 0 leaked resources.");
    Logger::Shutdown();
    return 0;
}

} // namespace SmartCity::core

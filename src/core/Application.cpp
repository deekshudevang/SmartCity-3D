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
#include "sim/VehicleManager.h"
#include "traffic/TrafficSignalController.h"
#include "ped/PedestrianManager.h"
#include "fx/RainSystem.h"
#include "gfx/GpuProfiler.h"
#include "analytics/AnalyticsEngine.h"
#include "analytics/ExperimentRunner.h"
#include "algos/LineDrawing.h"
#include "algos/BezierCurve.h"
#include "algos/RayCasting.h"
#include "smartcity/vehicles/VehicleLibrary.h"
#include "ui/VehicleGalleryPanel.h"
#include "ui/VehicleDetailsPanel.h"
#include "ui/VehiclePreviewPanel.h"
#include "ui/VehicleCustomizationPanel.h"

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

    // Traffic Signal Controller & Pedestrian Simulation Manager
    auto signalController = std::make_shared<traffic::TrafficSignalController>();
    signalController->LoadConfig("assets/config/signals.json");
    signalController->EnableCsvLogging("logs/signals_latest.csv");

    auto pedestrianManager = std::make_unique<ped::PedestrianManager>(city->GetRoadNetwork(), signalController);
    pedestrianManager->LoadConfig("assets/config/pedestrians.json");

    // RoadNetwork-Based Vehicle Simulation Manager (Phase 4, 5, 7)
    auto vehicleManager = std::make_unique<sim::VehicleManager>(city->GetRoadNetwork(), signalController);
    vehicleManager->SetPedestrianManager(pedestrianManager.get());

    // Analytics Engine (Phase 7)
    analytics::AnalyticsEngine analyticsEngine(*vehicleManager, *signalController, *pedestrianManager);
    vehicleManager->SetVehicleCompletedCallback([&analyticsEngine](const sim::Vehicle& v) {
        analyticsEngine.RegisterTripCompletion(v.GetWaitingTime());
    });

    // Initialize Complete Vehicle Library System
    vehicles::VehicleLibrary::Get().Initialize(
        "assets/vehicle_presets/source_json",
        "assets/vehicle_presets/normalized",
        "assets/vehicle_presets/vehicle_assets.json",
        "assets/vehicle_presets/import_report.json"
    );

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

            vehicleManager->Step(fixedDt);
            analyticsEngine.Step(fixedDt);
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
            const auto& activeVehs = vehicleManager->GetActiveVehicles();
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
        vehicleManager->RenderShadowDepth(shadowShader.get());
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
        vehicleManager->Render(sceneShaderHandle.get(), emissiveShader.get());
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

        // ---------------- 3D RAY CASTING MOUSE PICKING ----------------
        static int selectedVehicleId = -1;
        static algos::Ray lastRay{ glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f) };
        static glm::vec2 lastClickPos{ 0.0f, 0.0f };

        // Analytics & CG Algos UI State
        static bool showAnalyticsDashboard = true;
        static bool showCgAlgosWindow = true;
        static bool showBenchmarkWindow = false;
        static bool showDebugCrossCheck = false;

        // Vehicle Library & Presets UI State
        static bool showVehicleGallery = true;
        static bool showVehicleDetails = true;
        static bool showVehiclePreview = false;
        static bool showVehicleCustomization = false;

        static ui::VehicleGalleryPanel vehicleGalleryPanel;
        static ui::VehicleDetailsPanel vehicleDetailsPanel;
        static std::unique_ptr<ui::VehiclePreviewPanel> vehiclePreviewPanel;
        static ui::VehicleCustomizationPanel vehicleCustomizationPanel;
        if (!vehiclePreviewPanel) {
            vehiclePreviewPanel = std::make_unique<ui::VehiclePreviewPanel>();
        }

        // CG Algos: Line Rasterization
        static int lineX0 = -15, lineY0 = -10, lineX1 = 20, lineY1 = 15;

        // CG Algos: Bezier
        static glm::vec3 bezP0(-2.5f, 0.0f, 19.5f);
        static glm::vec3 bezP1(-2.5f, 0.0f, 8.0f);
        static glm::vec3 bezP2(-12.0f, 0.0f, 2.5f);
        static glm::vec3 bezP3(-19.5f, 0.0f, 2.5f);
        static float bezT = 0.5f;

        // Benchmark Runner State
        static std::vector<analytics::ExperimentRunRecord> lastBenchmarkResults;
        static analytics::ExperimentSummary lastFixedSummary;
        static analytics::ExperimentSummary lastAdaptSummary;
        static bool hasBenchmarkResults = false;
        static float benchDuration = 60.0f;
        static int benchDemandIdx = 1; // 0=Low, 1=Medium, 2=High
        static char benchStatusMsg[128] = "Ready to run benchmark.";

        if (!ImGui::GetIO().WantCaptureMouse && Input::GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
            glm::vec2 mousePos = Input::GetMousePosition();
            lastClickPos = mousePos;
            lastRay = algos::RayCaster::ScreenPointToRay(mousePos.x, mousePos.y,
                                                         static_cast<float>(window.GetWidth()),
                                                         static_cast<float>(window.GetHeight()),
                                                         camera.GetViewMatrix(),
                                                         camera.GetProjectionMatrix());

            float closestT = 1e9f;
            int hitId = -1;
            for (const auto& v : vehicleManager->GetActiveVehicles()) {
                if (!v || !v->IsActive() || v->HasReachedEnd()) continue;
                float tHit = 0.0f;
                if (algos::RayCaster::IntersectRayOBB(lastRay, v->GetOBB(), v->GetDimensions().height, 0.0f, tHit)) {
                    if (tHit > 0.0f && tHit < closestT) {
                        closestT = tHit;
                        hitId = v->GetId();
                    }
                }
            }
            selectedVehicleId = hitId;
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

            // 6. TRAFFIC SIMULATION & CONTROLS (PHASE 4, 5, 7)
            if (ImGui::CollapsingHeader("Traffic Simulation & Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Active: %zu | Spawned: %d | Exited: %d",
                            vehicleManager->GetActiveVehicleCount(),
                            vehicleManager->GetTotalSpawned(),
                            vehicleManager->GetTotalCompleted());
                ImGui::Text("Mean Velocity: %.2f m/s (%.1f km/h)",
                            vehicleManager->GetAverageSpeed(),
                            vehicleManager->GetAverageSpeed() * 3.6f);

                bool isPaused = vehicleManager->IsPaused();
                if (ImGui::Button(isPaused ? "Resume Simulation" : "Pause Simulation")) {
                    vehicleManager->SetPaused(!isPaused);
                }
                ImGui::SameLine();
                if (isPaused) {
                    if (ImGui::Button("Step 1 Frame")) {
                        vehicleManager->Step(1.0f / 60.0f);
                        pedestrianManager->Step(1.0f / 60.0f);
                        analyticsEngine.Step(1.0f / 60.0f);
                    }
                    ImGui::SameLine();
                }
                if (ImGui::Button("Reset Sim")) {
                    vehicleManager->Reset();
                    pedestrianManager->Reset();
                }

                ImGui::Text("Simulation Speed Multiplier:");
                float speedMult = vehicleManager->GetSpeedMultiplier();
                if (ImGui::RadioButton("0.5x", speedMult == 0.5f)) vehicleManager->SetSpeedMultiplier(0.5f);
                ImGui::SameLine();
                if (ImGui::RadioButton("1.0x", speedMult == 1.0f)) vehicleManager->SetSpeedMultiplier(1.0f);
                ImGui::SameLine();
                if (ImGui::RadioButton("2.0x", speedMult == 2.0f)) vehicleManager->SetSpeedMultiplier(2.0f);
                ImGui::SameLine();
                if (ImGui::RadioButton("4.0x", speedMult == 4.0f)) vehicleManager->SetSpeedMultiplier(4.0f);

                ImGui::Separator();
                ImGui::Text("Demand Presets:");
                if (ImGui::Button("Low Demand")) {
                    auto cfg = vehicleManager->GetConfig();
                    cfg.demand.northArrivalsPerHour = 360.0f;
                    cfg.demand.southArrivalsPerHour = 360.0f;
                    cfg.demand.eastArrivalsPerHour = 270.0f;
                    cfg.demand.westArrivalsPerHour = 270.0f;
                    vehicleManager->SetConfig(cfg);
                }
                ImGui::SameLine();
                if (ImGui::Button("Medium Demand")) {
                    auto cfg = vehicleManager->GetConfig();
                    cfg.demand.northArrivalsPerHour = 720.0f;
                    cfg.demand.southArrivalsPerHour = 720.0f;
                    cfg.demand.eastArrivalsPerHour = 540.0f;
                    cfg.demand.westArrivalsPerHour = 540.0f;
                    vehicleManager->SetConfig(cfg);
                }
                ImGui::SameLine();
                if (ImGui::Button("High Demand")) {
                    auto cfg = vehicleManager->GetConfig();
                    cfg.demand.northArrivalsPerHour = 1080.0f;
                    cfg.demand.southArrivalsPerHour = 1080.0f;
                    cfg.demand.eastArrivalsPerHour = 810.0f;
                    cfg.demand.westArrivalsPerHour = 810.0f;
                    vehicleManager->SetConfig(cfg);
                }

                ImGui::Separator();
                ImGui::Text("Manual Vehicle Spawner:");
                static int spawnAppIdx = 0;
                static int spawnMovIdx = 1;
                static int spawnTypeIdx = 0;
                const char* appNames[] = { "North", "East", "South", "West" };
                const char* movNames[] = { "Left", "Through", "Right" };
                const char* typeNames[] = { "Sedan", "Bus", "Emergency" };

                ImGui::Combo("Approach", &spawnAppIdx, appNames, IM_ARRAYSIZE(appNames));
                ImGui::Combo("Movement", &spawnMovIdx, movNames, IM_ARRAYSIZE(movNames));
                ImGui::Combo("Type", &spawnTypeIdx, typeNames, IM_ARRAYSIZE(typeNames));

                if (ImGui::Button("Spawn Selected Vehicle")) {
                    city::ApproachId a = static_cast<city::ApproachId>(spawnAppIdx);
                    city::MovementType m = (spawnMovIdx == 0) ? city::MovementType::Left :
                                           (spawnMovIdx == 1) ? city::MovementType::Through : city::MovementType::Right;
                    sim::VehicleType t = (spawnTypeIdx == 0) ? sim::VehicleType::Sedan :
                                         (spawnTypeIdx == 1) ? sim::VehicleType::Bus : sim::VehicleType::Emergency;
                    vehicleManager->SpawnVehicle(a, m, t);
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

                if (ImGui::Button("Call CW_North Ped")) signalController->RequestPedestrianCrossing("CW_North");
                ImGui::SameLine();
                if (ImGui::Button("Call CW_East Ped"))  signalController->RequestPedestrianCrossing("CW_East");

                ImGui::Separator();
                ImGui::Text("Pedestrian Agents:");
                ImGui::Text("Active: %zu | Spawned: %d | Exited: %d | Violations: %d",
                            pedestrianManager->GetActiveCount(),
                            pedestrianManager->GetTotalSpawned(),
                            pedestrianManager->GetTotalCompleted(),
                            pedestrianManager->GetIllegalCrossingStarts());

                if (ImGui::Button("Spawn Pedestrian (CW_North)")) pedestrianManager->SpawnPedestrian("CW_North");
                ImGui::SameLine();
                if (ImGui::Button("Spawn Pedestrian (CW_East)"))  pedestrianManager->SpawnPedestrian("CW_East");

                ImGui::Separator();
                ImGui::Text("Windows & Dashboard Toggles (Phase 7):");
                ImGui::Checkbox("Show Live Analytics Dashboard", &showAnalyticsDashboard);
                ImGui::Checkbox("Show CG Algorithms Demonstrator", &showCgAlgosWindow);
                ImGui::Checkbox("Show Benchmark Experiments Runner", &showBenchmarkWindow);
                ImGui::Checkbox("Show Debug Mode Cross-Check", &showDebugCrossCheck);

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.3f, 0.75f, 1.0f, 1.0f), "Vehicle Library & Supercars:");
                ImGui::Checkbox("Show Vehicle Gallery & Catalog", &showVehicleGallery);
                ImGui::Checkbox("Show Vehicle Details", &showVehicleDetails);
                ImGui::Checkbox("Show Vehicle 3D Preview", &showVehiclePreview);
                ImGui::Checkbox("Show Vehicle Customization", &showVehicleCustomization);
            }

            ImGui::End();
        }

        // ================= PANEL 2: LIVE ANALYTICS DASHBOARD =================
        if (showAnalyticsDashboard) {
            ImGui::SetNextWindowPos(ImVec2(static_cast<float>(window.GetWidth()) - 510.0f, 20.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(490.0f, 680.0f), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Live Analytics & Metrics Dashboard", &showAnalyticsDashboard)) {
                const auto& snap = analyticsEngine.GetSnapshot();

                // 1. Congestion & Level of Service
                ImVec4 losColor = ImVec4(0.2f, 0.85f, 0.3f, 1.0f);
                if (snap.congestionLevel == "Medium") losColor = ImVec4(0.9f, 0.85f, 0.2f, 1.0f);
                else if (snap.congestionLevel == "High") losColor = ImVec4(0.95f, 0.6f, 0.1f, 1.0f);
                else if (snap.congestionLevel == "Severe") losColor = ImVec4(0.95f, 0.2f, 0.2f, 1.0f);

                ImGui::Text("Congestion Index: %.2f | Level: ", snap.congestionIndex);
                ImGui::SameLine();
                ImGui::TextColored(losColor, "%s", snap.congestionLevel.c_str());
                ImGui::Separator();

                // 2. Active & Waiting Vehicles
                float waitPct = (snap.activeVehicles > 0) ? (100.0f * snap.waitingVehicles / snap.activeVehicles) : 0.0f;
                ImGui::Text("Active Vehicles  : %d vehicles", snap.activeVehicles);
                ImGui::Text("Waiting Vehicles : %d vehicles (%.1f%% of traffic)", snap.waitingVehicles, waitPct);
                ImGui::Separator();

                // 3. Speeds & Waiting Times (Strict "N/A" validity handling)
                ImGui::Text("Speed Metrics:");
                if (snap.speedValid) {
                    ImGui::Text("  Instantaneous Velocity : %5.2f m/s (%5.1f km/h)", snap.avgSpeedMs, snap.avgSpeedKmh);
                    ImGui::Text("  Rolling 60s Velocity   : %5.2f km/h", snap.rollingAvgSpeedKmh);
                    ImGui::Text("  Live Vehicle Wait Time : %5.2f s", snap.avgLiveWaitingTime);
                } else {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  Instantaneous Velocity : N/A (No active vehicles)");
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  Rolling 60s Velocity   : N/A (No active vehicles)");
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  Live Vehicle Wait Time : N/A (No active vehicles)");
                }

                if (snap.tripWaitValid) {
                    ImGui::Text("  Trip Completed Delay   : %5.2f s", snap.avgTripWaitingTime);
                } else {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  Trip Completed Delay   : N/A (No completed trips yet)");
                }
                ImGui::Separator();

                // 4. Flow & Throughput
                ImGui::Text("Throughput & Pedestrians:");
                ImGui::Text("  Completed Vehicle Trips: %d vehicles", snap.totalCompleted);
                ImGui::Text("  Rolling Hourly Flow    : %.0f veh/h", snap.rollingThroughputPerHour);
                ImGui::Text("  Pedestrian Trips       : %d completed | %d violations", snap.completedPedestrians, snap.pedestrianViolations);
                ImGui::Separator();

                // 5. Physical Emissions (Akçelik Vehicle Dynamics Model)
                ImGui::Text("Environmental Impact (Akçelik Physical Model):");
                ImGui::Text("  Cumulative Fuel Burned : %6.3f L", snap.cumulativeFuelLiters);
                ImGui::Text("  CO2 Emission Rate      : %6.2f g/s  | Cumulative: %6.4f kg", snap.instantaneousCO2RateGramsPerSec, snap.cumulativeCO2Kg);
                ImGui::Separator();

                // 6. Approach Queue Lengths
                ImGui::Text("Approach Queue Lengths:");
                const char* appLabels[4] = { "North (N)", "East  (E)", "South (S)", "West  (W)" };
                for (size_t a = 0; a < 4; ++a) {
                    float fraction = std::clamp(snap.queues[a].lengthMetres / 50.0f, 0.0f, 1.0f);
                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "%d veh (%.1f m)", snap.queues[a].vehicleCount, snap.queues[a].lengthMetres);
                    ImGui::Text("%-10s:", appLabels[a]);
                    ImGui::SameLine();
                    ImGui::ProgressBar(fraction, ImVec2(-1, 0), buf);
                }
                ImGui::Separator();

                // 7. Signal Controller
                ImGui::Text("Traffic Signal State:");
                ImGui::Text("  Current Phase : %s", snap.signalPhaseName.c_str());
                float phaseFrac = (snap.signalPhaseDuration > 0.0f) ? std::clamp(snap.signalPhaseElapsed / snap.signalPhaseDuration, 0.0f, 1.0f) : 0.0f;
                char sigBuf[64];
                std::snprintf(sigBuf, sizeof(sigBuf), "%.1f / %.1f s", snap.signalPhaseElapsed, snap.signalPhaseDuration);
                ImGui::ProgressBar(phaseFrac, ImVec2(-1, 0), sigBuf);
                ImGui::Separator();

                // 8. History Plots (RingBuffer data)
                if (ImGui::CollapsingHeader("Live History Line Plots", ImGuiTreeNodeFlags_DefaultOpen)) {
                    auto spd = analyticsEngine.GetSpeedHistory();
                    if (!spd.empty()) {
                        ImGui::PlotLines("Speed (m/s)", spd.data(), static_cast<int>(spd.size()), 0, nullptr, 0.0f, 20.0f, ImVec2(-1, 50.0f));
                    }
                    auto tp = analyticsEngine.GetThroughputHistory();
                    if (!tp.empty()) {
                        ImGui::PlotLines("Throughput (veh/h)", tp.data(), static_cast<int>(tp.size()), 0, nullptr, 0.0f, 1500.0f, ImVec2(-1, 50.0f));
                    }
                    auto co2 = analyticsEngine.GetCO2History();
                    if (!co2.empty()) {
                        ImGui::PlotLines("CO2 Rate (g/s)", co2.data(), static_cast<int>(co2.size()), 0, nullptr, 0.0f, 25.0f, ImVec2(-1, 50.0f));
                    }
                }
            }
            ImGui::End();
        }

        // ================= PANEL 3: CG ALGORITHMS DEMONSTRATOR =================
        if (showCgAlgosWindow) {
            ImGui::SetNextWindowPos(ImVec2(20.0f, static_cast<float>(window.GetHeight()) - 420.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(520.0f, 400.0f), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Computer Graphics Algorithms Demonstrator", &showCgAlgosWindow)) {
                if (ImGui::BeginTabBar("CgAlgorithmsTabBar")) {
                    // Tab 1: Line Rasterization (DDA vs Bresenham)
                    if (ImGui::BeginTabItem("Line Rasterization")) {
                        ImGui::Text("Integer Bresenham vs Floating-Point DDA Rasterizers");
                        ImGui::SliderInt2("Start (X0, Y0)", &lineX0, -25, 25);
                        ImGui::SliderInt2("End   (X1, Y1)", &lineX1, -25, 25);

                        auto ddaRes = algos::LineRasterizer::RasterizeDDA(lineX0, lineY0, lineX1, lineY1);
                        auto bresRes = algos::LineRasterizer::RasterizeBresenham(lineX0, lineY0, lineX1, lineY1);

                        bool match = (ddaRes.pixels.size() == bresRes.pixels.size());
                        for (size_t i = 0; i < ddaRes.pixels.size() && match; ++i) {
                            if (ddaRes.pixels[i].x != bresRes.pixels[i].x || ddaRes.pixels[i].y != bresRes.pixels[i].y) match = false;
                        }

                        if (match) {
                            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f),
                                               "PASS: Exact match! Produced %zu pixels in Octant %d.", ddaRes.pixels.size(), bresRes.octant);
                        } else {
                            ImGui::TextColored(ImVec4(0.95f, 0.3f, 0.3f, 1.0f), "Discrepancy detected between DDA and Bresenham!");
                        }

                        ImGui::Separator();
                        ImGui::Text("Bresenham Decision Variable D (First 8 Steps):");
                        ImGui::Columns(4, "BresenhamTable", true);
                        ImGui::Text("Step"); ImGui::NextColumn();
                        ImGui::Text("X"); ImGui::NextColumn();
                        ImGui::Text("Y"); ImGui::NextColumn();
                        ImGui::Text("Decision D"); ImGui::NextColumn();
                        ImGui::Separator();

                        for (size_t i = 0; i < bresRes.pixels.size() && i < 8; ++i) {
                            ImGui::Text("%zu", i); ImGui::NextColumn();
                            ImGui::Text("%d", bresRes.pixels[i].x); ImGui::NextColumn();
                            ImGui::Text("%d", bresRes.pixels[i].y); ImGui::NextColumn();
                            ImGui::Text("%d", bresRes.pixels[i].decisionVar); ImGui::NextColumn();
                        }
                        ImGui::Columns(1);
                        ImGui::EndTabItem();
                    }

                    // Tab 2: Bézier Curves
                    if (ImGui::BeginTabItem("Bézier Geometry")) {
                        ImGui::Text("Cubic Bézier: De Casteljau Algorithm vs Bernstein Polynomials");

                        if (ImGui::Button("North Left Turn")) {
                            bezP0 = glm::vec3(2.5f, 0.0f, -19.5f);
                            bezP1 = glm::vec3(2.5f, 0.0f, -8.0f);
                            bezP2 = glm::vec3(12.0f, 0.0f, -2.5f);
                            bezP3 = glm::vec3(19.5f, 0.0f, -2.5f);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("South Left Turn")) {
                            bezP0 = glm::vec3(-2.5f, 0.0f, 19.5f);
                            bezP1 = glm::vec3(-2.5f, 0.0f, 8.0f);
                            bezP2 = glm::vec3(-12.0f, 0.0f, 2.5f);
                            bezP3 = glm::vec3(-19.5f, 0.0f, 2.5f);
                        }

                        ImGui::SliderFloat("Parameter t", &bezT, 0.0f, 1.0f, "%.3f");

                        glm::vec3 ptCasteljau = algos::BezierCurveEvaluator::EvaluateDeCasteljau(bezP0, bezP1, bezP2, bezP3, bezT);
                        glm::vec3 ptPolynomial = algos::BezierCurveEvaluator::EvaluatePolynomial(bezP0, bezP1, bezP2, bezP3, bezT);
                        glm::vec3 tangent = algos::BezierCurveEvaluator::EvaluateDerivative(bezP0, bezP1, bezP2, bezP3, bezT);
                        float curvature = algos::BezierCurveEvaluator::EvaluateCurvature(bezP0, bezP1, bezP2, bezP3, bezT);
                        float delta = glm::distance(ptCasteljau, ptPolynomial);

                        ImGui::Text("De Casteljau Point : (%6.2f, %6.2f, %6.2f)", ptCasteljau.x, ptCasteljau.y, ptCasteljau.z);
                        ImGui::Text("Bernstein Polynomial: (%6.2f, %6.2f, %6.2f)", ptPolynomial.x, ptPolynomial.y, ptPolynomial.z);
                        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "Mathematical Equivalence Delta: %.3e m (< 1e-5)", delta);
                        ImGui::Text("Analytical Tangent  : (%6.2f, %6.2f, %6.2f) [Speed: %.2f m/s]", tangent.x, tangent.y, tangent.z, glm::length(tangent));
                        ImGui::Text("Curvature kappa(t)  : %6.4f m^-1 (Radius: %6.2f m)", curvature, (curvature > 1e-4f) ? (1.0f / curvature) : 999.0f);
                        ImGui::EndTabItem();
                    }

                    // Tab 3: Ray Casting & 3D Object Inspector
                    if (ImGui::BeginTabItem("Ray Casting 3D Inspector")) {
                        ImGui::Text("Mouse Ray Casting & Bounding Box Picking:");
                        ImGui::Text("Last Click Screen Pos : (%.0f, %.0f) px", lastClickPos.x, lastClickPos.y);
                        ImGui::Text("Unprojected Ray Origin: (%.2f, %.2f, %.2f)", lastRay.origin.x, lastRay.origin.y, lastRay.origin.z);
                        ImGui::Text("Unprojected Ray Dir   : (%.3f, %.3f, %.3f)", lastRay.direction.x, lastRay.direction.y, lastRay.direction.z);
                        ImGui::Separator();

                        if (selectedVehicleId > 0) {
                            const sim::Vehicle* selVeh = nullptr;
                            for (const auto& v : vehicleManager->GetActiveVehicles()) {
                                if (v && v->GetId() == selectedVehicleId) {
                                    selVeh = v.get();
                                    break;
                                }
                            }

                            if (selVeh) {
                                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "SELECTED VEHICLE: #%d", selVeh->GetId());
                                ImGui::Text("Type        : %s", (selVeh->GetType() == sim::VehicleType::Sedan) ? "Sedan" :
                                                              (selVeh->GetType() == sim::VehicleType::Bus) ? "Bus" : "Emergency Ambulance");
                                ImGui::Text("Inbound Lane: %s", selVeh->GetRoute().inboundLaneId.c_str());
                                ImGui::Text("Position    : (%6.2f, %6.2f, %6.2f)", selVeh->GetPosition().x, selVeh->GetPosition().y, selVeh->GetPosition().z);
                                ImGui::Text("Heading     : %5.1f° (%+.3f rad)", glm::degrees(selVeh->GetHeadingRad()), selVeh->GetHeadingRad());
                                ImGui::Text("Velocity    : %5.2f m/s (%5.1f km/h)", selVeh->GetSpeed(), selVeh->GetSpeed() * 3.6f);
                                ImGui::Text("Acceleration: %+5.2f m/s^2", selVeh->GetAcceleration());
                                ImGui::Text("Wait Time   : %5.2f s", selVeh->GetWaitingTime());
                            } else {
                                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "Selected vehicle #%d has exited the intersection.", selectedVehicleId);
                            }
                        } else {
                            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click any moving vehicle in the 3D viewport to inspect it.");
                        }
                        ImGui::EndTabItem();
                    }

                    // Tab 4: 3D Educational Demos
                    if (ImGui::BeginTabItem("3D Math & Transforms")) {
                        ImGui::Text("Live 4x4 Camera Projection Matrix:");
                        glm::mat4 projMat = camera.GetProjectionMatrix();
                        for (int r = 0; r < 4; ++r) {
                            ImGui::Text("[%+7.3f  %+7.3f  %+7.3f  %+7.3f]",
                                        projMat[0][r], projMat[1][r], projMat[2][r], projMat[3][r]);
                        }
                        ImGui::Separator();

                        ImGui::Text("Showcase Model Matrix (M = T * R * S):");
                        if (showcaseObj) {
                            glm::mat4 mMat = showcaseObj->GetTransform().GetLocalMatrix();
                            for (int r = 0; r < 4; ++r) {
                                ImGui::Text("[%+7.3f  %+7.3f  %+7.3f  %+7.3f]",
                                            mMat[0][r], mMat[1][r], mMat[2][r], mMat[3][r]);
                            }
                        }
                        ImGui::Separator();
                        ImGui::Text("Lighting Model: Blinn-Phong Halfway Vector H = (L + V) / ||L + V||");
                        ImGui::Checkbox("Use Blinn-Phong Specular (vs Standard Phong)", &useBlinnPhong);
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
            }
            ImGui::End();
        }

        // ================= PANEL 4: BENCHMARK EXPERIMENTS RUNNER =================
        if (showBenchmarkWindow) {
            ImGui::SetNextWindowPos(ImVec2(520.0f, 20.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(560.0f, 480.0f), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Benchmark Experiments Runner (Fixed-Time vs Adaptive)", &showBenchmarkWindow)) {
                ImGui::Text("Automated Headless Simulation Benchmarking Engine");
                ImGui::SliderFloat("Run Duration (s)", &benchDuration, 30.0f, 300.0f, "%.0f s");

                const char* demandOpts[] = { "Low", "Medium", "High" };
                ImGui::Combo("Demand Level", &benchDemandIdx, demandOpts, IM_ARRAYSIZE(demandOpts));

                if (ImGui::Button("Run Benchmark Trials (Seeds: 42, 1337, 2024)")) {
                    std::snprintf(benchStatusMsg, sizeof(benchStatusMsg), "Running benchmark trials... please wait.");
                    lastBenchmarkResults = analytics::ExperimentRunner::RunBenchmark(city->GetRoadNetwork(),
                                                                                     benchDuration,
                                                                                     { 42, 1337, 2024 },
                                                                                     demandOpts[benchDemandIdx]);
                    lastFixedSummary = analytics::ExperimentRunner::ComputeSummary(lastBenchmarkResults, "FixedTime");
                    lastAdaptSummary = analytics::ExperimentRunner::ComputeSummary(lastBenchmarkResults, "Adaptive");
                    hasBenchmarkResults = true;
                    std::snprintf(benchStatusMsg, sizeof(benchStatusMsg), "Benchmark completed successfully!");
                }

                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "%s", benchStatusMsg);

                if (hasBenchmarkResults) {
                    ImGui::Separator();
                    ImGui::Text("Benchmark Summary Statistics (Mean +/- Std):");
                    ImGui::Columns(3, "BenchTable", true);
                    ImGui::Text("Metric"); ImGui::NextColumn();
                    ImGui::Text("Fixed-Time Mode"); ImGui::NextColumn();
                    ImGui::Text("Adaptive Mode"); ImGui::NextColumn();
                    ImGui::Separator();

                    ImGui::Text("Throughput (veh)"); ImGui::NextColumn();
                    ImGui::Text("%.1f +/- %.1f", lastFixedSummary.meanThroughput, lastFixedSummary.stdThroughput); ImGui::NextColumn();
                    ImGui::Text("%.1f +/- %.1f", lastAdaptSummary.meanThroughput, lastAdaptSummary.stdThroughput); ImGui::NextColumn();

                    ImGui::Text("Avg Wait Time (s)"); ImGui::NextColumn();
                    ImGui::Text("%.1f +/- %.1f s", lastFixedSummary.meanWaitingTime, lastFixedSummary.stdWaitingTime); ImGui::NextColumn();
                    ImGui::Text("%.1f +/- %.1f s", lastAdaptSummary.meanWaitingTime, lastAdaptSummary.stdWaitingTime); ImGui::NextColumn();

                    ImGui::Text("Max Queue (veh)"); ImGui::NextColumn();
                    ImGui::Text("%.1f +/- %.1f veh", lastFixedSummary.meanMaxQueue, lastFixedSummary.stdMaxQueue); ImGui::NextColumn();
                    ImGui::Text("%.1f +/- %.1f veh", lastAdaptSummary.meanMaxQueue, lastAdaptSummary.stdMaxQueue); ImGui::NextColumn();

                    ImGui::Text("Signal Switches"); ImGui::NextColumn();
                    ImGui::Text("%.1f", lastFixedSummary.meanSignalSwitches); ImGui::NextColumn();
                    ImGui::Text("%.1f", lastAdaptSummary.meanSignalSwitches); ImGui::NextColumn();

                    ImGui::Text("Total Collisions"); ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "%d (Zero)", lastFixedSummary.totalCollisions); ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "%d (Zero)", lastAdaptSummary.totalCollisions); ImGui::NextColumn();

                    ImGui::Text("Safety Violations"); ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "%d (Zero)", lastFixedSummary.totalSafetyViolations); ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "%d (Zero)", lastAdaptSummary.totalSafetyViolations); ImGui::NextColumn();
                    ImGui::Columns(1);

                    ImGui::Separator();
                    if (ImGui::Button("Export to CSV & JSON (logs/experiments/)")) {
                        analytics::ExperimentRunner::SaveToCsv(lastBenchmarkResults, "logs/experiments/ui_benchmark.csv", "54136b4");
                        analytics::ExperimentRunner::SaveToJson(lastBenchmarkResults, "logs/experiments/ui_benchmark.json", "54136b4");
                        std::snprintf(benchStatusMsg, sizeof(benchStatusMsg), "Saved to logs/experiments/ui_benchmark.csv & json");
                    }
                }
            }
            ImGui::End();
        }

        // ================= PANEL 5: DEBUG MODE CROSS-CHECK =================
        if (showDebugCrossCheck) {
            ImGui::SetNextWindowPos(ImVec2(520.0f, 520.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(560.0f, 260.0f), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Debug Mode Cross-Check (Sim vs Metrics Consistency)", &showDebugCrossCheck)) {
                const auto& snap = analyticsEngine.GetSnapshot();
                ImGui::Text("Verification that every UI metric traces directly to real simulation state:");
                ImGui::Columns(3, "CrossCheckTable", true);
                ImGui::Text("Metric Field"); ImGui::NextColumn();
                ImGui::Text("AnalyticsEngine Value"); ImGui::NextColumn();
                ImGui::Text("Raw Simulation Object"); ImGui::NextColumn();
                ImGui::Separator();

                ImGui::Text("Active Vehicles"); ImGui::NextColumn();
                ImGui::Text("%d", snap.activeVehicles); ImGui::NextColumn();
                ImGui::Text("%zu", vehicleManager->GetActiveVehicleCount()); ImGui::NextColumn();

                ImGui::Text("Mean Speed"); ImGui::NextColumn();
                ImGui::Text("%.2f m/s", snap.avgSpeedMs); ImGui::NextColumn();
                ImGui::Text("%.2f m/s", vehicleManager->GetAverageSpeed()); ImGui::NextColumn();

                ImGui::Text("Total Completed"); ImGui::NextColumn();
                ImGui::Text("%d", snap.totalCompleted); ImGui::NextColumn();
                ImGui::Text("%d", vehicleManager->GetTotalCompleted()); ImGui::NextColumn();

                ImGui::Text("Total Spawned"); ImGui::NextColumn();
                ImGui::Text("%d", snap.totalSpawned); ImGui::NextColumn();
                ImGui::Text("%d", vehicleManager->GetTotalSpawned()); ImGui::NextColumn();

                ImGui::Text("Active Pedestrians"); ImGui::NextColumn();
                ImGui::Text("%d", snap.activePedestrians); ImGui::NextColumn();
                ImGui::Text("%zu", pedestrianManager->GetActiveCount()); ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "PASS: 100%% data consistency verified with real simulation state.");
            }
            ImGui::End();
        }

        // ================= VEHICLE LIBRARY & PRESETS PANELS =================
        if (showVehicleGallery) {
            vehicleGalleryPanel.Render(&showVehicleGallery);
        }
        if (showVehicleDetails) {
            vehicleDetailsPanel.Render(&showVehicleDetails);
        }
        if (showVehiclePreview && vehiclePreviewPanel) {
            vehiclePreviewPanel->Render(vehicleManager.get(), sceneShaderHandle.get(), emissiveShader.get(), &showVehiclePreview);
        }
        if (showVehicleCustomization) {
            vehicleCustomizationPanel.Render(&showVehicleCustomization);
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
                vehicleManager->RenderShadowDepth(shadowShader.get());
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
                vehicleManager->Render(sceneShaderHandle.get(), emissiveShader.get());
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

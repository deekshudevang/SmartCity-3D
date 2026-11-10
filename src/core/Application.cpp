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

#include "city/CityEnvironment.h"
#include "sim/TrafficManager.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

namespace SmartCity::core {

Application::Application(const AppCommandLineArgs& args)
    : m_args(args)
    , m_running(false)
{
}

Application::~Application() {
}

int Application::Run() {
    Logger::Init("logs/smartcity.log");
    Logger::Info("==================================================================");
    Logger::Info(" SmartCity 3D — Intelligent Traffic Intersection Simulation");
    Logger::Info(" Phase 4: Vehicle Simulation, IDM Dynamics & Lane Navigation");
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

    // Camera setup
    gfx::Camera camera(glm::radians(45.0f), window.GetAspectRatio(), 0.1f, 1000.0f);
    window.SetResizeCallback([&camera](int w, int h) {
        camera.SetViewportSize(w, h);
    });

    // Shaders
    auto sceneShader = std::make_unique<gfx::Shader>("shaders/scene.vert", "shaders/scene.frag", true);
    auto shadowShader = std::make_unique<gfx::Shader>("shaders/shadow_depth.vert", "shaders/shadow_depth.frag", true);
    auto emissiveShader = std::make_unique<gfx::Shader>("shaders/light_indicator.vert", "shaders/light_indicator.frag", true);

    auto shadowMap = std::make_unique<gfx::ShadowMap>(2048, 2048);
    auto dayNight = std::make_unique<gfx::DayNightCycle>(14.0f);

    // City Environment (Intersection geometry, lane markings, buildings, poles)
    auto city = std::make_unique<city::CityEnvironment>(interSettings);

    // Traffic Simulation Manager (Lane graph, multi-class vehicles, IDM dynamics, conflicts)
    auto traffic = std::make_unique<sim::TrafficManager>(simSettings, interSettings);

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

    Logger::Info("Entering main graphics & simulation loop...");
    m_running = true;

    while (!window.ShouldClose() && m_running) {
        window.PollEvents();
        timer.Update();

        float dt = static_cast<float>(timer.GetFrameDeltaTime());

        // Update window title with FPS and frame ms
        static float titleUpdateTimer = 0.0f;
        titleUpdateTimer += dt;
        if (titleUpdateTimer >= 0.25f) {
            titleUpdateTimer = 0.0f;
            char titleBuf[128];
            std::snprintf(titleBuf, sizeof(titleBuf), "SmartCity 3D — %.1f FPS (%.2f ms) | Vehicles: %zu",
                          timer.GetFPS(), timer.GetFrameDeltaTime() * 1000.0, traffic->GetActiveVehicleCount());
            window.SetTitle(titleBuf);
        }

        if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) {
            camera.Update(dt);
        }

        if (Input::GetKeyDown(GLFW_KEY_ESCAPE)) {
            window.SetShouldClose(true);
        }

        // Fixed-timestep simulation updates
        while (timer.ShouldSimulateStep()) {
            float fixedDt = static_cast<float>(timer.GetFixedDeltaTime());
            dayNight->Update(fixedDt);
            traffic->Update(fixedDt, nsSignal, ewSignal, nsLeft, ewLeft);
            timer.ConsumeSimulateStep();
        }

        // Matrices
        const auto& dirLight = dayNight->GetLight();
        glm::mat4 lightSpaceMatrix = shadowMap->CalculateLightSpaceMatrix(dirLight.direction);
        glm::mat4 viewProj = camera.GetViewProjectionMatrix();

        // ---------------- PASS 1: SHADOW DEPTH PASS ----------------
        shadowMap->BindForWriting();
        shadowShader->Bind();
        shadowShader->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);
        city->RenderShadowDepth(shadowShader.get());
        traffic->RenderShadowDepth(shadowShader.get());
        shadowMap->Unbind(window.GetWidth(), window.GetHeight());

        // ---------------- PASS 2: MAIN SCENE RENDER PASS ----------------
        glm::vec3 clearColor = dayNight->GetClearColor();
        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneShader->Bind();
        sceneShader->SetMat4("u_ViewProjection", viewProj);
        sceneShader->SetMat4("u_LightSpaceMatrix", lightSpaceMatrix);
        sceneShader->SetVec3("u_CameraPos", camera.GetPosition());

        sceneShader->SetVec3("u_DirLight.direction", dirLight.direction);
        sceneShader->SetVec3("u_DirLight.color", dirLight.color);
        sceneShader->SetFloat("u_DirLight.intensity", dirLight.intensity);
        sceneShader->SetVec3("u_DirLight.ambient", dirLight.ambient);

        bool streetlightsActive = dayNight->AreStreetlightsActive();
        const auto& streetlights = city->GetStreetlights();
        sceneShader->SetInt("u_NumPointLights", streetlightsActive ? static_cast<int>(streetlights.size()) : 0);
        for (size_t i = 0; i < streetlights.size() && i < 16; ++i) {
            std::string prefix = "u_PointLights[" + std::to_string(i) + "].";
            sceneShader->SetVec3(prefix + "position", streetlights[i].position);
            sceneShader->SetVec3(prefix + "color", streetlights[i].color);
            sceneShader->SetFloat(prefix + "intensity", streetlights[i].intensity);
            sceneShader->SetFloat(prefix + "radius", streetlights[i].radius);
        }

        sceneShader->SetInt("u_ShadowMap", 1);
        shadowMap->BindTexture(1);
        sceneShader->SetInt("u_HasTexture", 0);
        sceneShader->SetFloat("u_SpecularStrength", 0.35f);
        sceneShader->SetFloat("u_Shininess", 32.0f);

        // Render City Environment
        city->Render(sceneShader.get(), emissiveShader.get(), nsSignal, ewSignal, nsLeft, ewLeft, nsPedWalk, ewPedWalk);
        // Render 3D Vehicles & Lights
        traffic->Render(sceneShader.get(), emissiveShader.get());

        // ---------------- PASS 3: IMGUI UI OVERLAY ----------------
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(440.0f, 420.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("SmartCity 3D — Urban Environment Control");

            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.4f, 1.0f), "Urban Infrastructure: OPERATIONAL");
            ImGui::Text("FPS: %.1f (%.2f ms/frame)", timer.GetFPS(), timer.GetFrameDeltaTime() * 1000.0);
            ImGui::Separator();

            ImGui::Text("Camera Viewpoints:");
            if (ImGui::Button("Overview (45°)")) camera.ApplyPreset(gfx::CameraPreset::Overview);
            ImGui::SameLine();
            if (ImGui::Button("Top-Down")) camera.ApplyPreset(gfx::CameraPreset::TopDown);
            ImGui::SameLine();
            if (ImGui::Button("North Cam")) camera.ApplyPreset(gfx::CameraPreset::NorthTrafficCam);

            if (ImGui::Button("Pedestrian View")) camera.ApplyPreset(gfx::CameraPreset::PedestrianView);
            ImGui::SameLine();
            if (ImGui::Button("East Cam")) camera.ApplyPreset(gfx::CameraPreset::EastTrafficCam);

            float fovDeg = camera.GetFOVDegrees();
            if (ImGui::SliderFloat("FOV", &fovDeg, 25.0f, 90.0f, "%.1f°")) {
                camera.SetFOVDegrees(fovDeg);
            }

            ImGui::Separator();
            ImGui::Text("Atmosphere & Lighting:");
            float timeHours = dayNight->GetTimeOfDay();
            if (ImGui::SliderFloat("Time of Day", &timeHours, 0.0f, 24.0f, "%.1f hrs")) {
                dayNight->SetTimeOfDay(timeHours);
            }
            bool dayNightPaused = dayNight->IsPaused();
            if (ImGui::Checkbox("Pause Sun Cycle", &dayNightPaused)) {
                dayNight->SetPaused(dayNightPaused);
            }
            ImGui::Text("Streetlights: %s", streetlightsActive ? "ACTIVE (ON)" : "INACTIVE (OFF)");

            ImGui::Separator();
            ImGui::Text("Traffic Simulation & IDM Dynamics:");
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
            ImGui::Text("Signal Head Test Overrides:");
            const char* signalNames[] = { "RED", "YELLOW", "GREEN" };
            ImGui::Combo("N-S Signal", &nsSignal, signalNames, 3);
            ImGui::Combo("E-W Signal", &ewSignal, signalNames, 3);
            ImGui::Checkbox("N-S Pedestrian Walk", &nsPedWalk);
            ImGui::SameLine();
            ImGui::Checkbox("E-W Pedestrian Walk", &ewPedWalk);

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.SwapBuffers();

        framesRendered++;
        if (frameLimit > 0 && framesRendered >= frameLimit) {
            Logger::Info("Headless execution reached frame limit (", frameLimit, "). Exiting.");
            break;
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    window.Shutdown();
    Logger::Info("SmartCity 3D Phase 4 terminated successfully.");
    Logger::Shutdown();
    return 0;
}

} // namespace SmartCity::core

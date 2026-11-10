#include <iostream>
#include "core/Logger.h"
#include "core/Window.h"
#include "core/Time.h"
#include "core/Input.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

int main(int argc, char* argv[]) {
    SmartCity::Logger::Init();
    SmartCity::Logger::Info("==================================================================");
    SmartCity::Logger::Info(" SmartCity 3D — Intelligent Traffic Intersection Simulation");
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

    // Apply clean modern dark theme (Unslop design standard)
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window.GetNativeWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    SmartCity::Time timer(1.0 / 60.0); // 60Hz fixed simulation timestep

    SmartCity::Logger::Info("Initialization complete. Entering main simulation loop...");

    int frameLimit = headless ? 60 : -1;
    int framesRendered = 0;

    while (!window.ShouldClose()) {
        window.PollEvents();
        timer.Update();

        // Check for exit hotkey (Escape)
        if (SmartCity::Input::GetKeyDown(GLFW_KEY_ESCAPE)) {
            window.SetShouldClose(true);
        }

        // Fixed-timestep simulation update loop
        while (timer.ShouldSimulateStep()) {
            // Simulation logic updates at fixed 60Hz dt
            timer.ConsumeSimulateStep();
        }

        // Render frame
        glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Phase 1 System Diagnostics Window
        {
            ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(400.0f, 260.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("SmartCity 3D — System Diagnostics");

            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.4f, 1.0f), "System Status: OPERATIONAL");
            ImGui::Separator();
            ImGui::Text("OpenGL: 3.3 Core Profile");
            ImGui::Text("GLSL:   #version 330");
            ImGui::Text("Resolution: %d x %d", window.GetWidth(), window.GetHeight());
            ImGui::Text("FPS: %.1f (%.2f ms/frame)", timer.GetFPS(), timer.GetFrameDeltaTime() * 1000.0);
            ImGui::Text("Sim Ticks: %llu", static_cast<unsigned long long>(timer.GetTickCount()));
            ImGui::Text("Sim Time:  %.2f s", timer.GetTotalSimTime());

            ImGui::Separator();
            bool paused = timer.IsPaused();
            if (ImGui::Checkbox("Pause Simulation", &paused)) {
                timer.SetPaused(paused);
            }

            float speed = static_cast<float>(timer.GetTimeScale());
            if (ImGui::SliderFloat("Sim Speed", &speed, 0.1f, 4.0f, "%.1fx")) {
                timer.SetTimeScale(speed);
            }

            ImGui::End();
        }

        // Render Dear ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.SwapBuffers();

        framesRendered++;
        if (frameLimit > 0 && framesRendered >= frameLimit) {
            SmartCity::Logger::Info("Headless execution reached frame limit (", frameLimit, "). Exiting.");
            break;
        }
    }

    // Cleanup Dear ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    window.Shutdown();
    SmartCity::Logger::Info("SmartCity 3D terminated successfully.");
    return 0;
}

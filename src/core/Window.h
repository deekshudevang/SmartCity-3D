#pragma once

#include <string>
#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace SmartCity::core {

struct WindowProps {
    std::string title = "SmartCity 3D — Intelligent Traffic Intersection";
    int width = 1600;
    int height = 900;
    bool vsync = true;
    bool headless = false;
};

class Window {
public:
    Window(const WindowProps& props = WindowProps());
    ~Window();

    bool Init();
    void Shutdown();

    void PollEvents();
    void SwapBuffers();
    bool ShouldClose() const;
    void SetShouldClose(bool close);

    int GetWidth() const { return m_props.width; }
    int GetHeight() const { return m_props.height; }
    float GetAspectRatio() const { return static_cast<float>(m_props.width) / static_cast<float>(m_props.height); }
    GLFWwindow* GetNativeWindow() const { return m_window; }

    void SetVSync(bool enabled);
    bool IsVSync() const { return m_props.vsync; }

    void SetTitle(const std::string& title);

    using ResizeCallback = std::function<void(int width, int height)>;
    void SetResizeCallback(ResizeCallback callback) { m_resizeCallback = callback; }

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

    WindowProps m_props;
    GLFWwindow* m_window;
    ResizeCallback m_resizeCallback;
};

} // namespace SmartCity::core

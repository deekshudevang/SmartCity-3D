#include "core/Window.h"
#include "core/Logger.h"
#include "core/Input.h"

namespace SmartCity {

static void GLFWErrorCallback(int error, const char* description) {
    Logger::Error("GLFW Error (", error, "): ", description);
}

Window::Window(const WindowProps& props)
    : m_props(props)
    , m_window(nullptr)
{
}

Window::~Window() {
    Shutdown();
}

bool Window::Init() {
    glfwSetErrorCallback(GLFWErrorCallback);

    if (!glfwInit()) {
        Logger::Error("Failed to initialize GLFW!");
        return false;
    }

    // Configure OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    if (m_props.headless) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    m_window = glfwCreateWindow(m_props.width, m_props.height, m_props.title.c_str(), nullptr, nullptr);
    if (!m_window) {
        Logger::Error("Failed to create GLFW window!");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);

    // Initialize GLAD function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Logger::Error("Failed to initialize GLAD / OpenGL 3.3 function pointers!");
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = nullptr;
        return false;
    }

    Logger::Info("OpenGL Renderer: ", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    Logger::Info("OpenGL Vendor:   ", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    Logger::Info("OpenGL Version:  ", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    Logger::Info("GLSL Version:    ", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

    // Store 'this' pointer in window user pointer for callbacks
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);

    // Initialize Input system with this window
    Input::Init(m_window);

    // Configure initial viewport
    glViewport(0, 0, m_props.width, m_props.height);

    // Configure VSync
    SetVSync(m_props.vsync);

    // Enable Depth Testing by default
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable blending for transparent markings / antialiasing
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

void Window::Shutdown() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
        Logger::Info("Window and GLFW context terminated cleanly.");
    }
}

void Window::PollEvents() {
    glfwPollEvents();
}

void Window::SwapBuffers() {
    if (m_window) {
        glfwSwapBuffers(m_window);
        Input::EndFrame();
    }
}

bool Window::ShouldClose() const {
    if (!m_window) return true;
    return glfwWindowShouldClose(m_window);
}

void Window::SetShouldClose(bool close) {
    if (m_window) {
        glfwSetWindowShouldClose(m_window, close ? GLFW_TRUE : GLFW_FALSE);
    }
}

void Window::SetVSync(bool enabled) {
    m_props.vsync = enabled;
    glfwSwapInterval(enabled ? 1 : 0);
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->m_props.width = width;
        self->m_props.height = height;
        glViewport(0, 0, width, height);
        if (self->m_resizeCallback) {
            self->m_resizeCallback(width, height);
        }
    }
}

} // namespace SmartCity

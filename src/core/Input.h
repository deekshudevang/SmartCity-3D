#pragma once

#include <unordered_map>
#include <glm/glm.hpp>

struct GLFWwindow;

namespace SmartCity {

class Input {
public:
    static void Init(GLFWwindow* window);
    static void EndFrame();

    // Keyboard queries
    static bool GetKey(int keycode);        // True if currently held down
    static bool GetKeyDown(int keycode);    // True only on the frame pressed
    static bool GetKeyUp(int keycode);      // True only on the frame released

    // Mouse queries
    static bool GetMouseButton(int button);
    static bool GetMouseButtonDown(int button);
    static bool GetMouseButtonUp(int button);

    static glm::vec2 GetMousePosition();
    static glm::vec2 GetMouseDelta();
    static float GetMouseScrollDelta();

    // Callbacks from GLFW
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
    static inline GLFWwindow* s_window = nullptr;

    static inline bool s_keysCurrent[512] = { false };
    static inline bool s_keysPrevious[512] = { false };

    static inline bool s_mouseButtonsCurrent[8] = { false };
    static inline bool s_mouseButtonsPrevious[8] = { false };

    static inline glm::vec2 s_mousePosition{ 0.0f };
    static inline glm::vec2 s_mouseDelta{ 0.0f };
    static inline float s_scrollDelta{ 0.0f };
    static inline bool s_firstMouseMove = true;
};

} // namespace SmartCity

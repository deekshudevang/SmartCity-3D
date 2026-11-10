#include "core/Input.h"
#include <GLFW/glfw3.h>
#include <cstring>

namespace SmartCity {

void Input::Init(GLFWwindow* window) {
    s_window = window;
    std::memset(s_keysCurrent, 0, sizeof(s_keysCurrent));
    std::memset(s_keysPrevious, 0, sizeof(s_keysPrevious));
    std::memset(s_mouseButtonsCurrent, 0, sizeof(s_mouseButtonsCurrent));
    std::memset(s_mouseButtonsPrevious, 0, sizeof(s_mouseButtonsPrevious));
    s_mousePosition = glm::vec2(0.0f);
    s_mouseDelta = glm::vec2(0.0f);
    s_scrollDelta = 0.0f;
    s_firstMouseMove = true;

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);
}

void Input::EndFrame() {
    std::memcpy(s_keysPrevious, s_keysCurrent, sizeof(s_keysCurrent));
    std::memcpy(s_mouseButtonsPrevious, s_mouseButtonsCurrent, sizeof(s_mouseButtonsCurrent));
    s_mouseDelta = glm::vec2(0.0f);
    s_scrollDelta = 0.0f;
}

bool Input::GetKey(int keycode) {
    if (keycode >= 0 && keycode < 512) {
        return s_keysCurrent[keycode];
    }
    return false;
}

bool Input::GetKeyDown(int keycode) {
    if (keycode >= 0 && keycode < 512) {
        return s_keysCurrent[keycode] && !s_keysPrevious[keycode];
    }
    return false;
}

bool Input::GetKeyUp(int keycode) {
    if (keycode >= 0 && keycode < 512) {
        return !s_keysCurrent[keycode] && s_keysPrevious[keycode];
    }
    return false;
}

bool Input::GetMouseButton(int button) {
    if (button >= 0 && button < 8) {
        return s_mouseButtonsCurrent[button];
    }
    return false;
}

bool Input::GetMouseButtonDown(int button) {
    if (button >= 0 && button < 8) {
        return s_mouseButtonsCurrent[button] && !s_mouseButtonsPrevious[button];
    }
    return false;
}

bool Input::GetMouseButtonUp(int button) {
    if (button >= 0 && button < 8) {
        return !s_mouseButtonsCurrent[button] && s_mouseButtonsPrevious[button];
    }
    return false;
}

glm::vec2 Input::GetMousePosition() {
    return s_mousePosition;
}

glm::vec2 Input::GetMouseDelta() {
    return s_mouseDelta;
}

float Input::GetMouseScrollDelta() {
    return s_scrollDelta;
}

void Input::KeyCallback(GLFWwindow*, int key, int, int action, int) {
    if (key >= 0 && key < 512) {
        if (action == GLFW_PRESS) {
            s_keysCurrent[key] = true;
        } else if (action == GLFW_RELEASE) {
            s_keysCurrent[key] = false;
        }
    }
}

void Input::CursorPosCallback(GLFWwindow*, double xpos, double ypos) {
    glm::vec2 newPos(static_cast<float>(xpos), static_cast<float>(ypos));
    if (s_firstMouseMove) {
        s_mousePosition = newPos;
        s_firstMouseMove = false;
    }
    s_mouseDelta += newPos - s_mousePosition;
    s_mousePosition = newPos;
}

void Input::MouseButtonCallback(GLFWwindow*, int button, int action, int) {
    if (button >= 0 && button < 8) {
        if (action == GLFW_PRESS) {
            s_mouseButtonsCurrent[button] = true;
        } else if (action == GLFW_RELEASE) {
            s_mouseButtonsCurrent[button] = false;
        }
    }
}

void Input::ScrollCallback(GLFWwindow*, double, double yoffset) {
    s_scrollDelta += static_cast<float>(yoffset);
}

} // namespace SmartCity

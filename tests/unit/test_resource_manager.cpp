#include "scene/ResourceManager.h"
#include "core/Logger.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <iostream>

using namespace SmartCity::scene;
using namespace SmartCity::core;

int main() {
    Logger::Init("logs/test_resource_manager.log");
    Logger::Info("Running unit test_resource_manager...");

    // Initialize headless GLFW context so real OpenGL textures and meshes can be allocated
    if (!glfwInit()) {
        Logger::Error("Failed to initialize GLFW for headless test.");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(64, 64, "HeadlessTest", nullptr, nullptr);
    if (!window) {
        Logger::Error("Failed to create headless GLFW window.");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Logger::Error("Failed to load GLAD.");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    auto& resMgr = ResourceManager::Get();

    // 1. First Load of Model: Must read from disk
    uint32_t initialDiskLoads = resMgr.GetDiskLoadCount();
    uint32_t initialCacheHits = resMgr.GetCacheHitCount();

    auto model1 = resMgr.GetModel("assets/models/test_cube.obj");
    assert(model1 != nullptr);
    assert(model1->GetMeshes().size() > 0);
    assert(resMgr.GetDiskLoadCount() == initialDiskLoads + 1);
    assert(resMgr.GetCacheHitCount() == initialCacheHits);
    Logger::Info("[PASS] Model loaded from disk on first request.");

    // 2. Second Load of Same Model: Must NOT hit disk, must return cached handle
    auto model2 = resMgr.GetModel("assets/models/test_cube.obj");
    assert(model2 != nullptr);
    assert(model1 == model2); // Must be the exact same shared pointer handle!
    assert(resMgr.GetDiskLoadCount() == initialDiskLoads + 1); // NO extra disk load
    assert(resMgr.GetCacheHitCount() == initialCacheHits + 1); // Cache hit incremented
    Logger::Info("[PASS] Resource cache returned identical handle without reloading from disk.");

    // 3. Invalid Model Path: Must NOT crash or return null, must return placeholder
    auto invalidModel = resMgr.GetModel("assets/models/non_existent_model_12345.obj");
    assert(invalidModel != nullptr);
    assert(invalidModel->IsPlaceholder() == true);
    assert(invalidModel->GetMeshes().size() > 0); // Contains magenta fallback cube
    Logger::Info("[PASS] Invalid model path safely returned visible placeholder without crash.");

    // 4. Invalid Texture Path: Must NOT crash, generates magenta fallback texture
    auto invalidTexture = resMgr.GetTexture("assets/textures/missing_texture_98765.png");
    assert(invalidTexture != nullptr);
    assert(invalidTexture->IsLoaded() == false); // Flagged as fallback
    Logger::Info("[PASS] Invalid texture path safely generated fallback texture.");

    // 5. Cleanup
    resMgr.Shutdown();
    assert(resMgr.GetModelCount() == 0);
    assert(resMgr.GetTextureCount() == 0);
    Logger::Info("[PASS] ResourceManager reported 0 leaked resources on shutdown.");

    glfwDestroyWindow(window);
    glfwTerminate();

    Logger::Info("test_resource_manager PASSED successfully.");
    Logger::Shutdown();
    return 0;
}


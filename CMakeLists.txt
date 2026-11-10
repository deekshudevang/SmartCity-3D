cmake_minimum_required(VERSION 3.15)
project(SmartCity3D VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /EHsc)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

# 1. GLAD (OpenGL 3.3 Core Profile)
set(GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glad")
add_library(glad STATIC "${GLAD_DIR}/src/glad.c")
target_include_directories(glad PUBLIC "${GLAD_DIR}/include")

# 2. GLFW 3.4
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/CMakeLists.txt")
    add_subdirectory(external/glfw)
else()
    find_package(glfw3 3.3 REQUIRED)
endif()

# 3. GLM
set(GLM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glm")
if(EXISTS "${GLM_DIR}/CMakeLists.txt")
    add_library(glm INTERFACE)
    target_include_directories(glm INTERFACE "${GLM_DIR}")
else()
    find_package(glm REQUIRED)
endif()

# 4. Dear ImGui
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
set(IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
    "${GLAD_DIR}/include"
)
target_link_libraries(imgui PUBLIC glfw glad)

# 5. STB Image
set(STB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/stb")
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${STB_DIR}")

# 6. Core Engine Library
set(SMARTCITY_CORE_SOURCES
    src/core/Logger.cpp
    src/core/Time.cpp
    src/core/Input.cpp
    src/core/Window.cpp
    src/core/Config.cpp
    src/core/Application.cpp
    src/gfx/Buffer.cpp
    src/gfx/Mesh.cpp
    src/gfx/Shader.cpp
    src/gfx/Camera.cpp
    src/gfx/Light.cpp
    src/gfx/ShadowMap.cpp
    src/gfx/Texture2D.cpp
    src/gfx/Frustum.cpp
    src/gfx/Material.cpp
    src/gfx/Renderer.cpp
    src/gfx/Skybox.cpp
    src/scene/Transform.cpp
    src/scene/SceneObject.cpp
    src/scene/Scene.cpp
    src/scene/Model.cpp
    src/scene/ResourceManager.cpp
    src/city/IntersectionGeometry.cpp
    src/city/RoadMarkings.cpp
    src/city/BuildingGenerator.cpp
    src/city/StreetFurniture.cpp
    src/city/RoadNetwork.cpp
    src/city/CityEnvironment.cpp
    src/sim/LaneGraph.cpp
    src/sim/Vehicle.cpp
    src/sim/IDMController.cpp
    src/sim/TrafficManager.cpp
    src/sim/OBB.cpp
    src/sim/ISignalProvider.cpp
    src/sim/ArcLengthLUT.cpp
    src/sim/ConflictManager.cpp
    src/sim/VehicleManager.cpp
    src/traffic/TrafficSignalController.cpp
    src/ped/PedestrianAgent.cpp
    src/ped/PedestrianManager.cpp
)

set(SMARTCITY_CORE_HEADERS
    src/core/Logger.h
    src/core/Time.h
    src/core/Input.h
    src/core/Window.h
    src/core/Config.h
    src/core/Application.h
    src/gfx/Buffer.h
    src/gfx/Mesh.h
    src/gfx/Shader.h
    src/gfx/Camera.h
    src/gfx/Light.h
    src/gfx/ShadowMap.h
    src/gfx/Texture2D.h
    src/gfx/Frustum.h
    src/gfx/Material.h
    src/gfx/Renderer.h
    src/gfx/Skybox.h
    src/scene/Transform.h
    src/scene/SceneObject.h
    src/scene/Scene.h
    src/scene/Model.h
    src/scene/ResourceManager.h
    src/city/IntersectionGeometry.h
    src/city/RoadMarkings.h
    src/city/BuildingGenerator.h
    src/city/StreetFurniture.h
    src/city/CityEnvironment.h
    src/sim/LaneGraph.h
    src/sim/Vehicle.h
    src/sim/IDMController.h
    src/sim/TrafficManager.h
    src/traffic/SignalTypes.h
    src/traffic/TrafficSignalController.h
    src/ped/PedestrianAgent.h
    src/ped/PedestrianManager.h
)

add_library(SmartCityCore STATIC
    ${SMARTCITY_CORE_SOURCES}
    ${SMARTCITY_CORE_HEADERS}
)

target_include_directories(SmartCityCore PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${GLAD_DIR}/include"
    "${STB_DIR}"
)
target_link_libraries(SmartCityCore PUBLIC glad glfw glm imgui stb)

if(WIN32)
    target_link_libraries(SmartCityCore PUBLIC opengl32 gdi32 user32 shell32)
endif()

# Main Application Executable (Bootstrapping only)
add_executable(SmartCity3D
    src/main.cpp
)

target_link_libraries(SmartCity3D PRIVATE SmartCityCore)
target_include_directories(SmartCity3D PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Copy assets and shaders to build output directory
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:SmartCity3D>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:SmartCity3D>/shaders"
    COMMENT "Copying assets and shaders to output directory"
)

# Automated Unit & Integration Tests Subdirectory
add_subdirectory(tests)

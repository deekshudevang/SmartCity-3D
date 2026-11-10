cmake_minimum_required(VERSION 3.15)
project(SmartCity3D VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable CTest
enable_testing()

# Compiler options
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /EHsc)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Dependencies setup
# 1. GLAD (OpenGL 3.3 Core)
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
)

set(SMARTCITY_CORE_HEADERS
    src/core/Logger.h
    src/core/Time.h
    src/core/Input.h
    src/core/Window.h
)

add_library(SmartCityCore STATIC
    ${SMARTCITY_CORE_SOURCES}
    ${SMARTCITY_CORE_HEADERS}
)

target_include_directories(SmartCityCore PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${GLAD_DIR}/include"
    "${STB_DIR}"
)
target_link_libraries(SmartCityCore PUBLIC glad glfw glm imgui stb)

if(WIN32)
    target_link_libraries(SmartCityCore PUBLIC opengl32 gdi32 user32 shell32)
endif()

# Main Application Executable
add_executable(SmartCity3D
    src/main.cpp
)

target_link_libraries(SmartCity3D PRIVATE SmartCityCore)
target_include_directories(SmartCity3D PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Copy assets to build directory
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:SmartCity3D>/assets"
    COMMENT "Copying assets to output directory"
)

# Subdirectory for Automated Unit & Integration Tests
add_subdirectory(tests)

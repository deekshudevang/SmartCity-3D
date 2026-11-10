# SmartCity 3D — Build & Compilation Instructions

This guide provides complete, step-by-step instructions for building and running **SmartCity 3D** from source on Microsoft Windows 10/11.

---

## 1. Prerequisites

To build SmartCity 3D from source, ensure your development system has the following tools installed:

1. **Operating System**: Windows 10 or Windows 11 (64-bit).
2. **C++17 Compiler**:
   - **Recommended**: MinGW-w64 GCC 13.2.0 (x86_64) or later (e.g. from [w64devkit](https://github.com/skeeto/w64devkit)).
   - **Alternative**: Microsoft Visual Studio 2022 (MSVC v143 toolset with C++ Desktop Development workload).
3. **CMake**: Version 3.21 or later ([cmake.org](https://cmake.org/download/)).
4. **Git**: Git for Windows ([git-scm.com](https://git-scm.com/)).

*Note: All core external libraries (GLFW, GLAD, GLM, Dear ImGui, Assimp, stb_image, and nlohmann/json) are either vendored or statically linked, requiring no third-party package managers or complex system installs.*

---

## 2. Quick Automated Build (PowerShell)

We provide three PowerShell automation scripts in `/scripts/` designed to bootstrap, build, and run the project seamlessly:

### Step 1: Environment Setup
Open a standard Windows PowerShell terminal, navigate to the project root, and execute:
```powershell
powershell -ExecutionPolicy Bypass -File scripts/setup.ps1
```
This script validates your toolchain, detects Git, CMake, and the C++ compiler, and sets up path priority.

### Step 2: Compile the Engine
Build the project in **Release** mode with full optimizations:
```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Config Release
```
Or build in **Debug** mode for debugging with active assertions and GL debug output:
```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Config Debug
```

### Step 3: Run the Application
Launch the compiled binary:
```powershell
powershell -ExecutionPolicy Bypass -File scripts/run.ps1
```

---

## 3. Manual Command-Line Build (MinGW Makefiles)

If you prefer using CMake directly from the terminal:

```powershell
# 1. Create build directory and generate build files
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# 2. Compile executable and test suite using multi-threaded build
cmake --build build --config Release -j

# 3. Execute the full automated regression test suite
ctest --test-dir build -C Release --output-on-failure

# 4. Launch the simulator
.\build\SmartCity3D.exe
```

---

## 4. Visual Studio 2022 IDE Build

1. Open Visual Studio 2022.
2. Select **Open a local folder** and choose the `CG Project` root directory.
3. Visual Studio will automatically detect `CMakeLists.txt` and configure the project.
4. Select `x64-Release` or `x64-Debug` from the configuration dropdown.
5. In the toolbar, select `SmartCity3D.exe` as the startup item.
6. Press `F5` to build and debug, or `Ctrl+F5` to run without debugging.

---

## 5. Clean Windows VM / Sandbox Testing Guide

> **Note on Sandbox Verification**: As defined in the Phase 8 QA protocol, executing inside a completely isolated, clean Windows machine without any pre-installed developer tools is categorized as `MANUAL_REQUIRED` to verify the pre-compiled standalone binary.

Follow these steps to verify zero-dependency deployment in a fresh Windows Sandbox environment:

1. **Enable Windows Sandbox**:
   - In Windows 11 Start Menu, search for **Turn Windows features on or off**.
   - Check **Windows Sandbox** and click OK (requires a quick reboot if not previously enabled).
2. **Launch Windows Sandbox**:
   - Open **Windows Sandbox** from the Start Menu.
3. **Copy & Extract Release Package**:
   - Copy `release/SmartCity3D-1.0.0-Windows-x64.zip` from your host machine.
   - Paste it onto the Windows Sandbox desktop.
   - Right-click and choose **Extract All...**.
4. **Execute and Verify**:
   - Open the extracted `SmartCity3D-1.0.0` folder.
   - Double-click `SmartCity3D.exe`.
   - **Expected Result**: The application initializes immediately at 1600x900 resolution without displaying any missing DLL errors (such as `VCRUNTIME140.dll` or `MSVCP140.dll`), as all runtime libraries are statically linked with the exception of standard Windows operating system binaries (`KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `msvcrt.dll`, `SHELL32.dll`).


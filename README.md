# SmartCity 3D — Intelligent Traffic Intersection and Urban Environment Simulation

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![OpenGL](https://img.shields.io/badge/OpenGL-3.3%20Core-green.svg)](https://www.opengl.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**SmartCity 3D** is a high-performance, real-time 3D urban traffic intersection simulator engineered in modern C++17 and OpenGL 3.3 Core Profile. The application simulates microscopic vehicle dynamics using the **Intelligent Driver Model (IDM)**, multi-lane waypoint routing with cubic Bézier turning curves, day/night atmospheric solar cycles, cascaded PCF shadow mapping, and real-time telemetry overlays via Dear ImGui.

---

## 🏛 System Architecture

The simulation engine is architected with strict decoupling between rendering, physics, and world configuration:

```
SmartCity 3D
├── src/core/        — Windowing (GLFW), Leveled Logger, Clamped Time, Config Loader
├── src/gfx/         — RAII OpenGL Buffers, GLSL Shaders, 3D Camera, ShadowMap, Day/Night
├── src/city/        — Intersection Geometry, Double Yellow/White Markings, 12 Buildings, Signal Poles
├── src/sim/         — Waypoint LaneGraph, Multi-Class Vehicles, IDM Controller, TrafficManager
├── assets/          — External JSON configs, GLSL 330 shaders, textures
├── tests/unit/      — Automated CTest validation suite (Math, Time, Camera, Intersection, IDM, Collision)
└── scripts/         — PowerShell build, run, and environment setup automation
```

---

## 🚀 Key Engineering Capabilities

1. **Microscopic Traffic Simulation (IDM)**:
   - Gold-standard Intelligent Driver Model (Treiber et al., 2000) governing acceleration, closing speed, and headway convergence.
   - Stop bar compliance: Virtual obstacle modeling for red and yellow signal phases with dilemma zone avoidance.
   - Guaranteed collision-free car-following: Minimum spacing $s_0 \ge 2.0\text{ m}$ preserved under heavy queuing.

2. **Multi-Class Vehicle Fleet**:
   - **Sedan**: Agile urban cruiser ($v_0 = 13.88\text{ m/s}$, $a = 2.5\text{ m/s}^2$).
   - **SUV**: Heavy commuter vehicle ($v_0 = 13.0\text{ m/s}$, $a = 2.2\text{ m/s}^2$).
   - **Bus**: Multi-axle mass transit carrier ($L = 10.5\text{ m}$, $v_0 = 10.0\text{ m/s}$, $a = 1.2\text{ m/s}^2$).
   - **Emergency**: Priority vehicle with dual-alternating red/blue strobe beacons ($v_0 = 18.0\text{ m/s}$, $a = 3.5\text{ m/s}^2$) and corridor clearance preemption.

3. **Waypoint LaneGraph & Bézier Trajectories**:
   - 12 complete approach trajectories (North, South, East, West for Straight, Right, Left turns).
   - Smooth cubic Bézier curvature through the intersection box for realistic vehicular cornering.
   - Automated intersection crossing conflict detection and unprotected left-turn yielding.

4. **Modern OpenGL 3.3 Core Pipeline**:
   - Multi-pass rendering: 2048x2048 depth FBO pass with $3\times 3$ Percentage-Closer Filtering (PCF) soft shadows.
   - Dynamic directional lighting with astronomical solar angle transitions (sun, dusk, night, moon).
   - 16 streetlight point light sources with distance attenuation.
   - Emissive shader rendering for traffic signal lenses and vehicle lighting.

---

## 🎮 Interactive Controls & Camera Presets

| Control | Action |
| :--- | :--- |
| **W / A / S / D** | Fly camera forwards, left, backwards, right |
| **Q / E** | Move camera vertically down / up |
| **Shift** | Boost camera movement speed ($2.5\times$) |
| **Right Mouse + Drag** | Free-look camera rotation (pitch clamped $\pm 89^\circ$) |
| **Mouse Scroll** | Dynamic Field of View (FOV) zoom ($25^\circ$ to $90^\circ$) |
| **Tab** | Toggle mouse cursor capture / release for ImGui UI |
| **Escape** | Clean application exit |

### Quick Camera Viewpoint Buttons (UI Panel)
- **Overview (45°)**: Isometric view overlooking the full 4-way intersection.
- **Top-Down**: Orthographic-style tactical surveillance angle looking directly down.
- **North Traffic Cam**: Elevated approach camera tracking vehicles arriving from the North.
- **East Traffic Cam**: Approach camera tracking East corridor throughput.
- **Pedestrian View**: Eye-level street perspective at the crosswalk curb.

---

## 🛠 Prerequisites & Toolchain

- **Operating System**: Windows 10/11 (64-bit)
- **Compiler**: Visual Studio 2022 (MSVC cl.exe) or MinGW-w64 (GCC >= 10.0 x64)
- **Build System**: CMake >= 3.21
- **Graphics Hardware**: GPU with native OpenGL 3.3 Core Profile support

---

## 📦 Build & Execution Instructions

### 1. Environment Verification & Setup
Verify developer tools and toolchains:
```powershell
.\scripts\setup.ps1
```

### 2. Configure and Compile
Build in Debug or Release mode:
```powershell
# Debug build
.\scripts\build.ps1 -Config Debug

# Release build
.\scripts\build.ps1 -Config Release

# Clean build
.\scripts\build.ps1 -Config Release -Clean
```

### 3. Launch Application
Launch from the project root directory:
```powershell
.\scripts\run.ps1
```

For headless automated verification (renders 60 frames on GPU and exits cleanly):
```powershell
.\scripts\run.ps1 -AppArgs @("--headless")
```

---

## 🧪 Automated Unit Test Suite

Run all automated unit tests via CTest:
```powershell
ctest --test-dir build --output-on-failure
```

### Test Coverage Table
| Test Suite | Purpose |
| :--- | :--- |
| `test_math` | Validates vector operations, angle normalization, and transforms |
| `test_time` | Verifies fixed-timestep accumulator stability and delta-time clamping |
| `test_camera` | Validates view-projection matrix calculation, pitch clamping, and presets |
| `test_intersection` | Validates roadway dimensions, sidewalk curb heights, and crosswalk offsets |
| `test_lane_graph` | Validates 12 trajectory lengths, stop bar points, and Bézier smoothness |
| `test_idm` | Validates free-road acceleration, leader following, and standstill equilibrium |
| `test_collision_avoidance` | Verifies multi-vehicle platoon stopping at red light with zero collisions |

---

## 📄 License
This project is developed as an academic computer graphics and simulation engineering project.

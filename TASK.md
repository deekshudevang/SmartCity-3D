# SmartCity 3D — Intelligent Traffic Intersection and Urban Environment Simulation
## Master Engineering Task Tracker & Status Report

**Repository**: https://github.com/deekshudevang/SmartCity-3D.git  
**Target Platform**: Windows 10/11  
**Core Technologies**: C++17, OpenGL 3.3 Core Profile, GLSL 330, GLFW, GLAD, GLM, Dear ImGui, stb_image, Assimp, CMake, Visual Studio 2022, vcpkg  

---

### Project Phases & Status Matrix

| Phase | Description | Status | Verification Evidence |
| :--- | :--- | :--- | :--- |
| **Phase 1** | Project Architecture, Core Foundation & Build System | VERIFIED | CMake C++17 build, GLAD, GLFW, GLM, ImGui, tests passed 100%, RTX 4060 GL 3.3 init |
| **Phase 2** | 3D Graphics Pipeline, Shaders, Lighting & Camera | VERIFIED | GLSL 330 shaders, 2048x2048 ShadowMap FBO, Blinn-Phong + PCF shadows, Day/Night, Multi-cam |
| **Phase 3** | 4-Way Urban Intersection & Surrounding City Environment | IN_PROGRESS | Roads, multi-lane markings, crosswalks, sidewalks, buildings, street furniture |
| **Phase 4** | Traffic Simulation Engine & Realistic Vehicle Dynamics | PENDING | IDM car-following, Bézier turning curves, collision avoidance, multi-vehicle classes |
| **Phase 5** | Pedestrian Dynamics & Crosswalk Safety Logic | PENDING | Pedestrian agent navigation, waiting zones, zebra crossing compliance |
| **Phase 6** | Traffic Signal Controllers & Smart Urban Management | PENDING | Fixed-time, adaptive queue-actuated, and emergency vehicle preemption (EVP) |
| **Phase 7** | Real-Time Analytics Dashboard & ImGui Interactive Controls | PENDING | Real-time state metrics (throughput, delay, queues, CO2), interactive controls |
| **Phase 8** | Automated Testing Suite, Verification & Technical Documentation | PENDING | Unit tests, headless regression tests, architecture documentation |

---

### Phase 1: Project Architecture, Core Foundation & Build System
- [x] Cloned and verified empty git repository at `https://github.com/deekshudevang/SmartCity-3D.git`.
- [x] Configured build environment (CMake 3.30, GCC 13.2 C++17 x64, Make).
- [x] Create `vcpkg.json` manifest for Visual Studio 2022 + vcpkg workflow.
- [x] Create modular root and sub-directory `CMakeLists.txt` with dual-mode support (vcpkg / vendored fallback).
- [x] Integrate GLAD (OpenGL 3.3 Core), GLFW 3.4, GLM, Dear ImGui, and stb_image.
- [x] Implement core foundational architecture:
  - `src/core/Window.h/.cpp` (GLFW window creation, context initialization, input callbacks)
  - `src/core/Time.h/.cpp` (fixed-timestep simulation accumulator: 60Hz dt)
  - `src/core/Logger.h/.cpp` (console and file diagnostics logging)
  - `src/core/Input.h/.cpp` (keyboard and mouse state management)
- [x] Implement test runner and baseline unit tests verifying math, timer, and deterministic simulation stepping.
- [x] Build and execute test suite; confirm 100% pass rate (`test_math`: PASS, `test_time`: PASS).
- [x] Verify window creation and OpenGL context initialization (Verified on NVIDIA GeForce RTX 4060 Laptop GPU, OpenGL 3.3.0, GLSL 3.30).

---

### Phase 2: 3D Graphics Pipeline, Shaders, Lighting & Camera
- [x] Implement `Shader` class with shader compilation, linkage error checking, and hot-reload/fallback (`src/graphics/Shader.h/.cpp`).
- [x] Implement GLSL 330 shaders:
  - `scene.vert` / `scene.frag`: Blinn-Phong directional lighting, ambient transition, point lights (streetlights, headlights, emergency strobes), 3x3 PCF soft shadows.
  - `shadow_depth.vert` / `shadow_depth.frag`: Directional shadow depth pass.
  - `light_indicator.vert` / `light_indicator.frag`: Emissive traffic signal lamps and glowing elements.
- [x] Implement `Camera` system with View-Projection matrices:
  - Perspective projection (FOV 45°, near 0.1, far 1000.0)
  - Orbit camera mode (pan, rotate, zoom)
  - Free-flight camera mode (WASD + mouse look)
  - Fixed camera presets: Overview 45°, Top-Down Orthographic, North Traffic Cam, East Traffic Cam, Pedestrian View
  - Tested with unit test `test_camera` (100% pass).
- [x] Implement Shadow Mapping pipeline:
  - 2048x2048 Depth Framebuffer Object (FBO) with `GL_DEPTH_COMPONENT24`
  - Orthographic shadow projection matrix covering 80m x 80m intersection area
  - Percentage-Closer Filtering (PCF) 3x3 kernel soft shadows
- [x] Implement Day/Night lighting manager (`src/graphics/Light.h/.cpp`):
  - Dynamic sun/moon angle based on simulated time of day (0.0 to 24.0 hours)
  - Ambient light temperature transitions (dawn, noon, dusk, night)
  - Automatic dusk/night activation of streetlights and vehicle headlights.

---

### Phase 3: 4-Way Urban Intersection & Surrounding City Environment
- [ ] Procedural Road Geometry Generator:
  - North, South, East, West approaches with 30m+ approach lengths
  - Approach configuration: Left-turn lane, Straight through lane, Straight/Right-turn lane
  - Dedicated lane centerlines and turning paths (Cubic Bézier curves)
- [ ] Road Surface Markings & Decals:
  - Solid and dashed lane dividing lines
  - Double solid yellow centerline
  - Intersection stop lines (halt bar for red signals)
  - Zebra striped pedestrian crossings on all 4 intersection branches
  - Directional lane arrows (left, straight, right)
- [ ] Sidewalks, Curbs & Waiting Islands:
  - Elevated curbs (0.15m height) with beveled edges
  - Pedestrian walking sidewalks flanking all roads
  - Pedestrian waiting corners at zebra crossing approaches
- [ ] Surrounding City Architecture & Urban Props:
  - Textured modular city buildings (residential, commercial, office towers)
  - Traffic signal mast poles with 3-aspect vehicular heads and pedestrian walk/wait heads
  - Streetlight poles with overhead lamps
  - Street signs, fire hydrants, decorative trees and urban planters
- [ ] Robust Texture & Material System:
  - Procedural fallbacks for asphalt, concrete, building brick/glass, grass, and lane paint

---

### Phase 4: Traffic Simulation Engine & Realistic Vehicle Dynamics
- [ ] Lane Network & Topological Waypoint Graph:
  - Lane graph nodes and directed edges for North, South, East, West
  - Smooth Bézier curve waypoints for:
    - Straight through movements (N->S, S->N, E->W, W->E)
    - Right turns (N->W, S->E, E->N, W->S)
    - Unprotected and protected Left turns (N->E, S->W, E->S, W->N)
- [ ] Vehicle Entity System:
  - Base vehicle model with physical dimensions:
    - Sedan: 4.5m x 1.8m x 1.4m
    - SUV: 4.8m x 2.0m x 1.7m
    - Bus: 11.5m x 2.5m x 3.2m
    - Emergency Vehicle (Ambulance / Fire Truck): 7.0m x 2.3m x 2.8m with flashing roof sirens
  - Configurable dynamic properties: mass, max velocity, max acceleration, comfortable deceleration, braking limit
- [ ] Intelligent Driver Model (IDM):
  - Longitudinal acceleration calculation based on desired velocity, net distance headway, and approach rate
  - Collision-free headway preservation ($s^* = s_0 + v T + \frac{v \Delta v}{2\sqrt{a b}}$)
  - Natural stopping at traffic signal stop lines when signal is RED or YELLOW
- [ ] Intersection Conflict & Yielding Logic:
  - Stop line compliance: vehicles decelerate to halt before stop line when signal is Red
  - Yellow light dilemma zone logic: safe stop if distance permits, proceed if past point of no return
  - Left turn gap acceptance: vehicles turning left yield to oncoming straight traffic
  - Right turn on red / green filter logic
  - Emergency vehicle vehicle preemption yielding: standard traffic pulls to lane edge or halts when emergency siren is within range

---

### Phase 5: Pedestrian Dynamics & Crosswalk Safety Logic
- [ ] Pedestrian Agent System:
  - Agents with walking speed distributions (1.0 to 1.5 m/s)
  - Sidewalk routes and crossing itineraries
- [ ] Crosswalk Navigation & Safety Invariants:
  - Pedestrians wait at sidewalk edge when pedestrian signal is DON'T WALK (Red Hand / Solid Red)
  - Pedestrians enter zebra crossing when pedestrian signal transitions to WALK (Green Walking Figure)
  - Crossing countdown and clearance phase
  - Vehicle-pedestrian conflict safety: turning vehicles yield to pedestrians occupied on zebra crossing

---

### Phase 6: Traffic Signal Controllers & Smart Urban Management
- [ ] Signal Head Representation & State Machine:
  - 12 vehicular signal heads (3-aspect: Red, Yellow, Green + Left Turn arrow)
  - 8 pedestrian signal heads (2-aspect: Walk, Don't Walk)
- [ ] Signal Controller Mode 1: Fixed-Time Cyclic Controller
  - Standard NEMA-inspired 4-phase or 8-phase dual-ring cycle:
    - Phase A: North-South Through & Right
    - Phase B: North-South Protected Left Turns
    - Phase C: East-West Through & Right
    - Phase D: East-West Protected Left Turns
  - Dedicated yellow clearance (3.5s) and all-red intersection clearance (1.5s)
- [ ] Signal Controller Mode 2: Adaptive / Actuated Controller
  - Dynamic queue detection per approach
  - Green extension for approaches with heavy flow
  - Minimum green (7s) and maximum green (45s) safety bounds
  - Automatic skip of empty approaches
- [ ] Signal Controller Mode 3: Emergency Vehicle Preemption (EVP)
  - Radio/optical sensor detection when emergency vehicle approaches intersection
  - Rapid safe clearance of conflicting phases
  - Immediate green corridor along emergency vehicle approach
  - Return to coordinated cycle following passage of emergency vehicle
- [ ] Pedestrian Demand Actuation:
  - Push-button pedestrian call registered in controller
  - Pedestrian walk phase inserted at optimal cycle point

---

### Phase 7: Real-Time Analytics Dashboard & ImGui Interactive Controls
- [ ] Simulation State Derived Metrics (No random or hardcoded numbers):
  - Frame Rate (FPS) and 99th percentile frame latency (ms)
  - Live active vehicles & live active pedestrians
  - Cumulative vehicles spawned and successfully completed trips
  - Average travel time and average intersection delay per vehicle
  - Current queue lengths per approach (North, South, East, West) in vehicles and meters
  - Intersection throughput (vehicles cleared per simulated minute)
  - Instantaneous and cumulative CO2 emissions (g) estimated via physical engine load / idle models
  - Current signal phase, active stage, and cycle countdown
- [ ] Dear ImGui User Interface & Control Deck:
  - Traffic Signal Management panel (Switch mode: Fixed, Adaptive, Manual, Trigger Emergency Preempt)
  - Simulation Environment panel (Time of day slider, Simulation speed: 0.25x to 4x, Pause/Resume, Reset)
  - Traffic Demand panel (North/South/East/West arrival rates, vehicle type distribution, spawn burst button)
  - Camera Deck (Quick-select camera angles, FOV slider)
  - Diagnostics & Log panel (Export analytics CSV, view real-time events)

---

### Phase 8: Automated Testing Suite, Verification & Technical Documentation
- [ ] CTest test harness with automated test cases:
  - `test_math`: Bézier curve interpolation, ray-box intersections, vector math
  - `test_time`: Fixed timestep accumulator precision and overflow handling
  - `test_idm`: Acceleration, headway equilibrium, deceleration to stop line
  - `test_signal_safety`: Verification that conflicting greens can NEVER occur concurrently (fail-safe yellow/all-red invariant)
  - `test_analytics`: Mathematical verification that throughput, wait time, and queue counters strictly match entity states
  - `test_collision_avoidance`: Vehicle boundary spacing invariants under dense traffic
- [ ] Headless simulation verification runner:
  - Executes 1000 simulated ticks without GUI, records state integrity, asserts 0 collisions
- [ ] Comprehensive Project Documentation:
  - `README.md`: System requirements, Visual Studio 2022 build instructions, vcpkg instructions, keyboard/mouse controls, simulation features
  - `ARCHITECTURE.md`: Subsystem separation (Renderer, Simulation, Scene, UI, Controller, Analytics)
  - `USER_GUIDE.md`: Step-by-step user instructions for dashboard controls, camera manipulation, and experiment scenarios

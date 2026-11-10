# SmartCity 3D — System Blueprint & Technical Specification

## 1. Executive Summary

**SmartCity 3D** is a high-performance, real-time 3D urban traffic intersection simulation and computer graphics engine built from first principles in modern **C++17** and **OpenGL 3.3 Core Profile**. It couples physically grounded microscopic vehicle dynamics (Intelligent Driver Model), macroscopic traffic control algorithms (Fixed-Time, Actuated, and Adaptive Backpressure), pedestrian simulation, dynamic environmental effects (astronomical solar cycles, variance shadow mapping, volumetric fog, GPU rain particle systems), and an enterprise vehicle customization library handling GTA V entity configurations.

---

## 2. Technology Stack & Foundation

```
┌────────────────────────────────────────────────────────────────────────┐
│                        SmartCity 3D Application                        │
├────────────────────────────────────────────────────────────────────────┤
│ UI & Tools:     Dear ImGui 1.90 (Docking / Viewports)                  │
│ Graphics API:   OpenGL 3.3 Core Profile (GLSL 330)                     │
│ Window & Input: GLFW 3.4                                               │
│ Extension Load: GLAD (OpenGL 3.3 Loader)                               │
│ Linear Algebra: GLM (OpenGL Mathematics 0.9.9.8)                       │
│ Asset Import:   Assimp 5.3 & stb_image 2.28                            │
│ Data Parsing:   nlohmann/json 3.11.3 (Modern C++ JSON)                 │
│ Build Harness:  CMake 3.21+ / MinGW-w64 GCC 13.2.0 (C++17)            │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Architecture & Subsystem Decomposition

The engine is engineered around decoupled, single-responsibility modules:

```
src/
├── core/         # Engine lifecycle, Window, Timer, Input, Logger, Application
├── gfx/          # Hardware abstractions: VertexArray, Buffers, Shader, Texture2D,
│                 # ShadowMap, LightingEnvironment, Renderer (Render Queue & Culling)
├── scene/        # Hierarchical SceneGraph, SceneObject, Transform, Camera, Frustum,
│                 # ResourceManager (centralized zero-leak asset cache)
├── city/         # Procedural intersection geometry, RoadNetwork, LaneGraph,
│                 # Crosswalks, Sidewalks, Markings, TrafficLightMesh
├── sim/          # Headless traffic simulation, IDM physics, MOBIL lane changes,
│                 # Vehicle, Oriented Bounding Box (OBB) SAT collision avoidance
├── traffic/      # SignalController FSM, ActuatedController, AdaptiveController
│                 # (Max-Pressure), ConflictMatrix, EmergencyPreemption
├── pedestrians/  # PedestrianSimulator, Pedestrian, Crosswalk FSM, Yield monitors
├── env/          # DayNightCycle (astronomical sun arc), WeatherSystem (rain simulation)
├── analytics/    # SimulationMetrics, Fuel/CO2 emission models, ExperimentRunner
├── ui/           # Dear ImGui overlays: DashboardPanel, VehicleGalleryPanel,
│                 # VehicleDetailsPanel, VehiclePreviewPanel, CustomizationPanel
└── vehicles/     # VehicleLibrary, VehiclePresetImporter, VehicleAssetRegistry,
                  # VehiclePreset, VehicleCustomization CAD renderer
```

---

## 4. Phase-by-Phase System Progression

### Phase 1: OpenGL Foundation
- Established reproducible C++17 build environment using CMake and MinGW-w64 GCC.
- Initialized GLFW window, GLAD extension loader, GLM mathematics, and Dear ImGui.
- Configured OpenGL 3.3 Core pipeline with debug message callbacks, depth testing, CCW face culling, and automated PowerShell setup/build scripts.

### Phase 2: Decoupled Graphics Engine
- Refactored monolithic rendering code into a modular architecture: `Renderer`, `Scene`, `SceneObject`, `Transform`, `Camera`, `Frustum`, and `ResourceManager`.
- Implemented Gribb-Hartmann 6-plane view-frustum culling, testing transformed world-space Axis-Aligned Bounding Boxes (AABB).
- Created 64-bit state-sorted render queue `(ShaderID << 32) | MaterialID` to minimize OpenGL pipeline state changes.
- Dual-mode camera supporting perspective and orthographic projections with FreeFly and spherical Orbit modes.

### Phase 3: Single Intersection Environment
- Authored `docs/INTERSECTION_SPEC.md` establishing standardized geometric dimensions: 3.5m lane widths, 30m turn pockets, 1.5m medians, 0.15m curbs, 3m sidewalks, and 4m crosswalks.
- Procedural generation of 3D intersection meshes: asphalt surface, concrete curbs with rounded corner returns, stop bars, lane division dashed lines, and zebra crosswalks.
- Single source of truth: `RoadNetwork` polyline geometry drives both 3D mesh tessellation and simulation path navigation.

### Phase 4: Vehicle Simulation Engine
- Decoupled headless vehicle simulation advancing at a fixed timestep ($\Delta t = 1/60\text{ s}$) with frame accumulator and state interpolation.
- Physically validated Intelligent Driver Model (IDM) for longitudinal acceleration and gap regulation.
- Oriented Bounding Box (OBB) collision avoidance using the 2D Separating Axis Theorem (SAT).
- Multi-lane route navigation and conflict zone arbitration.

### Phase 5: Traffic Control & Pedestrian Simulation
- Finite-State Machine (FSM) signal controller strictly validating yellow clearance ($3.0\text{ s}$) and all-red clearance intervals ($2.0\text{ s}$).
- Conflict matrix safety monitor guaranteeing conflicting movements never display concurrent GREEN phases.
- Pedestrian crosswalk simulation with WALK, FLASHING_DONT_WALK, and DONT_WALK states, enforcing strict vehicle yielding.
- Adaptive Max-Pressure signal controller optimizing green phase durations based on real-time inbound/outbound queue differentials.
- Emergency vehicle preemption corridor clearing all conflicting traffic before priority vehicle entry.

### Phase 6: Environment & Visual Effects
- Astronomical solar arc calculating sun azimuth and elevation across a continuous 24-hour diurnal cycle.
- Dynamic Blinn-Phong lighting with directional sunlight, ambient fill, and distance-attenuated streetlights and vehicle headlights.
- $2048 \times 2048$ Directional Shadow Mapping with Percentage-Closer Filtering (PCF) and slope-scale depth bias.
- GPU-accelerated rain particle system simulating 10,000 falling raindrops with ground splash effects.
- Distance-based exponential atmospheric fog.

### Phase 7: Analytics, Dashboard & Computer Graphics Algorithms
- Real-time performance and traffic metrics: active vehicles, queue lengths, average speed, waiting delay, fuel consumption, and CO2 emissions.
- Comparative Experiment Runner executing multi-seed batch tests comparing Fixed-Time, Actuated, and Adaptive control strategies.
- Rigorous mathematical documentation authored in `docs/ALGORITHMS.md` and `docs/METRICS.md`.

### Phase 8: Final Testing, Audit & Release Packaging
- Independent QA audit identifying and fixing build script and compiler warning defects (DEF-01, DEF-02, DEF-03).
- 100% pass rate across all 25 automated CTest suites in both Debug and Release configurations.
- Real hardware performance profiling on NVIDIA RTX 4060: > 1,300 FPS under full stress load; > 1,570 FPS with shadows disabled.
- Fully self-contained Windows release package (`SmartCity3D-1.0.0-Windows-x64.zip`) with zero external runtime DLL dependencies.

### Phase 9: Complete Vehicle JSON Library Integration
- Scalable recursive JSON scanner discovering and parsing Cherax entity configurations in `assets/vehicle_presets/source_json/`.
- Normalized database of 66 GTA vehicle presets with 50 unique model hashes and FNV-1a content deduplication.
- Extracted 50 modification slots (`mod0`..`mod49`), 16 extras, wheel types, and standard 160-color GTA palette mappings.
- Real-time CAD chassis generator rendering proportional vehicle bodies, rotating cylinder wheels, and glowing underglow neon lights.
- Interactive ImGui suite: Vehicle Gallery, Details Inspector, 3D Offscreen FBO Turntable Preview, and Customization Studio.

---

## 5. Subsystem Data Flow

```
┌──────────────────────────────────────────────────────────────┐
│                    Fixed Timestep Simulation                 │
│                                                              │
│  ┌────────────────┐     ┌──────────────┐    ┌─────────────┐  │
│  │ Signal Control │ ──> │ Intersection │    │ Pedestrians │  │
│  │ (FSM/Adaptive) │     │ Conflict Map │    │  Crosswalks │  │
│  └────────┬───────┘     └──────┬───────┘    └──────┬──────┘  │
│           │                    │                   │         │
│           ▼                    ▼                   ▼         │
│  ┌────────────────────────────────────────────────────────┐  │
│  │     Vehicle Simulation Engine (IDM + OBB SAT Physics)   │  │
│  └─────────────────────────────┬──────────────────────────┘  │
└────────────────────────────────┼─────────────────────────────┘
                                 │ Snapshots (Interpolated)
                                 ▼
┌──────────────────────────────────────────────────────────────┐
│                     Rendering & Graphics Engine              │
│                                                              │
│  ┌─────────────────┐    ┌─────────────────┐                  │
│  │ Sun / Moon Arc  │ ─> │ Shadow Pass FBO │                  │
│  │ (Directional)   │    │  (2048 x 2048)  │                  │
│  └─────────────────┘    └────────┬────────┘                  │
│                                  │ Depth Texture             │
│                                  ▼                           │
│  ┌─────────────────┐    ┌─────────────────┐    ┌──────────┐  │
│  │ Frustum Culling │ ─> │  Render Queue   │ ─> │ Geometry │  │
│  │ (6 Planes AABB) │    │  (State Sorted) │    │   Pass   │  │
│  └─────────────────┘    └─────────────────┘    └────┬─────┘  │
│                                                     │        │
│  ┌─────────────────┐    ┌─────────────────┐         │        │
│  │ Rain Particles  │ ─> │ Skybox & ImGui  │ <───────┘        │
│  │ (Alpha Blended) │    │ UI Diagnostics  │                  │
│  └─────────────────┘    └─────────────────┘                  │
└──────────────────────────────────────────────────────────────┘
```

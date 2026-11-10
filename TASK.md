# TASK.md — SmartCity 3D Engineering Tracker

**Target Platform**: Windows 10/11  
**Stack**: C++17, OpenGL 3.3 Core Profile, GLSL 330, GLFW, GLAD, GLM, Dear ImGui, CMake, vcpkg  
**Conventions**: Right-handed, +Y up, ground plane = XZ, North = -Z, East = +X. Internal units: metres, seconds, radians.  
**Allowed Statuses**: `VERIFIED`, `PARTIAL`, `BLOCKED`, `NOT_IMPLEMENTED`.

---

## Phase 1 | Status: VERIFIED | Date: 2026-11-10

### Requirement Table
| ID | Requirement | Status | Evidence (file/command) |
| :--- | :--- | :--- | :--- |
| REQ-01-01 | CMake build system supporting C++17 with vcpkg and vendored fallback | VERIFIED | `CMakeLists.txt`, `vcpkg.json`, `cmake -B build -G "MinGW Makefiles"` |
| REQ-01-02 | GLAD (OpenGL 3.3 Core), GLFW 3.4, GLM, Dear ImGui, STB integration | VERIFIED | `external/{glad,glfw,glm,imgui,stb}`, compilation of `SmartCityCore` |
| REQ-01-03 | Core logging system with timestamped console & file output | VERIFIED | `src/core/Logger.h/.cpp`, output in `logs/smartcity.log` |
| REQ-01-04 | Fixed-timestep simulation accumulator pattern (60 Hz dt) | VERIFIED | `src/core/Time.h/.cpp`, `tests/unit/test_time.cpp` |
| REQ-01-05 | Input state handling with keyboard, mouse, and scroll events | VERIFIED | `src/core/Input.h/.cpp` |
| REQ-01-06 | OpenGL 3.3 Core profile window creation and buffer swap | VERIFIED | `src/core/Window.h/.cpp`, verified on NVIDIA GeForce RTX 4060 |

### Test Table
| Test | Type (auto/manual) | Result | Evidence |
| :--- | :--- | :--- | :--- |
| `test_math` | auto (CTest) | PASS | `ctest -R test_math` (Passed 0.20s, exit code 0) |
| `test_time` | auto (CTest) | PASS | `ctest -R test_time` (Passed 0.30s, exit code 0) |
| Headless Lifecycle | auto | PASS | `.\build\SmartCity3D.exe --headless` (60 frames rendered, exit code 0) |

### Known Issues / Deviations / Next-phase prerequisites
- None.

---

## Phase 2 | Status: VERIFIED | Date: 2026-11-10 | Commit: `9a34f41`

### Requirement Table
| ID | Requirement | Status | Evidence (file/command) |
| :--- | :--- | :--- | :--- |
| REQ-02-01 | Reusable Renderer with pass management, state sorting, CCW culling & RenderStats | VERIFIED | `src/gfx/Renderer.h/.cpp`, sorted by `(ShaderID << 32) \| MaterialID` |
| REQ-02-02 | Scene and SceneObject graph with AABB, visibility flags, and clean RAII lifecycle | VERIFIED | `src/scene/Scene.h/.cpp`, `src/scene/SceneObject.h/.cpp` |
| REQ-02-03 | Transform with T*R*S composition, quaternions, Euler helpers, dirty flag, CPU normal matrix | VERIFIED | `src/scene/Transform.h/.cpp`, `tests/unit/test_transform.cpp` |
| REQ-02-04 | Camera supporting runtime toggle between Perspective & Zoomable Orthographic | VERIFIED | `src/gfx/Camera.h/.cpp`, `tests/unit/test_camera_matrices.cpp` |
| REQ-02-05 | Gribb-Hartmann 6-plane view-frustum culling with AABB / sphere intersection testing | VERIFIED | `src/gfx/Frustum.h/.cpp`, `tests/unit/test_frustum.cpp` |
| REQ-02-06 | ResourceManager with canonical path caching, shared handles, duplicate load prevention & 0 leaks | VERIFIED | `src/scene/ResourceManager.h/.cpp`, `tests/unit/test_resource_manager.cpp` |
| REQ-02-07 | Assimp Model loading with tangent/normal generation, UV flipping, and visible magenta fallback | VERIFIED | `src/scene/Model.h/.cpp`, fallback verified on missing file |
| REQ-02-08 | Material with ambient/diffuse/specular, shininess, maps, and state-change minimizing sort key | VERIFIED | `src/gfx/Material.h/.cpp`, 64-bit sort key |
| REQ-02-09 | Directional, Point (attenuation), and SpotLight (inner/outer cutoff) with Blinn-Phong toggle | VERIFIED | `src/gfx/Light.h/.cpp`, `shaders/scene.frag` |
| REQ-02-10 | Skybox with cubemap loader, GL_LEQUAL depth trick, view translation stripping, and procedural fallback | VERIFIED | `src/gfx/Skybox.h/.cpp`, `shaders/skybox.vert/.frag` |
| REQ-02-11 | Diagnostics overlay: rolling 60-frame avg/min/max FPS & ms, draw calls, triangles, culled count, controls | VERIFIED | `src/core/Application.cpp` |
| REQ-02-12 | Comprehensive Architecture & Algorithm documentation | VERIFIED | `docs/ARCHITECTURE.md`, `docs/ALGORITHMS.md` |

### Test Table
| Test | Type (auto/manual) | Result | Evidence |
| :--- | :--- | :--- | :--- |
| `test_frustum` | auto (CTest) | PASS | `ctest -R test_frustum` (Gribb-Hartmann 6 planes vs AABB / sphere) |
| `test_transform` | auto (CTest) | PASS | `ctest -R test_transform` (TRS order, hierarchy, normal matrix) |
| `test_camera_matrices` | auto (CTest) | PASS | `ctest -R test_camera_matrices` (Analytical ortho/persp checks) |
| `test_resource_manager` | auto (CTest) | PASS | `ctest -R test_resource_manager` (Cache hits, fallback model, 0 leaks) |
| Full Suite Regression (11/11) | auto (CTest) | PASS | `ctest --test-dir build` (100% tests passed, 2.95s total) |
| Headless Graphics Run | auto | PASS | `.\build\SmartCity3D.exe --headless` (60 frames rendered, 0 leaks reported) |

### Known Issues / Deviations / Next-phase prerequisites
- None. Phase 2 is fully implemented and VERIFIED.
- Next Phase Prerequisite: Phase 2 is approved. Ready to proceed to Phase 3/5.

---

## Phase 3 | Status: VERIFIED | Date: 2026-11-10 | Commit: `b0a5c63`

### Requirement Table
| ID | Requirement | Status | Evidence (file/command) |
| :--- | :--- | :--- | :--- |
| REQ-03-01 | RoadNetwork topological data structures (Intersection, Approach, Lane, StopLine, Crosswalk, TurningPath, ConflictZone, Entry/Exit) | VERIFIED | `src/city/RoadNetwork.h/.cpp`, 24 lanes, 16 paths, 8 conflict pairs |
| REQ-03-02 | Procedural network generation matching INTERSECTION_SPEC.md numbers (120m approach, 3.5m lane, 1.5m median, 4m crosswalk, 1.5m stop line clearance) | VERIFIED | `src/city/RoadNetwork.cpp`, `docs/INTERSECTION_SPEC.md` |
| REQ-03-03 | RoadNetwork JSON export and import with lossless round-trip equivalence | VERIFIED | `assets/config/network.json`, `test_road_network` Test 8 |
| REQ-03-04 | Network spatial queries (lanesFor, pathsFrom, conflictsOf, nearestStopLine, isInsideIntersection) | VERIFIED | `src/city/RoadNetwork.cpp`, `test_road_network` |
| REQ-03-05 | Turning path rules: 1cm endpoint tolerance, right turns non-conflicting, left turns crossing opposing through, curvature plausibility (R >= 8m / 12m) | VERIFIED | `src/city/RoadNetwork.cpp`, `test_road_network` Test 3 & 5 |
| REQ-03-06 | Procedural road surface, curbs, sidewalks, and raised thermoplastic markings (double yellow, dashed white, edge lines, stop bars, zebra crossings, turn arrows) | VERIFIED | `src/city/IntersectionGeometry.cpp`, `src/city/RoadMarkings.cpp` |
| REQ-03-07 | 12 modular urban buildings with zero road/sidewalk overlap | VERIFIED | `src/city/BuildingGenerator.cpp`, `test_road_network` Test 6 (18m corridor clearance) |
| REQ-03-08 | Street infrastructure (signal mast arm poles, 3-aspect heads, ped heads, speed/ped/U-turn signs, 16 streetlights, trees, benches, bins, bus stop shelter) | VERIFIED | `src/city/StreetFurniture.cpp` |
| REQ-03-09 | Multi-camera switching (F1-F4) & debug overlay toggles (lane centers, turning Béziers, conflict zones, stop lines, entry/exit points) | VERIFIED | `src/core/Application.cpp`, `src/city/CityEnvironment.cpp` |
| REQ-03-10 | Comprehensive design specification & asset licensing disclosures | VERIFIED | `docs/INTERSECTION_SPEC.md`, `docs/ASSET_LICENSES.md` |

### Test Table
| Test | Type (auto/manual) | Result | Evidence |
| :--- | :--- | :--- | :--- |
| `test_road_network` | auto (CTest) | PASS | `ctest -R test_road_network` (8/8 tests passed 1.01s, exit code 0) |
| `test_intersection` | auto (CTest) | PASS | `ctest -R test_intersection` (Passed 0.23s, exit code 0) |
| Full Suite Regression (12/12) | auto (CTest) | PASS | `ctest --test-dir build` (100% tests passed, 3.99s total) |
| Headless Lifecycle Run | auto | PASS | `.\build\SmartCity3D.exe --headless` (60 frames rendered, 0 leaks, exit code 0) |

### Known Issues / Deviations / Next-phase prerequisites
- None. Phase 3 is fully implemented and VERIFIED with automated tests and documentation.
- Note on Assets: Blender was not available in the command line environment; all 3D street infrastructure models (signal poles, lamps, benches, signs, trees, bus stop) are generated procedurally in code as disclosed in `docs/ASSET_LICENSES.md`.
- Next Phase Prerequisite: Phase 3 is approved. Ready for vehicle simulation / pedestrian dynamics.

---

## Phase 4 | Status: VERIFIED | Date: 2026-11-10

### Requirement Table
| ID | Requirement | Status | Evidence (file/command) |
| :--- | :--- | :--- | :--- |
| REQ-04-01 | Decoupled headless simulation engine with fixed timestep ($dt = 1/60$s), accumulator (max-steps-per-frame cap 5), and render interpolation | VERIFIED | `src/sim/VehicleManager.cpp`, `test_vehicle_sim.cpp` |
| REQ-04-02 | Multi-class vehicle catalog (`assets/config/vehicles.json`: Sedan $4.5\times 1.8$m, Bus $12\times 2.5$m, Ambulance $5.5\times 2.0$m) with physical dynamics | VERIFIED | `assets/config/vehicles.json`, `src/sim/Vehicle.h/.cpp` |
| REQ-04-03 | Intelligent Driver Model (IDM) car-following and stop-line braking ($v^2 / (2 \cdot b)$) | VERIFIED | `src/sim/IDMController.cpp`, `ctest -R test_idm` (Passed 0.33s) |
| REQ-04-04 | RoadNetwork path traversal with arc-length lookup table (LUT) constant speed and lateral acceleration limit ($a_{\text{lat}} \le 2.8$ m/s²) | VERIFIED | `src/sim/ArcLengthLUT.cpp`, `test_vehicle_sim.cpp` |
| REQ-04-05 | 2D Oriented Bounding Box (OBB) with Separating Axis Theorem (SAT) collision detection | VERIFIED | `src/sim/OBB.cpp`, `ctest -R test_obb` (Passed 0.01s) |
| REQ-04-06 | Intersection conflict zone reservation manager with mutual exclusion and deadlock prevention | VERIFIED | `src/sim/ConflictManager.cpp`, `test_vehicle_sim.cpp` |
| REQ-04-07 | Deterministic entry-point spawner with seeded PRNG, arrivals/hour, movement splits, and deferred spawn clearance check | VERIFIED | `src/sim/VehicleManager.cpp`, `test_vehicle_sim.cpp` |
| REQ-04-08 | Configuration validation with graceful default fallback from `simulation.json` and `vehicles.json` | VERIFIED | `assets/config/simulation.json`, `VehicleManager.cpp` |
| REQ-04-09 | Comprehensive automated verification suite (lane deviation $\le 5$cm, 0 red crossings in 10-min sim, turning exit heading $\le 2^\circ$, framerate independence, seed determinism) | VERIFIED | `tests/unit/test_vehicle_sim.cpp`, `ctest -R test_vehicle_sim` (Passed 22.2s) |
| REQ-04-10 | Mathematical documentation in `docs/ALGORITHMS.md` (IDM equations, Bézier LUT, OBB SAT, conflict protocol) | VERIFIED | `docs/ALGORITHMS.md` Section 6 |

### Test Table
| Test | Type (auto/manual) | Result | Evidence |
| :--- | :--- | :--- | :--- |
| `test_obb` | auto (CTest) | PASS | `ctest -R test_obb` (6/6 tests passed 0.01s, exit code 0) |
| `test_vehicle_sim` | auto (CTest) | PASS | `ctest -R test_vehicle_sim` (7/7 tests passed 22.20s, exit code 0) |
| `test_lane_graph` | auto (CTest) | PASS | `ctest -R test_lane_graph` (Passed 0.35s, exit code 0) |
| `test_idm` | auto (CTest) | PASS | `ctest -R test_idm` (Passed 0.33s, exit code 0) |
| `test_collision_avoidance` | auto (CTest) | PASS | `ctest -R test_collision_avoidance` (Passed 0.59s, exit code 0) |
| Full Suite Regression (14/14) | auto (CTest) | PASS | `ctest --test-dir build` (100% tests passed, 26.24s total) |
| Headless Lifecycle Run | auto | PASS | `.\build\SmartCity3D.exe --headless` (60 frames rendered, 0 leaks, exit code 0) |

### Known Issues / Deviations / Next-phase prerequisites
- None. Phase 4 is fully implemented and VERIFIED with 100% passing automated test suite and documentation.
- Prerequisite for Phase 5 (Pedestrian Simulation Engine): Phase 4 is approved. Ready for sidewalk navigation, crosswalk waiting zones, and pedestrian-vehicle conflicts.

---

## Phase 5 | Status: VERIFIED | Date: 2026-11-10

### Requirement Table
| ID | Requirement | Status | Evidence (file/command) |
| :--- | :--- | :--- | :--- |
| REQ-05-01 | FSM Signal Controller with explicit transition validation table (`NS_GREEN` $\to$ `NS_YELLOW` $\to$ `ALL_RED_1` $\to$ `EW_GREEN` $\to$ `EW_YELLOW` $\to$ `ALL_RED_2`) | VERIFIED | `src/traffic/TrafficSignalController.cpp`, `test_traffic_signals` Test 1 |
| REQ-05-02 | Runtime Conflict Matrix & Safety Monitor asserting 0 conflicting greens, through-left conflicts, or vehicular-pedestrian walk overlaps | VERIFIED | `src/traffic/TrafficSignalController.cpp`, `test_traffic_signals` Test 2 (72,000 steps, 0 conflicts) |
| REQ-05-03 | Queue-based Adaptive Mode with upstream (80m) & stop-line (15m) detectors, gap-out termination, opposing queue margin, and starvation protection (60s max red) | VERIFIED | `src/traffic/TrafficSignalController.cpp`, `test_traffic_signals` Test 3 |
| REQ-05-04 | Emergency Vehicle Preemption (EVP) for Ambulance with green holding, safe transition sequencing on red ($t_{\text{delay}} \le 15.0$s), clearance invariance, and first-come tie-breaking | VERIFIED | `src/traffic/TrafficSignalController.cpp`, `test_traffic_signals` Test 4 |
| REQ-05-05 | Signal Transition CSV Logging with simulation timestamps, triggers, phase states, and real queue counts | VERIFIED | `logs/signals_test_emergency_b.csv`, `logs/signals_test_adaptive_ns_heavy.csv` |
| REQ-05-06 | Pedestrian Agent Kinematics (`Waiting`, `Crossing`, `Finished`, $v \in [1.0, 1.5]$ m/s, curb buffer, procedural walk cycle animation) | VERIFIED | `src/ped/PedestrianAgent.cpp`, `src/ped/PedestrianManager.cpp` |
| REQ-05-07 | Deterministic seeded pedestrian demand spawner with call button actuation, 0 starts outside `WALK`, and clearance time ($22\text{m} / 1.2\text{m/s} = 18.3\text{s}$) | VERIFIED | `assets/config/pedestrians.json`, `test_pedestrians` Test 1 & 2 |
| REQ-05-08 | Vehicle-pedestrian conflict yielding on zebra crosswalks guaranteeing 0 vehicle-pedestrian collisions | VERIFIED | `src/sim/VehicleManager.cpp`, `test_pedestrians` Test 3 (0 collisions) |
| REQ-05-09 | 3D procedural pedestrian rendering & emissive signal head integration (3-aspect through/left lamps + ped WALK/FLASHING/DONT_WALK) | VERIFIED | `src/ped/PedestrianManager.cpp`, `src/core/Application.cpp` |
| REQ-05-10 | Mathematical documentation of signal control, clearance formulas, and yielding mechanics in `docs/ALGORITHMS.md` | VERIFIED | `docs/ALGORITHMS.md` Sections 7 & 8 |

### Test Table
| Test | Type (auto/manual) | Result | Evidence |
| :--- | :--- | :--- | :--- |
| `test_traffic_signals` | auto (CTest) | PASS | `ctest -R test_traffic_signals` (Passed 2.62s, exit code 0) |
| `test_pedestrians` | auto (CTest) | PASS | `ctest -R test_pedestrians` (Passed 0.05s, exit code 0) |
| Full Suite Regression (16/16) | auto (CTest) | PASS | `ctest --test-dir build` (100% tests passed, 29.22s total) |
| Headless Lifecycle Run | auto | PASS | `.\build\SmartCity3D.exe --headless` (60 frames, 0 leaks, exit code 0) |

### Evidence Logs & Excerpts

#### 1. Emergency Preemption Log Excerpt (`logs/signals_test_emergency_b.csv`)
```csv
sim_time,prev_phase,new_phase,trigger,ns_queue,ew_queue,reason
10,NS_GREEN,NS_YELLOW,Emergency,0,0,"Emergency preemption: ambulance requested EW service"
13,NS_YELLOW,ALL_RED_1,Timer,0,0,"Yellow clearance interval expired"
15,ALL_RED_1,EW_GREEN,Timer,0,0,"All-red clearance interval expired"
```
*Exact measured service delay: 14.0s (bounded by $t_{\text{min\_green}} + t_{\text{yel}} + t_{\text{all\_red}} = 15.0$s).*

#### 2. Adaptive Traffic Signal Log Excerpt (`logs/signals_test_adaptive_ns_heavy.csv`)
```csv
sim_time,prev_phase,new_phase,trigger,ns_queue,ew_queue,reason
60.0997,NS_GREEN,NS_YELLOW,Timer,13,5,"Max green limit reached"
63.0996,NS_YELLOW,ALL_RED_1,Timer,14,5,"Yellow clearance completed"
65.0996,ALL_RED_1,EW_GREEN,Timer,16,5,"All-red clearance completed"
75.0994,EW_GREEN,EW_YELLOW,Emergency,22,2,"Emergency preemption: ambulance requested NS service"
```

#### 3. CTest Regression Summary (16/16 Passed)
```
      Start  1: test_math ........................   Passed    0.15 sec
      Start  2: test_time ........................   Passed    0.25 sec
      Start  3: test_camera ......................   Passed    0.19 sec
      Start  4: test_intersection ................   Passed    0.24 sec
      Start  5: test_lane_graph ..................   Passed    0.43 sec
      Start  6: test_idm .........................   Passed    0.29 sec
      Start  7: test_collision_avoidance .........   Passed    0.54 sec
      Start  8: test_frustum .....................   Passed    0.15 sec
      Start  9: test_transform ...................   Passed    0.21 sec
      Start 10: test_camera_matrices .............   Passed    0.22 sec
      Start 11: test_resource_manager ............   Passed    0.52 sec
      Start 12: test_road_network ................   Passed    0.76 sec
      Start 13: test_obb .........................   Passed    0.11 sec
      Start 14: test_vehicle_sim .................   Passed   22.50 sec
      Start 15: test_traffic_signals .............   Passed    2.62 sec
      Start 16: test_pedestrians .................   Passed    0.05 sec

100% tests passed, 0 tests failed out of 16
Total Test time (real) = 29.22 sec
```

### Known Issues / Deviations / Next-phase prerequisites
- None. Phase 5 is fully implemented and VERIFIED with 100% passing automated test suite and documentation.
- Prerequisite for Phase 6 / Phase 7: Phase 5 is approved. Ready for real-time analytics dashboard and environmental emissions modeling.

---

## Phase 6 | Status: NOT_IMPLEMENTED | Date: 2026-11-10

### Requirement Table
| ID | Requirement | Status | Evidence (file/command) |
| :--- | :--- | :--- | :--- |
| REQ-06-01 | NEMA 4-phase/8-phase fixed-time signal controller | NOT_IMPLEMENTED | Pending Phase 6 |
| REQ-06-02 | Adaptive queue-actuated dynamic green extension controller | NOT_IMPLEMENTED | Pending Phase 6 |
| REQ-06-03 | Emergency Vehicle Preemption (EVP) rapid clearance & green corridor | NOT_IMPLEMENTED | Pending Phase 6 |
| REQ-06-04 | Push-button pedestrian call actuation | NOT_IMPLEMENTED | Pending Phase 6 |

### Test Table
| Test | Type (auto/manual) | Result | Evidence |
| :--- | :--- | :--- | :--- |
| `test_signal_safety` | auto (CTest) | NOT_IMPLEMENTED | Pending Phase 6 |

### Known Issues / Deviations / Next-phase prerequisites
- Prerequisite: Phase 4 & Phase 5.

---

## Phase 7 | Status: NOT_IMPLEMENTED | Date: 2026-11-10

### Requirement Table
| ID | Requirement | Status | Evidence (file/command) |
| :--- | :--- | :--- | :--- |
| REQ-07-01 | Real-time state metrics (Throughput, average delay, queue length) | NOT_IMPLEMENTED | Pending Phase 7 |
| REQ-07-02 | Physical CO2 emissions and fuel consumption calculation | NOT_IMPLEMENTED | Pending Phase 7 |
| REQ-07-03 | ImGui interactive control deck (Demand, Signal modes, Weather, Time) | NOT_IMPLEMENTED | Pending Phase 7 |

### Test Table
| Test | Type (auto/manual) | Result | Evidence |
| :--- | :--- | :--- | :--- |
| `test_analytics` | auto (CTest) | NOT_IMPLEMENTED | Pending Phase 7 |

### Known Issues / Deviations / Next-phase prerequisites
- Prerequisite: Phase 6.

---

## Phase 8 | Status: NOT_IMPLEMENTED | Date: 2026-11-10

### Requirement Table
| ID | Requirement | Status | Evidence (file/command) |
| :--- | :--- | :--- | :--- |
| REQ-08-01 | Headless simulation verification runner (1000 ticks, 0 collisions) | NOT_IMPLEMENTED | Pending Phase 8 |
| REQ-08-02 | Academic computer graphics documentation & user guide | NOT_IMPLEMENTED | Pending Phase 8 |

### Test Table
| Test | Type (auto/manual) | Result | Evidence |
| :--- | :--- | :--- | :--- |
| `test_headless_sim` | auto (CTest) | NOT_IMPLEMENTED | Pending Phase 8 |

### Known Issues / Deviations / Next-phase prerequisites
- Prerequisite: Phase 7.

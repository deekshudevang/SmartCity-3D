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

## Phase 2 | Status: VERIFIED | Date: 2026-11-10

### Requirement Table
| ID | Requirement | Status | Evidence (file/command) |
| :--- | :--- | :--- | :--- |
| REQ-02-01 | Shader subsystem with compilation/link diagnostics & fallbacks | VERIFIED | `src/gfx/Shader.h/.cpp`, `shaders/scene.vert/.frag` |
| REQ-02-02 | RAII OpenGL Buffer wrappers (VAO, VBO, EBO) and Mesh classes | VERIFIED | `src/gfx/Buffer.h/.cpp`, `src/gfx/Mesh.h/.cpp` |
| REQ-02-03 | 3D Camera system with View-Projection, Orbit/Fly, and Presets | VERIFIED | `src/gfx/Camera.h/.cpp`, radians internally, `tests/unit/test_camera.cpp` |
| REQ-02-04 | Shadow mapping with 2048x2048 Depth FBO and 3x3 PCF filter | VERIFIED | `src/gfx/ShadowMap.h/.cpp`, `shaders/shadow_depth.vert/.frag` |
| REQ-02-05 | Day/Night cycle with sun/moon vector and ambient shifts | VERIFIED | `src/gfx/Light.h/.cpp`, dynamic directional lighting |
| REQ-02-06 | Multi-pass rendering pipeline (Shadow pass, scene pass, UI overlay) | VERIFIED | `src/core/Application.cpp` |

### Test Table
| Test | Type (auto/manual) | Result | Evidence |
| :--- | :--- | :--- | :--- |
| `test_camera` | auto (CTest) | PASS | `ctest -R test_camera` (Passed 0.20s, exit code 0) |
| Graphics Pipeline Run | auto | PASS | `.\build\SmartCity3D.exe --headless` (FBO created, 60 frames rendered) |

### Known Issues / Deviations / Next-phase prerequisites
- None.

---

## Phase 3 | Status: VERIFIED | Date: 2026-11-10

### Requirement Table
| ID | Requirement | Status | Evidence (file/command) |
| :--- | :--- | :--- | :--- |
| REQ-03-01 | Detailed 4-way intersection roadway geometry (North, South, East, West) | VERIFIED | `src/city/IntersectionGeometry.h/.cpp` (North = -Z, East = +X) |
| REQ-03-02 | Elevated concrete sidewalks (Y = 0.15m) and vertical curb steps | VERIFIED | `src/city/IntersectionGeometry.cpp`, 4 corner pedestrian plazas |
| REQ-03-03 | Road markings: double solid yellow centerlines and dashed lane dividers | VERIFIED | `src/city/RoadMarkings.h/.cpp`, elevated Y = 0.015m (anti-z-fighting) |
| REQ-03-04 | Stop bars and pedestrian zebra crossing stripes across all 4 approaches | VERIFIED | `src/city/RoadMarkings.cpp`, 0.5m stripes at crosswalk distance (9.5m) |
| REQ-03-05 | Surrounding urban architecture (12 modular buildings with window grids) | VERIFIED | `src/city/BuildingGenerator.h/.cpp`, heights 22m to 56m |
| REQ-03-06 | Street furniture: 4 signal mast arms, streetlights with point lights, trees | VERIFIED | `src/city/StreetFurniture.h/.cpp`, 16 streetlight point lights |
| REQ-03-07 | Emissive 3-aspect signal heads and pedestrian walk/wait heads | VERIFIED | `src/city/CityEnvironment.cpp`, dynamic lens illumination |
| REQ-03-08 | Configuration externalized to files under `/assets/config` | VERIFIED | `assets/config/simulation_config.json`, `intersection_config.json` |

### Test Table
| Test | Type (auto/manual) | Result | Evidence |
| :--- | :--- | :--- | :--- |
| `test_intersection` | auto (CTest) | PASS | `ctest -R test_intersection` (Passed 0.27s, exit code 0) |
| Full Suite Regression | auto (CTest) | PASS | `ctest` (4/4 tests passed 100%, 0.98s total) |
| Headless City Execution | auto | PASS | `.\build\SmartCity3D.exe --headless` (Verified in `logs/smartcity.log`) |

### Known Issues / Deviations / Next-phase prerequisites
- None. All Phase 3 requirements verified with automated tests and headless execution logs. Ready for Phase 4.

---

## Phase 4 | Status: NOT_IMPLEMENTED | Date: 2026-11-10

### Requirement Table
| ID | Requirement | Status | Evidence (file/command) |
| :--- | :--- | :--- | :--- |
| REQ-04-01 | Waypoint lane graph with cubic Bézier turning curves (Left/Right/Thru) | NOT_IMPLEMENTED | Pending Phase 4 |
| REQ-04-02 | Multi-class vehicles (Sedan, SUV, Bus, Emergency) with physics | NOT_IMPLEMENTED | Pending Phase 4 |
| REQ-04-03 | Intelligent Driver Model (IDM) car-following acceleration/braking | NOT_IMPLEMENTED | Pending Phase 4 |
| REQ-04-04 | Collision avoidance: safe headway preservation and stop bar compliance | NOT_IMPLEMENTED | Pending Phase 4 |
| REQ-04-05 | Intersection conflict yielding & unprotected left-turn gap acceptance | NOT_IMPLEMENTED | Pending Phase 4 |
| REQ-04-06 | Emergency vehicle preemption siren and corridor clearing | NOT_IMPLEMENTED | Pending Phase 4 |

### Test Table
| Test | Type (auto/manual) | Result | Evidence |
| :--- | :--- | :--- | :--- |
| `test_lane_graph` | auto (CTest) | NOT_IMPLEMENTED | Pending Phase 4 |
| `test_idm` | auto (CTest) | NOT_IMPLEMENTED | Pending Phase 4 |
| `test_collision_avoidance` | auto (CTest) | NOT_IMPLEMENTED | Pending Phase 4 |

### Known Issues / Deviations / Next-phase prerequisites
- Prerequisite: Phase 3 VERIFIED.

---

## Phase 5 | Status: NOT_IMPLEMENTED | Date: 2026-11-10

### Requirement Table
| ID | Requirement | Status | Evidence (file/command) |
| :--- | :--- | :--- | :--- |
| REQ-05-01 | Pedestrian agent navigation on sidewalks | NOT_IMPLEMENTED | Pending Phase 5 |
| REQ-05-02 | Crosswalk waiting zones at curbs | NOT_IMPLEMENTED | Pending Phase 5 |
| REQ-05-03 | Crosswalk traversal during WALK signal | NOT_IMPLEMENTED | Pending Phase 5 |
| REQ-05-04 | Vehicle-pedestrian conflict yielding on zebra crossings | NOT_IMPLEMENTED | Pending Phase 5 |

### Test Table
| Test | Type (auto/manual) | Result | Evidence |
| :--- | :--- | :--- | :--- |
| `test_pedestrian_nav` | auto (CTest) | NOT_IMPLEMENTED | Pending Phase 5 |

### Known Issues / Deviations / Next-phase prerequisites
- Prerequisite: Phase 4.

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

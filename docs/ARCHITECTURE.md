# SmartCity 3D — Graphics Engine Architecture

## 1. System Overview

SmartCity 3D separates engine systems into distinct, decoupled domains:
- **`core`**: Application lifecycle, window management, event dispatch, fixed-timestep simulation loop, configuration parsing, and leveled logging.
- **`gfx`**: Hardware rendering abstractions, shaders, materials, lights, shadow mapping, camera transformations, view-frustum culling, and skybox rendering.
- **`scene`**: Hierarchical scene graph, spatial transforms, model loading, and lifecycle-managed resource caching.
- **`city`**: Procedural urban intersection, road markings, sidewalk and crosswalk geometry, buildings, streetlights, and traffic light assemblies.
- **`sim`**: Microscopic traffic simulation, multi-class vehicles, Intelligent Driver Model (IDM), conflict zones, and lane graph navigation.

---

## 2. Core Class Diagram

```mermaid
classDiagram
    class Application {
        -Window m_window
        -Camera m_camera
        -Renderer m_renderer
        -Scene m_scene
        -ResourceManager m_resourceManager
        -CityEnvironment m_city
        -TrafficManager m_traffic
        +Run() int
    }

    class Renderer {
        -RenderStats m_stats
        -vector~RenderItem~ m_renderQueue
        -Frustum m_cameraFrustum
        +BeginFrame(Camera, LightingEnvironment, clearColor)
        +Submit(Mesh, Material, Transform, NormalMatrix)
        +Submit(SceneObject)
        +EndFrame()
        +SetFrustumCullingEnabled(bool)
    }

    class Scene {
        -vector~shared_ptr~SceneObject~~ m_objects
        +CreateObject(name) shared_ptr~SceneObject~
        +AddObject(SceneObject)
        +RemoveObject(id) bool
        +FindObjectById(id) shared_ptr~SceneObject~
    }

    class SceneObject {
        -uint32_t m_id
        -string m_name
        -Transform m_transform
        -shared_ptr~Model~ m_model
        -shared_ptr~Mesh~ m_mesh
        -shared_ptr~Material~ m_material
        -AABB m_localAABB
        -bool m_isVisible
        +GetWorldAABB() AABB
    }

    class Transform {
        -vec3 m_position
        -quat m_rotation
        -vec3 m_scale
        -Transform* m_parent
        -bool m_isDirty
        +GetWorldMatrix() mat4
        +GetNormalMatrix() mat3
    }

    class Camera {
        -ProjectionType m_projType
        -CameraMode m_mode
        -mat4 m_viewMatrix
        -mat4 m_projectionMatrix
        +GetFrustum() Frustum
        +ToggleProjectionType()
        +SetOrthoSize(size)
    }

    class Frustum {
        -Plane m_planes[6]
        +ExtractFromMatrix(mat4 viewProj)
        +IntersectsAABB(AABB) bool
        +IntersectsSphere(vec3 center, radius) bool
    }

    class Material {
        -uint32_t m_id
        -vec3 m_ambient
        -vec3 m_diffuse
        -vec3 m_specular
        -float m_shininess
        -shared_ptr~Shader~ m_shader
        +GetSortKey() uint64_t
        +Bind()
    }

    class ResourceManager {
        -map~string, shared_ptr~Shader~~ m_shaders
        -map~string, shared_ptr~Texture2D~~ m_textures
        -map~string, shared_ptr~Model~~ m_models
        -map~string, shared_ptr~Material~~ m_materials
        +GetModel(path) shared_ptr~Model~
        +GetShader(name, vert, frag) shared_ptr~Shader~
        +Shutdown()
    }

    Application --> Renderer
    Application --> Scene
    Application --> Camera
    Application --> ResourceManager
    Scene o-- SceneObject
    SceneObject *-- Transform
    SceneObject o-- Material
    Renderer ..> Frustum : Frustum Culling
    Renderer ..> Material : Sort Queue
```

---

## 3. Per-Frame Execution Pipeline

Every frame executes through six sequential stages:

```
[ Update Phase ]
      │
      ▼
[ 1. Input & Fixed-Timestep Simulation Update ]
      │  ├─ Poll GLFW window events and mouse/keyboard deltas
      │  ├─ Update Camera matrices and FreeFly / Orbit state
      │  └─ While timer.ShouldSimulateStep():
      │       ├─ Update DayNight sun angle & ambient conditions
      │       └─ Advance TrafficManager (IDM, signals, vehicle positions)
      ▼
[ 2. Pass 1: Shadow Depth Pass ]
      │  ├─ Bind ShadowMap Framebuffer (2048 x 2048 depth-only)
      │  ├─ Calculate Directional Light-Space Orthographic Matrix
      │  └─ Render City and Vehicle geometry to depth texture
      ▼
[ 3. Pass 2: Frustum Culling & Render Queue Submission ]
      │  ├─ Extract 6 Gribb-Hartmann planes from Camera View-Projection Matrix
      │  ├─ Iterate SceneObjects:
      │  │    ├─ Compute transformed World AABB
      │  │    ├─ Test against Frustum planes
      │  │    ├─ Culled? -> increment culledObjects, skip
      │  │    └─ Visible? -> push RenderItems to sorted Render Queue
      │  └─ Sort Render Queue by 64-bit key: (ShaderID << 32) | MaterialID
      ▼
[ 4. Pass 2: Scene Geometry Render Pass ]
      │  ├─ Bind Default Framebuffer & Clear Color/Depth buffers
      │  ├─ Configure Depth Test (GL_LESS) & Face Culling (GL_BACK, GL_CCW)
      │  ├─ Execute sorted Render Queue (batching Shader and Material state changes)
      │  ├─ Render City infrastructure & vehicles with Blinn-Phong lighting
      │  └─ Unbind textures and shaders
      ▼
[ 5. Pass 2: Skybox Render Pass ]
      │  ├─ Set glDepthFunc(GL_LEQUAL)
      │  ├─ Remove translation from View Matrix (mat4(mat3(view)))
      │  ├─ Render skybox cube / procedural gradient (depth z=w=1.0)
      │  └─ Restore glDepthFunc(GL_LESS)
      ▼
[ 6. Pass 3: Diagnostics & ImGui UI Overlay ]
      │  ├─ Calculate rolling 60-frame Avg/Min/Max FPS and frame ms
      │  ├─ Render control panels (Camera, Lighting, Material Inspector, Traffic)
      │  ├─ ImGui_ImplOpenGL3_RenderDrawData()
      │  └─ glfwSwapBuffers()
```

---

## 4. Ownership, Memory & Lifetime Rules

1. **No Raw Owning Pointers**:
   - All OpenGL buffers, textures, shaders, and framebuffers are wrapped in RAII classes (`VertexArray`, `VertexBuffer`, `IndexBuffer`, `Texture2D`, `Shader`, `ShadowMap`).
   - Destructors call the appropriate `glDelete*` function only while the OpenGL context remains valid.

2. **Resource Manager Centralization**:
   - Assets (textures, shaders, 3D models, materials) are loaded once through `ResourceManager::Get()`.
   - Returns immutable `std::shared_ptr<T>`.
   - Repeated requests for the same path return the cached pointer without disk I/O.
   - On `ResourceManager::Shutdown()`, maps are cleared and ref-counts are inspected; a leak summary is logged confirming zero dangling resources.

3. **Scene Graph Ownership**:
   - `scene::Scene` owns its top-level `std::shared_ptr<SceneObject>` instances.
   - Child objects are owned by their respective parents via `std::shared_ptr<SceneObject>`.
   - `Transform::SetParent()` maintains raw back-pointers to parent transforms without circular ownership.
   - Clearing or destroying a `Scene` releases only the objects it holds without destroying shared models or textures in `ResourceManager`.


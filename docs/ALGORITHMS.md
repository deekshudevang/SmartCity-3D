# SmartCity 3D — Mathematical & Algorithmic Formulations

This document specifies the core mathematical formulations, equations, and algorithmic pipelines implemented across the SmartCity 3D graphics engine.

---

## 1. Coordinate Space Transformations (MVP)

A 3D coordinate undergoes four sequential matrix transformations from local object space to normalized device coordinates (NDC):

$$\mathbf{v}_{\text{clip}} = \mathbf{P} \cdot \mathbf{V} \cdot \mathbf{M} \cdot \mathbf{v}_{\text{local}}$$

1. **Model Matrix ($\mathbf{M}$)**:
   Transforms local mesh vertices into world coordinates via translation ($\mathbf{T}$), quaternion rotation ($\mathbf{R}$), and scale ($\mathbf{S}$):
   $$\mathbf{M} = \mathbf{T} \cdot \mathbf{R} \cdot \mathbf{S}$$

2. **Normal Matrix ($\mathbf{N}$)**:
   Preserves geometric orthogonality under non-uniform scaling:
   $$\mathbf{N} = \left(\mathbf{M}_{3\times3}^{-1}\right)^T$$
   Computed on the CPU to avoid expensive per-vertex matrix inversions in GLSL shaders.

3. **View Matrix ($\mathbf{V}$)**:
   Transforms world coordinates into camera-centric eye space using standard Gram-Schmidt orthonormalization:
   $$\mathbf{V} = \begin{bmatrix}
   R_x & R_y & R_z & -\mathbf{R} \cdot \mathbf{P}_{\text{cam}} \\
   U_x & U_y & U_z & -\mathbf{U} \cdot \mathbf{P}_{\text{cam}} \\
   -F_x & -F_y & -F_z & \mathbf{F} \cdot \mathbf{P}_{\text{cam}} \\
   0 & 0 & 0 & 1
   \end{bmatrix}$$
   where $\mathbf{F} = \text{normalize}(\mathbf{T}_{\text{cam}} - \mathbf{P}_{\text{cam}})$, $\mathbf{R} = \text{normalize}(\mathbf{F} \times \mathbf{U}_{\text{world}})$, and $\mathbf{U} = \mathbf{R} \times \mathbf{F}$.

4. **Projection Matrix ($\mathbf{P}$)**:
   - **Perspective Mode**:
     $$\mathbf{P}_{\text{persp}} = \begin{bmatrix}
     \frac{1}{\text{aspect} \cdot \tan(\theta / 2)} & 0 & 0 & 0 \\
     0 & \frac{1}{\tan(\theta / 2)} & 0 & 0 \\
     0 & 0 & -\frac{z_{\text{far}} + z_{\text{near}}}{z_{\text{far}} - z_{\text{near}}} & -\frac{2 \cdot z_{\text{far}} \cdot z_{\text{near}}}{z_{\text{far}} - z_{\text{near}}} \\
     0 & 0 & -1 & 0
     \end{bmatrix}$$

   - **Orthographic Mode**:
     For symmetric bounds $[ -w/2, w/2 ]$ and $[ -h/2, h/2 ]$:
     $$\mathbf{P}_{\text{ortho}} = \begin{bmatrix}
     \frac{2}{w} & 0 & 0 & 0 \\
     0 & \frac{2}{h} & 0 & 0 \\
     0 & 0 & -\frac{2}{z_{\text{far}} - z_{\text{near}}} & -\frac{z_{\text{far}} + z_{\text{near}}}{z_{\text{far}} - z_{\text{near}}} \\
     0 & 0 & 0 & 1
     \end{bmatrix}$$

---

## 2. Illumination Models: Phong vs. Blinn-Phong

Given surface unit normal $\hat{\mathbf{n}}$, unit light direction vector $\hat{\mathbf{l}} = \text{normalize}(\mathbf{P}_{\text{light}} - \mathbf{P}_{\text{frag}})$, and view vector $\hat{\mathbf{v}} = \text{normalize}(\mathbf{P}_{\text{cam}} - \mathbf{P}_{\text{frag}})$:

### Diffuse Reflection (Lambertian)
$$I_{\text{diffuse}} = \max(\hat{\mathbf{n}} \cdot \hat{\mathbf{l}}, 0.0) \cdot \mathbf{C}_{\text{light}} \cdot \mathbf{K}_{\text{diffuse}}$$

### Specular Reflection

1. **Classical Phong Reflection**:
   Uses the ideal reflection vector $\hat{\mathbf{r}} = 2(\hat{\mathbf{n}} \cdot \hat{\mathbf{l}})\hat{\mathbf{n}} - \hat{\mathbf{l}}$:
   $$I_{\text{specular, Phong}} = \left[\max(\hat{\mathbf{v}} \cdot \hat{\mathbf{r}}, 0.0)\right]^{\alpha} \cdot \mathbf{C}_{\text{light}} \cdot \mathbf{K}_{\text{specular}}$$

2. **Blinn-Phong Reflection**:
   Uses the halfway vector $\hat{\mathbf{h}} = \text{normalize}(\hat{\mathbf{l}} + \hat{\mathbf{v}})$:
   $$I_{\text{specular, Blinn-Phong}} = \left[\max(\hat{\mathbf{n}} \cdot \hat{\mathbf{h}}, 0.0)\right]^{4\alpha} \cdot \mathbf{C}_{\text{light}} \cdot \mathbf{K}_{\text{specular}}$$
   *Advantage*: Avoids angle cutoff artifacts when viewing surfaces at glancing angles ($>90^\circ$ between reflection and eye vector).

---

## 3. Light Attenuation & Spotlight Formulations

### Point Light Attenuation
Distance $d = \|\mathbf{P}_{\text{light}} - \mathbf{P}_{\text{frag}}\|$. Attenuation factor:
$$f_{\text{att}}(d) = \frac{1.0}{k_c + k_l \cdot d + k_q \cdot d^2}$$
where standard physically plausible coefficients are $k_c = 1.0$, $k_l = 0.09$, $k_q = 0.032$.

### Spot Light Angular Falloff
Let $\hat{\mathbf{d}}_{\text{spot}}$ be the spotlight orientation vector.
The cosine of the angle between light direction and spot axis is:
$$\theta = \text{dot}(-\hat{\mathbf{l}}, \hat{\mathbf{d}}_{\text{spot}})$$

Given inner cutoff cosine $\cos(\phi_{\text{inner}})$ and outer cutoff cosine $\cos(\phi_{\text{outer}})$:
$$\epsilon = \cos(\phi_{\text{inner}}) - \cos(\phi_{\text{outer}})$$
$$I_{\text{spot}} = \text{clamp}\left(\frac{\theta - \cos(\phi_{\text{outer}})}{\epsilon}, 0.0, 1.0\right)$$
Provides smooth Hermite interpolation between full illumination and zero light at the cone boundary.

---

## 4. Gribb-Hartmann Frustum Plane Extraction

Given the composite View-Projection matrix $\mathbf{M} = \mathbf{P} \cdot \mathbf{V}$ represented by column vectors $\mathbf{m}_{\text{col}, i}$ ($i \in \{0, 1, 2, 3\}$), the 6 clip-space bounding planes $A x + B y + C z + D = 0$ are directly extracted:

| Plane | Row Vector Formulation |
| :--- | :--- |
| **Left Plane** | $\mathbf{Plane}_{\text{Left}} = \mathbf{row}_3(\mathbf{M}) + \mathbf{row}_0(\mathbf{M})$ |
| **Right Plane** | $\mathbf{Plane}_{\text{Right}} = \mathbf{row}_3(\mathbf{M}) - \mathbf{row}_0(\mathbf{M})$ |
| **Bottom Plane** | $\mathbf{Plane}_{\text{Bottom}} = \mathbf{row}_3(\mathbf{M}) + \mathbf{row}_1(\mathbf{M})$ |
| **Top Plane** | $\mathbf{Plane}_{\text{Top}} = \mathbf{row}_3(\mathbf{M}) - \mathbf{row}_1(\mathbf{M})$ |
| **Near Plane** | $\mathbf{Plane}_{\text{Near}} = \mathbf{row}_3(\mathbf{M}) + \mathbf{row}_2(\mathbf{M})$ |
| **Far Plane** | $\mathbf{Plane}_{\text{Far}} = \mathbf{row}_3(\mathbf{M}) - \mathbf{row}_2(\mathbf{M})$ |

Each extracted 4D vector $(A, B, C, D)$ is normalized:
$$L = \sqrt{A^2 + B^2 + C^2}, \quad (A', B', C', D') = \left(\frac{A}{L}, \frac{B}{L}, \frac{C}{L}, \frac{D}{L}\right)$$

### AABB vs. Frustum Culling Test (Arvo's Method)
For each frustum plane $\mathbf{p} = (A', B', C', D')$ and axis-aligned bounding box defined by $\mathbf{min}$ and $\mathbf{max}$:
The positive vertex $\mathbf{v}_p$ (most aligned in the direction of the plane normal) is chosen component-wise:
$$v_{p, i} = \begin{cases} \max_i & \text{if } p_i \ge 0 \\ \min_i & \text{if } p_i < 0 \end{cases}$$

If for any of the 6 planes:
$$A' \cdot v_{p, x} + B' \cdot v_{p, y} + C' \cdot v_{p, z} + D' < 0$$
then the bounding box lies completely outside the frustum and is culled immediately.

---

## 5. Skybox Far Plane Depth & Procedural Atmospheric Gradient

1. **Far Plane Projection Trick**:
   In `skybox.vert`, output position is calculated such that:
   $$\mathbf{p}_{\text{clip}} = \mathbf{P}_{\text{cam}} \cdot \mathbf{V}_{\text{no-trans}} \cdot \mathbf{v}_{\text{cube}}$$
   $$\text{gl\_Position} = \mathbf{p}_{\text{clip}}.xyww$$
   Since NDC depth is computed as $z_{\text{ndc}} = \frac{z_{\text{clip}}}{w_{\text{clip}}} = \frac{w}{w} = 1.0$, the skybox fragments are always rasterized at the absolute far depth plane. Drawing with `glDepthFunc(GL_LEQUAL)` ensures it renders behind all opaque geometry.

2. **Procedural Atmospheric Falloff**:
   When cubemap textures are not present, an atmospheric scattering gradient is evaluated procedurally in `skybox.frag` based on normalized elevation angle:
   $$\hat{\mathbf{d}} = \text{normalize}(\mathbf{v}_{\text{TexCoord}})$$
   $$t = \text{clamp}(\hat{d}_y, 0.0, 1.0)$$
   $$\mathbf{C}_{\text{sky}} = \text{mix}\left(\mathbf{C}_{\text{horizon}}, \mathbf{C}_{\text{zenith}}, t^{0.6}\right)$$
   For fragments below horizon ($\hat{d}_y < 0$):
   $$\mathbf{C}_{\text{sky}} = \text{mix}\left(\mathbf{C}_{\text{horizon}}, \mathbf{C}_{\text{ground}}, (-\hat{d}_y)^{0.5}\right)$$


---

## 6. Vehicle Simulation & Traffic Dynamics

### 6.1 Intelligent Driver Model (IDM) Car-Following Formulation

Longitudinal acceleration for each vehicle agent is governed by the continuous Intelligent Driver Model (Treiber et al.):

$$\dot{v} = a \left[ 1 - \left(\frac{v}{v_0}\right)^\delta - \left(\frac{s^*(v, \Delta v)}{s}\right)^2 \right]$$

where:
- $v$: current vehicle speed ($\text{m/s}$)
- $v_0$: desired free-flow velocity ($\text{m/s}$)
- $a$: maximum acceleration limit ($\text{m/s}^2$)
- $b$: comfortable deceleration rate ($\text{m/s}^2$)
- $\delta$: free-acceleration exponent (standard $\delta = 4$)
- $s$: net bumper-to-bumper distance to leader vehicle ($\text{m}$)
- $\Delta v = v - v_{\text{lead}}$: closing speed relative to leader ($\text{m/s}$)

The dynamic desired bumper-to-bumper gap $s^*(v, \Delta v)$ is computed as:

$$s^*(v, \Delta v) = s_0 + v T + \frac{v \cdot \Delta v}{2 \sqrt{a \cdot b}}$$

where $s_0$ is the minimum jam distance at standstill and $T$ is the safe time headway.

#### Virtual Stop-Line Deceleration
When approaching a red signal, yellow signal outside the dilemma zone, or an unreserved conflict zone, the stop bar acts as a virtual stationary obstacle ($v_{\text{lead}} = 0$, $\Delta v = v$):

$$s_{\text{stop}}^*(v) = s_0 + v T + \frac{v^2}{2\sqrt{a \cdot b}}$$

#### Yellow Light Dilemma Zone Evaluation
A vehicle evaluates whether it can comfortably stop before the stop bar without exceeding maximum comfortable deceleration:

$$d_{\text{min, stop}} = \frac{v^2}{2 \cdot b}$$

If distance to stop bar $d_{\text{stop}} < d_{\text{min, stop}} + v \cdot t_{\text{reaction}}$, the vehicle is inside the dilemma zone and proceeds through the intersection safely.

---

### 6.2 Arc-Length Parameterized Cubic Bézier Traversal

Intersection turning movements are modeled by cubic Bézier curves:

$$\mathbf{B}(t) = (1-t)^3 \mathbf{P}_0 + 3(1-t)^2 t \mathbf{P}_1 + 3(1-t) t^2 \mathbf{P}_2 + t^3 \mathbf{P}_3, \quad t \in [0, 1]$$

#### Arc-Length Lookup Table (LUT) Inversion
To enforce constant physical vehicle speed $v = \frac{ds}{dt}$ independent of polynomial parameterization non-linearities, a 100-segment lookup table discretizes cumulative arc length:

$$s_k = \sum_{i=1}^k \|\mathbf{B}(t_i) - \mathbf{B}(t_{i-1})\|$$

Given an integrated physical distance $s \in [0, s_N]$, the curve parameter $t$ is retrieved in $O(\log N)$ via binary search:

$$t(s) = t_k + \frac{s - s_k}{s_{k+1} - s_k} (t_{k+1} - t_k)$$

#### Lateral Acceleration Constraint
Vehicle velocity during turning maneuvers is dynamically constrained by lateral acceleration limits:

$$a_{\text{lat}} = v^2 \cdot \kappa \le a_{\text{lat, limit}}$$

where planar curve curvature $\kappa(t)$ is derived analytically from first and second derivatives:

$$\kappa(t) = \frac{|x'(t) z''(t) - z'(t) x''(t)|}{\left(x'(t)^2 + z'(t)^2\right)^{3/2}}$$

The maximum permissible cornering speed is:

$$v_{\text{turn, max}} = \sqrt{\frac{a_{\text{lat, limit}}}{\max(\kappa(t), \epsilon)}}$$

Vehicles decelerate prior to entering the curve to guarantee that $a_{\text{lat}} \le a_{\text{lat, limit}} = 2.8\text{ m/s}^2$ everywhere.

---

### 6.3 2D Oriented Bounding Box (OBB) & Separating Axis Theorem (SAT)

Every vehicle maintains a 2D Oriented Bounding Box in the ground XZ plane defined by center $\mathbf{c} \in \mathbb{R}^2$, half-extents $\mathbf{e} = (L/2, W/2)$, and orthonormal orientation axes:

$$\hat{\mathbf{u}}_{\text{long}} = (\sin\theta, \cos\theta), \quad \hat{\mathbf{u}}_{\text{lat}} = (\cos\theta, -\sin\theta)$$

Collision detection between two boxes $A$ and $B$ evaluates 4 potential separating axes ($\hat{\mathbf{u}}_{A, \text{long}}, \hat{\mathbf{u}}_{A, \text{lat}}, \hat{\mathbf{u}}_{B, \text{long}}, \hat{\mathbf{u}}_{B, \text{lat}}$). For each candidate axis $\hat{\mathbf{a}}$:

$$\text{dist} = |(\mathbf{c}_B - \mathbf{c}_A) \cdot \hat{\mathbf{a}}|$$

$$r_A = e_{A, \text{long}} |\hat{\mathbf{a}} \cdot \hat{\mathbf{u}}_{A, \text{long}}| + e_{A, \text{lat}} |\hat{\mathbf{a}} \cdot \hat{\mathbf{u}}_{A, \text{lat}}|$$

$$r_B = e_{B, \text{long}} |\hat{\mathbf{a}} \cdot \hat{\mathbf{u}}_{B, \text{long}}| + e_{B, \text{lat}} |\hat{\mathbf{a}} \cdot \hat{\mathbf{u}}_{B, \text{lat}}|$$

If $\text{dist} > r_A + r_B$ for any axis $\hat{\mathbf{a}}$, the boxes are separated. If no separating axis exists across all 4 directions, a geometric overlap is detected.

---

### 6.4 Intersection Conflict Zone Mutual Exclusion

To prevent intersection side-swipes and broadside collisions, the intersection core is partitioned into conflict zones.
- Vehicles request reservations on all conflict zones intersecting their turning trajectory prior to entering the intersection ($d_{\text{stop}} \le 14\text{ m}$).
- A reservation is granted if and only if all required zones are currently unreserved by any other vehicle.
- If any required zone is occupied, the vehicle halts at the stop line ($v = 0$, state `Waiting`).
- Reservations are released when the rear bumper clears the intersection and enters the departure lane.
- Mutual exclusion is guaranteed: $\forall t, \text{occupants}(\text{Zone}_k) \le 1$.

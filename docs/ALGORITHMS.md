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

---

## 7. Traffic Signal Control & Safety Architecture

### 7.1 Finite-State Machine & Legal Transition Table

The traffic intersection signal controller operates as a deterministic finite-state machine (FSM) cycling through six primary phases and coordinated pedestrian intervals:

$$\mathcal{S} = \{\text{NS\_GREEN}, \text{NS\_YELLOW}, \text{ALL\_RED\_1}, \text{EW\_GREEN}, \text{EW\_YELLOW}, \text{ALL\_RED\_2}\}$$

The transition relation $\mathcal{T}: \mathcal{S} \times \mathcal{S} \to \{0, 1\}$ strictly enforces the sequence:

$$\text{NS\_GREEN} \xrightarrow{\tau_{\text{green}}} \text{NS\_YELLOW} \xrightarrow{t_{\text{yel}}} \text{ALL\_RED\_1} \xrightarrow{t_{\text{red}}} \text{EW\_GREEN} \xrightarrow{\tau_{\text{green}}} \text{EW\_YELLOW} \xrightarrow{t_{\text{yel}}} \text{ALL\_RED\_2} \xrightarrow{t_{\text{red}}} \text{NS\_GREEN}$$

Any out-of-order transition is rejected as an illegal transition and asserted as a fatal error in Debug builds. Clearance intervals ($t_{\text{yel}}$ and $t_{\text{red}}$) and minimum green ($t_{\text{min\_green}}$) are invariant and can never be shortened by preemption or adaptive logic.

### 7.2 All-Red Clearance Interval Formulation

The all-red clearance interval guarantees that a vehicle legally crossing the stop line at the end of the yellow interval completely exits the conflicting intersection area before the conflicting approach receives green:

$$t_{\text{all\_red}} = \frac{W_{\text{int}} + L_{\text{veh}}}{v_{\text{design}}}$$

Where:
- $W_{\text{int}} = 28.0\text{ m}$ (stop line to opposing crosswalk clear boundary).
- $L_{\text{veh}} = 5.0\text{ m}$ (standard vehicle clearance length).
- $v_{\text{design}} = 14.0\text{ m/s}$ ($50\text{ km/h}$ intersection design speed).
- $t_{\text{all\_red}} = \frac{28.0 + 5.0}{14.0} = 2.36\text{ s} \approx 2.0 - 2.5\text{ s}$.

### 7.3 Pedestrian Clearance Formulation

Pedestrian crossing signals feature three distinct aspects: `WALK`, `FLASHING_DONT_WALK`, and `DONT_WALK`. The clearance interval $t_{\text{ped\_clear}}$ during `FLASHING_DONT_WALK` guarantees that any pedestrian who stepped into the crosswalk during `WALK` reaches the destination curb prior to the conflicting vehicular phase:

$$t_{\text{ped\_clear}} = \frac{L_{\text{crosswalk}}}{v_{\text{ped}}}$$

With roadway width $L_{\text{crosswalk}} = 22.0\text{ m}$ and design walking speed $v_{\text{ped}} = 1.2\text{ m/s}$:

$$t_{\text{ped\_clear}} = \frac{22.0}{1.2} = 18.33\text{ s}$$

During $t \in [0, t_{\text{walk}})$, the head displays `WALK`. During $t \in [t_{\text{walk}}, t_{\text{walk}} + t_{\text{ped\_clear}})$, the head displays `FLASHING_DONT_WALK`. Waiting pedestrians at the curb are forbidden from entering during flashing or solid `DONT_WALK`.

### 7.4 Conflict Matrix & Runtime Safety Monitor

At every simulation step $\Delta t$, the safety monitor evaluates the active signal aspects across all conflicting traffic movements:
- Conflicting through movements: $(\text{Aspect}(\text{North}) \in \{\text{G}, \text{Y}\} \lor \text{Aspect}(\text{South}) \in \{\text{G}, \text{Y}\}) \land (\text{Aspect}(\text{East}) \in \{\text{G}, \text{Y}\} \lor \text{Aspect}(\text{West}) \in \{\text{G}, \text{Y}\}) \implies \bot$
- Conflicting through and left turns: $(\text{Through}(\text{NS}) = \text{G} \land \text{Left}(\text{EW}) = \text{G}) \implies \bot$
- Pedestrian through conflict: $(\text{Through}(\text{NS}) = \text{G} \land \text{Aspect}(\text{CW\_North}) = \text{WALK}) \implies \bot$

If any conflicting condition evaluates to true, a fatal assertion is raised in Debug, and in Release the controller instantly logs a critical safety violation and forces the intersection to `ALL_RED`.

### 7.5 Adaptive Queue Decision Rules

In Adaptive mode, upstream and stop-line detectors monitor active queues on all approaches:
1. **Minimum Green Assurance**: While $t_{\text{phase}} < t_{\text{min\_green}}$, the phase remains green regardless of demand.
2. **Starvation Protection**: If the opposing approach has experienced continuous red for $t_{\text{red}} \ge t_{\text{starvation\_max}} = 60.0\text{ s}$ and opposing queue $Q_{\text{opp}} > 0$, the controller initiates a transition to yellow immediately.
3. **Gap-Out Detection**: If the active queue on the green approach clears ($Q_{\text{curr}} = 0$) and $t_{\text{phase}} \ge t_{\text{min\_green}}$, green is terminated.
4. **Demand Imbalance Switching**: If $Q_{\text{opp}} \ge Q_{\text{curr}} + \Delta Q_{\text{margin}}$ (with $\Delta Q_{\text{margin}} = 2$), green transitions to yellow to serve higher pending demand.
5. **Maximum Green Cap**: If $t_{\text{phase}} \ge t_{\text{max\_green}} = 60.0\text{ s}$, green terminates unconditionally.
6. **Green Extension**: Otherwise, green is extended by unit increment $t_{\text{ext}} = 3.0\text{ s}$ up to $t_{\text{max\_green}}$.

### 7.6 Emergency Vehicle Preemption & Tie-Breaking

Ambulances approaching within $d_{\text{detect}} \le 100\text{ m}$ of the stop line request priority:
- **Green Approach**: If the ambulance approach is already green, green duration is extended up to $t_{\text{max\_green}} + t_{\text{ext\_cap}}$ ($60\text{ s} + 15\text{ s} = 75\text{ s}$).
- **Red Approach**: The current phase is held until $t_{\text{min\_green}}$ is met, then safely transitions $\text{GREEN} \to \text{YELLOW} \to \text{ALL\_RED} \to \text{AMBULANCE\_GREEN}$. Service is guaranteed within bounded delay $t_{\text{delay}} \le t_{\text{min\_green}} + t_{\text{yel}} + t_{\text{red}} = 15.0\text{ s}$.
- **Simultaneous Conflict Resolution**: If multiple emergency vehicles request priority simultaneously on conflicting approaches, priority is granted first to the vehicle closest to the stop line ($d_{\text{stop1}} < d_{\text{stop2}}$) or oldest arrival timestamp. The second vehicle is queued and served at the subsequent safe transition.

---

## 8. Pedestrian Dynamics & Yielding Safety

### 8.1 Pedestrian Kinematic State Machine

Pedestrian agents transition through three strict topological states:
1. `Waiting`: Agents arrive from sidewalks and halt at the curb waiting buffer ($d_{\text{curb}} = 0.5\text{ m}$ back from the roadway edge, $Y = 0.15\text{ m}$). Agents press the pedestrian call button upon arrival.
2. `Crossing`: When the crossing aspect turns `WALK`, agents step down to road grade ($Y = 0.02\text{ m}$) and traverse the crosswalk polyline at individual velocity $v \sim \mathcal{U}(1.0, 1.5)\text{ m/s}$.
3. `Finished`: Upon reaching the destination curb, agents step up to sidewalk grade ($Y = 0.15\text{ m}$) and complete their journey.

### 8.2 Vehicle-Pedestrian Conflict Yielding

Vehicles performing left or right turns across an active crosswalk check for pedestrian agents in the crossing corridor. If a pedestrian is detected within the forward longitudinal trajectory ($\text{dist}_{\text{fwd}} \le 12.0\text{ m}$) and lateral corridor ($\text{dist}_{\text{lat}} \le W_{\text{lane}}/2 + 1.8\text{ m}$):

$$a_{\text{yield}} = \min\left(a_{\text{IDM}}, a_{\text{stop}}(d_{\text{ped}} - L/2 - 1.0\text{ m})\right)$$

If clear distance $d_{\text{clear}} \le 0.8\text{ m}$, the vehicle halts completely ($v = 0$, state `Waiting`). A vehicle never enters an occupied crosswalk corridor, guaranteeing zero vehicle-pedestrian collisions.

---

## 9. Environment, Visual Effects & Solar Calculations

### 9.1 Analytical Solar Arc & Celestial Mechanics

The sun's position and atmospheric illumination vary continuously as a function of the local simulation time of day $t \in [0.0, 24.0)$ hours.

1. **Solar Azimuth Angle ($\phi_{\text{noon}}$)**:
   $$\phi_{\text{noon}} = \frac{2\pi \cdot (t - 12.0)}{24.0}$$
   At solar noon ($t = 12.0\text{ h}$), $\phi = 0$; at midnight ($t = 0.0\text{ h}$), $\phi = -\pi$.

2. **Solar Elevation ($\theta_{\text{elev}}$)**:
   $$\theta_{\text{elev}} = \sin\left(\frac{\pi \cdot (t - 6.0)}{12.0}\right)$$
   - Solar Noon ($12:00$): $\theta_{\text{elev}} = +1.0$ (zenith).
   - Sunrise ($06:00$): $\theta_{\text{elev}} = 0.0$ (horizon).
   - Sunset ($18:00$): $\theta_{\text{elev}} = 0.0$ (horizon).
   - Midnight ($00:00$): $\theta_{\text{elev}} = -1.0$ (nadir).

3. **Sun Direction Vector ($\hat{\mathbf{l}}_{\text{sun}}$)**:
   $$\mathbf{l}_{\text{raw}} = \begin{bmatrix} \cos(\theta_{\text{elev}}) \sin(\phi_{\text{noon}}) \\ -\max(\theta_{\text{elev}}, 0.05) \\ \cos(\theta_{\text{elev}}) \cos(\phi_{\text{noon}}) \end{bmatrix}, \quad \hat{\mathbf{l}}_{\text{sun}} = \text{normalize}(\mathbf{l}_{\text{raw}})$$

4. **Smooth Civil Twilight Continuity**:
   To prevent color and intensity popping during dawn and dusk, a widened civil twilight transition interval $m_{\text{elevation}} \in [-0.15, +0.15]$ ($\sim 68$ minutes real-world duration) is parameterized:
   $$t_{\text{twilight}} = \text{clamp}\left(\frac{\theta_{\text{elev}} - (-0.15)}{0.30}, 0.0, 1.0\right)$$
   Using standard cubic Hermite interpolation $H(x) = 3x^2 - 2x^3$:
   - Sun intensity smoothly ramps from $0.0\text{ lx}$ at $\theta_{\text{elev}} \le -0.15$ to $1.25\text{ lx}$ at midday.
   - Sky colors smoothly blend between deep night navy `(0.02, 0.03, 0.07)`, warm twilight peach/orange `(0.85, 0.48, 0.28)`, and crisp daylight cerulean `(0.25, 0.52, 0.85)`.
   - Per-minute color and intensity deltas across all 1440 minutes of the solar day satisfy $\Delta_{\text{color}} \le 0.039 < 0.05$, guaranteeing zero visual pop.

### 9.2 Cascaded Shadow Mapping & Texel Stabilization

Directional shadows are computed using a single-pass 2048x2048 high-precision depth buffer ($\text{GL\_DEPTH\_COMPONENT24}$) with light-space orthographic bounds $[-95, 95]\text{ m}$ XZ and $[0.5, 300]\text{ m}$ Z.

1. **Light-Space Projection Matrix**:
   $$\mathbf{P}_{\text{light}} = \begin{bmatrix} \frac{1}{95.0} & 0 & 0 & 0 \\ 0 & \frac{1}{95.0} & 0 & 0 \\ 0 & 0 & -\frac{2}{300 - 0.5} & -\frac{300 + 0.5}{300 - 0.5} \\ 0 & 0 & 0 & 1 \end{bmatrix}$$

2. **Texel Snapping Formula (Sub-Texel Shimmer Elimination)**:
   When the virtual light camera tracks the scene center or moves, fractional texel offsets cause shimmering shadow edges. The light-space origin is stabilized by snapping the light-view translation vector to integer world-space texel boundaries:
   $$\Delta u = \frac{2 \cdot \text{orthoSize}}{\text{shadowMapResolution}} = \frac{190.0}{2048.0} \approx 0.09277\text{ m/texel}$$
   $$\mathbf{p}_{\text{snapped}, x} = \text{round}\left(\frac{\mathbf{p}_{\text{light}, x}}{\Delta u}\right) \cdot \Delta u, \quad \mathbf{p}_{\text{snapped}, y} = \text{round}\left(\frac{\mathbf{p}_{\text{light}, y}}{\Delta u}\right) \cdot \Delta u$$

3. **Normal-Offset Shadow Bias**:
   To completely eliminate shadow acne and surface self-shadowing without detached shadow bias ("Peter Panning"), vertices project into light space with an outward normal-offset bias:
   $$\mathbf{x}_{\text{biased}} = \mathbf{x}_{\text{world}} + \hat{\mathbf{n}} \cdot \delta_{\text{normal\_bias}}$$
   where $\delta_{\text{normal\_bias}} = 0.04\text{ m}$. In the fragment shader, a slope-scaled depth bias is applied:
   $$\text{bias} = \max(0.003 \cdot (1.0 - \hat{\mathbf{n}} \cdot \hat{\mathbf{l}}), 0.0005)$$

4. **Percentage Closer Filtering (PCF)**:
   Soft shadow penumbrae are generated via a 9-tap kernel sampling a $3\times 3$ texel grid:
   $$S(\mathbf{p}) = \frac{1}{9} \sum_{x=-1}^{1} \sum_{y=-1}^{1} \left[ D_{\text{map}}(\mathbf{p}_{xy} + (x, y)\cdot \text{texelSize}) > (\mathbf{p}_z - \text{bias}) ? 1.0 : 0.0 \right]$$

### 9.3 Atmospheric Fog & Rain Precipitation System

1. **Exponential-Squared Atmospheric Fog**:
   Fog density $\rho_{\text{fog}} \in [0.005, 0.030]\text{ m}^{-1}$ produces physically accurate Rayleigh-type extinction with distance $d = \|\mathbf{P}_{\text{cam}} - \mathbf{P}_{\text{frag}}\|$:
   $$f(d) = \exp\left(-(d \cdot \rho_{\text{fog}})^2\right)$$
   $$C_{\text{final}} = f(d) \cdot C_{\text{lit}} + (1.0 - f(d)) \cdot C_{\text{horizon}}$$
   Fog blends smoothly into the dynamic horizon color $C_{\text{horizon}}(t)$ across all times of day.

2. **Rain Particle Pool & Velocity Integration**:
   A GPU particle pool maintains $N = 10,000$ active rain streaks.
   - Bounding Volume: Camera-centric box $X, Z \in [-35, 35]\text{ m}$, $Y \in [0, 25]\text{ m}$.
   - Fall Velocity: $v_{\text{fall}} \sim \mathcal{U}(18, 26)\text{ m/s}$ downward with horizontal wind tilt vector $\mathbf{w} = (-2.0, 0, -1.0)\text{ m/s}$.
   - Wrap-Around Boundary: Streaks below $Y < 0$ or outside radius $R > 35\text{ m}$ wrap around to the top ($Y \approx 25\text{ m}$) with randomized horizontal offsets.
   - Rendering: Dynamically uploaded VBO drawn as `GL_LINES` with vertex color alpha fading (head $\alpha = 0.7$, tail $\alpha = 0.0$) and depth-mask disabled.

3. **Wet-Surface Road Sheen**:
   When rain is active with wetness factor $w \in [0.0, 1.0]$:
   - Road surface specular intensity is scaled: $K_{\text{spec}} \leftarrow K_{\text{spec}} \cdot (1.0 + 0.85 \cdot w)$
   - Shininess exponent is sharpened: $\alpha \leftarrow \alpha \cdot (1.0 + 1.20 \cdot w)$
   - Diffuse albedo is slightly darkened: $C_{\text{diffuse}} \leftarrow C_{\text{diffuse}} \cdot (1.0 - 0.25 \cdot w)$

### 9.4 Streetlights & Vehicle Headlights Selection

1. **Nearest N-Light Distance Sorting**:
   To satisfy GLSL uniform array limits (max 16 point lights) without unbounded shader overhead:
   $$d_i = \|\mathbf{P}_{\text{cam}} - \mathbf{P}_{\text{light}, i}\|$$
   Registered Phase 3 streetlight positions are sorted by Euclidean distance $d_i$ each frame. The closest 16 lights are bound to the shader.
   Intensity is modulated by the solar elevation: $I_{\text{streetlight}} = \text{smoothstep}(0.05, -0.10, \theta_{\text{elev}})$.

2. **Vehicle Headlight Spotlights**:
   At night ($I_{\text{streetlight}} > 0$), the 4 closest active vehicles to the camera are selected. Each vehicle contributes dual forward spotlights:
   $$\mathbf{P}_{\text{left}} = \mathbf{P}_{\text{veh}} + \mathbf{R} \cdot (-0.7, 0.65, 2.1)^T, \quad \mathbf{P}_{\text{right}} = \mathbf{P}_{\text{veh}} + \mathbf{R} \cdot (0.7, 0.65, 2.1)^T$$
   Spotlights are directed forward with a subtle downward pitch ($1.5^\circ$) for realistic road surface coverage ($\phi_{\text{inner}} = 14^\circ$, $\phi_{\text{outer}} = 22^\circ$, range $35\text{ m}$).

### 9.5 Double-Buffered Asynchronous GPU Profiler

To profile OpenGL rendering passes without inducing CPU-GPU synchronization stalls:
- Ping-Pong Query Objects: Two query IDs (`queryID[0]`, `queryID[1]`) are maintained per profiled section ("Shadow Pass", "Main Scene Pass", "Skybox & FX Pass", "UI Pass").
- Asynchronous Retrieval: In frame $N$, query $N$ records execution duration via `glBeginQuery(GL_TIME_ELAPSED)` / `glEndQuery(GL_TIME_ELAPSED)`, while results from frame $N-1$ are queried via `glGetQueryObjectui64v`.
- Pipeline Independence: Eliminates `glFinish()` stalls, providing microsecond-accurate GPU execution timings on the Dear ImGui performance overlay.

---

## 10. Computer Graphics Algorithms & Mathematical Demonstrators (Phase 7)

This section details the fundamental 2D and 3D computer graphics algorithms implemented in `SmartCity::algos` and `SmartCity::analytics`.

---

### 10.1 Line Rasterization: DDA vs. Bresenham's Integer Algorithm

Given two integer endpoints $P_0(x_0, y_0)$ and $P_1(x_1, y_1)$ in screen/raster space:
$$\Delta x = x_1 - x_0, \quad \Delta y = y_1 - y_0$$

#### 1. Digital Differential Analyzer (DDA)
The DDA algorithm solves the differential equation of a straight line by taking unit steps along the driving axis and calculating proportional increments along the passive axis:
$$N = \max(|\Delta x|, |\Delta y|)$$
$$\Delta x_{\text{step}} = \frac{\Delta x}{N}, \quad \Delta y_{\text{step}} = \frac{\Delta y}{N}$$

For each step $k \in [0, N]$:
$$x_{k+1} = x_k + \Delta x_{\text{step}}, \quad y_{k+1} = y_k + \Delta y_{\text{step}}$$
The rasterized pixel is determined by rounding:
$$\text{Pixel}_k = (\lfloor x_k + 0.5 \rfloor, \lfloor y_k + 0.5 \rfloor)$$

*Computational Characteristics*: Requires floating-point division at setup and floating-point additions plus float-to-integer conversions per pixel. Prone to accumulative round-off errors over long line segments.

#### 2. Bresenham's Integer Line Drawing Algorithm (All 8 Octants)
Bresenham's algorithm utilizes only integer additions, subtractions, and bitwise shifts ($2 \cdot x = x \ll 1$), eliminating all floating-point math.

##### Mathematical Derivation for Octant 0 ($0 \le \Delta y \le \Delta x$, $m \in [0, 1]$):
At pixel $(x_k, y_k)$, the true continuous y-coordinate at $x_k + 1$ is:
$$y = m(x_k + 1) + c$$
The algorithm must choose between pixel $E(x_k + 1, y_k)$ and pixel $NE(x_k + 1, y_k + 1)$.
The vertical distances from the true line to the candidate pixel centers are:
$$d_1 = y - y_k = m(x_k + 1) + c - y_k$$
$$d_2 = (y_k + 1) - y = y_k + 1 - m(x_k + 1) - c$$
The distance difference determines which candidate is closer:
$$d_1 - d_2 = 2m(x_k + 1) - 2y_k + 2c - 1$$
Multiplying by $\Delta x > 0$ defines the integer decision variable $D_k$:
$$D_k = \Delta x (d_1 - d_2) = 2\Delta y \cdot x_k - 2\Delta x \cdot y_k + C$$
where $C = 2\Delta y + \Delta x(2c - 1)$.

At the starting pixel $(x_0, y_0)$, since $y_0 = m x_0 + c \implies c = y_0 - \frac{\Delta y}{\Delta x} x_0$:
$$D_0 = 2\Delta y - \Delta x$$

The step-to-step recurrence relation is:
$$D_{k+1} - D_k = 2\Delta y (x_{k+1} - x_k) - 2\Delta x (y_{k+1} - y_k)$$
Since $x_{k+1} = x_k + 1$:
- If $D_k < 0$: choose pixel $E(x_k + 1, y_k) \implies y_{k+1} = y_k$:
  $$D_{k+1} = D_k + 2\Delta y$$
- If $D_k \ge 0$: choose pixel $NE(x_k + 1, y_k + 1) \implies y_{k+1} = y_k + 1$:
  $$D_{k+1} = D_k + 2(\Delta y - \Delta x)$$

##### Generalization to All 8 Octants:
The 8 octants of the 2D Cartesian plane are classified by the directional signs $s_x = \text{sgn}(\Delta x)$, $s_y = \text{sgn}(\Delta y)$ and the dominant axis condition:

| Octant | Slope $m$ | $s_x$ | $s_y$ | Dominant Axis | Decision Variable $D_0$ | Step Choice ($D_k < 0$) | Step Choice ($D_k \ge 0$) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **0** | $[0, 1]$ | $+1$ | $+1$ | $X$ ($dx \ge dy$) | $2dy - dx$ | $x \mathrel{+}= 1, D \mathrel{+}= 2dy$ | $x \mathrel{+}= 1, y \mathrel{+}= 1, D \mathrel{+}= 2(dy - dx)$ |
| **1** | $[1, \infty)$ | $+1$ | $+1$ | $Y$ ($dy > dx$) | $2dx - dy$ | $y \mathrel{+}= 1, D \mathrel{+}= 2dx$ | $x \mathrel{+}= 1, y \mathrel{+}= 1, D \mathrel{+}= 2(dx - dy)$ |
| **2** | $(-\infty, -1]$ | $-1$ | $+1$ | $Y$ ($dy > dx$) | $2dx - dy$ | $y \mathrel{+}= 1, D \mathrel{+}= 2dx$ | $x \mathrel{-}= 1, y \mathrel{+}= 1, D \mathrel{+}= 2(dx - dy)$ |
| **3** | $[-1, 0]$ | $-1$ | $+1$ | $X$ ($dx \ge dy$) | $2dy - dx$ | $x \mathrel{-}= 1, D \mathrel{+}= 2dy$ | $x \mathrel{-}= 1, y \mathrel{+}= 1, D \mathrel{+}= 2(dy - dx)$ |
| **4** | $[0, 1]$ | $-1$ | $-1$ | $X$ ($dx \ge dy$) | $2dy - dx$ | $x \mathrel{-}= 1, D \mathrel{+}= 2dy$ | $x \mathrel{-}= 1, y \mathrel{-}= 1, D \mathrel{+}= 2(dy - dx)$ |
| **5** | $[1, \infty)$ | $-1$ | $-1$ | $Y$ ($dy > dx$) | $2dx - dy$ | $y \mathrel{-}= 1, D \mathrel{+}= 2dx$ | $x \mathrel{-}= 1, y \mathrel{-}= 1, D \mathrel{+}= 2(dx - dy)$ |
| **6** | $(-\infty, -1]$ | $+1$ | $-1$ | $Y$ ($dy > dx$) | $2dx - dy$ | $y \mathrel{-}= 1, D \mathrel{+}= 2dx$ | $x \mathrel{+}= 1, y \mathrel{-}= 1, D \mathrel{+}= 2(dx - dy)$ |
| **7** | $[-1, 0]$ | $+1$ | $-1$ | $X$ ($dx \ge dy$) | $2dy - dx$ | $x \mathrel{+}= 1, D \mathrel{+}= 2dy$ | $x \mathrel{+}= 1, y \mathrel{-}= 1, D \mathrel{+}= 2(dy - dx)$ |

---

### 10.2 Parametric Bézier Curves: De Casteljau, Derivatives & Curvature

Given four 3D control points $\mathbf{P}_0, \mathbf{P}_1, \mathbf{P}_2, \mathbf{P}_3 \in \mathbb{R}^3$ defining a turning path in the intersection:

#### 1. Explicit Bernstein Polynomial Formulation
$$\mathbf{B}(t) = \sum_{i=0}^3 B_{i, 3}(t) \mathbf{P}_i = (1-t)^3 \mathbf{P}_0 + 3(1-t)^2 t \mathbf{P}_1 + 3(1-t) t^2 \mathbf{P}_2 + t^3 \mathbf{P}_3, \quad t \in [0, 1]$$

#### 2. De Casteljau Recursive Algorithm
Computes $\mathbf{B}(t)$ through geometrically stable successive affine combinations:
$$\mathbf{P}_i^{(0)} = \mathbf{P}_i \quad (i = 0, 1, 2, 3)$$
$$\mathbf{P}_i^{(k)}(t) = (1-t)\mathbf{P}_i^{(k-1)}(t) + t \mathbf{P}_{i+1}^{(k-1)}(t) \quad (k = 1, 2, 3; \, i = 0, \dots, 3-k)$$
$$\mathbf{B}(t) = \mathbf{P}_0^{(3)}(t)$$

*Verification*: In `SmartCity::algos::BezierCurveEvaluator::VerifyFormulationsAgreement`, points evaluated via De Casteljau and Bernstein polynomials agree within machine epsilon:
$$\max_{t \in [0, 1]} \|\mathbf{B}_{\text{DeCasteljau}}(t) - \mathbf{B}_{\text{Bernstein}}(t)\| < 1.0 \times 10^{-6}\text{ m}$$

#### 3. Analytical First Derivative (Velocity / Tangent Vector)
$$\mathbf{B}'(t) = \frac{d\mathbf{B}(t)}{dt} = 3(1-t)^2 (\mathbf{P}_1 - \mathbf{P}_0) + 6(1-t)t (\mathbf{P}_2 - \mathbf{P}_1) + 3t^2 (\mathbf{P}_3 - \mathbf{P}_2)$$
The instantaneous unit tangent $\hat{\mathbf{T}}(t)$ and road heading $\theta(t)$ are:
$$\hat{\mathbf{T}}(t) = \frac{\mathbf{B}'(t)}{\|\mathbf{B}'(t)\|}, \quad \theta(t) = \text{atan2}(B'_x(t), B'_z(t))$$

#### 4. Analytical Second Derivative (Acceleration Vector)
$$\mathbf{B}''(t) = \frac{d^2\mathbf{B}(t)}{dt^2} = 6(1-t)(\mathbf{P}_2 - 2\mathbf{P}_1 + \mathbf{P}_0) + 6t(\mathbf{P}_3 - 2\mathbf{P}_2 + \mathbf{P}_1)$$

#### 5. Differential Curvature & Turn Radius
The signed 3D curvature $\kappa(t)$ and minimum radius of curvature $R(t)$ along the trajectory are:
$$\kappa(t) = \frac{\|\mathbf{B}'(t) \times \mathbf{B}''(t)\|}{\|\mathbf{B}'(t)\|^3}, \quad R(t) = \frac{1}{\kappa(t)}$$
Highway engineering standards require $R(t) \ge 8.0\text{ m}$ for passenger vehicles at urban intersections to prevent lateral skidding.

#### 6. Arc Length Parameterization
Because $\|\mathbf{B}'(t)\|$ is non-constant, advancing $t$ linearly does not yield constant vehicle speed. The arc-length function is:
$$s(t) = \int_0^t \|\mathbf{B}'(u)\| du$$
A precomputed Lookup Table (LUT) of $N = 200$ samples discretizes $s_i = s(t_i)$. For a requested distance along path $s$, the exact parameter $t$ is retrieved via binary search and linear interpolation:
$$t(s) = t_i + \frac{s - s_i}{s_{i+1} - s_i} (t_{i+1} - t_i)$$
guaranteeing exact kinematic velocity compliance $v = \frac{ds}{dt_{\text{sim}}}$.

---

### 10.3 Ray Casting & 3D Spatial Intersection Tests

#### 1. Screen-to-World Ray Unprojection
Given mouse viewport coordinates $(x_s, y_s)$, viewport dimensions $(w, h)$, and the composite camera matrix $\mathbf{M} = \mathbf{P} \cdot \mathbf{V}$:
Normalized device coordinates (NDC) with $Y$ inverted (OpenGL origin at bottom-left):
$$x_{\text{ndc}} = \frac{2 x_s}{w} - 1.0, \quad y_{\text{ndc}} = 1.0 - \frac{2 y_s}{h}$$

Inverse View-Projection Matrix:
$$\mathbf{M}_{\text{inv}} = (\mathbf{P} \cdot \mathbf{V})^{-1}$$

Transform near and far clip plane points:
$$\mathbf{p}'_{\text{near}} = \mathbf{M}_{\text{inv}} \cdot \begin{bmatrix} x_{\text{ndc}} \\ y_{\text{ndc}} \\ -1.0 \\ 1.0 \end{bmatrix}, \quad \mathbf{p}'_{\text{far}} = \mathbf{M}_{\text{inv}} \cdot \begin{bmatrix} x_{\text{ndc}} \\ y_{\text{ndc}} \\ 1.0 \\ 1.0 \end{bmatrix}$$
Homogeneous perspective divide:
$$\mathbf{p}_{\text{near}} = \frac{\mathbf{p}'_{\text{near}, xyz}}{\mathbf{p}'_{\text{near}, w}}, \quad \mathbf{p}_{\text{far}} = \frac{\mathbf{p}'_{\text{far}, xyz}}{\mathbf{p}'_{\text{far}, w}}$$

- **Perspective Projection**:
  $$\mathbf{o} = \mathbf{P}_{\text{cam}}, \quad \mathbf{d} = \text{normalize}(\mathbf{p}_{\text{far}} - \mathbf{p}_{\text{near}})$$
- **Orthographic Projection**:
  $$\mathbf{o} = \mathbf{p}_{\text{near}}, \quad \mathbf{d} = -\mathbf{F}_{\text{cam}} \quad (\text{constant forward direction})$$

#### 2. Ray-AABB Intersection: Kay-Kajiya Slab Method
For an Axis-Aligned Bounding Box defined by $[\mathbf{b}_{\min}, \mathbf{b}_{\max}]$ and a 3D ray $\mathbf{r}(t) = \mathbf{o} + t \mathbf{d}$:
For each dimension $i \in \{x, y, z\}$:
$$t_{1, i} = \frac{b_{\min, i} - o_i}{d_i}, \quad t_{2, i} = \frac{b_{\max, i} - o_i}{d_i}$$
$$t_{\min, i} = \min(t_{1, i}, t_{2, i}), \quad t_{\max, i} = \max(t_{1, i}, t_{2, i})$$

The global intersection interval is the intersection of all 3 slabs:
$$t_{\text{enter}} = \max(t_{\min, x}, t_{\min, y}, t_{\min, z})$$
$$t_{\text{exit}} = \min(t_{\max, x}, t_{\max, y}, t_{\max, z})$$

*Intersection Condition*: The ray hits the AABB if and only if:
$$t_{\text{enter}} \le t_{\text{exit}} \quad \text{and} \quad t_{\text{exit}} \ge 0$$
The hit distance is $t = t_{\text{enter}} > 0 ? t_{\text{enter}} : t_{\text{exit}}$.

#### 3. Ray-OBB Intersection via Local Space Transformation
For an Oriented Bounding Box with center $\mathbf{C}$, orthonormal orientation axes $\mathbf{U} = [\hat{\mathbf{u}}_0 \mid \hat{\mathbf{u}}_1 \mid \hat{\mathbf{u}}_2]$, and half-extents $\mathbf{e}$:
The ray origin $\mathbf{o}$ and direction $\mathbf{d}$ are transformed into the local coordinate frame of the OBB:
$$\mathbf{o}_{\text{local}} = \mathbf{U}^T (\mathbf{o} - \mathbf{C}), \quad \mathbf{d}_{\text{local}} = \mathbf{U}^T \mathbf{d}$$
The test reduces directly to the Ray-AABB Kay-Kajiya slab test against $[-\mathbf{e}, +\mathbf{e}]$ in local space. If a hit occurs at distance $t$, the surface normal $\hat{\mathbf{n}}_{\text{local}}$ is rotated back to world space:
$$\hat{\mathbf{n}}_{\text{world}} = \mathbf{U} \cdot \hat{\mathbf{n}}_{\text{local}}$$

---

### 10.4 Traffic Dynamics: Akçelik Fuel & Emissions Model

Vehicle fuel consumption and greenhouse gas emissions are calculated using the physically derived Akçelik Tractive Power Model (ARRB / Highway Capacity Manual):

#### 1. Instantaneous Wheel Power Demand
$$P_{\text{wheels}} = \max\left(0, \left(m \cdot a + R_{\text{aero}} + R_{\text{roll}}\right) \cdot v\right) \quad (\text{kW})$$
- Aerodynamic drag force: $R_{\text{aero}} = \frac{1}{2} \rho_{\text{air}} C_d A v^2$ (with $\rho_{\text{air}} = 1.205\text{ kg/m}^3$)
- Rolling resistance force: $R_{\text{roll}} = m \cdot g \cdot C_{rr}$ (with $C_{rr} = 0.015$, $g = 9.81\text{ m/s}^2$)

#### 2. Fuel Consumption Rate
$$f_t = \begin{cases}
\alpha, & \text{if } P_{\text{wheels}} = 0 \text{ (idling, deceleration, or braking)} \\
\alpha + \beta_1 P_{\text{wheels}} + \beta_2 m a^2 v, & \text{if } P_{\text{wheels}} > 0 \text{ (positive tractive acceleration)}
\end{cases}$$
where:
- $\alpha \in [0.25, 0.90]\text{ mL/s}$: base engine idling fuel consumption rate
- $\beta_1 \in [0.045, 0.080]\text{ mL/(kJ)}$: engine efficiency coefficient per unit work done
- $\beta_2 \in [0.001, 0.003]\text{ mL/(kN}\cdot\text{m/s}^2)$: inertia and transient transmission loss factor

#### 3. Carbon Dioxide ($\text{CO}_2$) Emissions
Based on stoichiometric oxidation of hydrocarbon fuel:
$$e_{\text{CO}_2}(t) = f_t \cdot \gamma_{\text{emission}} \quad (\text{g/s})$$
where $\gamma_{\text{emission}} = 2.392\text{ g }\text{CO}_2/\text{mL}$ for gasoline and $2.640\text{ g }\text{CO}_2/\text{mL}$ for diesel.


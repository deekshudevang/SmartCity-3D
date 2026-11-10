# SmartCity 3D — Intersection Geometric Specification

## 1. Overview & Traffic Rule

The SmartCity 3D intersection environment simulates a standard urban four-way intersection centered at the Cartesian world origin $(0, 0, 0)$.

- **Coordinate System**:
  - Right-handed Cartesian space: $+Y$ is Up, Ground plane is $XZ$.
  - North = $-Z$, South = $+Z$, East = $+X$, West = $-X$.
  - Internal units: Metres ($\text{m}$), seconds ($\text{s}$), radians ($\text{rad}$).
- **Traffic Convention**:
  - **Right-Hand Traffic (RHT)** (configurable via `rightHandTraffic = true`).
  - Vehicles drive on the right half of the roadway relative to their forward direction.
  - Left turns cross oncoming traffic. Right turns stay in the outermost lane without crossing opposing flows.

---

## 2. Geometric Dimensions & Configuration Defaults

All dimensions are config-driven and match the following master specification:

| Parameter | Value | Description |
| :--- | :--- | :--- |
| **Approach Length** | $120.0\text{ m}$ | Length of each approach arm measured from intersection center |
| **Lane Width** | $3.5\text{ m}$ | Standard vehicular travel lane width |
| **Median Width** | $1.5\text{ m}$ | Raised central dividing island separating opposing traffic |
| **Through Lanes per Direction** | $2$ | Two through travel lanes inbound and two outbound |
| **Turn Pockets** | $30.0\text{ m}$ length | Dedicated Left-Turn pocket ($3.5\text{ m}$) and Right-Turn pocket ($3.5\text{ m}$) on each approach |
| **Inbound Roadway Width** | $14.0\text{ m}$ | 4 lanes total at intersection mouth: Left + Through 1 + Through 2 + Right |
| **Outbound Roadway Width** | $7.0\text{ m}$ | 2 through departure lanes |
| **Total Road Width at Mouth** | $22.5\text{ m}$ | $14.0\text{ m (inbound)} + 1.5\text{ m (median)} + 7.0\text{ m (outbound)}$ |
| **Curb Height** | $0.15\text{ m}$ | Vertical elevation step from road surface ($Y=0.0$) to sidewalk ($Y=0.15$) |
| **Sidewalk Width** | $3.0\text{ m}$ | Concrete pedestrian walkway flanking both sides of every approach |
| **Corner Curb Radius** | $8.0\text{ m}$ | Rounded curb returns at the 4 intersection corner quadrants |
| **Crosswalk Width** | $4.0\text{ m}$ | Zebra crossing striping width across each approach |
| **Zebra Bar Dimensions** | $0.5\text{ m} \times 4.0\text{ m}$ | $0.5\text{ m}$ white bar, $0.5\text{ m}$ gap between bars |
| **Crosswalk Distance** | $14.0\text{ m}$ to $18.0\text{ m}$ | Longitudinal distance from origin to crosswalk outer boundary |
| **Stop Line Distance** | $19.5\text{ m}$ from origin | Solid stop bar placed exactly $1.5\text{ m}$ upstream from crosswalk edge |
| **Stop Line Width** | $0.5\text{ m}$ | Solid reflective white transverse bar |

---

## 3. Lane Numbering Scheme

Every lane in the network is uniquely identified by the tuple:
$$\text{LaneKey} = (\text{ApproachId}, \text{Direction}, \text{LaneIndex})$$

- **ApproachId**: `N` (North), `E` (East), `S` (South), `W` (West).
- **Direction**: `IN` (Inbound toward center) or `OUT` (Outbound away from center).
- **LaneIndex**: Numbered sequentially from the median outward to the curb ($0$-indexed).

### Inbound Approaches (Mouth Section, Length $30\text{ m}$ to Stop Line)
1. **Lane 0 (Left Pocket)**:
   - Width: $3.5\text{ m}$, Adjacent to Median.
   - Length: $30.0\text{ m}$ storage pocket.
   - Permitted Movements: **Left-Turn only**.
2. **Lane 1 (Inner Through)**:
   - Width: $3.5\text{ m}$.
   - Full approach length ($120.0\text{ m}$).
   - Permitted Movements: **Through only**.
3. **Lane 2 (Outer Through)**:
   - Width: $3.5\text{ m}$.
   - Full approach length ($120.0\text{ m}$).
   - Permitted Movements: **Through only**.
4. **Lane 3 (Right Pocket)**:
   - Width: $3.5\text{ m}$, Adjacent to Curb.
   - Length: $30.0\text{ m}$ storage pocket.
   - Permitted Movements: **Right-Turn only**.

### Outbound Approaches (Departure Lanes)
1. **Lane 0 (Inner Departure)**:
   - Width: $3.5\text{ m}$, Adjacent to Median.
   - Receives: Left-turning vehicles and inner through vehicles.
2. **Lane 1 (Outer Departure)**:
   - Width: $3.5\text{ m}$, Adjacent to Curb.
   - Receives: Right-turning vehicles and outer through vehicles.

---

## 4. Turning Paths & Bézier Formulations

All vehicular turning maneuvers across the intersection are represented by $G^1$-continuous cubic Bézier curves:
$$\mathbf{B}(t) = (1-t)^3 \mathbf{P}_0 + 3(1-t)^2 t \mathbf{P}_1 + 3(1-t) t^2 \mathbf{P}_2 + t^3 \mathbf{P}_3, \quad t \in [0, 1]$$

### Endpoint Matching Tolerance
Every path's start point $\mathbf{P}_0$ matches its inbound lane center endpoint within $< 1.0\text{ cm}$ ($\|\mathbf{P}_0 - \mathbf{L}_{\text{in, end}}\| < 0.01\text{ m}$).  
Every path's end point $\mathbf{P}_3$ matches its outbound lane center startpoint within $< 1.0\text{ cm}$ ($\|\mathbf{P}_3 - \mathbf{L}_{\text{out, start}}\| < 0.01\text{ m}$).

### Tangency & Curvature Constraints
- $\mathbf{P}_1 = \mathbf{P}_0 + d_1 \cdot \hat{\mathbf{v}}_{\text{in}}$ (tangent to inbound lane heading).
- $\mathbf{P}_2 = \mathbf{P}_3 - d_2 \cdot \hat{\mathbf{v}}_{\text{out}}$ (tangent to outbound lane heading).
- Minimum Turning Radius:
  - Right Turns: $R_{\min} \ge 8.0\text{ m}$.
  - Left Turns: $R_{\min} \ge 12.0\text{ m}$.

---

## 5. Conflict Zones & Topology Matrix

In Right-Hand Traffic:
1. **Right Turns** (`Lane 3` $\to$ Right approach `Lane 1`):
   - Curving along curb radius.
   - **No conflict** with opposing left-turn or through streams.
2. **Through Movements** (`Lane 1` / `Lane 2` $\to$ Opposite approach):
   - Conflicts with cross-street through traffic (managed by signal phases).
   - Conflicts with opposing left-turning traffic.
3. **Left Turns** (`Lane 0` $\to$ Left approach `Lane 0`):
   - Crosses opposing through paths.
   - **ConflictZone** established at intersection of Bézier curve and opposing straight corridors.
   - Conflicting relationship is symmetric: $\text{Conflicts}(A, B) \iff \text{Conflicts}(B, A)$.

---

## 6. CityMap and RoadNetwork Architecture

The `RoadNetwork` class in `/src/city` serves as the single immutable source of truth:
- Both 3D visual render meshes (road surface, markings, curbs, stop lines, crosswalks) and microscopic traffic simulation graphs are generated **directly from this data structure**.
- Prevents geometric drift between visual rendering and physical collision/path logic.
- Full round-trip JSON serialization via `assets/config/network.json`.


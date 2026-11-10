# SmartCity 3D — Real-Time Analytics & Metrics Specification

This document provides the formal mathematical specification, data sources, update rates, and validity conditions for all metrics computed by the `AnalyticsEngine` (`/src/analytics`). Every metric displayed in the Dear ImGui dashboard or logged during experiments is computed strictly from real simulation state.

---

## 1. Architectural Principles

1. **Fixed-Step Synchronization**: Metrics are sampled at the simulation fixed-timestep boundary ($\Delta t = 1/60\text{ s}$), completely decoupled from the rendering frame rate.
2. **Pause Freezing**: When the simulation is paused, all timers, ring buffers, and accumulators freeze. No elapsed time or metric drift occurs during pause.
3. **Lossless Reset**: Calling `Reset()` restores all counters, ring buffers, cumulative totals, and statistical windows to their initial pristine state.
4. **Strict Validity Handling**: Undefined conditions (e.g., division by zero when no vehicles are active or completed) are marked as `Invalid` and displayed as `"N/A"`, never misleadingly displayed as `0.0` or `0/0`.
5. **Ring Buffer History**: Temporal history for line plots (speed, queues, throughput) is stored in fixed-capacity ring buffers ($N = 300$ samples, representing up to 5 minutes of 1 Hz aggregated state).

---

## 2. Comprehensive Metric Definitions

### 2.1 Active Vehicles ($N_{\text{active}}$)
- **Data Source**: `sim::VehicleManager::GetActiveVehicles()` (`v->IsActive() == true`).
- **Description**: The instantaneous count of all simulated vehicles currently traversing the road network.
- **Formula**:
  $$N_{\text{active}} = \sum_{i=1}^{M} [v_i.\text{isActive}()]$$
- **Unit**: Vehicles (integer count).
- **Measurement Interval**: Instantaneous, sampled at each fixed step.
- **Reset Behaviour**: Set to 0.
- **Pause Behaviour**: Freezes at current value.
- **Validity Condition**: Always valid ($\ge 0$).
- **Update Rate**: 60 Hz (every sim step).

---

### 2.2 Waiting Vehicles ($N_{\text{waiting}}$)
- **Data Source**: `sim::Vehicle::GetSpeed()`, `sim::Vehicle::GetDistanceAlongRoute()`, `sim::Vehicle::GetRoute()`.
- **Description**: Vehicles situated in an approach deceleration/storage zone ($s_{\text{stop}} - 80\text{ m} \le s \le s_{\text{stop}} + 2\text{ m}$) whose instantaneous speed is below the stopped threshold ($v < v_{\text{stopped}} = 0.10\text{ m/s}$).
- **Formula**:
  $$N_{\text{waiting}} = \sum_{v \in \mathcal{V}_{\text{active}}} \left[ (v.\text{speed} < 0.10\text{ m/s}) \land (s_{\text{stop}} - 80\text{ m} \le v.s \le s_{\text{stop}} + 2\text{ m}) \right]$$
- **Unit**: Vehicles (integer count).
- **Measurement Interval**: Instantaneous, sampled at each fixed step.
- **Reset Behaviour**: Set to 0.
- **Pause Behaviour**: Freezes at current value.
- **Validity Condition**: Always valid ($\ge 0$).
- **Update Rate**: 60 Hz.

---

### 2.3 Queue Length per Approach ($Q_{\text{veh}}[a]$, $Q_{\text{metres}}[a]$)
- **Data Source**: Inbound approach lanes for each approach $a \in \{\text{North}, \text{East}, \text{South}, \text{West}\}$.
- **Queue Membership Rule**:
  A vehicle belongs to the contiguous queue of approach $a$ if:
  1. It is on an inbound lane of approach $a$ prior to the stop line ($s \le s_{\text{stop}} + 1.5\text{ m}$).
  2. Its speed is below the queue crawl threshold ($v \le 0.50\text{ m/s}$).
  3. It is either the lead vehicle closest to the stop line ($d_{\text{stop}} \le 15.0\text{ m}$) OR it is directly behind another queued vehicle with bumper-to-bumper spacing $\Delta d_{\text{gap}} \le 8.5\text{ m}$ (where standard bumper gap $s_0 = 2.0\text{ m} + L_{\text{veh}} \approx 6.5 - 7.5\text{ m}$).
- **Formulas**:
  $$Q_{\text{veh}}[a] = |\mathcal{Q}_a|$$
  $$Q_{\text{metres}}[a] = \begin{cases} 
  0.0, & \text{if } |\mathcal{Q}_a| = 0 \\
  s_{\text{stop}}[a] - \min_{v \in \mathcal{Q}_a}(v.s) + \frac{L_{\text{rear}}}{2}, & \text{if } |\mathcal{Q}_a| > 0 
  \end{cases}$$
- **Units**: 
  - $Q_{\text{veh}}$: Vehicles (integer).
  - $Q_{\text{metres}}$: Metres ($\text{m}$, float with 1 decimal precision).
- **Measurement Interval**: Instantaneous per fixed step, rolling peak tracked per phase.
- **Reset Behaviour**: Reset to 0 vehicles and 0.0 metres.
- **Pause Behaviour**: Freezes.
- **Validity Condition**: Always valid ($\ge 0$).
- **Update Rate**: 60 Hz.

---

### 2.4 Average Vehicle Speed ($\bar{v}_{\text{inst}}$, $\bar{v}_{\text{rolling}}$)
- **Data Source**: `v->GetSpeed()` for all $v \in \mathcal{V}_{\text{active}}$.
- **Description**: The spatial mean speed of all active vehicles on the network.
- **Formulas**:
  - **Instantaneous Spatial Mean**:
    $$\bar{v}_{\text{inst}} = \frac{1}{N_{\text{active}}} \sum_{i=1}^{N_{\text{active}}} v_i.\text{speed}$$
  - **Time-Weighted Rolling Window Mean** ($W = 60\text{ s}$):
    $$\bar{v}_{\text{rolling}} = \frac{\sum_{k \in W} \sum_{i} v_{i, k} \cdot \Delta t}{\sum_{k \in W} N_{\text{active}, k} \cdot \Delta t}$$
- **Units**: $\text{m/s}$ internally; displayed as $\text{km/h}$ ($v_{\text{km/h}} = v_{\text{m/s}} \cdot 3.6$) and $\text{m/s}$.
- **Measurement Interval**: 60-second rolling sliding window.
- **Reset Behaviour**: Window buffer cleared, value reset to `Invalid` (`"N/A"`).
- **Pause Behaviour**: Freezes.
- **Validity Condition**: Valid if and only if $N_{\text{active}} > 0$. When $N_{\text{active}} = 0$, returns `N/A`.
- **Update Rate**: Sampled at 60 Hz; rolling window aggregated at 1 Hz.

---

### 2.5 Average Waiting Time ($\bar{W}_{\text{live}}$, $\bar{W}_{\text{trip}}$)
- **Data Source**: Per-vehicle accumulator `v->m_waitingTime` incremented whenever $v.\text{speed} < 0.10\text{ m/s}$ in an approach zone.
- **Description**:
  1. **Live Waiting Time ($\bar{W}_{\text{live}}$)**: Average accumulated waiting time of currently active vehicles that have entered an approach queue.
  2. **Completed Trip Waiting Time ($\bar{W}_{\text{trip}}$)**: Average total waiting time experienced by all vehicles that have successfully completed their trips and exited the network.
- **Formulas**:
  $$\bar{W}_{\text{live}} = \frac{1}{|\mathcal{V}_{\text{waiting}}|} \sum_{v \in \mathcal{V}_{\text{waiting}}} v.t_{\text{wait}}$$
  $$\bar{W}_{\text{trip}} = \frac{1}{N_{\text{completed}}} \sum_{j=1}^{N_{\text{completed}}} W_{\text{total}, j}$$
- **Unit**: Seconds ($\text{s}$).
- **Measurement Interval**: Cumulative and 5-minute rolling window.
- **Reset Behaviour**: Cumulative sums and counts cleared; returns `N/A`.
- **Pause Behaviour**: Freezes.
- **Validity Condition**:
  - $\bar{W}_{\text{live}}$: Valid if $|\mathcal{V}_{\text{waiting}}| > 0$, else `"N/A"`.
  - $\bar{W}_{\text{trip}}$: Valid if $N_{\text{completed}} > 0$, else `"N/A"`.
- **Update Rate**: 60 Hz.

---

### 2.6 Intersection Throughput ($TH_{\text{rolling}}$, $TH_{\text{cumul}}$)
- **Data Source**: Trip exit events (`v->HasReachedEnd()` and stop-line cross events).
- **Description**: The rate of vehicles completing passage through the intersection, expressed on an hourly basis.
- **Formulas**:
  - **Rolling Window Throughput** ($W = 60\text{ s}$ or $300\text{ s}$):
    $$TH_{\text{rolling}} = \frac{N_{\text{exits}}(t - W, t)}{W} \times 3600\text{ veh/h}$$
  - **Cumulative Session Throughput**:
    $$TH_{\text{cumul}} = \begin{cases} 
    \text{N/A}, & \text{if } t_{\text{sim}} < 5.0\text{ s} \\
    \frac{N_{\text{totalCompleted}}}{t_{\text{sim}}} \times 3600\text{ veh/h}, & \text{if } t_{\text{sim}} \ge 5.0\text{ s}
    \end{cases}$$
- **Unit**: Vehicles per hour ($\text{veh/h}$).
- **Measurement Interval**: Rolling $W \in \{60\text{ s}, 300\text{ s}\}$ and cumulative.
- **Reset Behaviour**: Exit event timestamps cleared, $N_{\text{totalCompleted}} = 0$, returns `N/A`.
- **Pause Behaviour**: Freezes.
- **Validity Condition**: Valid if $t_{\text{sim}} \ge 5.0\text{ s}$. During the first 5 seconds of a run, displays `"N/A"` to prevent asymptotic initial spikes.
- **Update Rate**: Event-driven update on vehicle exit; recomputed at 1 Hz for plot history.

---

### 2.7 Pedestrian Counts & Crossing Safety
- **Data Source**: `ped::PedestrianManager::GetActiveCount()`, `GetTotalCompleted()`, `GetIllegalCrossingStarts()`.
- **Description**:
  - $P_{\text{active}}$: Count of pedestrians currently in `Crossing` or `Waiting` state.
  - $P_{\text{completed}}$: Cumulative count of pedestrians who successfully reached destination sidewalks.
  - $P_{\text{violations}}$: Illegal crossing starts (pedestrians who stepped into crosswalk during solid or flashing `DONT_WALK`). Must be strictly 0.
- **Unit**: Pedestrians (integer count).
- **Measurement Interval**: Cumulative and instantaneous.
- **Reset Behaviour**: Reset to 0.
- **Pause Behaviour**: Freezes.
- **Validity Condition**: Always valid ($\ge 0$).
- **Update Rate**: 60 Hz.

---

### 2.8 Signal State & Phase Time Remaining
- **Data Source**: `traffic::TrafficSignalController::GetCurrentPhase()`, `GetPhaseElapsed()`, `GetPhaseDuration()`, `GetPhaseRemaining()`.
- **Description**: Real-time signal aspect, active NEMA phase name, elapsed phase time, total programmed duration, and remaining green/clearance seconds.
- **Formulas**:
  $$t_{\text{rem}} = \max(t_{\text{duration}} - t_{\text{elapsed}}, 0.0\text{ s})$$
- **Units**: Phase name (string), Time remaining in seconds ($\text{s}$, 1 decimal precision).
- **Measurement Interval**: Instantaneous.
- **Reset Behaviour**: Synchronized with controller reset.
- **Pause Behaviour**: Freezes.
- **Validity Condition**: Always valid.
- **Update Rate**: 60 Hz.

---

### 2.9 Intersection Congestion Index ($CI$) & Level of Service (LOS)
- **Data Source**: Approach queues $Q_{\text{metres}}[a]$, network approach capacity $C_{\text{approach}} = 120.0\text{ m} \times 2\text{ lanes} = 240.0\text{ m}$.
- **Formula**:
  Normalized intersection queue occupancy:
  $$CI = \frac{\sum_{a \in \{\text{N,E,S,W}\}} Q_{\text{metres}}[a]}{4 \times 120.0\text{ m}} = \frac{\sum Q_{\text{metres}}[a]}{480.0\text{ m}}$$
  Bounded $CI \in [0.0, 1.0]$.
- **Level of Service Mapping**:
  | Congestion Index ($CI$) | Qualitative Rating | Highway Capacity Manual (HCM) LOS Equivalent |
  | :--- | :--- | :--- |
  | $CI < 0.15$ | **Low (Free Flow)** | LOS A / B |
  | $0.15 \le CI < 0.40$ | **Medium (Stable Flow)** | LOS C |
  | $0.40 \le CI < 0.70$ | **High (Approaching Capacity)** | LOS D / E |
  | $CI \ge 0.70$ | **Severe (Gridlock Hazard)** | LOS F |
- **Unit**: Dimensionless ratio $[0, 1]$ and categorical status string.
- **Measurement Interval**: 5-second moving average.
- **Reset Behaviour**: Reset to $0.0$ ("Low").
- **Pause Behaviour**: Freezes.
- **Validity Condition**: Always valid.
- **Update Rate**: 60 Hz.

---

## 3. Physical Environmental Emissions Modeling (CO₂ & Fuel Consumption)

To provide actionable urban planning analytics, the `AnalyticsEngine` computes instantaneous and cumulative vehicle emissions derived from instantaneous engine power demand (Akçelik drive-cycle modal formulation):

### 3.1 Instantaneous Tractive Power ($P_{\text{tractive}}$)
For each vehicle $v_i$ with mass $M$, speed $v$, and acceleration $a$:
$$P_{\text{tractive}} = \max\left(0.0, \; M \cdot v \cdot a + M \cdot g \cdot C_r \cdot v + \frac{1}{2} \rho_{\text{air}} C_d A \cdot v^3\right)$$
Where:
- Sedan: $M = 1450\text{ kg}$, $A = 2.2\text{ m}^2$, $C_d = 0.31$, $C_r = 0.012$.
- Bus: $M = 11000\text{ kg}$, $A = 7.5\text{ m}^2$, $C_d = 0.65$, $C_r = 0.015$.
- Ambulance: $M = 3200\text{ kg}$, $A = 3.8\text{ m}^2$, $C_d = 0.45$, $C_r = 0.014$.
- Constants: $g = 9.81\text{ m/s}^2$, $\rho_{\text{air}} = 1.225\text{ kg/m}^3$.

### 3.2 Fuel Rate & CO₂ Emission Rate
- **Idle Rate** ($v < 0.1\text{ m/s}$): $\dot{F}_{\text{idle}} \approx 0.28\text{ mL/s}$ (Sedan), $1.10\text{ mL/s}$ (Bus).
- **Cruise/Drive Rate**: $\dot{F} = \dot{F}_{\text{idle}} + \beta \cdot P_{\text{tractive}}$ ($\beta \approx 0.065\text{ mL/kJ}$).
- **CO₂ Conversion**: Combustion of gasoline/diesel produces $2.31\text{ kg CO}_2\text{ per litre}$ of fuel ($2.31\text{ g/mL}$).
  $$\dot{E}_{\text{CO2}} = \dot{F} \times 2.31\text{ g/mL}$$
- **Outputs**:
  - Instantaneous CO₂ Rate: $\text{g/s}$.
  - Cumulative CO₂ Released: $\text{kg}$.
  - Cumulative Fuel Consumed: Litres ($\text{L}$).


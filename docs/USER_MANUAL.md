# SmartCity 3D — User Manual & Operations Guide

Welcome to **SmartCity 3D**, an interactive real-time 3D urban traffic simulator and computer graphics showcase. This manual explains how to operate the application, navigate camera modes, manipulate traffic control systems, adjust atmospheric parameters, and explore the vehicle customization studio.

---

## 1. System Requirements

| Component | Minimum Requirements | Recommended Specifications |
| :--- | :--- | :--- |
| **Operating System** | Microsoft Windows 10 (64-bit) | Microsoft Windows 11 (64-bit) |
| **Processor (CPU)** | Intel Core i5-8400 / AMD Ryzen 5 2600 | Intel Core i7-12700 / AMD Ryzen 7 5800X or better |
| **Memory (RAM)** | 8 GB RAM | 16 GB DDR4/DDR5 |
| **Graphics (GPU)** | NVIDIA GTX 1060 / AMD RX 580 (4GB VRAM) | NVIDIA RTX 3060 / 4060 or better (8GB VRAM) |
| **OpenGL Support** | OpenGL 3.3 Core Profile compatible driver | OpenGL 4.6 Core Profile driver |
| **Display Resolution**| 1280 x 720 | 1920 x 1080 or 2560 x 1440 |

---

## 2. Quick Start

### Running the Pre-Built Release
1. Extract `SmartCity3D-1.0.0-Windows-x64.zip` into any folder (e.g. `C:\Games\SmartCity3D`).
2. Double-click `SmartCity3D.exe`.
3. The simulator initializes at 1600x900 resolution with the interactive Dear ImGui interface ready for input.

### Command-Line Arguments
The application supports multiple command-line flags for automation, benchmarking, and headless testing:

```powershell
# Launch in automated 10-second benchmark mode (NVIDIA RTX 4060 profile)
.\SmartCity3D.exe --benchmark --benchmark-duration 10.0 --benchmark-vehicles 45

# Benchmark without shadows or rain to profile baseline rasterization
.\SmartCity3D.exe --benchmark --no-shadows --no-rain

# Run in headless mode for 1000 fixed simulation ticks (no window created)
.\SmartCity3D.exe --headless --ticks 1000

# Capture official release screenshots and exit cleanly
.\SmartCity3D.exe --release-screenshots

# Run in kiosk demonstration mode with rotating camera and dynamic time-of-day
.\SmartCity3D.exe --demo
```

---

## 3. Camera Navigation & Controls

The simulator provides three distinct camera modes to inspect the intersection from any vantage point.

### Camera Mode Controls

| Mode | Shortcut | Controls |
| :--- | :--- | :--- |
| **Orbit Camera** | `F1` | • **Left-Click + Drag**: Rotate azimuth and elevation around intersection<br>• **Scroll Wheel**: Zoom distance in/out ($10\text{ m}$ to $250\text{ m}$)<br>• **Middle-Click + Drag**: Pan orbit focus point |
| **FreeFly Camera**| `F2` | • **W / S**: Move forward / backward along camera look vector<br>• **A / D**: Strafe left / right<br>• **Q / E**: Move downward / upward vertically<br>• **Right-Click + Drag**: Freelook mouse look (yaw and pitch)<br>• **Shift (Hold)**: Double flight speed |
| **Top-Down Ortho**| `F3` | • **W / A / S / D**: Pan orthographic plane across intersection<br>• **Scroll Wheel**: Adjust orthographic viewport scale ($20\text{ m}$ to $300\text{ m}$) |

*Camera bounds strictly prevent clipping below road surface ($y \ge 0.5\text{ m}$), and pitch is clamped to $[-89^\circ, +89^\circ]$ to eliminate gimbal lock.*

---

## 4. Dear ImGui Control Panels

All simulation and rendering parameters are exposed via docking Dear ImGui panels. You can drag and dock panels anywhere or toggle their visibility under the **Panels** menu.

### 4.1 Traffic & Simulation Controls
- **Play / Pause**: Instantly halts or resumes fixed-timestep vehicle dynamics.
- **Single Step (`Space`)**: Advances the simulation by exactly one physics tick ($\Delta t = 1/60\text{ s}$).
- **Time Scale Slider**: Adjust simulation speed from $0.1\times$ (slow motion) to $10.0\times$ (high-speed traffic analysis).
- **Signal Control Mode Selector**:
  - `Fixed-Time`: Deterministic cyclic timing based on pre-configured phase durations.
  - `Actuated`: Sensor-driven extension of green phases up to maximum green timers.
  - `Adaptive (Max-Pressure)`: Dynamically shifts green phases based on real-time inbound and outbound queue pressures.
- **Manual Phase Force**: Buttons to manually trigger `NS_GREEN` or `EW_GREEN` for testing.
- **Spawn Emergency Vehicle**: Spawns an ambulance on the selected approach, triggering preemption logic, clearing conflicting traffic, and activating sirens.

### 4.2 Lighting & Atmosphere Panel
- **Time of Day Slider**: Adjust the clock between `00:00` and `24:00`. The astronomical solar arc automatically calculates sun/moon azimuth, elevation, sky colors, and ambient lighting.
- **Sun Animation Speed**: Toggle real-time diurnal progression with adjustable minute multipliers.
- **Directional Shadows**: Toggle $2048 \times 2048$ shadow mapping on/off, adjust shadow bias, and select PCF filter samples ($1\times, 4\times, 16\times$).
- **Volumetric Fog**: Adjust atmospheric fog density and distance falloff.
- **Weather / Rain**: Toggle GPU-accelerated rain simulation, adjust precipitation density (up to 10,000 active particles), and control road surface wetness reflectivity.

### 4.3 Analytics & Metrics Dashboard
The Analytics Dashboard plots real-time traffic telemetry updated at 60 Hz:
- **Active Vehicles**: Total live vehicles currently traveling through the network.
- **Queue Lengths**: Measured per approach in both vehicle counts and linear meters.
- **Average Speed**: Mean instantaneous speed versus the $50\text{ km/h}$ design speed limit.
- **Average Delay & Wait Time**: Mean seconds vehicles spend stopped at red signals.
- **Fuel & Emissions**: Instantaneous and cumulative liters of fuel consumed and kilograms of CO2 emitted.

### 4.4 Comparative Experiment Runner
- Execute multi-seed batch tests directly within the application.
- Select test duration (e.g. 120 seconds) and number of seeds (e.g. 3 or 5).
- Click **Run Comparative Study** to run Fixed vs. Actuated vs. Adaptive back-to-back.
- Results are displayed in a clean comparative variance table and can be exported directly to JSON or CSV.

---

## 5. Vehicle Customization Studio

SmartCity 3D includes a comprehensive vehicle customization system supporting GTA V Cherax entity presets.

### 5.1 Vehicle Gallery Panel
- Browse through 66 normalized vehicle presets.
- Full-text search by model name (e.g. "Jesko", "M3", "Growler", "Fast bike").
- Filter by vehicle class (`Sedan`, `Sports`, `Super`, `SUV`, `Motorcycle`, `Emergency`).
- Asset status indicators: `AVAILABLE` (custom 3D mesh) vs `MISSING_MODEL` (rendered with procedural CAD chassis).

### 5.2 Vehicle Details & Mod Inspector
- **Identity Tab**: View vehicle ID, model hash, display name, class, and dimensions.
- **Modification Slots (`mod0`..`mod49`)**: Inspect installed upgrades including spoilers, front bumpers, exhausts, roll cages, engine upgrades, and turbo tuning.
- **Extras (`extra0`..`extra15`)**: View active toggleable body components.
- **Raw JSON Inspector**: View the normalized JSON configuration structure.

### 5.3 3D Turntable Preview
- Real-time offscreen 3D rendering preview rendered to a $512 \times 384$ Framebuffer Object (FBO).
- **Auto-Rotate**: Smooth 360-degree turntable rotation.
- **Manual Orbit**: Click and drag inside the viewport to inspect modifications from any angle.
- **Wheel Steer & Spin**: Sliders to test front wheel steering angles and tire rotation.
- **Spawn into Simulation**: Click **Spawn Vehicle** to drop the customized preset directly into the active traffic simulation network!

### 5.4 Live Customization Studio
- **Primary & Secondary Paint**: Adjust body color using interactive HDR color pickers or select from 160 authentic GTA factory color swatches.
- **Neon Underglow**: Toggle front, back, left, and right neon tubes with full RGB glow control.
- **License Plate**: Edit the custom vehicle license plate text in real time.

---

## 6. Troubleshooting & FAQ

### Q: The application fails to launch or reports an OpenGL error.
**A**: Ensure your graphics drivers are up to date. SmartCity 3D requires an OpenGL 3.3 Core Profile compatible GPU and driver. If running on a laptop with dual GPUs (Intel/AMD integrated + NVIDIA discrete), ensure the application is set to run on the High-Performance discrete GPU via Windows Graphics Settings or NVIDIA Control Panel.

### Q: The window opens but shows a black screen.
**A**: Verify that the `shaders/` directory is located in the same folder as `SmartCity3D.exe` or in the root project directory. The engine requires vertex and fragment shaders for the geometry and skybox passes.

### Q: How do I hide the Dear ImGui interface for clean screenshots?
**A**: Press `F11` or toggle the **Show UI** option in the View menu. Press `F12` or run `SmartCity3D.exe --release-screenshots` to automatically output high-resolution screenshots to the `screenshots/` directory.


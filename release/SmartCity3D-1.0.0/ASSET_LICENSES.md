# SmartCity 3D — Asset Licenses & Asset Register

All assets, models, materials, shaders, configuration files, and presets utilized within SmartCity 3D conform to open source (CC0, CC-BY, or MIT) licensing.

## 1. 3D Geometry & Procedural Infrastructure Register

> [!NOTE]
> All 3D urban infrastructure assets (traffic signal poles, pedestrian signals, streetlights, traffic signs, benches, trees, bus stops, and trash bins) are **generated procedurally in clean C++ code** using vertex buffers, indexed geometry, and procedural primitives. They are not pre-baked or falsely claimed to be Blender-made.

| Asset Name | Generation / Source | Vertices | Triangles | License | Attribution & Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Traffic Signal Mast Arm Pole** | Procedural C++ (`StreetFurniture.cpp`) | 196 | 184 | MIT / Project Original | Octagonal tapered vertical pole (6.5m) with horizontal cantilever arm (8.5m) |
| **Vehicular Signal Head (3-Lens)** | Procedural C++ (`StreetFurniture.cpp`) | 120 | 112 | MIT / Project Original | Visored housing with 3 emissive spherical lenses (Red, Yellow, Green) |
| **Pedestrian Signal Head (2-Lens)** | Procedural C++ (`StreetFurniture.cpp`) | 84 | 78 | MIT / Project Original | Dual-aspect enclosure with Walk (white/green) and Don't Walk (orange/red) lenses |
| **Urban Streetlight Pole & Luminaire**| Procedural C++ (`StreetFurniture.cpp`) | 168 | 152 | MIT / Project Original | 8.0m mast with curved gooseneck arm and downward-directed lamp housing |
| **Park Bench** | Procedural C++ (`StreetFurniture.cpp`) | 96 | 88 | MIT / Project Original | Cast-iron support frames with horizontal cedar wood slats |
| **Roadside Traffic Signs** | Procedural C++ (`StreetFurniture.cpp`) | 72 | 64 | MIT / Project Original | Tubular galvanized post with reflective octagonal/circular aluminum sign plates |
| **Urban Shade Tree** | Procedural C++ (`StreetFurniture.cpp`) | 224 | 210 | MIT / Project Original | Tapered trunk cylinder with dual-sphere leafy deciduous canopy |
| **Transit Bus Stop Shelter** | Procedural C++ (`StreetFurniture.cpp`) | 148 | 136 | MIT / Project Original | Structural steel frame with tinted acrylic roof canopy and rear windscreen |
| **Street Litter Bin** | Procedural C++ (`StreetFurniture.cpp`) | 64 | 56 | MIT / Project Original | Heavy-duty ribbed cylindrical receptacle |
| **Modular Urban Buildings (12 types)**| Procedural C++ (`BuildingGenerator.cpp`)| 48-180 | 36-150 | MIT / Project Original | Varied heights (22m-56m), setback stepped roofs, and procedural multi-window grids |
| **Asphalt Road Surface & Curbs** | Procedural C++ (`IntersectionGeometry.cpp`)| 512 | 480 | MIT / Project Original | 4-approach 120m roadway network with rounded curb returns and sidewalk ramps |
| **Thermoplastic Road Markings** | Procedural C++ (`RoadMarkings.cpp`) | 1,024 | 960 | MIT / Project Original | Raised anti-z-fighting zebra crossings, stop bars, double yellow and lane dashes |
| **Test Cube Mesh** | Assimp Disk Model (`assets/models/test_cube.obj`) | 24 | 12 | MIT / Project Original | Diagnostic 3D cube model for ResourceManager verification |
| **Proportional Vehicle CAD Chassis** | Procedural C++ (`VehicleCustomization.cpp`)| 24 (box) + 68 (wheels) | 12 (box) + 32 (wheels) | MIT / Project Original | CAD chassis with animated spinning/steered cylinder wheels and underglow quad |

---

## 2. Configuration & Preset Data

| Asset Path | Type | License | Attribution & Notes |
| :--- | :--- | :--- | :--- |
| `assets/config/simulation_config.json` | JSON Config | MIT | Default simulation physics, speeds, IDM parameters |
| `assets/config/intersection_config.json` | JSON Config | MIT | Intersection lane widths, curb radii, approach lengths |
| `assets/config/signals.json` | JSON Config | MIT | Traffic signal timings, min/max green, clearance parameters |
| `assets/config/pedestrians.json` | JSON Config | MIT | Pedestrian spawn rates, walking speeds, compliance |
| `assets/config/vehicles.json` | JSON Config | MIT | Vehicle classifications (Sedan, Bus, Ambulance) |
| `assets/vehicle_presets/source_json/*.json` | JSON Presets | Open Community / Research | Cherax Entity exports for traffic simulation research |
| `assets/vehicle_presets/normalized/*.json` | JSON Presets | MIT | Normalized SmartCity 3D vehicle customization database |
| `assets/vehicle_presets/vehicle_assets.json` | JSON Manifest | MIT | Vehicle 3D asset metadata and IDM simulation profile registry |
| `assets/vehicle_presets/import_report.json` | JSON Report | MIT | Automated import verification report |

---

## 3. Shaders

All GLSL 330 core shaders located in `/shaders/` are original project code released under the MIT License:
- `shaders/scene.vert`, `shaders/scene.frag`: Blinn-Phong lighting, directional shadow mapping, fog, day/night transitions, wetness.
- `shaders/shadow_depth.vert`, `shaders/shadow_depth.frag`: Directional light-space depth pass for variance shadow mapping.
- `shaders/skybox.vert`, `shaders/skybox.frag`: Procedural atmospheric Rayleigh/Mie horizon and sun disk rendering.
- `shaders/light_indicator.vert`, `shaders/light_indicator.frag`: Emissive signal lenses, brake lights, neon underglow, debug overlays.
- `shaders/rain.vert`, `shaders/rain.frag`: 10,000 streak particle simulation with motion blur and depth attenuation.

---

## 4. External Textures & Fallback Policy

- Textures utilize power-of-two dimensions with trilinear mipmapping.
- When external bitmap texture files are not located on disk, all materials seamlessly default to physically calibrated procedural diffuse/specular colors and procedural UV grids, ensuring zero crashes and zero missing-texture artifacts.

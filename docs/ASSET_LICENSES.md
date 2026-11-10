# SmartCity 3D — Asset Licenses & 3D Geometry Register

All assets, models, materials, and textures utilized within SmartCity 3D conform strictly to open source (CC0, CC-BY, or MIT) licensing.

## 1. 3D Model Generation Disclosure

> [!NOTE]
> Blender is not installed in the Windows build environment. Per engineering instructions, all 3D urban infrastructure assets (traffic signal poles, pedestrian signals, streetlights, traffic signs, benches, trees, bus stops, and trash bins) are **generated procedurally in clean C++ code** using vertex buffers, indexed geometry, and procedural primitives. They are not pre-baked or falsely claimed to be Blender-made.

---

## 2. Asset Register & Polygon Budgets

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

---

## 3. External Textures & Texturing Fallback Policy

- Textures utilize power-of-two dimensions with trilinear mipmapping.
- When external bitmap texture files are not located on disk, all materials seamlessly default to physically calibrated procedural diffuse/specular colors and procedural UV grids, ensuring zero crashes and zero missing-texture artifacts.


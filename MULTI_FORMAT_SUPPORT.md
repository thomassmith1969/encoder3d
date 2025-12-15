# Multi-Format Model Support

## Overview
The Encoder3D web interface now supports multiple 3D model and 2D vector formats, enabling it to be used as a universal CNC control platform for 3D printing, milling, and laser cutting.

## Implemented Formats

### 3D Model Formats
| Format | Extension | Status | Use Case | Features |
|--------|-----------|--------|----------|----------|
| **STL** | `.stl` | ✅ Fully Implemented | 3D Printing, Milling | Binary & ASCII, industry standard |
| **OBJ** | `.obj` | ✅ Fully Implemented | 3D Printing, Milling | Supports materials, vertex normals |
| **PLY** | `.ply` | ✅ Fully Implemented | 3D Scans | Vertex colors, binary & ASCII |

### 2D Vector Formats
| Format | Extension | Status | Use Case | Features |
|--------|-----------|--------|----------|----------|
| **SVG** | `.svg` | ✅ Fully Implemented | Laser Cutting, Engraving | Paths, circles, rectangles, auto-extrude for preview |

### Planned Formats
| Format | Extension | Status | Use Case |
|--------|-----------|--------|----------|
| **3MF** | `.3mf` | 🚧 Planned | Modern 3D printing with multi-material support |
| **DXF** | `.dxf` | 🚧 Planned | 2D CAD for laser cutting and light milling |
| **STEP** | `.step`, `.stp` | 🔮 Future | High-precision CAD models |
| **IGES** | `.iges`, `.igs` | 🔮 Future | CAD exchange format |

## File Loaders

Custom Three.js loaders are implemented for each format:

### 1. STLLoader.js
- Binary and ASCII STL parsing
- Automatic normal computation
- Volume calculation for material estimation

### 2. OBJLoader.js
- Wavefront OBJ parser
- Handles vertex positions, normals, UVs
- Multi-object support
- Automatic material application

### 3. PLYLoader.js
- Stanford PLY format (binary & ASCII)
- Vertex color support
- Point cloud data handling

### 4. SVGLoader.js
- SVG path parsing
- Shape extraction (paths, circles, rectangles)
- Automatic 2D-to-3D extrusion for visualization
- Suitable for laser cutting toolpath generation

## Usage

### Loading Models
1. Click **"+ Add Model"** button in the Objects panel (bottom-left)
2. Select a file with supported extension
3. Model will be loaded, centered, and displayed in the 3D viewer
4. Multiple models can be loaded and manipulated independently

### Multi-Object Management
- **Load Multiple Files**: Each model is added as a separate object
- **Select Objects**: Click on objects in the list to select
- **Transform Objects**: Adjust position, rotation, and scale per object
- **Remove Objects**: Click × button next to object name
- **View Toggles**: Show/hide toolhead, GCode preview, and loaded objects

### Slicing/Toolpath Generation
When slicing for 3D printing:
- All loaded objects are combined with their transforms applied
- Individual object positions and rotations are preserved in the GCode output
- Slicer settings (layer height, infill, temperature) apply to all objects

For laser cutting (SVG):
- 2D paths are extracted from the SVG
- Can be used to generate cutting/engraving toolpaths
- Extrusion depth is for visualization only

## Technical Implementation

### Format Detection
File format is automatically detected based on file extension:
```javascript
const extension = file.name.split('.').pop().toLowerCase();
```

### Loading Pipeline
1. **File Selection**: User selects file via file input
2. **Format Detection**: Extension-based router
3. **Data Parsing**: Format-specific loader parses the data
4. **Geometry Creation**: Three.js geometry/mesh objects created
5. **Scene Integration**: Object added to scene with proper centering
6. **UI Update**: Object list, info panel, and camera adjusted

### Memory Efficiency
- Files are read once and processed client-side
- Three.js BufferGeometry for efficient GPU rendering
- Geometry shared across cloned objects (future optimization)

## Future Enhancements

### 3MF Support
- ZIP archive extraction
- Multi-material parsing
- Texture support
- Build plate metadata

### DXF Support
- 2D entity parsing (lines, arcs, circles, polylines)
- Layer management
- Coordinate system conversion
- 2.5D toolpath generation

### Advanced Features
- **Format Conversion**: Convert between formats in-browser
- **Geometry Repair**: Auto-fix non-manifold meshes
- **Nesting/Packing**: Optimize build plate usage
- **Hybrid Operations**: Combine 3D printing with laser engraving

## Integration with CNC Operations

### 3D Printing
- STL/OBJ/PLY → Slicer → GCode → Print

### CNC Milling
- STL/OBJ/PLY → Toolpath Generator → GCode → Mill
- DXF → 2.5D Pocketing/Profiling → GCode → Mill

### Laser Cutting/Engraving
- SVG/DXF → Vector Paths → GCode → Cut/Engrave
- Raster Images → Grayscale → Power Modulation → Engrave

## Architecture Benefits

1. **Unified Interface**: Single web UI for all CNC operations
2. **Client-Side Processing**: No server upload required, works offline
3. **Real-Time Preview**: See models immediately in 3D viewer
4. **Extensible**: Easy to add new format loaders
5. **Open Source**: All loaders are custom implementations

## Notes

- All coordinate transformations preserve the encoder-driven PID control system
- GCode generation maintains compatibility with the custom firmware
- Multi-material support will require firmware updates for tool changes
- Laser power modulation hooks into existing PWM channels

# Advanced Slicer Features

This document summarizes the advanced slicer features that have been implemented.

## UI Controls (index.html)

Added comprehensive slicer configuration controls:

- **Walls (perimeters)**: Number of perimeter wall loops (0-10)
- **Line Width**: Extrusion width in mm (default: 0.4mm)
- **Infill Pattern**: Dropdown selector
  - Rectilinear (parallel lines)
  - Grid (perpendicular crosses)
  - Honeycomb (hexagonal pattern)
- **Infill Angle**: Base angle for infill lines in degrees (0-180°)
- **Infill Overlap**: Percentage overlap into perimeter region (0-100%)
- **Min Segment Length**: Minimum line segment length in mm (filters tiny segments)
- **Perimeter Speed**: Print speed for perimeter walls in mm/s
- **Infill Speed**: Print speed for infill in mm/s

## Settings Integration (app.js)

All UI controls are properly wired to read values and pass them to the slicer:

```javascript
const settings = {
    // Basic settings
    layerHeight, maxZHeight, infillDensity, nozzleTemp, bedTemp,
    
    // Perimeter / Walls
    wallCount, lineWidth, perimeterSpeed,
    
    // Infill
    infillPattern, infillAngle, infillOverlap, infillSpeed,
    
    // Advanced
    minSegmentLength
};
```

## Path Generation (slicer-paths.js)

### Multi-Wall Perimeters
- Generates multiple concentric wall loops based on `wallCount`
- Each wall offset by `lineWidth` distance
- Filters out degenerate paths using `minSegmentLength`
- Distinguishes outer-wall vs inner-wall for speed control

### Infill Angle Control
- Base angle configurable via `infillAngle` parameter
- Supports layer alternation (0° → 90° on alternate layers)
- All patterns respect the angle setting

### Infill Overlap
- Extends infill segments by percentage of line width
- Improves perimeter-infill adhesion
- Configured via `infillOverlap` (percentage 0-100)

### Min Segment Filtering
- `simplifyPathByMinLength()` removes vertices creating short segments
- Applied to both perimeters and infill
- Prevents tiny extrusions that can cause print quality issues

## GCode Generation (slicer-gcode.js)

### Per-Section Speeds
- **Outer walls**: Uses `perimeterSpeed` (slower for quality)
- **Inner walls**: Uses `innerPerimeterSpeed` or falls back to `perimeterSpeed`
- **Infill**: Uses `infillSpeed` (faster for efficiency)
- Speeds converted to feedrate (mm/min) in generated GCode

## Default Settings (slicer-main.js)

Sane defaults for all new parameters:

```javascript
wallCount: 2
lineWidth: 0.4
perimeterSpeed: 50
infillSpeed: 80
infillAngle: 45
infillOverlap: 15
minSegmentLength: 0.3
```

## Unit Tests

All features validated with automated tests:

- `perimeter_test.js`: Validates wallCount and minSegmentLength
- `infill_test.js`: Validates infillAngle configuration
- `infill_overlap_test.js`: Validates overlap extension
- `gcode_speed_test.js`: Validates per-section feedrates

All tests passing ✓

## Notes

- NO support generation features (those were removed per user request)
- Coordinate transform bug fix preserved (90° X rotation applied consistently)
- Module exports added for Node.js testing compatibility
- All code browser-compatible (classes work in both browser and Node)

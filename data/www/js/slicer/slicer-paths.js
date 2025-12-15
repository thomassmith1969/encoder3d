/**
 * Path Generator - Perimeters and Infill
 * Generates toolpaths from sliced layers
 */

class PathGenerator {
    constructor() {
        this.settings = {};
    }

    /**
     * Generate perimeters (outer walls) for a layer
     * @param {Array} contours - Closed contours from slicing
     * @param {Object} settings - Wall settings
     * @returns {Array} Perimeter paths
     */
    generatePerimeters(contours, settings) {
        this.settings = settings;
        const perimeters = [];
        const wallCount = settings.wallCount || 2;
        const lineWidth = settings.lineWidth || 0.4;
        
        contours.forEach(contour => {
            // Generate multiple wall loops
            for (let i = 0; i < wallCount; i++) {
                const offset = i * lineWidth;
                const offsetContour = this.offsetPath(contour, -offset);
                
                if (offsetContour && offsetContour.length > 2) {
                    perimeters.push({
                        type: i === 0 ? 'outer-wall' : 'inner-wall',
                        points: offsetContour,
                        width: lineWidth
                    });
                }
            }
        });
        
        return perimeters;
    }

    /**
     * Generate infill pattern for a layer
     * @param {Array} contours - Boundary contours
     * @param {Array} perimeters - Already generated perimeters
     * @param {Object} settings - Infill settings
     * @returns {Array} Infill paths
     */
    generateInfill(contours, perimeters, settings) {
        const infillDensity = (settings.infillDensity || 20) / 100;
        const lineWidth = settings.lineWidth || 0.4;
        const pattern = settings.infillPattern || 'rectilinear';
        
        if (infillDensity === 0) return [];
        
        // Get infill boundary (innermost perimeter)
        const boundary = this.getInfillBoundary(perimeters);
        if (!boundary || boundary.length < 3) return [];
        
        // Generate pattern based on type
        switch (pattern) {
            case 'rectilinear':
                return this.generateRectilinearInfill(boundary, infillDensity, lineWidth, settings);
            case 'grid':
                return this.generateGridInfill(boundary, infillDensity, lineWidth, settings);
            case 'honeycomb':
                return this.generateHoneycombInfill(boundary, infillDensity, lineWidth, settings);
            default:
                return this.generateRectilinearInfill(boundary, infillDensity, lineWidth, settings);
        }
    }

    /**
     * Generate rectilinear (lines) infill pattern
     */
    generateRectilinearInfill(boundary, density, lineWidth, settings) {
        const infill = [];
        const spacing = lineWidth / density;
        const angle = (settings.layerIndex % 2) * 90; // Alternate 0° and 90°
        
        // Get bounding box
        const bbox = this.getBoundingBox(boundary);
        const diagonal = Math.sqrt(
            Math.pow(bbox.max.x - bbox.min.x, 2) + 
            Math.pow(bbox.max.y - bbox.min.y, 2)
        );
        
        // Generate scan lines
        const numLines = Math.ceil(diagonal / spacing);
        const angleRad = angle * Math.PI / 180;
        const cosA = Math.cos(angleRad);
        const sinA = Math.sin(angleRad);
        
        for (let i = 0; i < numLines; i++) {
            const offset = bbox.min.x + (i * spacing) - diagonal / 2;
            
            // Create scan line
            const lineStart = {
                x: bbox.min.x + offset * cosA - diagonal * sinA,
                y: bbox.min.y + offset * sinA + diagonal * cosA
            };
            const lineEnd = {
                x: bbox.min.x + offset * cosA + diagonal * sinA,
                y: bbox.min.y + offset * sinA - diagonal * cosA
            };
            
            // Intersect with boundary
            const intersections = this.linePolygonIntersection(lineStart, lineEnd, boundary);
            
            // Create infill segments from intersection pairs
            for (let j = 0; j < intersections.length - 1; j += 2) {
                if (intersections[j + 1]) {
                    infill.push({
                        type: 'infill',
                        points: [intersections[j], intersections[j + 1]],
                        width: lineWidth
                    });
                }
            }
        }
        
        return infill;
    }

    /**
     * Generate grid infill (perpendicular lines)
     */
    generateGridInfill(boundary, density, lineWidth, settings) {
        // Generate horizontal lines
        const horizontal = this.generateRectilinearInfill(boundary, density, lineWidth, 
            { ...settings, layerIndex: 0 });
        
        // Generate vertical lines
        const vertical = this.generateRectilinearInfill(boundary, density, lineWidth, 
            { ...settings, layerIndex: 1 });
        
        return [...horizontal, ...vertical];
    }

    /**
     * Generate honeycomb infill (hexagonal pattern)
     */
    generateHoneycombInfill(boundary, density, lineWidth, settings) {
        const infill = [];
        const spacing = lineWidth / density;
        const hexSize = spacing * 2;
        
        // Get bounding box
        const bbox = this.getBoundingBox(boundary);
        
        // Generate hexagonal grid
        const rows = Math.ceil((bbox.max.y - bbox.min.y) / (hexSize * 0.866));
        const cols = Math.ceil((bbox.max.x - bbox.min.x) / (hexSize * 1.5));
        
        for (let row = 0; row < rows; row++) {
            for (let col = 0; col < cols; col++) {
                const cx = bbox.min.x + col * hexSize * 1.5;
                const cy = bbox.min.y + row * hexSize * 0.866 + (col % 2) * hexSize * 0.433;
                
                // Generate hexagon edges
                const hex = this.generateHexagon(cx, cy, hexSize);
                
                // Clip hexagon to boundary
                hex.forEach((edge, i) => {
                    const nextEdge = hex[(i + 1) % hex.length];
                    if (this.isLineInPolygon(edge, nextEdge, boundary)) {
                        infill.push({
                            type: 'infill',
                            points: [edge, nextEdge],
                            width: lineWidth
                        });
                    }
                });
            }
        }
        
        return infill;
    }

    /**
     * Offset a path inward or outward
     */
    offsetPath(path, offset) {
        // Simple offset - for production use Clipper library
        if (path.length < 3) return null;
        
        const offsetPath = [];
        
        for (let i = 0; i < path.length; i++) {
            const prev = path[(i - 1 + path.length) % path.length];
            const curr = path[i];
            const next = path[(i + 1) % path.length];
            
            // Calculate perpendicular offset vectors
            const v1 = this.perpendicular(prev, curr, offset);
            const v2 = this.perpendicular(curr, next, offset);
            
            // Average the offsets (simple miter)
            offsetPath.push({
                x: curr.x + (v1.x + v2.x) / 2,
                y: curr.y + (v1.y + v2.y) / 2
            });
        }
        
        return offsetPath;
    }

    /**
     * Calculate perpendicular offset vector
     */
    perpendicular(p1, p2, distance) {
        const dx = p2.x - p1.x;
        const dy = p2.y - p1.y;
        const len = Math.sqrt(dx * dx + dy * dy);
        
        if (len === 0) return { x: 0, y: 0 };
        
        return {
            x: -dy / len * distance,
            y: dx / len * distance
        };
    }

    /**
     * Get bounding box of points
     */
    getBoundingBox(points) {
        const bbox = {
            min: { x: Infinity, y: Infinity },
            max: { x: -Infinity, y: -Infinity }
        };
        
        points.forEach(p => {
            bbox.min.x = Math.min(bbox.min.x, p.x);
            bbox.min.y = Math.min(bbox.min.y, p.y);
            bbox.max.x = Math.max(bbox.max.x, p.x);
            bbox.max.y = Math.max(bbox.max.y, p.y);
        });
        
        return bbox;
    }

    /**
     * Find intersections between line and polygon
     */
    linePolygonIntersection(lineStart, lineEnd, polygon) {
        const intersections = [];
        
        for (let i = 0; i < polygon.length; i++) {
            const p1 = polygon[i];
            const p2 = polygon[(i + 1) % polygon.length];
            
            const intersection = this.lineLineIntersection(lineStart, lineEnd, p1, p2);
            if (intersection) {
                intersections.push(intersection);
            }
        }
        
        // Sort intersections along line direction
        intersections.sort((a, b) => {
            const distA = Math.sqrt(
                Math.pow(a.x - lineStart.x, 2) + 
                Math.pow(a.y - lineStart.y, 2)
            );
            const distB = Math.sqrt(
                Math.pow(b.x - lineStart.x, 2) + 
                Math.pow(b.y - lineStart.y, 2)
            );
            return distA - distB;
        });
        
        return intersections;
    }

    /**
     * Line-line intersection
     */
    lineLineIntersection(p1, p2, p3, p4) {
        const x1 = p1.x, y1 = p1.y;
        const x2 = p2.x, y2 = p2.y;
        const x3 = p3.x, y3 = p3.y;
        const x4 = p4.x, y4 = p4.y;
        
        const denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (Math.abs(denom) < 0.0001) return null;
        
        const t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
        const u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;
        
        if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
            return {
                x: x1 + t * (x2 - x1),
                y: y1 + t * (y2 - y1)
            };
        }
        
        return null;
    }

    /**
     * Get infill boundary from perimeters
     */
    getInfillBoundary(perimeters) {
        if (perimeters.length === 0) return null;
        
        // Return innermost perimeter
        const innerPerimeters = perimeters.filter(p => p.type === 'inner-wall');
        if (innerPerimeters.length > 0) {
            return innerPerimeters[innerPerimeters.length - 1].points;
        }
        
        return perimeters[perimeters.length - 1].points;
    }

    /**
     * Generate hexagon vertices
     */
    generateHexagon(cx, cy, size) {
        const hex = [];
        for (let i = 0; i < 6; i++) {
            const angle = (Math.PI / 3) * i;
            hex.push({
                x: cx + size * Math.cos(angle),
                y: cy + size * Math.sin(angle)
            });
        }
        return hex;
    }

    /**
     * Check if line is inside polygon
     */
    isLineInPolygon(p1, p2, polygon) {
        const midpoint = {
            x: (p1.x + p2.x) / 2,
            y: (p1.y + p2.y) / 2
        };
        return this.pointInPolygon(midpoint, polygon);
    }

    /**
     * Point in polygon test (ray casting)
     */
    pointInPolygon(point, polygon) {
        let inside = false;
        for (let i = 0, j = polygon.length - 1; i < polygon.length; j = i++) {
            const xi = polygon[i].x, yi = polygon[i].y;
            const xj = polygon[j].x, yj = polygon[j].y;
            
            const intersect = ((yi > point.y) !== (yj > point.y)) &&
                (point.x < (xj - xi) * (point.y - yi) / (yj - yi) + xi);
            if (intersect) inside = !inside;
        }
        return inside;
    }
}

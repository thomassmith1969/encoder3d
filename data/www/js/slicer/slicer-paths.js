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
        const minSeg = settings.minSegmentLength || 0.3;
        
        contours.forEach(contour => {
            // Generate multiple wall loops
            for (let i = 0; i < wallCount; i++) {
                const offset = i * lineWidth;
                const offsetContour = this.offsetPath(contour, -offset);
                
                // Remove tiny segments and degenerate contours
                const clean = this.simplifyPathByMinLength(offsetContour, minSeg);
                if (clean && clean.length > 2) {
                    perimeters.push({
                        type: i === 0 ? 'outer-wall' : 'inner-wall',
                        points: clean,
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
        const angle = (settings.infillAngle || 0) + ((settings.layerIndex || 0) % 2) * 90; // support angle + optional alternation
        const minSeg = settings.minSegmentLength || 0.3;
        const overlapPercent = settings.infillOverlap || 0;
        const overlapDist = (overlapPercent / 100) * lineWidth;
        
        // Get bounding box
        const bbox = this.getBoundingBox(boundary);
        const width = bbox.max.x - bbox.min.x;
        const height = bbox.max.y - bbox.min.y;
        const diagonal = Math.sqrt(width * width + height * height);
        const length = diagonal * 1.5;
        
        // Generate scan lines
        const numLines = Math.ceil(diagonal / spacing) + 2;
        const angleRad = angle * Math.PI / 180;
        const cosA = Math.cos(angleRad);
        const sinA = Math.sin(angleRad);
        const center = { x: (bbox.min.x + bbox.max.x) / 2, y: (bbox.min.y + bbox.max.y) / 2 };
        
        for (let i = -Math.floor(numLines/2); i <= Math.floor(numLines/2); i++) {
            const d = i * spacing;
            // normal direction to offset the line
            const nx = -sinA;
            const ny = cosA;
            const offX = center.x + nx * d;
            const offY = center.y + ny * d;

            // Line along angle passing through offX/offY
            const lineStart = {
                x: offX - cosA * (length / 2),
                y: offY - sinA * (length / 2)
            };
            const lineEnd = {
                x: offX + cosA * (length / 2),
                y: offY + sinA * (length / 2)
            };

            // Intersect with original boundary
            const intersections = this.linePolygonIntersection(lineStart, lineEnd, boundary);

            // Create infill segments from intersection pairs
            for (let j = 0; j < intersections.length - 1; j += 2) {
                const a = intersections[j];
                const b = intersections[j + 1];
                if (b) {
                    const dx = a.x - b.x;
                    const dy = a.y - b.y;
                    const segLen = Math.sqrt(dx*dx + dy*dy);
                    if (segLen >= minSeg) {
                        // Extend endpoints by overlap distance along segment direction
                        let newA = a;
                        let newB = b;
                        if (overlapDist > 0) {
                            const dirX = (b.x - a.x) / segLen;
                            const dirY = (b.y - a.y) / segLen;
                            newA = { x: a.x - dirX * overlapDist, y: a.y - dirY * overlapDist };
                            newB = { x: b.x + dirX * overlapDist, y: b.y + dirY * overlapDist };
                        }

                        const newDx = newA.x - newB.x;
                        const newDy = newA.y - newB.y;
                        const newLen = Math.sqrt(newDx*newDx + newDy*newDy);

                        if (newLen >= minSeg) {
                            infill.push({
                                type: 'infill',
                                points: [newA, newB],
                                width: lineWidth
                            });
                        }
                    }
                }
            }
        }
        
        return infill;
    }

    /**
     * Generate grid infill (perpendicular lines)
     */
    generateGridInfill(boundary, density, lineWidth, settings) {
        // First pass at requested angle
        const a0 = this.generateRectilinearInfill(boundary, density, lineWidth, 
            { ...settings, layerIndex: settings.layerIndex || 0 });
        
        // Second pass at +90°
        const a1 = this.generateRectilinearInfill(boundary, density, lineWidth, 
            { ...settings, layerIndex: settings.layerIndex || 0, infillAngle: (settings.infillAngle || 0) + 90 });
        
        return [...a0, ...a1];
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
     * Simplify path by removing vertices that create segments shorter than minLength
     */
    simplifyPathByMinLength(path, minLength) {
        if (!path || path.length < 3) return null;
        if (!minLength || minLength <= 0) return path;

        const simplified = [];
        for (let i = 0; i < path.length; i++) {
            const prev = simplified.length ? simplified[simplified.length - 1] : null;
            const curr = path[i];
            if (!prev) {
                simplified.push(curr);
                continue;
            }
            const dx = curr.x - prev.x;
            const dy = curr.y - prev.y;
            const dist = Math.sqrt(dx * dx + dy * dy);
            if (dist >= minLength) {
                simplified.push(curr);
            }
        }

        // Ensure closed loop: check first and last
        if (simplified.length >= 3) {
            const first = simplified[0];
            const last = simplified[simplified.length - 1];
            const dx = first.x - last.x;
            const dy = first.y - last.y;
            if (Math.sqrt(dx * dx + dy * dy) < minLength) {
                simplified.pop();
            }
        }

        return simplified.length >= 3 ? simplified : null;
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

// Export for Node.js testing
if (typeof module !== 'undefined' && module.exports) {
    module.exports = PathGenerator;
}

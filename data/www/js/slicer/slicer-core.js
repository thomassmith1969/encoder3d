/**
 * Slicer Core Engine - Pure JavaScript
 * Slices 3D meshes into 2D layers for GCode generation
 * No backend required - runs entirely in browser
 */

class SlicerCore {
    constructor() {
        this.layers = [];
        this.settings = {};
    }

    /**
     * Slice a Three.js geometry into layers
     * @param {THREE.BufferGeometry} geometry - The mesh to slice
     * @param {Object} transform - Position, rotation, scale transforms
     * @param {Object} settings - Slicer settings
     * @returns {Array} Array of layer objects
     */
    sliceGeometry(geometry, transform, settings) {
        this.settings = settings;
        this.layers = [];

        // Ensure geometry has position attribute
        if (!geometry.attributes.position) {
            console.error('Geometry missing position attribute');
            return [];
        }

        // Get bounding box
        geometry.computeBoundingBox();
        const bbox = geometry.boundingBox;
        
        // Apply transform to get actual build space bounds
        let minZ = bbox.min.z + (transform.position.z || 0);
        let maxZ = bbox.max.z + (transform.position.z || 0);
        
        console.log(`Object bounds BEFORE limit: minZ=${minZ.toFixed(2)}, maxZ=${maxZ.toFixed(2)}`);
        console.log(`Settings maxZHeight: ${settings.maxZHeight}`);
        
        // Apply max Z height limit if specified (absolute height from build plate)
        // ALWAYS use the user's setting if provided - don't only limit when object is taller
        if (settings.maxZHeight !== undefined && settings.maxZHeight > 0) {
            console.log(`FORCING: maxZ to user setting ${settings.maxZHeight}`);
            maxZ = settings.maxZHeight;
            
            // Also ensure we don't start above the limit
            if (minZ > settings.maxZHeight) {
                console.warn(`Object starts above max Z height limit!`);
                return [];
            }
        }
        
        const layerHeight = settings.layerHeight || 0.2;
        const numLayers = Math.ceil((maxZ - minZ) / layerHeight);

        console.log(`FINAL: Slicing ${numLayers} layers from Z=${minZ.toFixed(2)} to Z=${maxZ.toFixed(2)}, layer height=${layerHeight}mm`);

        // Slice each layer
        for (let i = 0; i < numLayers; i++) {
            const z = minZ + (i * layerHeight);
            // Only slice if within bounds
            if (z <= maxZ) {
                const layer = this.sliceLayer(geometry, z, transform, i);
                if (layer && layer.segments.length > 0) {
                    this.layers.push(layer);
                }
            }
        }

        console.log(`Sliced ${this.layers.length} non-empty layers`);
        return this.layers;
    }

    /**
     * Slice geometry at a specific Z height
     * @param {THREE.BufferGeometry} geometry
     * @param {Number} z - Z height to slice at
     * @param {Object} transform - Transforms to apply
     * @param {Number} layerIndex - Layer number
     * @returns {Object} Layer object with segments
     */
    sliceLayer(geometry, z, transform, layerIndex) {
        const positions = geometry.attributes.position.array;
        const segments = [];
        
        // Apply transform matrix
        const matrix = this.getTransformMatrix(transform);
        
        // Iterate through triangles
        for (let i = 0; i < positions.length; i += 9) {
            // Get triangle vertices
            const v1 = this.applyMatrix(new THREE.Vector3(positions[i], positions[i+1], positions[i+2]), matrix);
            const v2 = this.applyMatrix(new THREE.Vector3(positions[i+3], positions[i+4], positions[i+5]), matrix);
            const v3 = this.applyMatrix(new THREE.Vector3(positions[i+6], positions[i+7], positions[i+8]), matrix);
            
            // Find intersections with Z plane
            const intersection = this.trianglePlaneIntersection(v1, v2, v3, z);
            if (intersection) {
                segments.push(intersection);
            }
        }

        return {
            index: layerIndex,
            z: z,
            segments: segments,
            perimeters: [],
            infill: []
        };
    }

    /**
     * Get transform matrix from position, rotation, scale
     */
    getTransformMatrix(transform) {
        const matrix = new THREE.Matrix4();
        
        // Position
        matrix.makeTranslation(
            transform.position.x || 0,
            transform.position.y || 0,
            transform.position.z || 0
        );
        
        // Rotation
        const rotMatrix = new THREE.Matrix4();
        rotMatrix.makeRotationFromEuler(new THREE.Euler(
            (transform.rotation.x || 0) * Math.PI / 180,
            (transform.rotation.y || 0) * Math.PI / 180,
            (transform.rotation.z || 0) * Math.PI / 180
        ));
        matrix.multiply(rotMatrix);
        
        // Scale
        const scale = transform.scale || 1;
        const scaleMatrix = new THREE.Matrix4();
        scaleMatrix.makeScale(scale, scale, scale);
        matrix.multiply(scaleMatrix);
        
        return matrix;
    }

    /**
     * Apply transformation matrix to vector
     */
    applyMatrix(vector, matrix) {
        return vector.clone().applyMatrix4(matrix);
    }

    /**
     * Find intersection of triangle with horizontal plane at Z
     * Returns line segment if triangle intersects plane
     */
    trianglePlaneIntersection(v1, v2, v3, z) {
        const epsilon = 0.0001;
        
        // Check which vertices are above/below plane
        const above = [];
        const below = [];
        const vertices = [v1, v2, v3];
        
        vertices.forEach(v => {
            if (Math.abs(v.z - z) < epsilon) {
                // Vertex on plane
                above.push(v);
                below.push(v);
            } else if (v.z > z) {
                above.push(v);
            } else {
                below.push(v);
            }
        });

        // Triangle doesn't intersect plane
        if (above.length === 0 || below.length === 0) {
            return null;
        }

        // Find edge intersections
        const intersections = [];
        const edges = [[v1, v2], [v2, v3], [v3, v1]];
        
        edges.forEach(edge => {
            const [p1, p2] = edge;
            
            // Skip if both points on same side
            if ((p1.z > z && p2.z > z) || (p1.z < z && p2.z < z)) {
                return;
            }
            
            // Calculate intersection point
            if (Math.abs(p1.z - p2.z) > epsilon) {
                const t = (z - p1.z) / (p2.z - p1.z);
                const x = p1.x + t * (p2.x - p1.x);
                const y = p1.y + t * (p2.y - p1.y);
                intersections.push({ x, y });
            } else if (Math.abs(p1.z - z) < epsilon) {
                intersections.push({ x: p1.x, y: p1.y });
            }
        });

        // Remove duplicate points
        const unique = [];
        intersections.forEach(p => {
            const isDuplicate = unique.some(u => 
                Math.abs(u.x - p.x) < epsilon && Math.abs(u.y - p.y) < epsilon
            );
            if (!isDuplicate) {
                unique.push(p);
            }
        });

        // Need exactly 2 points for line segment
        if (unique.length === 2) {
            return {
                start: unique[0],
                end: unique[1]
            };
        }

        return null;
    }

    /**
     * Connect segments into closed contours
     */
    connectSegments(segments) {
        if (segments.length === 0) return [];
        
        const contours = [];
        const used = new Array(segments.length).fill(false);
        const epsilon = 0.1; // Connection tolerance in mm
        
        for (let i = 0; i < segments.length; i++) {
            if (used[i]) continue;
            
            const contour = [segments[i].start, segments[i].end];
            used[i] = true;
            
            // Try to build a closed contour
            let foundConnection = true;
            while (foundConnection && contour.length < 10000) { // Safety limit
                foundConnection = false;
                const lastPoint = contour[contour.length - 1];
                
                // Find segment that connects to last point
                for (let j = 0; j < segments.length; j++) {
                    if (used[j]) continue;
                    
                    const seg = segments[j];
                    const distToStart = this.distance2D(lastPoint, seg.start);
                    const distToEnd = this.distance2D(lastPoint, seg.end);
                    
                    if (distToStart < epsilon) {
                        contour.push(seg.end);
                        used[j] = true;
                        foundConnection = true;
                        break;
                    } else if (distToEnd < epsilon) {
                        contour.push(seg.start);
                        used[j] = true;
                        foundConnection = true;
                        break;
                    }
                }
                
                // Check if contour is closed
                if (this.distance2D(contour[0], contour[contour.length - 1]) < epsilon) {
                    break;
                }
            }
            
            if (contour.length > 2) {
                contours.push(contour);
            }
        }
        
        return contours;
    }

    /**
     * Calculate 2D distance between points
     */
    distance2D(p1, p2) {
        const dx = p1.x - p2.x;
        const dy = p1.y - p2.y;
        return Math.sqrt(dx * dx + dy * dy);
    }
}

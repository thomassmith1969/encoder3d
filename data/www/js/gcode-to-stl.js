/**
 * GCode to STL Converter
 * Converts GCode extrusion paths into a 3D mesh (STL-compatible geometry)
 */

class GCodeToSTL {
    constructor() {
        this.vertices = [];
        this.normals = [];
        this.triangles = [];
    }

    /**
     * Parse GCode and generate 3D geometry (async version with progress)
     * @param {String} gcode - GCode text
     * @param {Object} settings - Conversion settings
     * @returns {Promise<THREE.BufferGeometry>} Generated mesh
     */
    async parseAsync(gcode, settings = {}) {
        // Reset arrays for new conversion
        this.triangles = [];
        this.vertices = [];
        this.normals = [];
        
        const s = {
            nozzleDiameter: settings.nozzleDiameter || 0.4,
            layerHeight: settings.layerHeight || 0.2,
            extrusionWidth: settings.extrusionWidth || 0.4,
            resolution: settings.resolution || 8,
            minExtrusionLength: settings.minExtrusionLength || 0.1,
            maxSegments: settings.maxSegments || Infinity // No limit by default
        };

        const progressCallback = settings.progressCallback || (() => {});

        console.log('Converting GCode to mesh...');
        progressCallback(0, 'Parsing GCode');
        
        const lines = gcode.split('\n');
        let x = 0, y = 0, z = 0;
        let lastX = 0, lastY = 0, lastZ = 0;
        let e = 0, lastE = 0;
        let layerPaths = [];
        let currentPath = [];
        let segmentCount = 0;
        let skippedSegments = 0;

        // Parse GCode and extract extrusion paths
        for (let i = 0; i < lines.length; i++) {
            if (i % 1000 === 0) {
                progressCallback((i / lines.length) * 50, 'Parsing GCode');
                // Yield to browser every 5000 lines
                if (i % 5000 === 0 && i > 0) {
                    await new Promise(resolve => setTimeout(resolve, 1));
                }
            }

            const line = lines[i].trim().split(';')[0];
            if (!line.startsWith('G')) continue;

            const parts = line.split(' ');
            const cmd = parts[0];

            if (cmd === 'G0' || cmd === 'G1') {
                let hasMove = false;
                let hasE = false;

                for (let part of parts.slice(1)) {
                    if (part.startsWith('X')) { x = parseFloat(part.substring(1)); hasMove = true; }
                    if (part.startsWith('Y')) { y = parseFloat(part.substring(1)); hasMove = true; }
                    if (part.startsWith('Z')) { z = parseFloat(part.substring(1)); hasMove = true; }
                    if (part.startsWith('E')) { e = parseFloat(part.substring(1)); hasE = true; }
                }

                if (hasMove && hasE && e > lastE) {
                    const distance = Math.sqrt(
                        Math.pow(x - lastX, 2) + 
                        Math.pow(y - lastY, 2) + 
                        Math.pow(z - lastZ, 2)
                    );

                    if (distance > s.minExtrusionLength) {
                        if (segmentCount < s.maxSegments) {
                            currentPath.push({
                                x1: lastX, y1: lastY, z1: lastZ,
                                x2: x, y2: y, z2: z,
                                e: e - lastE,
                                distance: distance
                            });
                            segmentCount++;
                        } else {
                            skippedSegments++;
                        }
                    }
                } else if (hasMove && currentPath.length > 0) {
                    layerPaths.push([...currentPath]);
                    currentPath = [];
                }

                lastX = x;
                lastY = y;
                lastZ = z;
                lastE = e;
            } else if (cmd === 'G92') {
                for (let part of parts.slice(1)) {
                    if (part.startsWith('E')) {
                        lastE = parseFloat(part.substring(1));
                        e = lastE;
                    }
                }
            }
        }

        if (currentPath.length > 0) {
            layerPaths.push(currentPath);
        }

        console.log(`GCode Parsing Complete:`);
        console.log(`  - Total paths: ${layerPaths.length}`);
        console.log(`  - Total segments: ${segmentCount}`);
        console.log(`  - Skipped segments: ${skippedSegments}`);
        
        if (skippedSegments > 0) {
            const msg = `Warning: Skipped ${skippedSegments} segments due to complexity limit`;
            console.warn(msg);
            if (typeof appendLog !== 'undefined') {
                appendLog(msg);
            }
        }

        progressCallback(55, 'Generating mesh');

        // Generate mesh from paths
        for (let pathIndex = 0; pathIndex < layerPaths.length; pathIndex++) {
            if (pathIndex % 100 === 0) {
                progressCallback(55 + (pathIndex / layerPaths.length) * 40, `Generating mesh ${pathIndex}/${layerPaths.length}`);
                // Yield every 500 paths
                if (pathIndex % 500 === 0 && pathIndex > 0) {
                    await new Promise(resolve => setTimeout(resolve, 1));
                }
            }
            this.generatePathMesh(layerPaths[pathIndex], s);
        }

        console.log(`Generated ${this.triangles.length} triangles from ${layerPaths.length} paths`);
        
        progressCallback(95, 'Creating geometry');
        await new Promise(resolve => setTimeout(resolve, 1));

        // Create Three.js geometry
        const geometry = new THREE.BufferGeometry();
        
        const positions = new Float32Array(this.triangles.length * 9);
        const normalsArray = new Float32Array(this.triangles.length * 9);

        this.triangles.forEach((tri, i) => {
            const idx = i * 9;
            positions[idx + 0] = tri.v1.x;
            positions[idx + 1] = tri.v1.y;
            positions[idx + 2] = tri.v1.z;
            positions[idx + 3] = tri.v2.x;
            positions[idx + 4] = tri.v2.y;
            positions[idx + 5] = tri.v2.z;
            positions[idx + 6] = tri.v3.x;
            positions[idx + 7] = tri.v3.y;
            positions[idx + 8] = tri.v3.z;

            normalsArray[idx + 0] = tri.n.x;
            normalsArray[idx + 1] = tri.n.y;
            normalsArray[idx + 2] = tri.n.z;
            normalsArray[idx + 3] = tri.n.x;
            normalsArray[idx + 4] = tri.n.y;
            normalsArray[idx + 5] = tri.n.z;
            normalsArray[idx + 6] = tri.n.x;
            normalsArray[idx + 7] = tri.n.y;
            normalsArray[idx + 8] = tri.n.z;
        });

        geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
        geometry.setAttribute('normal', new THREE.BufferAttribute(normalsArray, 3));
        geometry.computeBoundingBox();

        console.log(`Generated mesh with ${this.triangles.length} triangles`);
        progressCallback(100, 'Complete');
        
        return geometry;
    }

    /**
     * Parse GCode and generate 3D geometry (synchronous version - kept for compatibility)
     * @param {String} gcode - GCode text
     * @param {Object} settings - Conversion settings
     * @returns {THREE.BufferGeometry} Generated mesh
     */
    parse(gcode, settings = {}) {
        // Reset arrays for new conversion
        this.triangles = [];
        this.vertices = [];
        this.normals = [];
        
        const s = {
            nozzleDiameter: settings.nozzleDiameter || 0.4,
            layerHeight: settings.layerHeight || 0.2,
            extrusionWidth: settings.extrusionWidth || 0.4,
            resolution: settings.resolution || 8, // Segments around extrusion path
            minExtrusionLength: settings.minExtrusionLength || 0.1
        };

        console.log('Converting GCode to mesh...');
        
        const lines = gcode.split('\n');
        let x = 0, y = 0, z = 0;
        let lastX = 0, lastY = 0, lastZ = 0;
        let e = 0, lastE = 0;
        let extruding = false;
        let layerPaths = []; // Store all extrusion paths
        let currentPath = [];

        // Parse GCode and extract extrusion paths
        for (let line of lines) {
            line = line.trim().split(';')[0];
            if (!line.startsWith('G')) continue;

            const parts = line.split(' ');
            const cmd = parts[0];

            if (cmd === 'G0' || cmd === 'G1') {
                let hasMove = false;
                let hasE = false;

                for (let part of parts.slice(1)) {
                    if (part.startsWith('X')) { x = parseFloat(part.substring(1)); hasMove = true; }
                    if (part.startsWith('Y')) { y = parseFloat(part.substring(1)); hasMove = true; }
                    if (part.startsWith('Z')) { z = parseFloat(part.substring(1)); hasMove = true; }
                    if (part.startsWith('E')) { e = parseFloat(part.substring(1)); hasE = true; }
                }

                if (hasMove && hasE && e > lastE) {
                    // Extruding move
                    const distance = Math.sqrt(
                        Math.pow(x - lastX, 2) + 
                        Math.pow(y - lastY, 2) + 
                        Math.pow(z - lastZ, 2)
                    );

                    if (distance > s.minExtrusionLength) {
                        currentPath.push({
                            x1: lastX, y1: lastY, z1: lastZ,
                            x2: x, y2: y, z2: z,
                            e: e - lastE,
                            distance: distance
                        });
                    }
                } else if (hasMove && currentPath.length > 0) {
                    // End of extrusion path
                    layerPaths.push([...currentPath]);
                    currentPath = [];
                }

                lastX = x;
                lastY = y;
                lastZ = z;
                lastE = e;
            } else if (cmd === 'G92') {
                // Reset extruder
                for (let part of parts.slice(1)) {
                    if (part.startsWith('E')) {
                        lastE = parseFloat(part.substring(1));
                        e = lastE;
                    }
                }
            }
        }

        if (currentPath.length > 0) {
            layerPaths.push(currentPath);
        }

        console.log(`Found ${layerPaths.length} extrusion paths`);

        // Generate mesh from paths
        layerPaths.forEach((path, pathIndex) => {
            if (pathIndex % 100 === 0) {
                console.log(`Processing path ${pathIndex}/${layerPaths.length}`);
            }
            this.generatePathMesh(path, s);
        });

        // Create Three.js geometry
        const geometry = new THREE.BufferGeometry();
        
        const positions = new Float32Array(this.triangles.length * 9); // 3 vertices * 3 coords
        const normalsArray = new Float32Array(this.triangles.length * 9);

        this.triangles.forEach((tri, i) => {
            const idx = i * 9;
            // Vertex 1
            positions[idx + 0] = tri.v1.x;
            positions[idx + 1] = tri.v1.y;
            positions[idx + 2] = tri.v1.z;
            // Vertex 2
            positions[idx + 3] = tri.v2.x;
            positions[idx + 4] = tri.v2.y;
            positions[idx + 5] = tri.v2.z;
            // Vertex 3
            positions[idx + 6] = tri.v3.x;
            positions[idx + 7] = tri.v3.y;
            positions[idx + 8] = tri.v3.z;

            // Normals
            normalsArray[idx + 0] = tri.n.x;
            normalsArray[idx + 1] = tri.n.y;
            normalsArray[idx + 2] = tri.n.z;
            normalsArray[idx + 3] = tri.n.x;
            normalsArray[idx + 4] = tri.n.y;
            normalsArray[idx + 5] = tri.n.z;
            normalsArray[idx + 6] = tri.n.x;
            normalsArray[idx + 7] = tri.n.y;
            normalsArray[idx + 8] = tri.n.z;
        });

        geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
        geometry.setAttribute('normal', new THREE.BufferAttribute(normalsArray, 3));
        geometry.computeBoundingBox();

        console.log(`Generated mesh with ${this.triangles.length} triangles`);
        
        return geometry;
    }

    /**
     * Generate mesh for a single extrusion path
     */
    generatePathMesh(path, settings) {
        const width = settings.extrusionWidth;
        const height = settings.layerHeight;

        path.forEach((segment, segIndex) => {
            // Convert GCode coordinates (Z-up) to Three.js coordinates (Y-up)
            // GCode: X=X, Y=Y, Z=height
            // Three.js: X=X, Y=height, Z=Y
            const seg = {
                x1: segment.x1,
                y1: segment.z1,  // GCode Z becomes Three.js Y (height)
                z1: segment.y1,  // GCode Y becomes Three.js Z (depth)
                x2: segment.x2,
                y2: segment.z2,  // GCode Z becomes Three.js Y (height)
                z2: segment.y2   // GCode Y becomes Three.js Z (depth)
            };

            // Create rectangular cross-section for the extrusion
            const dx = seg.x2 - seg.x1;
            const dz = seg.z2 - seg.z1;  // Using Z now (was Y in GCode)
            const length = Math.sqrt(dx * dx + dz * dz);
            
            if (length < 0.001) return;

            // Direction vector in XZ plane
            const dirX = dx / length;
            const dirZ = dz / length;

            // Perpendicular vector (for width) in XZ plane
            const perpX = -dirZ;
            const perpZ = dirX;

            // Half width
            const hw = width / 2;

            // Four corners of the extrusion segment
            const corners = [
                { // Bottom-left
                    x1: seg.x1 + perpX * hw,
                    y1: seg.y1,
                    z1: seg.z1 + perpZ * hw,
                    x2: seg.x2 + perpX * hw,
                    y2: seg.y2,
                    z2: seg.z2 + perpZ * hw
                },
                { // Bottom-right
                    x1: seg.x1 - perpX * hw,
                    y1: seg.y1,
                    z1: seg.z1 - perpZ * hw,
                    x2: seg.x2 - perpX * hw,
                    y2: seg.y2,
                    z2: seg.z2 - perpZ * hw
                },
                { // Top-right
                    x1: seg.x1 - perpX * hw,
                    y1: seg.y1 + height,
                    z1: seg.z1 - perpZ * hw,
                    x2: seg.x2 - perpX * hw,
                    y2: seg.y2 + height,
                    z2: seg.z2 - perpZ * hw
                },
                { // Top-left
                    x1: seg.x1 + perpX * hw,
                    y1: seg.y1 + height,
                    z1: seg.z1 + perpZ * hw,
                    x2: seg.x2 + perpX * hw,
                    y2: seg.y2 + height,
                    z2: seg.z2 + perpZ * hw
                }
            ];

            // Create box mesh for this segment
            // Front face (start)
            this.addQuad(
                { x: corners[0].x1, y: corners[0].y1, z: corners[0].z1 },
                { x: corners[1].x1, y: corners[1].y1, z: corners[1].z1 },
                { x: corners[2].x1, y: corners[2].y1, z: corners[2].z1 },
                { x: corners[3].x1, y: corners[3].y1, z: corners[3].z1 }
            );

            // Back face (end)
            this.addQuad(
                { x: corners[1].x2, y: corners[1].y2, z: corners[1].z2 },
                { x: corners[0].x2, y: corners[0].y2, z: corners[0].z2 },
                { x: corners[3].x2, y: corners[3].y2, z: corners[3].z2 },
                { x: corners[2].x2, y: corners[2].y2, z: corners[2].z2 }
            );

            // Bottom face
            this.addQuad(
                { x: corners[0].x1, y: corners[0].y1, z: corners[0].z1 },
                { x: corners[0].x2, y: corners[0].y2, z: corners[0].z2 },
                { x: corners[1].x2, y: corners[1].y2, z: corners[1].z2 },
                { x: corners[1].x1, y: corners[1].y1, z: corners[1].z1 }
            );

            // Top face
            this.addQuad(
                { x: corners[3].x1, y: corners[3].y1, z: corners[3].z1 },
                { x: corners[2].x1, y: corners[2].y1, z: corners[2].z1 },
                { x: corners[2].x2, y: corners[2].y2, z: corners[2].z2 },
                { x: corners[3].x2, y: corners[3].y2, z: corners[3].z2 }
            );

            // Left face
            this.addQuad(
                { x: corners[0].x1, y: corners[0].y1, z: corners[0].z1 },
                { x: corners[3].x1, y: corners[3].y1, z: corners[3].z1 },
                { x: corners[3].x2, y: corners[3].y2, z: corners[3].z2 },
                { x: corners[0].x2, y: corners[0].y2, z: corners[0].z2 }
            );

            // Right face
            this.addQuad(
                { x: corners[1].x1, y: corners[1].y1, z: corners[1].z1 },
                { x: corners[1].x2, y: corners[1].y2, z: corners[1].z2 },
                { x: corners[2].x2, y: corners[2].y2, z: corners[2].z2 },
                { x: corners[2].x1, y: corners[2].y1, z: corners[2].z1 }
            );
        });
    }

    /**
     * Add a quad (2 triangles) to the mesh
     */
    addQuad(v1, v2, v3, v4) {
        // Calculate normal
        const u = { 
            x: v2.x - v1.x, 
            y: v2.y - v1.y, 
            z: v2.z - v1.z 
        };
        const v = { 
            x: v3.x - v1.x, 
            y: v3.y - v1.y, 
            z: v3.z - v1.z 
        };
        
        const normal = {
            x: u.y * v.z - u.z * v.y,
            y: u.z * v.x - u.x * v.z,
            z: u.x * v.y - u.y * v.x
        };
        
        const len = Math.sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (len > 0) {
            normal.x /= len;
            normal.y /= len;
            normal.z /= len;
        }

        // Triangle 1: v1, v2, v3
        this.triangles.push({
            v1: v1,
            v2: v2,
            v3: v3,
            n: normal
        });

        // Triangle 2: v1, v3, v4
        this.triangles.push({
            v1: v1,
            v2: v3,
            v3: v4,
            n: normal
        });
    }

    /**
     * Export geometry to STL format (binary)
     */
    exportSTL(geometry) {
        const triangles = [];
        const positions = geometry.attributes.position.array;
        
        for (let i = 0; i < positions.length; i += 9) {
            const v1 = new THREE.Vector3(positions[i], positions[i+1], positions[i+2]);
            const v2 = new THREE.Vector3(positions[i+3], positions[i+4], positions[i+5]);
            const v3 = new THREE.Vector3(positions[i+6], positions[i+7], positions[i+8]);
            
            // Calculate normal
            const u = new THREE.Vector3().subVectors(v2, v1);
            const v = new THREE.Vector3().subVectors(v3, v1);
            const normal = new THREE.Vector3().crossVectors(u, v).normalize();
            
            triangles.push({
                normal: normal,
                v1: v1,
                v2: v2,
                v3: v3
            });
        }

        // Binary STL format
        const headerSize = 80;
        const triangleSize = 50; // 12 floats * 4 bytes + 2 bytes attribute
        const bufferSize = headerSize + 4 + (triangles.length * triangleSize);
        const buffer = new ArrayBuffer(bufferSize);
        const view = new DataView(buffer);
        
        // Header (80 bytes)
        const header = 'GCode to STL - Encoder3D';
        for (let i = 0; i < Math.min(header.length, 80); i++) {
            view.setUint8(i, header.charCodeAt(i));
        }
        
        // Number of triangles
        view.setUint32(80, triangles.length, true);
        
        // Triangle data
        let offset = 84;
        triangles.forEach(tri => {
            // Normal
            view.setFloat32(offset, tri.normal.x, true); offset += 4;
            view.setFloat32(offset, tri.normal.y, true); offset += 4;
            view.setFloat32(offset, tri.normal.z, true); offset += 4;
            // Vertex 1
            view.setFloat32(offset, tri.v1.x, true); offset += 4;
            view.setFloat32(offset, tri.v1.y, true); offset += 4;
            view.setFloat32(offset, tri.v1.z, true); offset += 4;
            // Vertex 2
            view.setFloat32(offset, tri.v2.x, true); offset += 4;
            view.setFloat32(offset, tri.v2.y, true); offset += 4;
            view.setFloat32(offset, tri.v2.z, true); offset += 4;
            // Vertex 3
            view.setFloat32(offset, tri.v3.x, true); offset += 4;
            view.setFloat32(offset, tri.v3.y, true); offset += 4;
            view.setFloat32(offset, tri.v3.z, true); offset += 4;
            // Attribute byte count
            view.setUint16(offset, 0, true); offset += 2;
        });
        
        return buffer;
    }

    /**
     * Export geometry to STL ASCII format
     */
    exportSTLAscii(geometry) {
        let stl = 'solid GCodeToSTL\n';
        
        const positions = geometry.attributes.position.array;
        
        for (let i = 0; i < positions.length; i += 9) {
            const v1 = new THREE.Vector3(positions[i], positions[i+1], positions[i+2]);
            const v2 = new THREE.Vector3(positions[i+3], positions[i+4], positions[i+5]);
            const v3 = new THREE.Vector3(positions[i+6], positions[i+7], positions[i+8]);
            
            // Calculate normal
            const u = new THREE.Vector3().subVectors(v2, v1);
            const v = new THREE.Vector3().subVectors(v3, v1);
            const normal = new THREE.Vector3().crossVectors(u, v).normalize();
            
            stl += `  facet normal ${normal.x.toExponential(6)} ${normal.y.toExponential(6)} ${normal.z.toExponential(6)}\n`;
            stl += `    outer loop\n`;
            stl += `      vertex ${v1.x.toExponential(6)} ${v1.y.toExponential(6)} ${v1.z.toExponential(6)}\n`;
            stl += `      vertex ${v2.x.toExponential(6)} ${v2.y.toExponential(6)} ${v2.z.toExponential(6)}\n`;
            stl += `      vertex ${v3.x.toExponential(6)} ${v3.y.toExponential(6)} ${v3.z.toExponential(6)}\n`;
            stl += `    endloop\n`;
            stl += `  endfacet\n`;
        }
        
        stl += 'endsolid GCodeToSTL\n';
        
        return stl;
    }
}

// Global instance
const gcodeToSTL = new GCodeToSTL();

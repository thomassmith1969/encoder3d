/**
 * Async GCode to STL Converter with BIG chunks
 */

class GCodeToSTL {
    constructor() {
        this.vertices = [];
        this.normals = [];
        this.triangles = [];
    }

    /**
     * Parse GCode and generate mesh ASYNC with big chunks
     */
    async parse(gcode, settings = {}) {
        console.log('Starting async conversion with BIG chunks...');
        
        // Reset
        this.triangles = [];
        this.vertices = [];
        this.normals = [];
        
        const nozzleDiameter = settings.nozzleDiameter || 0.4;
        const layerHeight = settings.layerHeight || 0.2;
        const extrusionWidth = settings.extrusionWidth || nozzleDiameter;
        const minLength = settings.minExtrusionLength || 0.1;
        const progressCallback = settings.progressCallback;
        
        const lines = gcode.split('\n');
        let x = 0, y = 0, z = 0, e = 0;
        let lastX = 0, lastY = 0, lastZ = 0, lastE = 0;
        
        const paths = [];
        let currentPath = null;
        let totalSegments = 0;
        
        // Parse in HUGE chunks - only yield every 50k lines
        const parseChunkSize = 50000;
        
        for (let start = 0; start < lines.length; start += parseChunkSize) {
            const end = Math.min(start + parseChunkSize, lines.length);
            
            for (let i = start; i < end; i++) {
                const line = lines[i].trim().split(';')[0];
                if (!line) continue;
                
                const parts = line.split(/\s+/);
                const cmd = parts[0];
                
                if (cmd === 'G92') {
                    for (let j = 1; j < parts.length; j++) {
                        if (parts[j].startsWith('E')) {
                            e = parseFloat(parts[j].substring(1)) || 0;
                            lastE = e;
                        }
                    }
                } else if (cmd === 'G0' || cmd === 'G1') {
                    let newX = x, newY = y, newZ = z, newE = e;
                    let hasE = false;
                    
                    for (let j = 1; j < parts.length; j++) {
                        const p = parts[j];
                        if (p.startsWith('X')) newX = parseFloat(p.substring(1));
                        else if (p.startsWith('Y')) newY = parseFloat(p.substring(1));
                        else if (p.startsWith('Z')) newZ = parseFloat(p.substring(1));
                        else if (p.startsWith('E')) {
                            newE = parseFloat(p.substring(1));
                            hasE = true;
                        }
                    }
                    
                    if (hasE && newE > lastE) {
                        const dist = Math.sqrt(
                            Math.pow(newX - lastX, 2) + 
                            Math.pow(newY - lastY, 2) + 
                            Math.pow(newZ - lastZ, 2)
                        );
                        
                        if (dist >= minLength) {
                            if (!currentPath || currentPath.z !== newZ) {
                                if (currentPath) paths.push(currentPath);
                                currentPath = { z: newZ, segments: [] };
                            }
                            currentPath.segments.push({
                                x1: lastX, y1: lastY, z1: lastZ,
                                x2: newX, y2: newY, z2: newZ
                            });
                            totalSegments++;
                        }
                    }
                    
                    lastX = x = newX;
                    lastY = y = newY;
                    lastZ = z = newZ;
                    lastE = e = newE;
                }
            }
            
            // Yield to browser every 50k lines
            if (progressCallback) {
                progressCallback((end / lines.length) * 50, `Parsing ${end}/${lines.length}`);
            }
            await new Promise(resolve => setTimeout(resolve, 1));
        }
        
        if (currentPath) paths.push(currentPath);
        
        console.log(`Parsed: ${paths.length} paths, ${totalSegments} segments`);
        
        // Generate mesh in BIG chunks - only yield every 5000 paths
        const meshChunkSize = 5000;
        let processedPaths = 0;
        
        for (let pathIdx = 0; pathIdx < paths.length; pathIdx += meshChunkSize) {
            const end = Math.min(pathIdx + meshChunkSize, paths.length);
            
            for (let i = pathIdx; i < end; i++) {
                const path = paths[i];
                for (const seg of path.segments) {
                    this.addSegmentBox(seg, extrusionWidth, layerHeight);
                }
                processedPaths++;
            }
            
            // Yield every 5000 paths
            if (progressCallback) {
                progressCallback(50 + (processedPaths / paths.length) * 50, `Mesh ${processedPaths}/${paths.length}`);
            }
            await new Promise(resolve => setTimeout(resolve, 1));
        }
        
        console.log(`Generated ${this.triangles.length} triangles from ${paths.length} paths`);
        
        // Create geometry
        const geometry = new THREE.BufferGeometry();
        const positions = new Float32Array(this.triangles.length * 9);
        const normals = new Float32Array(this.triangles.length * 9);
        
        for (let i = 0; i < this.triangles.length; i++) {
            const tri = this.triangles[i];
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
            
            normals[idx + 0] = tri.n.x;
            normals[idx + 1] = tri.n.y;
            normals[idx + 2] = tri.n.z;
            normals[idx + 3] = tri.n.x;
            normals[idx + 4] = tri.n.y;
            normals[idx + 5] = tri.n.z;
            normals[idx + 6] = tri.n.x;
            normals[idx + 7] = tri.n.y;
            normals[idx + 8] = tri.n.z;
        }
        
        geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
        geometry.setAttribute('normal', new THREE.BufferAttribute(normals, 3));
        
        if (progressCallback) progressCallback(100, 'Complete');
        
        return geometry;
    }

    /**
     * Add a box for an extrusion segment
     */
    addSegmentBox(seg, width, height) {
        // GCode coordinates are X,Y,Z with Z up. Map directly to Three.js coordinates with Z up.
        // No axis swap or 180° rotation here; keep slicing coordinates consistent with model axes.
        const x1 = seg.x1;
        const y1 = seg.y1;
        const z1 = seg.z1;
        const x2 = seg.x2;
        const y2 = seg.y2;
        const z2 = seg.z2;
        
        const hw = width / 2;
        const hh = height / 2;
        
        // 8 corners of the box
        const corners = [
            { x: x1 - hw, y: y1 - hh, z: z1 - hw },
            { x: x1 + hw, y: y1 - hh, z: z1 - hw },
            { x: x1 + hw, y: y1 + hh, z: z1 - hw },
            { x: x1 - hw, y: y1 + hh, z: z1 - hw },
            { x: x2 - hw, y: y2 - hh, z: z2 - hw },
            { x: x2 + hw, y: y2 - hh, z: z2 - hw },
            { x: x2 + hw, y: y2 + hh, z: z2 - hw },
            { x: x2 - hw, y: y2 + hh, z: z2 - hw }
        ];
        
        // 6 faces (12 triangles)
        this.addQuad(corners[0], corners[1], corners[2], corners[3]); // Back
        this.addQuad(corners[5], corners[4], corners[7], corners[6]); // Front
        this.addQuad(corners[4], corners[0], corners[3], corners[7]); // Left
        this.addQuad(corners[1], corners[5], corners[6], corners[2]); // Right
        this.addQuad(corners[3], corners[2], corners[6], corners[7]); // Top
        this.addQuad(corners[4], corners[5], corners[1], corners[0]); // Bottom
    }

    /**
     * Add a quad as 2 triangles
     */
    addQuad(v1, v2, v3, v4) {
        const normal = this.computeNormal(v1, v2, v3);
        this.triangles.push({ v1, v2, v3, n: normal });
        this.triangles.push({ v1: v1, v2: v3, v3: v4, n: normal });
    }

    /**
     * Compute face normal
     */
    computeNormal(v1, v2, v3) {
        const u = { x: v2.x - v1.x, y: v2.y - v1.y, z: v2.z - v1.z };
        const v = { x: v3.x - v1.x, y: v3.y - v1.y, z: v3.z - v1.z };
        
        const nx = u.y * v.z - u.z * v.y;
        const ny = u.z * v.x - u.x * v.z;
        const nz = u.x * v.y - u.y * v.x;
        
        const len = Math.sqrt(nx * nx + ny * ny + nz * nz);
        return len > 0 ? { x: nx / len, y: ny / len, z: nz / len } : { x: 0, y: 1, z: 0 };
    }

    /**
     * Export to binary STL
     */
    exportSTL(geometry) {
        const triangleCount = geometry.attributes.position.count / 3;
        const buffer = new ArrayBuffer(84 + triangleCount * 50);
        const view = new DataView(buffer);
        
        // Header (80 bytes)
        for (let i = 0; i < 80; i++) view.setUint8(i, 0);
        
        // Triangle count
        view.setUint32(80, triangleCount, true);
        
        const positions = geometry.attributes.position.array;
        const normals = geometry.attributes.normal.array;
        
        let offset = 84;
        for (let i = 0; i < triangleCount; i++) {
            const idx = i * 9;
            
            // Normal
            view.setFloat32(offset, normals[idx], true);
            view.setFloat32(offset + 4, normals[idx + 1], true);
            view.setFloat32(offset + 8, normals[idx + 2], true);
            offset += 12;
            
            // Vertices
            for (let j = 0; j < 3; j++) {
                view.setFloat32(offset, positions[idx + j * 3], true);
                view.setFloat32(offset + 4, positions[idx + j * 3 + 1], true);
                view.setFloat32(offset + 8, positions[idx + j * 3 + 2], true);
                offset += 12;
            }
            
            // Attribute byte count
            view.setUint16(offset, 0, true);
            offset += 2;
        }
        
        return buffer;
    }
}

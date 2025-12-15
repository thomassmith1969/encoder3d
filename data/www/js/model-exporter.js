/**
 * Generic 3D Model Exporter
 * Exports Three.js geometries to various formats with async processing
 */

class ModelExporter {
    /**
     * Export geometry to STL format (binary) - async with progress
     */
    async exportSTL(geometry, filename = 'model.stl', progressCallback = null) {
        const triangleCount = geometry.attributes.position.count / 3;
        const buffer = new ArrayBuffer(84 + triangleCount * 50);
        const view = new DataView(buffer);
        
        // Header (80 bytes)
        const headerText = `Exported from Encoder3D`;
        for (let i = 0; i < 80; i++) {
            view.setUint8(i, i < headerText.length ? headerText.charCodeAt(i) : 0);
        }
        
        // Triangle count
        view.setUint32(80, triangleCount, true);
        
        const positions = geometry.attributes.position.array;
        const normals = geometry.attributes.normal ? geometry.attributes.normal.array : null;
        
        let offset = 84;
        const chunkSize = 5000; // Process 5000 triangles at a time
        
        for (let start = 0; start < triangleCount; start += chunkSize) {
            const end = Math.min(start + chunkSize, triangleCount);
            
            for (let i = start; i < end; i++) {
                const idx = i * 9;
                
                // Calculate normal if not provided
                let nx = 0, ny = 0, nz = 0;
                if (normals) {
                    nx = normals[idx];
                    ny = normals[idx + 1];
                    nz = normals[idx + 2];
                } else {
                    // Calculate from vertices
                    const v1x = positions[idx + 0], v1y = positions[idx + 1], v1z = positions[idx + 2];
                    const v2x = positions[idx + 3], v2y = positions[idx + 4], v2z = positions[idx + 5];
                    const v3x = positions[idx + 6], v3y = positions[idx + 7], v3z = positions[idx + 8];
                    
                    const ux = v2x - v1x, uy = v2y - v1y, uz = v2z - v1z;
                    const vx = v3x - v1x, vy = v3y - v1y, vz = v3z - v1z;
                    
                    nx = uy * vz - uz * vy;
                    ny = uz * vx - ux * vz;
                    nz = ux * vy - uy * vx;
                    
                    const len = Math.sqrt(nx * nx + ny * ny + nz * nz);
                    if (len > 0) {
                        nx /= len; ny /= len; nz /= len;
                    }
                }
                
                // Normal
                view.setFloat32(offset, nx, true);
                view.setFloat32(offset + 4, ny, true);
                view.setFloat32(offset + 8, nz, true);
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
            
            // Report progress and yield to browser
            if (progressCallback) {
                progressCallback((end / triangleCount) * 100, `Exporting STL: ${end}/${triangleCount} triangles`);
            }
            await new Promise(resolve => setTimeout(resolve, 1));
        }
        
        return buffer;
    }

    /**
     * Export geometry to OBJ format - async with progress
     */
    async exportOBJ(geometry, filename = 'model.obj', progressCallback = null) {
        let obj = `# Exported from Encoder3D\n`;
        obj += `# File: ${filename}\n`;
        obj += `o Model\n\n`;
        
        const positions = geometry.attributes.position.array;
        const normals = geometry.attributes.normal ? geometry.attributes.normal.array : null;
        
        const chunkSize = 10000;
        
        // Vertices
        for (let start = 0; start < positions.length; start += chunkSize * 3) {
            const end = Math.min(start + chunkSize * 3, positions.length);
            
            for (let i = start; i < end; i += 3) {
                obj += `v ${positions[i].toFixed(6)} ${positions[i + 1].toFixed(6)} ${positions[i + 2].toFixed(6)}\n`;
            }
            
            if (progressCallback) {
                progressCallback((end / positions.length) * 30, `Exporting vertices: ${end / 3}/${positions.length / 3}`);
            }
            await new Promise(resolve => setTimeout(resolve, 1));
        }
        obj += '\n';
        
        // Normals
        if (normals) {
            for (let start = 0; start < normals.length; start += chunkSize * 3) {
                const end = Math.min(start + chunkSize * 3, normals.length);
                
                for (let i = start; i < end; i += 3) {
                    obj += `vn ${normals[i].toFixed(6)} ${normals[i + 1].toFixed(6)} ${normals[i + 2].toFixed(6)}\n`;
                }
                
                if (progressCallback) {
                    progressCallback(30 + (end / normals.length) * 30, `Exporting normals: ${end / 3}/${normals.length / 3}`);
                }
                await new Promise(resolve => setTimeout(resolve, 1));
            }
            obj += '\n';
        }
        
        // Faces
        const vertexCount = positions.length / 3;
        for (let start = 1; start <= vertexCount; start += chunkSize * 3) {
            const end = Math.min(start + chunkSize * 3, vertexCount + 1);
            
            for (let i = start; i < end; i += 3) {
                if (normals) {
                    obj += `f ${i}//${i} ${i + 1}//${i + 1} ${i + 2}//${i + 2}\n`;
                } else {
                    obj += `f ${i} ${i + 1} ${i + 2}\n`;
                }
            }
            
            if (progressCallback) {
                const baseProgress = normals ? 60 : 30;
                progressCallback(baseProgress + (end / vertexCount) * 40, `Exporting faces: ${(end - 1) / 3}/${vertexCount / 3}`);
            }
            await new Promise(resolve => setTimeout(resolve, 1));
        }
        
        return obj;
    }

    /**
     * Export geometry to PLY format - async with progress
     */
    async exportPLY(geometry, filename = 'model.ply', progressCallback = null) {
        const positions = geometry.attributes.position.array;
        const normals = geometry.attributes.normal ? geometry.attributes.normal.array : null;
        const vertexCount = positions.length / 3;
        const faceCount = vertexCount / 3;
        
        let ply = `ply\n`;
        ply += `format ascii 1.0\n`;
        ply += `comment Exported from Encoder3D\n`;
        ply += `element vertex ${vertexCount}\n`;
        ply += `property float x\n`;
        ply += `property float y\n`;
        ply += `property float z\n`;
        if (normals) {
            ply += `property float nx\n`;
            ply += `property float ny\n`;
            ply += `property float nz\n`;
        }
        ply += `element face ${faceCount}\n`;
        ply += `property list uchar int vertex_indices\n`;
        ply += `end_header\n`;
        
        const chunkSize = 10000;
        
        // Vertices
        for (let start = 0; start < positions.length; start += chunkSize * 3) {
            const end = Math.min(start + chunkSize * 3, positions.length);
            
            for (let i = start; i < end; i += 3) {
                ply += `${positions[i].toFixed(6)} ${positions[i + 1].toFixed(6)} ${positions[i + 2].toFixed(6)}`;
                if (normals) {
                    ply += ` ${normals[i].toFixed(6)} ${normals[i + 1].toFixed(6)} ${normals[i + 2].toFixed(6)}`;
                }
                ply += `\n`;
            }
            
            if (progressCallback) {
                progressCallback((end / positions.length) * 50, `Exporting vertices: ${end / 3}/${positions.length / 3}`);
            }
            await new Promise(resolve => setTimeout(resolve, 1));
        }
        
        // Faces
        for (let start = 0; start < vertexCount; start += chunkSize * 3) {
            const end = Math.min(start + chunkSize * 3, vertexCount);
            
            for (let i = start; i < end; i += 3) {
                ply += `3 ${i} ${i + 1} ${i + 2}\n`;
            }
            
            if (progressCallback) {
                progressCallback(50 + (end / vertexCount) * 50, `Exporting faces: ${end / 3}/${vertexCount / 3}`);
            }
            await new Promise(resolve => setTimeout(resolve, 1));
        }
        
        return ply;
    }

    /**
     * Download file to user's computer
     */
    download(data, filename, mimeType = 'application/octet-stream') {
        const blob = new Blob([data], { type: mimeType });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.click();
        URL.revokeObjectURL(url);
    }

    /**
     * Export and download in specified format - async with progress
     */
    async export(geometry, filename, format, progressCallback = null) {
        let data, mimeType;
        const baseName = filename.replace(/\.[^/.]+$/, "");
        
        switch (format.toLowerCase()) {
            case 'stl':
                data = await this.exportSTL(geometry, baseName + '.stl', progressCallback);
                mimeType = 'application/octet-stream';
                filename = baseName + '.stl';
                break;
                
            case 'obj':
                data = await this.exportOBJ(geometry, baseName + '.obj', progressCallback);
                mimeType = 'text/plain';
                filename = baseName + '.obj';
                break;
                
            case 'ply':
                data = await this.exportPLY(geometry, baseName + '.ply', progressCallback);
                mimeType = 'text/plain';
                filename = baseName + '.ply';
                break;
                
            default:
                throw new Error(`Unsupported format: ${format}`);
        }
        
        this.download(data, filename, mimeType);
        return filename;
    }
}

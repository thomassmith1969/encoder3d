/**
 * Web Worker for background slicing
 * Prevents UI freezing during intensive slicing operations
 */

// Import slicer modules
importScripts('../STLLoader.js');
importScripts('slicer-core.js');
importScripts('slicer-paths.js');
importScripts('slicer-gcode.js');
importScripts('slicer-main.js');

// Import Three.js (needed for geometry operations)
importScripts('https://cdn.jsdelivr.net/npm/three@0.128.0/build/three.min.js');

self.addEventListener('message', function(e) {
    const { type, data } = e.data;
    
    if (type === 'slice') {
        try {
            const { geometry, settings } = data;
            
            // Reconstruct Three.js geometry from transferred data
            const geom = new THREE.BufferGeometry();
            geom.setAttribute('position', new THREE.BufferAttribute(geometry.positions, 3));
            if (geometry.normals) {
                geom.setAttribute('normal', new THREE.BufferAttribute(geometry.normals, 3));
            }
            geom.computeBoundingBox();
            
            // Create progress callback
            const progressCallback = (progress, message) => {
                self.postMessage({
                    type: 'progress',
                    progress: progress,
                    message: message
                });
            };
            
            // Perform slicing
            const result = encoder3DSlicer.slice(geom, settings, progressCallback);
            
            // Send result back
            self.postMessage({
                type: 'complete',
                result: result
            });
            
        } catch (error) {
            self.postMessage({
                type: 'error',
                error: error.message
            });
        }
    } else if (type === 'cancel') {
        // Signal cancellation (worker will be terminated from main thread)
        self.postMessage({
            type: 'cancelled'
        });
        self.close();
    }
});

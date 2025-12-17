// Initialize three-bvh-csg library
(function() {
    // Wait for libraries to load
    if (typeof MeshBVHLib === 'undefined' || typeof ThreBvhCsg === 'undefined') {
        console.error('CSG libraries not loaded yet, retrying...');
        setTimeout(arguments.callee, 100);
        return;
    }
    
    // Polyfill Triangle.getInterpolation for older three.js (r128)
    if (typeof THREE.Triangle.getInterpolation !== 'function') {
        const _baryCoord = new THREE.Vector3();
        THREE.Triangle.getInterpolation = function(point, p0, p1, p2, t0, t1, t2, target) {
            THREE.Triangle.getBarycoord(point, p0, p1, p2, _baryCoord);
            if (!target) target = t0.clone();
            target.set(
                t0.x * _baryCoord.x + t1.x * _baryCoord.y + t2.x * _baryCoord.z,
                t0.y * _baryCoord.x + t1.y * _baryCoord.y + t2.y * _baryCoord.z
            );
            return target;
        };
    }

    // Polyfill Triangle.getInterpolation for three r128 (needed by three-mesh-bvh >=0.6.x)
    if (!THREE.Triangle.getInterpolation) {
        const _bary = new THREE.Vector3();
        THREE.Triangle.getInterpolation = function(point, p0, p1, p2, v0, v1, v2, target) {
            THREE.Triangle.getBarycoord(point, p0, p1, p2, _bary);
            const u = _bary.x, v = _bary.y, w = _bary.z;
            if (target.isVector2 || target instanceof THREE.Vector2) {
                target.set(
                    v0.x * u + v1.x * v + v2.x * w,
                    v0.y * u + v1.y * v + v2.y * w
                );
            } else {
                target.set(
                    v0.x * u + v1.x * v + v2.x * w,
                    v0.y * u + v1.y * v + v2.y * w,
                    v0.z * u + v1.z * v + v2.z * w
                );
            }
            return target;
        };
    }

    // Expose CSG API on window for easy access
    const { computeBoundsTree, disposeBoundsTree, acceleratedRaycast } = MeshBVHLib;
    THREE.BufferGeometry.prototype.computeBoundsTree = computeBoundsTree;
    THREE.BufferGeometry.prototype.disposeBoundsTree = disposeBoundsTree;
    THREE.Mesh.prototype.raycast = acceleratedRaycast;

    window.THREEDCSG = {
        Brush: ThreBvhCsg.Brush,
        Evaluator: ThreBvhCsg.Evaluator,
        ADDITION: ThreBvhCsg.ADDITION,
        SUBTRACTION: ThreBvhCsg.SUBTRACTION,
        REVERSE_SUBTRACTION: ThreBvhCsg.REVERSE_SUBTRACTION,
        DIFFERENCE: ThreBvhCsg.DIFFERENCE,
        INTERSECTION: ThreBvhCsg.INTERSECTION,
        HOLLOW_SUBTRACTION: ThreBvhCsg.HOLLOW_SUBTRACTION,
        HOLLOW_INTERSECTION: ThreBvhCsg.HOLLOW_INTERSECTION
    };
    
    console.log('three-bvh-csg initialized successfully');
})();

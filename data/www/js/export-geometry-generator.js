/**
 * Export Geometry Generator - EXACT export logic extraction
 * Generates processed geometry with booleans EXACTLY as exportModel() does
 */

async function generateExportGeometry(progressCallback = null) {
    const additiveObjects = loadedObjects.filter(obj => obj.mode !== 'subtractive');
    const subtractiveObjects = loadedObjects.filter(obj => obj.mode === 'subtractive');
    
    const update = (p, msg) => {
        if (progressCallback) progressCallback(p, msg);
    };
    
    if (additiveObjects.length === 0) {
        throw new Error('No additive objects to process');
    }
    
    update(10, 'Merging additive objects...');
    
    // Merge all additive geometries first (EXACT export logic)
    const mergedPositions = [];
    additiveObjects.forEach((obj, idx) => {
        const geo = obj.geometry.clone();
        geo.applyMatrix4(obj.mesh.matrix);
        const pos = geo.attributes.position.array;
        for (let i = 0; i < pos.length; i++) {
            mergedPositions.push(pos[i]);
        }
        update(10 + (idx / additiveObjects.length) * 20, `Merging ${idx + 1}/${additiveObjects.length}...`);
    });
    
    let resultGeometry = new THREE.BufferGeometry();
    resultGeometry.setAttribute('position', new THREE.Float32BufferAttribute(mergedPositions, 3));
    resultGeometry.computeVertexNormals();
    
    // If no subtractive objects, return merged result
    if (subtractiveObjects.length === 0) {
        update(100, 'Merge complete');
        return resultGeometry;
    }
    
    update(30, 'Applying boolean subtractions...');
    
    // Create mesh for CSG (EXACT export logic)
    let resultMesh = new THREE.Mesh(resultGeometry);
    resultMesh.updateMatrixWorld(true);
    
    // Subtract each subtractive object using three-bvh-csg (EXACT export logic)
    const { Brush, Evaluator, SUBTRACTION } = window.THREEDCSG;
    const evaluator = new Evaluator();
    
    for (let i = 0; i < subtractiveObjects.length; i++) {
        const subObj = subtractiveObjects[i];
        const baseProgress = 30 + (i / subtractiveObjects.length) * 60;
        
        update(baseProgress, `Subtracting object ${i + 1}/${subtractiveObjects.length}...`);
        
        // Run async to prevent UI freeze (EXACT export logic)
        await new Promise(resolve => {
            setTimeout(() => {
                // Clone and prepare geometries - ensure they're indexed (EXACT export logic)
                const geomA = resultMesh.geometry.clone();
                if (!geomA.index) {
                    geomA.setIndex(null);
                    const posArray = geomA.attributes.position.array;
                    const indices = [];
                    for (let i = 0; i < posArray.length / 3; i++) {
                        indices.push(i);
                    }
                    geomA.setIndex(indices);
                }
                if (!geomA.attributes.normal) {
                    geomA.computeVertexNormals();
                }
                if (!geomA.attributes.uv) {
                    const uvArray = new Float32Array(geomA.attributes.position.count * 2);
                    geomA.setAttribute('uv', new THREE.BufferAttribute(uvArray, 2));
                }
                
                const geomB = subObj.mesh.geometry.clone();
                if (!geomB.index) {
                    geomB.setIndex(null);
                    const posArray = geomB.attributes.position.array;
                    const indices = [];
                    for (let i = 0; i < posArray.length / 3; i++) {
                        indices.push(i);
                    }
                    geomB.setIndex(indices);
                }
                if (!geomB.attributes.normal) {
                    geomB.computeVertexNormals();
                }
                if (!geomB.attributes.uv) {
                    const uvArray = new Float32Array(geomB.attributes.position.count * 2);
                    geomB.setAttribute('uv', new THREE.BufferAttribute(uvArray, 2));
                }
                
                // Build BVH trees for CSG operations (EXACT export logic)
                const { computeBoundsTree, disposeBoundsTree, acceleratedRaycast } = window.MeshBVHLib;
                THREE.BufferGeometry.prototype.computeBoundsTree = computeBoundsTree;
                THREE.BufferGeometry.prototype.disposeBoundsTree = disposeBoundsTree;
                THREE.Mesh.prototype.raycast = acceleratedRaycast;
                
                geomA.computeBoundsTree();
                geomB.computeBoundsTree();
                
                // Create brushes - Brush extends Mesh, pass geometry (EXACT export logic)
                const brushA = new Brush(geomA);
                brushA.position.copy(resultMesh.position);
                brushA.rotation.copy(resultMesh.rotation);
                brushA.scale.copy(resultMesh.scale);
                brushA.updateMatrixWorld(true);
                
                const brushB = new Brush(geomB);
                brushB.position.copy(subObj.mesh.position);
                brushB.rotation.copy(subObj.mesh.rotation);
                brushB.scale.copy(subObj.mesh.scale);
                brushB.updateMatrixWorld(true);
                
                // Perform CSG subtraction (EXACT export logic)
                const result = evaluator.evaluate(brushA, brushB, SUBTRACTION);
                resultGeometry = result.geometry;
                resultMesh = new THREE.Mesh(resultGeometry);
                resultMesh.updateMatrixWorld(true);
                resolve();
            }, 0);
        });
    }
    
    update(100, 'Boolean operations complete');
    return resultGeometry;
}

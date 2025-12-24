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
    
    // Apply boolean subtractions if there are any subtractive objects
    if (subtractiveObjects.length > 0) {
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
        
        update(90, 'Boolean operations complete');
    }
    
    // ALWAYS apply 90-degree X rotation for slicer coordinate system
    // This happens ONCE, regardless of whether we did boolean operations or not
    update(95, 'Applying coordinate transform...');
    const rotationMatrix = new THREE.Matrix4().makeRotationX(Math.PI / 2);
    resultGeometry.applyMatrix4(rotationMatrix);
    resultGeometry.computeVertexNormals();
    
    update(100, 'Complete');
    return resultGeometry;
}

/**
 * Generate export-ready geometries PER additive object (with booleans applied).
 * This preserves object identity so the slicer can apply per-object settings,
 * while still respecting subtractive models.
 *
 * Returns: Array of { sourceObjectId, filename, geometry }
 */
async function generateExportGeometriesPerObject(progressCallback = null, options = {}) {
    const additiveObjects = loadedObjects.filter(obj => obj.mode !== 'subtractive');
    const subtractiveObjects = loadedObjects.filter(obj => obj.mode === 'subtractive');
    const resolveAdditiveOverlaps = !!(options && options.resolveAdditiveOverlaps);

    const update = (p, msg) => {
        if (progressCallback) progressCallback(p, msg);
    };

    if (additiveObjects.length === 0) {
        throw new Error('No additive objects to process');
    }

    update(5, `Preparing ${additiveObjects.length} additive object(s)...`);

    const results = [];

    const needsCSG = subtractiveObjects.length > 0 || resolveAdditiveOverlaps;
    const csg = needsCSG ? window.THREEDCSG : null;
    const bvh = needsCSG ? window.MeshBVHLib : null;
    const evaluator = (needsCSG && csg) ? new csg.Evaluator() : null;

    const prepareIndexed = (geom, { allowEmpty = false } = {}) => {
        if (!geom || !geom.isBufferGeometry) {
            if (allowEmpty) return null;
            throw new Error('CSG: expected BufferGeometry');
        }

        const posAttr = geom.getAttribute('position');
        if (!posAttr || !Number.isFinite(posAttr.count) || posAttr.count <= 0) {
            if (allowEmpty) return null;
            throw new Error('CSG: geometry missing position attribute');
        }

        if (!geom.index || !Number.isFinite(geom.index.count)) {
            const vertexCount = posAttr.count;
            const useUint32 = vertexCount > 65535;
            const indexArray = useUint32 ? new Uint32Array(vertexCount) : new Uint16Array(vertexCount);
            for (let i = 0; i < vertexCount; i++) indexArray[i] = i;
            geom.setIndex(new THREE.BufferAttribute(indexArray, 1));
        }

        if (!geom.attributes.normal) geom.computeVertexNormals();
        if (!geom.attributes.uv) {
            const uvArray = new Float32Array(posAttr.count * 2);
            geom.setAttribute('uv', new THREE.BufferAttribute(uvArray, 2));
        }

        return geom;
    };

    const buildBrushFromGeometry = (geom) => {
        if (!needsCSG) return null;
        const prepared = prepareIndexed(geom, { allowEmpty: true });
        if (!prepared) return null;

        const { computeBoundsTree, disposeBoundsTree, acceleratedRaycast } = bvh;
        THREE.BufferGeometry.prototype.computeBoundsTree = computeBoundsTree;
        THREE.BufferGeometry.prototype.disposeBoundsTree = disposeBoundsTree;
        THREE.Mesh.prototype.raycast = acceleratedRaycast;
        prepared.computeBoundsTree();

        const { Brush } = csg;
        const brush = new Brush(prepared);
        brush.position.set(0, 0, 0);
        brush.rotation.set(0, 0, 0);
        brush.scale.set(1, 1, 1);
        brush.updateMatrixWorld(true);
        return brush;
    };

    const getBakedObjectGeometry = (obj, { allowEmpty = false } = {}) => {
        const sourceGeom = (obj && obj.geometry && obj.geometry.isBufferGeometry)
            ? obj.geometry
            : (obj && obj.mesh && obj.mesh.geometry && obj.mesh.geometry.isBufferGeometry)
                ? obj.mesh.geometry
                : null;

        const baked = sourceGeom ? sourceGeom.clone() : null;
        if (!baked) {
            if (allowEmpty) return null;
            throw new Error('CSG: object has no geometry');
        }

        if (obj && obj.mesh && obj.mesh.matrix) {
            baked.applyMatrix4(obj.mesh.matrix);
        }

        return baked;
    };

    // Priority order: the list order of additiveObjects (mirrors UI list / load order)
    const blockerBrushes = [];

    for (let a = 0; a < additiveObjects.length; a++) {
        const obj = additiveObjects[a];
        update(5 + (a / additiveObjects.length) * 70, `Processing ${a + 1}/${additiveObjects.length}: ${obj.filename}`);

        let resultGeometry;
        try {
            resultGeometry = getBakedObjectGeometry(obj);
        } catch (e) {
            console.warn('Skipping object due to invalid geometry:', obj && obj.filename, e);
            resultGeometry = new THREE.BufferGeometry();
        }

        // Apply booleans (overlap resolution + subtractive objects)
        if (needsCSG) {
            const { Brush, SUBTRACTION } = csg;
            let currentGeometry = resultGeometry;

            // Overlap resolution: later objects subtract earlier blockers
            if (resolveAdditiveOverlaps && blockerBrushes.length > 0) {
                update(5 + (a / additiveObjects.length) * 70, `Resolving overlaps for ${obj.filename}...`);
                for (let b = 0; b < blockerBrushes.length; b++) {
                    const blocker = blockerBrushes[b];
                    if (!blocker) continue;
                    await new Promise(resolve => {
                        setTimeout(() => {
                            try {
                                const geomA = currentGeometry.clone();
                                const preparedA = prepareIndexed(geomA, { allowEmpty: true });
                                if (!preparedA) {
                                    currentGeometry = new THREE.BufferGeometry();
                                    return resolve();
                                }

                                const { computeBoundsTree, disposeBoundsTree, acceleratedRaycast } = bvh;
                                THREE.BufferGeometry.prototype.computeBoundsTree = computeBoundsTree;
                                THREE.BufferGeometry.prototype.disposeBoundsTree = disposeBoundsTree;
                                THREE.Mesh.prototype.raycast = acceleratedRaycast;
                                preparedA.computeBoundsTree();

                                const brushA = new Brush(preparedA);
                                brushA.position.set(0, 0, 0);
                                brushA.rotation.set(0, 0, 0);
                                brushA.scale.set(1, 1, 1);
                                brushA.updateMatrixWorld(true);

                                const result = evaluator.evaluate(brushA, blocker, SUBTRACTION);
                                if (!result || !result.geometry || !result.geometry.getAttribute || !result.geometry.getAttribute('position') || result.geometry.getAttribute('position').count === 0) {
                                    currentGeometry = new THREE.BufferGeometry();
                                } else {
                                    currentGeometry = result.geometry;
                                }
                            } catch (err) {
                                console.warn('CSG overlap resolution failed:', err);
                            }
                            resolve();
                        }, 0);
                    });

                    // Early exit if empty
                    const pos = currentGeometry.getAttribute && currentGeometry.getAttribute('position');
                    if (!pos || pos.count === 0) break;
                }
            }

            // Subtractive objects
            for (let s = 0; s < subtractiveObjects.length; s++) {
                const subObj = subtractiveObjects[s];
                const baseProgress = 5 + (a / additiveObjects.length) * 70 + ((s / Math.max(1, subtractiveObjects.length)) * (70 / additiveObjects.length));
                update(baseProgress, `Subtracting ${subObj.filename} from ${obj.filename}...`);

                await new Promise(resolve => {
                    setTimeout(() => {
                        try {
                            const geomA = currentGeometry.clone();
                            const preparedA = prepareIndexed(geomA, { allowEmpty: true });
                            if (!preparedA) {
                                currentGeometry = new THREE.BufferGeometry();
                                return resolve();
                            }

                            const subGeom = getBakedObjectGeometry(subObj, { allowEmpty: true });
                            const preparedB = prepareIndexed(subGeom, { allowEmpty: true });
                            if (!preparedB) return resolve();

                            const { computeBoundsTree, disposeBoundsTree, acceleratedRaycast } = bvh;
                            THREE.BufferGeometry.prototype.computeBoundsTree = computeBoundsTree;
                            THREE.BufferGeometry.prototype.disposeBoundsTree = disposeBoundsTree;
                            THREE.Mesh.prototype.raycast = acceleratedRaycast;
                            preparedA.computeBoundsTree();
                            preparedB.computeBoundsTree();

                            const brushA = new Brush(preparedA);
                            brushA.position.set(0, 0, 0);
                            brushA.rotation.set(0, 0, 0);
                            brushA.scale.set(1, 1, 1);
                            brushA.updateMatrixWorld(true);

                            const brushB = new Brush(preparedB);
                            brushB.position.set(0, 0, 0);
                            brushB.rotation.set(0, 0, 0);
                            brushB.scale.set(1, 1, 1);
                            brushB.updateMatrixWorld(true);

                            const result = evaluator.evaluate(brushA, brushB, SUBTRACTION);
                            if (!result || !result.geometry || !result.geometry.getAttribute || !result.geometry.getAttribute('position') || result.geometry.getAttribute('position').count === 0) {
                                currentGeometry = new THREE.BufferGeometry();
                            } else {
                                currentGeometry = result.geometry;
                            }
                        } catch (err) {
                            console.warn('CSG subtractive operation failed:', err);
                        }
                        resolve();
                    }, 0);
                });

                const pos = currentGeometry.getAttribute && currentGeometry.getAttribute('position');
                if (!pos || pos.count === 0) break;
            }

            resultGeometry = currentGeometry;
        }

        // Add blocker AFTER all ops, so it matches what will be printed
        if (resolveAdditiveOverlaps && needsCSG) {
            try {
                blockerBrushes.push(buildBrushFromGeometry(resultGeometry.clone()));
            } catch (e) {
                console.warn('Failed to build overlap blocker for', obj && obj.filename, e);
                blockerBrushes.push(null);
            }
        }

        // Coordinate transform (same as generateExportGeometry)
        const rotationMatrix = new THREE.Matrix4().makeRotationX(Math.PI / 2);
        const posAttr = resultGeometry.getAttribute && resultGeometry.getAttribute('position');
        if (posAttr && posAttr.count > 0) {
            resultGeometry.applyMatrix4(rotationMatrix);
            resultGeometry.computeVertexNormals();
        }

        results.push({
            sourceObjectId: obj.id,
            filename: obj.filename,
            geometry: resultGeometry
        });
    }

    update(100, 'Complete');
    return results;
}

/**
 * Main Slicer - Coordinates all slicing operations
 * 100% client-side, no backend required
 */

class Encoder3DSlicer {
    constructor() {
        this.core = new SlicerCore();
        this.pathGen = new PathGenerator();
        this.gcodeGen = new GCodeGenerator();
        this.settings = this.getDefaultSettings();
    }

    /**
     * Get default slicer settings
     */
    getDefaultSettings() {
        return {
            // Layer settings
            layerHeight: 0.2,
            firstLayerHeight: 0.3,
            
            // Perimeter settings
            wallCount: 2,
            lineWidth: 0.4,
            perimeterSpeed: 50,
            
            // Infill settings
            infillDensity: 20,
            infillPattern: 'rectilinear', // rectilinear, grid, honeycomb
            infillSpeed: 80,
            infillAngle: 45,
            infillOverlap: 15,
            
            // Temperature
            nozzleTemp: 200,
            bedTemp: 60,
            
            // Speed
            printSpeed: 60,
            travelSpeed: 150,
            firstLayerSpeed: 20,
            
            // Retraction
            retraction: 5,
            retractionSpeed: 40,
            
            // Filament
            filamentDiameter: 1.75,
            
            // Cooling
            fanSpeed: 100,
            firstLayerFanSpeed: 0,
            
            // Support
            enableSupport: false,
            supportDensity: 15,
            supportAngle: 45,
            
            // Advanced
            zHop: 0.2,
            combing: true,
            minSegmentLength: 0.3
        };
    }

    /**
     * Slice objects and generate GCode
     * @param {Array} objects - Array of {mesh, geometry, transform} objects
     * @param {Object} customSettings - Override settings
     * @param {Function} progressCallback - Progress updates
     * @returns {Promise} Resolves with {gcode, layers, stats}
     */
    async slice(objects, customSettings = {}, progressCallback = null) {
        // Merge settings
        const settings = { ...this.settings, ...customSettings };
        
        if (progressCallback) progressCallback(0, 'Initializing...');
        
        // Allow UI to update
        await new Promise(resolve => setTimeout(resolve, 10));
        
        // Slice all objects (apply per-object overrides)
        const allLayers = [];
        for (let i = 0; i < objects.length; i++) {
            const obj = objects[i];
            if (progressCallback) {
                progressCallback((i / objects.length) * 30, `Slicing object ${i + 1}/${objects.length}: ${obj.filename}`);
            }

            await new Promise(resolve => setTimeout(resolve, 10));

            const objSettings = this.applyPerObjectOverrides(settings, obj.slicerOverrides);
            const layers = this.core.sliceGeometry(obj.geometry, obj.transform, objSettings);
            allLayers.push({ object: obj, layers, settings: objSettings });
        }

        if (progressCallback) progressCallback(35, 'Merging layers...');
        await new Promise(resolve => setTimeout(resolve, 10));

        // Merge layers by Z height, while preserving object identity
        const mergedLayers = this.mergeLayersByZ(allLayers, settings.layerHeight || 0.2);
        
        console.log(`After merge: ${mergedLayers.length} total merged layers`);
        
        if (progressCallback) progressCallback(40, 'Generating toolpaths...');

        // Generate toolpaths per object per layer and flatten into merged layers
        for (let i = 0; i < mergedLayers.length; i++) {
            if (progressCallback) {
                progressCallback(40 + (i / mergedLayers.length) * 40, `Layer ${i + 1}/${mergedLayers.length}`);
            }

            if (i % 5 === 0) {
                await new Promise(resolve => setTimeout(resolve, 10));
            }

            const layer = mergedLayers[i];
            layer.perimeters = [];
            layer.infill = [];

            for (const entry of layer.objectEntries) {
                // Skip subtractive objects (they're only used for cutting)
                if (entry.object && entry.object.mode === 'subtractive') continue;

                const contours = this.core.connectSegments(entry.segments);

                const perimeters = this.pathGen.generatePerimeters(contours, {
                    ...entry.settings,
                    layerIndex: i
                });

                // Attach per-path settings
                perimeters.forEach(p => {
                    p.sourceObjectId = entry.object && entry.object.id;
                    p.sourceFilename = entry.object && entry.object.filename;
                    const os = entry.settings || {};
                    const speed = (p.type === 'outer-wall')
                        ? (os.perimeterSpeed || os.printSpeed)
                        : (os.innerPerimeterSpeed || os.perimeterSpeed || os.printSpeed);
                    p.speed = speed;
                    p.width = p.width || os.lineWidth;
                });

                const infill = this.pathGen.generateInfill(contours, perimeters, {
                    ...entry.settings,
                    layerIndex: i
                });
                infill.forEach(p => {
                    p.sourceObjectId = entry.object && entry.object.id;
                    p.sourceFilename = entry.object && entry.object.filename;
                    const os = entry.settings || {};
                    p.speed = os.infillSpeed || (os.printSpeed ? os.printSpeed * 1.5 : undefined);
                    p.width = p.width || os.lineWidth;
                });

                layer.perimeters.push(...perimeters);
                layer.infill.push(...infill);
            }
        }
        
        if (progressCallback) progressCallback(85, 'Generating GCode...');
        await new Promise(resolve => setTimeout(resolve, 10));
        
        console.log(`Generating GCode for ${mergedLayers.length} layers...`);
        
        // Generate GCode
        const gcode = this.gcodeGen.generate(mergedLayers, settings);
        
        console.log(`GCode generation complete. Length: ${gcode.length} chars`);
        
        if (progressCallback) progressCallback(95, 'Calculating statistics...');
        await new Promise(resolve => setTimeout(resolve, 10));
        
        // Calculate stats
        const stats = this.calculateStats(mergedLayers, gcode, settings);
        
        if (progressCallback) progressCallback(100, 'Complete!');
        
        return {
            gcode: gcode,
            layers: mergedLayers,
            stats: stats
        };
    }

    /**
     * Apply per-object slicer overrides while keeping critical global constraints.
     * For now, we only allow a small set of overrides that are safe with a single
     * interwoven GCode output.
     */
    applyPerObjectOverrides(baseSettings, overrides) {
        if (!overrides) return { ...baseSettings };
        const allowed = [
            'wallCount',
            'infillDensity',
            'printSpeed',
            'perimeterSpeed',
            'innerPerimeterSpeed',
            'infillSpeed',
            'lineWidth',
            'infillPattern',
            'infillAngle',
            'infillOverlap',
            'minSegmentLength'
        ];
        const merged = { ...baseSettings };
        for (const key of allowed) {
            if (overrides[key] !== undefined) merged[key] = overrides[key];
        }
        return merged;
    }

    /**
     * Merge layers from multiple objects by Z height while preserving per-object segments/settings.
     */
    mergeLayersByZ(allLayers, layerHeight) {
        const epsilon = Math.max(0.0005, (layerHeight || 0.2) * 0.1);
        const merged = [];

        const findLayer = (z) => merged.find(l => Math.abs(l.z - z) < epsilon);

        // Build merged layer buckets
        allLayers.forEach(objLayers => {
            objLayers.layers.forEach(layer => {
                let bucket = findLayer(layer.z);
                if (!bucket) {
                    bucket = {
                        index: merged.length,
                        z: layer.z,
                        objectEntries: [],
                        perimeters: [],
                        infill: []
                    };
                    merged.push(bucket);
                }
                bucket.objectEntries.push({
                    object: objLayers.object,
                    segments: layer.segments,
                    settings: objLayers.settings
                });
            });
        });

        // If there are subtractive objects and additive objects, apply segment filtering per object
        merged.forEach(layer => {
            const subtractiveSegs = [];
            const additiveEntries = [];
            for (const entry of layer.objectEntries) {
                if (entry.object && entry.object.mode === 'subtractive') subtractiveSegs.push(...entry.segments);
                else additiveEntries.push(entry);
            }

            if (subtractiveSegs.length > 0 && additiveEntries.length > 0) {
                const subtractiveContours = this.core.connectSegments(subtractiveSegs);
                for (const entry of additiveEntries) {
                    entry.segments = this.subtractSegments(entry.segments, subtractiveContours);
                }
            }

            // Remove entries with no segments
            layer.objectEntries = layer.objectEntries.filter(e => e.segments && e.segments.length > 0);
        });

        return merged
            .filter(layer => layer.objectEntries.length > 0)
            .sort((a, b) => a.z - b.z)
            .map((layer, index) => ({ ...layer, index }));
    }
    
    /**
     * Subtract segments that are inside subtractive contours
     */
    subtractSegments(additiveSegments, subtractiveContours) {
        if (subtractiveContours.length === 0) return additiveSegments;
        
        const resultSegments = [];
        let removedCount = 0;
        
        additiveSegments.forEach((seg, idx) => {
            if (!seg.start || !seg.end) {
                console.error(`Segment ${idx} missing start/end:`, seg);
                resultSegments.push(seg);
                return;
            }
            
            const midX = (seg.start.x + seg.end.x) / 2;
            const midY = (seg.start.y + seg.end.y) / 2;
            
            if (isNaN(midX) || isNaN(midY)) {
                console.error(`Invalid midpoint for segment ${idx}:`, midX, midY, seg);
                resultSegments.push(seg);
                return;
            }
            
            // Check if segment midpoint is inside any subtractive contour
            let inside = false;
            for (const contour of subtractiveContours) {
                if (this.isPointInPolygon(midX, midY, contour)) {
                    inside = true;
                    removedCount++;
                    break;
                }
            }
            
            // Only keep segment if it's NOT inside a subtractive contour
            if (!inside) {
                resultSegments.push(seg);
            }
        });
        
        console.log(`  Subtraction: ${additiveSegments.length} input → ${removedCount} removed → ${resultSegments.length} remaining`);
        return resultSegments;
    }
    
    /**
     * Point-in-polygon test using ray casting algorithm
     */
    isPointInPolygon(x, y, polygon) {
        let inside = false;
        for (let i = 0, j = polygon.length - 1; i < polygon.length; j = i++) {
            const xi = polygon[i].x, yi = polygon[i].y;
            const xj = polygon[j].x, yj = polygon[j].y;
            
            const intersect = ((yi > y) !== (yj > y))
                && (x < (xj - xi) * (y - yi) / (yj - yi) + xi);
            if (intersect) inside = !inside;
        }
        return inside;
    }

    /**
     * Calculate print statistics
     */
    calculateStats(layers, gcode, settings) {
        const gcodeLines = gcode.split('\n');
        
        // Count moves
        let extrusionMoves = 0;
        let travelMoves = 0;
        let totalExtrusion = 0;
        
        gcodeLines.forEach(line => {
            if (line.startsWith('G1') && line.includes('E')) {
                extrusionMoves++;
                const eMatch = line.match(/E([\d.-]+)/);
                if (eMatch) {
                    totalExtrusion = Math.max(totalExtrusion, parseFloat(eMatch[1]));
                }
            } else if (line.startsWith('G0') || (line.startsWith('G1') && !line.includes('E'))) {
                travelMoves++;
            }
        });
        
        // Calculate filament usage
        const filamentDiameter = settings.filamentDiameter || 1.75;
        const filamentArea = Math.PI * Math.pow(filamentDiameter / 2, 2);
        const filamentLength = totalExtrusion; // mm
        const filamentVolume = filamentLength * filamentArea; // mm³
        const filamentWeight = (filamentVolume / 1000) * 1.24; // g (PLA density ~1.24 g/cm³)
        
        // Estimate print time
        const printTime = this.gcodeGen.estimatePrintTime(gcode);
        
        return {
            layers: layers.length,
            gcodeLines: gcodeLines.length,
            extrusionMoves: extrusionMoves,
            travelMoves: travelMoves,
            filamentLength: filamentLength.toFixed(2) + ' mm',
            filamentVolume: (filamentVolume / 1000).toFixed(2) + ' cm³',
            filamentWeight: filamentWeight.toFixed(2) + ' g',
            estimatedTime: printTime
        };
    }

    /**
     * Update settings
     */
    updateSettings(newSettings) {
        this.settings = { ...this.settings, ...newSettings };
    }

    /**
     * Get current settings
     */
    getSettings() {
        return { ...this.settings };
    }

    /**
     * Load settings profile
     */
    loadProfile(profile) {
        const profiles = {
            'fast': {
                layerHeight: 0.3,
                infillDensity: 10,
                wallCount: 2,
                printSpeed: 80
            },
            'balanced': {
                layerHeight: 0.2,
                infillDensity: 20,
                wallCount: 2,
                printSpeed: 60
            },
            'quality': {
                layerHeight: 0.1,
                infillDensity: 30,
                wallCount: 3,
                printSpeed: 40
            },
            'strong': {
                layerHeight: 0.2,
                infillDensity: 50,
                wallCount: 4,
                printSpeed: 50
            }
        };
        
        if (profiles[profile]) {
            this.updateSettings(profiles[profile]);
        }
    }
}

// Global slicer instance
let encoder3DSlicer = null;

// Initialize slicer when page loads
window.addEventListener('load', function() {
    encoder3DSlicer = new Encoder3DSlicer();
    console.log('Encoder3D Slicer initialized');
});

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
            combing: true
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
        
        // Slice all objects
        const allLayers = [];
        let maxLayerCount = 0;
        
        for (let i = 0; i < objects.length; i++) {
            const obj = objects[i];
            if (progressCallback) {
                progressCallback((i / objects.length) * 30, `Slicing object ${i + 1}/${objects.length}: ${obj.filename}`);
            }
            
            // Allow UI to update and check for cancellation
            await new Promise(resolve => setTimeout(resolve, 10));
            
            const layers = this.core.sliceGeometry(
                obj.geometry,
                obj.transform,
                settings
            );
            
            allLayers.push({ object: obj, layers: layers });
            maxLayerCount = Math.max(maxLayerCount, layers.length);
        }
        
        if (progressCallback) progressCallback(35, 'Merging layers...');
        await new Promise(resolve => setTimeout(resolve, 10));
        
        // Merge layers from all objects by Z height
        const mergedLayers = this.mergeLayers(allLayers, maxLayerCount);
        
        console.log(`After merge: ${mergedLayers.length} total merged layers`);
        
        if (progressCallback) progressCallback(40, 'Generating toolpaths...');
        
        // Generate paths for each layer
        for (let i = 0; i < mergedLayers.length; i++) {
            if (progressCallback) {
                progressCallback(40 + (i / mergedLayers.length) * 40, `Layer ${i + 1}/${mergedLayers.length}`);
            }
            
            // Allow UI to update every 5 layers
            if (i % 5 === 0) {
                await new Promise(resolve => setTimeout(resolve, 10));
            }
            
            const layer = mergedLayers[i];
            
            // Connect segments into contours
            const contours = this.core.connectSegments(layer.segments);
            
            // Generate perimeters
            layer.perimeters = this.pathGen.generatePerimeters(contours, {
                ...settings,
                layerIndex: i
            });
            
            // Generate infill
            layer.infill = this.pathGen.generateInfill(contours, layer.perimeters, {
                ...settings,
                layerIndex: i
            });
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
     * Merge layers from multiple objects by Z height
     */
    mergeLayers(allLayers, maxLayerCount) {
        const mergedLayers = [];
        const epsilon = 0.01; // Z height tolerance
        
        // Create layer structure
        for (let i = 0; i < maxLayerCount; i++) {
            mergedLayers.push({
                index: i,
                z: 0,
                additiveSegments: [],
                subtractiveSegments: [],
                segments: [],
                perimeters: [],
                infill: []
            });
        }
        
        // Separate additive and subtractive objects
        allLayers.forEach(objLayers => {
            const isSubtractive = objLayers.object.mode === 'subtractive';
            console.log(`Object "${objLayers.object.filename}" mode: ${objLayers.object.mode} (isSubtractive: ${isSubtractive})`);
            
            objLayers.layers.forEach(layer => {
                // Find matching merged layer by Z height
                let targetLayer = mergedLayers.find(ml => 
                    Math.abs(ml.z - layer.z) < epsilon
                );
                
                if (!targetLayer) {
                    // Create new layer if needed (layer.index might be out of bounds)
                    targetLayer = {
                        index: mergedLayers.length,
                        z: layer.z,
                        additiveSegments: [],
                        subtractiveSegments: [],
                        segments: [],
                        perimeters: [],
                        infill: []
                    };
                    mergedLayers.push(targetLayer);
                }
                
                // Add segments to appropriate array
                if (isSubtractive) {
                    targetLayer.subtractiveSegments.push(...layer.segments);
                } else {
                    targetLayer.additiveSegments.push(...layer.segments);
                }
            });
        });
        
        // Process boolean operations for each layer
        mergedLayers.forEach(layer => {
            if (layer.subtractiveSegments.length > 0 && layer.additiveSegments.length > 0) {
                console.log(`Layer ${layer.index}: ${layer.additiveSegments.length} additive segments, ${layer.subtractiveSegments.length} subtractive segments`);
                // Build subtractive contours
                const subtractiveContours = this.core.connectSegments(layer.subtractiveSegments);
                console.log(`Layer ${layer.index}: Built ${subtractiveContours.length} subtractive contours`);
                
                // Filter additive segments that aren't inside subtractive contours
                layer.segments = this.subtractSegments(layer.additiveSegments, subtractiveContours);
                console.log(`Layer ${layer.index}: After subtraction, ${layer.segments.length} segments remain`);
            } else {
                // No subtraction needed, just use additive segments
                layer.segments = layer.additiveSegments;
            }
        });
        
        // Remove empty layers and sort by Z
        return mergedLayers
            .filter(layer => layer.segments.length > 0)
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

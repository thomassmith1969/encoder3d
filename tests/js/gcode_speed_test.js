// Test GCode generator uses different speeds for perimeters vs infill
const GCodeGenerator = require('../../data/www/js/slicer/slicer-gcode.js');

const gen = new GCodeGenerator();

const layers = [{
    z: 0.2,
    perimeters: [
        {
            type: 'outer-wall',
            points: [{ x: 0, y: 0 }, { x: 10, y: 0 }],
            width: 0.4
        },
        {
            type: 'inner-wall',
            points: [{ x: 1, y: 1 }, { x: 9, y: 1 }],
            width: 0.4
        }
    ],
    infill: [
        {
            type: 'infill',
            points: [{ x: 2, y: 2 }, { x: 8, y: 2 }],
            width: 0.4
        }
    ]
}];

const settings = {
    layerHeight: 0.2,
    lineWidth: 0.4,
    nozzleTemp: 200,
    bedTemp: 60,
    perimeterSpeed: 50,
    infillSpeed: 80,
    infillDensity: 20,
    wallCount: 2,
    retraction: 5,
    travelSpeed: 150,
    printSpeed: 60
};

const gcode = gen.generate(layers, settings);

console.log('Test: GCode uses different speeds for perimeters and infill');

// Check for perimeter speed (F3000 = 50mm/s * 60)
const hasPerimeterSpeed = gcode.includes('F3000');
console.log('  Has perimeter speed F3000 (50mm/s):', hasPerimeterSpeed ? '✓' : '✗');

// Check for infill speed (F4800 = 80mm/s * 60)
const hasInfillSpeed = gcode.includes('F4800');
console.log('  Has infill speed F4800 (80mm/s):', hasInfillSpeed ? '✓' : '✗');

if (hasPerimeterSpeed && hasInfillSpeed) {
    console.log('  ✓ PASS');
} else {
    console.log('  ✗ FAIL');
    process.exit(1);
}

console.log('\n✓ GCode speed test passed');

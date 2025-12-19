// Test perimeter generation with wallCount and minSegmentLength
const PathGenerator = require('../../data/www/js/slicer/slicer-paths.js');

const gen = new PathGenerator();

// Test: wallCount affects number of perimeters
const square = [
    { x: 0, y: 0 },
    { x: 10, y: 0 },
    { x: 10, y: 10 },
    { x: 0, y: 10 }
];

const settings1 = { wallCount: 3, lineWidth: 0.4, minSegmentLength: 0.1 };
const perims1 = gen.generatePerimeters([square], settings1);

console.log('Test 1: wallCount=3 produces 3 perimeters per contour');
console.log('  Generated', perims1.length, 'perimeters');
if (perims1.length === 3) {
    console.log('  ✓ PASS');
} else {
    console.log('  ✗ FAIL');
    process.exit(1);
}

// Test: minSegmentLength filters out tiny segments
const zigzag = [
    { x: 0, y: 0 },
    { x: 0.05, y: 0 },
    { x: 5, y: 0 },
    { x: 5.05, y: 0 },
    { x: 5, y: 5 },
    { x: 0, y: 5 }
];

const settings2 = { wallCount: 1, lineWidth: 0.4, minSegmentLength: 0.3 };
const perims2 = gen.generatePerimeters([zigzag], settings2);

console.log('\nTest 2: minSegmentLength removes tiny segments');
console.log('  Input had 6 points, minSegmentLength=0.3');
console.log('  Output has', perims2[0].points.length, 'points');
if (perims2[0].points.length < 6) {
    console.log('  ✓ PASS (filtered out some tiny segments)');
} else {
    console.log('  ✗ FAIL');
    process.exit(1);
}

console.log('\n✓ All perimeter tests passed');

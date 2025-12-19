// Test infill overlap extension
const PathGenerator = require('../../data/www/js/slicer/slicer-paths.js');

const gen = new PathGenerator();

const square = [
    { x: 0, y: 0 },
    { x: 10, y: 0 },
    { x: 10, y: 10 },
    { x: 0, y: 10 }
];

// Generate without overlap
const settings1 = { 
    layerIndex: 0,
    infillAngle: 0,
    minSegmentLength: 0.1,
    infillOverlap: 0
};
const infill1 = gen.generateRectilinearInfill(square, 0.2, 0.4, settings1);

// Generate with overlap
const settings2 = { 
    layerIndex: 0,
    infillAngle: 0,
    minSegmentLength: 0.1,
    infillOverlap: 25  // 25% overlap
};
const infill2 = gen.generateRectilinearInfill(square, 0.2, 0.4, settings2);

// Calculate average segment length for each
function avgLength(infill) {
    let total = 0;
    infill.forEach(seg => {
        const dx = seg.points[1].x - seg.points[0].x;
        const dy = seg.points[1].y - seg.points[0].y;
        total += Math.sqrt(dx * dx + dy * dy);
    });
    return total / infill.length;
}

const avg1 = avgLength(infill1);
const avg2 = avgLength(infill2);

console.log('Test: infillOverlap extends segments');
console.log('  Without overlap: avg length =', avg1.toFixed(2));
console.log('  With 25% overlap: avg length =', avg2.toFixed(2));

if (avg2 > avg1) {
    console.log('  ✓ PASS (overlap increases segment length)');
} else {
    console.log('  ✗ FAIL');
    process.exit(1);
}

console.log('\n✓ Infill overlap test passed');

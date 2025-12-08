// WebSocket Connection
const ws = new WebSocket('ws://' + window.location.hostname + ':81/');

ws.onopen = function() {
    console.log('WebSocket Connected');
};

ws.onmessage = function(event) {
    const data = JSON.parse(event.data);
    if (data.x !== undefined) {
        document.getElementById('pos-x').innerText = data.x.toFixed(2);
        document.getElementById('pos-y').innerText = data.y.toFixed(2);
        document.getElementById('pos-z').innerText = data.z.toFixed(2);
        document.getElementById('pos-e').innerText = data.e.toFixed(2);
        
        // Update Visualizer Toolhead
        updateVisualizer(data.x, data.y, data.z);
    }
};

function sendGCode(cmd) {
    ws.send(cmd);
}

function setTemps() {
    const ext = document.getElementById('target-ext').value;
    const bed = document.getElementById('target-bed').value;
    sendGCode(`M104 S${ext}`);
    sendGCode(`M140 S${bed}`);
}

function uploadGCode() {
    const fileInput = document.getElementById('gcode-upload');
    const file = fileInput.files[0];
    if (!file) return;

    const formData = new FormData();
    formData.append("file", file);

    fetch('/api/upload', {
        method: 'POST',
        body: formData
    })
    .then(response => response.text())
    .then(result => {
        console.log('Success:', result);
        alert('Upload Complete');
    })
    .catch(error => {
        console.error('Error:', error);
    });
}

// --- Three.js Visualizer ---
let scene, camera, renderer, toolhead;

function initVisualizer() {
    const container = document.getElementById('viewer-container');
    scene = new THREE.Scene();
    scene.background = new THREE.Color(0x1e1e1e);
    
    // Grid
    const gridHelper = new THREE.GridHelper(200, 20);
    scene.add(gridHelper);
    
    // Camera
    camera = new THREE.PerspectiveCamera(75, container.clientWidth / container.clientHeight, 0.1, 1000);
    camera.position.set(100, 100, 100);
    camera.lookAt(0, 0, 0);
    
    // Renderer
    renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(container.clientWidth, container.clientHeight);
    container.appendChild(renderer.domElement);
    
    // Toolhead (Cone)
    const geometry = new THREE.ConeGeometry(5, 20, 32);
    const material = new THREE.MeshBasicMaterial({ color: 0xff0000 });
    toolhead = new THREE.Mesh(geometry, material);
    toolhead.rotation.x = Math.PI; // Point down
    scene.add(toolhead);
    
    // Lights
    const light = new THREE.AmbientLight(0x404040); // soft white light
    scene.add(light);
    
    animate();
}

function updateVisualizer(x, y, z) {
    if (toolhead) {
        toolhead.position.set(x, z, y); // Swap Y/Z for 3D space
    }
}

function animate() {
    requestAnimationFrame(animate);
    renderer.render(scene, camera);
}

// Handle Resize
window.addEventListener('resize', () => {
    const container = document.getElementById('viewer-container');
    camera.aspect = container.clientWidth / container.clientHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(container.clientWidth, container.clientHeight);
});

initVisualizer();

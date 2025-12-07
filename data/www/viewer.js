/*
 * Encoder3D CNC Controller - 3D STL Viewer
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 */

let scene, camera, renderer, controls;
let currentModel = null;
let gridHelper, axesHelper;
let wireframeMode = false;

// Initialize Three.js viewer
function initViewer() {
    const container = document.getElementById('viewer-container');
    if (!container) return;

    // Scene
    scene = new THREE.Scene();
    scene.background = new THREE.Color(0x1a1a1a);

    // Camera
    camera = new THREE.PerspectiveCamera(
        45,
        container.clientWidth / container.clientHeight,
        0.1,
        10000
    );
    camera.position.set(200, 200, 200);

    // Renderer
    renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(container.clientWidth, container.clientHeight);
    renderer.shadowMap.enabled = true;
    container.appendChild(renderer.domElement);

    // Controls
    controls = new THREE.OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.05;
    controls.minDistance = 10;
    controls.maxDistance = 1000;

    // Lights
    const ambientLight = new THREE.AmbientLight(0x404040, 2);
    scene.add(ambientLight);

    const directionalLight1 = new THREE.DirectionalLight(0xffffff, 1);
    directionalLight1.position.set(1, 1, 1);
    scene.add(directionalLight1);

    const directionalLight2 = new THREE.DirectionalLight(0xffffff, 0.5);
    directionalLight2.position.set(-1, -1, -1);
    scene.add(directionalLight2);

    // Grid
    gridHelper = new THREE.GridHelper(200, 20, 0x444444, 0x222222);
    scene.add(gridHelper);

    // Axes
    axesHelper = new THREE.AxesHelper(100);
    scene.add(axesHelper);

    // Build plate
    const plateGeometry = new THREE.PlaneGeometry(200, 200);
    const plateMaterial = new THREE.MeshBasicMaterial({
        color: 0x333333,
        side: THREE.DoubleSide,
        transparent: true,
        opacity: 0.3
    });
    const plate = new THREE.Mesh(plateGeometry, plateMaterial);
    plate.rotation.x = -Math.PI / 2;
    scene.add(plate);

    // Handle window resize
    window.addEventListener('resize', onViewerResize);

    // Animation loop
    animate();
}

function animate() {
    requestAnimationFrame(animate);
    controls.update();
    renderer.render(scene, camera);
}

function onViewerResize() {
    const container = document.getElementById('viewer-container');
    if (!container || !camera || !renderer) return;

    camera.aspect = container.clientWidth / container.clientHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(container.clientWidth, container.clientHeight);
}

// Load STL file
function loadSTL() {
    const fileInput = document.getElementById('stl-file');
    const file = fileInput.files[0];
    
    if (!file) {
        alert('Please select an STL file');
        return;
    }

    const reader = new FileReader();
    reader.onload = function(event) {
        const arrayBuffer = event.target.result;
        
        const loader = new THREE.STLLoader();
        const geometry = loader.parse(arrayBuffer);
        
        // Remove old model
        if (currentModel) {
            scene.remove(currentModel);
        }

        // Create material
        const material = new THREE.MeshPhongMaterial({
            color: 0xff6b35,
            specular: 0x111111,
            shininess: 30
        });

        // Create mesh
        currentModel = new THREE.Mesh(geometry, material);
        
        // Center the model
        geometry.computeBoundingBox();
        const bbox = geometry.boundingBox;
        const center = new THREE.Vector3();
        bbox.getCenter(center);
        currentModel.position.sub(center);
        
        // Position on build plate
        currentModel.position.y = -bbox.min.y + center.y;
        
        scene.add(currentModel);

        // Update info
        updateModelInfo(file.name, geometry);

        // Fit camera to model
        fitCameraToModel();

        console.log('STL loaded:', file.name);
    };

    reader.readAsArrayBuffer(file);
}

function updateModelInfo(filename, geometry) {
    document.getElementById('model-name').textContent = filename;
    
    geometry.computeBoundingBox();
    const bbox = geometry.boundingBox;
    const size = new THREE.Vector3();
    bbox.getSize(size);
    
    document.getElementById('model-size').textContent = 
        `${size.x.toFixed(1)} × ${size.y.toFixed(1)} × ${size.z.toFixed(1)} mm`;
    
    const vertices = geometry.attributes.position.count;
    document.getElementById('model-vertices').textContent = vertices.toLocaleString();
}

function fitCameraToModel() {
    if (!currentModel) return;

    const box = new THREE.Box3().setFromObject(currentModel);
    const size = box.getSize(new THREE.Vector3());
    const center = box.getCenter(new THREE.Vector3());

    const maxDim = Math.max(size.x, size.y, size.z);
    const fov = camera.fov * (Math.PI / 180);
    let cameraZ = Math.abs(maxDim / 2 / Math.tan(fov / 2));
    cameraZ *= 2; // Add some padding

    camera.position.set(center.x + cameraZ * 0.5, center.y + cameraZ * 0.5, center.z + cameraZ);
    camera.lookAt(center);
    controls.target.copy(center);
    controls.update();
}

function resetCamera() {
    if (currentModel) {
        fitCameraToModel();
    } else {
        camera.position.set(200, 200, 200);
        camera.lookAt(0, 0, 0);
        controls.target.set(0, 0, 0);
        controls.update();
    }
}

function toggleWireframe() {
    if (!currentModel) return;
    
    wireframeMode = !wireframeMode;
    currentModel.material.wireframe = wireframeMode;
}

function toggleGrid() {
    const show = document.getElementById('show-grid').checked;
    gridHelper.visible = show;
}

function toggleAxes() {
    const show = document.getElementById('show-axes').checked;
    axesHelper.visible = show;
}

function clearModel() {
    if (currentModel) {
        scene.remove(currentModel);
        currentModel = null;
        
        document.getElementById('model-name').textContent = 'None loaded';
        document.getElementById('model-size').textContent = '-';
        document.getElementById('model-vertices').textContent = '-';
    }
}

// Tab navigation
function showTab(tabName) {
    // Hide all tabs
    document.querySelectorAll('.tab-content').forEach(tab => {
        tab.classList.remove('active');
    });
    
    // Show selected tab
    document.getElementById(`tab-${tabName}`).classList.add('active');
    
    // Update nav buttons
    document.querySelectorAll('.nav-btn').forEach(btn => {
        btn.classList.remove('active');
    });
    event.target.classList.add('active');
    
    // Initialize viewer when model tab is shown
    if (tabName === 'model' && !scene) {
        setTimeout(initViewer, 100);
    }
    
    // Refresh files when file tab is shown
    if (tabName === 'files') {
        refreshLittleFSFiles();
        refreshSDFiles();
    }
}

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    // Don't auto-init viewer, wait for tab switch
    console.log('3D Viewer ready');
});

// WebSocket Connection
const ws = new WebSocket('ws://' + window.location.hostname + ':81/');

ws.onopen = function() {
    console.log('WebSocket Connected');
    const statusEl = document.getElementById('ws-status');
    if(statusEl) {
        statusEl.innerText = '(Connected)';
        statusEl.style.color = '#4ec9b0';
    }
};

ws.onclose = function() {
    console.log('WebSocket Disconnected');
    const statusEl = document.getElementById('ws-status');
    if(statusEl) {
        statusEl.innerText = '(Disconnected)';
        statusEl.style.color = '#f48771';
    }
};

ws.onmessage = function(event) {
    // Support both JSON status broadcasts and plain text responses (ok:..., warn:...)
    // Avoid attempting JSON.parse on non-JSON strings (which throws) — first
    // quickly inspect the first non-whitespace character; only parse when it
    // looks like an object.
    let parsed = null;
    const dataStr = (event.data || '').toString().trim();
    if (dataStr.startsWith('{')) {
        try {
            parsed = JSON.parse(dataStr);
        } catch (e) {
            // Keep parsed null on parse failure
            parsed = null;
            console.warn('ws.onmessage: JSON parse failed, treating as text:', dataStr);
        }
    }

    // Handle structured responses from the server (type: 'response')
    if (parsed && parsed.type === 'response') {
        const status = parsed.status || 'info';
        const detail = parsed.detail || '';
        const raw = parsed.raw || (status + (detail ? ':' + detail : ''));

        if (status === 'error') {
            console.error('System Error:', detail || raw);
            const statusEl = document.getElementById('ws-status');
            if (statusEl) { statusEl.innerText = '(ERROR: ' + (detail || raw) + ')'; statusEl.style.color = '#f48771'; }
            const haltEl = document.getElementById('halt-reason');
            if (haltEl) haltEl.innerText = detail || raw;
            appendLog('ERROR: ' + (detail || raw));
            return;
        }

        if (status === 'warn') {
            const statusEl = document.getElementById('ws-status');
            if (statusEl) { statusEl.innerText = '(' + raw + ')'; statusEl.style.color = '#f0a500'; }
            appendLog('WARN: ' + raw);
            return;
        }

        if (status === 'ok') {
            const statusEl = document.getElementById('ws-status');
            if (statusEl) { statusEl.innerText = '(' + raw + ')'; statusEl.style.color = '#4ec9b0'; }
            appendLog(raw);
            return;
        }

        // otherwise it's an info/other response — log it
        appendLog(raw);
        return;
    }

    // Job lifecycle events from server (job_event)
    if (parsed && parsed.type === 'job_event') {
        const fname = parsed.filename || '(unknown)';
        const ev = parsed.event || '(?)';
        // Update global job UI elements if present (both left and right panels)
        const jf = document.getElementById('job-filename'); if (jf) jf.innerText = fname;
        const js = document.getElementById('job-state'); if (js) js.innerText = ev;
        const jfLeft = document.getElementById('job-filename-left'); if (jfLeft) jfLeft.innerText = fname;
        const jsLeft = document.getElementById('job-state-left'); if (jsLeft) jsLeft.innerText = ev;
        appendLog(`JobEvent: ${ev} ${fname}`);
        return;
    }

    if (parsed && parsed.x !== undefined) {
        // JSON status message
        document.getElementById('pos-x').innerText = Number(parsed.x).toFixed(2);
        document.getElementById('pos-y').innerText = Number(parsed.y).toFixed(2);
        document.getElementById('pos-z').innerText = Number(parsed.z).toFixed(2);
        document.getElementById('pos-e').innerText = Number(parsed.e).toFixed(2);

        // Update temperatures if present
        if (parsed.extTemp !== undefined) {
            document.getElementById('temp-ext').innerText = Number(parsed.extTemp).toFixed(1);
        }
        if (parsed.bedTemp !== undefined) {
            document.getElementById('temp-bed').innerText = Number(parsed.bedTemp).toFixed(1);
        }

        // Executor state
        const busyEl = document.getElementById('exec-busy');
        const ownerEl = document.getElementById('exec-owner');
        if (busyEl) busyEl.innerText = parsed.executor_busy ? 'busy' : 'idle';
        if (ownerEl) ownerEl.innerText = `(owner ${parsed.executor_owner_type}:${parsed.executor_owner_id})`;

        // Motor outputs diagnostics (if provided)
        if (parsed.motorOutX !== undefined) document.getElementById('mot-x').innerText = parsed.motorOutX;
        if (parsed.motorOutY !== undefined) document.getElementById('mot-y').innerText = parsed.motorOutY;
        if (parsed.motorOutZ !== undefined) document.getElementById('mot-z').innerText = parsed.motorOutZ;
        if (parsed.motorOutE !== undefined) document.getElementById('mot-e').innerText = parsed.motorOutE;

        // Raw encoder counts (debug)
        if (parsed.encCountsX !== undefined) document.getElementById('enc-x').innerText = parsed.encCountsX;
        if (parsed.encCountsY !== undefined) document.getElementById('enc-y').innerText = parsed.encCountsY;
        if (parsed.encCountsZ !== undefined) document.getElementById('enc-z').innerText = parsed.encCountsZ;
        if (parsed.encCountsE !== undefined) document.getElementById('enc-e').innerText = parsed.encCountsE;

        // Update halt reason if provided
        if (parsed.error) {
            const haltEl = document.getElementById('halt-reason');
            if (haltEl) haltEl.innerText = parsed.error;
        }

        // Update Visualizer Toolhead
        updateVisualizer(parsed.x, parsed.y, parsed.z);

        // Show setpoints and last movement ages
        if (parsed.setX !== undefined) document.getElementById('set-x').innerText = Number(parsed.setX).toFixed(2);
        if (parsed.setY !== undefined) document.getElementById('set-y').innerText = Number(parsed.setY).toFixed(2);
        if (parsed.setZ !== undefined) document.getElementById('set-z').innerText = Number(parsed.setZ).toFixed(2);
        if (parsed.setE !== undefined) document.getElementById('set-e').innerText = Number(parsed.setE).toFixed(2);

        if (parsed.lastMoveX_ms !== undefined) document.getElementById('lastmove-x').innerText = parsed.lastMoveX_ms + 'ms';
        if (parsed.lastMoveY_ms !== undefined) document.getElementById('lastmove-y').innerText = parsed.lastMoveY_ms + 'ms';
        if (parsed.lastMoveZ_ms !== undefined) document.getElementById('lastmove-z').innerText = parsed.lastMoveZ_ms + 'ms';
        if (parsed.lastMoveE_ms !== undefined) document.getElementById('lastmove-e').innerText = parsed.lastMoveE_ms + 'ms';

        return;
    }

    // If not JSON, treat as one-line response
    const line = dataStr;
    appendLog(line.trim());
    // If the line indicates an error/halt or warn, reflect in UI
    if (line.startsWith('error:')) {
        const statusEl = document.getElementById('ws-status');
        if(statusEl) { statusEl.innerText = '(' + line + ')'; statusEl.style.color = '#f48771'; }
        const haltEl = document.getElementById('halt-reason');
        if (haltEl) haltEl.innerText = line.substring(6);
    } else if (line.startsWith('warn:')) {
        // warnings are transient but shown in the log
        const statusEl = document.getElementById('ws-status');
        if(statusEl) { statusEl.innerText = '(' + line + ')'; statusEl.style.color = '#f0a500'; }
    } else if (line.startsWith('ok:')) {
        const statusEl = document.getElementById('ws-status');
        if(statusEl) { statusEl.innerText = '(' + line + ')'; statusEl.style.color = '#4ec9b0'; }
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

// Configuration API
function populateConfig(doc) {
    if (!doc) return;
    document.getElementById('cfg-cpm-x').value = Number(doc.countsPerMM.x).toFixed(4);
    document.getElementById('cfg-cpm-y').value = Number(doc.countsPerMM.y).toFixed(4);
    document.getElementById('cfg-cpm-z').value = Number(doc.countsPerMM.z).toFixed(4);
    document.getElementById('cfg-cpm-e').value = Number(doc.countsPerMM.e).toFixed(4);

    if (doc.pid) {
        if (doc.pid.x) { document.getElementById('cfg-pid-x-p').value = Number(doc.pid.x.p); document.getElementById('cfg-pid-x-i').value = Number(doc.pid.x.i); document.getElementById('cfg-pid-x-d').value = Number(doc.pid.x.d); }
        if (doc.pid.y) { document.getElementById('cfg-pid-y-p').value = Number(doc.pid.y.p); document.getElementById('cfg-pid-y-i').value = Number(doc.pid.y.i); document.getElementById('cfg-pid-y-d').value = Number(doc.pid.y.d); }
        if (doc.pid.z) { document.getElementById('cfg-pid-z-p').value = Number(doc.pid.z.p); document.getElementById('cfg-pid-z-i').value = Number(doc.pid.z.i); document.getElementById('cfg-pid-z-d').value = Number(doc.pid.z.d); }
        if (doc.pid.e) { document.getElementById('cfg-pid-e-p').value = Number(doc.pid.e.p); document.getElementById('cfg-pid-e-i').value = Number(doc.pid.e.i); document.getElementById('cfg-pid-e-d').value = Number(doc.pid.e.d); }
    }
    if (doc.maxFeedrate) {
        document.getElementById('cfg-maxf-x').value = Number(doc.maxFeedrate.x);
        document.getElementById('cfg-maxf-y').value = Number(doc.maxFeedrate.y);
        document.getElementById('cfg-maxf-z').value = Number(doc.maxFeedrate.z);
        document.getElementById('cfg-maxf-e').value = Number(doc.maxFeedrate.e);
    }
}

function loadConfig() {
    fetch('/api/config').then(r => r.json()).then(doc => {
        populateConfig(doc);
        appendLog('Config loaded');
    }).catch(e => { appendLog('Config load failed'); console.error(e); });
}

function saveConfig() {
    const body = {
        countsPerMM: {
            x: parseFloat(document.getElementById('cfg-cpm-x').value),
            y: parseFloat(document.getElementById('cfg-cpm-y').value),
            z: parseFloat(document.getElementById('cfg-cpm-z').value),
            e: parseFloat(document.getElementById('cfg-cpm-e').value)
        },
        pid: {
            x: { p: parseFloat(document.getElementById('cfg-pid-x-p').value), i: parseFloat(document.getElementById('cfg-pid-x-i').value), d: parseFloat(document.getElementById('cfg-pid-x-d').value) },
            y: { p: parseFloat(document.getElementById('cfg-pid-y-p').value), i: parseFloat(document.getElementById('cfg-pid-y-i').value), d: parseFloat(document.getElementById('cfg-pid-y-d').value) },
            z: { p: parseFloat(document.getElementById('cfg-pid-z-p').value), i: parseFloat(document.getElementById('cfg-pid-z-i').value), d: parseFloat(document.getElementById('cfg-pid-z-d').value) },
            e: { p: parseFloat(document.getElementById('cfg-pid-e-p').value), i: parseFloat(document.getElementById('cfg-pid-e-i').value), d: parseFloat(document.getElementById('cfg-pid-e-d').value) }
        },
        maxFeedrate: {
            x: parseInt(document.getElementById('cfg-maxf-x').value),
            y: parseInt(document.getElementById('cfg-maxf-y').value),
            z: parseInt(document.getElementById('cfg-maxf-z').value),
            e: parseInt(document.getElementById('cfg-maxf-e').value)
        }
    };
    fetch('/api/config', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) })
        .then(r => r.json()).then(j => { appendLog('Config saved'); }).catch(e => { appendLog('Config save failed'); console.error(e); });
}

// Load config on startup
// Run / Spindle / Laser controls
function setRunAction(action) {
    fetch(`/api/run/${action}`, { method: 'POST' }).then(r => r.json()).then(j => {
        appendLog(`Run ${action}`);
    }).catch(e => { appendLog('Run action failed'); console.error(e); });
}

function setRunSpeed(percent) {
    const body = { speedPercent: parseInt(percent) };
    fetch('/api/run/speed', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) })
        .then(r => r.json()).then(j => { appendLog('Run speed set'); })
        .catch(e => { appendLog('Set speed failed'); console.error(e); });
}

function setSpindlePower(val) {
    const body = { power: parseInt(val) };
    fetch('/api/spindle', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) })
        .then(r => r.json()).then(j => { appendLog('Spindle set'); })
        .catch(e => { appendLog('Spindle set failed'); console.error(e); });
}

function setLaserPower(val) {
    const body = { power: parseInt(val) };
    fetch('/api/laser', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) })
        .then(r => r.json()).then(j => { appendLog('Laser set'); })
        .catch(e => { appendLog('Laser set failed'); console.error(e); });
}

// Initialize Run/Spindle/Laser UI bindings
window.addEventListener('load', function() {
    // Run buttons
    const btnPlay = document.getElementById('btn-play'); if (btnPlay) btnPlay.addEventListener('click', () => setRunAction('play'));
    const btnPause = document.getElementById('btn-pause'); if (btnPause) btnPause.addEventListener('click', () => setRunAction('pause'));
    const btnStop = document.getElementById('btn-stop'); if (btnStop) btnStop.addEventListener('click', () => setRunAction('stop'));
    const speed = document.getElementById('run-speed'); const speedVal = document.getElementById('run-speed-val');
    if (speed) { speed.addEventListener('input', (e) => { if (speedVal) speedVal.innerText = e.target.value + '%'; }); speed.addEventListener('change', (e) => setRunSpeed(e.target.value)); }

    // Spindle / Laser
    const spd = document.getElementById('spindle-power'); const spdVal = document.getElementById('spindle-val');
    if (spd) { spd.addEventListener('input', (e) => { if (spdVal) spdVal.innerText = e.target.value; }); spd.addEventListener('change', (e) => setSpindlePower(e.target.value)); }
    const spdOff = document.getElementById('spindle-off'); if (spdOff) spdOff.addEventListener('click', () => { if (spd) { spd.value = 0; if (spdVal) spdVal.innerText = '0'; setSpindlePower(0); } });

    const lsr = document.getElementById('laser-power'); const lsrVal = document.getElementById('laser-val');
    if (lsr) { lsr.addEventListener('input', (e) => { if (lsrVal) lsrVal.innerText = e.target.value; }); lsr.addEventListener('change', (e) => setLaserPower(e.target.value)); }
    const lsrOff = document.getElementById('laser-off'); if (lsrOff) lsrOff.addEventListener('click', () => { if (lsr) { lsr.value = 0; if (lsrVal) lsrVal.innerText = '0'; setLaserPower(0); } });

    // Fetch initial states
    fetch('/api/spindle').then(r=>r.json()).then(j=>{ if (j && j.power !== undefined && spd) { spd.value = j.power; if (spdVal) spdVal.innerText = j.power; } }).catch(()=>{});
    fetch('/api/laser').then(r=>r.json()).then(j=>{ if (j && j.power !== undefined && lsr) { lsr.value = j.power; if (lsrVal) lsrVal.innerText = j.power; } }).catch(()=>{});
    fetch('/api/run').then(r=>r.json()).then(j=>{ if (j && j.speedPercent !== undefined && speed) { speed.value = j.speedPercent; if (speedVal) speedVal.innerText = j.speedPercent + '%'; } }).catch(()=>{});
});

function appendLog(msg) {
    const log = document.getElementById('msg-log');
    if (!log) return;
    const now = new Date().toLocaleTimeString();
    log.innerText = now + '  ' + msg + '\n' + log.innerText;
}

// --- Three.js Visualizer ---
let scene, camera, renderer, toolhead, controls;
let gcodePreviewGroup = null;
let currentGCodeData = null;

// Object management
let loadedObjects = [];
let selectedObject = null;
let objectIdCounter = 0;

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

    // Controls
    if (typeof THREE.OrbitControls !== 'undefined') {
        controls = new THREE.OrbitControls(camera, renderer.domElement);
        controls.enableDamping = true;
        controls.dampingFactor = 0.25;
        controls.enableZoom = true;
    }
    
    // Toolhead (small indicator)
    const geometry = new THREE.ConeGeometry(3, 12, 16);
    const material = new THREE.MeshBasicMaterial({ color: 0xff0000, transparent: true, opacity: 0.7 });
    toolhead = new THREE.Mesh(geometry, material);
    toolhead.rotation.x = Math.PI;
    toolhead.visible = true;
    scene.add(toolhead);
    
    // Lights
    const ambientLight = new THREE.AmbientLight(0x606060);
    scene.add(ambientLight);
    const dirLight = new THREE.DirectionalLight(0xffffff, 0.8);
    dirLight.position.set(100, 200, 100);
    scene.add(dirLight);
    
    animate();
    
    // Handle Window Resize
    window.addEventListener('resize', onWindowResize, false);
}

function onWindowResize() {
    const container = document.getElementById('viewer-container');
    if (camera && renderer && container) {
        camera.aspect = container.clientWidth / container.clientHeight;
        camera.updateProjectionMatrix();
        renderer.setSize(container.clientWidth, container.clientHeight);
    }
}

function updateVisualizer(x, y, z) {
    if (toolhead) {
        toolhead.position.set(x, z, y); // Swap Y/Z for 3D space
    }
}

function animate() {
    requestAnimationFrame(animate);
    if (controls) controls.update();
    renderer.render(scene, camera);
}

// Initialize on load
window.addEventListener('load', initVisualizer);

// Handle Resize
window.addEventListener('resize', () => {
    const container = document.getElementById('viewer-container');
    camera.aspect = container.clientWidth / container.clientHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(container.clientWidth, container.clientHeight);
});

// --- Object Management Functions ---
function addObjectToScene(mesh, filename, geometry) {
    const obj = {
        id: objectIdCounter++,
        mesh: mesh,
        geometry: geometry,
        filename: filename,
        transform: {
            position: { x: 0, y: 0, z: 0 },
            rotation: { x: 0, y: 0, z: 0 },
            scale: 1
        }
    };
    loadedObjects.push(obj);
    scene.add(mesh);
    updateObjectList();
    
    // Enable slice button if it exists
    const sliceBtn = document.getElementById('slice-btn');
    if (sliceBtn) sliceBtn.disabled = false;
    
    return obj;
}

function removeObject(obj) {
    scene.remove(obj.mesh);
    const index = loadedObjects.indexOf(obj);
    if (index > -1) {
        loadedObjects.splice(index, 1);
    }
    if (selectedObject === obj) {
        selectedObject = null;
        document.getElementById('object-transform').style.display = 'none';
    }
    updateObjectList();
}

function selectObject(obj) {
    selectedObject = obj;
    updateObjectList();
    
    // Show transform panel
    document.getElementById('object-transform').style.display = 'block';
    document.getElementById('selected-obj-name').innerText = obj.filename;
    
    // Update transform inputs
    document.getElementById('obj-pos-x').value = obj.transform.position.x;
    document.getElementById('obj-pos-y').value = obj.transform.position.y;
    document.getElementById('obj-pos-z').value = obj.transform.position.z;
    document.getElementById('obj-rot-x').value = Math.round(obj.transform.rotation.x * 180 / Math.PI);
    document.getElementById('obj-rot-y').value = Math.round(obj.transform.rotation.y * 180 / Math.PI);
    document.getElementById('obj-rot-z').value = Math.round(obj.transform.rotation.z * 180 / Math.PI);
    document.getElementById('obj-scale').value = obj.transform.scale * 100;
}

function applyTransform() {
    if (!selectedObject) return;
    
    const posX = parseFloat(document.getElementById('obj-pos-x').value) || 0;
    const posY = parseFloat(document.getElementById('obj-pos-y').value) || 0;
    const posZ = parseFloat(document.getElementById('obj-pos-z').value) || 0;
    
    const rotX = (parseFloat(document.getElementById('obj-rot-x').value) || 0) * Math.PI / 180;
    const rotY = (parseFloat(document.getElementById('obj-rot-y').value) || 0) * Math.PI / 180;
    const rotZ = (parseFloat(document.getElementById('obj-rot-z').value) || 0) * Math.PI / 180;
    
    const scale = (parseFloat(document.getElementById('obj-scale').value) || 100) / 100;
    
    selectedObject.transform.position = { x: posX, y: posY, z: posZ };
    selectedObject.transform.rotation = { x: rotX, y: rotY, z: rotZ };
    selectedObject.transform.scale = scale;
    
    selectedObject.mesh.position.set(posX, posZ, posY);
    selectedObject.mesh.rotation.set(rotX, rotY, rotZ);
    selectedObject.mesh.scale.set(scale, scale, scale);
}

function resetTransform() {
    if (!selectedObject) return;
    document.getElementById('obj-pos-x').value = 0;
    document.getElementById('obj-pos-y').value = 0;
    document.getElementById('obj-pos-z').value = 0;
    document.getElementById('obj-rot-x').value = 0;
    document.getElementById('obj-rot-y').value = 0;
    document.getElementById('obj-rot-z').value = 0;
    document.getElementById('obj-scale').value = 100;
    applyTransform();
}

function updateObjectList() {
    const listDiv = document.getElementById('object-list');
    if (!listDiv) return;
    
    listDiv.innerHTML = '';
    
    if (loadedObjects.length === 0) {
        listDiv.innerHTML = '<div style="color: #999; font-size: 0.8em; padding: 8px;">No objects loaded</div>';
        return;
    }
    
    loadedObjects.forEach(obj => {
        const item = document.createElement('div');
        item.className = 'object-item' + (selectedObject === obj ? ' selected' : '');
        item.onclick = () => selectObject(obj);
        
        const name = document.createElement('span');
        name.innerText = obj.filename;
        name.style.flexGrow = '1';
        
        const actions = document.createElement('div');
        actions.className = 'object-item-actions';
        
        const delBtn = document.createElement('button');
        delBtn.innerText = '×';
        delBtn.onclick = (e) => {
            e.stopPropagation();
            removeObject(obj);
        };
        
        actions.appendChild(delBtn);
        item.appendChild(name);
        item.appendChild(actions);
        listDiv.appendChild(item);
    });
}

function addNewSTL() {
    document.getElementById('stl-upload').click();
}

// View toggle functions
function toggleToolheadView() {
    const show = document.getElementById('show-toolhead').checked;
    if (toolhead) toolhead.visible = show;
}

function toggleGCodeView() {
    const show = document.getElementById('show-gcode-view').checked;
    if (gcodePreviewGroup) gcodePreviewGroup.visible = show;
}

function toggleObjectsView() {
    const show = document.getElementById('show-objects').checked;
    loadedObjects.forEach(obj => {
        obj.mesh.visible = show;
    });
}

// --- STL Loading Functions ---
function loadSTL() {
    const fileInput = document.getElementById('stl-upload');
    const file = fileInput.files[0];
    if (!file) {
        alert('Please select an STL file first');
        return;
    }

    appendLog('Loading STL: ' + file.name + '...');
    
    const reader = new FileReader();
    reader.onload = function(e) {
        try {
            const loader = new THREE.STLLoader();
            const geometry = loader.parse(e.target.result);
            
            // Compute normals for proper lighting
            geometry.computeVertexNormals();
            geometry.computeBoundingBox();
            
            // Create mesh with better material
            const material = new THREE.MeshPhongMaterial({ 
                color: 0x4a90e2, 
                specular: 0x222222, 
                shininess: 30,
                flatShading: false,
                side: THREE.DoubleSide
            });
            const mesh = new THREE.Mesh(geometry, material);
            
            // Center and position the model
            const bbox = geometry.boundingBox;
            const center = new THREE.Vector3();
            bbox.getCenter(center);
            geometry.translate(-center.x, -center.y, -bbox.min.z); // Place on build plate
            
            // Add to scene as a new object
            const obj = addObjectToScene(mesh, file.name, geometry);
            selectObject(obj);
            
            // Update info in right panel
            const size = new THREE.Vector3();
            bbox.getSize(size);
            const triangles = geometry.attributes.position.count / 3;
            document.getElementById('stl-info').innerHTML = 
                `<strong>${file.name}</strong><br>` +
                `Size: ${size.x.toFixed(1)} × ${size.y.toFixed(1)} × ${size.z.toFixed(1)} mm<br>` +
                `Triangles: ${triangles.toLocaleString()}<br>` +
                `Volume: ${calculateVolume(geometry).toFixed(2)} cm³`;
            
            // Adjust camera to view the model
            const maxDim = Math.max(size.x, size.y, size.z);
            camera.position.set(maxDim * 1.5, maxDim * 1.5, maxDim * 1.5);
            camera.lookAt(0, size.z / 2, 0);
            if (controls) controls.target.set(0, size.z / 2, 0);
            
            appendLog('STL loaded: ' + file.name + ' (' + triangles.toLocaleString() + ' triangles)');
            
            // Clear file input to allow loading same file again
            fileInput.value = '';
        } catch (error) {
            appendLog('ERROR loading STL: ' + error.message);
            console.error('STL Load Error:', error);
            alert('Failed to load STL file: ' + error.message);
        }
    };
    
    reader.onerror = function() {
        appendLog('ERROR: Failed to read STL file');
        alert('Failed to read file');
    };
    
    reader.readAsArrayBuffer(file);
}

function calculateVolume(geometry) {
    // Simple volume calculation for STL
    const positions = geometry.attributes.position.array;
    let volume = 0;
    for (let i = 0; i < positions.length; i += 9) {
        const v1 = new THREE.Vector3(positions[i], positions[i+1], positions[i+2]);
        const v2 = new THREE.Vector3(positions[i+3], positions[i+4], positions[i+5]);
        const v3 = new THREE.Vector3(positions[i+6], positions[i+7], positions[i+8]);
        volume += v1.dot(new THREE.Vector3().crossVectors(v2, v3)) / 6;
    }
    return Math.abs(volume) / 1000; // Convert mm³ to cm³
}

// --- Slicing Functions ---
function sliceModel() {
    if (loadedObjects.length === 0) {
        alert('Please load at least one STL file first');
        return;
    }
    
    const sliceBtn = document.getElementById('slice-btn');
    const progressDiv = document.getElementById('slice-progress');
    
    sliceBtn.disabled = true;
    progressDiv.innerText = 'Slicing ' + loadedObjects.length + ' object(s)...';
    
    // Get slicer settings
    const settings = {
        layerHeight: parseFloat(document.getElementById('slice-layer-height').value),
        infill: parseInt(document.getElementById('slice-infill').value),
        printSpeed: parseInt(document.getElementById('slice-speed').value),
        nozzleTemp: parseInt(document.getElementById('slice-temp').value),
        bedTemp: parseInt(document.getElementById('slice-bed-temp').value)
    };
    
    // Simple slicing simulation - combine all objects
    setTimeout(() => {
        const gcode = generateCombinedGCode(loadedObjects, settings);
        currentGCodeData = {
            filename: 'multi_object.gcode',
            content: gcode,
            lines: gcode.split('\\n').length
        };
        
        // Update GCode info
        document.getElementById('gcode-filename').innerText = currentGCodeData.filename;
        document.getElementById('gcode-lines').innerText = currentGCodeData.lines;
        document.getElementById('gcode-time').innerText = estimatePrintTime(gcode);
        
        // Enable preview and print buttons
        document.getElementById('preview-btn').disabled = false;
        document.getElementById('print-btn').disabled = false;
        
        progressDiv.innerText = 'Slicing complete!';
        sliceBtn.disabled = false;
        
        appendLog('Slicing complete: ' + currentGCodeData.lines + ' lines');
    }, 1000);
}

function generateCombinedGCode(objects, settings) {
    let gcode = '; Generated by Encoder3D - Multi-object print\n';
    gcode += `; Objects: ${objects.length}\n`;
    gcode += `; Layer height: ${settings.layerHeight}mm\n`;
    gcode += `; Infill: ${settings.infill}%\n`;
    gcode += '; Start GCode\n';
    gcode += `M104 S${settings.nozzleTemp} ; Set nozzle temp\n`;
    gcode += `M140 S${settings.bedTemp} ; Set bed temp\n`;
    gcode += 'G28 ; Home all axes\n';
    gcode += 'G1 Z5 F5000 ; Lift nozzle\n';
    gcode += 'M109 S' + settings.nozzleTemp + ' ; Wait for nozzle temp\n';
    gcode += 'M190 S' + settings.bedTemp + ' ; Wait for bed temp\n';
    gcode += 'G92 E0 ; Reset extruder\n';
    gcode += 'G1 F' + settings.printSpeed * 60 + '\n';
    
    let totalE = 0;
    
    // Generate simple GCode for each object based on its transform
    objects.forEach((obj, objIndex) => {
        const geom = obj.geometry;
        const bbox = geom.boundingBox.clone();
        const size = new THREE.Vector3();
        bbox.getSize(size);
        
        // Apply object transforms to bounding box
        const pos = obj.transform.position;
        const scale = obj.transform.scale;
        
        const minX = (bbox.min.x * scale) + pos.x;
        const maxX = (bbox.max.x * scale) + pos.x;
        const minY = (bbox.min.y * scale) + pos.y;
        const maxY = (bbox.max.y * scale) + pos.y;
        const minZ = (bbox.min.z * scale) + pos.z;
        const maxZ = (bbox.max.z * scale) + pos.z;
        
        gcode += `; Object ${objIndex + 1}: ${obj.filename}\n`;
        
        const numLayers = Math.ceil((maxZ - minZ) / settings.layerHeight);
        for (let layer = 0; layer < Math.min(numLayers, 5); layer++) {
            const z = minZ + (layer * settings.layerHeight);
            gcode += `; Layer ${layer}\n`;
            gcode += `G1 Z${z.toFixed(3)} F300\n`;
            
            // Simple perimeter
            gcode += `G1 X${minX.toFixed(2)} Y${minY.toFixed(2)} E${totalE.toFixed(3)}\n`;
            totalE += 1;
            gcode += `G1 X${maxX.toFixed(2)} Y${minY.toFixed(2)} E${totalE.toFixed(3)}\n`;
            totalE += 1;
            gcode += `G1 X${maxX.toFixed(2)} Y${maxY.toFixed(2)} E${totalE.toFixed(3)}\n`;
            totalE += 1;
            gcode += `G1 X${minX.toFixed(2)} Y${maxY.toFixed(2)} E${totalE.toFixed(3)}\n`;
            totalE += 1;
            gcode += `G1 X${minX.toFixed(2)} Y${minY.toFixed(2)} E${totalE.toFixed(3)}\n`;
            totalE += 1;
        }
    });
    
    gcode += '; End GCode\n';
    gcode += 'M104 S0 ; Turn off nozzle\n';
    gcode += 'M140 S0 ; Turn off bed\n';
    gcode += 'G28 X0 ; Home X\n';
    gcode += 'M84 ; Disable motors\n';
    
    return gcode;
}

function generateSimpleGCode(geometry, settings) {
    // Simple GCode generator (placeholder - replace with real slicer)
    const bbox = geometry.boundingBox;
    const size = new THREE.Vector3();
    bbox.getSize(size);
    
    let gcode = '; Generated by Encoder3D\\n';
    gcode += `; Layer height: ${settings.layerHeight}mm\\n`;
    gcode += `; Infill: ${settings.infill}%\\n`;
    gcode += '; Start GCode\\n';
    gcode += `M104 S${settings.nozzleTemp} ; Set nozzle temp\\n`;
    gcode += `M140 S${settings.bedTemp} ; Set bed temp\\n`;
    gcode += 'G28 ; Home all axes\\n';
    gcode += 'G1 Z5 F5000 ; Lift nozzle\\n';
    gcode += 'M109 S' + settings.nozzleTemp + ' ; Wait for nozzle temp\\n';
    gcode += 'M190 S' + settings.bedTemp + ' ; Wait for bed temp\\n';
    gcode += 'G92 E0 ; Reset extruder\\n';
    gcode += 'G1 F' + settings.printSpeed * 60 + '\\n';
    
    // Generate simple layer-by-layer perimeter (very basic)
    const numLayers = Math.ceil(size.z / settings.layerHeight);
    for (let layer = 0; layer < Math.min(numLayers, 10); layer++) {
        const z = layer * settings.layerHeight;
        gcode += `; Layer ${layer}\\n`;
        gcode += `G1 Z${z.toFixed(3)} F300\\n`;
        // Simple square perimeter
        gcode += `G1 X0 Y0 E${(layer * 5).toFixed(3)}\\n`;
        gcode += `G1 X${size.x.toFixed(2)} Y0 E${(layer * 5 + 1).toFixed(3)}\\n`;
        gcode += `G1 X${size.x.toFixed(2)} Y${size.y.toFixed(2)} E${(layer * 5 + 2).toFixed(3)}\\n`;
        gcode += `G1 X0 Y${size.y.toFixed(2)} E${(layer * 5 + 3).toFixed(3)}\\n`;
        gcode += `G1 X0 Y0 E${(layer * 5 + 4).toFixed(3)}\\n`;
    }
    
    gcode += '; End GCode\\n';
    gcode += 'M104 S0 ; Turn off nozzle\\n';
    gcode += 'M140 S0 ; Turn off bed\\n';
    gcode += 'G28 X0 ; Home X\\n';
    gcode += 'M84 ; Disable motors\\n';
    
    return gcode;
}

function estimatePrintTime(gcode) {
    // Simple time estimation
    const lines = gcode.split('\\n').filter(l => l.startsWith('G1'));
    const minutes = Math.ceil(lines.length / 20);
    if (minutes < 60) return minutes + ' min';
    return Math.floor(minutes / 60) + 'h ' + (minutes % 60) + 'min';
}

// --- GCode Loading and Preview Functions ---
function loadGCodeFile() {
    const fileInput = document.getElementById('gcode-file-input');
    const file = fileInput.files[0];
    if (!file) {
        alert('Please select a GCode file first');
        return;
    }

    const reader = new FileReader();
    reader.onload = function(e) {
        const content = e.target.result;
        currentGCodeData = {
            filename: file.name,
            content: content,
            lines: content.split('\\n').length
        };
        
        // Update info
        document.getElementById('gcode-filename').innerText = file.name;
        document.getElementById('gcode-lines').innerText = currentGCodeData.lines;
        document.getElementById('gcode-time').innerText = estimatePrintTime(content);
        
        // Enable buttons
        document.getElementById('preview-btn').disabled = false;
        document.getElementById('print-btn').disabled = false;
        
        appendLog('GCode loaded: ' + file.name);
    };
    
    reader.readAsText(file);
}

function toggleGCodePreview() {
    if (gcodePreviewGroup) {
        // Hide preview
        scene.remove(gcodePreviewGroup);
        gcodePreviewGroup = null;
        document.getElementById('preview-btn').innerText = 'Show Preview';
        document.getElementById('layer-slider').disabled = true;
    } else {
        // Show preview
        if (!currentGCodeData) {
            alert('Please load a GCode file first');
            return;
        }
        renderGCodePreview(currentGCodeData.content);
        document.getElementById('preview-btn').innerText = 'Hide Preview';
    }
}

function renderGCodePreview(gcode) {
    gcodePreviewGroup = new THREE.Group();
    
    const lines = gcode.split('\\n');
    let x = 0, y = 0, z = 0;
    let lastX = 0, lastY = 0, lastZ = 0;
    let extruding = false;
    
    const showMoves = document.getElementById('show-moves').checked;
    const showExtrusions = document.getElementById('show-extrusions').checked;
    
    const travelMaterial = new THREE.LineBasicMaterial({ color: 0x00ff00, opacity: 0.3, transparent: true });
    const extrusionMaterial = new THREE.LineBasicMaterial({ color: 0xff8800, linewidth: 2 });
    
    const travelPoints = [];
    const extrusionPoints = [];
    
    for (let line of lines) {
        line = line.trim().split(';')[0]; // Remove comments
        if (!line.startsWith('G')) continue;
        
        const parts = line.split(' ');
        const cmd = parts[0];
        
        if (cmd === 'G0' || cmd === 'G1') {
            let hasE = false;
            for (let part of parts.slice(1)) {
                if (part.startsWith('X')) x = parseFloat(part.substring(1));
                if (part.startsWith('Y')) y = parseFloat(part.substring(1));
                if (part.startsWith('Z')) z = parseFloat(part.substring(1));
                if (part.startsWith('E')) hasE = true;
            }
            
            const points = [
                new THREE.Vector3(lastX, lastZ, lastY),
                new THREE.Vector3(x, z, y)
            ];
            
            if (hasE && showExtrusions) {
                extrusionPoints.push(...points);
            } else if (!hasE && showMoves) {
                travelPoints.push(...points);
            }
            
            lastX = x;
            lastY = y;
            lastZ = z;
        }
    }
    
    // Create line geometries
    if (travelPoints.length > 0) {
        const travelGeometry = new THREE.BufferGeometry().setFromPoints(travelPoints);
        const travelLine = new THREE.LineSegments(travelGeometry, travelMaterial);
        gcodePreviewGroup.add(travelLine);
    }
    
    if (extrusionPoints.length > 0) {
        const extrusionGeometry = new THREE.BufferGeometry().setFromPoints(extrusionPoints);
        const extrusionLine = new THREE.LineSegments(extrusionGeometry, extrusionMaterial);
        gcodePreviewGroup.add(extrusionLine);
    }
    
    scene.add(gcodePreviewGroup);
    
    // Enable layer slider (simplified - just showing all for now)
    document.getElementById('layer-slider').disabled = false;
    document.getElementById('layer-slider').max = 100;
    document.getElementById('layer-num').innerText = 'All';
}

// --- Print Functions ---
function uploadAndPrint() {
    if (!currentGCodeData) {
        alert('Please load or slice a file first');
        return;
    }
    
    const blob = new Blob([currentGCodeData.content], { type: 'text/plain' });
    const formData = new FormData();
    formData.append('file', blob, currentGCodeData.filename);
    
    fetch('/api/upload', {
        method: 'POST',
        body: formData
    })
    .then(response => response.text())
    .then(result => {
        appendLog('Upload complete: ' + currentGCodeData.filename);
        document.getElementById('job-state').innerText = 'printing';
        document.getElementById('job-filename').innerText = currentGCodeData.filename;
        alert('Upload & Print started');
    })
    .catch(error => {
        appendLog('Upload failed: ' + error);
        console.error('Error:', error);
    });
}

function printFromServer() {
    // Trigger print of file already on server
    const filename = prompt('Enter filename on server (e.g., test.gcode):');
    if (!filename) return;
    
    fetch('/api/print', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ filename: filename })
    })
    .then(response => response.json())
    .then(result => {
        appendLog('Print started: ' + filename);
        document.getElementById('job-state').innerText = 'printing';
        document.getElementById('job-filename').innerText = filename;
    })
    .catch(error => {
        appendLog('Print failed: ' + error);
        console.error('Error:', error);
    });
}

// Update preview when checkboxes change
window.addEventListener('load', function() {
    const showMoves = document.getElementById('show-moves');
    const showExtrusions = document.getElementById('show-extrusions');
    
    if (showMoves) {
        showMoves.addEventListener('change', () => {
            if (gcodePreviewGroup && currentGCodeData) {
                scene.remove(gcodePreviewGroup);
                gcodePreviewGroup = null;
                renderGCodePreview(currentGCodeData.content);
            }
        });
    }
    
    if (showExtrusions) {
        showExtrusions.addEventListener('change', () => {
            if (gcodePreviewGroup && currentGCodeData) {
                scene.remove(gcodePreviewGroup);
                gcodePreviewGroup = null;
                renderGCodePreview(currentGCodeData.content);
            }
        });
    }
});


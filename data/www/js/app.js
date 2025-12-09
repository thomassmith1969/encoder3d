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

function appendLog(msg) {
    const log = document.getElementById('msg-log');
    if (!log) return;
    const now = new Date().toLocaleTimeString();
    log.innerText = now + '  ' + msg + '\n' + log.innerText;
}

// --- Three.js Visualizer ---
let scene, camera, renderer, toolhead, controls;

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

initVisualizer();

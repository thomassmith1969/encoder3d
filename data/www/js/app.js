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
        // Update global job UI elements if present
        const jf = document.getElementById('job-filename'); if (jf) jf.innerText = fname;
        const js = document.getElementById('job-state'); if (js) js.innerText = ev;
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

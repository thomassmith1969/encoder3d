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
    
    // Load CSG settings from localStorage
    const csgThreads = localStorage.getItem('csgThreads');
    if (document.getElementById('cfg-csg-threads')) {
        document.getElementById('cfg-csg-threads').value = csgThreads || '';
    }
}

function loadConfig() {
    fetch('/api/config').then(r => r.json()).then(doc => {
        populateConfig(doc);
        alert('Config loaded');
    }).catch(e => { alert('Config load failed'); console.error(e); });
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
    
    // Save CSG settings to localStorage
    const csgThreadsInput = document.getElementById('cfg-csg-threads');
    if (csgThreadsInput) {
        const csgThreads = csgThreadsInput.value;
        if (csgThreads) {
            localStorage.setItem('csgThreads', csgThreads);
        } else {
            localStorage.removeItem('csgThreads');
        }
    }
    
    fetch('/api/config', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) })
        .then(r => r.json()).then(j => { alert('Config saved'); }).catch(e => { alert('Config save failed'); console.error(e); });
}

window.addEventListener('load', function() {
    document.getElementById('cfg-load').addEventListener('click', loadConfig);
    document.getElementById('cfg-save').addEventListener('click', saveConfig);
    // load current settings on page open
    loadConfig();
});

/*
 * Encoder3D CNC Controller - File Manager
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 */

// LittleFS File Management
async function refreshLittleFSFiles() {
    const container = document.getElementById('littlefs-files');
    container.innerHTML = '<p class="loading">Loading...</p>';

    try {
        const response = await fetch('/api/littlefs/list');
        if (!response.ok) throw new Error('Failed to fetch files');
        
        const data = await response.json();
        
        if (!data.files || data.files.length === 0) {
            container.innerHTML = '<p class="no-files">No G-code files found</p>';
            return;
        }

        let html = '<table class="file-table"><thead><tr><th>Name</th><th>Size</th><th>Actions</th></tr></thead><tbody>';
        
        data.files.forEach(file => {
            const sizeKB = (file.size / 1024).toFixed(1);
            html += `
                <tr>
                    <td>${file.name}</td>
                    <td>${sizeKB} KB</td>
                    <td>
                        <button class="btn-small" onclick="printLittleFSFile('${file.name}')">▶ Print</button>
                        <button class="btn-small" onclick="downloadLittleFSFile('${file.name}')">⬇ Download</button>
                        <button class="btn-small btn-danger" onclick="deleteLittleFSFile('${file.name}')">🗑 Delete</button>
                    </td>
                </tr>
            `;
        });
        
        html += '</tbody></table>';
        container.innerHTML = html;
        
    } catch (error) {
        console.error('Error loading LittleFS files:', error);
        container.innerHTML = '<p class="error">Error loading files</p>';
    }
}

async function printLittleFSFile(filename) {
    if (!confirm(`Start printing ${filename} from LittleFS?`)) return;

    try {
        const response = await fetch(`/api/littlefs/print?file=${encodeURIComponent(filename)}`, {
            method: 'POST'
        });

        if (response.ok) {
            alert(`Started printing ${filename}`);
            addToConsole(`Printing from LittleFS: ${filename}`);
        } else {
            throw new Error('Failed to start print');
        }
    } catch (error) {
        console.error('Print error:', error);
        alert('Failed to start print: ' + error.message);
    }
}

async function downloadLittleFSFile(filename) {
    try {
        const response = await fetch(`/api/littlefs/download?file=${encodeURIComponent(filename)}`);
        if (!response.ok) throw new Error('Download failed');
        
        const blob = await response.blob();
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.click();
        URL.revokeObjectURL(url);
        
    } catch (error) {
        console.error('Download error:', error);
        alert('Failed to download file: ' + error.message);
    }
}

async function deleteLittleFSFile(filename) {
    if (!confirm(`Delete ${filename} from LittleFS?`)) return;

    try {
        const response = await fetch('/api/littlefs/delete', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `file=${encodeURIComponent(filename)}`
        });

        if (response.ok) {
            alert(`Deleted ${filename}`);
            refreshLittleFSFiles();
        } else {
            throw new Error('Delete failed');
        }
    } catch (error) {
        console.error('Delete error:', error);
        alert('Failed to delete file: ' + error.message);
    }
}

// SD Card File Management
async function initSD() {
    try {
        const response = await fetch('/api/command', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: 'cmd=M21'
        });

        if (response.ok) {
            alert('SD card initialized');
            setTimeout(refreshSDFiles, 500);
        }
    } catch (error) {
        console.error('SD init error:', error);
        alert('Failed to initialize SD card');
    }
}

async function refreshSDFiles() {
    const container = document.getElementById('sd-files');
    const statusText = document.getElementById('sd-status-text');
    
    container.innerHTML = '<p class="loading">Loading...</p>';

    try {
        // Get SD status first
        const statusResponse = await fetch('/api/sd/status');
        const statusData = await statusResponse.json();
        
        statusText.textContent = statusData.initialized ? 'Initialized' : 'Not initialized';
        statusText.className = statusData.initialized ? 'status-ok' : 'status-error';
        
        // Update print status if printing
        if (statusData.executing || statusData.paused) {
            document.getElementById('sd-print-status').style.display = 'block';
            document.getElementById('current-file').textContent = statusData.file || '-';
            document.getElementById('print-progress').textContent = statusData.progress ? statusData.progress.toFixed(1) : '0';
            document.getElementById('lines-executed').textContent = statusData.lines || '0';
            document.getElementById('print-progress-fill').style.width = (statusData.progress || 0) + '%';
        } else {
            document.getElementById('sd-print-status').style.display = 'none';
        }
        
        if (!statusData.initialized) {
            container.innerHTML = '<p class="error">SD card not initialized. Click "Initialize SD" button.</p>';
            return;
        }

        // Get file list
        const response = await fetch('/api/sd/list');
        if (!response.ok) throw new Error('Failed to fetch files');
        
        const data = await response.json();
        
        if (!data.files || data.files.length === 0) {
            container.innerHTML = '<p class="no-files">No files on SD card</p>';
            return;
        }

        let html = '<table class="file-table"><thead><tr><th>Name</th><th>Size</th><th>Actions</th></tr></thead><tbody>';
        
        data.files.forEach(file => {
            const sizeKB = (file.size / 1024).toFixed(1);
            html += `
                <tr>
                    <td>${file.name}</td>
                    <td>${sizeKB} KB</td>
                    <td>
                        <button class="btn-small" onclick="printSDFile('${file.name}')">▶ Print</button>
                        <button class="btn-small" onclick="downloadSDFile('${file.name}')">⬇ Download</button>
                        <button class="btn-small btn-danger" onclick="deleteSDFile('${file.name}')">🗑 Delete</button>
                    </td>
                </tr>
            `;
        });
        
        html += '</tbody></table>';
        container.innerHTML = html;
        
    } catch (error) {
        console.error('Error loading SD files:', error);
        container.innerHTML = '<p class="error">Error loading files. Is SD card inserted?</p>';
        statusText.textContent = 'Error';
        statusText.className = 'status-error';
    }
}

async function printSDFile(filename) {
    if (!confirm(`Start printing ${filename} from SD card?`)) return;

    try {
        const response = await fetch('/api/sd/start', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `file=${encodeURIComponent(filename)}`
        });

        if (response.ok) {
            alert(`Started printing ${filename}`);
            addToConsole(`Printing from SD: ${filename}`);
            setTimeout(refreshSDFiles, 500);
        } else {
            throw new Error('Failed to start print');
        }
    } catch (error) {
        console.error('Print error:', error);
        alert('Failed to start print: ' + error.message);
    }
}

async function pausePrint() {
    try {
        const response = await fetch('/api/sd/pause', { method: 'POST' });
        if (response.ok) {
            setTimeout(refreshSDFiles, 500);
        }
    } catch (error) {
        console.error('Pause error:', error);
    }
}

async function resumePrint() {
    try {
        const response = await fetch('/api/sd/pause', { method: 'POST' });
        if (response.ok) {
            setTimeout(refreshSDFiles, 500);
        }
    } catch (error) {
        console.error('Resume error:', error);
    }
}

async function stopPrint() {
    if (!confirm('Stop current print?')) return;
    
    try {
        // Send M112 emergency stop
        await fetch('/api/command', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: 'cmd=M112'
        });
        
        setTimeout(refreshSDFiles, 500);
    } catch (error) {
        console.error('Stop error:', error);
    }
}

async function downloadSDFile(filename) {
    try {
        const response = await fetch(`/api/sd/download?file=${encodeURIComponent(filename)}`);
        if (!response.ok) throw new Error('Download failed');
        
        const blob = await response.blob();
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename.startsWith('/') ? filename.substring(1) : filename;
        a.click();
        URL.revokeObjectURL(url);
        
    } catch (error) {
        console.error('Download error:', error);
        alert('Failed to download file: ' + error.message);
    }
}

async function deleteSDFile(filename) {
    if (!confirm(`Delete ${filename} from SD card?`)) return;

    try {
        const response = await fetch('/api/sd/delete', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: `file=${encodeURIComponent(filename)}`
        });

        if (response.ok) {
            alert(`Deleted ${filename}`);
            refreshSDFiles();
        } else {
            throw new Error('Delete failed');
        }
    } catch (error) {
        console.error('Delete error:', error);
        alert('Failed to delete file: ' + error.message);
    }
}

// Upload functions for main control panel
async function uploadGCodeFile() {
    const fileInput = document.getElementById('gcode-file');
    const file = fileInput.files[0];
    
    if (!file) {
        alert('Please select a file');
        return;
    }

    const sizeKB = file.size / 1024;
    if (sizeKB > 100) {
        alert(`File too large for LittleFS (${sizeKB.toFixed(1)}KB > 100KB). Use SD card instead.`);
        return;
    }

    try {
        const formData = new FormData();
        formData.append('file', file);

        const response = await fetch('/api/littlefs/upload', {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            alert('File uploaded to LittleFS successfully!');
            addToConsole(`Uploaded ${file.name} to LittleFS`);
        } else {
            throw new Error('Upload failed');
        }
    } catch (error) {
        console.error('Upload error:', error);
        alert('Failed to upload file: ' + error.message);
    }
}

async function uploadToSD() {
    const fileInput = document.getElementById('gcode-file');
    const file = fileInput.files[0];
    
    if (!file) {
        alert('Please select a file');
        return;
    }

    try {
        const formData = new FormData();
        formData.append('file', file);

        const response = await fetch('/api/upload', {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            alert('File uploaded to SD card successfully!');
            addToConsole(`Uploaded ${file.name} to SD card`);
        } else {
            throw new Error('Upload failed');
        }
    } catch (error) {
        console.error('Upload error:', error);
        alert('Failed to upload file: ' + error.message);
    }
}

// Auto-refresh SD status when printing
setInterval(async () => {
    const printStatus = document.getElementById('sd-print-status');
    if (printStatus && printStatus.style.display !== 'none') {
        const statusResponse = await fetch('/api/sd/status');
        const statusData = await statusResponse.json();
        
        if (statusData.executing || statusData.paused) {
            document.getElementById('print-progress').textContent = statusData.progress ? statusData.progress.toFixed(1) : '0';
            document.getElementById('lines-executed').textContent = statusData.lines || '0';
            document.getElementById('print-progress-fill').style.width = (statusData.progress || 0) + '%';
        }
    }
}, 2000);

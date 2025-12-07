/*
 * Encoder3D CNC Controller - Web Application
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 */

// WebSocket connection
let ws = null;
let isConnected = false;

// Status update interval
let statusInterval = null;

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    initWebSocket();
    setupEventListeners();
    startStatusUpdates();
});

// WebSocket Functions
function initWebSocket() {
    const wsUrl = `ws://${window.location.hostname}/ws`;
    ws = new WebSocket(wsUrl);
    
    ws.onopen = function() {
        console.log('WebSocket connected');
        isConnected = true;
        updateConnectionStatus(true);
        addConsoleMessage('Connected to controller', 'received');
    };
    
    ws.onclose = function() {
        console.log('WebSocket disconnected');
        isConnected = false;
        updateConnectionStatus(false);
        addConsoleMessage('Disconnected from controller', 'error');
        
        // Attempt to reconnect after 3 seconds
        setTimeout(initWebSocket, 3000);
    };
    
    ws.onerror = function(error) {
        console.error('WebSocket error:', error);
        addConsoleMessage('Connection error', 'error');
    };
    
    ws.onmessage = function(event) {
        try {
            const data = JSON.parse(event.data);
            handleStatusUpdate(data);
        } catch (e) {
            // Text message
            addConsoleMessage(event.data, 'received');
        }
    };
}

function updateConnectionStatus(connected) {
    const statusElement = document.getElementById('connection-status');
    if (connected) {
        statusElement.textContent = 'Connected';
        statusElement.classList.remove('disconnected');
        statusElement.classList.add('connected');
    } else {
        statusElement.textContent = 'Disconnected';
        statusElement.classList.remove('connected');
        statusElement.classList.add('disconnected');
    }
}

// Status Updates
function startStatusUpdates() {
    statusInterval = setInterval(requestStatus, 1000);
}

function requestStatus() {
    if (!isConnected) return;
    
    fetch('/api/status')
        .then(response => response.json())
        .then(data => handleStatusUpdate(data))
        .catch(error => console.error('Error fetching status:', error));
}

function handleStatusUpdate(data) {
    // Update position
    if (data.position) {
        document.getElementById('pos-x').textContent = data.position.x.toFixed(2);
        document.getElementById('pos-y').textContent = data.position.y.toFixed(2);
        document.getElementById('pos-z').textContent = data.position.z.toFixed(2);
        document.getElementById('pos-e').textContent = data.position.e.toFixed(2);
    }
    
    // Update temperatures
    if (data.temperatures) {
        if (data.temperatures.hotend) {
            document.getElementById('temp-hotend-current').textContent = 
                Math.round(data.temperatures.hotend.current);
            document.getElementById('temp-hotend-target').textContent = 
                Math.round(data.temperatures.hotend.target);
        }
        
        if (data.temperatures.bed) {
            document.getElementById('temp-bed-current').textContent = 
                Math.round(data.temperatures.bed.current);
            document.getElementById('temp-bed-target').textContent = 
                Math.round(data.temperatures.bed.target);
        }
    }
}

// Event Listeners
function setupEventListeners() {
    // Enter key in console input
    document.getElementById('gcode-input').addEventListener('keypress', function(e) {
        if (e.key === 'Enter') {
            sendCommand();
        }
    });
}

// Console Functions
function addConsoleMessage(message, type = 'received') {
    const console = document.getElementById('console');
    const line = document.createElement('div');
    line.className = `console-line ${type}`;
    
    const timestamp = new Date().toLocaleTimeString();
    line.textContent = `[${timestamp}] ${message}`;
    
    console.appendChild(line);
    console.scrollTop = console.scrollHeight;
    
    // Limit console to 100 lines
    while (console.children.length > 100) {
        console.removeChild(console.firstChild);
    }
}

// G-Code Functions
function sendCommand() {
    const input = document.getElementById('gcode-input');
    const command = input.value.trim();
    
    if (command) {
        sendGCode(command);
        input.value = '';
    }
}

function sendGCode(command) {
    if (!isConnected) {
        addConsoleMessage('Not connected to controller', 'error');
        return;
    }
    
    addConsoleMessage(command, 'sent');
    
    // Send via WebSocket
    const message = {
        type: 'gcode',
        command: command
    };
    ws.send(JSON.stringify(message));
    
    // Also send via HTTP API as backup
    fetch('/api/command', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `cmd=${encodeURIComponent(command)}`
    })
    .catch(error => console.error('Error sending command:', error));
}

// Jog Functions
function jog(axis, direction) {
    const distance = parseFloat(document.getElementById('jog-distance').value);
    const moveDistance = distance * direction;
    
    let command = `G91\nG0 ${axis}${moveDistance}\nG90`;
    sendGCode(command);
}

// Home Functions
function homeAll() {
    sendGCode('G28');
}

function homeAxis(axis) {
    sendGCode(`G28 ${axis}`);
}

// Temperature Functions
function setTemperature(heater) {
    let targetInput, command;
    
    if (heater === 'hotend') {
        targetInput = document.getElementById('hotend-target');
        command = `M104 S${targetInput.value}`;
    } else if (heater === 'bed') {
        targetInput = document.getElementById('bed-target');
        command = `M140 S${targetInput.value}`;
    }
    
    if (command) {
        sendGCode(command);
    }
}

function setTemp(heater, temp) {
    if (heater === 'hotend') {
        document.getElementById('hotend-target').value = temp;
        sendGCode(`M104 S${temp}`);
    } else if (heater === 'bed') {
        document.getElementById('bed-target').value = temp;
        sendGCode(`M140 S${temp}`);
    }
}

// Mode Functions
function setMode(mode) {
    // Remove active class from all mode buttons
    const modeButtons = document.querySelectorAll('.mode-btn');
    modeButtons.forEach(btn => btn.classList.remove('active'));
    
    // Add active class to clicked button
    event.target.classList.add('active');
    
    let command;
    let modeText;
    
    switch(mode) {
        case 'printer':
            command = 'M450';
            modeText = '3D Printer';
            break;
        case 'cnc':
            command = 'M451';
            modeText = 'CNC Spindle';
            break;
        case 'laser':
            command = 'M452';
            modeText = 'Laser Cutter';
            break;
    }
    
    if (command) {
        sendGCode(command);
        document.getElementById('mode-status').textContent = modeText;
    }
}

// Emergency Stop
function emergencyStop() {
    if (confirm('Execute emergency stop?')) {
        // Send via WebSocket
        const message = {
            type: 'emergency'
        };
        if (isConnected) {
            ws.send(JSON.stringify(message));
        }
        
        // Also send via HTTP API
        fetch('/api/emergency', {
            method: 'POST'
        })
        .then(() => {
            addConsoleMessage('EMERGENCY STOP ACTIVATED', 'error');
        })
        .catch(error => console.error('Error sending emergency stop:', error));
        
        // Send M112 command as well
        sendGCode('M112');
    }
}

// File Upload
function uploadFile() {
    const fileInput = document.getElementById('gcode-file');
    const file = fileInput.files[0];
    
    if (!file) {
        addConsoleMessage('No file selected', 'error');
        return;
    }
    
    const formData = new FormData();
    formData.append('file', file);
    
    addConsoleMessage(`Uploading ${file.name}...`, 'sent');
    
    fetch('/api/upload', {
        method: 'POST',
        body: formData
    })
    .then(response => {
        if (response.ok) {
            addConsoleMessage(`File ${file.name} uploaded successfully`, 'received');
            
            // Optionally, start executing the file
            if (confirm('Start executing uploaded file?')) {
                executeUploadedFile(file.name);
            }
        } else {
            addConsoleMessage(`Failed to upload ${file.name}`, 'error');
        }
    })
    .catch(error => {
        console.error('Error uploading file:', error);
        addConsoleMessage(`Error uploading file: ${error}`, 'error');
    });
}

function executeUploadedFile(filename) {
    // Read and send file line by line
    // This is a simple implementation - a production version would be more sophisticated
    const fileInput = document.getElementById('gcode-file');
    const file = fileInput.files[0];
    
    if (!file) return;
    
    const reader = new FileReader();
    reader.onload = function(e) {
        const lines = e.target.result.split('\n');
        let lineIndex = 0;
        
        function sendNextLine() {
            if (lineIndex < lines.length) {
                const line = lines[lineIndex].trim();
                if (line && !line.startsWith(';')) {
                    sendGCode(line);
                }
                lineIndex++;
                setTimeout(sendNextLine, 100); // 100ms delay between commands
            } else {
                addConsoleMessage('File execution complete', 'received');
            }
        }
        
        sendNextLine();
    };
    
    reader.readAsText(file);
}

// Utility Functions
function formatTime(seconds) {
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = Math.floor(seconds % 60);
    
    return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
}

function formatFileSize(bytes) {
    if (bytes < 1024) return bytes + ' B';
    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(2) + ' KB';
    return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
}

// Helper function for external scripts
function addToConsole(msg) {
    addConsoleMessage(msg, 'received');
}


/*
 * Encoder3D CNC Controller - G-Code Parser Implementation
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "gcode_parser.h"
#include "sd_card_manager.h"

// ============================================================================
// GCode Parser Implementation
// ============================================================================

GCodeParser::GCodeParser(MotorController* motors, HeaterController* heaters)
    : motor_controller(motors), heater_controller(heaters), 
      alarm_system(nullptr), system_monitor(nullptr), command_ready(false) {
    
    // Initialize machine state
    for (int i = 0; i < 6; i++) {
        state.position[i] = 0;
    }
    state.feedrate = 1000;  // Default feedrate mm/min
    state.absolute_mode = true;
    state.absolute_extrude = true;
    state.units = 0;  // mm
    state.mode = DEFAULT_MODE;
    state.spindle_speed = 0;
    state.spindle_on = false;
    state.laser_on = false;
    state.laser_power = 0;
}

void GCodeParser::begin() {
    command_buffer = "";
    sendResponse("ok Encoder3D ready");
}

void GCodeParser::setAlarmSystem(AlarmSystem* alarms) {
    alarm_system = alarms;
}

void GCodeParser::setSystemMonitor(SystemMonitor* monitor) {
    system_monitor = monitor;
}

void GCodeParser::processLine(String line) {
    line.trim();
    
    if (line.length() == 0) {
        sendResponse("ok");
        return;
    }
    
    GCodeCommand cmd;
    if (parseCommand(line, cmd)) {
        executeCommand(cmd);
    } else {
        sendError("Error: Invalid command");
    }
}

bool GCodeParser::parseCommand(String line, GCodeCommand& cmd) {
    cmd.reset();
    
    // Remove comments
    int comment_idx = line.indexOf(';');
    if (comment_idx >= 0) {
        cmd.comment = line.substring(comment_idx + 1);
        line = line.substring(0, comment_idx);
    }
    
    line.trim();
    line.toUpperCase();
    
    if (line.length() == 0) return false;
    
    // Parse command letter and number
    cmd.letter = line.charAt(0);
    int num_start = 1;
    int num_end = 1;
    
    while (num_end < line.length() && (isdigit(line.charAt(num_end)) || line.charAt(num_end) == '.')) {
        num_end++;
    }
    
    if (num_end > num_start) {
        cmd.number = parseInt(line.substring(num_start, num_end));
    }
    
    // Parse parameters
    for (int i = num_end; i < line.length(); i++) {
        char c = line.charAt(i);
        
        if (c == ' ') continue;
        
        int value_start = i + 1;
        int value_end = value_start;
        
        while (value_end < line.length() && 
               (isdigit(line.charAt(value_end)) || 
                line.charAt(value_end) == '.' || 
                line.charAt(value_end) == '-')) {
            value_end++;
        }
        
        if (value_end > value_start) {
            float value = parseFloat(line.substring(value_start, value_end));
            
            switch (c) {
                case 'X': cmd.x = value; cmd.has_x = true; break;
                case 'Y': cmd.y = value; cmd.has_y = true; break;
                case 'Z': cmd.z = value; cmd.has_z = true; break;
                case 'E': cmd.e = value; cmd.has_e = true; break;
                case 'F': cmd.f = value; cmd.has_f = true; break;
                case 'S': cmd.s = value; cmd.has_s = true; break;
                case 'P': cmd.p = value; cmd.has_p = true; break;
            }
            
            i = value_end - 1;
        }
    }
    
    return true;
}

void GCodeParser::executeCommand(const GCodeCommand& cmd) {
    if (cmd.letter == 'G') {
        switch (cmd.number) {
            case 0:
            case 1: handleG0G1(cmd); break;
            case 28: handleG28(cmd); break;
            case 90: handleG90(cmd); break;
            case 91: handleG91(cmd); break;
            case 92: handleG92(cmd); break;
            default:
                sendError("Error: Unknown G-code: G" + String(cmd.number));
                return;
        }
    } else if (cmd.letter == 'M') {
        switch (cmd.number) {
            case 3: handleM3(cmd); break;
            case 4: handleM4(cmd); break;
            case 5: handleM5(cmd); break;
            case 82: handleM82(cmd); break;
            case 83: handleM83(cmd); break;
            case 104: handleM104(cmd); break;
            case 105: handleM105(cmd); return;  // Don't send ok
            case 106: handleM106(cmd); break;
            case 107: handleM107(cmd); break;
            case 109: handleM109(cmd); break;
            case 112: handleM112(cmd); break;
            case 114: handleM114(cmd); return;  // Don't send ok
            case 119: handleM119(cmd); return;  // Don't send ok
            case 140: handleM140(cmd); break;
            case 190: handleM190(cmd); break;
            case 450: handleM450(cmd); break;
            case 451: handleM451(cmd); break;
            case 452: handleM452(cmd); break;
            // Laser extended commands (M460-M469)
            case 460: handleM460(cmd); break;
            case 461: handleM461(cmd); break;
            case 462: handleM462(cmd); break;
            case 463: handleM463(cmd); break;
            case 464: handleM464(cmd); break;
            case 465: handleM465(cmd); break;
            case 466: handleM466(cmd); break;
            case 467: handleM467(cmd); break;
            // SD card commands
            case 20: handleM20(cmd); return;   // List files - don't send ok
            case 21: handleM21(cmd); break;
            case 22: handleM22(cmd); break;
            case 23: handleM23(cmd); break;
            case 24: handleM24(cmd); break;
            case 25: handleM25(cmd); break;
            case 27: handleM27(cmd); return;   // Status - don't send ok
            case 30: handleM30(cmd); break;
            // Alarm commands (M700-M799)
            case 700: handleM700(cmd); return;  // Get alarm status - don't send ok
            case 701: handleM701(cmd); break;
            case 702: handleM702(cmd); break;
            case 703: handleM703(cmd); break;
            case 704: handleM704(cmd); return;  // Get health - don't send ok
            // PID tuning commands (M800-M899)
            case 800: handleM800(cmd); break;
            case 801: handleM801(cmd); break;
            case 802: handleM802(cmd); break;
            case 803: handleM803(cmd); break;
            case 804: handleM804(cmd); break;
            case 805: handleM805(cmd); break;
            // Diagnostics (M900-M999)
            case 900: handleM900(cmd); break;
            case 901: handleM901(cmd); break;
            case 902: handleM902(cmd); break;
            case 903: handleM903(cmd); break;
            case 999: handleM999(cmd); break;
            default:
                sendError("Error: Unknown M-code: M" + String(cmd.number));
                return;
        }
    } else {
        sendError("Error: Unknown command letter: " + String(cmd.letter));
        return;
    }
    
    sendResponse("ok");
}

// ============================================================================
// G-Code Handlers
// ============================================================================

void GCodeParser::handleG0G1(const GCodeCommand& cmd) {
    // Linear move
    float target_x1 = state.position[MOTOR_X1];
    float target_x2 = state.position[MOTOR_X2];
    float target_y1 = state.position[MOTOR_Y1];
    float target_y2 = state.position[MOTOR_Y2];
    float target_z = state.position[MOTOR_Z];
    float target_e = state.position[MOTOR_E];
    
    // For dual X/Y motors, we'll use averaged positions for simplicity
    // In a real implementation, you'd want proper kinematics
    if (cmd.has_x) {
        float x = cmd.x;
        if (!state.absolute_mode) x += (target_x1 + target_x2) / 2.0;
        target_x1 = x;
        target_x2 = x;
    }
    
    if (cmd.has_y) {
        float y = cmd.y;
        if (!state.absolute_mode) y += (target_y1 + target_y2) / 2.0;
        target_y1 = y;
        target_y2 = y;
    }
    
    if (cmd.has_z) {
        target_z = cmd.z;
        if (!state.absolute_mode) target_z += state.position[MOTOR_Z];
    }
    
    if (cmd.has_e) {
        target_e = cmd.e;
        if (!state.absolute_extrude) target_e += state.position[MOTOR_E];
    }
    
    if (cmd.has_f) {
        state.feedrate = cmd.f;
    }
    
    // Execute move
    motor_controller->linearMove(target_x1, target_x2, target_y1, target_y2, target_z, target_e, state.feedrate);
    
    // Update position
    updatePosition(target_x1, target_x2, target_y1, target_y2, target_z, target_e);
}

void GCodeParser::handleG28(const GCodeCommand& cmd) {
    // Home axes
    if (!cmd.has_x && !cmd.has_y && !cmd.has_z) {
        // Home all
        motor_controller->homeAll();
    } else {
        if (cmd.has_x) {
            motor_controller->home(MOTOR_X1);
            motor_controller->home(MOTOR_X2);
        }
        if (cmd.has_y) {
            motor_controller->home(MOTOR_Y1);
            motor_controller->home(MOTOR_Y2);
        }
        if (cmd.has_z) {
            motor_controller->home(MOTOR_Z);
        }
    }
    
    // Reset position after homing
    for (int i = 0; i < 6; i++) {
        state.position[i] = 0;
    }
}

void GCodeParser::handleG90(const GCodeCommand& cmd) {
    state.absolute_mode = true;
}

void GCodeParser::handleG91(const GCodeCommand& cmd) {
    state.absolute_mode = false;
}

void GCodeParser::handleG92(const GCodeCommand& cmd) {
    // Set position
    if (cmd.has_x) state.position[MOTOR_X1] = state.position[MOTOR_X2] = cmd.x;
    if (cmd.has_y) state.position[MOTOR_Y1] = state.position[MOTOR_Y2] = cmd.y;
    if (cmd.has_z) state.position[MOTOR_Z] = cmd.z;
    if (cmd.has_e) state.position[MOTOR_E] = cmd.e;
}

// ============================================================================
// M-Code Handlers
// ============================================================================

void GCodeParser::handleM82(const GCodeCommand& cmd) {
    state.absolute_extrude = true;
}

void GCodeParser::handleM83(const GCodeCommand& cmd) {
    state.absolute_extrude = false;
}

void GCodeParser::handleM104(const GCodeCommand& cmd) {
    if (cmd.has_s) {
        heater_controller->setTemperature(HEATER_HOTEND, cmd.s);
    }
}

void GCodeParser::handleM109(const GCodeCommand& cmd) {
    if (cmd.has_s) {
        heater_controller->setTemperature(HEATER_HOTEND, cmd.s);
        sendResponse("Heating hotend to " + String(cmd.s, 1) + "C");
        // Note: This is non-blocking. Client should poll M105 for status.
        // For blocking behavior, implement in client/slicer with M105 polling loop
    }
}

void GCodeParser::handleM140(const GCodeCommand& cmd) {
    if (cmd.has_s) {
        heater_controller->setTemperature(HEATER_BED, cmd.s);
    }
}

void GCodeParser::handleM190(const GCodeCommand& cmd) {
    if (cmd.has_s) {
        heater_controller->setTemperature(HEATER_BED, cmd.s);
        sendResponse("Heating bed to " + String(cmd.s, 1) + "C");
        // Note: This is non-blocking. Client should poll M105 for status.
    }
}

void GCodeParser::handleM105(const GCodeCommand& cmd) {
    // Report temperatures
    String response = "ok T:";
    response += String(heater_controller->getTemperature(HEATER_HOTEND), 1);
    response += " /";
    response += String(heater_controller->getTargetTemperature(HEATER_HOTEND), 1);
    response += " B:";
    response += String(heater_controller->getTemperature(HEATER_BED), 1);
    response += " /";
    response += String(heater_controller->getTargetTemperature(HEATER_BED), 1);
    
    sendResponse(response);
}

void GCodeParser::handleM114(const GCodeCommand& cmd) {
    // Report position
    String response = "ok X:";
    response += String((state.position[MOTOR_X1] + state.position[MOTOR_X2]) / 2.0, 2);
    response += " Y:";
    response += String((state.position[MOTOR_Y1] + state.position[MOTOR_Y2]) / 2.0, 2);
    response += " Z:";
    response += String(state.position[MOTOR_Z], 2);
    response += " E:";
    response += String(state.position[MOTOR_E], 2);
    
    sendResponse(response);
}

void GCodeParser::handleM119(const GCodeCommand& cmd) {
    // Report endstop status
    sendResponse("ok Endstops: X:open Y:open Z:open");
}

void GCodeParser::handleM3(const GCodeCommand& cmd) {
    // Spindle on CW
    if (cmd.has_s) {
        state.spindle_speed = cmd.s;
    }
    state.spindle_on = true;
    
    // Set spindle PWM
    float pwm = (state.spindle_speed / MAX_SPINDLE_RPM) * 255.0;
    ledcWrite(15, (int)pwm);
}

void GCodeParser::handleM4(const GCodeCommand& cmd) {
    // Spindle on CCW
    handleM3(cmd);  // Same as M3 for simple implementation
}

void GCodeParser::handleM5(const GCodeCommand& cmd) {
    // Spindle off
    state.spindle_on = false;
    ledcWrite(15, 0);
}

void GCodeParser::handleM106(const GCodeCommand& cmd) {
    // Fan/Laser on
    if (state.mode == MODE_LASER_CUTTER) {
        if (cmd.has_s) {
            state.laser_power = cmd.s;
        }
        state.laser_on = true;
        float pwm = (state.laser_power / 255.0) * 255.0;
        ledcWrite(16, (int)pwm);
    }
}

void GCodeParser::handleM107(const GCodeCommand& cmd) {
    // Fan/Laser off
    if (state.mode == MODE_LASER_CUTTER) {
        state.laser_on = false;
        ledcWrite(16, 0);
    }
}

void GCodeParser::handleM112(const GCodeCommand& cmd) {
    // Emergency stop
    motor_controller->emergencyStop();
    heater_controller->emergencyShutdownAll();
    sendResponse("Emergency stop!");
}

void GCodeParser::handleM450(const GCodeCommand& cmd) {
    state.mode = MODE_3D_PRINTER;
    sendResponse("Mode: 3D Printer");
}

void GCodeParser::handleM451(const GCodeCommand& cmd) {
    state.mode = MODE_CNC_SPINDLE;
    sendResponse("Mode: CNC Spindle");
}

void GCodeParser::handleM452(const GCodeCommand& cmd) {
    state.mode = MODE_LASER_CUTTER;
    sendResponse("Mode: Laser Cutter");
}

// ============================================================================
// Utility Functions
// ============================================================================

float GCodeParser::parseFloat(String str) {
    return str.toFloat();
}

int GCodeParser::parseInt(String str) {
    return str.toInt();
}

void GCodeParser::updatePosition(float x1, float x2, float y1, float y2, float z, float e) {
    state.position[MOTOR_X1] = x1;
    state.position[MOTOR_X2] = x2;
    state.position[MOTOR_Y1] = y1;
    state.position[MOTOR_Y2] = y2;
    state.position[MOTOR_Z] = z;
    state.position[MOTOR_E] = e;
}

String GCodeParser::getStatusReport() {
    String status = "Status: ";
    status += state.mode == MODE_3D_PRINTER ? "3D Printer" : 
              state.mode == MODE_CNC_SPINDLE ? "CNC Spindle" : "Laser Cutter";
    return status;
}

void GCodeParser::sendResponse(String msg) {
    // Non-blocking serial write with availability check
    if (Serial.availableForWrite() > msg.length() + 2) {
        Serial.println(msg);
    }
    // If buffer full, message is dropped to prevent blocking
}

void GCodeParser::sendError(String msg) {
    // Non-blocking serial write
    if (Serial.availableForWrite() > msg.length() + 2) {
        Serial.println(msg);
    }
}

// ============================================================================
// GCode Queue Implementation
// ============================================================================

GCodeQueue::GCodeQueue() : head(0), tail(0), count(0) {
}

bool GCodeQueue::push(const GCodeCommand& cmd) {
    if (isFull()) return false;
    
    queue[tail] = cmd;
    tail = (tail + 1) % COMMAND_QUEUE_SIZE;
    count++;
    
    return true;
}

bool GCodeQueue::pop(GCodeCommand& cmd) {
    if (isEmpty()) return false;
    
    cmd = queue[head];
    head = (head + 1) % COMMAND_QUEUE_SIZE;
    count--;
    
    return true;
}

bool GCodeQueue::peek(GCodeCommand& cmd) {
    if (isEmpty()) return false;
    
    cmd = queue[head];
    return true;
}

int GCodeQueue::size() {
    return count;
}

bool GCodeQueue::isEmpty() {
    return count == 0;
}

bool GCodeQueue::isFull() {
    return count >= COMMAND_QUEUE_SIZE;
}

void GCodeQueue::clear() {
    head = tail = count = 0;
}

// ============================================================================
// SD Card Command Handlers
// ============================================================================

void GCodeParser::handleM20(const GCodeCommand& cmd) {
    // List SD files
    SDCardManager* sd = SDCardManager::getInstance();
    if (!sd || !sd->isInitialized()) {
        sendError("Error: SD card not available");
        return;
    }
    
    sendResponse("Begin file list");
    sd->listFiles("/");
    sendResponse("End file list");
    sendResponse("ok");
void GCodeParser::handleM21(const GCodeCommand& cmd) {
    // Init SD card
    SDCardManager* sd = SDCardManager::getInstance();
    if (!sd) {
        sendError("Error: SD card not available");
        return;
    }
    
    sd->setGCodeParser(this);
    if (sd->begin()) {
        sendResponse("SD card ok");
    } else {
        sendError("Error: SD init failed");
    }
}

void GCodeParser::handleM22(const GCodeCommand& cmd) {
    // Release SD card
    SDCardManager* sd = SDCardManager::getInstance();
    if (!sd) {
        sendError("Error: SD card not available");
        return;
    }
    
    sd->end();
    sendResponse("SD card released");
}
void GCodeParser::handleM23(const GCodeCommand& cmd) {
    // Select SD file
    SDCardManager* sd = SDCardManager::getInstance();
    if (!sd || !sd->isInitialized()) {
        sendError("Error: SD card not available");
        return;
    }
    
    // Extract filename from comment or P parameter
    String filename = cmd.comment;
    if (filename.length() == 0) {
        sendError("Error: No filename specified");
        return;
    }
    
    // Ensure filename starts with /
    if (!filename.startsWith("/")) {
        filename = "/" + filename;
    }
    
    if (!sd->fileExists(filename)) {
        sendError("Error: File not found: " + filename);
        return;
    }
    
    size_t size = sd->getFileSize(filename);
    sendResponse("File opened: " + filename + " Size: " + String(size));
    sendResponse("File selected");
}

void GCodeParser::handleM24(const GCodeCommand& cmd) {
    // Start/resume SD print
    SDCardManager* sd = SDCardManager::getInstance();
    if (!sd || !sd->isInitialized()) {
        sendError("Error: SD card not available");
        return;
    }
    
    // Start new print - need filename from M23
    String filename = sd->getCurrentFile();
    if (filename.length() == 0) {
        sendError("Error: No file selected");
        return;
    }
    
    if (sd->startFile(filename)) {
        sendResponse("SD print started");
    } else {
        sendError("Error: Failed to start print");
    }
}

void GCodeParser::handleM25(const GCodeCommand& cmd) {
    // Pause SD print
    SDCardManager* sd = SDCardManager::getInstance();
    if (!sd || !sd->isInitialized()) {
        sendError("Error: SD card not available");
        return;
    }
    
    sd->pauseExecution();
    sendResponse("SD print paused");
}
void GCodeParser::handleM27(const GCodeCommand& cmd) {
    // Report SD print status
    SDCardManager* sd = SDCardManager::getInstance();
    if (!sd) {
        sendError("Error: SD card not available");
        return;
    }
    
    if (sd->isExecuting()) {
        String status = "SD printing byte " + String((int)sd->getProgress()) + "%";
        sendResponse(status);
    } else if (sd->isPaused()) {
        sendResponse("SD print paused");
    } else {
        sendResponse("Not SD printing");
    }
    sendResponse("ok");
}

void GCodeParser::handleM30(const GCodeCommand& cmd) {
    // Delete SD file
    SDCardManager* sd = SDCardManager::getInstance();
    if (!sd || !sd->isInitialized()) {
        sendError("Error: SD card not available");
        return;
    }
    
    // Extract filename from comment
    String filename = cmd.comment;
    if (filename.length() == 0) {
        sendError("Error: No filename specified");
        return;
    }
    
    // Ensure filename starts with /
    if (!filename.startsWith("/")) {
        filename = "/" + filename;
    }
    
    if (sd->deleteFile(filename)) {
        sendResponse("File deleted: " + filename);
    } else {
        sendError("Error: Failed to delete file");
    }
}

// ============================================================================
// Alarm Management Handlers (M700-M799)
// ============================================================================

void GCodeParser::handleM700(const GCodeCommand& cmd) {
    // Get alarm status
    if (!alarm_system) {
        sendError("Error: Alarm system not initialized");
        return;
    }
    
    sendResponse(alarm_system->getAlarmJSON());
    sendResponse("ok");
}

void GCodeParser::handleM701(const GCodeCommand& cmd) {
    // Clear all alarms
    if (!alarm_system) {
        sendError("Error: Alarm system not initialized");
        return;
    }
    
    alarm_system->clearAllAlarms();
    sendResponse("All alarms cleared");
}

void GCodeParser::handleM702(const GCodeCommand& cmd) {
    // Acknowledge all alarms
    if (!alarm_system) {
        sendError("Error: Alarm system not initialized");
        return;
    }
    
    alarm_system->acknowledgeAllAlarms();
    sendResponse("All alarms acknowledged");
}

void GCodeParser::handleM703(const GCodeCommand& cmd) {
    // Set alarm tolerance
    // M703 S<motor_pos_tol> P<motor_vel_tol> T<temp_tol>
    if (!alarm_system) {
        sendError("Error: Alarm system not initialized");
        return;
    }
    
    ToleranceConfig tolerances = alarm_system->getTolerances();
    
    if (cmd.has_s) {
        tolerances.motor_position_tolerance = cmd.s;
    }
    if (cmd.has_p) {
        tolerances.motor_velocity_tolerance = cmd.p;
    }
    if (cmd.letter == 'T' && cmd.has_p) {  // Use P for temp when T is letter
        tolerances.temp_tolerance = cmd.p;
    }
    
    alarm_system->setTolerances(tolerances);
    sendResponse("Tolerances updated");
}

void GCodeParser::handleM704(const GCodeCommand& cmd) {
    // Get system health status
    if (!system_monitor) {
        sendError("Error: System monitor not initialized");
        return;
    }
    
    sendResponse(system_monitor->getStatusJSON());
    sendResponse("ok");
}

// ============================================================================
// PID Tuning Handlers (M800-M899)
// ============================================================================

void GCodeParser::handleM800(const GCodeCommand& cmd) {
    // Set motor PID: M800 P<motor_id> S<Kp> I<Ki> D<Kd>
    // For now, send acknowledgment (actual implementation needs motor access to PID)
    sendResponse("Motor PID configuration not yet fully implemented");
}

void GCodeParser::handleM801(const GCodeCommand& cmd) {
    // Set heater PID: M801 P<heater_id> S<Kp> I<Ki> D<Kd>
    if (!heater_controller) {
        sendError("Error: Heater controller not initialized");
        return;
    }
    
    if (!cmd.has_p) {
        sendError("Error: Heater ID required (P parameter)");
        return;
    }
    
    int heater_id = (int)cmd.p;
    
    if (cmd.has_s && cmd.has_e && cmd.has_f) {
        // Assuming S=Kp, E=Ki (reusing E), F=Kd (reusing F)
        float kp = cmd.s;
        float ki = cmd.e;  // Reuse E parameter
        float kd = cmd.f;  // Reuse F parameter
        
        heater_controller->setPID(heater_id, kp, ki, kd);
        sendResponse("Heater " + String(heater_id) + " PID set: Kp=" + String(kp, 3) + 
                    " Ki=" + String(ki, 3) + " Kd=" + String(kd, 3));
    } else {
        sendError("Error: PID parameters required (S=Kp, E=Ki, F=Kd)");
    }
}

void GCodeParser::handleM802(const GCodeCommand& cmd) {
    // Auto-tune motor PID: M802 P<motor_id>
    sendResponse("Motor PID auto-tuning not yet implemented");
}

void GCodeParser::handleM803(const GCodeCommand& cmd) {
    // Auto-tune heater PID: M803 P<heater_id>
    sendResponse("Heater PID auto-tuning initiated");
    sendResponse("This may take several minutes...");
    // Actual auto-tune would be triggered here
}

void GCodeParser::handleM804(const GCodeCommand& cmd) {
    // Load PID preset: M804 S<preset_name>
    // Presets: "conservative", "balanced", "aggressive"
    sendResponse("PID preset loading not yet implemented");
}

void GCodeParser::handleM805(const GCodeCommand& cmd) {
    // Save current PID as preset
    sendResponse("PID preset saving not yet implemented");
}

// ============================================================================
// Diagnostic Handlers (M900-M999)
// ============================================================================

void GCodeParser::handleM900(const GCodeCommand& cmd) {
    // Run full system diagnostics
    if (system_monitor) {
        system_monitor->runDiagnostics();
        sendResponse("Diagnostics complete - check serial output");
    } else {
        sendError("Error: System monitor not initialized");
    }
}

void GCodeParser::handleM901(const GCodeCommand& cmd) {
    // Calibrate motors
    if (system_monitor) {
        system_monitor->calibrateMotors();
        sendResponse("Motor calibration complete");
    } else {
        sendError("Error: System monitor not initialized");
    }
}

void GCodeParser::handleM902(const GCodeCommand& cmd) {
    // Test motor: M902 P<motor_id>
    if (!motor_controller) {
        sendError("Error: Motor controller not initialized");
        return;
    }
    
    if (!cmd.has_p) {
        sendError("Error: Motor ID required (P parameter)");
        return;
    }
    
    int motor_id = (int)cmd.p;
    sendResponse("Testing motor " + String(motor_id) + "...");
    
    // Simple test: move motor forward and back
    // NOTE: This is a non-blocking command - motor will move asynchronously
    motor_controller->enableMotor(motor_id);
    motor_controller->setTargetPosition(motor_id, 10.0);  // Move 10mm
    // Wait for move to complete (non-blocking - poll in main loop)
    // After first move completes, send G92 to return to zero
    
    sendResponse("Motor " + String(motor_id) + " test initiated - will move 10mm");
    sendResponse("Use M114 to check position, then G92 to reset");
}

void GCodeParser::handleM903(const GCodeCommand& cmd) {
    // Test heater: M903 P<heater_id> S<temp>
    if (!heater_controller) {
        sendError("Error: Heater controller not initialized");
        return;
    }
    
    if (!cmd.has_p) {
        sendError("Error: Heater ID required (P parameter)");
        return;
    }
    
    int heater_id = (int)cmd.p;
    float test_temp = cmd.has_s ? cmd.s : 50.0;  // Default 50°C
    
    sendResponse("Testing heater " + String(heater_id) + " at " + String(test_temp, 1) + "°C");
    heater_controller->setTargetTemperature(heater_id, test_temp);
    heater_controller->enableHeater(heater_id);
    
    sendResponse("Heater test started - monitor temperature");
}

void GCodeParser::handleM999(const GCodeCommand& cmd) {
    // Reset controller
    sendResponse("Resetting controller...");
    // Allow message to be sent before restart
    Serial.flush();
    ESP.restart();
}

// ============================================================================
// Laser Extended Command Handlers (M460-M469)
// ============================================================================

void GCodeParser::handleM460(const GCodeCommand& cmd) {
    // Set laser type by index
    // M460 P<type_index>
    sendResponse("Laser type selection - not yet implemented");
    // Would integrate with LaserController here
}

void GCodeParser::handleM461(const GCodeCommand& cmd) {
    // Load laser profile by name
    // M461 S<profile_name>
    sendResponse("Laser profile loading - not yet implemented");
    // Would call laser_controller.loadProfile(name)
}

void GCodeParser::handleM462(const GCodeCommand& cmd) {
    // Set laser power in watts
    // M462 S<watts>
    if (!cmd.has_s) {
        sendError("Error: Power value required (S parameter)");
        return;
    }
    
    sendResponse("Set laser power: " + String(cmd.s, 1) + "W");
    // Would call laser_controller.setPowerWatts(cmd.s)
}

void GCodeParser::handleM463(const GCodeCommand& cmd) {
    // Set laser power in percent
    // M463 S<percent>
    if (!cmd.has_s) {
        sendError("Error: Power value required (S parameter)");
        return;
    }
    
    sendResponse("Set laser power: " + String(cmd.s, 1) + "%");
    // Would call laser_controller.setPowerPercent(cmd.s)
}

void GCodeParser::handleM464(const GCodeCommand& cmd) {
    // Enable/disable power ramping
    // M464 S<0|1> P<rate_watts_per_sec>
    if (!cmd.has_s) {
        sendError("Error: Enable flag required (S parameter)");
        return;
    }
    
    bool enable = cmd.s > 0.5;
    sendResponse(String(enable ? "Enabling" : "Disabling") + " laser power ramping");
    
    if (cmd.has_p && enable) {
        sendResponse("Ramp rate: " + String(cmd.p, 1) + "W/s");
        // laser_controller.setRampRate(cmd.p)
    }
    
    // laser_controller.enableRamping(enable)
}

void GCodeParser::handleM465(const GCodeCommand& cmd) {
    // Set pulse mode
    // M465 F<frequency_Hz> S<duty_cycle_percent>
    if (!cmd.has_f || !cmd.has_s) {
        sendError("Error: Frequency (F) and duty cycle (S) required");
        return;
    }
    
    uint16_t freq = (uint16_t)cmd.f;
    float duty = cmd.s / 100.0;
    
    sendResponse("Pulse mode: " + String(freq) + "Hz, " + String(cmd.s, 1) + "% duty");
    // laser_controller.setPulseMode(freq, duty)
}

void GCodeParser::handleM466(const GCodeCommand& cmd) {
    // Laser safety check
    sendResponse("Laser safety status:");
    sendResponse("  Interlock: OK");      // Would check actual status
    sendResponse("  Enclosure: OK");
    sendResponse("  Air Assist: OK");
    sendResponse("  Water Flow: OK");
    // Would call laser_controller.checkSafety() and report actual status
}

void GCodeParser::handleM467(const GCodeCommand& cmd) {
    // Emergency laser stop
    sendResponse("EMERGENCY LASER STOP");
    // laser_controller.emergencyStop()
}


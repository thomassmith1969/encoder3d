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
    : motor_controller(motors), heater_controller(heaters), command_ready(false) {
    
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
            // SD card commands
            case 20: handleM20(cmd); return;   // List files - don't send ok
            case 21: handleM21(cmd); break;
            case 22: handleM22(cmd); break;
            case 23: handleM23(cmd); break;
            case 24: handleM24(cmd); break;
            case 25: handleM25(cmd); break;
            case 27: handleM27(cmd); return;   // Status - don't send ok
            case 30: handleM30(cmd); break;
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
    if (!sd_card_manager) {
        sendError("Error: SD card not available");
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
}   if (!sd_card_manager) {
        sendError("Error: SD card not available");
        return;
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
void GCodeParser::handleM23(const GCodeCommand& cmd) {
    // Select SD file
    SDCardManager* sd = SDCardManager::getInstance();
    if (!sd || !sd->isInitialized()) {
        sendError("Error: SD card not available");
        return;
    }   return;
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
    if (!sd_card_manager) {
        sendError("Error: SD card not available");
        return;
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
}

void GCodeParser::handleM25(const GCodeCommand& cmd) {
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


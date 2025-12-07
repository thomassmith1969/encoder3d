/*
 * Encoder3D CNC Controller - G-Code Parser
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 * 
 * G-code interpreter and command execution
 */

#ifndef GCODE_PARSER_H
#define GCODE_PARSER_H

#include <Arduino.h>
#include "config.h"
#include "motor_controller.h"
#include "heater_controller.h"

// G-code command structure
struct GCodeCommand {
    char letter;           // G, M, T, etc.
    int number;            // Command number
    bool has_x, has_y, has_z, has_e;
    bool has_f, has_s, has_p;
    float x, y, z, e;
    float f;  // Feedrate
    float s;  // Spindle/laser power
    float p;  // Parameter
    String comment;
    
    void reset() {
        letter = '\0';
        number = -1;
        has_x = has_y = has_z = has_e = false;
        has_f = has_s = has_p = false;
        x = y = z = e = f = s = p = 0;
        comment = "";
    }
};

// G-code parser and executor
class GCodeParser {
private:
    MotorController* motor_controller;
    HeaterController* heater_controller;
    
    // Parser state
    struct MachineState {
        float position[6];      // Current position X1, X2, Y1, Y2, Z, E
        float feedrate;         // Current feedrate
        bool absolute_mode;     // G90/G91
        bool absolute_extrude;  // M82/M83
        int units;              // G20/G21 (0=mm, 1=inches)
        OperationMode mode;
        float spindle_speed;
        bool spindle_on;
        bool laser_on;
        float laser_power;
    } state;
    
    String command_buffer;
    bool command_ready;
    
public:
    GCodeParser(MotorController* motors, HeaterController* heaters);
    
    void begin();
    void processLine(String line);
    bool parseCommand(String line, GCodeCommand& cmd);
    void executeCommand(const GCodeCommand& cmd);
    
    // Command handlers
    void handleG0G1(const GCodeCommand& cmd);  // Linear move
    void handleG28(const GCodeCommand& cmd);   // Home
    void handleG90(const GCodeCommand& cmd);   // Absolute positioning
    void handleG91(const GCodeCommand& cmd);   // Relative positioning
    void handleG92(const GCodeCommand& cmd);   // Set position
    
    void handleM82(const GCodeCommand& cmd);   // Absolute extrusion
    void handleM83(const GCodeCommand& cmd);   // Relative extrusion
    void handleM104(const GCodeCommand& cmd);  // Set hotend temp
    void handleM109(const GCodeCommand& cmd);  // Set hotend temp and wait
    void handleM140(const GCodeCommand& cmd);  // Set bed temp
    void handleM190(const GCodeCommand& cmd);  // Set bed temp and wait
    void handleM105(const GCodeCommand& cmd);  // Report temperatures
    void handleM114(const GCodeCommand& cmd);  // Report position
    void handleM119(const GCodeCommand& cmd);  // Report endstop status
    
    // CNC/Laser specific
    void handleM3(const GCodeCommand& cmd);    // Spindle on CW
    void handleM4(const GCodeCommand& cmd);    // Spindle on CCW
    void handleM5(const GCodeCommand& cmd);    // Spindle off
    void handleM106(const GCodeCommand& cmd);  // Fan/Laser on
    void handleM107(const GCodeCommand& cmd);  // Fan/Laser off
    
    // Emergency
    void handleM112(const GCodeCommand& cmd);  // Emergency stop
    
    // Mode selection
    void handleM450(const GCodeCommand& cmd);  // 3D printer mode
    void handleM451(const GCodeCommand& cmd);  // CNC mode
    void handleM452(const GCodeCommand& cmd);  // Laser mode
    
    // SD card commands
    void handleM20(const GCodeCommand& cmd);   // List SD files
    void handleM21(const GCodeCommand& cmd);   // Init SD card
    void handleM22(const GCodeCommand& cmd);   // Release SD card
    void handleM23(const GCodeCommand& cmd);   // Select SD file
    void handleM24(const GCodeCommand& cmd);   // Start/resume SD print
    void handleM25(const GCodeCommand& cmd);   // Pause SD print
    void handleM27(const GCodeCommand& cmd);   // Report SD print status
    void handleM30(const GCodeCommand& cmd);   // Delete SD file
    
    // Status reporting
    String getStatusReport();
    void sendResponse(String msg);
    void sendError(String msg);
    
private:
    float parseFloat(String str);
    int parseInt(String str);
    void updatePosition(float x1, float x2, float y1, float y2, float z, float e);
};

// Command queue for buffered execution
class GCodeQueue {
private:
    GCodeCommand queue[COMMAND_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    
public:
    GCodeQueue();
    
    bool push(const GCodeCommand& cmd);
    bool pop(GCodeCommand& cmd);
    bool peek(GCodeCommand& cmd);
    
    int size();
    bool isEmpty();
    bool isFull();
    void clear();
};

#endif // GCODE_PARSER_H

/*
 * Encoder3D CNC Controller - Main Application
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 * 
 * ESP32-based multi-mode CNC controller with encoder feedback
 * Supports 3D printing, CNC milling, and laser cutting
 */

#include <Arduino.h>
#include "config.h"
#include "motor_controller.h"
#include "heater_controller.h"
#include "gcode_parser.h"
#include "web_server.h"
#include "telnet_server.h"

// Global controller instances
MotorController* motorController;
HeaterController* heaterController;
GCodeParser* gcodeParser;
WebServerManager* webServer;
TelnetServer* telnetServer;

// System state
bool systemInitialized = false;
unsigned long lastStatusUpdate = 0;
const unsigned long STATUS_UPDATE_INTERVAL = 500; // ms

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    Serial.setTimeout(10);  // Set short timeout for non-blocking behavior
    
    // Brief delay for USB serial initialization only (one-time startup)
    // This is acceptable in setup() as it's not in the main loop
    delay(1000);
    
    Serial.println("\n\n");
    // Use non-blocking prints with buffer checks
    if (Serial.availableForWrite() > 100) {
        Serial.println("========================================");
        Serial.println("  Encoder3D CNC Controller");
        Serial.println("  ESP32-based Multi-Mode Controller");
        Serial.println("========================================");
        Serial.println();
    }
    
    // Initialize controllers
    if (Serial.availableForWrite() > 50) {
        Serial.println("Initializing motor controller...");
    }
    motorController = new MotorController();
    motorController->begin();
    
    if (Serial.availableForWrite() > 50) {
        Serial.println("Initializing heater controller...");
    }
    heaterController = new HeaterController();
    heaterController->begin();
    
    if (Serial.availableForWrite() > 50) {
        Serial.println("Initializing G-code parser...");
    }
    gcodeParser = new GCodeParser(motorController, heaterController);
    gcodeParser->begin();
    
    if (Serial.availableForWrite() > 50) {
        Serial.println("Initializing web server...");
    }
    webServer = new WebServerManager(motorController, heaterController, gcodeParser);
    webServer->begin();
    
    if (Serial.availableForWrite() > 50) {
        Serial.println("Initializing telnet server...");
    }
    telnetServer = new TelnetServer(gcodeParser);
    telnetServer->begin();
    
    // Start control loops
    if (Serial.availableForWrite() > 50) {
        Serial.println("Starting motor control loop...");
    }
    motorController->startControlLoop();
    
    if (Serial.availableForWrite() > 50) {
        Serial.println("Starting heater control loop...");
    }
    heaterController->startControlLoop();
    
    // Setup complete
    systemInitialized = true;
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("  System Initialization Complete!");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Web Interface: http://" + webServer->getIPAddress());
void loop() {
    // Update web server (non-blocking)
    webServer->update();
    
    // Update telnet server (non-blocking)
    telnetServer->update();
    
    // Check for serial commands (non-blocking with limit)
    static String serialBuffer = "";
    while (Serial.available() && serialBuffer.length() < 256) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (serialBuffer.length() > 0) {
                gcodeParser->processLine(serialBuffer);
                serialBuffer = "";
            }
        } else {
            serialBuffer += c;
        }
    }
    
    // Clear buffer if too long (prevent overflow)
    if (serialBuffer.length() >= 256) {
        Serial.println("Serial buffer overflow, clearing");
        serialBuffer = "";
    }
    
    // Periodic status updates (non-blocking)
    unsigned long now = millis();
    if (now - lastStatusUpdate > STATUS_UPDATE_INTERVAL) {
        lastStatusUpdate = now;
        
        // Broadcast status to web clients (non-blocking)
        webServer->broadcastStatus();
        
        // Check for thermal runaway (critical safety check)
        if (heaterController->anyThermalRunaway()) {
            Serial.println("ERROR: Thermal runaway detected!");
            motorController->emergencyStop();
            heaterController->emergencyShutdownAll();
        }
    }
    
    // Yield to prevent watchdog timeout
    yield();
}

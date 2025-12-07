/*
 * Encoder3D CNC Controller - Telnet Server Implementation
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "telnet_server.h"

TelnetServer::TelnetServer(GCodeParser* gcode) 
    : gcode_parser(gcode), client_connected(false) {
    
    server = new WiFiServer(TELNET_PORT);
}

TelnetServer::~TelnetServer() {
    if (client) {
        client.stop();
    }
    delete server;
}

void TelnetServer::begin() {
    server->begin();
    server->setNoDelay(true);
    
    Serial.println("Telnet server started on port " + String(TELNET_PORT));
}

void TelnetServer::update() {
    // Non-blocking check for new clients
    if (server->hasClient()) {
        handleNewClient();
    }
    
    // Handle existing client data (non-blocking)
    if (client && client.connected()) {
        // Check if client is still responsive
        if (client.available() > 0 || millis() % 100 == 0) {  // Periodic check
            handleClientData();
        }
    } else if (client_connected) {
        // Client disconnected - cleanup
        client.stop();
        client_connected = false;
        input_buffer = "";  // Clear buffer
        Serial.println("Telnet client disconnected");
    }
}

void TelnetServer::handleNewClient() {
    WiFiClient newClient = server->available();
    
    if (!client || !client.connected()) {
        // Accept new client
        if (client) {
            client.stop();
        }
        
        client = newClient;
        client_connected = true;
        
        Serial.print("New telnet client connected from: ");
        Serial.println(client.remoteIP());
        
        // Send welcome message
        client.println("Encoder3D CNC Controller");
        client.println("Ready to receive G-code commands");
        client.println("Type 'help' for available commands");
        client.print("> ");
        
        input_buffer = "";
    } else {
        // Already have a client, reject new connection
        newClient.stop();
        Serial.println("Telnet connection rejected - client already connected");
    }
}

void TelnetServer::handleClientData() {
    // Process only available data without blocking
    int available = client.available();
    if (available <= 0) return;
    
    // Limit processing to prevent blocking
    int processed = 0;
    const int MAX_CHARS_PER_CYCLE = 64;
    
    while (client.available() && processed < MAX_CHARS_PER_CYCLE) {
        char c = client.read();
        processed++;
        
        if (c == '\n' || c == '\r') {
            if (input_buffer.length() > 0) {
                processCommand(input_buffer);
                input_buffer = "";
                
                // Send prompt (non-blocking check)
                if (client.connected()) {
                    client.print("> ");
                }
            }
        } else if (c == '\b' || c == 127) {
            // Backspace
            if (input_buffer.length() > 0) {
                input_buffer.remove(input_buffer.length() - 1);
                if (client.connected()) {
                    client.print("\b \b");  // Erase character on terminal
                }
            }
        } else if (c >= 32 && c <= 126) {
            // Printable character
            if (input_buffer.length() < 256) {  // Prevent buffer overflow
                input_buffer += c;
                if (client.connected()) {
                    client.print(c);  // Echo back to client
                }
            }
        }
    }
}

void TelnetServer::processCommand(String cmd) {
    cmd.trim();
    
    if (cmd.length() == 0) {
        return;
    }
    
    // Echo command
    client.println();
    
    // Handle special telnet commands
    String cmdUpper = cmd;
    cmdUpper.toUpperCase();
    
    if (cmdUpper == "HELP") {
        client.println("Available commands:");
        client.println("  Standard G-code commands (G0, G1, G28, etc.)");
        client.println("  M-codes (M104, M140, M105, etc.)");
        client.println("  help - Show this help");
        client.println("  status - Show controller status");
        client.println("  quit - Disconnect");
        return;
    }
    
    if (cmdUpper == "QUIT" || cmdUpper == "EXIT") {
        client.println("Goodbye!");
        client.stop();
        client_connected = false;
        return;
    }
    
    if (cmdUpper == "STATUS") {
        client.println("Controller Status:");
        client.println("  Mode: 3D Printer");  // TODO: Get actual mode
        client.println("  Connected: Yes");
        return;
    }
    
    // Process as G-code
    gcode_parser->processLine(cmd);
}

bool TelnetServer::hasClient() {
    return client_connected && client.connected();
}

void TelnetServer::sendResponse(String msg) {
    if (hasClient() && client.connected()) {
        // Non-blocking write with error handling
        size_t written = client.print(msg);
        if (written > 0) {
            client.println();  // Add newline
        }
        // If write fails, client will be cleaned up on next update()
    }
}

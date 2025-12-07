/*
 * Encoder3D CNC Controller - Telnet Server
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 * 
 * Telnet interface for G-code commands
 */

#ifndef TELNET_SERVER_H
#define TELNET_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "gcode_parser.h"

class TelnetServer {
private:
    WiFiServer* server;
    WiFiClient client;
    GCodeParser* gcode_parser;
    
    bool client_connected;
    String input_buffer;
    
    static const int MAX_CLIENTS = 1;  // Only support one telnet client at a time
    
public:
    TelnetServer(GCodeParser* gcode);
    ~TelnetServer();
    
    void begin();
    void update();
    
    bool hasClient();
    void sendResponse(String msg);
    
private:
    void handleNewClient();
    void handleClientData();
    void processCommand(String cmd);
};

#endif // TELNET_SERVER_H

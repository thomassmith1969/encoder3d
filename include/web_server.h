/*
 * Encoder3D CNC Controller - Web Server
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 * 
 * HTTP server, WebSocket, and REST API
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"
#include "motor_controller.h"
#include "heater_controller.h"
#include "gcode_parser.h"

class WebServerManager {
private:
    AsyncWebServer* server;
    AsyncWebSocket* ws;
    MotorController* motor_controller;
    HeaterController* heater_controller;
    GCodeParser* gcode_parser;
    
    bool wifi_connected;
    
public:
    WebServerManager(MotorController* motors, HeaterController* heaters, GCodeParser* gcode);
    ~WebServerManager();
    
    void begin();
    void update();
    
    // WiFi management
    void setupWiFi();
    bool isConnected();
    String getIPAddress();
    
    // WebSocket handlers
    void broadcastStatus();
    void sendMessage(String msg);
    
private:
    // Route handlers
    void setupRoutes();
    void handleRoot(AsyncWebServerRequest *request);
    void handleStatus(AsyncWebServerRequest *request);
    void handleCommand(AsyncWebServerRequest *request);
    void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    
    // SD card REST API
    void handleSDList(AsyncWebServerRequest *request);
    void handleSDSelect(AsyncWebServerRequest *request);
    void handleSDStart(AsyncWebServerRequest *request);
    void handleSDPause(AsyncWebServerRequest *request);
    void handleSDStatus(AsyncWebServerRequest *request);
    void handleSDDelete(AsyncWebServerRequest *request);
    void handleSDDownload(AsyncWebServerRequest *request);
    
    // LittleFS REST API
    void handleLittleFSList(AsyncWebServerRequest *request);
    void handleLittleFSUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void handleLittleFSDownload(AsyncWebServerRequest *request);
    void handleLittleFSDelete(AsyncWebServerRequest *request);
    void handleLittleFSPrint(AsyncWebServerRequest *request);
    
    // WebSocket handlers
    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    static void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
    
    // API handlers
    String getStatusJSON();
    String getPositionJSON();
    String getTemperatureJSON();
    
    // File system
    void setupFileSystem();
};

#endif // WEB_SERVER_H

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include "config.h"
#include "thermal.h"

// Forward declarations
class ThermalManager;

class WebServerManager {
private:
    WebServer* server;
    WebSocketsServer* ws;
    DNSServer* dnsServer;
    WiFiServer* telnetServer; // Telnet Server
    WiFiClient telnetClient;  // Single Telnet Client
    ThermalManager* thermal;
    StreamBufferHandle_t* gcodeStream;
    
    bool isAPMode;
    String deviceHostname;

    void setupRoutes();
    void handleTelnet(); // Handle Telnet Logic
    void setupFileSystem();
    void setupWiFi();
    void handleUpload();
    void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

public:
    WebServerManager(ThermalManager* t, StreamBufferHandle_t* stream);
    void begin();
    void update();
    void broadcastStatus(float x, float y, float z, float e);
    void broadcastError(String message);
};

#endif

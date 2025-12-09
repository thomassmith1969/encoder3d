#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/stream_buffer.h>
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

// Owner/source types
#define SRC_SERIAL 0
#define SRC_WEBSOCKET 1
#define SRC_TELNET 2

// Raw command from a client
struct RawCommand {
    uint8_t srcType; // SRC_*
    int srcId;       // ws client id or telnet id (fixed)
    char line[128];
    size_t len;
};

// Executor owner info is shared from main
extern volatile uint8_t executorOwnerType;
extern volatile int executorOwnerId;
extern volatile bool executorBusy;
// (no test-only globals declared here)
// encoder presence flags (shared)
extern volatile bool encSeenX;
extern volatile bool encSeenY;
extern volatile bool encSeenZ;
extern volatile bool encSeenE;

class WebServerManager {
private:
    WebServer* server;
    WebSocketsServer* ws;
    DNSServer* dnsServer;
    WiFiServer* telnetServer; // Telnet Server
    WiFiClient telnetClient;  // Single Telnet Client
    ThermalManager* thermal;
    StreamBufferHandle_t* gcodeStream;
    QueueHandle_t* commandQueue; // Raw commands from clients
    
    bool isAPMode;
    String deviceHostname;

    void setupRoutes();
    void handleTelnet(); // Handle Telnet Logic
    void setupFileSystem();
    void setupWiFi();
    void handleUpload();
    void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

public:
    WebServerManager(ThermalManager* t, StreamBufferHandle_t* stream, QueueHandle_t* cmdQueue);
    void begin();
    void update();
    // Broadcast encoder positions (mm), temperature, targets and additional diagnostics
    void broadcastStatus(float x, float y, float z, float e,
                         float extTemp, float bedTemp, float extTarget, float bedTarget,
                         float setX, float setY, float setZ, float setE,
                         unsigned long lastMoveX, unsigned long lastMoveY, unsigned long lastMoveZ, unsigned long lastMoveE);
    void broadcastError(String message);
    void broadcastWarning(String message);
    void sendTelnet(String message);
    // Returns empty string on success, or 'busy' when the executor is owned by a different client.
    String pushClientCommand(const RawCommand &cmd); // Check ownership and push to queue
    void sendResponseToClient(uint8_t srcType, int srcId, const String &msg);
};

#endif

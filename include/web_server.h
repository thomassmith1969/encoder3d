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
#include "pid_controller.h"

// Forward declarations
class ThermalManager;

// Owner/source types
#define SRC_SERIAL 0
#define SRC_WEBSOCKET 1
#define SRC_TELNET 2
#define SRC_JOB 3

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

// Runtime-configurable globals (defined in main.cpp)
extern float countsPerMM_X;
extern float countsPerMM_Y;
extern float countsPerMM_Z;
extern float countsPerMM_E;

// Global executor spinlock (shared across translation units)
extern portMUX_TYPE g_executorMux;

extern float pid_kp_x; extern float pid_ki_x; extern float pid_kd_x;
extern float pid_kp_y; extern float pid_ki_y; extern float pid_kd_y;
extern float pid_kp_z; extern float pid_ki_z; extern float pid_kd_z;
extern float pid_kp_e; extern float pid_ki_e; extern float pid_kd_e;

extern int maxFeedrateX; extern int maxFeedrateY; extern int maxFeedrateZ; extern int maxFeedrateE;

// Run controls (pause/play/stop and speed multiplier)
extern volatile bool runPaused;
extern volatile bool runStopped;
extern volatile int runSpeedPercent; // 5..500
extern volatile float runSpeedMultiplier; // derived from percent

// Spindle / Laser runtime state
extern volatile int spindlePower; // 0..255
extern volatile int laserPower; // 0..255

// Global instance (defined in main.cpp)
extern class WebServerManager* webServer;

// Expose queues so admin endpoints can clear them if needed
extern QueueHandle_t motionQueue;
extern QueueHandle_t commandQueue;

// Reservation state set by web_server when push-time reservation is made
extern volatile unsigned long executorReservedUntil;

// Job state exported so parser can tag streamed input as job-origin
extern volatile bool jobActive;

// Job event broadcast API (member on WebServerManager)

// Storage selection for file operations
#define STORAGE_LITTLEFS 0
#define STORAGE_SD 1


// Expose PID controllers declared in main
extern PIDController pidX;
extern PIDController pidY;
extern PIDController pidZ;
extern PIDController pidE;

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
                         unsigned long lastMoveX, unsigned long lastMoveY, unsigned long lastMoveZ, unsigned long lastMoveE,
                         int motorOutX, int motorOutY, int motorOutZ, int motorOutE,
                         long encCountsX, long encCountsY, long encCountsZ, long encCountsE);
    void broadcastError(String message);
    void broadcastWarning(String message);
    void broadcastJobEvent(const char* event, const char* filename, const char* storage, int progress = -1);
    void sendTelnet(String message);
    // Returns empty string on success, or 'busy' when the executor is owned by a different client.
    String pushClientCommand(const RawCommand &cmd); // Check ownership and push to queue
    void sendResponseToClient(uint8_t srcType, int srcId, const String &msg);
};

#endif

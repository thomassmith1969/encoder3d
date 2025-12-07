/*
 * Encoder3D CNC Controller - Web Server Implementation
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "web_server.h"
#include "sd_card_manager.h"

WebServerManager::WebServerManager(MotorController* motors, HeaterController* heaters, GCodeParser* gcode)
    : motor_controller(motors), heater_controller(heaters), gcode_parser(gcode), wifi_connected(false) {
    
    server = new AsyncWebServer(WEB_SERVER_PORT);
    ws = new AsyncWebSocket("/ws");
}

WebServerManager::~WebServerManager() {
    delete ws;
    delete server;
}

void WebServerManager::begin() {
    setupFileSystem();
    setupWiFi();
    setupRoutes();
    
    // Start WebSocket
    ws->onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        this->onWebSocketEvent(server, client, type, arg, data, len);
    });
    server->addHandler(ws);
    
    // Start server
    server->begin();
    
    Serial.println("Web server started");
    Serial.print("IP Address: ");
    Serial.println(getIPAddress());
}

void WebServerManager::setupFileSystem() {
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }
    Serial.println("LittleFS mounted successfully");
}

void WebServerManager::setupWiFi() {
    if (WIFI_AP_MODE) {
        // Access Point mode
        WiFi.mode(WIFI_AP);
        WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
        wifi_connected = true;
        
        Serial.println("WiFi AP started");
        Serial.print("SSID: ");
        Serial.println(WIFI_SSID);
        Serial.print("IP: ");
        Serial.println(WiFi.softAPIP());
    } else {
        // Station mode
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        Serial.print("Connecting to WiFi");
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            wifi_connected = true;
            Serial.println("\nWiFi connected");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("\nWiFi connection failed");
        }
    }
}

void WebServerManager::setupRoutes() {
    // Serve static files from LittleFS
    server->serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");
    
    // API endpoints
    server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", this->getStatusJSON());
    });
    
    server->on("/api/position", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", this->getPositionJSON());
    });
    
    server->on("/api/temperature", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", this->getTemperatureJSON());
    });
    
    // Command endpoint
    server->on("/api/command", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("cmd", true)) {
            String cmd = request->getParam("cmd", true)->value();
            this->gcode_parser->processLine(cmd);
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "No command provided");
        }
    });
    
    // File upload for G-code
    server->on("/api/upload", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            request->send(200);
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            this->handleUpload(request, filename, index, data, len, final);
        }
    );
    
    // Emergency stop
    server->on("/api/emergency", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->motor_controller->emergencyStop();
        this->heater_controller->emergencyShutdownAll();
        request->send(200, "text/plain", "Emergency stop activated");
    });
    
    // SD card REST API endpoints
    server->on("/api/sd/list", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleSDList(request);
    });
    
    server->on("/api/sd/select", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleSDSelect(request);
    });
    
    server->on("/api/sd/start", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleSDStart(request);
    });
    
    server->on("/api/sd/pause", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleSDPause(request);
    });
    
    server->on("/api/sd/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleSDStatus(request);
    });
    
    server->on("/api/sd/delete", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleSDDelete(request);
    });
    
    server->on("/api/sd/download", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleSDDownload(request);
    });
    
    // LittleFS REST API endpoints
    server->on("/api/littlefs/list", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleLittleFSList(request);
    });
    
    server->on("/api/littlefs/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            request->send(200);
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            this->handleLittleFSUpload(request, filename, index, data, len, final);
        }
    );
    
    server->on("/api/littlefs/download", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleLittleFSDownload(request);
    });
    
    server->on("/api/littlefs/delete", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleLittleFSDelete(request);
    });
    
    server->on("/api/littlefs/print", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleLittleFSPrint(request);
    });
    
    // 404 handler
    server->onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });
}

void WebServerManager::handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    // Upload G-code files to SD card (if available), otherwise LittleFS
    static File uploadFile;
    static bool useSD = false;
    const size_t MAX_UPLOAD_SIZE = 100 * 1024 * 1024;  // 100MB limit
    
    if (index == 0) {
        // Check if we have space and valid filename
        if (filename.length() == 0 || filename.length() > 64) {
            Serial.println("Invalid filename");
            return;
        }
        
        Serial.printf("Upload start: %s\n", filename.c_str());
        
        // Try SD card first
        SDCardManager* sd = SDCardManager::getInstance();
        if (sd && sd->isInitialized()) {
            String path = "/" + filename;
            uploadFile = sd->openFile(path, FILE_WRITE);
            useSD = true;
            Serial.println("Uploading to SD card");
        } else {
            // Fallback to LittleFS
            uploadFile = LittleFS.open("/uploads/" + filename, "w");
            useSD = false;
            Serial.println("Uploading to LittleFS (SD not available)");
        }
        
        if (!uploadFile) {
            Serial.println("Failed to open file for writing");
            return;
        }
    }
    
    // Check upload size limit to prevent overflow
    if (index + len > MAX_UPLOAD_SIZE) {
        Serial.println("Upload too large, aborting");
        if (uploadFile) {
            uploadFile.close();
        }
        return;
    }
    
    if (uploadFile) {
        // Write with error checking
        size_t written = uploadFile.write(data, len);
        if (written != len) {
            Serial.println("Write error during upload");
        }
    }
    
    if (final) {
        if (uploadFile) {
            uploadFile.flush();  // Ensure data is written
            uploadFile.close();
        }
        Serial.printf("Upload complete: %s (%u bytes) to %s\n", 
                      filename.c_str(), index + len, useSD ? "SD" : "LittleFS");
    }
}

void WebServerManager::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
            
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
            
        case WS_EVT_DATA:
            {
                AwsFrameInfo *info = (AwsFrameInfo*)arg;
                
                // Validate frame info to prevent crashes
                if (!info || len == 0 || len > 1024) {  // Limit message size
                    Serial.println("Invalid WebSocket frame");
                    break;
                }
                
                if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                    // Ensure null termination without buffer overflow
                    if (len < 1024) {
                        data[len] = 0;
                        String message = (char*)data;
                        
                        // Limit message length
                        if (message.length() > 512) {
                            Serial.println("WebSocket message too long");
                            break;
                        }
                        
                        // Parse JSON command with error handling
                        StaticJsonDocument<256> doc;
                        DeserializationError error = deserializeJson(doc, message);
                        
                        if (!error) {
                            String cmd_type = doc["type"].as<String>();
                            
                            if (cmd_type == "gcode") {
                                String gcode = doc["command"].as<String>();
                                if (gcode.length() > 0 && gcode.length() < 256) {
                                    gcode_parser->processLine(gcode);
                                }
                            } else if (cmd_type == "emergency") {
                                motor_controller->emergencyStop();
                                heater_controller->emergencyShutdownAll();
                            }
                        } else {
                            Serial.println("JSON parse error in WebSocket message");
                        }
                    }
                }
            }
            break;
            
        case WS_EVT_PONG:
            break;
            
        case WS_EVT_ERROR:
            Serial.printf("WebSocket error from client #%u\n", client->id());
            break;
    }
}

String WebServerManager::getStatusJSON() {
    StaticJsonDocument<512> doc;
    
    doc["connected"] = true;
    doc["mode"] = "3D_PRINTER";  // TODO: Get actual mode from parser
    doc["moving"] = motor_controller->isMoving();
    
    JsonObject temps = doc.createNestedObject("temperatures");
    temps["hotend"]["current"] = heater_controller->getTemperature(HEATER_HOTEND);
    temps["hotend"]["target"] = heater_controller->getTargetTemperature(HEATER_HOTEND);
    temps["bed"]["current"] = heater_controller->getTemperature(HEATER_BED);
    temps["bed"]["target"] = heater_controller->getTargetTemperature(HEATER_BED);
    
    JsonObject pos = doc.createNestedObject("position");
    pos["x"] = (motor_controller->getPosition(MOTOR_X1) + motor_controller->getPosition(MOTOR_X2)) / 2.0;
    pos["y"] = (motor_controller->getPosition(MOTOR_Y1) + motor_controller->getPosition(MOTOR_Y2)) / 2.0;
    pos["z"] = motor_controller->getPosition(MOTOR_Z);
    pos["e"] = motor_controller->getPosition(MOTOR_E);
    
    String output;
    serializeJson(doc, output);
    return output;
}

String WebServerManager::getPositionJSON() {
    StaticJsonDocument<256> doc;
    
    doc["x"] = (motor_controller->getPosition(MOTOR_X1) + motor_controller->getPosition(MOTOR_X2)) / 2.0;
    doc["y"] = (motor_controller->getPosition(MOTOR_Y1) + motor_controller->getPosition(MOTOR_Y2)) / 2.0;
    doc["z"] = motor_controller->getPosition(MOTOR_Z);
    doc["e"] = motor_controller->getPosition(MOTOR_E);
    
    String output;
    serializeJson(doc, output);
    return output;
}

String WebServerManager::getTemperatureJSON() {
    StaticJsonDocument<256> doc;
    
    JsonObject hotend = doc.createNestedObject("hotend");
    hotend["current"] = heater_controller->getTemperature(HEATER_HOTEND);
    hotend["target"] = heater_controller->getTargetTemperature(HEATER_HOTEND);
    
    JsonObject bed = doc.createNestedObject("bed");
    bed["current"] = heater_controller->getTemperature(HEATER_BED);
    bed["target"] = heater_controller->getTargetTemperature(HEATER_BED);
    
    String output;
    serializeJson(doc, output);
    return output;
}

void WebServerManager::update() {
    // Non-blocking WebSocket cleanup with timeout protection
    static unsigned long last_cleanup = 0;
    unsigned long now = millis();
    
    // Limit cleanup frequency to prevent blocking
    if (now - last_cleanup > 1000) {  // Max once per second
        ws->cleanupClients();
        last_cleanup = now;
    }
    
    // Update SD card manager if it's executing a file
    SDCardManager* sd = SDCardManager::getInstance();
    if (sd && (sd->isExecuting() || sd->isPaused())) {
        sd->update();
    }
}

void WebServerManager::broadcastStatus() {
    // Only broadcast if we have connected clients
    if (ws->count() > 0) {
        String status = getStatusJSON();
        
        // Check status size to prevent blocking on large messages
        if (status.length() < 2048) {
            ws->textAll(status);
        } else {
            Serial.println("Status message too large, skipping broadcast");
        }
    }
}

void WebServerManager::sendMessage(String msg) {
    ws->textAll(msg);
}

bool WebServerManager::isConnected() {
    return wifi_connected;
}

String WebServerManager::getIPAddress() {
    if (WIFI_AP_MODE) {
        return WiFi.softAPIP().toString();
    } else {
        return WiFi.localIP().toString();
    }
}

// ============================================================================
// SD Card REST API Handlers Implementation
// ============================================================================

void WebServerManager::handleSDDownload(AsyncWebServerRequest *request) {
    SDCardManager* sd = SDCardManager::getInstance();
    if (!sd || !sd->isInitialized()) {
        request->send(503, "text/plain", "SD card not available");
        return;
    }
    
    if (!request->hasParam("file")) {
        request->send(400, "text/plain", "No file specified");
        return;
    }
    
    String filename = request->getParam("file")->value();
    if (!filename.startsWith("/")) {
        filename = "/" + filename;
    }
    
    if (!sd->fileExists(filename)) {
        request->send(404, "text/plain", "File not found");
        return;
    }
    
    request->send(SD, filename, "application/octet-stream");
}

// ============================================================================
// LittleFS REST API Handlers
// ============================================================================

void WebServerManager::handleLittleFSList(AsyncWebServerRequest *request) {
    StaticJsonDocument<4096> doc;
    JsonArray files = doc.createNestedArray("files");
    
    File root = LittleFS.open("/gcode");
    if (!root) {
        LittleFS.mkdir("/gcode");
        root = LittleFS.open("/gcode");
    }
    
    if (!root || !root.isDirectory()) {
        request->send(500, "application/json", "{\"error\":\"Cannot open gcode directory\"}");
        return;
    }
    
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String name = String(file.name());
            if (name.endsWith(".gcode") || name.endsWith(".nc") || name.endsWith(".txt")) {
                JsonObject fileObj = files.createNestedObject();
                int lastSlash = name.lastIndexOf('/');
                if (lastSlash >= 0) {
                    name = name.substring(lastSlash + 1);
                }
                fileObj["name"] = name;
                fileObj["size"] = file.size();
            }
        }
        file = root.openNextFile();
    }
    root.close();
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void WebServerManager::handleLittleFSUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    static File uploadFile;
    const size_t MAX_LITTLEFS_SIZE = 100 * 1024;
    
    if (index == 0) {
        if (filename.length() == 0 || filename.length() > 64) {
            Serial.println("Invalid filename");
            return;
        }
        
        Serial.printf("LittleFS upload start: %s\n", filename.c_str());
        
        if (!LittleFS.exists("/gcode")) {
            LittleFS.mkdir("/gcode");
        }
        
        String path = "/gcode/" + filename;
        uploadFile = LittleFS.open(path, "w");
        
        if (!uploadFile) {
            Serial.println("Failed to open file for writing");
            return;
        }
    }
    
    if (index + len > MAX_LITTLEFS_SIZE) {
        Serial.println("File too large for LittleFS");
        if (uploadFile) {
            uploadFile.close();
        }
        return;
    }
    
    if (uploadFile) {
        size_t written = uploadFile.write(data, len);
        if (written != len) {
            Serial.println("Write error");
        }
    }
    
    if (final) {
        if (uploadFile) {
            uploadFile.flush();
            uploadFile.close();
        }
        Serial.printf("Upload complete: %s (%u bytes)\n", filename.c_str(), index + len);
    }
}

void WebServerManager::handleLittleFSDownload(AsyncWebServerRequest *request) {
    if (!request->hasParam("file")) {
        request->send(400, "text/plain", "No file specified");
        return;
    }
    
    String filename = request->getParam("file")->value();
    String path = "/gcode/" + filename;
    
    if (!LittleFS.exists(path)) {
        request->send(404, "text/plain", "File not found");
        return;
    }
    
    request->send(LittleFS, path, "application/octet-stream");
}

void WebServerManager::handleLittleFSDelete(AsyncWebServerRequest *request) {
    if (!request->hasParam("file", true)) {
        request->send(400, "application/json", "{\"error\":\"No file specified\"}");
        return;
    }
    
    String filename = request->getParam("file", true)->value();
    String path = "/gcode/" + filename;
    
    if (LittleFS.remove(path)) {
        request->send(200, "application/json", "{\"status\":\"deleted\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Delete failed\"}");
    }
}

void WebServerManager::handleLittleFSPrint(AsyncWebServerRequest *request) {
    if (!request->hasParam("file")) {
        request->send(400, "application/json", "{\"error\":\"No file specified\"}");
        return;
    }
    
    String filename = request->getParam("file")->value();
    String path = "/gcode/" + filename;
    
    if (!LittleFS.exists(path)) {
        request->send(404, "application/json", "{\"error\":\"File not found\"}");
        return;
    }
    
    File file = LittleFS.open(path, "r");
    if (!file) {
        request->send(500, "application/json", "{\"error\":\"Cannot open file\"}");
        return;
    }
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() > 0 && !line.startsWith(";")) {
            gcode_parser->processLine(line);
        }
    }
    file.close();
    
    request->send(200, "application/json", "{\"status\":\"printing\"}");
}


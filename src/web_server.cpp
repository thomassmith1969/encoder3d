#include "web_server.h"

WebServerManager::WebServerManager(ThermalManager* t, StreamBufferHandle_t* stream) {
    thermal = t;
    gcodeStream = stream;
    server = new WebServer(80);
    ws = new WebSocketsServer(81);
    telnetServer = new WiFiServer(23); // Telnet Port
    dnsServer = new DNSServer();
    isAPMode = false;
}

void WebServerManager::begin() {
    setupFileSystem();
    setupWiFi(); // Initialize Network
    setupRoutes();
    
    ws->begin();
    ws->onEvent([this](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
        this->onWebSocketEvent(num, type, payload, length);
    });
    
    server->begin();
    telnetServer->begin();
    telnetServer->setNoDelay(true);
}

void WebServerManager::update() {
    if (isAPMode) {
        dnsServer->processNextRequest();
    }
    server->handleClient();
    ws->loop();
    handleTelnet();
}

void WebServerManager::handleTelnet() {
    // Check for new clients
    if (telnetServer->hasClient()) {
        if (!telnetClient || !telnetClient.connected()) {
            if (telnetClient) telnetClient.stop();
            telnetClient = telnetServer->available();
            telnetClient.println("Encoder3D Telnet Connected");
            telnetClient.flush();
        } else {
            // Reject multiple clients
            WiFiClient reject = telnetServer->available();
            reject.stop();
        }
    }

    // Read Data
    if (telnetClient && telnetClient.connected() && telnetClient.available()) {
        while (telnetClient.available()) {
            char c = telnetClient.read();
            xStreamBufferSend(*gcodeStream, &c, 1, 0);
        }
    }
}

void WebServerManager::setupWiFi() {
    Preferences prefs;
    prefs.begin("wifi", true); // Read-only
    String ssid = prefs.getString("ssid", "");
    String pass = prefs.getString("pass", "");
    deviceHostname = prefs.getString("hostname", "encoder3d");
    prefs.end();

    if (ssid.length() > 0) {
        Serial.print("Connecting to "); Serial.println(ssid);
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(deviceHostname.c_str());
        WiFi.begin(ssid.c_str(), pass.c_str());
        
        // Wait up to 10s for connection
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
            isAPMode = false;
            
            // Start mDNS after connection
            if (MDNS.begin(deviceHostname.c_str())) {
                Serial.println("MDNS responder started: " + deviceHostname);
                MDNS.addService("http", "tcp", 80);
            }
            return;
        }
    }

    // Fallback to AP Mode
    Serial.println("\nStarting AP Mode...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(deviceHostname.c_str());
    Serial.println("AP IP: " + WiFi.softAPIP().toString());
    
    // Start mDNS in AP Mode
    if (MDNS.begin(deviceHostname.c_str())) {
        Serial.println("MDNS responder started: " + deviceHostname);
        MDNS.addService("http", "tcp", 80);
    }
    
    // Start DNS Server for Captive Portal (Redirect all to AP IP)
    dnsServer->start(53, "*", WiFi.softAPIP());
    isAPMode = true;
}

void WebServerManager::setupFileSystem() {
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }
}

void WebServerManager::setupRoutes() {
    // API: Scan Networks (Always available)
    server->on("/api/wifi/scan", HTTP_GET, [this]() {
        int n = WiFi.scanNetworks();
        JsonDocument doc;
        JsonArray array = doc.to<JsonArray>();
        for (int i = 0; i < n; ++i) {
            JsonObject obj = array.add<JsonObject>();
            obj["ssid"] = WiFi.SSID(i);
            obj["rssi"] = WiFi.RSSI(i);
            obj["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        }
        String output;
        serializeJson(doc, output);
        server->send(200, "application/json", output);
    });

    // API: Save WiFi (Always available)
    server->on("/api/wifi/save", HTTP_POST, [this]() {
        if (server->hasArg("ssid") && server->hasArg("pass")) {
            String ssid = server->arg("ssid");
            String pass = server->arg("pass");
            String hostname = server->hasArg("hostname") ? server->arg("hostname") : "encoder3d";

            // Save credentials
            Preferences prefs;
            prefs.begin("wifi", false);
            prefs.putString("ssid", ssid);
            prefs.putString("pass", pass);
            prefs.putString("hostname", hostname);
            prefs.end();

            // Attempt connection to verify and get IP
            WiFi.mode(WIFI_AP_STA); // Keep AP alive
            WiFi.setHostname(hostname.c_str());
            WiFi.begin(ssid.c_str(), pass.c_str());
            
            // Wait up to 10s
            unsigned long start = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
                delay(100);
            }

            JsonDocument doc;
            if (WiFi.status() == WL_CONNECTED) {
                doc["success"] = true;
                doc["ip"] = WiFi.localIP().toString();
                doc["hostname"] = hostname + ".local";
                String output;
                serializeJson(doc, output);
                server->send(200, "application/json", output);
                
                // Reboot after response is sent
                delay(500);
                ESP.restart();
            } else {
                doc["success"] = false;
                doc["message"] = "Connection failed. Credentials saved.";
                String output;
                serializeJson(doc, output);
                server->send(200, "application/json", output);
            }
        } else {
            server->send(400, "text/plain", "Missing SSID or Password");
        }
    });

    // WiFi Config Page (Always available)
    server->on("/wifi.html", HTTP_GET, [this]() {
        if (LittleFS.exists("/www/wifi.html")) {
            File file = LittleFS.open("/www/wifi.html", "r");
            server->streamFile(file, "text/html");
            file.close();
        } else {
            // Fallback if LittleFS fails
            server->send(200, "text/html", "<h1>WiFi Setup</h1><form action='/api/wifi/save' method='POST'>SSID: <input name='ssid'><br>Pass: <input name='pass'><br><input type='submit'></form>");
        }
    });

    // --- STA MODE ROUTES ---

    // G-Code Upload
    server->on("/api/upload", HTTP_POST, 
        [this]() { server->send(200, "text/plain", "Upload successful"); },
        [this]() { this->handleUpload(); }
    );

    // API: Status
    server->on("/api/status", HTTP_GET, [this]() {
        JsonDocument doc;
        doc["connected"] = true;
        doc["hostname"] = deviceHostname;
        String output;
        serializeJson(doc, output);
        server->send(200, "application/json", output);
    });
    
    // Default to index.html
    server->on("/", HTTP_GET, [this]() {
        if (LittleFS.exists("/www/index.html")) {
            File file = LittleFS.open("/www/index.html", "r");
            server->streamFile(file, "text/html");
            file.close();
        } else {
            server->send(200, "text/plain", "Encoder3D UI - Upload index.html to /www/");
        }
    });

    // Serve Static Files (Last to avoid intercepting API calls)
    server->serveStatic("/", LittleFS, "/www/");

    server->onNotFound([this]() {
        if (isAPMode) {
            // Redirect captive portal checks to index
            server->sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
            server->send(302, "text/plain", "");
        } else {
            server->send(404, "text/plain", "Not found");
        }
    });
}

void WebServerManager::handleUpload() {
    HTTPUpload& upload = server->upload();
    static File uploadFile;
    
    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        if (!filename.startsWith("/")) filename = "/" + filename;
        String path = "/gcode" + filename;
        // Ensure directory exists
        if (!LittleFS.exists("/gcode")) LittleFS.mkdir("/gcode");
        uploadFile = LittleFS.open(path, "w");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) uploadFile.close();
    }
}

void WebServerManager::onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
        // Assume payload is G-Code or JSON command
        // For now, treat as raw G-Code line
        if (length < 64) {
            xStreamBufferSend(*gcodeStream, payload, length, 0);
            char newline = '\n';
            xStreamBufferSend(*gcodeStream, &newline, 1, 0);
        }
    }
}

void WebServerManager::broadcastStatus(float x, float y, float z, float e) {
    JsonDocument doc;
    doc["x"] = x;
    doc["y"] = y;
    doc["z"] = z;
    doc["e"] = e;
    String output;
    serializeJson(doc, output);
    ws->broadcastTXT(output);
    
    // Optional: Send status to Telnet? 
    // Usually Telnet expects "ok" or "X:10.00 Y:..." format, not JSON.
    // For now, let's keep Telnet quiet unless requested (M114)
}

void WebServerManager::broadcastError(String message) {
    JsonDocument doc;
    doc["error"] = message;
    String output;
    serializeJson(doc, output);
    ws->broadcastTXT(output);
    
    if (telnetClient && telnetClient.connected()) {
        telnetClient.print("Error: ");
        telnetClient.println(message);
    }
}


#include "web_server.h"

// File-local locks
static portMUX_TYPE g_executorMux = portMUX_INITIALIZER_UNLOCKED;

WebServerManager::WebServerManager(ThermalManager* t, StreamBufferHandle_t* stream, QueueHandle_t* cmdQueue) {
    thermal = t;
    gcodeStream = stream;
    commandQueue = cmdQueue;
    server = new WebServer(80);
    ws = new WebSocketsServer(81);
    telnetServer = new WiFiServer(23); // Telnet Port
    dnsServer = new DNSServer();
    isAPMode = false;
    // nothing to initialize for waiters here
}

// Background worker args for enqueue waiting
struct EnqueueWaitArgs {
    WebServerManager* mgr;
    QueueHandle_t* cmdQueue;
    RawCommand cmd;
    uint32_t timeoutMs;
};

// Background task which waits for queue space and enqueues the command,
// then sends the appropriate response back to the originating client.
static void enqueueWaiterTask(void* pvParameters) {
    EnqueueWaitArgs* args = reinterpret_cast<EnqueueWaitArgs*>(pvParameters);
    WebServerManager* self = args->mgr;
    RawCommand cmd = args->cmd;
    uint32_t timeoutMs = args->timeoutMs;

    if (args->cmdQueue == nullptr) {
        Serial.printf("enqueueWaiterTask: cmdQueue == NULL for %d/%d -> error:busy\n", cmd.srcType, cmd.srcId);
        self->sendResponseToClient(cmd.srcType, cmd.srcId, String("error:busy"));
    } else {
        Serial.printf("enqueueWaiterTask: %d/%d waiting up to %u ms to enqueue\n", cmd.srcType, cmd.srcId, (unsigned)timeoutMs);
        BaseType_t ok = xQueueSend(*args->cmdQueue, &cmd, pdMS_TO_TICKS(timeoutMs));
        if (ok == pdTRUE) {
            Serial.printf("enqueueWaiterTask: %d/%d succeeded -> ok:queued\n", cmd.srcType, cmd.srcId);
            self->sendResponseToClient(cmd.srcType, cmd.srcId, String("ok:queued"));
        } else {
            // Timed out or failed
            Serial.printf("enqueueWaiterTask: %d/%d failed to enqueue -> error:busy\n", cmd.srcType, cmd.srcId);
            self->sendResponseToClient(cmd.srcType, cmd.srcId, String("error:busy"));
        }
    }

    // free args and exit task

    delete args;
    vTaskDelete(NULL);
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
        // accumulate into line buffer and push full lines as commands
        static char linebuf[256];
        static size_t idx = 0;
        while (telnetClient.available()) {
            char c = telnetClient.read();
            if (c == '\r') continue;
            if (c == '\n') {
                if (idx > 0) {
                    RawCommand rc;
                    rc.srcType = SRC_TELNET;
                    rc.srcId = 0; // single telnet
                    size_t l = min((size_t)127, idx);
                    memcpy(rc.line, linebuf, l);
                    rc.line[l] = '\0'; rc.len = l;
                    String reason = pushClientCommand(rc);
                    // Acknowledge to telnet client so they know it was queued/rejected.
                    // If the reply is 'pending' a background waiter will reply later
                    // so we DON'T send any immediate acknowledgment here.
                    if (reason.length() == 0) sendResponseToClient(rc.srcType, rc.srcId, String("ok:queued"));
                    else if (reason == "pending") {
                        // Intentionally don't respond; the background waiter will reply
                    } else {
                        sendResponseToClient(rc.srcType, rc.srcId, String("error:") + reason);
                    }
                    idx = 0;
                }
            } else {
                if (idx < sizeof(linebuf)-1) linebuf[idx++] = c;
            }
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

    // (no test-only HTTP endpoints are installed in production firmware)
    
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
        // For now, treat as raw G-Code line and forward to command queue
        RawCommand rc;
        rc.srcType = SRC_WEBSOCKET;
        rc.srcId = num;
        size_t l = min((size_t)127, length);
        memcpy(rc.line, payload, l);
        rc.line[l] = '\0'; rc.len = l;
        // push to command queue with ownership checks
        String reason = pushClientCommand(rc);
        if (reason.length() == 0) {
            sendResponseToClient(rc.srcType, rc.srcId, "ok:queued");
        } else if (reason == "pending") {
            // Background enqueue waiter will send final ack — don't reply now
        } else {
            sendResponseToClient(rc.srcType, rc.srcId, String("error:") + reason);
        }
    }
}

String WebServerManager::pushClientCommand(const RawCommand &cmd) {
    // Emergency commands (M112 / M999) should be accepted regardless of owner
    String s = String(cmd.line);
    s.toUpperCase();
    bool isEmergency = (s.indexOf("M112") != -1);
    bool isClear = (s.indexOf("M999") != -1);
    // Always queue motion commands — execution-time errors will be emitted by the control task
    bool isMotion = (s.indexOf("G0") != -1) || (s.indexOf("G1") != -1);
    // If executor is busy and owned by a different client, reject (unless emergency/clear)
    if (!isEmergency && !isClear && executorBusy && !(executorOwnerType == cmd.srcType && executorOwnerId == cmd.srcId)) {
        return String("busy");
    }
    // Reserve executor early: if this is a motion/homing/emergency command and
    // the executor is not already busy, claim executor ownership right now so
    // other clients will receive immediate error:busy when attempting to push.
    bool isHoming = (s.indexOf("G28") != -1);
    if ((isMotion || isHoming || isEmergency) && !executorBusy) {
        // claim ownership atomically to avoid races across cores
        portENTER_CRITICAL(&g_executorMux);
        if (!executorBusy) {
            executorBusy = true;
            executorOwnerType = cmd.srcType;
            executorOwnerId = cmd.srcId;
            Serial.printf("pushClientCommand: reserved executor for %d/%d at push-time\n", cmd.srcType, cmd.srcId);
        }
        portEXIT_CRITICAL(&g_executorMux);
    }
    // If there's no command queue, treat it as a busy/unavailable state
    if (commandQueue == nullptr) return String("busy");

    // Try to enqueue immediately (don't block the network task).
    BaseType_t ok = xQueueSend(*commandQueue, &cmd, 0);
    Serial.printf("pushClientCommand: incoming %d/%d executorBusy=%d immediateEnqueue=%d\n", cmd.srcType, cmd.srcId, (int)executorBusy, (int)(ok==pdTRUE));
    if (ok == pdTRUE) return String("");

    // Queue is full. If the requestor is the current executor owner, spawn a
    // background enqueue waiter so we don't block the network thread waiting
    // for space. If the requester isn't the owner, return busy immediately.
    if (executorBusy && (executorOwnerType == cmd.srcType && executorOwnerId == cmd.srcId)) {
        const uint32_t OWNER_WAIT_MS = 2000; // wait up to 2s for queue space

        EnqueueWaitArgs* args = new EnqueueWaitArgs();
        args->mgr = this;
        args->cmdQueue = commandQueue;
        args->cmd = cmd;
        args->timeoutMs = OWNER_WAIT_MS;

        BaseType_t created = xTaskCreate(
            enqueueWaiterTask,
            "enqueueWaiter",
            2048 / sizeof(portSTACK_TYPE),
            args,
            1,
            NULL
        );
        if (created != pdTRUE) {
            delete args;
            Serial.printf("pushClientCommand: failed to spawn waiter for %d/%d\n", cmd.srcType, cmd.srcId);
            return String("busy");
        }

        Serial.printf("pushClientCommand: waiter spawned for %d/%d (pending)\n", cmd.srcType, cmd.srcId);

        // Indicate that final response will be delivered later by the waiter
        return String("pending");
    }

    // Not executor owner or can't wait -> busy
    return String("busy");
}

void WebServerManager::sendResponseToClient(uint8_t srcType, int srcId, const String &msg) {
    if (srcType == SRC_WEBSOCKET) {
        if (!ws) return;
        // Convert plain responses (ok:..., error:..., warn:...) into
        // structured JSON so WebSocket clients always receive JSON.
        String status = "info";
        String detail = msg;
        if (msg.startsWith("ok:")) {
            status = "ok";
            detail = msg.substring(3);
        } else if (msg.startsWith("ok")) {
            status = "ok";
            detail = msg.length() > 2 ? msg.substring(3) : "";
        } else if (msg.startsWith("error:")) {
            status = "error";
            detail = msg.substring(6);
        } else if (msg.startsWith("warn:")) {
            status = "warn";
            detail = msg.substring(5);
        }

        // Build a safe JSON string. We don't expect quotes in the messages,
        // but escape double-quotes just in case.
        String raw = msg;
        raw.replace("\"", "\\\"");

        String out;
        out += "{\"type\":\"response\",\"status\":\"" + status + "\"";
        out += ",\"detail\":\"" + detail + "\"";
        out += ",\"raw\":\"" + raw + "\"}";
        ws->sendTXT(srcId, out.c_str());
    } else if (srcType == SRC_TELNET) {
        if (telnetClient && telnetClient.connected()) telnetClient.println(msg);
    } else { // serial
        Serial.println(msg);
    }
}

void WebServerManager::broadcastStatus(float x, float y, float z, float e, float extTemp, float bedTemp, float extTarget, float bedTarget,
                                      float setX, float setY, float setZ, float setE,
                                      unsigned long lastMoveX, unsigned long lastMoveY, unsigned long lastMoveZ, unsigned long lastMoveE) {
    JsonDocument doc;
    // positions (mm)
    doc["x"] = x;
    doc["y"] = y;
    doc["z"] = z;
    doc["e"] = e;
    // temperatures
    doc["extTemp"] = extTemp;
    doc["bedTemp"] = bedTemp;
    doc["extTarget"] = extTarget;
    doc["bedTarget"] = bedTarget;
    // setpoints (mm)
    doc["setX"] = setX;
    doc["setY"] = setY;
    doc["setZ"] = setZ;
    doc["setE"] = setE;
    // last movement (ms ago)
    doc["lastMoveX_ms"] = lastMoveX;
    doc["lastMoveY_ms"] = lastMoveY;
    doc["lastMoveZ_ms"] = lastMoveZ;
    doc["lastMoveE_ms"] = lastMoveE;
    // executor state
    doc["executor_busy"] = executorBusy;
    doc["executor_owner_type"] = (int)executorOwnerType;
    doc["executor_owner_id"] = executorOwnerId;
    String output;
    serializeJson(doc, output);
    ws->broadcastTXT(output);
}

void WebServerManager::sendTelnet(String message) {
    if (telnetClient && telnetClient.connected()) {
        telnetClient.println(message);
    }
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

void WebServerManager::broadcastWarning(String message) {
    JsonDocument doc;
    doc["warning"] = message;
    String output;
    serializeJson(doc, output);
    ws->broadcastTXT(output);

    if (telnetClient && telnetClient.connected()) {
        telnetClient.print("Warning: ");
        telnetClient.println(message);
    }
}


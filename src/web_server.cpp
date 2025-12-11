#include "web_server.h"
#include <SD.h>
#include <FS.h>

// Global executor spinlock (shared across translation units)
portMUX_TYPE g_executorMux = portMUX_INITIALIZER_UNLOCKED;

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

// Job streamer args (background task will stream G-Code file into gcodeStream)
struct JobStreamArgs {
    WebServerManager* mgr;
    StreamBufferHandle_t* gcodeStream;
    char filename[128];
    uint8_t storage; // STORAGE_LITTLEFS or STORAGE_SD
};

// File-scope job state
volatile bool jobActive = false;
static volatile bool jobStopRequested = false;
static char currentJobFile[128] = "";

// SD availability flag (attempt to init in setupFileSystem)
static bool sdAvailable = false;

// Executor reservation timeout (ms)
static const uint32_t EXECUTOR_RESERVATION_MS = 5000;
// Reservation expiry timestamp (millis) when set during push-time reservation.
volatile unsigned long executorReservedUntil = 0;

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

// Background task which streams a G-Code file into the gcode stream buffer.
// Runs off the network thread so file IO doesn't block HTTP handlers.
static void jobStreamerTask(void* pvParameters) {
    JobStreamArgs* args = reinterpret_cast<JobStreamArgs*>(pvParameters);
    WebServerManager* self = args->mgr;
    StreamBufferHandle_t* gcodeStream = args->gcodeStream;

    jobActive = true;
    jobStopRequested = false;
    strncpy(currentJobFile, args->filename, sizeof(currentJobFile)-1);
    currentJobFile[sizeof(currentJobFile)-1] = '\0';

    // Broadcast job started
    if (self) self->broadcastJobEvent("started", args->filename, (args->storage == STORAGE_SD) ? "sd" : "littlefs", -1);

    String path;
    File f;
    if (args->storage == STORAGE_SD) {
        path = String(args->filename); // SD paths are root-relative on SD
        Serial.printf("jobStreamer: starting job from SD %s\n", path.c_str());
        if (!sdAvailable) {
            Serial.printf("jobStreamer: SD not available for %s\n", path.c_str());
            if (self) self->broadcastError(String("Job open failed (SD not available): ") + path);
            jobActive = false;
            delete args;
            vTaskDelete(NULL);
            return;
        }
        f = SD.open(path, FILE_READ);
        if (!f) {
            Serial.printf("jobStreamer: failed to open SD %s\n", path.c_str());
            if (self) self->broadcastError(String("Job open failed: ") + path);
            if (self) self->broadcastJobEvent("error", args->filename, "sd", -1);
            jobActive = false;
            delete args;
            vTaskDelete(NULL);
            return;
        }
    } else {
        path = String("/gcode/") + String(args->filename);
        Serial.printf("jobStreamer: starting job %s\n", path.c_str());
        f = LittleFS.open(path, "r");
        if (!f) {
            Serial.printf("jobStreamer: failed to open %s\n", path.c_str());
            if (self) self->broadcastError(String("Job open failed: ") + path);
            if (self) self->broadcastJobEvent("error", args->filename, "littlefs", -1);
            jobActive = false;
            delete args;
            vTaskDelete(NULL);
            return;
        }
    }

    // Read line-by-line and push into gcode stream buffer. Honor stop request.
    char linebuf[256];
    while (f.available() && !jobStopRequested) {
        size_t len = f.readBytesUntil('\n', linebuf, sizeof(linebuf)-1);
        if (len == 0) { // skip empty lines
            continue;
        }
        linebuf[len] = '\0';
        // Send to gcode stream; block briefly to avoid busy-looping if buffer full
        if (gcodeStream != NULL) {
            xStreamBufferSend(*gcodeStream, linebuf, len + 1, pdMS_TO_TICKS(200));
        }
        // tiny pause to let parser pick up lines; this also yields CPU
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    f.close();
    // Broadcast job finished (unless stop requested was set)
    if (self) self->broadcastJobEvent(jobStopRequested ? "stopped" : "finished", args->filename, (args->storage == STORAGE_SD) ? "sd" : "littlefs", -1);
    jobActive = false;
    jobStopRequested = false;
    currentJobFile[0] = '\0';
    Serial.println("jobStreamer: finished");
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
    // Try to initialize SD (optional). If the board has no SD card or init fails,
    // we'll leave sdAvailable false and SD endpoints will report not available.
    sdAvailable = false;
    if (SD.begin()) {
        sdAvailable = true;
        Serial.println("SD card initialized");
    } else {
        Serial.println("SD init failed or not present");
    }
}

void WebServerManager::setupRoutes() {
    // API: Scan Networks (Always available)
    server->on("/api/wifi/scan", HTTP_GET, [this]() {
        int n = WiFi.scanNetworks();
        DynamicJsonDocument doc(1024);
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
        // Accept optional ssid/pass/hostname. If SSID is empty, only update MDNS/hostname.
        String ssid = server->hasArg("ssid") ? server->arg("ssid") : String("");
        String pass = server->hasArg("pass") ? server->arg("pass") : String("");
        String hostname = server->hasArg("hostname") ? server->arg("hostname") : String("");

        // Persist values (allow empty hostname)
        Preferences prefs;
        prefs.begin("wifi", false);
        prefs.putString("ssid", ssid);
        prefs.putString("pass", pass);
        prefs.putString("hostname", hostname);
        prefs.end();

        // Update in-memory hostname
        if (hostname.length() > 0) deviceHostname = hostname;
        else deviceHostname = String("");

        DynamicJsonDocument doc(256);

        if (ssid.length() == 0) {
            // No SSID provided: don't attempt to connect, just update mDNS handling
            if (hostname.length() > 0) {
                // start/refresh mDNS with provided hostname
                if (MDNS.begin(deviceHostname.c_str())) {
                    MDNS.addService("http", "tcp", 80);
                    doc["success"] = true;
                    doc["message"] = "Saved hostname and updated mDNS";
                    doc["hostname"] = deviceHostname + ".local";
                } else {
                    doc["success"] = false;
                    doc["message"] = "mDNS start failed";
                }
            } else {
                // Empty hostname: disable mDNS
                MDNS.end();
                doc["success"] = true;
                doc["message"] = "Saved empty hostname and disabled mDNS";
            }

            String output; serializeJson(doc, output);
            server->send(200, "application/json", output);
            return;
        }

        // SSID provided: attempt to connect (keep AP alive)
        WiFi.mode(WIFI_AP_STA);
        if (hostname.length() > 0) WiFi.setHostname(hostname.c_str());
        WiFi.begin(ssid.c_str(), pass.c_str());

        // Wait up to 10s for connection
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
            delay(100);
        }

        if (WiFi.status() == WL_CONNECTED) {
            // If hostname provided, ensure mDNS is started with it
            if (hostname.length() > 0) {
                MDNS.begin(hostname.c_str());
                MDNS.addService("http", "tcp", 80);
            }
            doc["success"] = true;
            doc["ip"] = WiFi.localIP().toString();
            doc["hostname"] = (hostname.length() > 0) ? hostname + ".local" : String("");
            String output; serializeJson(doc, output);
            server->send(200, "application/json", output);

            // Reboot to apply WiFi changes
            delay(500);
            ESP.restart();
        } else {
            // Save done but connection failed
            // If hostname provided, try to start mDNS locally (best-effort)
            if (hostname.length() > 0) {
                MDNS.begin(hostname.c_str());
                MDNS.addService("http", "tcp", 80);
            }
            doc["success"] = false;
            doc["message"] = "Connection failed. Credentials saved.";
            String output; serializeJson(doc, output);
            server->send(200, "application/json", output);
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
        DynamicJsonDocument doc(128);
        doc["connected"] = true;
        doc["hostname"] = deviceHostname;
        String output;
        serializeJson(doc, output);
        server->send(200, "application/json", output);
    });

    // API: Executor status + control
    server->on("/api/executor", HTTP_GET, [this]() {
        DynamicJsonDocument doc(256);
        doc["busy"] = executorBusy;
        doc["owner_type"] = (int)executorOwnerType;
        doc["owner_id"] = executorOwnerId;
        unsigned long now = millis();
        unsigned long remaining = 0;
        if (executorReservedUntil != 0 && executorReservedUntil > now) remaining = executorReservedUntil - now;
        doc["reserved_ms"] = remaining;
        String out; serializeJson(doc, out);
        server->send(200, "application/json", out);
    });

    server->on("/api/executor/release", HTTP_POST, [this]() {
        // Release executor ownership immediately (admin action)
        portENTER_CRITICAL(&g_executorMux);
        executorBusy = false;
        executorOwnerType = SRC_SERIAL;
        executorOwnerId = -1;
        executorReservedUntil = 0;
        portEXIT_CRITICAL(&g_executorMux);
        DynamicJsonDocument doc(128); doc["success"] = true; doc["released"] = true;
        String out; serializeJson(doc, out);
        server->send(200, "application/json", out);
    });

    // API: Config (get)
    server->on("/api/config", HTTP_GET, [this]() {
        DynamicJsonDocument doc(1024);
        JsonObject c = doc["countsPerMM"].to<JsonObject>();
        c["x"] = countsPerMM_X; c["y"] = countsPerMM_Y; c["z"] = countsPerMM_Z; c["e"] = countsPerMM_E;
        JsonObject pid = doc["pid"].to<JsonObject>();
        JsonObject px = pid["x"].to<JsonObject>(); px["p"] = pid_kp_x; px["i"] = pid_ki_x; px["d"] = pid_kd_x;
        JsonObject py = pid["y"].to<JsonObject>(); py["p"] = pid_kp_y; py["i"] = pid_ki_y; py["d"] = pid_kd_y;
        JsonObject pz = pid["z"].to<JsonObject>(); pz["p"] = pid_kp_z; pz["i"] = pid_ki_z; pz["d"] = pid_kd_z;
        JsonObject pe = pid["e"].to<JsonObject>(); pe["p"] = pid_kp_e; pe["i"] = pid_ki_e; pe["d"] = pid_kd_e;
        JsonObject mf = doc["maxFeedrate"].to<JsonObject>();
        mf["x"] = maxFeedrateX; mf["y"] = maxFeedrateY; mf["z"] = maxFeedrateZ; mf["e"] = maxFeedrateE;
        String output;
        serializeJson(doc, output);
        server->send(200, "application/json", output);
    });

    // API: Config (set)
    server->on("/api/config", HTTP_POST, [this]() {
        if (!server->hasArg("plain")) { server->send(400, "text/plain", "Missing body"); return; }
        String body = server->arg("plain");
        DynamicJsonDocument doc(1024);
        DeserializationError err = deserializeJson(doc, body);
        if (err) { server->send(400, "text/plain", "Invalid JSON"); return; }
        Preferences prefs; prefs.begin("cnc", false);
        if (doc["countsPerMM"].is<JsonObject>()) {
            JsonObject c = doc["countsPerMM"].as<JsonObject>();
            if (c["x"].is<float>()) { countsPerMM_X = c["x"].as<float>(); prefs.putFloat("cpm_x", countsPerMM_X); }
            if (c["y"].is<float>()) { countsPerMM_Y = c["y"].as<float>(); prefs.putFloat("cpm_y", countsPerMM_Y); }
            if (c["z"].is<float>()) { countsPerMM_Z = c["z"].as<float>(); prefs.putFloat("cpm_z", countsPerMM_Z); }
            if (c["e"].is<float>()) { countsPerMM_E = c["e"].as<float>(); prefs.putFloat("cpm_e", countsPerMM_E); }
        }
        if (doc["pid"].is<JsonObject>()) {
            JsonObject p = doc["pid"].as<JsonObject>();
            if (p["x"].is<JsonObject>()) {
                JsonObject px = p["x"].as<JsonObject>();
                if (px["p"].is<float>()) pid_kp_x = px["p"].as<float>();
                if (px["i"].is<float>()) pid_ki_x = px["i"].as<float>();
                if (px["d"].is<float>()) pid_kd_x = px["d"].as<float>();
                prefs.putFloat("pid_kp_x", pid_kp_x);
                prefs.putFloat("pid_ki_x", pid_ki_x);
                prefs.putFloat("pid_kd_x", pid_kd_x);
                pidX.setTunings(pid_kp_x, pid_ki_x, pid_kd_x);
            }
            if (p["y"].is<JsonObject>()) {
                JsonObject py = p["y"].as<JsonObject>();
                if (py["p"].is<float>()) pid_kp_y = py["p"].as<float>();
                if (py["i"].is<float>()) pid_ki_y = py["i"].as<float>();
                if (py["d"].is<float>()) pid_kd_y = py["d"].as<float>();
                prefs.putFloat("pid_kp_y", pid_kp_y);
                prefs.putFloat("pid_ki_y", pid_ki_y);
                prefs.putFloat("pid_kd_y", pid_kd_y);
                pidY.setTunings(pid_kp_y, pid_ki_y, pid_kd_y);
            }
            if (p["z"].is<JsonObject>()) {
                JsonObject pz = p["z"].as<JsonObject>();
                if (pz["p"].is<float>()) pid_kp_z = pz["p"].as<float>();
                if (pz["i"].is<float>()) pid_ki_z = pz["i"].as<float>();
                if (pz["d"].is<float>()) pid_kd_z = pz["d"].as<float>();
                prefs.putFloat("pid_kp_z", pid_kp_z);
                prefs.putFloat("pid_ki_z", pid_ki_z);
                prefs.putFloat("pid_kd_z", pid_kd_z);
                pidZ.setTunings(pid_kp_z, pid_ki_z, pid_kd_z);
            }
            if (p["e"].is<JsonObject>()) {
                JsonObject pe = p["e"].as<JsonObject>();
                if (pe["p"].is<float>()) pid_kp_e = pe["p"].as<float>();
                if (pe["i"].is<float>()) pid_ki_e = pe["i"].as<float>();
                if (pe["d"].is<float>()) pid_kd_e = pe["d"].as<float>();
                prefs.putFloat("pid_kp_e", pid_kp_e);
                prefs.putFloat("pid_ki_e", pid_ki_e);
                prefs.putFloat("pid_kd_e", pid_kd_e);
                pidE.setTunings(pid_kp_e, pid_ki_e, pid_kd_e);
            }
        }
        if (doc["maxFeedrate"].is<JsonObject>()) {
            JsonObject m = doc["maxFeedrate"].as<JsonObject>();
            if (m["x"].is<int>()) { maxFeedrateX = m["x"].as<int>(); prefs.putInt("maxF_x", maxFeedrateX); }
            if (m["y"].is<int>()) { maxFeedrateY = m["y"].as<int>(); prefs.putInt("maxF_y", maxFeedrateY); }
            if (m["z"].is<int>()) { maxFeedrateZ = m["z"].as<int>(); prefs.putInt("maxF_z", maxFeedrateZ); }
            if (m["e"].is<int>()) { maxFeedrateE = m["e"].as<int>(); prefs.putInt("maxF_e", maxFeedrateE); }
        }
        prefs.end();
        server->send(200, "application/json", "{\"success\":true}");
    });

    // API: Files listing. Accept optional query arg `storage=sd|littlefs` (default littlefs).
    server->on("/api/files", HTTP_GET, [this]() {
        String storageArg = server->hasArg("storage") ? server->arg("storage") : String("littlefs");
        DynamicJsonDocument res(2048);
        JsonArray arr = res.to<JsonArray>();
        if (storageArg == "sd") {
            if (!sdAvailable) {
                server->send(503, "application/json", "{\"error\":\"sd not available\"}");
                return;
            }
            File root = SD.open("/");
            if (root) {
                File file = root.openNextFile();
                while (file) {
                    String name = file.name();
                    JsonObject o = arr.createNestedObject();
                    o["name"] = name;
                    o["size"] = file.size();
                    o["storage"] = "sd";
                    file = root.openNextFile();
                }
            }
        } else {
            // littlefs: list /gcode
            File root = LittleFS.open("/gcode");
            if (root) {
                File file = root.openNextFile();
                while (file) {
                    String name = file.name();
                    if (name.startsWith("/gcode/")) name = name.substring(7);
                    JsonObject o = arr.createNestedObject();
                    o["name"] = name;
                    o["size"] = file.size();
                    o["storage"] = "littlefs";
                    file = root.openNextFile();
                }
            }
        }
        String out; serializeJson(res, out);
        server->send(200, "application/json", out);
    });

    // API: Download a file (GET) - query args: storage=sd|littlefs, filename=<name>
    server->on("/api/files/download", HTTP_GET, [this]() {
        String storageArg = server->hasArg("storage") ? server->arg("storage") : String("littlefs");
        String filename = server->hasArg("filename") ? server->arg("filename") : String("");
        if (filename.length() == 0) { server->send(400, "text/plain", "Missing filename"); return; }
        if (storageArg == "sd") {
            if (!sdAvailable) { server->send(503, "text/plain", "SD not available"); return; }
            File f = SD.open(filename, FILE_READ);
            if (!f) { server->send(404, "text/plain", "File not found"); return; }
            server->streamFile(f, "application/octet-stream");
            f.close();
            return;
        } else {
            String path = String("/gcode/") + filename;
            if (!LittleFS.exists(path)) { server->send(404, "text/plain", "File not found"); return; }
            File f = LittleFS.open(path, "r");
            server->streamFile(f, "application/octet-stream");
            f.close();
            return;
        }
    });

    // API: Delete file (POST) body JSON { storage: "sd"|"littlefs", filename: "..." }
    server->on("/api/files/delete", HTTP_POST, [this]() {
        if (!server->hasArg("plain")) { server->send(400, "text/plain", "Missing body"); return; }
        String body = server->arg("plain");
        DynamicJsonDocument doc(256);
        if (deserializeJson(doc, body)) { server->send(400, "text/plain", "Invalid JSON"); return; }
        String storage = doc["storage"].is<const char*>() ? String((const char*)doc["storage"]) : String("littlefs");
        String filename = doc["filename"].is<const char*>() ? String((const char*)doc["filename"]) : String("");
        if (filename.length() == 0) { server->send(400, "text/plain", "Missing filename"); return; }
        if (storage == "sd") {
            if (!sdAvailable) { server->send(503, "text/plain", "SD not available"); return; }
            bool ok = SD.remove(filename);
            DynamicJsonDocument res(128); res["success"] = ok; String out; serializeJson(res, out); server->send(200, "application/json", out); return;
        } else {
            String path = String("/gcode/") + filename;
            bool ok = LittleFS.remove(path);
            DynamicJsonDocument res(128); res["success"] = ok; String out; serializeJson(res, out); server->send(200, "application/json", out); return;
        }
    });

    // Serve Files page
    server->on("/files.html", HTTP_GET, [this]() {
        if (LittleFS.exists("/www/files.html")) {
            File file = LittleFS.open("/www/files.html", "r");
            server->streamFile(file, "text/html");
            file.close();
        } else {
            server->send(200, "text/html", "<h1>Files</h1><p>Upload files to /gcode</p>");
        }
    });

    // API: Run controls
    server->on("/api/run", HTTP_GET, [this]() {
        DynamicJsonDocument doc(256);
        doc["paused"] = runPaused;
        doc["stopped"] = runStopped;
        doc["speedPercent"] = runSpeedPercent;
        doc["speedMultiplier"] = runSpeedMultiplier;
        String out; serializeJson(doc, out);
        server->send(200, "application/json", out);
    });

    server->on("/api/run/pause", HTTP_POST, [this]() {
        runPaused = true;
        if (webServer) webServer->broadcastWarning("Run paused");
        server->send(200, "application/json", "{\"success\":true,\"state\":\"paused\"}");
    });

    server->on("/api/run/play", HTTP_POST, [this]() {
        runPaused = false;
        // clear runStopped if it was set
        runStopped = false;
        if (webServer) webServer->broadcastStatus(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0, 0,0,0,0); // quick status ping (zeros for motor outputs and enc counts)
        server->send(200, "application/json", "{\"success\":true,\"state\":\"running\"}");
    });

    server->on("/api/run/stop", HTTP_POST, [this]() {
        // Signal immediate stop; controlTask will clear queues and release executor
        runStopped = true;
        if (webServer) webServer->broadcastError("Run stopped");
        server->send(200, "application/json", "{\"success\":true,\"state\":\"stopped\"}");
    });

    server->on("/api/run/speed", HTTP_POST, [this]() {
        int percent = -1;
        // Accept JSON body or form arg
        if (server->hasArg("plain")) {
            String body = server->arg("plain");
                DynamicJsonDocument doc(256);
            if (!deserializeJson(doc, body)) {
                if (doc["speedPercent"].is<int>()) percent = doc["speedPercent"].as<int>();
            }
        }
        if (percent == -1 && server->hasArg("speedPercent")) {
            percent = server->arg("speedPercent").toInt();
        }
        if (percent < 5) percent = 5;
        if (percent > 500) percent = 500;
        runSpeedPercent = percent;
        runSpeedMultiplier = ((float)runSpeedPercent) / 100.0f;
        DynamicJsonDocument res(128);
        res["success"] = true;
        res["speedPercent"] = runSpeedPercent;
        res["speedMultiplier"] = runSpeedMultiplier;
        String out; serializeJson(res, out);
        server->send(200, "application/json", out);
    });

    // API: Spindle control
    server->on("/api/spindle", HTTP_GET, [this]() {
        DynamicJsonDocument res(128);
        int current = 0;
        if (PIN_SPINDLE >= 0) current = ledcRead(PWM_CHAN_SPINDLE);
        res["power"] = current;
        res["available"] = (PIN_SPINDLE >= 0);
        String out; serializeJson(res, out);
        server->send(200, "application/json", out);
    });

    server->on("/api/spindle", HTTP_POST, [this]() {
        int val = -1;
        if (server->hasArg("plain")) {
            String body = server->arg("plain");
            DynamicJsonDocument doc(128);
            if (!deserializeJson(doc, body)) {
                if (doc["power"].is<int>()) val = doc["power"].as<int>();
            }
        }
        if (val == -1 && server->hasArg("power")) val = server->arg("power").toInt();
        if (val < 0) { server->send(400, "text/plain", "Missing power"); return; }
        if (val < 0) val = 0; if (val > 255) val = 255;
        if (PIN_SPINDLE >= 0) ledcWrite(PWM_CHAN_SPINDLE, val);
        Preferences prefs; prefs.begin("cnc", false); prefs.putInt("spindle_p", val); prefs.end();
        DynamicJsonDocument res(128);
        res["success"] = true; res["power"] = val;
        String out; serializeJson(res, out);
        server->send(200, "application/json", out);
    });

    // API: Laser control
    server->on("/api/laser", HTTP_GET, [this]() {
        DynamicJsonDocument res(128);
        int current = 0;
        if (PIN_LASER >= 0) current = ledcRead(PWM_CHAN_LASER);
        res["power"] = current;
        res["available"] = (PIN_LASER >= 0);
        String out; serializeJson(res, out);
        server->send(200, "application/json", out);
    });

    server->on("/api/laser", HTTP_POST, [this]() {
        int val = -1;
        if (server->hasArg("plain")) {
            String body = server->arg("plain");
            DynamicJsonDocument doc(128);
            if (!deserializeJson(doc, body)) {
                if (doc["power"].is<int>()) val = doc["power"].as<int>();
            }
        }
        if (val == -1 && server->hasArg("power")) val = server->arg("power").toInt();
        if (val < 0) { server->send(400, "text/plain", "Missing power"); return; }
        if (val < 0) val = 0; if (val > 255) val = 255;
        if (PIN_LASER >= 0) ledcWrite(PWM_CHAN_LASER, val);
        Preferences prefs; prefs.begin("cnc", false); prefs.putInt("laser_p", val); prefs.end();
        DynamicJsonDocument res(128);
        res["success"] = true; res["power"] = val;
        String out; serializeJson(res, out);
        server->send(200, "application/json", out);
    });

    // Job control: start/stop/status
    server->on("/api/job", HTTP_GET, [this]() {
        DynamicJsonDocument res(256);
        res["active"] = jobActive;
        res["filename"] = String(currentJobFile);
        String out; serializeJson(res, out);
        server->send(200, "application/json", out);
    });

    server->on("/api/job/start", HTTP_POST, [this]() {
        String filename;
        String storageArg = String("littlefs");
        if (server->hasArg("plain")) {
            String body = server->arg("plain");
            DynamicJsonDocument doc(256);
            if (!deserializeJson(doc, body)) {
                if (doc["filename"].is<const char*>()) filename = String((const char*)doc["filename"]);
                if (doc["storage"].is<const char*>()) storageArg = String((const char*)doc["storage"]);
                // optional: reserve executor for the job so other clients cannot steal ownership
                bool reserveJob = false;
                if (doc["reserve"].is<bool>()) reserveJob = doc["reserve"].as<bool>();
                if (reserveJob) {
                    // attempt to reserve executor for serial owner (src=SRC_SERIAL, id=0)
                    portENTER_CRITICAL(&g_executorMux);
                    if (executorBusy) {
                        portEXIT_CRITICAL(&g_executorMux);
                        server->send(409, "application/json", "{\"success\":false,\"message\":\"executor busy\"}");
                        return;
                    }
                    executorBusy = true;
                    executorOwnerType = SRC_SERIAL;
                    executorOwnerId = 0;
                    // leave executorReservedUntil as 0 to indicate an indefinite reservation
                    executorReservedUntil = 0;
                    portEXIT_CRITICAL(&g_executorMux);
                }
            }
        }
        if (filename.length() == 0 && server->hasArg("filename")) filename = server->arg("filename");
        if (server->hasArg("storage")) storageArg = server->arg("storage");
        uint8_t storage = STORAGE_LITTLEFS;
        if (storageArg == "sd") storage = STORAGE_SD;
        if (filename.length() == 0) { server->send(400, "text/plain", "Missing filename"); return; }

        // Verify file exists in selected storage
        if (storage == STORAGE_LITTLEFS) {
            String path = String("/gcode/") + filename;
            if (!LittleFS.exists(path)) { server->send(404, "text/plain", "File not found"); return; }
        } else {
            if (!sdAvailable) { server->send(503, "text/plain", "SD not available"); return; }
            if (!SD.exists(filename)) { server->send(404, "text/plain", "File not found on SD"); return; }
        }

        // Spawn background streamer task
        JobStreamArgs* args = new JobStreamArgs();
        args->mgr = this; args->gcodeStream = gcodeStream; strncpy(args->filename, filename.c_str(), sizeof(args->filename)-1); args->filename[sizeof(args->filename)-1] = '\0';
        args->storage = storage;
        BaseType_t created = xTaskCreate(
            jobStreamerTask,
            "jobStreamer",
            4096 / sizeof(portSTACK_TYPE),
            args,
            1,
            NULL
        );
        if (created != pdTRUE) {
            delete args;
            server->send(500, "text/plain", "Failed to start job streamer");
            return;
        }
        DynamicJsonDocument res(128); res["success"] = true; res["started"] = true; res["filename"] = filename;
        // echo reservation state
        res["reserved"] = (executorBusy && executorOwnerType == SRC_SERIAL && executorOwnerId == 0);
        String out; serializeJson(res, out);
        server->send(200, "application/json", out);
    });

    server->on("/api/job/stop", HTTP_POST, [this]() {
        // Request streamer to stop and signal controlTask to stop current motion
        jobStopRequested = true;
        runStopped = true; // controlTask will clear queues and release executor
        DynamicJsonDocument res(128); res["success"] = true; res["stopping"] = true;
        String out; serializeJson(res, out);
        server->send(200, "application/json", out);
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
    // Clear expired reservation if present
    portENTER_CRITICAL(&g_executorMux);
    if (executorBusy && executorReservedUntil != 0 && millis() > executorReservedUntil) {
        // reservation expired -> release
        executorBusy = false;
        executorOwnerType = SRC_SERIAL;
        executorOwnerId = -1;
        executorReservedUntil = 0;
        Serial.println("pushClientCommand: executor reservation expired -> released");
    }
    portEXIT_CRITICAL(&g_executorMux);

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
            // set a short reservation window; controlTask will claim executor properly
            executorReservedUntil = millis() + EXECUTOR_RESERVATION_MS;
            Serial.printf("pushClientCommand: reserved executor for %d/%d at push-time (reservedUntil=%lu)\n", cmd.srcType, cmd.srcId, executorReservedUntil);
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
                                      unsigned long lastMoveX, unsigned long lastMoveY, unsigned long lastMoveZ, unsigned long lastMoveE,
                                      int motorOutX, int motorOutY, int motorOutZ, int motorOutE,
                                      long encCountsX, long encCountsY, long encCountsZ, long encCountsE) {
    DynamicJsonDocument doc(384);
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
    // motor outputs (signed -255..255)
    doc["motorOutX"] = motorOutX;
    doc["motorOutY"] = motorOutY;
    doc["motorOutZ"] = motorOutZ;
    doc["motorOutE"] = motorOutE;
    // raw encoder counts for diagnostic inspection
    doc["encCountsX"] = encCountsX;
    doc["encCountsY"] = encCountsY;
    doc["encCountsZ"] = encCountsZ;
    doc["encCountsE"] = encCountsE;
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
    DynamicJsonDocument doc(128);
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
    DynamicJsonDocument doc(128);
    doc["warning"] = message;
    String output;
    serializeJson(doc, output);
    ws->broadcastTXT(output);

    if (telnetClient && telnetClient.connected()) {
        telnetClient.print("Warning: ");
        telnetClient.println(message);
    }
}

void WebServerManager::broadcastJobEvent(const char* event, const char* filename, const char* storage, int progress) {
    DynamicJsonDocument doc(256);
    doc["type"] = "job_event";
    doc["event"] = String(event);
    doc["filename"] = String(filename);
    doc["storage"] = String(storage);
    if (progress >= 0) doc["progress"] = progress;
    String output; serializeJson(doc, output);
    ws->broadcastTXT(output);

    if (telnetClient && telnetClient.connected()) {
        telnetClient.print("JobEvent: "); telnetClient.print(event); telnetClient.print(" "); telnetClient.println(filename);
    }
}


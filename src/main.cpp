#include <Arduino.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/stream_buffer.h>
#include <freertos/queue.h>
#include "config.h"
#include "motor_driver.h"
#include "pid_controller.h"
#include "thermal.h"
#include "web_server.h"

// --- GLOBAL OBJECTS ---
MotorDriver motorX(PIN_X_MOTOR_A, PIN_X_MOTOR_B, PWM_CHAN_X);
MotorDriver motorY(PIN_Y_MOTOR_A, PIN_Y_MOTOR_B, PWM_CHAN_Y);
MotorDriver motorZ(PIN_Z_MOTOR_A, PIN_Z_MOTOR_B, PWM_CHAN_Z);
MotorDriver motorE(PIN_E_MOTOR_A, PIN_E_MOTOR_B, PWM_CHAN_E);

PIDController pidX(KP_DEFAULT, KI_DEFAULT, KD_DEFAULT);
PIDController pidY(KP_DEFAULT, KI_DEFAULT, KD_DEFAULT);
PIDController pidZ(KP_DEFAULT, KI_DEFAULT, KD_DEFAULT);
PIDController pidE(KP_DEFAULT, KI_DEFAULT, KD_DEFAULT);

ThermalManager thermal;
WebServerManager* webServer = nullptr; // created in networkTask

// --- RTOS HANDLES ---
StreamBufferHandle_t gcodeStream = NULL;
QueueHandle_t motionQueue = NULL; // MotionCommand queue (Parser -> Control)
QueueHandle_t commandQueue = NULL; // RawCommand queue (Web/Telnet -> Parser)

// --- EXECUTOR / CLIENT STATE (shared with web_server) ---
volatile uint8_t executorOwnerType = SRC_SERIAL;
volatile int executorOwnerId = -1;
volatile bool executorBusy = false;

// --- ENCODER ISRs & state ---
volatile long encX = 0;
volatile long encY = 0;
volatile long encZ = 0;
volatile long encE = 0;

// For stall detection / diagnostics
long lastSeenEncX = 0, lastSeenEncY = 0, lastSeenEncZ = 0, lastSeenEncE = 0;
unsigned long lastEncChangeX = 0, lastEncChangeY = 0, lastEncChangeZ = 0, lastEncChangeE = 0;

// Last time we broadcasted position warnings (rate-limit)
unsigned long lastPosWarnX = 0, lastPosWarnY = 0, lastPosWarnZ = 0, lastPosWarnE = 0;

// Current desired positions (scaled counts)
long currentPosX = 0, currentPosY = 0, currentPosZ = 0, currentPosE = 0;

// Positioning mode
bool absolutePositioning = false; // Default to relative (G91) for simple jogs

// System flags
volatile bool isHalted = false;
String haltReason = "";

// Encoder presence flags - set when we observe any encoder edges
volatile bool encSeenX = false;
volatile bool encSeenY = false;
volatile bool encSeenZ = false;
volatile bool encSeenE = false;

void IRAM_ATTR isrX() {
    if (digitalRead(PIN_X_ENC_A) == digitalRead(PIN_X_ENC_B)) encX = encX + 1; else encX = encX - 1;
    encSeenX = true;
}
void IRAM_ATTR isrY() {
    if (digitalRead(PIN_Y_ENC_A) == digitalRead(PIN_Y_ENC_B)) encY = encY + 1; else encY = encY - 1;
    encSeenY = true;
}
void IRAM_ATTR isrZ() {
    if (digitalRead(PIN_Z_ENC_A) == digitalRead(PIN_Z_ENC_B)) encZ = encZ + 1; else encZ = encZ - 1;
    encSeenZ = true;
}
void IRAM_ATTR isrE() {
    if (digitalRead(PIN_E_ENC_A) == digitalRead(PIN_E_ENC_B)) encE = encE + 1; else encE = encE - 1;
    encSeenE = true;
}

// --- Network + Thermal tasks (Core 0)
void networkTask(void *pvParameters) {
    // Initialize WebServer (give it pointer to commandQueue)
    webServer = new WebServerManager(&thermal, &gcodeStream, &commandQueue);
    webServer->begin();

    char buffer[64];
    while (true) {
        // Handle Web Server & WebSockets
        webServer->update();

        // Broadcast status and/or errors periodically
        static unsigned long lastStatus = 0;
        if (millis() - lastStatus > 100) {
            if (isHalted) {
                webServer->broadcastError(haltReason);
            } else {
                // Send encoder positions (converted to mm), setpoints (mm), and last movement ages
                float encXm = encX / 100.0; float encYm = encY / 100.0; float encZm = encZ / 100.0; float encEm = encE / 100.0;
                float setXm = currentPosX / 100.0; float setYm = currentPosY / 100.0; float setZm = currentPosZ / 100.0; float setEm = currentPosE / 100.0;
                unsigned long now = millis();
                unsigned long lmX = now - lastEncChangeX;
                unsigned long lmY = now - lastEncChangeY;
                unsigned long lmZ = now - lastEncChangeZ;
                unsigned long lmE = now - lastEncChangeE;

                webServer->broadcastStatus(
                    encXm, encYm, encZm, encEm,
                    (float)thermal.getExtruderTemp(), (float)thermal.getBedTemp(),
                    (float)thermal.getExtruderTarget(), (float)thermal.getBedTarget(),
                    setXm, setYm, setZm, setEm,
                    lmX, lmY, lmZ, lmE);
            }

            // (networkTask) -- no feedrate clamping here
            lastStatus = millis();
        }

        // Serial Input (Legacy)
        if (Serial.available()) {
            int len = Serial.readBytesUntil('\n', buffer, 63);
            buffer[len] = '\0';
            xStreamBufferSend(gcodeStream, buffer, len + 1, 0);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

}

void thermalTask(void *pvParameters) {
    while (true) {
        thermal.update();
        vTaskDelay(100 / portTICK_PERIOD_MS); // 10Hz
    }
}

// Core 1: Motion Control
struct MotionCommand {
    long targetX, targetY, targetZ, targetE;
    bool hasX, hasY, hasZ, hasE; // which axes were specified
    bool isRelative;
    bool isHoming;
    bool isEmergency; // M112
    uint8_t ownerType; // SRC_*
    int ownerId;
    int feedrate; // mm/min (0 == unspecified / full)
};

void parserTask(void *pvParameters) {
    char buffer[64];
    MotionCommand cmd;
    // Initialize targets to 0 and clear axis flags
    cmd.targetX = 0; cmd.targetY = 0; cmd.targetZ = 0; cmd.targetE = 0;
    cmd.hasX = cmd.hasY = cmd.hasZ = cmd.hasE = false;
    cmd.isRelative = true; // Default to relative for jog buttons
    cmd.isHoming = false;

    // Wait for stream to be initialized
    while (gcodeStream == NULL || motionQueue == NULL) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    while (true) {
        // Prefer queued client commands over serial stream
        RawCommand raw;
        String line;
        if (commandQueue != NULL && xQueueReceive(commandQueue, &raw, (TickType_t)10) == pdTRUE) {
            // use raw.line as input
            line = String(raw.line);
            // We'll set owner info in the MotionCommand below (fall through to parsing)
            line.trim();
            // Normalize to uppercase so both 'X' and 'x' (and 'g','G') are parsed
            line.toUpperCase();
            if (line.length() == 0) continue;
        } else {
            size_t bytes = xStreamBufferReceive(gcodeStream, buffer, 64, portMAX_DELAY);
            if (bytes <= 0) continue;
            buffer[bytes] = '\0'; // Ensure null termination
            line = String(buffer);
            line.trim();
            // Normalize to uppercase so both 'X' and 'x' (and 'g','G') are parsed
            line.toUpperCase();
            if (line.length() == 0) continue;
            // default owner: serial
            raw.srcType = SRC_SERIAL;
            raw.srcId = 0;
            raw.len = bytes;
            strncpy(raw.line, buffer, min((size_t)127, bytes));
            raw.line[min((size_t)127, bytes)] = '\0';
        }

            // Parse G-Code
            if (line.startsWith("G1") || line.startsWith("G0")) {
                // Linear Move
                cmd.isHoming = false;
                cmd.hasX = cmd.hasY = cmd.hasZ = cmd.hasE = false;

                // Parse X
                int xIdx = line.indexOf('X');
                if (xIdx != -1) {
                    cmd.targetX = line.substring(xIdx + 1).toInt();
                    cmd.hasX = true;
                }

                // Parse Y
                int yIdx = line.indexOf('Y');
                if (yIdx != -1) {
                    cmd.targetY = line.substring(yIdx + 1).toInt();
                    cmd.hasY = true;
                }

                // Parse Z
                int zIdx = line.indexOf('Z');
                if (zIdx != -1) {
                    cmd.targetZ = line.substring(zIdx + 1).toInt();
                    cmd.hasZ = true;
                }

                // Parse E
                int eIdx = line.indexOf('E');
                if (eIdx != -1) {
                    cmd.targetE = line.substring(eIdx + 1).toInt();
                    cmd.hasE = true;
                }

                // Parse Feedrate F (optional)
                int fIdx = line.indexOf('F');
                if (fIdx != -1) {
                    cmd.feedrate = line.substring(fIdx + 1).toInt();
                } else {
                    cmd.feedrate = 0;
                }

                // set owner fields
                cmd.ownerType = raw.srcType;
                cmd.ownerId = raw.srcId;
                Serial.printf("parserTask: received raw from %d/%d -> enqueue motion\n", raw.srcType, raw.srcId);
                cmd.isEmergency = false;
                xQueueSend(motionQueue, &cmd, portMAX_DELAY);
            }
            else if (line.startsWith("G2") || line.startsWith("G3")) {
                // Arc move (XY plane). Support I and J center offsets (relative to start).
                bool clockwise = line.startsWith("G2");
                // Parse X/Y target (absolute or relative depending on mode)
                float targetXmm = NAN, targetYmm = NAN;
                int xIdx = line.indexOf('X');
                if (xIdx != -1) targetXmm = line.substring(xIdx + 1).toFloat();
                int yIdx = line.indexOf('Y');
                if (yIdx != -1) targetYmm = line.substring(yIdx + 1).toFloat();
                // Parse I/J center offsets
                float iOff = 0.0, jOff = 0.0;
                int iIdx = line.indexOf('I'); if (iIdx != -1) iOff = line.substring(iIdx + 1).toFloat();
                int jIdx = line.indexOf('J'); if (jIdx != -1) jOff = line.substring(jIdx + 1).toFloat();
                // Feedrate
                int fIdx2 = line.indexOf('F'); int feed = 0; if (fIdx2 != -1) feed = line.substring(fIdx2 + 1).toInt();

                // Determine start (current) position in mm
                float startXmm = currentPosX / 100.0f;
                float startYmm = currentPosY / 100.0f;
                // Resolve absolute/relative target
                float endXmm = isnan(targetXmm) ? startXmm : (absolutePositioning ? targetXmm : startXmm + targetXmm);
                float endYmm = isnan(targetYmm) ? startYmm : (absolutePositioning ? targetYmm : startYmm + targetYmm);

                // Compute center
                float cx = startXmm + iOff;
                float cy = startYmm + jOff;
                float r = hypot(startXmm - cx, startYmm - cy);
                if (r <= 0.0f) {
                    Serial.println("parserTask: arc radius zero -> ignoring");
                } else {
                    float startAng = atan2(startYmm - cy, startXmm - cx);
                    float endAng = atan2(endYmm - cy, endXmm - cx);
                    float delta = endAng - startAng;
                    if (clockwise && delta > 0) delta -= 2.0 * PI;
                    if (!clockwise && delta < 0) delta += 2.0 * PI;
                    float absDelta = fabs(delta);
                    int segments = max(8, (int)(r * absDelta));
                    for (int s = 1; s <= segments; ++s) {
                        float t = (float)s / (float)segments;
                        float ang = startAng + delta * t;
                        float px = cx + r * cos(ang);
                        float py = cy + r * sin(ang);
                        MotionCommand arcCmd;
                        arcCmd.isHoming = false; arcCmd.isEmergency = false;
                        arcCmd.ownerType = raw.srcType; arcCmd.ownerId = raw.srcId;
                        arcCmd.hasX = true; arcCmd.hasY = true; arcCmd.hasZ = false; arcCmd.hasE = false;
                        arcCmd.targetX = (long)round(px);
                        arcCmd.targetY = (long)round(py);
                        arcCmd.isRelative = false;
                        arcCmd.feedrate = feed;
                        xQueueSend(motionQueue, &arcCmd, portMAX_DELAY);
                    }
                }
            }
            else if (line.startsWith("G28")) {
                // Homing (reset encoders and position)
                cmd.isHoming = true;
                cmd.ownerType = raw.srcType;
                cmd.ownerId = raw.srcId;
                Serial.printf("parserTask: enqueue homing from %d/%d\n", raw.srcType, raw.srcId);
                cmd.isEmergency = false;
                xQueueSend(motionQueue, &cmd, portMAX_DELAY);
            }
            else if (line.startsWith("G90")) {
                // Absolute Positioning
                absolutePositioning = true;
            }
            else if (line.startsWith("G91")) {
                // Relative Positioning
                absolutePositioning = false;
            }
            else if (line.startsWith("M114")) {
                // Report Position
                char response[128];
                snprintf(response, sizeof(response), "X:%.2f Y:%.2f Z:%.2f E:%.2f\n",
                    currentPosX / 100.0, currentPosY / 100.0, currentPosZ / 100.0, currentPosE / 100.0);
                if (webServer) {
                    // Echo to serial and Telnet if connected
                    Serial.println(response);
                    webServer->sendTelnet(response);
                } else {
                    Serial.println(response);
                }
            }
            else if (line.startsWith("M104")) {
                // Set Extruder Temp
                int sIdx = line.indexOf('S');
                if (sIdx != -1) {
                    float temp = line.substring(sIdx + 1).toFloat();
                    thermal.setExtruderTarget(temp);
                }
            }
            else if (line.startsWith("M140")) {
                // Set Bed Temp
                int sIdx = line.indexOf('S');
                if (sIdx != -1) {
                    float temp = line.substring(sIdx + 1).toFloat();
                    thermal.setBedTarget(temp);
                }
            }
            else if (line.startsWith("M112")) {
                // Emergency Stop
                // pass emergency command into queue immediately
                cmd.isEmergency = true;
                cmd.ownerType = raw.srcType;
                cmd.ownerId = raw.srcId;
                xQueueSend(motionQueue, &cmd, portMAX_DELAY);
            }
            else if (line.startsWith("M999")) {
                // Clear Halt
                isHalted = false;
                haltReason = "";
            }
            else if (line.startsWith("G92")) {
                // Set current position (absolute) without moving motors
                int xIdx = line.indexOf('X');
                if (xIdx != -1) {
                    float vx = line.substring(xIdx + 1).toFloat();
                    currentPosX = (long)round(vx * 100.0);
                    encX = currentPosX;
                }
                int yIdx = line.indexOf('Y');
                if (yIdx != -1) {
                    float vy = line.substring(yIdx + 1).toFloat();
                    currentPosY = (long)round(vy * 100.0);
                    encY = currentPosY;
                }
                int zIdx = line.indexOf('Z');
                if (zIdx != -1) {
                    float vz = line.substring(zIdx + 1).toFloat();
                    currentPosZ = (long)round(vz * 100.0);
                    encZ = currentPosZ;
                }
                int eIdx2 = line.indexOf('E');
                if (eIdx2 != -1) {
                    float ve = line.substring(eIdx2 + 1).toFloat();
                    currentPosE = (long)round(ve * 100.0);
                    encE = currentPosE;
                }
            }
            else if (line.startsWith("M106") || line.startsWith("M107")) {
                // Fan control: M106 S<0-255>  or M107 (off)
                if (PIN_FAN < 0) {
                    Serial.println("M106/M107: fan pin not configured");
                } else {
                    if (line.startsWith("M107")) {
                        ledcWrite(PWM_CHAN_FAN, 0);
                    } else {
                        int sIdx = line.indexOf('S');
                        int val = 255;
                        if (sIdx != -1) val = line.substring(sIdx + 1).toInt();
                        if (val < 0) val = 0; if (val > 255) val = 255;
                        ledcWrite(PWM_CHAN_FAN, val);
                    }
                }
            }
            else if (line.startsWith("M105")) {
                // Temperature report
                char response[128];
                snprintf(response, sizeof(response), "ok T:%.1f / %.1f B:%.1f / %.1f",
                    thermal.getExtruderTemp(), thermal.getExtruderTarget(), thermal.getBedTemp(), thermal.getBedTarget());
                if (webServer) {
                    Serial.println(response);
                    webServer->sendTelnet(response);
                } else {
                    Serial.println(response);
                }
            }
            else if (line.startsWith("M3") || line.startsWith("M5")) {
                // Spindle / Laser on/off. M3 S<0-255> to set power, M5 to stop
                if (line.startsWith("M5")) {
                    if (PIN_SPINDLE >= 0) ledcWrite(PWM_CHAN_SPINDLE, 0);
                    if (PIN_LASER >= 0) ledcWrite(PWM_CHAN_LASER, 0);
                } else {
                    int sIdx = line.indexOf('S');
                    int val = 255;
                    if (sIdx != -1) val = line.substring(sIdx + 1).toInt();
                    if (PIN_SPINDLE >= 0) ledcWrite(PWM_CHAN_SPINDLE, constrain(val, 0, 255));
                    if (PIN_LASER >= 0) ledcWrite(PWM_CHAN_LASER, constrain(val, 0, 255));
                }
            }
        }
}

// Execution / Control Loop (Core 1) - single executor semantics
// declared above
void controlTask(void *pvParameters) {
    MotionCommand currentTarget = {0, 0, 0, 0, false, false, false, false, false, false};
    long setpointX = 0, setpointY = 0, setpointZ = 0, setpointE = 0;
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = 1; // 1ms = 1kHz

    // Wait for queue to be initialized
    while (motionQueue == NULL) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    while (true) {
        // If we're halted globally, stop motors and wait for clear
        if (isHalted) {
            // Stop all motors if halted
            motorX.setSpeed(0);
            motorY.setSpeed(0);
            motorZ.setSpeed(0);
            motorE.setSpeed(0);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        // Wait for next motion command (executor semantics)
        MotionCommand cmd;
        if (xQueueReceive(motionQueue, &cmd, portMAX_DELAY) == pdTRUE) {
            // Handle emergency commands immediately
            if (cmd.isEmergency) {
                isHalted = true;
                haltReason = "Emergency Stop (M112)";
                Serial.println("Emergency stop received");
                if (webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("ok:emergency"));
                // continue to next command
                continue;
            }

            // Claim executor if not busy
            if (!executorBusy) {
                executorBusy = true;
                executorOwnerType = cmd.ownerType;
                executorOwnerId = cmd.ownerId;
                Serial.printf("controlTask: claimed executor for %d/%d\n", cmd.ownerType, cmd.ownerId);
            }

            // Safety: reject commands from other owners (shouldn't be queued by web_server)
            if (executorBusy && !(executorOwnerType == cmd.ownerType && executorOwnerId == cmd.ownerId)) {
                Serial.printf("controlTask: rejecting command from %d/%d because executor owned by %d/%d -> error:busy\n", cmd.ownerType, cmd.ownerId, executorOwnerType, executorOwnerId);
                if (webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:busy"));
                continue;
            }

            // Apply the setpoints
            if (cmd.isHoming) {
                encX = encY = encZ = encE = 0;
                setpointX = setpointY = setpointZ = setpointE = 0;
                pidX.reset(); pidY.reset(); pidZ.reset(); pidE.reset();
            } else {
                if (absolutePositioning) {
                    if (cmd.hasX) { setpointX = cmd.targetX * 100; currentPosX = setpointX; }
                    if (cmd.hasY) { setpointY = cmd.targetY * 100; currentPosY = setpointY; }
                    if (cmd.hasZ) { setpointZ = cmd.targetZ * 100; currentPosZ = setpointZ; }
                    if (cmd.hasE) { setpointE = cmd.targetE * 100; currentPosE = setpointE; }
                } else {
                    if (cmd.hasX) { setpointX += cmd.targetX * 100; currentPosX = setpointX; }
                    if (cmd.hasY) { setpointY += cmd.targetY * 100; currentPosY = setpointY; }
                    if (cmd.hasZ) { setpointZ += cmd.targetZ * 100; currentPosZ = setpointZ; }
                    if (cmd.hasE) { setpointE += cmd.targetE * 100; currentPosE = setpointE; }
                }
            }

            // Prepare trajectory following if feedrate provided
            bool useTrajectory = false;
            long trajStartX = 0, trajStartY = 0, trajStartZ = 0, trajStartE = 0;
            long trajTargetX = setpointX, trajTargetY = setpointY, trajTargetZ = setpointZ, trajTargetE = setpointE;
            float trajDurationMs = 0.0f;
            float totalDistanceCounts = 0.0f;
            if (cmd.feedrate > 0) {
                // Use actual encoder positions as the start point
                trajStartX = encX; trajStartY = encY; trajStartZ = encZ; trajStartE = encE;
                long dx = trajTargetX - trajStartX;
                long dy = trajTargetY - trajStartY;
                long dz = trajTargetZ - trajStartZ;
                long de = trajTargetE - trajStartE;
                totalDistanceCounts = sqrt((float)dx*(float)dx + (float)dy*(float)dy + (float)dz*(float)dz + (float)de*(float)de);
                if (totalDistanceCounts > 0.0f) {
                    // distance in mm = counts / 100.0
                    float dist_mm = totalDistanceCounts / 100.0f;
                    // feedrate is mm per minute -> convert to ms: durationMs = dist_mm (mm) / (feedrate mm/min) * 60000 ms/min
                    trajDurationMs = (dist_mm / (float)cmd.feedrate) * 60000.0f;
                    if (trajDurationMs < 1.0f) trajDurationMs = 1.0f;
                    useTrajectory = true;
                }
            }

            // Execute command until completion or timeout/halt
            unsigned long startTime = millis();
            bool finished = false;
            while (!isHalted) {
                // Determine desired setpoints (if using trajectory, compute time-based desired positions)
                long desiredX = setpointX;
                long desiredY = setpointY;
                long desiredZ = setpointZ;
                long desiredE = setpointE;
                unsigned long now = millis();
                if (useTrajectory) {
                    float elapsed = (float)(now - startTime);
                    float frac = elapsed / trajDurationMs;
                    if (frac > 1.0f) frac = 1.0f;
                    desiredX = trajStartX + (long)round((trajTargetX - trajStartX) * frac);
                    desiredY = trajStartY + (long)round((trajTargetY - trajStartY) * frac);
                    desiredZ = trajStartZ + (long)round((trajTargetZ - trajStartZ) * frac);
                    desiredE = trajStartE + (long)round((trajTargetE - trajStartE) * frac);

                    // Trajectory deviation handling: warn range and halt range (broadcasted)
                    if (cmd.hasX) {
                        long dev = labs(encX - desiredX);
                        if (dev > POSITION_HALT_TOLERANCE_COUNTS) {
                            isHalted = true; haltReason = "X position deviation";
                            if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:X_position_deviation")); }
                            break;
                        } else if (dev > POSITION_WARN_TOLERANCE_COUNTS) {
                            if (now - lastPosWarnX > 200) {
                                lastPosWarnX = now;
                                if (webServer) { webServer->broadcastWarning("X position deviation"); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:X_position_deviation")); }
                            }
                        }
                    }
                    if (cmd.hasY) {
                        long dev = labs(encY - desiredY);
                        if (dev > POSITION_HALT_TOLERANCE_COUNTS) {
                            isHalted = true; haltReason = "Y position deviation";
                            if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:Y_position_deviation")); }
                            break;
                        } else if (dev > POSITION_WARN_TOLERANCE_COUNTS) {
                            if (now - lastPosWarnY > 200) {
                                lastPosWarnY = now;
                                if (webServer) { webServer->broadcastWarning("Y position deviation"); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:Y_position_deviation")); }
                            }
                        }
                    }
                    if (cmd.hasZ) {
                        long dev = labs(encZ - desiredZ);
                        if (dev > POSITION_HALT_TOLERANCE_COUNTS) {
                            isHalted = true; haltReason = "Z position deviation";
                            if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:Z_position_deviation")); }
                            break;
                        } else if (dev > POSITION_WARN_TOLERANCE_COUNTS) {
                            if (now - lastPosWarnZ > 200) {
                                lastPosWarnZ = now;
                                if (webServer) { webServer->broadcastWarning("Z position deviation"); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:Z_position_deviation")); }
                            }
                        }
                    }
                    if (cmd.hasE) {
                        long dev = labs(encE - desiredE);
                        if (dev > POSITION_HALT_TOLERANCE_COUNTS) {
                            isHalted = true; haltReason = "E position deviation";
                            if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:E_position_deviation")); }
                            break;
                        } else if (dev > POSITION_WARN_TOLERANCE_COUNTS) {
                            if (now - lastPosWarnE > 200) {
                                lastPosWarnE = now;
                                if (webServer) { webServer->broadcastWarning("E position deviation"); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:E_position_deviation")); }
                            }
                        }
                    }
                }

                // Compute outputs toward desired setpoints
                int outX = pidX.compute(desiredX, encX);
                int outY = pidY.compute(desiredY, encY);
                int outZ = pidZ.compute(desiredZ, encZ);
                int outE = pidE.compute(desiredE, encE);

                // Apply outputs
                motorX.setSpeed(outX);
                motorY.setSpeed(outY);
                motorZ.setSpeed(outZ);
                motorE.setSpeed(outE);

                // update now/timestamps
                // 'now' may have been set earlier; if not, update it
                if (!useTrajectory) now = millis();
                if (encX != lastSeenEncX) { lastSeenEncX = encX; lastEncChangeX = now; }
                if (encY != lastSeenEncY) { lastSeenEncY = encY; lastEncChangeY = now; }
                if (encZ != lastSeenEncZ) { lastSeenEncZ = encZ; lastEncChangeZ = now; }
                if (encE != lastSeenEncE) { lastSeenEncE = encE; lastEncChangeE = now; }

                // Check following deviation warnings & halts
                if (cmd.hasX) {
                    long d = abs(setpointX - encX);
                    if (d > FOLLOWING_ERROR_WARN && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:X_following"));
                    if (d > FOLLOWING_ERROR_HALT) { isHalted = true; haltReason = "X following error"; if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:X_following")); } break; }
                }
                if (cmd.hasY) {
                    long d = abs(setpointY - encY);
                    if (d > FOLLOWING_ERROR_WARN && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:Y_following"));
                    if (d > FOLLOWING_ERROR_HALT) { isHalted = true; haltReason = "Y following error"; if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:Y_following")); } break; }
                }
                if (cmd.hasZ) {
                    long d = abs(setpointZ - encZ);
                    if (d > FOLLOWING_ERROR_WARN && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:Z_following"));
                    if (d > FOLLOWING_ERROR_HALT) { isHalted = true; haltReason = "Z following error"; if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:Z_following")); } break; }
                }
                if (cmd.hasE) {
                    long d = abs(setpointE - encE);
                    if (d > FOLLOWING_ERROR_WARN && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:E_following"));
                    if (d > FOLLOWING_ERROR_HALT) { isHalted = true; haltReason = "E following error"; if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:E_following")); } break; }
                }

                // Stall warnings and halts (no encoder change while motor commanded)
                if (abs(outX) >= MIN_MOTOR_COMMAND) {
                    if ((now - lastEncChangeX) > STALL_WARNING_MS && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:X_no_movement"));
                    if ((now - lastEncChangeX) > STALL_TIMEOUT_MS) { isHalted = true; haltReason = "X Axis Stall Detected"; if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:X_no_movement")); } break; }
                }
                if (abs(outY) >= MIN_MOTOR_COMMAND) {
                    if ((now - lastEncChangeY) > STALL_WARNING_MS && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:Y_no_movement"));
                    if ((now - lastEncChangeY) > STALL_TIMEOUT_MS) { isHalted = true; haltReason = "Y Axis Stall Detected"; if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:Y_no_movement")); } break; }
                }
                if (abs(outZ) >= MIN_MOTOR_COMMAND) {
                    if ((now - lastEncChangeZ) > STALL_WARNING_MS && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:Z_no_movement"));
                    if ((now - lastEncChangeZ) > STALL_TIMEOUT_MS) { isHalted = true; haltReason = "Z Axis Stall Detected"; if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:Z_no_movement")); } break; }
                }
                if (abs(outE) >= MIN_MOTOR_COMMAND) {
                    if ((now - lastEncChangeE) > STALL_WARNING_MS && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:E_no_movement"));
                    if ((now - lastEncChangeE) > STALL_TIMEOUT_MS) { isHalted = true; haltReason = "E Axis Stall Detected"; if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:E_no_movement")); } break; }
                }

                // Check finished
                bool done = true;
                if (cmd.hasX && abs(setpointX - encX) > POSITION_TOLERANCE) done = false;
                if (cmd.hasY && abs(setpointY - encY) > POSITION_TOLERANCE) done = false;
                if (cmd.hasZ && abs(setpointZ - encZ) > POSITION_TOLERANCE) done = false;
                if (cmd.hasE && abs(setpointE - encE) > POSITION_TOLERANCE) done = false;
                if (done) { finished = true; break; }

                if ((now - startTime) > COMMAND_EXECUTE_TIMEOUT_MS) {
                    isHalted = true;
                    haltReason = "Command timeout";
                    if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:timeout")); }
                    break;
                }

                vTaskDelayUntil(&xLastWakeTime, xFrequency);
            }

            // Stop motors for this command
            motorX.setSpeed(0); motorY.setSpeed(0); motorZ.setSpeed(0); motorE.setSpeed(0);

            // Respond to originating client
            if (finished && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("ok"));

            // Allow a small grace period for additional commands from same owner
            unsigned long graceStart = millis();
            bool keepOwnership = false;
            while ((millis() - graceStart) < 250) {
                MotionCommand nextCmd;
                if (xQueueReceive(motionQueue, &nextCmd, 0) == pdTRUE) {
                    if (nextCmd.ownerType == executorOwnerType && nextCmd.ownerId == executorOwnerId) {
                        // process inline by setting cmd to nextCmd and continuing
                        cmd = nextCmd;
                        keepOwnership = true;
                        break; // outer while will now continue (we will loop back)
                    } else {
                        // requeue for later
                        Serial.printf("controlTask: requeueing command from %d/%d for later\n", nextCmd.ownerType, nextCmd.ownerId);
                        xQueueSend(motionQueue, &nextCmd, 0);
                        break;
                    }
                }
                vTaskDelay(5 / portTICK_PERIOD_MS);
            }

            if (keepOwnership) {
                // continue processing next command from same owner immediately
                continue;
            }

            // release executor
            Serial.printf("controlTask: releasing executor from %d/%d\n", executorOwnerType, executorOwnerId);
            executorBusy = false;
            executorOwnerType = SRC_SERIAL;
            executorOwnerId = -1;
        }

        // Small scheduling delay (keep loop predictable)
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void setup() {
    Serial.begin(115200);

    // Init Hardware
    motorX.begin(); motorY.begin(); motorZ.begin(); motorE.begin();
    thermal.begin();

    // Optional outputs: fan / spindle / laser PWM setup (if configured)
    if (PIN_FAN >= 0) {
        pinMode(PIN_FAN, OUTPUT);
        ledcSetup(PWM_CHAN_FAN, PWM_FREQ, PWM_RES);
        ledcAttachPin(PIN_FAN, PWM_CHAN_FAN);
        ledcWrite(PWM_CHAN_FAN, 0);
    }
    if (PIN_SPINDLE >= 0) {
        pinMode(PIN_SPINDLE, OUTPUT);
        ledcSetup(PWM_CHAN_SPINDLE, PWM_FREQ, PWM_RES);
        ledcAttachPin(PIN_SPINDLE, PWM_CHAN_SPINDLE);
        ledcWrite(PWM_CHAN_SPINDLE, 0);
    }
    if (PIN_LASER >= 0) {
        pinMode(PIN_LASER, OUTPUT);
        ledcSetup(PWM_CHAN_LASER, PWM_FREQ, PWM_RES);
        ledcAttachPin(PIN_LASER, PWM_CHAN_LASER);
        ledcWrite(PWM_CHAN_LASER, 0);
    }

    // Init Encoders (Use PULLUP to prevent floating noise)
    pinMode(PIN_X_ENC_A, INPUT_PULLUP); pinMode(PIN_X_ENC_B, INPUT_PULLUP);
    pinMode(PIN_Y_ENC_A, INPUT_PULLUP); pinMode(PIN_Y_ENC_B, INPUT_PULLUP);
    pinMode(PIN_Z_ENC_A, INPUT_PULLUP); pinMode(PIN_Z_ENC_B, INPUT_PULLUP);
    pinMode(PIN_E_ENC_A, INPUT_PULLUP); pinMode(PIN_E_ENC_B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_X_ENC_A), isrX, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_Y_ENC_A), isrY, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_Z_ENC_A), isrZ, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_E_ENC_A), isrE, CHANGE);

    // Initialize last-seen encoder values and timestamps so stall detection doesn't trigger immediately
    lastSeenEncX = encX; lastSeenEncY = encY; lastSeenEncZ = encZ; lastSeenEncE = encE;
    unsigned long now = millis();
    lastEncChangeX = lastEncChangeY = lastEncChangeZ = lastEncChangeE = now;

    // Init RTOS Objects
    gcodeStream = xStreamBufferCreate(1024, 1);
    motionQueue = xQueueCreate(10, sizeof(MotionCommand));
    commandQueue = xQueueCreate(32, sizeof(RawCommand));

    // Create Tasks
    xTaskCreatePinnedToCore(networkTask, "Network", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(thermalTask, "Thermal", 2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(parserTask, "Parser", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(controlTask, "Control", 4096, NULL, 2, NULL, 1); // High Priority
}

void loop() {
    vTaskDelete(NULL); // Loop not used
}

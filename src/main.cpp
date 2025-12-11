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

// Last motor outputs (signed -255..255) for diagnostics
volatile int motorOutX = 0;
volatile int motorOutY = 0;
volatile int motorOutZ = 0;
volatile int motorOutE = 0;

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

// Runtime-configurable parameters (stored in Preferences)
float countsPerMM_X = DEFAULT_COUNTS_PER_MM_X;
float countsPerMM_Y = DEFAULT_COUNTS_PER_MM_Y;
float countsPerMM_Z = DEFAULT_COUNTS_PER_MM_Z;
float countsPerMM_E = DEFAULT_COUNTS_PER_MM_E;

// Per-axis PID tunings (defaults from config.h KP_DEFAULT...)
float pid_kp_x = KP_DEFAULT, pid_ki_x = KI_DEFAULT, pid_kd_x = KD_DEFAULT;
float pid_kp_y = KP_DEFAULT, pid_ki_y = KI_DEFAULT, pid_kd_y = KD_DEFAULT;
float pid_kp_z = KP_DEFAULT, pid_ki_z = KI_DEFAULT, pid_kd_z = KD_DEFAULT;
float pid_kp_e = KP_DEFAULT, pid_ki_e = KI_DEFAULT, pid_kd_e = KD_DEFAULT;

// Max feedrates (mm/min)
int maxFeedrateX = MAX_FEEDRATE;
int maxFeedrateY = MAX_FEEDRATE;
int maxFeedrateZ = MAX_FEEDRATE;
int maxFeedrateE = MAX_FEEDRATE;

// Positioning mode
bool absolutePositioning = false; // Default to relative (G91) for simple jogs

// System flags
volatile bool isHalted = false;
String haltReason = "";

// Helper: disable spindle and laser immediately and persist state
void disableSpindleAndLaser() {
    // Turn off hardware outputs if present
    if (PIN_SPINDLE >= 0) ledcWrite(PWM_CHAN_SPINDLE, 0);
    if (PIN_LASER >= 0) ledcWrite(PWM_CHAN_LASER, 0);
    // Update runtime globals
    spindlePower = 0;
    laserPower = 0;
    // Persist to Preferences
    Preferences prefs;
    prefs.begin("cnc", false);
    prefs.putInt("spindle_p", 0);
    prefs.putInt("laser_p", 0);
    prefs.end();
    // Notify connected clients
    if (webServer) {
        webServer->broadcastWarning(String("Spindle and laser disabled: ") + haltReason);
    }
}

// Run control globals
volatile bool runPaused = false;
volatile bool runStopped = false;
volatile int runSpeedPercent = 100; // percent (5 - 500)
volatile float runSpeedMultiplier = 1.0f; // = runSpeedPercent / 100.0

// Spindle / Laser runtime globals (0..255)
volatile int spindlePower = 0;
volatile int laserPower = 0;

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
                // Ensure spindle/laser are disabled on halt
                disableSpindleAndLaser();
                webServer->broadcastError(haltReason);
            } else {
                // Send encoder positions (converted to mm), setpoints (mm), and last movement ages
                float encXm = encX / countsPerMM_X; float encYm = encY / countsPerMM_Y; float encZm = encZ / countsPerMM_Z; float encEm = encE / countsPerMM_E;
                float setXm = currentPosX / countsPerMM_X; float setYm = currentPosY / countsPerMM_Y; float setZm = currentPosZ / countsPerMM_Z; float setEm = currentPosE / countsPerMM_E;
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
                    lmX, lmY, lmZ, lmE,
                    motorOutX, motorOutY, motorOutZ, motorOutE,
                    encX, encY, encZ, encE);
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
    float targetXmm, targetYmm, targetZmm, targetEmm;
    bool hasX, hasY, hasZ, hasE; // which axes were specified
    bool isRelative;
    bool isHoming;
    bool isEmergency; // M112
    uint8_t ownerType; // SRC_*
    int ownerId;
    float feedrate; // mm/min (0 == unspecified / full)
};

void parserTask(void *pvParameters) {
    char buffer[64];
    MotionCommand cmd;
    // Initialize targets to 0 and clear axis flags
    cmd.targetXmm = 0.0f; cmd.targetYmm = 0.0f; cmd.targetZmm = 0.0f; cmd.targetEmm = 0.0f;
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
            // Avoid blocking forever on the serial stream so we can still
            // service queued client commands that arrive while no serial
            // input is present. Use a short timeout and loop back to check
            // the `commandQueue` frequently.
            size_t bytes = xStreamBufferReceive(gcodeStream, buffer, 64, (TickType_t)10);
            if (bytes <= 0) continue;
            buffer[bytes] = '\0'; // Ensure null termination
            line = String(buffer);
            line.trim();
            // Normalize to uppercase so both 'X' and 'x' (and 'g','G') are parsed
            line.toUpperCase();
            if (line.length() == 0) continue;
            // default owner: if a job streamer is active, mark as SRC_JOB so
            // controlTask and executor semantics know these motions originate
            // from a file job rather than an interactive client.
            raw.srcType = jobActive ? SRC_JOB : SRC_SERIAL;
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

                // Parse X (float mm)
                int xIdx = line.indexOf('X');
                if (xIdx != -1) {
                    cmd.targetXmm = line.substring(xIdx + 1).toFloat();
                    cmd.hasX = true;
                }

                // Parse Y
                int yIdx = line.indexOf('Y');
                if (yIdx != -1) {
                    cmd.targetYmm = line.substring(yIdx + 1).toFloat();
                    cmd.hasY = true;
                }

                // Parse Z
                int zIdx = line.indexOf('Z');
                if (zIdx != -1) {
                    cmd.targetZmm = line.substring(zIdx + 1).toFloat();
                    cmd.hasZ = true;
                }

                // Parse E
                int eIdx = line.indexOf('E');
                if (eIdx != -1) {
                    cmd.targetEmm = line.substring(eIdx + 1).toFloat();
                    cmd.hasE = true;
                }

                // Parse Feedrate F (optional)
                int fIdx = line.indexOf('F');
                if (fIdx != -1) {
                    cmd.feedrate = line.substring(fIdx + 1).toFloat();
                } else {
                    cmd.feedrate = 0.0f;
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
                            arcCmd.targetXmm = px;
                            arcCmd.targetYmm = py;
                            arcCmd.isRelative = false;
                            arcCmd.feedrate = (float)feed;
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
                char response[256];
                snprintf(response, sizeof(response), "X:%.4f Y:%.4f Z:%.4f E:%.4f\n",
                    currentPosX / countsPerMM_X, currentPosY / countsPerMM_Y, currentPosZ / countsPerMM_Z, currentPosE / countsPerMM_E);
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
            else if (line.startsWith("M92")) {
                // Set steps/counts per mm: M92 Xnnn Ynnn Znnn Enn
                int xIdx = line.indexOf('X');
                if (xIdx != -1) countsPerMM_X = line.substring(xIdx + 1).toFloat();
                int yIdx = line.indexOf('Y');
                if (yIdx != -1) countsPerMM_Y = line.substring(yIdx + 1).toFloat();
                int zIdx = line.indexOf('Z');
                if (zIdx != -1) countsPerMM_Z = line.substring(zIdx + 1).toFloat();
                int eIdx2 = line.indexOf('E');
                if (eIdx2 != -1) countsPerMM_E = line.substring(eIdx2 + 1).toFloat();
                Serial.println("M92: updated counts per mm");
            }
            else if (line.startsWith("M301")) {
                // Set PID tuning: M301 [X P... I... D...] [Y ...] [Z ...] [E ...]
                // Parse P/I/D groups for each axis if present
                auto parseAxisTunings = [&](char axis, float &kp, float &ki, float &kd) {
                    int idx = line.indexOf(axis);
                    if (idx != -1) {
                        // substring from axis letter to end; parse tokens for P/I/D
                        String rest = line.substring(idx + 1);
                        int pIdx = rest.indexOf('P'); if (pIdx != -1) {
                            kp = rest.substring(pIdx + 1).toFloat();
                        }
                        int iIdx = rest.indexOf('I'); if (iIdx != -1) {
                            ki = rest.substring(iIdx + 1).toFloat();
                        }
                        int dIdx = rest.indexOf('D'); if (dIdx != -1) {
                            kd = rest.substring(dIdx + 1).toFloat();
                        }
                        return true;
                    }
                    return false;
                };

                bool changed = false;
                if (parseAxisTunings('X', pid_kp_x, pid_ki_x, pid_kd_x)) { pidX.setTunings(pid_kp_x, pid_ki_x, pid_kd_x); changed = true; }
                if (parseAxisTunings('Y', pid_kp_y, pid_ki_y, pid_kd_y)) { pidY.setTunings(pid_kp_y, pid_ki_y, pid_kd_y); changed = true; }
                if (parseAxisTunings('Z', pid_kp_z, pid_ki_z, pid_kd_z)) { pidZ.setTunings(pid_kp_z, pid_ki_z, pid_kd_z); changed = true; }
                if (parseAxisTunings('E', pid_kp_e, pid_ki_e, pid_kd_e)) { pidE.setTunings(pid_kp_e, pid_ki_e, pid_kd_e); changed = true; }
                if (changed) Serial.println("M301: PID tunings updated");
            }
            else if (line.startsWith("M503")) {
                // Report settings
                char buf[256];
                snprintf(buf, sizeof(buf), "M92 X%.4f Y%.4f Z%.4f E%.4f\n", countsPerMM_X, countsPerMM_Y, countsPerMM_Z, countsPerMM_E);
                Serial.print(buf);
                if (webServer) webServer->sendTelnet(String(buf));
                snprintf(buf, sizeof(buf), "PID X P%.4f I%.4f D%.4f\n", pid_kp_x, pid_ki_x, pid_kd_x);
                Serial.print(buf); if (webServer) webServer->sendTelnet(String(buf));
                snprintf(buf, sizeof(buf), "PID Y P%.4f I%.4f D%.4f\n", pid_kp_y, pid_ki_y, pid_kd_y);
                Serial.print(buf); if (webServer) webServer->sendTelnet(String(buf));
                snprintf(buf, sizeof(buf), "PID Z P%.4f I%.4f D%.4f\n", pid_kp_z, pid_ki_z, pid_kd_z);
                Serial.print(buf); if (webServer) webServer->sendTelnet(String(buf));
                snprintf(buf, sizeof(buf), "PID E P%.4f I%.4f D%.4f\n", pid_kp_e, pid_ki_e, pid_kd_e);
                Serial.print(buf); if (webServer) webServer->sendTelnet(String(buf));
            }
            else if (line.startsWith("M500")) {
                // Save settings to Preferences
                Preferences prefs;
                prefs.begin("cnc", false);
                prefs.putFloat("cpm_x", countsPerMM_X);
                prefs.putFloat("cpm_y", countsPerMM_Y);
                prefs.putFloat("cpm_z", countsPerMM_Z);
                prefs.putFloat("cpm_e", countsPerMM_E);
                prefs.putFloat("pid_kp_x", pid_kp_x); prefs.putFloat("pid_ki_x", pid_ki_x); prefs.putFloat("pid_kd_x", pid_kd_x);
                prefs.putFloat("pid_kp_y", pid_kp_y); prefs.putFloat("pid_ki_y", pid_ki_y); prefs.putFloat("pid_kd_y", pid_kd_y);
                prefs.putFloat("pid_kp_z", pid_kp_z); prefs.putFloat("pid_ki_z", pid_ki_z); prefs.putFloat("pid_kd_z", pid_kd_z);
                prefs.putFloat("pid_kp_e", pid_kp_e); prefs.putFloat("pid_ki_e", pid_ki_e); prefs.putFloat("pid_kd_e", pid_kd_e);
                prefs.end();
                Serial.println("Settings saved (M500)");
            }
            else if (line.startsWith("M501")) {
                // Load settings from Preferences
                Preferences prefs;
                prefs.begin("cnc", true);
                countsPerMM_X = prefs.getFloat("cpm_x", countsPerMM_X);
                countsPerMM_Y = prefs.getFloat("cpm_y", countsPerMM_Y);
                countsPerMM_Z = prefs.getFloat("cpm_z", countsPerMM_Z);
                countsPerMM_E = prefs.getFloat("cpm_e", countsPerMM_E);
                pid_kp_x = prefs.getFloat("pid_kp_x", pid_kp_x); pid_ki_x = prefs.getFloat("pid_ki_x", pid_ki_x); pid_kd_x = prefs.getFloat("pid_kd_x", pid_kd_x);
                pid_kp_y = prefs.getFloat("pid_kp_y", pid_kp_y); pid_ki_y = prefs.getFloat("pid_ki_y", pid_ki_y); pid_kd_y = prefs.getFloat("pid_kd_y", pid_kd_y);
                pid_kp_z = prefs.getFloat("pid_kp_z", pid_kp_z); pid_ki_z = prefs.getFloat("pid_ki_z", pid_ki_z); pid_kd_z = prefs.getFloat("pid_kd_z", pid_kd_z);
                pid_kp_e = prefs.getFloat("pid_kp_e", pid_kp_e); pid_ki_e = prefs.getFloat("pid_ki_e", pid_ki_e); pid_kd_e = prefs.getFloat("pid_kd_e", pid_kd_e);
                prefs.end();
                // Apply loaded tunings to controllers
                pidX.setTunings(pid_kp_x, pid_ki_x, pid_kd_x);
                pidY.setTunings(pid_kp_y, pid_ki_y, pid_kd_y);
                pidZ.setTunings(pid_kp_z, pid_ki_z, pid_kd_z);
                pidE.setTunings(pid_kp_e, pid_ki_e, pid_kd_e);
                Serial.println("Settings loaded (M501)");
            }
            else if (line.startsWith("M3") || line.startsWith("M5")) {
                // Spindle / Laser on/off. M3 S<0-255> to set power, M5 to stop
                if (line.startsWith("M5")) {
                    if (PIN_SPINDLE >= 0) ledcWrite(PWM_CHAN_SPINDLE, 0);
                    if (PIN_LASER >= 0) ledcWrite(PWM_CHAN_LASER, 0);
                    // update runtime state and persist
                    spindlePower = 0; laserPower = 0;
                    Preferences prefs; prefs.begin("cnc", false);
                    prefs.putInt("spindle_p", 0);
                    prefs.putInt("laser_p", 0);
                    prefs.end();
                } else {
                    int sIdx = line.indexOf('S');
                    int val = 255;
                    if (sIdx != -1) val = line.substring(sIdx + 1).toInt();
                    int v = constrain(val, 0, 255);
                    if (PIN_SPINDLE >= 0) ledcWrite(PWM_CHAN_SPINDLE, v);
                    if (PIN_LASER >= 0) ledcWrite(PWM_CHAN_LASER, v);
                    // update runtime state and persist
                    spindlePower = v; laserPower = v;
                    Preferences prefs; prefs.begin("cnc", false);
                    prefs.putInt("spindle_p", spindlePower);
                    prefs.putInt("laser_p", laserPower);
                    prefs.end();
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
            // Ensure spindle/laser are off while halted
            disableSpindleAndLaser();
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
                // Ensure spindle/laser are disabled immediately
                disableSpindleAndLaser();
                if (webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("ok:emergency"));
                // continue to next command
                continue;
            }

            // Claim executor if not busy (protected by global executor spinlock)
            portENTER_CRITICAL(&g_executorMux);
            if (!executorBusy) {
                executorBusy = true;
                executorOwnerType = cmd.ownerType;
                executorOwnerId = cmd.ownerId;
                // controlTask claims executor for real; clear any push-time reservation
                executorReservedUntil = 0;
                Serial.printf("controlTask: claimed executor for %d/%d\n", cmd.ownerType, cmd.ownerId);
            }
            portEXIT_CRITICAL(&g_executorMux);

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
                    if (cmd.hasX) { setpointX = (long)round(cmd.targetXmm * countsPerMM_X); currentPosX = setpointX; }
                    if (cmd.hasY) { setpointY = (long)round(cmd.targetYmm * countsPerMM_Y); currentPosY = setpointY; }
                    if (cmd.hasZ) { setpointZ = (long)round(cmd.targetZmm * countsPerMM_Z); currentPosZ = setpointZ; }
                    if (cmd.hasE) { setpointE = (long)round(cmd.targetEmm * countsPerMM_E); currentPosE = setpointE; }
                } else {
                    if (cmd.hasX) { setpointX += (long)round(cmd.targetXmm * countsPerMM_X); currentPosX = setpointX; }
                    if (cmd.hasY) { setpointY += (long)round(cmd.targetYmm * countsPerMM_Y); currentPosY = setpointY; }
                    if (cmd.hasZ) { setpointZ += (long)round(cmd.targetZmm * countsPerMM_Z); currentPosZ = setpointZ; }
                    if (cmd.hasE) { setpointE += (long)round(cmd.targetEmm * countsPerMM_E); currentPosE = setpointE; }
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
                    // apply run speed multiplier
                    float effectiveFeed = (float)cmd.feedrate * runSpeedMultiplier;
                    if (effectiveFeed < 0.001f) effectiveFeed = 0.001f;
                    // feedrate is mm per minute -> convert to ms: durationMs = dist_mm (mm) / (feedrate mm/min) * 60000 ms/min
                    trajDurationMs = (dist_mm / effectiveFeed) * 60000.0f;
                    if (trajDurationMs < 1.0f) trajDurationMs = 1.0f;
                    useTrajectory = true;
                }
            }

            // Reset pause accounting for this command
            unsigned long pauseAccumMs = 0;
            unsigned long pauseStartMs = 0;

            // Execute command until completion or timeout/halt
            unsigned long startTime = millis();
            bool finished = false;
            while (!isHalted) {
                // Handle run stop: immediate cancel of this command
                if (runStopped) {
                    // Stop motors, clear queues and release executor
                    motorX.setSpeed(0); motorY.setSpeed(0); motorZ.setSpeed(0); motorE.setSpeed(0);
                    // Clear pending motion commands
                    if (motionQueue != NULL) xQueueReset(motionQueue);
                    if (commandQueue != NULL) xQueueReset(commandQueue);
                    if (webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("ok:stopped"));
                    // release ownership (protected)
                    portENTER_CRITICAL(&g_executorMux);
                    executorBusy = false; executorOwnerType = SRC_SERIAL; executorOwnerId = -1;
                    portEXIT_CRITICAL(&g_executorMux);
                    runStopped = false; // clear
                    finished = false;
                    break;
                }

                // Handle pause: park motors but keep ownership and adjust pause accounting
                if (runPaused) {
                    // record pause start if not already
                    if (pauseStartMs == 0) pauseStartMs = millis();
                    motorX.setSpeed(0); motorY.setSpeed(0); motorZ.setSpeed(0); motorE.setSpeed(0);
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                    continue; // stay in pause loop until unpaused or stopped
                } else {
                    // if we were paused, accumulate pause duration
                    if (pauseStartMs != 0) {
                        pauseAccumMs += (millis() - pauseStartMs);
                        pauseStartMs = 0;
                    }
                }
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
                            // Immediately disable spindle/laser for safety
                            disableSpindleAndLaser();
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
                            disableSpindleAndLaser();
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
                            disableSpindleAndLaser();
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
                            disableSpindleAndLaser();
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

                // Publish outputs into diagnostic globals before applying
                motorOutX = outX; motorOutY = outY; motorOutZ = outZ; motorOutE = outE;

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
                    if (d > FOLLOWING_ERROR_HALT) {
                        isHalted = true; haltReason = "X following error";
                        disableSpindleAndLaser();
                        if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:X_following")); }
                        break;
                    }
                }
                if (cmd.hasY) {
                    long d = abs(setpointY - encY);
                    if (d > FOLLOWING_ERROR_WARN && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:Y_following"));
                    if (d > FOLLOWING_ERROR_HALT) {
                        isHalted = true; haltReason = "Y following error";
                        disableSpindleAndLaser();
                        if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:Y_following")); }
                        break;
                    }
                }
                if (cmd.hasZ) {
                    long d = abs(setpointZ - encZ);
                    if (d > FOLLOWING_ERROR_WARN && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:Z_following"));
                    if (d > FOLLOWING_ERROR_HALT) {
                        isHalted = true; haltReason = "Z following error";
                        disableSpindleAndLaser();
                        if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:Z_following")); }
                        break;
                    }
                }
                if (cmd.hasE) {
                    long d = abs(setpointE - encE);
                    if (d > FOLLOWING_ERROR_WARN && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:E_following"));
                    if (d > FOLLOWING_ERROR_HALT) {
                        isHalted = true; haltReason = "E following error";
                        disableSpindleAndLaser();
                        if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:E_following")); }
                        break;
                    }
                }

                // Stall warnings and halts (no encoder change while motor commanded)
                if (abs(outX) >= MIN_MOTOR_COMMAND) {
                    if ((now - lastEncChangeX) > STALL_WARNING_MS && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:X_no_movement"));
                    if ((now - lastEncChangeX) > STALL_TIMEOUT_MS) {
                        isHalted = true; haltReason = "X Axis Stall Detected";
                        disableSpindleAndLaser();
                        if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:X_no_movement")); }
                        break;
                    }
                }
                if (abs(outY) >= MIN_MOTOR_COMMAND) {
                    if ((now - lastEncChangeY) > STALL_WARNING_MS && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:Y_no_movement"));
                    if ((now - lastEncChangeY) > STALL_TIMEOUT_MS) {
                        isHalted = true; haltReason = "Y Axis Stall Detected";
                        disableSpindleAndLaser();
                        if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:Y_no_movement")); }
                        break;
                    }
                }
                if (abs(outZ) >= MIN_MOTOR_COMMAND) {
                    if ((now - lastEncChangeZ) > STALL_WARNING_MS && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:Z_no_movement"));
                    if ((now - lastEncChangeZ) > STALL_TIMEOUT_MS) {
                        isHalted = true; haltReason = "Z Axis Stall Detected";
                        disableSpindleAndLaser();
                        if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:Z_no_movement")); }
                        break;
                    }
                }
                if (abs(outE) >= MIN_MOTOR_COMMAND) {
                    if ((now - lastEncChangeE) > STALL_WARNING_MS && webServer) webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("warn:E_no_movement"));
                    if ((now - lastEncChangeE) > STALL_TIMEOUT_MS) {
                        isHalted = true; haltReason = "E Axis Stall Detected";
                        disableSpindleAndLaser();
                        if (webServer) { webServer->broadcastError(haltReason); webServer->sendResponseToClient(cmd.ownerType, cmd.ownerId, String("error:halt:E_no_movement")); }
                        break;
                    }
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
                    // Turn off spindle/laser on timeout
                    disableSpindleAndLaser();
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

            // release executor (protected)
            Serial.printf("controlTask: releasing executor from %d/%d\n", executorOwnerType, executorOwnerId);
            portENTER_CRITICAL(&g_executorMux);
            executorBusy = false;
            executorOwnerType = SRC_SERIAL;
            executorOwnerId = -1;
            portEXIT_CRITICAL(&g_executorMux);
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
        // Restore persisted spindle power (if any)
        Preferences prefs; prefs.begin("cnc", true);
        spindlePower = prefs.getInt("spindle_p", 0);
        prefs.end();
        ledcWrite(PWM_CHAN_SPINDLE, constrain(spindlePower, 0, 255));
    }
    if (PIN_LASER >= 0) {
        pinMode(PIN_LASER, OUTPUT);
        ledcSetup(PWM_CHAN_LASER, PWM_FREQ, PWM_RES);
        ledcAttachPin(PIN_LASER, PWM_CHAN_LASER);
        // Restore persisted laser power (if any)
        Preferences lprefs; lprefs.begin("cnc", true);
        laserPower = lprefs.getInt("laser_p", 0);
        lprefs.end();
        ledcWrite(PWM_CHAN_LASER, constrain(laserPower, 0, 255));
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

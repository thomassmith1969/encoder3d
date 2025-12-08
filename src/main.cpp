#include <Arduino.h>
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
WebServerManager* webServer; // Pointer to WebServer

// --- RTOS HANDLES ---
StreamBufferHandle_t gcodeStream;
QueueHandle_t motionQueue;

// --- ENCODER ISRs ---
volatile long encX = 0;
volatile long encY = 0;
volatile long encZ = 0;
volatile long encE = 0;

// --- SYSTEM STATE ---
volatile bool isHalted = false;
String haltReason = "";

void IRAM_ATTR isrX() {
    if (digitalRead(PIN_X_ENC_A) == digitalRead(PIN_X_ENC_B)) encX = encX + 1; else encX = encX - 1;
}
void IRAM_ATTR isrY() {
    if (digitalRead(PIN_Y_ENC_A) == digitalRead(PIN_Y_ENC_B)) encY = encY + 1; else encY = encY - 1;
}
void IRAM_ATTR isrZ() {
    if (digitalRead(PIN_Z_ENC_A) == digitalRead(PIN_Z_ENC_B)) encZ = encZ + 1; else encZ = encZ - 1;
}
void IRAM_ATTR isrE() {
    if (digitalRead(PIN_E_ENC_A) == digitalRead(PIN_E_ENC_B)) encE = encE + 1; else encE = encE - 1;
}

// --- TASKS ---

// Core 0: Network & Thermal
void networkTask(void *pvParameters) {
    // Initialize WebServer
    webServer = new WebServerManager(&thermal, &gcodeStream);
    webServer->begin();

    char buffer[64];
    while (true) {
        // Handle Web Server & WebSockets
        webServer->update();

        // Broadcast Status (e.g., every 100ms)
        static long lastStatus = 0;
        if (millis() - lastStatus > 100) {
            if (isHalted) {
                webServer->broadcastError(haltReason);
            } else {
                webServer->broadcastStatus(encX, encY, encZ, encE); // Send raw encoder counts for now
            }
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
    bool isRelative;
    bool isHoming;
};

void parserTask(void *pvParameters) {
    char buffer[64];
    MotionCommand cmd;
    // Initialize targets to 0
    cmd.targetX = 0; cmd.targetY = 0; cmd.targetZ = 0; cmd.targetE = 0;
    cmd.isRelative = true; // Default to relative for jog buttons
    cmd.isHoming = false;

    // Wait for stream to be initialized
    while (gcodeStream == NULL || motionQueue == NULL) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    while (true) {
        size_t bytes = xStreamBufferReceive(gcodeStream, buffer, 64, portMAX_DELAY);
        if (bytes > 0) {
            buffer[bytes] = '\0'; // Ensure null termination
            String line = String(buffer);
            line.trim();
            
            if (line.length() == 0) continue;

            // Parse G-Code
            if (line.startsWith("G1") || line.startsWith("G0")) {
                // Linear Move
                cmd.isHoming = false;
                
                // Parse X
                int xIdx = line.indexOf('X');
                if (xIdx != -1) {
                    cmd.targetX = line.substring(xIdx + 1).toInt();
                } else {
                    cmd.targetX = 0; // No change if relative, or 0 if absolute (needs state tracking)
                }

                // Parse Y
                int yIdx = line.indexOf('Y');
                if (yIdx != -1) {
                    cmd.targetY = line.substring(yIdx + 1).toInt();
                } else {
                    cmd.targetY = 0;
                }

                // Parse Z
                int zIdx = line.indexOf('Z');
                if (zIdx != -1) {
                    cmd.targetZ = line.substring(zIdx + 1).toInt();
                } else {
                    cmd.targetZ = 0;
                }

                // Parse E
                int eIdx = line.indexOf('E');
                if (eIdx != -1) {
                    cmd.targetE = line.substring(eIdx + 1).toInt();
                } else {
                    cmd.targetE = 0;
                }

                xQueueSend(motionQueue, &cmd, portMAX_DELAY);
            }
            else if (line.startsWith("G28")) {
                // Homing (Fake for now - just reset encoders)
                cmd.isHoming = true;
                xQueueSend(motionQueue, &cmd, portMAX_DELAY);
            }
            else if (line.startsWith("M104") || line.startsWith("M140")) {
                // Temp commands (TODO: Send to Thermal Task)
            }
        }
    }
}

void controlTask(void *pvParameters) {
    MotionCommand currentTarget = {0, 0, 0, 0, false, false};
    long setpointX = 0, setpointY = 0, setpointZ = 0, setpointE = 0;
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = 1; // 1ms = 1kHz

    // Wait for queue to be initialized
    while (motionQueue == NULL) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    while (true) {
        if (isHalted) {
            // Stop all motors if halted
            motorX.setSpeed(0);
            motorY.setSpeed(0);
            motorZ.setSpeed(0);
            motorE.setSpeed(0);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        // Check for new target
        MotionCommand newCmd;
        if (xQueueReceive(motionQueue, &newCmd, 0) == pdTRUE) {
            if (newCmd.isHoming) {
                // Reset Encoders
                encX = 0; encY = 0; encZ = 0; encE = 0;
                setpointX = 0; setpointY = 0; setpointZ = 0; setpointE = 0;
                pidX.reset(); pidY.reset(); pidZ.reset(); pidE.reset();
            } else {
                // Update Setpoints
                // Note: The frontend sends "G1 X10" for jog. 
                // If we assume relative mode for jogs:
                setpointX += newCmd.targetX * 100; // Scale factor for steps/mm? 
                setpointY += newCmd.targetY * 100; // Let's assume 100 counts per unit for now
                setpointZ += newCmd.targetZ * 100;
                setpointE += newCmd.targetE * 100;
            }
        }

        // PID Loops
        int outX = pidX.compute(setpointX, encX);
        int outY = pidY.compute(setpointY, encY);
        int outZ = pidZ.compute(setpointZ, encZ);
        int outE = pidE.compute(setpointE, encE);

        // Check for Following Error (Stall Detection)
        if (abs(setpointX - encX) > MAX_FOLLOWING_ERROR) {
            isHalted = true;
            haltReason = "X Axis Stall Detected";
        }
        if (abs(setpointY - encY) > MAX_FOLLOWING_ERROR) {
            isHalted = true;
            haltReason = "Y Axis Stall Detected";
        }
        if (abs(setpointZ - encZ) > MAX_FOLLOWING_ERROR) {
            isHalted = true;
            haltReason = "Z Axis Stall Detected";
        }
        if (abs(setpointE - encE) > MAX_FOLLOWING_ERROR) {
            isHalted = true;
            haltReason = "E Axis Stall Detected";
        }

        // Drive Motors
        if (!isHalted) {
            motorX.setSpeed(outX);
            motorY.setSpeed(outY);
            motorZ.setSpeed(outZ);
            motorE.setSpeed(outE);
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void setup() {
    Serial.begin(115200);

    // Init Hardware
    motorX.begin(); motorY.begin(); motorZ.begin(); motorE.begin();
    thermal.begin();

    // Init Encoders (Use PULLUP to prevent floating noise)
    pinMode(PIN_X_ENC_A, INPUT_PULLUP); pinMode(PIN_X_ENC_B, INPUT_PULLUP);
    pinMode(PIN_Y_ENC_A, INPUT_PULLUP); pinMode(PIN_Y_ENC_B, INPUT_PULLUP);
    pinMode(PIN_Z_ENC_A, INPUT_PULLUP); pinMode(PIN_Z_ENC_B, INPUT_PULLUP);
    pinMode(PIN_E_ENC_A, INPUT_PULLUP); pinMode(PIN_E_ENC_B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_X_ENC_A), isrX, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_Y_ENC_A), isrY, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_Z_ENC_A), isrZ, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_E_ENC_A), isrE, CHANGE);

    // Init RTOS Objects
    gcodeStream = xStreamBufferCreate(1024, 1);
    motionQueue = xQueueCreate(10, sizeof(MotionCommand));

    // Create Tasks
    xTaskCreatePinnedToCore(networkTask, "Network", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(thermalTask, "Thermal", 2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(parserTask, "Parser", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(controlTask, "Control", 4096, NULL, 2, NULL, 1); // High Priority
}

void loop() {
    vTaskDelete(NULL); // Loop not used
}

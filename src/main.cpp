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
            webServer->broadcastStatus(encX, encY, encZ, encE); // Send raw encoder counts for now
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
};

void parserTask(void *pvParameters) {
    char buffer[64];
    MotionCommand cmd;
    // Initialize targets to 0
    cmd.targetX = 0; cmd.targetY = 0; cmd.targetZ = 0; cmd.targetE = 0;

    // Wait for stream to be initialized
    while (gcodeStream == NULL || motionQueue == NULL) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    while (true) {
        size_t bytes = xStreamBufferReceive(gcodeStream, buffer, 64, portMAX_DELAY);
        if (bytes > 0) {
            // TODO: Real G-Code Parsing
            // Simple test: "X1000" moves X to 1000
            if (buffer[0] == 'X') {
                cmd.targetX = atol(&buffer[1]);
                xQueueSend(motionQueue, &cmd, portMAX_DELAY);
            }
        }
    }
}

void controlTask(void *pvParameters) {
    MotionCommand currentTarget = {0, 0, 0, 0};
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = 1; // 1ms = 1kHz

    // Wait for queue to be initialized
    while (motionQueue == NULL) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    while (true) {
        // Check for new target
        if (xQueueReceive(motionQueue, &currentTarget, 0) == pdTRUE) {
            // New target received
        }

        // PID Loops
        int outX = pidX.compute(currentTarget.targetX, encX);
        int outY = pidY.compute(currentTarget.targetY, encY);
        int outZ = pidZ.compute(currentTarget.targetZ, encZ);
        int outE = pidE.compute(currentTarget.targetE, encE);

        // Drive Motors
        motorX.setSpeed(outX);
        motorY.setSpeed(outY);
        motorZ.setSpeed(outZ);
        motorE.setSpeed(outE);

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void setup() {
    Serial.begin(115200);

    // Init Hardware
    motorX.begin(); motorY.begin(); motorZ.begin(); motorE.begin();
    thermal.begin();

    // Init Encoders
    pinMode(PIN_X_ENC_A, INPUT); pinMode(PIN_X_ENC_B, INPUT);
    pinMode(PIN_Y_ENC_A, INPUT); pinMode(PIN_Y_ENC_B, INPUT);
    pinMode(PIN_Z_ENC_A, INPUT); pinMode(PIN_Z_ENC_B, INPUT);
    pinMode(PIN_E_ENC_A, INPUT); pinMode(PIN_E_ENC_B, INPUT);

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

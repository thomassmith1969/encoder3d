/*
 * Encoder3D CNC Controller - Configuration
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 * 
 * Hardware and system configuration settings
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================

// Motor Configuration - 6 axis encoder motors
#define NUM_MOTORS 6

// Motor indices
#define MOTOR_X1 0
#define MOTOR_X2 1
#define MOTOR_Y1 2
#define MOTOR_Y2 3
#define MOTOR_Z 4
#define MOTOR_E 5

// Motor pin assignments (PWM, DIR, ENCODER_A, ENCODER_B)
struct MotorPins {
    uint8_t pwm;
    uint8_t dir;
    uint8_t enc_a;
    uint8_t enc_b;
    uint8_t enable;
};

// Pin definitions for Lolin32 Lite
const MotorPins MOTOR_PINS[NUM_MOTORS] = {
    {25, 26, 34, 35, 27},  // X1
    {32, 33, 36, 39, 14},  // X2
    {12, 13, 15, 2, 4},    // Y1
    {16, 17, 5, 18, 19},   // Y2
    {21, 22, 23, 19, 27},  // Z
    {0, 4, 16, 17, 5}      // Extruder
};

// Encoder Configuration
#define ENCODER_PPR 600  // Pulses per revolution
#define COUNTS_PER_REV (ENCODER_PPR * 4)  // Quadrature = 4x

// Motor Specifications
#define STEPS_PER_MM_X 80.0
#define STEPS_PER_MM_Y 80.0
#define STEPS_PER_MM_Z 400.0
#define STEPS_PER_MM_E 95.0

// Maximum speeds (mm/s)
#define MAX_SPEED_X 200.0
#define MAX_SPEED_Y 200.0
#define MAX_SPEED_Z 20.0
#define MAX_SPEED_E 50.0

// Maximum accelerations (mm/s^2)
#define MAX_ACCEL_X 3000.0
#define MAX_ACCEL_Y 3000.0
#define MAX_ACCEL_Z 100.0
#define MAX_ACCEL_E 1000.0

// ============================================================================
// HEATER CONFIGURATION
// ============================================================================

#define NUM_HEATERS 2

// Heater indices
#define HEATER_HOTEND 0
#define HEATER_BED 1

struct HeaterPins {
    uint8_t output;
    uint8_t thermistor;
    uint8_t max_temp_pin;  // Optional MAX31865 CS pin
};

const HeaterPins HEATER_PINS[NUM_HEATERS] = {
    {25, 34, 255},  // Hotend
    {26, 35, 255}   // Bed
};

// Heater limits
#define MAX_TEMP_HOTEND 280
#define MAX_TEMP_BED 120
#define MIN_TEMP_THRESHOLD 5
#define THERMAL_RUNAWAY_PERIOD 40000  // ms
#define THERMAL_RUNAWAY_HYSTERESIS 4  // degrees

// PID Settings (tune these for your hardware)
#define HOTEND_PID_KP 22.2
#define HOTEND_PID_KI 1.08
#define HOTEND_PID_KD 114.0

#define BED_PID_KP 70.0
#define BED_PID_KI 10.0
#define BED_PID_KD 200.0

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

#define WIFI_SSID "CNC_Controller"
#define WIFI_PASSWORD "encoder3d"
#define WIFI_AP_MODE true  // Set to false for station mode

#define WEB_SERVER_PORT 80
#define TELNET_PORT 23
#define WEBSOCKET_PORT 8080

// ============================================================================
// CONTROL SYSTEM CONFIGURATION
// ============================================================================

// PID Controller for motors
#define MOTOR_PID_KP 2.0
#define MOTOR_PID_KI 0.5
#define MOTOR_PID_KD 0.1
#define PID_OUTPUT_LIMIT 255

// Control loop timing
#define MOTOR_CONTROL_FREQ 1000  // Hz
#define HEATER_CONTROL_FREQ 10   // Hz
#define STATUS_UPDATE_FREQ 2     // Hz

// ============================================================================
// MACHINE LIMITS
// ============================================================================

#define MAX_X 200.0  // mm
#define MAX_Y 200.0  // mm
#define MAX_Z 200.0  // mm
#define MIN_X 0.0
#define MIN_Y 0.0
#define MIN_Z 0.0

// ============================================================================
// SD CARD CONFIGURATION
// ============================================================================

// SD Card SPI pins (hardware SPI)
#define SD_CS_PIN 5
#define SD_MOSI_PIN 23
#define SD_MISO_PIN 19
#define SD_CLK_PIN 18

// SD Card settings
#define SD_ENABLED true
#define SD_MAX_FILE_SIZE (100 * 1024 * 1024)  // 100MB max file size

// ============================================================================
// GCODE CONFIGURATION
// ============================================================================

#define GCODE_BUFFER_SIZE 256
#define COMMAND_QUEUE_SIZE 32

// ============================================================================
// SPINDLE/LASER CONFIGURATION (for CNC mode)
// ============================================================================

#define SPINDLE_PWM_PIN 32
#define SPINDLE_DIR_PIN 33
#define SPINDLE_ENABLE_PIN 14
#define LASER_PWM_PIN 32

#define MAX_SPINDLE_RPM 24000
#define MIN_SPINDLE_RPM 0

// ============================================================================
// OPERATIONAL MODES
// ============================================================================

enum OperationMode {
    MODE_3D_PRINTER,
    MODE_CNC_SPINDLE,
    MODE_LASER_CUTTER
};

#define DEFAULT_MODE MODE_3D_PRINTER

#endif // CONFIG_H

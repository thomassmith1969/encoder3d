#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- PIN DEFINITIONS (Matches DESIGN.md) ---

// X-AXIS (Moved to 32/33 for Internal Pullups)
#define PIN_X_MOTOR_A   4
#define PIN_X_MOTOR_B   13
#define PIN_X_ENC_A     32
#define PIN_X_ENC_B     33

// Y-AXIS (Moved to 15/16 for Internal Pullups)
#define PIN_Y_MOTOR_A   12
#define PIN_Y_MOTOR_B   14
#define PIN_Y_ENC_A     15
#define PIN_Y_ENC_B     16

// Z-AXIS (Unchanged - Has Pullups)
#define PIN_Z_MOTOR_A   27
#define PIN_Z_MOTOR_B   5
#define PIN_Z_ENC_A     19
#define PIN_Z_ENC_B     21

// E-AXIS (Unchanged - Has Pullups)
#define PIN_E_MOTOR_A   17
#define PIN_E_MOTOR_B   18
#define PIN_E_ENC_A     22
#define PIN_E_ENC_B     23

// THERMAL (Moved to Input-Only Pins 36/39)
#define PIN_HEATER_EXT  25
#define PIN_HEATER_BED  26
#define PIN_TEMP_EXT    36 // ADC1_CH0 (VP)
#define PIN_TEMP_BED    39 // ADC1_CH3 (VN)

// --- PWM CHANNELS (0-15) ---
#define PWM_CHAN_X      0
#define PWM_CHAN_Y      1
#define PWM_CHAN_Z      2
#define PWM_CHAN_E      3
#define PWM_CHAN_HEAT_E 4
#define PWM_CHAN_HEAT_B 5

#define PWM_FREQ        20000 // 20kHz for motors
#define PWM_RES         8     // 8-bit resolution (0-255)

// --- CONTROL LOOP ---
#define CONTROL_FREQ    1000  // 1kHz Control Loop
#define MAX_FOLLOWING_ERROR 400 // Max encoder counts error before stall (Lowered for sensitivity)
#define THERMAL_FREQ    10    // 10Hz Thermal Loop

// --- PID CONSTANTS (Placeholder - Needs Tuning) ---
#define KP_DEFAULT      1.0
#define KI_DEFAULT      0.0
#define KD_DEFAULT      0.0

#endif

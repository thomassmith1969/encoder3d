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
// Stall detection config
#define STALL_TIMEOUT_MS 300 // ms without encoder movement while motor commanded -> stall
#define MIN_MOTOR_COMMAND 20 // PWM threshold to consider motor actively driving
// Stall/warning tiers
#define STALL_WARNING_MS 150 // ms without encoder movement -> warning
#define FOLLOWING_ERROR_WARN 200 // encoder counts deviation to trigger a warning
#define FOLLOWING_ERROR_HALT 400 // encoder counts deviation to force a halt

// Position tolerance (counts) for considering a move finished
#define POSITION_TOLERANCE 5

// Command execution behavior
#define COMMAND_EXECUTE_TIMEOUT_MS 5000 // maximum time allowed per command (ms)

// --- Optional I/O (set to -1 if not present on your board) ---
// Fan, spindle and laser pins are optional. Configure to match hardware.
#define PIN_FAN         -1
#define PIN_SPINDLE     -1
#define PIN_LASER       -1

// PWM channels for optional outputs (change if overlapping other channels)
#define PWM_CHAN_FAN    6
#define PWM_CHAN_SPINDLE 7
#define PWM_CHAN_LASER  8

// Feedrate limits (mm/min). If feedrate is 0 in G-code, controller will use full power.
#define DEFAULT_FEEDRATE 1500
#define MAX_FEEDRATE     12000

// Position deviation thresholds (counts)
// - POSITION_WARN_TOLERANCE_COUNTS: deviation above this sends warnings (broadcast)
// - POSITION_HALT_TOLERANCE_COUNTS: deviation above this triggers a halt (broadcast error)
// Tune these to avoid false positives; encoders are high-resolution (counts).
#define POSITION_WARN_TOLERANCE_COUNTS 20
#define POSITION_HALT_TOLERANCE_COUNTS 200

// --- PID CONSTANTS (Placeholder - Needs Tuning) ---
#define KP_DEFAULT      1.0
#define KI_DEFAULT      0.0
#define KD_DEFAULT      0.0

#endif

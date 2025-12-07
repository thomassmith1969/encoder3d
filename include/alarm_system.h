/*
 * Encoder3D CNC Controller - Alarm System
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 * 
 * Comprehensive alarm monitoring and management system
 * Designed for integration with AI-driven manufacturing systems
 */

#ifndef ALARM_SYSTEM_H
#define ALARM_SYSTEM_H

#include <Arduino.h>
#include "config.h"

// Alarm severity levels
enum AlarmSeverity {
    ALARM_INFO,      // Informational, no action needed
    ALARM_WARNING,   // Warning, monitor closely
    ALARM_ERROR,     // Error, may affect quality
    ALARM_CRITICAL   // Critical, immediate action required
};

// Alarm types
enum AlarmType {
    ALARM_NONE = 0,
    
    // Motor alarms
    ALARM_MOTOR_POSITION_ERROR,      // Position error exceeds tolerance
    ALARM_MOTOR_VELOCITY_ERROR,      // Velocity error exceeds tolerance
    ALARM_MOTOR_STALL,               // Motor stall detected
    ALARM_MOTOR_ENCODER_FAULT,       // Encoder not responding
    ALARM_MOTOR_OVERSPEED,           // Exceeds max velocity
    ALARM_MOTOR_CURRENT_LIMIT,       // Current limiting active
    
    // Temperature alarms
    ALARM_TEMP_OVERSHOOT,            // Temperature overshot target
    ALARM_TEMP_UNDERSHOOT,           // Temperature below minimum
    ALARM_TEMP_THERMAL_RUNAWAY,      // Thermal runaway detected
    ALARM_TEMP_SENSOR_FAULT,         // Thermistor disconnected
    ALARM_TEMP_SETTLING_TIMEOUT,     // Failed to reach temperature
    ALARM_TEMP_OSCILLATION,          // Excessive temperature oscillation
    
    // System alarms
    ALARM_COMMUNICATION_TIMEOUT,     // No commands received
    ALARM_BUFFER_OVERFLOW,           // Command buffer full
    ALARM_POWER_FLUCTUATION,         // Power supply unstable
    ALARM_EMERGENCY_STOP,            // E-stop activated
    ALARM_LIMIT_SWITCH,              // Limit switch triggered
    
    // Quality alarms (for AI integration)
    ALARM_DIMENSIONAL_TOLERANCE,     // Part out of dimensional spec
    ALARM_SURFACE_QUALITY,           // Surface finish issue
    ALARM_LAYER_ADHESION,            // Layer bonding issue
    ALARM_EXTRUSION_INCONSISTENT,    // Flow rate variation
    
    ALARM_TYPE_COUNT
};

// Alarm state structure
struct Alarm {
    AlarmType type;
    AlarmSeverity severity;
    unsigned long timestamp;
    unsigned long duration;
    float value;                     // Current value that triggered alarm
    float threshold;                 // Threshold that was exceeded
    String message;
    bool active;
    bool acknowledged;
    uint16_t count;                  // Number of times this alarm occurred
};

// Tolerance configuration
struct ToleranceConfig {
    // Motor tolerances
    float motor_position_tolerance;   // mm
    float motor_velocity_tolerance;   // mm/s
    float motor_accel_tolerance;      // mm/s²
    
    // Temperature tolerances
    float temp_tolerance;             // °C
    float temp_overshoot_limit;       // °C
    float temp_settling_time;         // seconds
    float temp_oscillation_limit;     // °C amplitude
    
    // Quality tolerances (for future AI integration)
    float dimensional_tolerance;      // mm
    float surface_roughness_limit;    // Ra value
    float layer_height_variance;      // mm
    float extrusion_flow_variance;    // %
};

class AlarmSystem {
private:
    static const int MAX_ALARMS = 50;
    static const int MAX_ACTIVE_ALARMS = 20;
    
    Alarm alarms[MAX_ALARMS];
    int alarm_count;
    int active_alarm_count;
    
    ToleranceConfig tolerances;
    
    // Alarm callbacks for external notification
    typedef void (*AlarmCallback)(const Alarm& alarm);
    AlarmCallback alarm_callback;
    
    // Alarm filtering to prevent spam
    unsigned long last_alarm_time[ALARM_TYPE_COUNT];
    static const unsigned long ALARM_DEBOUNCE_MS = 1000;
    
    // Statistics
    unsigned long total_alarms_raised;
    unsigned long critical_alarms_count;
    
public:
    AlarmSystem();
    
    void begin();
    void update();
    
    // Tolerance configuration
    void setTolerances(const ToleranceConfig& config);
    ToleranceConfig getTolerances() { return tolerances; }
    
    void setMotorPositionTolerance(float tolerance);
    void setMotorVelocityTolerance(float tolerance);
    void setTempTolerance(float tolerance);
    void setTempOvershotLimit(float limit);
    
    // Alarm management
    bool raiseAlarm(AlarmType type, AlarmSeverity severity, float value, float threshold, String message);
    void clearAlarm(AlarmType type);
    void clearAllAlarms();
    void acknowledgeAlarm(int index);
    void acknowledgeAllAlarms();
    
    // Alarm queries
    bool hasActiveAlarms();
    bool hasCriticalAlarms();
    bool hasAlarmType(AlarmType type);
    int getActiveAlarmCount() { return active_alarm_count; }
    int getTotalAlarmCount() { return alarm_count; }
    Alarm* getAlarm(int index);
    Alarm* getActiveAlarms(int& count);
    
    // Statistics
    unsigned long getTotalAlarmsRaised() { return total_alarms_raised; }
    unsigned long getCriticalAlarmCount() { return critical_alarms_count; }
    
    // Alarm callback registration
    void setAlarmCallback(AlarmCallback callback) { alarm_callback = callback; }
    
    // Status reporting
    String getAlarmSummary();
    String getAlarmJSON();
    String getAlarmHistory(int count);
    
    // System health metrics
    float getSystemHealthScore(); // 0-100%
    
private:
    int findAlarmIndex(AlarmType type);
    int findFreeAlarmSlot();
    void shiftAlarmHistory();
    bool shouldDebounceAlarm(AlarmType type);
};

#endif // ALARM_SYSTEM_H

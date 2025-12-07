/*
 * Encoder3D CNC Controller - Alarm System Implementation
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "alarm_system.h"

AlarmSystem::AlarmSystem() 
    : alarm_count(0), active_alarm_count(0), alarm_callback(nullptr),
      total_alarms_raised(0), critical_alarms_count(0) {
    
    // Initialize alarm array
    for (int i = 0; i < MAX_ALARMS; i++) {
        alarms[i].type = ALARM_NONE;
        alarms[i].active = false;
        alarms[i].acknowledged = false;
        alarms[i].count = 0;
    }
    
    // Initialize debounce timers
    for (int i = 0; i < ALARM_TYPE_COUNT; i++) {
        last_alarm_time[i] = 0;
    }
}

void AlarmSystem::begin() {
    // Set default tolerances
    tolerances.motor_position_tolerance = 0.5;      // 0.5mm
    tolerances.motor_velocity_tolerance = 10.0;     // 10mm/s
    tolerances.motor_accel_tolerance = 100.0;       // 100mm/s²
    
    tolerances.temp_tolerance = 2.0;                // ±2°C
    tolerances.temp_overshoot_limit = 5.0;          // 5°C overshoot
    tolerances.temp_settling_time = 30.0;           // 30 seconds
    tolerances.temp_oscillation_limit = 3.0;        // ±3°C oscillation
    
    tolerances.dimensional_tolerance = 0.1;         // 0.1mm
    tolerances.surface_roughness_limit = 3.2;       // Ra 3.2µm
    tolerances.layer_height_variance = 0.05;        // 0.05mm
    tolerances.extrusion_flow_variance = 5.0;       // 5%
    
    Serial.println("Alarm system initialized");
}

void AlarmSystem::update() {
    // Update alarm durations
    unsigned long now = millis();
    for (int i = 0; i < alarm_count; i++) {
        if (alarms[i].active) {
            alarms[i].duration = now - alarms[i].timestamp;
        }
    }
}

void AlarmSystem::setTolerances(const ToleranceConfig& config) {
    tolerances = config;
    Serial.println("Tolerances updated");
}

void AlarmSystem::setMotorPositionTolerance(float tolerance) {
    tolerances.motor_position_tolerance = tolerance;
}

void AlarmSystem::setMotorVelocityTolerance(float tolerance) {
    tolerances.motor_velocity_tolerance = tolerance;
}

void AlarmSystem::setTempTolerance(float tolerance) {
    tolerances.temp_tolerance = tolerance;
}

void AlarmSystem::setTempOvershotLimit(float limit) {
    tolerances.temp_overshoot_limit = limit;
}

bool AlarmSystem::raiseAlarm(AlarmType type, AlarmSeverity severity, float value, float threshold, String message) {
    // Check debounce
    if (shouldDebounceAlarm(type)) {
        return false;
    }
    
    // Update debounce timer
    last_alarm_time[type] = millis();
    
    // Find existing alarm of this type
    int index = findAlarmIndex(type);
    
    if (index >= 0) {
        // Update existing alarm
        alarms[index].severity = severity;
        alarms[index].value = value;
        alarms[index].threshold = threshold;
        alarms[index].message = message;
        alarms[index].count++;
        
        if (!alarms[index].active) {
            alarms[index].active = true;
            alarms[index].timestamp = millis();
            alarms[index].duration = 0;
            alarms[index].acknowledged = false;
            active_alarm_count++;
        }
    } else {
        // Create new alarm
        index = findFreeAlarmSlot();
        if (index < 0) {
            // No free slots, shift history
            shiftAlarmHistory();
            index = MAX_ALARMS - 1;
        }
        
        alarms[index].type = type;
        alarms[index].severity = severity;
        alarms[index].timestamp = millis();
        alarms[index].duration = 0;
        alarms[index].value = value;
        alarms[index].threshold = threshold;
        alarms[index].message = message;
        alarms[index].active = true;
        alarms[index].acknowledged = false;
        alarms[index].count = 1;
        
        if (index >= alarm_count) {
            alarm_count = index + 1;
        }
        active_alarm_count++;
    }
    
    // Update statistics
    total_alarms_raised++;
    if (severity == ALARM_CRITICAL) {
        critical_alarms_count++;
    }
    
    // Log alarm
    Serial.printf("ALARM [%s]: %s (Value: %.2f, Threshold: %.2f)\n",
                  severity == ALARM_CRITICAL ? "CRITICAL" :
                  severity == ALARM_ERROR ? "ERROR" :
                  severity == ALARM_WARNING ? "WARNING" : "INFO",
                  message.c_str(), value, threshold);
    
    // Call callback if registered
    if (alarm_callback) {
        alarm_callback(alarms[index]);
    }
    
    return true;
}

void AlarmSystem::clearAlarm(AlarmType type) {
    int index = findAlarmIndex(type);
    if (index >= 0 && alarms[index].active) {
        alarms[index].active = false;
        active_alarm_count--;
        Serial.printf("Alarm cleared: %s\n", alarms[index].message.c_str());
    }
}

void AlarmSystem::clearAllAlarms() {
    for (int i = 0; i < alarm_count; i++) {
        if (alarms[i].active) {
            alarms[i].active = false;
        }
    }
    active_alarm_count = 0;
    Serial.println("All alarms cleared");
}

void AlarmSystem::acknowledgeAlarm(int index) {
    if (index >= 0 && index < alarm_count) {
        alarms[index].acknowledged = true;
    }
}

void AlarmSystem::acknowledgeAllAlarms() {
    for (int i = 0; i < alarm_count; i++) {
        alarms[i].acknowledged = true;
    }
}

bool AlarmSystem::hasActiveAlarms() {
    return active_alarm_count > 0;
}

bool AlarmSystem::hasCriticalAlarms() {
    for (int i = 0; i < alarm_count; i++) {
        if (alarms[i].active && alarms[i].severity == ALARM_CRITICAL) {
            return true;
        }
    }
    return false;
}

bool AlarmSystem::hasAlarmType(AlarmType type) {
    int index = findAlarmIndex(type);
    return (index >= 0 && alarms[index].active);
}

Alarm* AlarmSystem::getAlarm(int index) {
    if (index >= 0 && index < alarm_count) {
        return &alarms[index];
    }
    return nullptr;
}

Alarm* AlarmSystem::getActiveAlarms(int& count) {
    static Alarm active_alarms[MAX_ACTIVE_ALARMS];
    count = 0;
    
    for (int i = 0; i < alarm_count && count < MAX_ACTIVE_ALARMS; i++) {
        if (alarms[i].active) {
            active_alarms[count++] = alarms[i];
        }
    }
    
    return active_alarms;
}

String AlarmSystem::getAlarmSummary() {
    if (active_alarm_count == 0) {
        return "No active alarms";
    }
    
    int critical = 0, errors = 0, warnings = 0;
    for (int i = 0; i < alarm_count; i++) {
        if (alarms[i].active) {
            switch (alarms[i].severity) {
                case ALARM_CRITICAL: critical++; break;
                case ALARM_ERROR: errors++; break;
                case ALARM_WARNING: warnings++; break;
                default: break;
            }
        }
    }
    
    String summary = "Active Alarms: ";
    if (critical > 0) summary += String(critical) + " Critical, ";
    if (errors > 0) summary += String(errors) + " Error, ";
    if (warnings > 0) summary += String(warnings) + " Warning";
    
    return summary;
}

String AlarmSystem::getAlarmJSON() {
    String json = "{\"active_alarms\":[";
    bool first = true;
    
    for (int i = 0; i < alarm_count; i++) {
        if (alarms[i].active) {
            if (!first) json += ",";
            first = false;
            
            json += "{";
            json += "\"type\":" + String((int)alarms[i].type) + ",";
            json += "\"severity\":" + String((int)alarms[i].severity) + ",";
            json += "\"message\":\"" + alarms[i].message + "\",";
            json += "\"value\":" + String(alarms[i].value, 2) + ",";
            json += "\"threshold\":" + String(alarms[i].threshold, 2) + ",";
            json += "\"duration\":" + String(alarms[i].duration) + ",";
            json += "\"acknowledged\":" + String(alarms[i].acknowledged ? "true" : "false") + ",";
            json += "\"count\":" + String(alarms[i].count);
            json += "}";
        }
    }
    
    json += "],";
    json += "\"total_count\":" + String(active_alarm_count) + ",";
    json += "\"health_score\":" + String(getSystemHealthScore(), 1);
    json += "}";
    
    return json;
}

String AlarmSystem::getAlarmHistory(int count) {
    String history = "Recent Alarms:\n";
    int displayed = 0;
    
    for (int i = alarm_count - 1; i >= 0 && displayed < count; i--) {
        history += String(alarm_count - i) + ". " + alarms[i].message;
        history += " [" + String(alarms[i].active ? "ACTIVE" : "Cleared") + "]\n";
        displayed++;
    }
    
    return history;
}

float AlarmSystem::getSystemHealthScore() {
    if (active_alarm_count == 0) {
        return 100.0;
    }
    
    // Calculate health score based on alarm severity and count
    float score = 100.0;
    
    for (int i = 0; i < alarm_count; i++) {
        if (alarms[i].active) {
            switch (alarms[i].severity) {
                case ALARM_CRITICAL: score -= 25.0; break;
                case ALARM_ERROR: score -= 10.0; break;
                case ALARM_WARNING: score -= 5.0; break;
                case ALARM_INFO: score -= 1.0; break;
            }
        }
    }
    
    return max(0.0f, score);
}

int AlarmSystem::findAlarmIndex(AlarmType type) {
    for (int i = 0; i < alarm_count; i++) {
        if (alarms[i].type == type) {
            return i;
        }
    }
    return -1;
}

int AlarmSystem::findFreeAlarmSlot() {
    for (int i = 0; i < MAX_ALARMS; i++) {
        if (alarms[i].type == ALARM_NONE) {
            return i;
        }
    }
    return -1;
}

void AlarmSystem::shiftAlarmHistory() {
    // Remove oldest inactive alarm
    for (int i = 0; i < MAX_ALARMS - 1; i++) {
        if (!alarms[i].active) {
            // Shift remaining alarms
            for (int j = i; j < MAX_ALARMS - 1; j++) {
                alarms[j] = alarms[j + 1];
            }
            alarms[MAX_ALARMS - 1].type = ALARM_NONE;
            alarms[MAX_ALARMS - 1].active = false;
            alarm_count = max(0, alarm_count - 1);
            return;
        }
    }
}

bool AlarmSystem::shouldDebounceAlarm(AlarmType type) {
    unsigned long now = millis();
    return (now - last_alarm_time[type]) < ALARM_DEBOUNCE_MS;
}

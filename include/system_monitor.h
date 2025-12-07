/*
 * Encoder3D CNC Controller - System Monitor
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 * 
 * Centralized system monitoring and health management
 * Integrates alarms, performance metrics, and diagnostics
 */

#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <Arduino.h>
#include "alarm_system.h"
#include "pid_tuner.h"
#include "motor_controller.h"
#include "heater_controller.h"

// System health status
enum SystemHealth {
    HEALTH_EXCELLENT,   // 90-100% - All systems optimal
    HEALTH_GOOD,        // 70-89% - Minor warnings
    HEALTH_FAIR,        // 50-69% - Multiple warnings or errors
    HEALTH_POOR,        // 30-49% - Critical issues present
    HEALTH_CRITICAL     // 0-29% - Immediate attention required
};

// Performance metrics
struct SystemMetrics {
    float system_health_score;
    int active_alarm_count;
    int critical_alarm_count;
    
    // Motor metrics
    float max_position_error;
    float avg_position_error;
    float max_velocity_error;
    
    // Temperature metrics
    float max_temp_error;
    float avg_temp_error;
    bool thermal_runaway;
    
    // System stats
    unsigned long uptime;
    float cpu_usage;
    unsigned int free_heap;
    unsigned int min_free_heap;
};

class SystemMonitor {
private:
    AlarmSystem* alarm_system;
    MotorController* motor_controller;
    HeaterController* heater_controller;
    
    SystemMetrics metrics;
    unsigned long last_update_time;
    unsigned long update_interval;
    
    // Performance tracking
    unsigned long task_start_times[10];
    unsigned long task_durations[10];
    int task_count;
    
    // Logging
    bool logging_enabled;
    
public:
    SystemMonitor();
    
    void begin(AlarmSystem* alarms, MotorController* motors, HeaterController* heaters);
    void update();
    
    // Health assessment
    SystemHealth getSystemHealth();
    float getHealthScore();
    SystemMetrics getMetrics();
    
    // Alarm management
    void checkAllAlarms();
    void acknowledgeAllAlarms();
    String getAlarmReport();
    
    // Performance monitoring
    void startTask(int task_id);
    void endTask(int task_id);
    float getTaskDuration(int task_id); // microseconds
    float getCPUUsage();
    
    // Diagnostic commands
    void runDiagnostics();
    void calibrateMotors();
    void tuneAllPIDs();
    
    // Status reporting
    String getStatusJSON();
    String getHealthReport();
    void printStatus();
    
    // Configuration
    void setUpdateInterval(unsigned long interval_ms);
    void enableLogging(bool enable);
    
private:
    void updateMetrics();
    void checkSystemAlarms();
    float calculateHealthScore();
};

#endif // SYSTEM_MONITOR_H

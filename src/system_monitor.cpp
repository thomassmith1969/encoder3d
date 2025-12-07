/*
 * Encoder3D CNC Controller - System Monitor Implementation
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "system_monitor.h"

SystemMonitor::SystemMonitor()
    : alarm_system(nullptr), motor_controller(nullptr), heater_controller(nullptr),
      last_update_time(0), update_interval(1000), task_count(0), logging_enabled(false) {
    
    memset(&metrics, 0, sizeof(SystemMetrics));
    memset(task_start_times, 0, sizeof(task_start_times));
    memset(task_durations, 0, sizeof(task_durations));
}

void SystemMonitor::begin(AlarmSystem* alarms, MotorController* motors, HeaterController* heaters) {
    alarm_system = alarms;
    motor_controller = motors;
    heater_controller = heaters;
    
    last_update_time = millis();
    
    Serial.println("System Monitor initialized");
}

void SystemMonitor::update() {
    unsigned long now = millis();
    
    if (now - last_update_time >= update_interval) {
        updateMetrics();
        checkAllAlarms();
        checkSystemAlarms();
        
        if (logging_enabled) {
            printStatus();
        }
        
        last_update_time = now;
    }
}

SystemHealth SystemMonitor::getSystemHealth() {
    float score = getHealthScore();
    
    if (score >= 90.0) return HEALTH_EXCELLENT;
    if (score >= 70.0) return HEALTH_GOOD;
    if (score >= 50.0) return HEALTH_FAIR;
    if (score >= 30.0) return HEALTH_POOR;
    return HEALTH_CRITICAL;
}

float SystemMonitor::getHealthScore() {
    return metrics.system_health_score;
}

SystemMetrics SystemMonitor::getMetrics() {
    return metrics;
}

void SystemMonitor::checkAllAlarms() {
    if (!alarm_system) return;
    
    alarm_system->update();
    
    metrics.active_alarm_count = alarm_system->getActiveAlarmCount();
    metrics.critical_alarm_count = 0;
    
    // Count critical alarms
    for (int i = 0; i < alarm_system->getTotalAlarmCount(); i++) {
        Alarm* alarm = alarm_system->getAlarm(i);
        if (alarm && alarm->active && alarm->severity == ALARM_CRITICAL) {
            metrics.critical_alarm_count++;
        }
    }
}

void SystemMonitor::acknowledgeAllAlarms() {
    if (alarm_system) {
        alarm_system->acknowledgeAllAlarms();
    }
}

String SystemMonitor::getAlarmReport() {
    if (!alarm_system) return "Alarm system not initialized";
    
    return alarm_system->getAlarmJSON();
}

void SystemMonitor::startTask(int task_id) {
    if (task_id >= 0 && task_id < 10) {
        task_start_times[task_id] = micros();
    }
}

void SystemMonitor::endTask(int task_id) {
    if (task_id >= 0 && task_id < 10) {
        unsigned long duration = micros() - task_start_times[task_id];
        task_durations[task_id] = duration;
    }
}

float SystemMonitor::getTaskDuration(int task_id) {
    if (task_id >= 0 && task_id < 10) {
        return task_durations[task_id];
    }
    return 0;
}

float SystemMonitor::getCPUUsage() {
    // Simple CPU usage estimation based on task durations
    unsigned long total_time = 0;
    for (int i = 0; i < 10; i++) {
        total_time += task_durations[i];
    }
    
    // Assume 1ms update interval
    float usage = (total_time / 1000.0) * 100.0;
    return min(100.0f, usage);
}

void SystemMonitor::runDiagnostics() {
    Serial.println("\\n========== SYSTEM DIAGNOSTICS ==========");
    
    // Test alarm system
    Serial.println("\\nAlarm System:");
    if (alarm_system) {
        Serial.printf("  Active alarms: %d\\n", alarm_system->getActiveAlarmCount());
        Serial.printf("  Total alarms: %d\\n", alarm_system->getTotalAlarmCount());
        Serial.printf("  Health score: %.1f%%\\n", alarm_system->getSystemHealthScore());
    } else {
        Serial.println("  NOT INITIALIZED");
    }
    
    // Test motors
    Serial.println("\\nMotor System:");
    if (motor_controller) {
        for (int i = 0; i < NUM_MOTORS; i++) {
            Serial.printf("  Motor %d: Pos=%.2fmm, Vel=%.2fmm/s\\n",
                         i,
                         motor_controller->getCurrentPosition(i),
                         motor_controller->getCurrentVelocity(i));
        }
    } else {
        Serial.println("  NOT INITIALIZED");
    }
    
    // Test heaters
    Serial.println("\\nHeater System:");
    if (heater_controller) {
        for (int i = 0; i < NUM_HEATERS; i++) {
            Serial.printf("  Heater %d: %.1f°C / %.1f°C\\n",
                         i,
                         heater_controller->getCurrentTemperature(i),
                         heater_controller->getTargetTemperature(i));
        }
    } else {
        Serial.println("  NOT INITIALIZED");
    }
    
    // Memory stats
    Serial.println("\\nMemory:");
    Serial.printf("  Free heap: %u bytes\\n", ESP.getFreeHeap());
    Serial.printf("  Min free heap: %u bytes\\n", ESP.getMinFreeHeap());
    Serial.printf("  Heap size: %u bytes\\n", ESP.getHeapSize());
    
    // System stats
    Serial.println("\\nSystem:");
    Serial.printf("  Uptime: %lu seconds\\n", millis() / 1000);
    Serial.printf("  CPU freq: %u MHz\\n", ESP.getCpuFreqMHz());
    Serial.printf("  Health: %.1f%%\\n", getHealthScore());
    
    Serial.println("\\n========================================\\n");
}

void SystemMonitor::calibrateMotors() {
    if (!motor_controller) return;
    
    Serial.println("Starting motor calibration...");
    
    // Reset all motor positions
    for (int i = 0; i < NUM_MOTORS; i++) {
        motor_controller->resetMotorPosition(i);
    }
    
    Serial.println("Motor calibration complete");
}

void SystemMonitor::tuneAllPIDs() {
    Serial.println("Starting PID auto-tuning...");
    Serial.println("This may take several minutes...");
    
    // TODO: Implement sequential auto-tuning for all motors and heaters
    // This would typically:
    // 1. Disable other systems
    // 2. Run relay feedback test on each controller
    // 3. Apply calculated PID values
    // 4. Verify performance
    
    Serial.println("PID tuning not yet implemented");
}

String SystemMonitor::getStatusJSON() {
    String json = "{";
    
    json += "\"health\":{";
    json += "\"score\":" + String(metrics.system_health_score, 1) + ",";
    json += "\"status\":\"" + String(
        getSystemHealth() == HEALTH_EXCELLENT ? "excellent" :
        getSystemHealth() == HEALTH_GOOD ? "good" :
        getSystemHealth() == HEALTH_FAIR ? "fair" :
        getSystemHealth() == HEALTH_POOR ? "poor" : "critical"
    ) + "\"";
    json += "},";
    
    json += "\"alarms\":{";
    json += "\"active\":" + String(metrics.active_alarm_count) + ",";
    json += "\"critical\":" + String(metrics.critical_alarm_count);
    json += "},";
    
    json += "\"motors\":{";
    json += "\"max_pos_error\":" + String(metrics.max_position_error, 3) + ",";
    json += "\"avg_pos_error\":" + String(metrics.avg_position_error, 3) + ",";
    json += "\"max_vel_error\":" + String(metrics.max_velocity_error, 2);
    json += "},";
    
    json += "\"temperature\":{";
    json += "\"max_error\":" + String(metrics.max_temp_error, 1) + ",";
    json += "\"avg_error\":" + String(metrics.avg_temp_error, 1) + ",";
    json += "\"thermal_runaway\":" + String(metrics.thermal_runaway ? "true" : "false");
    json += "},";
    
    json += "\"system\":{";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"cpu_usage\":" + String(metrics.cpu_usage, 1) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"min_free_heap\":" + String(ESP.getMinFreeHeap());
    json += "}";
    
    json += "}";
    
    return json;
}

String SystemMonitor::getHealthReport() {
    String report = "System Health Report\\n";
    report += "====================\\n";
    report += "Overall Health: " + String(metrics.system_health_score, 1) + "%\\n";
    report += "Status: ";
    
    switch (getSystemHealth()) {
        case HEALTH_EXCELLENT: report += "EXCELLENT\\n"; break;
        case HEALTH_GOOD: report += "GOOD\\n"; break;
        case HEALTH_FAIR: report += "FAIR\\n"; break;
        case HEALTH_POOR: report += "POOR\\n"; break;
        case HEALTH_CRITICAL: report += "CRITICAL\\n"; break;
    }
    
    report += "\\nAlarms: " + String(metrics.active_alarm_count) + " active";
    if (metrics.critical_alarm_count > 0) {
        report += " (" + String(metrics.critical_alarm_count) + " CRITICAL)";
    }
    report += "\\n";
    
    if (metrics.max_position_error > 0.1) {
        report += "⚠ Motor position error: " + String(metrics.max_position_error, 2) + "mm\\n";
    }
    
    if (metrics.max_temp_error > 5.0) {
        report += "⚠ Temperature error: " + String(metrics.max_temp_error, 1) + "°C\\n";
    }
    
    if (metrics.thermal_runaway) {
        report += "🔥 THERMAL RUNAWAY DETECTED!\\n";
    }
    
    return report;
}

void SystemMonitor::printStatus() {
    Serial.println(getHealthReport());
}

void SystemMonitor::setUpdateInterval(unsigned long interval_ms) {
    update_interval = interval_ms;
}

void SystemMonitor::enableLogging(bool enable) {
    logging_enabled = enable;
}

void SystemMonitor::updateMetrics() {
    metrics.uptime = millis() / 1000;
    metrics.cpu_usage = getCPUUsage();
    metrics.free_heap = ESP.getFreeHeap();
    metrics.min_free_heap = ESP.getMinFreeHeap();
    
    // Motor metrics
    if (motor_controller) {
        metrics.max_position_error = 0;
        metrics.avg_position_error = 0;
        metrics.max_velocity_error = 0;
        
        for (int i = 0; i < NUM_MOTORS; i++) {
            float pos = motor_controller->getCurrentPosition(i);
            float target = motor_controller->getTargetPosition(i);
            float pos_error = abs(target - pos);
            
            if (pos_error > metrics.max_position_error) {
                metrics.max_position_error = pos_error;
            }
            metrics.avg_position_error += pos_error;
        }
        metrics.avg_position_error /= NUM_MOTORS;
    }
    
    // Temperature metrics
    if (heater_controller) {
        metrics.max_temp_error = 0;
        metrics.avg_temp_error = 0;
        metrics.thermal_runaway = false;
        
        for (int i = 0; i < NUM_HEATERS; i++) {
            float temp = heater_controller->getCurrentTemperature(i);
            float target = heater_controller->getTargetTemperature(i);
            float temp_error = abs(target - temp);
            
            if (temp_error > metrics.max_temp_error) {
                metrics.max_temp_error = temp_error;
            }
            metrics.avg_temp_error += temp_error;
            
            // Check for thermal runaway would go here
        }
        metrics.avg_temp_error /= NUM_HEATERS;
    }
    
    // Calculate overall health score
    metrics.system_health_score = calculateHealthScore();
}

void SystemMonitor::checkSystemAlarms() {
    if (!alarm_system) return;
    
    // Check for memory issues
    if (ESP.getFreeHeap() < 10000) {
        alarm_system->raiseAlarm(
            ALARM_BUFFER_OVERFLOW,
            ESP.getFreeHeap() < 5000 ? ALARM_CRITICAL : ALARM_WARNING,
            ESP.getFreeHeap(),
            10000,
            "Low memory: " + String(ESP.getFreeHeap()) + " bytes free"
        );
    } else {
        alarm_system->clearAlarm(ALARM_BUFFER_OVERFLOW);
    }
}

float SystemMonitor::calculateHealthScore() {
    float score = 100.0;
    
    // Deduct for alarms
    if (alarm_system) {
        score = alarm_system->getSystemHealthScore();
    }
    
    // Additional deductions for system issues
    if (metrics.free_heap < 10000) {
        score -= 10.0;
    }
    
    if (metrics.max_position_error > 1.0) {
        score -= 15.0;
    }
    
    if (metrics.max_temp_error > 10.0) {
        score -= 15.0;
    }
    
    if (metrics.thermal_runaway) {
        score = 0; // Critical failure
    }
    
    return max(0.0f, min(100.0f, score));
}

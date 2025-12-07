/*
 * Encoder3D CNC Controller - Heater Controller Implementation
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "heater_controller.h"
#include <math.h>

// ============================================================================
// Thermistor Implementation
// ============================================================================

Thermistor::Thermistor(uint8_t analog_pin) 
    : pin(analog_pin), beta(3950), r0(100000), t0(25) {
}

float Thermistor::readTemperature() {
    int analog_value = analogRead(pin);
    float resistance = analogToResistance(analog_value);
    return resistanceToTemperature(resistance);
}

float Thermistor::analogToResistance(int analog_value) {
    // Assuming a voltage divider with 4.7k pullup
    const float SERIES_RESISTOR = 4700.0;
    const float ADC_MAX = 4095.0;  // 12-bit ADC
    
    if (analog_value <= 0) return 999999;
    if (analog_value >= ADC_MAX) return 0;
    
    float voltage_ratio = analog_value / ADC_MAX;
    float resistance = SERIES_RESISTOR * voltage_ratio / (1.0 - voltage_ratio);
    
    return resistance;
}

float Thermistor::resistanceToTemperature(float resistance) {
    // Steinhart-Hart equation simplified (Beta parameter equation)
    float steinhart;
    steinhart = resistance / r0;              // (R/Ro)
    steinhart = log(steinhart);               // ln(R/Ro)
    steinhart /= beta;                        // 1/B * ln(R/Ro)
    steinhart += 1.0 / (t0 + 273.15);         // + (1/To)
    steinhart = 1.0 / steinhart;              // Invert
    steinhart -= 273.15;                      // convert to C
    
    return steinhart;
}

// ============================================================================
// Heater Implementation
// ============================================================================

Heater::Heater(uint8_t heater_id, const HeaterPins& heater_pins, float max_temperature, float p, float i, float d)
    : id(heater_id), pins(heater_pins), max_temp(max_temperature), 
      kp(p), ki(i), kd(d), current_temp(0), target_temp(0),
      integral(0), prev_error(0), enabled(false), thermal_runaway_detected(false),
      pwm_channel(heater_id + 10),  // Offset to avoid motor PWM channels
      alarm_system(nullptr), pid_tuner(nullptr), temp_tolerance(2.0),
      last_alarm_check(0), settling_start_time(0), is_settling(false),
      temp_history_index(0) {
    
    thermistor = new Thermistor(pins.thermistor);
    last_time = millis();
    safety_timer = millis();
    last_temp = 0;
    
    // Initialize temperature history
    for (int i = 0; i < 20; i++) {
        temp_history[i] = 0;
    }
}

Heater::~Heater() {
    delete thermistor;
}

void Heater::begin() {
    pinMode(pins.output, OUTPUT);
    
    // Setup PWM for heater
    ledcSetup(pwm_channel, 1000, 8);  // 1kHz, 8-bit resolution
    ledcAttachPin(pins.output, pwm_channel);
    
    ledcWrite(pwm_channel, 0);
    
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
}

void Heater::update() {
    // Read current temperature
    current_temp = thermistor->readTemperature();
    
    // Store in history for analysis
    temp_history[temp_history_index] = current_temp;
    temp_history_index = (temp_history_index + 1) % 20;
    
    // Safety checks
    if (current_temp > max_temp) {
        emergencyShutdown();
        return;
    }
    
    if (current_temp < MIN_TEMP_THRESHOLD && target_temp > 0) {
        // Thermistor disconnected or shorted
        emergencyShutdown();
        return;
    }
    
    checkThermalRunaway();
    
    // Check alarms periodically (every 500ms)
    if (alarm_system && (millis() - last_alarm_check > 500)) {
        checkAlarms();
        last_alarm_check = millis();
    }
    
    // Update PID tuner if active
    if (pid_tuner && pid_tuner->isAutoTuning()) {
        pid_tuner->update();
    }
    
    if (!enabled || target_temp <= 0) {
        applyPower(0);
        return;
    }
    
    // Compute PID and apply
    float power = computePID();
    applyPower(power);
}

float Heater::computePID() {
    unsigned long now = millis();
    float dt = (now - last_time) / 1000.0;  // Convert to seconds
    
    if (dt <= 0) dt = 0.001;
    
    float error = target_temp - current_temp;
    
    // Proportional
    float p_term = kp * error;
    
    // Integral with anti-windup
    integral += error * dt;
    if (integral > 255) integral = 255;
    if (integral < 0) integral = 0;
    float i_term = ki * integral;
    
    // Derivative
    float derivative = (error - prev_error) / dt;
    float d_term = kd * derivative;
    
    float output = p_term + i_term + d_term;
    
    // Limit output to 0-255
    if (output > 255) output = 255;
    if (output < 0) output = 0;
    
    prev_error = error;
    last_time = now;
    
    return output;
}

void Heater::checkThermalRunaway() {
    unsigned long now = millis();
    
    if (enabled && target_temp > 0) {
        // Check if temperature is rising when it should be
        if (now - safety_timer > THERMAL_RUNAWAY_PERIOD) {
            float temp_change = current_temp - last_temp;
            
            // If target is much higher than current, temp should be rising
            if (target_temp - current_temp > THERMAL_RUNAWAY_HYSTERESIS) {
                if (temp_change < 1.0) {  // Not heating up enough
                    thermal_runaway_detected = true;
                    emergencyShutdown();
                    return;
                }
            }
            
            last_temp = current_temp;
            safety_timer = now;
        }
    } else {
        safety_timer = now;
        last_temp = current_temp;
    }
}

void Heater::applyPower(float power) {
    if (!enabled) {
        ledcWrite(pwm_channel, 0);
        return;
    }
    
    int pwm_value = (int)power;
    if (pwm_value > 255) pwm_value = 255;
    if (pwm_value < 0) pwm_value = 0;
    
    ledcWrite(pwm_channel, pwm_value);
}

void Heater::setTargetTemperature(float temp) {
    if (temp < 0) temp = 0;
    if (temp > max_temp) temp = max_temp;
    
    target_temp = temp;
    
    // Reset PID when changing target
    integral = 0;
    prev_error = 0;
}

float Heater::getTargetTemperature() {
    return target_temp;
}

float Heater::getCurrentTemperature() {
    return current_temp;
}

void Heater::enable() {
    enabled = true;
    thermal_runaway_detected = false;
}

void Heater::disable() {
    enabled = false;
    applyPower(0);
}

bool Heater::isEnabled() {
    return enabled;
}

bool Heater::isAtTarget(float tolerance) {
    return abs(current_temp - target_temp) < tolerance;
}

bool Heater::isThermalRunaway() {
    return thermal_runaway_detected;
}

void Heater::setPID(float p, float i, float d) {
    kp = p;
    ki = i;
    kd = d;
}

void Heater::emergencyShutdown() {
    enabled = false;
    target_temp = 0;
    applyPower(0);
}

// ============================================================================
// Heater Controller Implementation
// ============================================================================

HeaterController::HeaterController() : is_running(false), control_task_handle(NULL) {
    heaters[HEATER_HOTEND] = new Heater(
        HEATER_HOTEND, 
        HEATER_PINS[HEATER_HOTEND], 
        MAX_TEMP_HOTEND,
        HOTEND_PID_KP, HOTEND_PID_KI, HOTEND_PID_KD
    );
    
    heaters[HEATER_BED] = new Heater(
        HEATER_BED,
        HEATER_PINS[HEATER_BED],
        MAX_TEMP_BED,
        BED_PID_KP, BED_PID_KI, BED_PID_KD
    );
}

HeaterController::~HeaterController() {
    stopControlLoop();
    for (int i = 0; i < NUM_HEATERS; i++) {
        delete heaters[i];
    }
}

void HeaterController::begin() {
    for (int i = 0; i < NUM_HEATERS; i++) {
        heaters[i]->begin();
    }
}

void HeaterController::startControlLoop() {
    if (!is_running) {
        is_running = true;
        xTaskCreatePinnedToCore(
            controlLoopTask,
            "HeaterControl",
            4096,
            this,
            1,
            &control_task_handle,
            0  // Run on core 0
        );
    }
}

void HeaterController::stopControlLoop() {
    if (is_running) {
        is_running = false;
        if (control_task_handle != NULL) {
            vTaskDelete(control_task_handle);
            control_task_handle = NULL;
        }
    }
}

void HeaterController::controlLoopTask(void* parameter) {
    HeaterController* controller = (HeaterController*)parameter;
    
    const TickType_t xFrequency = pdMS_TO_TICKS(1000 / HEATER_CONTROL_FREQ);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (controller->is_running) {
        controller->controlLoop();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void HeaterController::controlLoop() {
    for (int i = 0; i < NUM_HEATERS; i++) {
        heaters[i]->update();
    }
}

void HeaterController::setTemperature(uint8_t heater_id, float temp) {
    if (heater_id < NUM_HEATERS) {
        heaters[heater_id]->setTargetTemperature(temp);
        if (temp > 0) {
            heaters[heater_id]->enable();
        }
    }
}

float HeaterController::getTemperature(uint8_t heater_id) {
    if (heater_id < NUM_HEATERS) {
        return heaters[heater_id]->getCurrentTemperature();
    }
    return 0;
}

float HeaterController::getTargetTemperature(uint8_t heater_id) {
    if (heater_id < NUM_HEATERS) {
        return heaters[heater_id]->getTargetTemperature();
    }
    return 0;
}

bool HeaterController::isAtTarget(uint8_t heater_id, float tolerance) {
    if (heater_id < NUM_HEATERS) {
        return heaters[heater_id]->isAtTarget(tolerance);
    }
    return false;
}

void HeaterController::enableHeater(uint8_t heater_id) {
    if (heater_id < NUM_HEATERS) {
        heaters[heater_id]->enable();
    }
}

void HeaterController::disableHeater(uint8_t heater_id) {
    if (heater_id < NUM_HEATERS) {
        heaters[heater_id]->disable();
    }
}

// ============================================================================
// Heater Alarm and Tuning Support
// ============================================================================

void Heater::setAlarmSystem(AlarmSystem* alarms) {
    alarm_system = alarms;
}

void Heater::setPIDTuner(PIDTuner* tuner) {
    pid_tuner = tuner;
}

void Heater::setTempTolerance(float tolerance) {
    temp_tolerance = tolerance;
}

void Heater::checkAlarms() {
    if (!alarm_system) return;
    
    float temp_error = abs(target_temp - current_temp);
    
    // Check if enabled and trying to heat
    if (enabled && target_temp > 0) {
        // Check temperature overshoot
        if (current_temp > target_temp + 5.0) {
            alarm_system->raiseAlarm(
                ALARM_TEMP_OVERSHOOT,
                current_temp > target_temp + 10.0 ? ALARM_ERROR : ALARM_WARNING,
                current_temp,
                target_temp + 5.0,
                "Heater " + String(id) + " overshoot: " + String(current_temp - target_temp, 1) + "°C"
            );\n        } else {
            alarm_system->clearAlarm(ALARM_TEMP_OVERSHOOT);
        }
        
        // Check settling timeout (30 seconds to reach target)
        if (!is_settling && temp_error > temp_tolerance) {
            is_settling = true;
            settling_start_time = millis();
        } else if (is_settling && temp_error <= temp_tolerance) {
            is_settling = false;
            alarm_system->clearAlarm(ALARM_TEMP_SETTLING_TIMEOUT);
        } else if (is_settling && (millis() - settling_start_time > 30000)) {
            alarm_system->raiseAlarm(
                ALARM_TEMP_SETTLING_TIMEOUT,
                ALARM_WARNING,
                millis() - settling_start_time,
                30000,
                "Heater " + String(id) + " settling timeout"
            );
        }
        
        // Check for oscillation (rapid temperature swings)
        float temp_variance = 0;
        float mean_temp = 0;
        for (int i = 0; i < 20; i++) {
            mean_temp += temp_history[i];
        }
        mean_temp /= 20.0;
        
        for (int i = 0; i < 20; i++) {
            float diff = temp_history[i] - mean_temp;
            temp_variance += diff * diff;
        }
        temp_variance /= 20.0;
        float std_dev = sqrt(temp_variance);
        
        if (std_dev > 3.0) {
            alarm_system->raiseAlarm(
                ALARM_TEMP_OSCILLATION,
                ALARM_WARNING,
                std_dev,
                3.0,
                "Heater " + String(id) + " oscillating: σ=" + String(std_dev, 2) + "°C"
            );
        } else {
            alarm_system->clearAlarm(ALARM_TEMP_OSCILLATION);
        }
    }
    
    // Check sensor fault (unrealistic readings)
    if (current_temp < MIN_TEMP_THRESHOLD || current_temp > 500) {
        alarm_system->raiseAlarm(
            ALARM_TEMP_SENSOR_FAULT,
            ALARM_CRITICAL,
            current_temp,
            25.0,
            "Heater " + String(id) + " sensor fault: " + String(current_temp, 1) + "°C"
        );
        emergencyShutdown();
    } else {
        alarm_system->clearAlarm(ALARM_TEMP_SENSOR_FAULT);
    }
    
    // Thermal runaway detection
    if (thermal_runaway_detected) {
        alarm_system->raiseAlarm(
            ALARM_TEMP_THERMAL_RUNAWAY,
            ALARM_CRITICAL,
            current_temp,
            target_temp,
            "Heater " + String(id) + " thermal runaway detected!"
        );
    } else {
        alarm_system->clearAlarm(ALARM_TEMP_THERMAL_RUNAWAY);
    }
}

float Heater::getTempError() {
    return target_temp - current_temp;
}

void Heater::startAutoTune() {
    if (!pid_tuner) return;
    
    // Start auto-tuning using relay feedback method
    pid_tuner->startAutoTune(TUNING_TYREUS_LUYBEN, &current_temp, &target_temp, &prev_error);
    
    Serial.printf("Starting auto-tune for heater %d at %.1f°C\\n", id, target_temp);
}

void HeaterController::emergencyShutdownAll() {
    for (int i = 0; i < NUM_HEATERS; i++) {
        heaters[i]->emergencyShutdown();
    }
}

bool HeaterController::anyThermalRunaway() {
    for (int i = 0; i < NUM_HEATERS; i++) {
        if (heaters[i]->isThermalRunaway()) {
            return true;
        }
    }
    return false;
}

Heater* HeaterController::getHeater(uint8_t heater_id) {
    if (heater_id < NUM_HEATERS) {
        return heaters[heater_id];
    }
    return nullptr;
}

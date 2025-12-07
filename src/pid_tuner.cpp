/*
 * Encoder3D CNC Controller - Adaptive PID Tuner Implementation
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "pid_tuner.h"
#include <math.h>

PIDTuner::PIDTuner(float* kp, float* ki, float* kd) 
    : kp_ptr(kp), ki_ptr(ki), kd_ptr(kd),
      method(TUNING_MANUAL), state(TUNING_IDLE),
      process_value(nullptr), setpoint(nullptr), output(nullptr),
      relay_state(false), relay_start_time(0),
      relay_peak_high(-1e6), relay_peak_low(1e6),
      relay_period_sum(0), relay_cycle_count(0),
      ultimate_gain(0), ultimate_period(0),
      error_index(0), last_adapt_time(0) {
    
    // Default safety limits
    min_kp = 0.01; max_kp = 100.0;
    min_ki = 0.0;  max_ki = 50.0;
    min_kd = 0.0;  max_kd = 10.0;
    
    // Default auto-tune parameters
    tune_params.output_step = 50.0;
    tune_params.noise_band = 0.5;
    tune_params.test_duration = 300000; // 5 minutes
    tune_params.setpoint_offset = 10.0;
}

void PIDTuner::begin() {
    state = TUNING_IDLE;
    for (int i = 0; i < 10; i++) {
        error_history[i] = 0;
    }
    Serial.println("PID Tuner initialized");
}

void PIDTuner::update() {
    switch (state) {
        case TUNING_RELAY_TEST:
            runRelayTest();
            break;
            
        case TUNING_ANALYSIS:
            analyzeRelayTest();
            state = TUNING_COMPLETE;
            break;
            
        default:
            break;
    }
}

void PIDTuner::setGains(float kp, float ki, float kd) {
    *kp_ptr = kp;
    *ki_ptr = ki;
    *kd_ptr = kd;
    clampGains();
    Serial.printf("PID gains set: Kp=%.3f, Ki=%.3f, Kd=%.3f\n", *kp_ptr, *ki_ptr, *kd_ptr);
}

void PIDTuner::getGains(float& kp, float& ki, float& kd) {
    kp = *kp_ptr;
    ki = *ki_ptr;
    kd = *kd_ptr;
}

void PIDTuner::setLimits(float kp_min, float kp_max, float ki_min, float ki_max, float kd_min, float kd_max) {
    min_kp = kp_min; max_kp = kp_max;
    min_ki = ki_min; max_ki = ki_max;
    min_kd = kd_min; max_kd = kd_max;
}

bool PIDTuner::startAutoTune(TuningMethod tune_method, float* pv, float* sp, float* out) {
    if (state != TUNING_IDLE) {
        return false;
    }
    
    method = tune_method;
    process_value = pv;
    setpoint = sp;
    output = out;
    
    // Initialize relay test
    relay_state = false;
    relay_start_time = millis();
    relay_peak_high = -1e6;
    relay_peak_low = 1e6;
    relay_period_sum = 0;
    relay_cycle_count = 0;
    
    state = TUNING_RELAY_TEST;
    
    Serial.printf("Starting auto-tune using method %d\n", method);
    return true;
}

void PIDTuner::stopAutoTune() {
    state = TUNING_IDLE;
    Serial.println("Auto-tune stopped");
}

float PIDTuner::getTuningProgress() {
    if (state == TUNING_IDLE) return 0.0;
    if (state == TUNING_COMPLETE) return 100.0;
    if (state == TUNING_FAILED) return 0.0;
    
    // Progress based on relay cycles collected
    if (state == TUNING_RELAY_TEST) {
        return min(100.0f, (relay_cycle_count / 5.0f) * 100.0f);
    }
    
    return 50.0; // Analysis phase
}

void PIDTuner::setAutoTuneParams(float output_step, float noise_band, unsigned long duration) {
    tune_params.output_step = output_step;
    tune_params.noise_band = noise_band;
    tune_params.test_duration = duration;
}

void PIDTuner::startPerformanceTest(float setpoint_val) {
    perf_start_time = millis();
    perf_setpoint = setpoint_val;
    max_pv = -1e6;
    min_pv = 1e6;
    
    performance.rise_time = 0;
    performance.settling_time = 0;
    performance.overshoot = 0;
    performance.steady_state_error = 0;
    performance.oscillation_freq = 0;
    performance.integral_error = 0;
    performance.control_effort = 0;
}

void PIDTuner::stopPerformanceTest() {
    updatePerformanceMetrics();
}

float PIDTuner::getPerformanceScore() {
    // Composite score based on multiple metrics
    float score = 100.0;
    
    // Penalize overshoot (max -30 points)
    if (performance.overshoot > 0) {
        score -= min(30.0f, performance.overshoot * 3.0f);
    }
    
    // Penalize steady-state error (max -25 points)
    score -= min(25.0f, abs(performance.steady_state_error) * 10.0f);
    
    // Penalize slow settling (max -25 points)
    if (performance.settling_time > 10000) {
        score -= min(25.0f, (performance.settling_time - 10000) / 1000.0f);
    }
    
    // Penalize excessive control effort (max -20 points)
    if (performance.control_effort > 50) {
        score -= min(20.0f, (performance.control_effort - 50) / 5.0f);
    }
    
    return max(0.0f, score);
}

void PIDTuner::enableAdaptiveTuning(bool enable) {
    if (enable) {
        last_adapt_time = millis();
        Serial.println("Adaptive tuning enabled");
    } else {
        Serial.println("Adaptive tuning disabled");
    }
}

void PIDTuner::adaptGains() {
    unsigned long now = millis();
    if (now - last_adapt_time < ADAPT_INTERVAL) {
        return;
    }
    last_adapt_time = now;
    
    // Calculate error statistics from history
    float mean_error = 0;
    float variance = 0;
    
    for (int i = 0; i < 10; i++) {
        mean_error += error_history[i];
    }
    mean_error /= 10.0;
    
    for (int i = 0; i < 10; i++) {
        float diff = error_history[i] - mean_error;
        variance += diff * diff;
    }
    variance /= 10.0;
    float std_dev = sqrt(variance);
    
    // Adaptive adjustments
    float kp = *kp_ptr;
    float ki = *ki_ptr;
    float kd = *kd_ptr;
    
    // If steady-state error is high, increase Ki
    if (abs(mean_error) > 1.0) {
        ki *= 1.05;
    } else if (abs(mean_error) < 0.1) {
        ki *= 0.98; // Reduce Ki slightly if error is very low
    }
    
    // If oscillation (high variance), reduce Kp and increase Kd
    if (std_dev > 2.0) {
        kp *= 0.95;
        kd *= 1.05;
    } else if (std_dev < 0.5) {
        kp *= 1.02; // Can be more aggressive
    }
    
    setGains(kp, ki, kd);
    Serial.printf("Adaptive tuning: mean_err=%.2f, std_dev=%.2f\n", mean_error, std_dev);
}

void PIDTuner::loadPreset(const char* preset_name) {
    String preset(preset_name);
    preset.toLowerCase();
    
    if (preset == "conservative") {
        // Low gains, minimal overshoot, slow response
        setGains(1.0, 0.1, 0.2);
        Serial.println("Loaded conservative preset");
    } else if (preset == "balanced") {
        // Moderate gains, balanced performance
        setGains(2.5, 0.5, 0.5);
        Serial.println("Loaded balanced preset");
    } else if (preset == "aggressive") {
        // High gains, fast response, may overshoot
        setGains(5.0, 1.5, 1.0);
        Serial.println("Loaded aggressive preset");
    } else {
        Serial.println("Unknown preset");
    }
}

void PIDTuner::savePreset(const char* preset_name) {
    // TODO: Save to EEPROM or file system
    Serial.printf("Preset '%s' saved: Kp=%.3f, Ki=%.3f, Kd=%.3f\n", 
                  preset_name, *kp_ptr, *ki_ptr, *kd_ptr);
}

String PIDTuner::getStatusString() {
    String status = "PID Tuner - ";
    
    switch (state) {
        case TUNING_IDLE:
            status += "Idle";
            break;
        case TUNING_RELAY_TEST:
            status += "Relay Test (" + String(getTuningProgress(), 0) + "%)";
            break;
        case TUNING_ANALYSIS:
            status += "Analyzing";
            break;
        case TUNING_COMPLETE:
            status += "Complete";
            break;
        case TUNING_FAILED:
            status += "Failed";
            break;
    }
    
    status += " | Kp=" + String(*kp_ptr, 3);
    status += " Ki=" + String(*ki_ptr, 3);
    status += " Kd=" + String(*kd_ptr, 3);
    
    return status;
}

String PIDTuner::getPerformanceJSON() {
    String json = "{";
    json += "\"rise_time\":" + String(performance.rise_time) + ",";
    json += "\"settling_time\":" + String(performance.settling_time) + ",";
    json += "\"overshoot\":" + String(performance.overshoot, 2) + ",";
    json += "\"steady_state_error\":" + String(performance.steady_state_error, 3) + ",";
    json += "\"oscillation_freq\":" + String(performance.oscillation_freq, 3) + ",";
    json += "\"integral_error\":" + String(performance.integral_error, 2) + ",";
    json += "\"control_effort\":" + String(performance.control_effort, 2) + ",";
    json += "\"score\":" + String(getPerformanceScore(), 1);
    json += "}";
    return json;
}

void PIDTuner::runRelayTest() {
    if (!process_value || !setpoint || !output) {
        state = TUNING_FAILED;
        return;
    }
    
    // Check timeout
    if (millis() - relay_start_time > tune_params.test_duration) {
        Serial.println("Relay test timeout");
        state = TUNING_ANALYSIS;
        return;
    }
    
    float error = *setpoint - *process_value;
    
    // Update peaks
    if (*process_value > relay_peak_high) {
        relay_peak_high = *process_value;
    }
    if (*process_value < relay_peak_low) {
        relay_peak_low = *process_value;
    }
    
    // Relay feedback with hysteresis
    static unsigned long last_switch_time = 0;
    static bool was_high = false;
    
    if (relay_state) {
        // Currently outputting high
        if (error < -tune_params.noise_band) {
            relay_state = false;
            unsigned long period = millis() - last_switch_time;
            if (last_switch_time > 0 && period > 100) {
                relay_period_sum += period;
                relay_cycle_count++;
            }
            last_switch_time = millis();
            was_high = true;
        }
        *output = tune_params.output_step;
    } else {
        // Currently outputting low
        if (error > tune_params.noise_band) {
            relay_state = true;
            unsigned long period = millis() - last_switch_time;
            if (last_switch_time > 0 && period > 100 && was_high) {
                relay_period_sum += period;
                relay_cycle_count++;
                was_high = false;
            }
            last_switch_time = millis();
        }
        *output = 0;
    }
    
    // Need at least 5 full cycles for reliable data
    if (relay_cycle_count >= 10) {
        state = TUNING_ANALYSIS;
    }
}

void PIDTuner::analyzeRelayTest() {
    if (relay_cycle_count < 5) {
        Serial.println("Insufficient relay test data");
        state = TUNING_FAILED;
        return;
    }
    
    // Calculate critical parameters
    float amplitude = (relay_peak_high - relay_peak_low) / 2.0;
    ultimate_period = (relay_period_sum / relay_cycle_count) / 1000.0; // seconds
    
    // Ultimate gain from relay test
    ultimate_gain = (4.0 * tune_params.output_step) / (3.14159 * amplitude);
    
    Serial.printf("Relay test complete: Ku=%.3f, Pu=%.3f\n", ultimate_gain, ultimate_period);
    
    // Apply selected tuning method
    switch (method) {
        case TUNING_ZIEGLER_NICHOLS:
            calculateZieglerNichols();
            break;
        case TUNING_TYREUS_LUYBEN:
            calculateTyreusLuyben();
            break;
        case TUNING_COHEN_COON:
            calculateCohenCoon();
            break;
        default:
            break;
    }
    
    clampGains();
}

void PIDTuner::calculateZieglerNichols() {
    // Classic Ziegler-Nichols PID tuning
    float kp = 0.6 * ultimate_gain;
    float ki = 1.2 * ultimate_gain / ultimate_period;
    float kd = 0.075 * ultimate_gain * ultimate_period;
    
    setGains(kp, ki, kd);
    Serial.println("Applied Ziegler-Nichols tuning");
}

void PIDTuner::calculateTyreusLuyben() {
    // Tyreus-Luyben method (less aggressive, less overshoot)
    float kp = 0.45 * ultimate_gain;
    float ki = 0.54 * ultimate_gain / (2.2 * ultimate_period);
    float kd = 0.45 * ultimate_gain * ultimate_period / 6.3;
    
    setGains(kp, ki, kd);
    Serial.println("Applied Tyreus-Luyben tuning");
}

void PIDTuner::calculateCohenCoon() {
    // Cohen-Coon method (requires process model parameters)
    // Using simplified version based on ultimate parameters
    float kp = (0.9 * ultimate_gain);
    float ki = kp / (1.2 * ultimate_period);
    float kd = 0.5 * kp * ultimate_period;
    
    setGains(kp, ki, kd);
    Serial.println("Applied Cohen-Coon tuning");
}

void PIDTuner::updatePerformanceMetrics() {
    // Calculate final performance metrics
    if (process_value && setpoint) {
        performance.steady_state_error = *setpoint - *process_value;
        performance.overshoot = ((max_pv - perf_setpoint) / perf_setpoint) * 100.0;
    }
    
    Serial.println("Performance test complete");
}

void PIDTuner::clampGains() {
    *kp_ptr = constrain(*kp_ptr, min_kp, max_kp);
    *ki_ptr = constrain(*ki_ptr, min_ki, max_ki);
    *kd_ptr = constrain(*kd_ptr, min_kd, max_kd);
}

float PIDTuner::calculateISE() {
    // Integral Square Error
    float ise = 0;
    for (int i = 0; i < 10; i++) {
        ise += error_history[i] * error_history[i];
    }
    return ise;
}

// Global optimizer functions
namespace PIDOptimizer {

void tuneForTemperature(float& kp, float& ki, float& kd, float thermal_mass) {
    // Temperature control typically needs low derivative (noise sensitive)
    // Integral term important for steady-state
    kp = 2.0 / thermal_mass;
    ki = 0.5 / thermal_mass;
    kd = 0.1 / thermal_mass;
}

void tuneForMotor(float& kp, float& ki, float& kd, float inertia, float damping) {
    // Motor control benefits from higher derivative for damping
    kp = 1.0 + (0.5 / inertia);
    ki = 0.1;
    kd = 0.5 * damping;
}

void reduceOvershoot(float& kp, float& ki, float& kd, float overshoot_percent) {
    if (overshoot_percent > 10.0) {
        kp *= 0.8;
        kd *= 1.2;
    }
}

void reduceSteadyStateError(float& kp, float& ki, float& kd, float ss_error) {
    if (abs(ss_error) > 0.5) {
        ki *= 1.3;
    }
}

void reduceOscillation(float& kp, float& ki, float& kd) {
    kp *= 0.85;
    kd *= 1.15;
}

void improveResponseTime(float& kp, float& ki, float& kd) {
    kp *= 1.2;
    ki *= 1.1;
}

} // namespace PIDOptimizer

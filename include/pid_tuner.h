/*
 * Encoder3D CNC Controller - Adaptive PID Tuner
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 * 
 * Automatic PID tuning and optimization system
 * Uses Ziegler-Nichols and adaptive methods for optimal control
 */

#ifndef PID_TUNER_H
#define PID_TUNER_H

#include <Arduino.h>

// Tuning methods
enum TuningMethod {
    TUNING_MANUAL,              // User-specified values
    TUNING_ZIEGLER_NICHOLS,     // Classic Z-N method
    TUNING_TYREUS_LUYBEN,       // T-L method (less overshoot)
    TUNING_COHEN_COON,          // C-C method for processes with lag
    TUNING_ADAPTIVE             // Continuous adaptive tuning
};

// Tuning state
enum TuningState {
    TUNING_IDLE,
    TUNING_INIT,
    TUNING_RELAY_TEST,          // Relay feedback test for auto-tuning
    TUNING_ANALYSIS,
    TUNING_COMPLETE,
    TUNING_FAILED
};

// Performance metrics
struct PIDPerformance {
    float rise_time;            // Time to reach 90% of setpoint
    float settling_time;        // Time to settle within tolerance
    float overshoot;            // Maximum overshoot percentage
    float steady_state_error;   // Final error at steady state
    float oscillation_freq;     // Oscillation frequency (Hz)
    float integral_error;       // Cumulative error (ISE)
    float control_effort;       // RMS of control output
};

// Auto-tuning parameters
struct AutoTuneParams {
    float output_step;          // Step size for relay test
    float noise_band;           // Dead-band for relay
    unsigned long test_duration; // Maximum test duration (ms)
    float setpoint_offset;      // Offset from current value for test
};

class PIDTuner {
private:
    // PID parameters
    float* kp_ptr;
    float* ki_ptr;
    float* kd_ptr;
    
    // Tuning state
    TuningMethod method;
    TuningState state;
    
    // Auto-tune data collection
    float* process_value;
    float* setpoint;
    float* output;
    
    AutoTuneParams tune_params;
    
    // Relay test data
    bool relay_state;
    unsigned long relay_start_time;
    float relay_peak_high;
    float relay_peak_low;
    float relay_period_sum;
    int relay_cycle_count;
    
    // Critical parameters (from relay test)
    float ultimate_gain;        // Ku
    float ultimate_period;      // Pu
    
    // Performance monitoring
    PIDPerformance performance;
    unsigned long perf_start_time;
    float perf_setpoint;
    float max_pv;
    float min_pv;
    
    // Adaptive tuning
    float error_history[10];
    int error_index;
    unsigned long last_adapt_time;
    static const unsigned long ADAPT_INTERVAL = 60000; // 1 minute
    
    // Safety limits
    float min_kp, max_kp;
    float min_ki, max_ki;
    float min_kd, max_kd;
    
public:
    PIDTuner(float* kp, float* ki, float* kd);
    
    void begin();
    void update();
    
    // Manual tuning
    void setGains(float kp, float ki, float kd);
    void getGains(float& kp, float& ki, float& kd);
    
    // Safety limits
    void setLimits(float kp_min, float kp_max, float ki_min, float ki_max, float kd_min, float kd_max);
    
    // Auto-tuning
    bool startAutoTune(TuningMethod method, float* pv, float* sp, float* out);
    void stopAutoTune();
    bool isAutoTuning() { return state != TUNING_IDLE; }
    TuningState getTuningState() { return state; }
    float getTuningProgress(); // 0-100%
    
    // Configure auto-tune parameters
    void setAutoTuneParams(float output_step, float noise_band, unsigned long duration);
    
    // Performance evaluation
    void startPerformanceTest(float setpoint_val);
    void stopPerformanceTest();
    PIDPerformance getPerformance() { return performance; }
    float getPerformanceScore(); // 0-100%
    
    // Adaptive tuning
    void enableAdaptiveTuning(bool enable);
    void adaptGains(); // Call periodically for continuous adaptation
    
    // Presets for common scenarios
    void loadPreset(const char* preset_name); // "conservative", "balanced", "aggressive"
    void savePreset(const char* preset_name);
    
    // Status
    String getStatusString();
    String getPerformanceJSON();
    
private:
    void runRelayTest();
    void analyzeRelayTest();
    void calculateZieglerNichols();
    void calculateTyreusLuyben();
    void calculateCohenCoon();
    void updatePerformanceMetrics();
    void clampGains();
    float calculateISE(); // Integral Square Error
};

// Global helper functions for PID optimization
namespace PIDOptimizer {
    // Heuristic tuning based on system characteristics
    void tuneForTemperature(float& kp, float& ki, float& kd, float thermal_mass);
    void tuneForMotor(float& kp, float& ki, float& kd, float inertia, float damping);
    
    // Performance-based adjustments
    void reduceOvershoot(float& kp, float& ki, float& kd, float overshoot_percent);
    void reduceSteadyStateError(float& kp, float& ki, float& kd, float ss_error);
    void reduceOscillation(float& kp, float& ki, float& kd);
    void improveResponseTime(float& kp, float& ki, float& kd);
}

#endif // PID_TUNER_H

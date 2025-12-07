/*
 * Encoder3D CNC Controller - Laser Controller Implementation
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "laser_controller.h"
#include <math.h>

// ============================================================================
// Predefined Laser Profiles
// ============================================================================

namespace LaserProfiles {

const LaserSpec CO2_40W = {
    .type = LASER_CO2_IR,
    .max_power = 40.0,
    .min_power = 5.0,
    .wavelength = 10600.0,
    .min_pwm_freq = 1000,
    .max_pwm_freq = 20000,
    .optimal_pwm_freq = 5000,
    .supports_ttl = true,
    .supports_analog = true,
    .supports_pwm = true,
    .focus_distance = 50.8,
    .spot_size = 0.1
};

const LaserSpec CO2_100W = {
    .type = LASER_CO2_IR,
    .max_power = 100.0,
    .min_power = 10.0,
    .wavelength = 10600.0,
    .min_pwm_freq = 1000,
    .max_pwm_freq = 20000,
    .optimal_pwm_freq = 5000,
    .supports_ttl = true,
    .supports_analog = true,
    .supports_pwm = true,
    .focus_distance = 63.5,
    .spot_size = 0.15
};

const LaserSpec DIODE_BLUE_5W = {
    .type = LASER_DIODE_BLUE,
    .max_power = 5.0,
    .min_power = 0.1,
    .wavelength = 445.0,
    .min_pwm_freq = 500,
    .max_pwm_freq = 50000,
    .optimal_pwm_freq = 10000,
    .supports_ttl = true,
    .supports_analog = false,
    .supports_pwm = true,
    .focus_distance = 10.0,
    .spot_size = 0.08
};

const LaserSpec DIODE_BLUE_10W = {
    .type = LASER_DIODE_BLUE,
    .max_power = 10.0,
    .min_power = 0.2,
    .wavelength = 450.0,
    .min_pwm_freq = 500,
    .max_pwm_freq = 50000,
    .optimal_pwm_freq = 10000,
    .supports_ttl = true,
    .supports_analog = false,
    .supports_pwm = true,
    .focus_distance = 10.0,
    .spot_size = 0.08
};

const LaserSpec DIODE_BLUE_20W = {
    .type = LASER_DIODE_BLUE,
    .max_power = 20.0,
    .min_power = 0.5,
    .wavelength = 450.0,
    .min_pwm_freq = 1000,
    .max_pwm_freq = 50000,
    .optimal_pwm_freq = 15000,
    .supports_ttl = true,
    .supports_analog = false,
    .supports_pwm = true,
    .focus_distance = 12.0,
    .spot_size = 0.1
};

const LaserSpec DIODE_RED_500MW = {
    .type = LASER_DIODE_RED,
    .max_power = 0.5,
    .min_power = 0.01,
    .wavelength = 650.0,
    .min_pwm_freq = 500,
    .max_pwm_freq = 30000,
    .optimal_pwm_freq = 5000,
    .supports_ttl = true,
    .supports_analog = false,
    .supports_pwm = true,
    .focus_distance = 5.0,
    .spot_size = 0.05
};

const LaserSpec FIBER_20W = {
    .type = LASER_FIBER,
    .max_power = 20.0,
    .min_power = 2.0,
    .wavelength = 1064.0,
    .min_pwm_freq = 5000,
    .max_pwm_freq = 100000,
    .optimal_pwm_freq = 20000,
    .supports_ttl = true,
    .supports_analog = true,
    .supports_pwm = true,
    .focus_distance = 100.0,
    .spot_size = 0.03
};

const LaserSpec FIBER_50W = {
    .type = LASER_FIBER,
    .max_power = 50.0,
    .min_power = 5.0,
    .wavelength = 1064.0,
    .min_pwm_freq = 5000,
    .max_pwm_freq = 100000,
    .optimal_pwm_freq = 20000,
    .supports_ttl = true,
    .supports_analog = true,
    .supports_pwm = true,
    .focus_distance = 110.0,
    .spot_size = 0.04
};

const LaserSpec WELDER_1000W = {
    .type = LASER_WELDER,
    .max_power = 1000.0,
    .min_power = 100.0,
    .wavelength = 1064.0,
    .min_pwm_freq = 1000,
    .max_pwm_freq = 50000,
    .optimal_pwm_freq = 10000,
    .supports_ttl = true,
    .supports_analog = true,
    .supports_pwm = true,
    .focus_distance = 150.0,
    .spot_size = 0.5
};

const LaserSpec WELDER_2000W = {
    .type = LASER_WELDER,
    .max_power = 2000.0,
    .min_power = 200.0,
    .wavelength = 1064.0,
    .min_pwm_freq = 1000,
    .max_pwm_freq = 50000,
    .optimal_pwm_freq = 10000,
    .supports_ttl = true,
    .supports_analog = true,
    .supports_pwm = true,
    .focus_distance = 160.0,
    .spot_size = 0.6
};

} // namespace LaserProfiles

// ============================================================================
// Laser Controller Implementation
// ============================================================================

LaserController::LaserController(uint8_t pwm, uint8_t enable, uint8_t analog, uint8_t ttl)
    : pwm_pin(pwm), enable_pin(enable), analog_pin(analog), ttl_pin(ttl),
      pwm_channel(6),  // Use channel 6 for laser PWM
      mode(LASER_MODE_PWM), power_unit(POWER_PERCENT),
      current_power(0), target_power(0), power_percent(0),
      enabled(false), firing(false), fire_start_time(0), total_fire_time(0),
      interlock_ok(true), enclosure_ok(true), air_assist_ok(true), water_flow_ok(true),
      last_safety_check(0), alarm_system(nullptr),
      ramping_enabled(false), ramp_rate(100.0), last_ramp_time(0),
      fire_count(0), total_runtime(0) {
    
    // Default safety settings
    safety.interlock_enabled = false;
    safety.enclosure_required = false;
    safety.air_assist_required = false;
    safety.water_cooling_required = false;
    safety.max_duty_cycle = 1.0;
    safety.max_continuous_time = 60000;  // 60 seconds max continuous
    safety.beam_detect_enabled = false;
    safety.interlock_pin = 255;
    safety.enclosure_pin = 255;
    safety.air_assist_pin = 255;
    safety.water_flow_pin = 255;
    
    // Default to blue diode 5W
    spec = LaserProfiles::DIODE_BLUE_5W;
}

void LaserController::begin() {
    // Setup pins
    pinMode(enable_pin, OUTPUT);
    digitalWrite(enable_pin, LOW);
    
    // Setup PWM
    ledcSetup(pwm_channel, spec.optimal_pwm_freq, 8);  // 8-bit resolution
    ledcAttachPin(pwm_pin, pwm_channel);
    ledcWrite(pwm_channel, 0);
    
    // Setup optional pins
    if (analog_pin != 255) {
        pinMode(analog_pin, OUTPUT);
        digitalWrite(analog_pin, LOW);
    }
    
    if (ttl_pin != 255) {
        pinMode(ttl_pin, OUTPUT);
        digitalWrite(ttl_pin, LOW);
    }
    
    // Setup safety pins
    if (safety.interlock_pin != 255) {
        pinMode(safety.interlock_pin, INPUT_PULLUP);
    }
    if (safety.enclosure_pin != 255) {
        pinMode(safety.enclosure_pin, INPUT_PULLUP);
    }
    if (safety.air_assist_pin != 255) {
        pinMode(safety.air_assist_pin, INPUT_PULLUP);
    }
    if (safety.water_flow_pin != 255) {
        pinMode(safety.water_flow_pin, INPUT_PULLUP);
    }
    
    Serial.printf("Laser controller initialized: %s, %.1fW max\n",
                  spec.type == LASER_CO2_IR ? "CO2" :
                  spec.type == LASER_DIODE_BLUE ? "Blue Diode" :
                  spec.type == LASER_DIODE_RED ? "Red Diode" :
                  spec.type == LASER_FIBER ? "Fiber" :
                  spec.type == LASER_WELDER ? "Welder" : "Custom",
                  spec.max_power);
}

void LaserController::update() {
    unsigned long now = millis();
    
    // Check safety every 100ms
    if (now - last_safety_check > 100) {
        checkSafetyLimits();
        last_safety_check = now;
    }
    
    // Update ramping if enabled
    if (ramping_enabled && firing) {
        updateRamping();
    }
    
    // Track fire time
    if (firing) {
        unsigned long fire_duration = now - fire_start_time;
        
        // Check maximum continuous fire time
        if (fire_duration > safety.max_continuous_time) {
            if (alarm_system) {
                alarm_system->raiseAlarm(
                    ALARM_MOTOR_OVERSPEED,  // Reuse for laser overheat
                    ALARM_WARNING,
                    fire_duration,
                    safety.max_continuous_time,
                    "Laser continuous fire time exceeded"
                );
            }
            stopFire();
        }
    }
}

void LaserController::setLaserType(LaserType type) {
    switch (type) {
        case LASER_CO2_IR:
            spec = LaserProfiles::CO2_40W;
            break;
        case LASER_DIODE_BLUE:
            spec = LaserProfiles::DIODE_BLUE_5W;
            break;
        case LASER_DIODE_RED:
            spec = LaserProfiles::DIODE_RED_500MW;
            break;
        case LASER_FIBER:
            spec = LaserProfiles::FIBER_20W;
            break;
        case LASER_WELDER:
            spec = LaserProfiles::WELDER_1000W;
            break;
        default:
            break;
    }
    
    // Reconfigure PWM frequency
    ledcSetup(pwm_channel, spec.optimal_pwm_freq, 8);
}

void LaserController::setLaserSpec(const LaserSpec& specification) {
    spec = specification;
    ledcSetup(pwm_channel, spec.optimal_pwm_freq, 8);
}

void LaserController::setLaserMode(LaserMode laser_mode) {
    mode = laser_mode;
}

void LaserController::setSafety(const LaserSafety& safety_config) {
    safety = safety_config;
    
    // Reconfigure safety pins
    if (safety.interlock_pin != 255) {
        pinMode(safety.interlock_pin, INPUT_PULLUP);
    }
    if (safety.enclosure_pin != 255) {
        pinMode(safety.enclosure_pin, INPUT_PULLUP);
    }
    if (safety.air_assist_pin != 255) {
        pinMode(safety.air_assist_pin, INPUT_PULLUP);
    }
    if (safety.water_flow_pin != 255) {
        pinMode(safety.water_flow_pin, INPUT_PULLUP);
    }
}

void LaserController::loadProfile(const char* profile_name) {
    String profile(profile_name);
    profile.toLowerCase();
    
    if (profile.indexOf("co2") >= 0) {
        if (profile.indexOf("100") >= 0) {
            spec = LaserProfiles::CO2_100W;
        } else {
            spec = LaserProfiles::CO2_40W;
        }
    } else if (profile.indexOf("blue") >= 0 || profile.indexOf("diode") >= 0) {
        if (profile.indexOf("20") >= 0) {
            spec = LaserProfiles::DIODE_BLUE_20W;
        } else if (profile.indexOf("10") >= 0) {
            spec = LaserProfiles::DIODE_BLUE_10W;
        } else {
            spec = LaserProfiles::DIODE_BLUE_5W;
        }
    } else if (profile.indexOf("red") >= 0) {
        spec = LaserProfiles::DIODE_RED_500MW;
    } else if (profile.indexOf("fiber") >= 0) {
        if (profile.indexOf("50") >= 0) {
            spec = LaserProfiles::FIBER_50W;
        } else {
            spec = LaserProfiles::FIBER_20W;
        }
    } else if (profile.indexOf("weld") >= 0) {
        if (profile.indexOf("2000") >= 0 || profile.indexOf("2k") >= 0) {
            spec = LaserProfiles::WELDER_2000W;
        } else {
            spec = LaserProfiles::WELDER_1000W;
        }
    }
    
    ledcSetup(pwm_channel, spec.optimal_pwm_freq, 8);
    Serial.printf("Loaded laser profile: %s\n", profile_name);
}

void LaserController::setPower(float power, PowerUnit unit) {
    power_unit = unit;
    
    switch (unit) {
        case POWER_PERCENT:
            setPowerPercent(power);
            break;
        case POWER_MILLIWATT:
            setPowerWatts(power / 1000.0);
            break;
        case POWER_WATT:
            setPowerWatts(power);
            break;
    }
}

void LaserController::setPowerPercent(float percent) {
    percent = constrain(percent, 0.0, 100.0);
    power_percent = percent;
    target_power = percentToWatts(percent);
    
    if (!ramping_enabled) {
        current_power = target_power;
        applyPower();
    }
}

void LaserController::setPowerWatts(float watts) {
    watts = constrain(watts, 0.0, spec.max_power);
    target_power = watts;
    power_percent = wattsToPercent(watts);
    
    if (!ramping_enabled) {
        current_power = target_power;
        applyPower();
    }
}

float LaserController::getPower(PowerUnit unit) {
    switch (unit) {
        case POWER_PERCENT:
            return power_percent;
        case POWER_MILLIWATT:
            return current_power * 1000.0;
        case POWER_WATT:
            return current_power;
    }
    return 0;
}

float LaserController::getPowerPercent() {
    return power_percent;
}

float LaserController::getPowerWatts() {
    return current_power;
}

void LaserController::enable() {
    if (!checkSafety()) {
        Serial.println("Laser safety check failed - cannot enable");
        return;
    }
    
    enabled = true;
    digitalWrite(enable_pin, HIGH);
    Serial.println("Laser enabled");
}

void LaserController::disable() {
    enabled = false;
    firing = false;
    digitalWrite(enable_pin, LOW);
    updatePWM(0);
    if (ttl_pin != 255) updateTTL(false);
    if (analog_pin != 255) updateAnalog(0);
    Serial.println("Laser disabled");
}

void LaserController::fire() {
    if (!enabled) {
        Serial.println("Laser not enabled");
        return;
    }
    
    if (!checkSafety()) {
        Serial.println("Laser safety check failed");
        emergencyStop();
        return;
    }
    
    firing = true;
    fire_start_time = millis();
    fire_count++;
    applyPower();
}

void LaserController::stopFire() {
    if (firing) {
        total_fire_time += millis() - fire_start_time;
    }
    firing = false;
    current_power = 0;
    applyPower();
}

bool LaserController::checkSafety() {
    interlock_ok = checkInterlock();
    enclosure_ok = checkEnclosure();
    air_assist_ok = checkAirAssist();
    water_flow_ok = checkWaterFlow();
    
    return interlock_ok && enclosure_ok && air_assist_ok && water_flow_ok;
}

bool LaserController::checkInterlock() {
    if (!safety.interlock_enabled || safety.interlock_pin == 255) {
        return true;
    }
    
    bool ok = digitalRead(safety.interlock_pin) == LOW;  // Active low
    if (!ok) {
        raiseInterlockAlarm();
    }
    return ok;
}

bool LaserController::checkEnclosure() {
    if (!safety.enclosure_required || safety.enclosure_pin == 255) {
        return true;
    }
    
    bool ok = digitalRead(safety.enclosure_pin) == LOW;  // Active low (closed)
    if (!ok) {
        raiseEnclosureAlarm();
    }
    return ok;
}

bool LaserController::checkAirAssist() {
    if (!safety.air_assist_required || safety.air_assist_pin == 255) {
        return true;
    }
    
    bool ok = digitalRead(safety.air_assist_pin) == HIGH;  // Active high (flowing)
    if (!ok) {
        raiseAirAssistAlarm();
    }
    return ok;
}

bool LaserController::checkWaterFlow() {
    if (!safety.water_cooling_required || safety.water_flow_pin == 255) {
        return true;
    }
    
    bool ok = digitalRead(safety.water_flow_pin) == HIGH;  // Active high (flowing)
    if (!ok) {
        raiseWaterFlowAlarm();
    }
    return ok;
}

void LaserController::emergencyStop() {
    firing = false;
    enabled = false;
    current_power = 0;
    target_power = 0;
    digitalWrite(enable_pin, LOW);
    updatePWM(0);
    if (ttl_pin != 255) updateTTL(false);
    if (analog_pin != 255) updateAnalog(0);
    
    Serial.println("LASER EMERGENCY STOP");
}

void LaserController::enableRamping(bool enable) {
    ramping_enabled = enable;
    last_ramp_time = millis();
}

void LaserController::setRampRate(float rate) {
    ramp_rate = rate;  // Watts per second
}

void LaserController::setSpindleSpeed(float rpm) {
    // Convert RPM to power percentage for M3/M4 compatibility
    float percent = (rpm / MAX_SPINDLE_RPM) * 100.0;
    setPowerPercent(percent);
}

void LaserController::setLaserPWM(float value) {
    // M106 compatibility (0-255)
    float percent = (value / 255.0) * 100.0;
    setPowerPercent(percent);
}

String LaserController::getStatus() {
    String status = "Laser: ";
    status += enabled ? "Enabled" : "Disabled";
    status += ", ";
    status += firing ? "Firing" : "Standby";
    status += ", Power: " + String(power_percent, 1) + "% (" + String(current_power, 1) + "W)";
    return status;
}

String LaserController::getStatusJSON() {
    String json = "{";
    json += "\"enabled\":" + String(enabled ? "true" : "false") + ",";
    json += "\"firing\":" + String(firing ? "true" : "false") + ",";
    json += "\"power_percent\":" + String(power_percent, 1) + ",";
    json += "\"power_watts\":" + String(current_power, 2) + ",";
    json += "\"max_power\":" + String(spec.max_power, 1) + ",";
    json += "\"type\":\"" + String(
        spec.type == LASER_CO2_IR ? "CO2" :
        spec.type == LASER_DIODE_BLUE ? "Blue Diode" :
        spec.type == LASER_DIODE_RED ? "Red Diode" :
        spec.type == LASER_FIBER ? "Fiber" :
        spec.type == LASER_WELDER ? "Welder" : "Custom"
    ) + "\",";
    json += "\"wavelength\":" + String(spec.wavelength, 0) + ",";
    json += "\"safety\":{";
    json += "\"interlock\":" + String(interlock_ok ? "true" : "false") + ",";
    json += "\"enclosure\":" + String(enclosure_ok ? "true" : "false") + ",";
    json += "\"air_assist\":" + String(air_assist_ok ? "true" : "false") + ",";
    json += "\"water_flow\":" + String(water_flow_ok ? "true" : "false");
    json += "},";
    json += "\"stats\":{";
    json += "\"fire_count\":" + String(fire_count) + ",";
    json += "\"total_fire_time\":" + String(total_fire_time);
    json += "}";
    json += "}";
    return json;
}

void LaserController::setAlarmSystem(AlarmSystem* alarms) {
    alarm_system = alarms;
}

void LaserController::setPulseMode(uint16_t frequency, float duty_cycle) {
    // Set PWM frequency and duty cycle for pulsed operation
    ledcSetup(pwm_channel, frequency, 8);
    updatePWM(duty_cycle);
}

void LaserController::setFocusHeight(float height) {
    // Future: control focus height with servo or stepper
    // For now, just log
    Serial.printf("Focus height set to: %.2fmm\n", height);
}

void LaserController::applyPower() {
    if (!enabled || !firing) {
        updatePWM(0);
        if (ttl_pin != 255) updateTTL(false);
        if (analog_pin != 255) updateAnalog(0);
        return;
    }
    
    float duty_cycle = current_power / spec.max_power;
    duty_cycle = constrain(duty_cycle, 0.0, safety.max_duty_cycle);
    
    switch (mode) {
        case LASER_MODE_PWM:
            updatePWM(duty_cycle);
            break;
            
        case LASER_MODE_TTL:
            if (ttl_pin != 255) {
                updateTTL(duty_cycle > 0.1);
            }
            break;
            
        case LASER_MODE_ANALOG:
            if (analog_pin != 255) {
                float voltage = duty_cycle * 10.0;  // 0-10V
                updateAnalog(voltage);
            }
            break;
            
        case LASER_MODE_CONTINUOUS:
            updatePWM(duty_cycle);
            break;
            
        default:
            break;
    }
}

void LaserController::updateRamping() {
    unsigned long now = millis();
    float dt = (now - last_ramp_time) / 1000.0;
    
    if (dt <= 0) return;
    
    float power_diff = target_power - current_power;
    float max_change = ramp_rate * dt;
    
    if (abs(power_diff) <= max_change) {
        current_power = target_power;
    } else {
        current_power += (power_diff > 0) ? max_change : -max_change;
    }
    
    applyPower();
    last_ramp_time = now;
}

void LaserController::checkSafetyLimits() {
    if (!checkSafety() && firing) {
        emergencyStop();
    }
}

void LaserController::updatePWM(float duty_cycle) {
    uint8_t pwm_value = (uint8_t)(duty_cycle * 255.0);
    ledcWrite(pwm_channel, pwm_value);
}

void LaserController::updateAnalog(float voltage) {
    // ESP32 DAC is 8-bit, 0-3.3V
    // For 0-10V, need external DAC or voltage divider
    uint8_t dac_value = (uint8_t)((voltage / 10.0) * 255.0);
    dacWrite(analog_pin, dac_value);
}

void LaserController::updateTTL(bool state) {
    digitalWrite(ttl_pin, state ? HIGH : LOW);
}

float LaserController::wattsToPercent(float watts) {
    return (watts / spec.max_power) * 100.0;
}

float LaserController::percentToWatts(float percent) {
    return (percent / 100.0) * spec.max_power;
}

void LaserController::raiseInterlockAlarm() {
    if (alarm_system) {
        alarm_system->raiseAlarm(
            ALARM_EMERGENCY_STOP,
            ALARM_CRITICAL,
            0,
            1,
            "Laser interlock open!"
        );
    }
}

void LaserController::raiseEnclosureAlarm() {
    if (alarm_system) {
        alarm_system->raiseAlarm(
            ALARM_EMERGENCY_STOP,
            ALARM_CRITICAL,
            0,
            1,
            "Laser enclosure open!"
        );
    }
}

void LaserController::raiseAirAssistAlarm() {
    if (alarm_system) {
        alarm_system->raiseAlarm(
            ALARM_EMERGENCY_STOP,
            ALARM_ERROR,
            0,
            1,
            "Laser air assist fault!"
        );
    }
}

void LaserController::raiseWaterFlowAlarm() {
    if (alarm_system) {
        alarm_system->raiseAlarm(
            ALARM_EMERGENCY_STOP,
            ALARM_CRITICAL,
            0,
            1,
            "Laser water cooling fault!"
        );
    }
}

void LaserController::raiseDutyCycleAlarm() {
    if (alarm_system) {
        alarm_system->raiseAlarm(
            ALARM_MOTOR_OVERSPEED,  // Reuse
            ALARM_WARNING,
            current_power / spec.max_power,
            safety.max_duty_cycle,
            "Laser duty cycle limit exceeded"
        );
    }
}

// ============================================================================
// Spindle Controller Implementation
// ============================================================================

SpindleController::SpindleController(uint8_t pwm, uint8_t dir, uint8_t enable)
    : pwm_pin(pwm), dir_pin(dir), enable_pin(enable), pwm_channel(7),
      current_rpm(0), target_rpm(0), max_rpm(MAX_SPINDLE_RPM), min_rpm(0),
      enabled(false), clockwise(true), alarm_system(nullptr) {
}

void SpindleController::begin() {
    pinMode(dir_pin, OUTPUT);
    pinMode(enable_pin, OUTPUT);
    
    digitalWrite(dir_pin, HIGH);  // CW
    digitalWrite(enable_pin, LOW);
    
    ledcSetup(pwm_channel, 25000, 8);  // 25kHz PWM
    ledcAttachPin(pwm_pin, pwm_channel);
    ledcWrite(pwm_channel, 0);
    
    Serial.println("Spindle controller initialized");
}

void SpindleController::update() {
    // Could implement ramping here if needed
}

void SpindleController::setSpeed(float rpm) {
    target_rpm = constrain(rpm, min_rpm, max_rpm);
    current_rpm = target_rpm;
    if (enabled) {
        applySpeed();
    }
}

void SpindleController::setDirection(bool cw) {
    clockwise = cw;
    digitalWrite(dir_pin, cw ? HIGH : LOW);
}

void SpindleController::enable() {
    enabled = true;
    digitalWrite(enable_pin, HIGH);
    applySpeed();
}

void SpindleController::disable() {
    enabled = false;
    digitalWrite(enable_pin, LOW);
    ledcWrite(pwm_channel, 0);
}

void SpindleController::emergencyStop() {
    disable();
    current_rpm = 0;
    target_rpm = 0;
}

void SpindleController::setAlarmSystem(AlarmSystem* alarms) {
    alarm_system = alarms;
}

String SpindleController::getStatusJSON() {
    String json = "{";
    json += "\"enabled\":" + String(enabled ? "true" : "false") + ",";
    json += "\"rpm\":" + String(current_rpm, 0) + ",";
    json += "\"max_rpm\":" + String(max_rpm, 0) + ",";
    json += "\"direction\":\"" + String(clockwise ? "CW" : "CCW") + "\"";
    json += "}";
    return json;
}

void SpindleController::applySpeed() {
    if (!enabled) {
        ledcWrite(pwm_channel, 0);
        return;
    }
    
    uint8_t pwm = (uint8_t)rpmToPWM(current_rpm);
    ledcWrite(pwm_channel, pwm);
}

float SpindleController::rpmToPWM(float rpm) {
    return (rpm / max_rpm) * 255.0;
}

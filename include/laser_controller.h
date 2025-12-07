/*
 * Encoder3D CNC Controller - Laser Controller
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 * 
 * Comprehensive laser control supporting multiple laser types:
 * - Traditional IR CO2 lasers (10.6µm)
 * - Blue diode lasers (405-450nm)
 * - Red diode lasers (635-650nm)
 * - Fiber lasers (1064nm)
 * - Laser welders/cutters (various wavelengths)
 */

#ifndef LASER_CONTROLLER_H
#define LASER_CONTROLLER_H

#include <Arduino.h>
#include "config.h"
#include "alarm_system.h"

// Laser types supported
enum LaserType {
    LASER_NONE = 0,
    LASER_CO2_IR,           // CO2 IR laser (10.6µm) - traditional cutting/engraving
    LASER_DIODE_BLUE,       // Blue diode (405-450nm) - wood/acrylic engraving
    LASER_DIODE_RED,        // Red diode (635-650nm) - lower power engraving
    LASER_FIBER,            // Fiber laser (1064nm) - metal marking/cutting
    LASER_WELDER,           // Laser welder (various) - high power welding
    LASER_CUSTOM            // User-defined custom laser
};

// Laser control modes
enum LaserMode {
    LASER_MODE_OFF,         // Laser disabled
    LASER_MODE_CONTINUOUS,  // Continuous wave (CW)
    LASER_MODE_PWM,         // PWM power control
    LASER_MODE_TTL,         // TTL on/off only
    LASER_MODE_ANALOG       // Analog 0-10V control
};

// Laser power units
enum PowerUnit {
    POWER_PERCENT,          // 0-100%
    POWER_MILLIWATT,        // Absolute mW
    POWER_WATT              // Absolute W
};

// Laser safety features
struct LaserSafety {
    bool interlock_enabled;     // Hardware interlock switch
    bool enclosure_required;    // Enclosure door must be closed
    bool air_assist_required;   // Air assist must be active
    bool water_cooling_required; // Water cooling flow sensor
    float max_duty_cycle;       // Maximum duty cycle (0.0-1.0)
    float max_continuous_time;  // Max continuous fire time (ms)
    bool beam_detect_enabled;   // Beam detection sensor
    uint8_t interlock_pin;      // Pin for interlock switch
    uint8_t enclosure_pin;      // Pin for enclosure sensor
    uint8_t air_assist_pin;     // Pin for air assist sensor
    uint8_t water_flow_pin;     // Pin for water flow sensor
};

// Laser specifications
struct LaserSpec {
    LaserType type;
    float max_power;            // Maximum power in watts
    float min_power;            // Minimum stable power in watts
    float wavelength;           // Wavelength in nm
    uint16_t min_pwm_freq;      // Minimum PWM frequency (Hz)
    uint16_t max_pwm_freq;      // Maximum PWM frequency (Hz)
    uint16_t optimal_pwm_freq;  // Optimal PWM frequency (Hz)
    bool supports_ttl;          // TTL control available
    bool supports_analog;       // Analog control available
    bool supports_pwm;          // PWM control available
    float focus_distance;       // Optimal focus distance (mm)
    float spot_size;            // Beam spot size (mm)
};

// Predefined laser profiles
namespace LaserProfiles {
    extern const LaserSpec CO2_40W;
    extern const LaserSpec CO2_100W;
    extern const LaserSpec DIODE_BLUE_5W;
    extern const LaserSpec DIODE_BLUE_10W;
    extern const LaserSpec DIODE_BLUE_20W;
    extern const LaserSpec DIODE_RED_500MW;
    extern const LaserSpec FIBER_20W;
    extern const LaserSpec FIBER_50W;
    extern const LaserSpec WELDER_1000W;
    extern const LaserSpec WELDER_2000W;
}

// Laser controller class
class LaserController {
private:
    // Hardware pins
    uint8_t pwm_pin;
    uint8_t enable_pin;
    uint8_t analog_pin;         // For 0-10V analog control (DAC)
    uint8_t ttl_pin;            // For TTL control
    
    // PWM channel (ESP32)
    uint8_t pwm_channel;
    
    // Laser configuration
    LaserSpec spec;
    LaserMode mode;
    LaserSafety safety;
    
    // Power control
    float current_power;        // Current power in watts
    float target_power;         // Target power in watts
    float power_percent;        // Current power as percentage
    PowerUnit power_unit;
    
    // State
    bool enabled;
    bool firing;
    unsigned long fire_start_time;
    unsigned long total_fire_time;
    
    // Safety monitoring
    bool interlock_ok;
    bool enclosure_ok;
    bool air_assist_ok;
    bool water_flow_ok;
    unsigned long last_safety_check;
    
    // Alarm system integration
    AlarmSystem* alarm_system;
    
    // Ramping for smooth power transitions
    bool ramping_enabled;
    float ramp_rate;            // Power change per second
    unsigned long last_ramp_time;
    
    // Statistics
    unsigned long fire_count;
    unsigned long total_runtime;
    
public:
    LaserController(uint8_t pwm, uint8_t enable, uint8_t analog = 255, uint8_t ttl = 255);
    
    void begin();
    void update();
    
    // Configuration
    void setLaserType(LaserType type);
    void setLaserSpec(const LaserSpec& specification);
    void setLaserMode(LaserMode laser_mode);
    void setSafety(const LaserSafety& safety_config);
    void loadProfile(const char* profile_name);
    
    // Power control
    void setPower(float power, PowerUnit unit = POWER_PERCENT);
    void setPowerPercent(float percent);
    void setPowerWatts(float watts);
    float getPower(PowerUnit unit = POWER_PERCENT);
    float getPowerPercent();
    float getPowerWatts();
    
    // Laser control
    void enable();
    void disable();
    void fire();
    void stopFire();
    bool isFiring() { return firing; }
    bool isEnabled() { return enabled; }
    
    // Safety
    bool checkSafety();
    bool checkInterlock();
    bool checkEnclosure();
    bool checkAirAssist();
    bool checkWaterFlow();
    void emergencyStop();
    
    // Ramping
    void enableRamping(bool enable);
    void setRampRate(float rate);  // Watts per second
    
    // M-code compatibility
    void setSpindleSpeed(float rpm);  // For M3/M4 compatibility
    void setLaserPWM(float value);    // For M106 compatibility (0-255)
    
    // Status
    String getStatus();
    String getStatusJSON();
    unsigned long getTotalFireTime() { return total_fire_time; }
    unsigned long getFireCount() { return fire_count; }
    
    // Alarm integration
    void setAlarmSystem(AlarmSystem* alarms);
    
    // Advanced features
    void setPulseMode(uint16_t frequency, float duty_cycle);  // For pulsed operation
    void setFocusHeight(float height);  // Auto-focus control (future)
    
private:
    void applyPower();
    void updateRamping();
    void checkSafetyLimits();
    void updatePWM(float duty_cycle);
    void updateAnalog(float voltage);
    void updateTTL(bool state);
    float wattsToPercent(float watts);
    float percentToWatts(float percent);
    
    // Safety alarm handlers
    void raiseInterlockAlarm();
    void raiseEnclosureAlarm();
    void raiseAirAssistAlarm();
    void raiseWaterFlowAlarm();
    void raiseDutyCycleAlarm();
};

// Spindle controller for CNC mode (shares similar interface)
class SpindleController {
private:
    uint8_t pwm_pin;
    uint8_t dir_pin;
    uint8_t enable_pin;
    uint8_t pwm_channel;
    
    float current_rpm;
    float target_rpm;
    float max_rpm;
    float min_rpm;
    
    bool enabled;
    bool clockwise;
    
    AlarmSystem* alarm_system;
    
public:
    SpindleController(uint8_t pwm, uint8_t dir, uint8_t enable);
    
    void begin();
    void update();
    
    void setSpeed(float rpm);
    void setDirection(bool cw);
    void enable();
    void disable();
    void emergencyStop();
    
    float getSpeed() { return current_rpm; }
    bool isEnabled() { return enabled; }
    bool isClockwise() { return clockwise; }
    
    void setAlarmSystem(AlarmSystem* alarms);
    String getStatusJSON();
    
private:
    void applySpeed();
    float rpmToPWM(float rpm);
};

#endif // LASER_CONTROLLER_H

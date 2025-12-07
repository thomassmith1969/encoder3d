/**
 * Tool Controller - Comprehensive CNC End Effector Support
 * 
 * Supports various CNC tools:
 * - Spindles (cheap DC to water-cooled VFD)
 * - Plasma cutters
 * - Drag knives
 * - Pen plotters
 * - Hot wire cutters
 * - Vacuum/pneumatic tools
 * - 3D printer extruders (basic support)
 * 
 * Author: Thomas Smith
 * License: GPL-3.0
 */

#ifndef TOOL_CONTROLLER_H
#define TOOL_CONTROLLER_H

#include <Arduino.h>
#include "alarm_system.h"

// Tool types
enum ToolType {
    TOOL_NONE = 0,
    TOOL_SPINDLE_DC,           // Simple DC motor spindle
    TOOL_SPINDLE_BRUSHLESS,    // BLDC spindle (ESC control)
    TOOL_SPINDLE_VFD,          // VFD-controlled spindle (RS485/Modbus)
    TOOL_SPINDLE_WATER,        // Water-cooled spindle
    TOOL_PLASMA_TORCH,         // Plasma cutter
    TOOL_DRAG_KNIFE,           // Drag knife for vinyl/paper
    TOOL_PEN_PLOTTER,          // Pen plotter
    TOOL_HOT_WIRE,             // Hot wire foam cutter
    TOOL_VACUUM_PICKUP,        // Vacuum pick-and-place
    TOOL_PNEUMATIC_DRILL,      // Air-powered drill
    TOOL_EXTRUDER,             // 3D printer extruder
    TOOL_CUSTOM                // User-defined tool
};

// Spindle types (detailed)
enum SpindleType {
    SPINDLE_DC_775,            // 775 DC motor (cheap, 12-24V)
    SPINDLE_DC_555,            // 555 DC motor (smaller)
    SPINDLE_BRUSHLESS_ER11,    // ER11 collet, ESC control
    SPINDLE_BRUSHLESS_ER20,    // ER20 collet, ESC control
    SPINDLE_VFD_1_5KW,         // 1.5kW water-cooled VFD
    SPINDLE_VFD_2_2KW,         // 2.2kW water-cooled VFD
    SPINDLE_VFD_3_0KW,         // 3.0kW water-cooled VFD
    SPINDLE_MAKITA_RT0700,     // Makita trim router (popular DIY)
    SPINDLE_DEWALT_611,        // DeWalt trim router (popular DIY)
    SPINDLE_CUSTOM
};

// Plasma torch types
enum PlasmaType {
    PLASMA_PILOT_ARC,          // Pilot arc (Hypertherm, Everlast)
    PLASMA_HF_START,           // High-frequency start
    PLASMA_BLOWBACK_START,     // Blowback/touch start
    PLASMA_CUT50,              // Generic 50A cutter
    PLASMA_CUT60,              // Generic 60A cutter
    PLASMA_CUSTOM
};

// Tool control modes
enum ToolControlMode {
    CONTROL_PWM,               // PWM speed/power control
    CONTROL_ANALOG,            // 0-10V analog
    CONTROL_TTL,               // Simple on/off
    CONTROL_MODBUS,            // RS485 Modbus (VFD)
    CONTROL_STEP_DIR,          // Step/direction (unusual)
    CONTROL_RC_ESC,            // RC ESC (1-2ms pulse)
    CONTROL_RELAY              // Simple relay switching
};

// Tool safety features
struct ToolSafety {
    bool requires_coolant;          // Water/air cooling required
    bool requires_air_assist;       // Air assist required
    bool requires_torch_height;     // Torch height control (plasma)
    bool requires_interlock;        // Safety interlock
    bool has_tachometer;            // RPM feedback available
    bool has_temperature_sensor;    // Temperature monitoring
    bool has_current_sensor;        // Current monitoring
    bool requires_fume_extraction;  // Fume extraction needed
};

// Tool specifications
struct ToolSpec {
    ToolType type;
    String name;
    
    // Speed/power parameters
    float max_speed;                // Max RPM or power
    float min_speed;                // Min RPM or power
    float idle_speed;               // Idle/standby speed
    
    // Control parameters
    ToolControlMode control_mode;
    uint16_t pwm_frequency;         // PWM frequency in Hz
    float analog_min_voltage;       // For 0-10V control
    float analog_max_voltage;
    
    // Physical parameters
    float max_current;              // Maximum current draw (A)
    float rated_voltage;            // Rated voltage
    float collet_size;              // Collet size (mm)
    
    // Timing parameters
    uint32_t spinup_time;           // Time to reach full speed (ms)
    uint32_t spindown_time;         // Time to stop (ms)
    uint32_t min_on_time;           // Minimum on time (ms)
    uint32_t cooldown_time;         // Cooldown before shutdown (ms)
    
    // Safety
    ToolSafety safety;
    
    // Plasma-specific
    float pierce_height;            // Pierce height (mm)
    float cut_height;               // Cutting height (mm)
    uint32_t pierce_delay;          // Pierce delay (ms)
    
    // Thermal limits
    float max_temperature;          // Max temperature (°C)
    uint32_t max_duty_cycle;        // Max duty cycle (%)
};

// Predefined tool profiles
namespace ToolProfiles {
    // DC Spindles
    extern const ToolSpec DC_775_12V;
    extern const ToolSpec DC_775_24V;
    extern const ToolSpec DC_555_12V;
    
    // Brushless Spindles
    extern const ToolSpec BLDC_ER11_300W;
    extern const ToolSpec BLDC_ER11_500W;
    extern const ToolSpec BLDC_ER20_1000W;
    
    // VFD Spindles
    extern const ToolSpec VFD_1_5KW_WATER;
    extern const ToolSpec VFD_2_2KW_WATER;
    extern const ToolSpec VFD_3_0KW_WATER;
    
    // DIY Router Conversions
    extern const ToolSpec MAKITA_RT0700;
    extern const ToolSpec DEWALT_611;
    
    // Plasma Torches
    extern const ToolSpec PLASMA_CUT50_PILOT;
    extern const ToolSpec PLASMA_CUT60_PILOT;
    extern const ToolSpec PLASMA_HYPERTHERM_45;
    
    // Other Tools
    extern const ToolSpec DRAG_KNIFE_STANDARD;
    extern const ToolSpec PEN_PLOTTER_STANDARD;
    extern const ToolSpec HOT_WIRE_STANDARD;
    extern const ToolSpec VACUUM_PICKUP_STANDARD;
}

// Main tool controller class
class ToolController {
private:
    // Pin assignments
    uint8_t pwm_pin;
    uint8_t dir_pin;
    uint8_t enable_pin;
    uint8_t analog_pin;
    uint8_t tach_pin;
    uint8_t temp_pin;
    uint8_t coolant_pin;
    uint8_t air_pin;
    
    // PWM channel
    uint8_t pwm_channel;
    
    // Current state
    ToolSpec spec;
    bool enabled;
    bool direction_cw;
    float current_speed;
    float target_speed;
    float measured_rpm;
    float temperature;
    
    // Timing
    unsigned long last_update;
    unsigned long enable_time;
    unsigned long last_tach_time;
    unsigned long tach_pulse_count;
    
    // Safety
    AlarmSystem* alarm_system;
    bool coolant_flow_ok;
    bool air_pressure_ok;
    bool temperature_ok;
    bool interlock_ok;
    
    // Ramping
    bool ramping_enabled;
    float ramp_rate;  // Speed units per second
    
public:
    ToolController();
    
    // Initialization
    void begin();
    void setPins(uint8_t pwm, uint8_t dir, uint8_t enable, 
                 uint8_t analog = 255, uint8_t tach = 255);
    void setToolSpec(const ToolSpec& tool_spec);
    void loadProfile(const String& profile_name);
    
    // Control
    void update();
    void setSpeed(float speed);          // Set speed (RPM or %)
    void setSpeedRPM(float rpm);         // Set speed in RPM
    void setSpeedPercent(float percent); // Set speed as percentage
    void setDirection(bool clockwise);
    void enable();
    void disable();
    void emergencyStop();
    
    // Plasma-specific
    void setTorchHeight(float height_mm);
    void pierce();
    void arcOn();
    void arcOff();
    
    // Status
    float getSpeed() { return current_speed; }
    float getTargetSpeed() { return target_speed; }
    float getMeasuredRPM() { return measured_rpm; }
    float getTemperature() { return temperature; }
    bool isEnabled() { return enabled; }
    bool isReady();
    bool checkSafety();
    String getStatusJSON();
    
    // Configuration
    void enableRamping(bool enable, float rate = 1000.0);
    void setCoolantPin(uint8_t pin) { coolant_pin = pin; }
    void setAirPin(uint8_t pin) { air_pin = pin; }
    void setTemperaturePin(uint8_t pin) { temp_pin = pin; }
    
    // Calibration
    void calibrateTachometer(uint8_t pulses_per_revolution = 1);
    void setCurrentLimit(float max_amps);
    
    // Alarm system integration
    void setAlarmSystem(AlarmSystem* alarms);
    
private:
    // Internal control
    void applySpeed();
    void updateRamping();
    void updateTachometer();
    void updateTemperature();
    void checkSafetyLimits();
    
    // Control mode implementations
    void applyPWM(float duty_cycle);
    void applyAnalog(float voltage);
    void applyModbus(float rpm);
    void applyESC(float throttle);
    
    // Conversions
    float speedToPercent(float speed);
    float percentToSpeed(float percent);
    float rpmToPWM(float rpm);
    float pwmToRPM(float pwm);
    
    // Safety
    void checkCoolantFlow();
    void checkAirPressure();
    void checkTemperature();
    void checkInterlock();
    void raiseToolAlarm(const String& message);
};

// Plasma torch controller (specialized)
class PlasmaController {
private:
    uint8_t arc_start_pin;
    uint8_t arc_ok_pin;
    uint8_t torch_height_pin;
    uint8_t ohmic_sense_pin;
    
    PlasmaType plasma_type;
    float pierce_height;
    float cut_height;
    float current_height;
    uint32_t pierce_delay;
    
    bool arc_on;
    bool arc_ok;
    bool ohmic_contact;
    
    AlarmSystem* alarm_system;
    
    unsigned long arc_start_time;
    unsigned long last_height_update;
    
public:
    PlasmaController();
    
    void begin();
    void setPins(uint8_t arc_start, uint8_t arc_ok, uint8_t height, uint8_t ohmic = 255);
    void setPlasmaType(PlasmaType type);
    
    // Control
    void update();
    void startArc();
    void stopArc();
    void pierce();
    void setHeight(float height_mm);
    void touchOff();  // Ohmic sensing
    
    // Status
    bool isArcOn() { return arc_on; }
    bool isArcOK() { return arc_ok; }
    float getHeight() { return current_height; }
    bool hasOhmicContact() { return ohmic_contact; }
    String getStatusJSON();
    
    // Configuration
    void setPierceHeight(float height) { pierce_height = height; }
    void setCutHeight(float height) { cut_height = height; }
    void setPierceDelay(uint32_t delay_ms) { pierce_delay = delay_ms; }
    
    void setAlarmSystem(AlarmSystem* alarms);
    
private:
    void checkArcStatus();
    void updateHeight();
    void raiseArcAlarm(const String& message);
};

#endif // TOOL_CONTROLLER_H

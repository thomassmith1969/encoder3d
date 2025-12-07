/**
 * Tool Controller Implementation
 * 
 * Comprehensive support for CNC end effectors
 * 
 * Author: Thomas Smith
 * License: GPL-3.0
 */

#include "tool_controller.h"
#include "config.h"

// ============================================================================
// PREDEFINED TOOL PROFILES
// ============================================================================

// DC Motor Spindles - Cheap and DIY-friendly
const ToolSpec ToolProfiles::DC_775_12V = {
    .type = TOOL_SPINDLE_DC,
    .name = "775 DC Motor 12V",
    .max_speed = 10000.0,
    .min_speed = 1000.0,
    .idle_speed = 2000.0,
    .control_mode = CONTROL_PWM,
    .pwm_frequency = 25000,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 12.0,
    .max_current = 3.0,
    .rated_voltage = 12.0,
    .collet_size = 3.175,  // 1/8" common
    .spinup_time = 1000,
    .spindown_time = 2000,
    .min_on_time = 100,
    .cooldown_time = 0,
    .safety = {
        .requires_coolant = false,
        .requires_air_assist = false,
        .requires_torch_height = false,
        .requires_interlock = false,
        .has_tachometer = false,
        .has_temperature_sensor = false,
        .has_current_sensor = false,
        .requires_fume_extraction = false
    },
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 80.0,
    .max_duty_cycle = 80
};

const ToolSpec ToolProfiles::DC_775_24V = {
    .type = TOOL_SPINDLE_DC,
    .name = "775 DC Motor 24V",
    .max_speed = 15000.0,
    .min_speed = 1500.0,
    .idle_speed = 3000.0,
    .control_mode = CONTROL_PWM,
    .pwm_frequency = 25000,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 24.0,
    .max_current = 4.5,
    .rated_voltage = 24.0,
    .collet_size = 3.175,
    .spinup_time = 1000,
    .spindown_time = 2000,
    .min_on_time = 100,
    .cooldown_time = 0,
    .safety = {false, false, false, false, false, false, false, false},
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 85.0,
    .max_duty_cycle = 80
};

const ToolSpec ToolProfiles::DC_555_12V = {
    .type = TOOL_SPINDLE_DC,
    .name = "555 DC Motor 12V",
    .max_speed = 12000.0,
    .min_speed = 2000.0,
    .idle_speed = 3000.0,
    .control_mode = CONTROL_PWM,
    .pwm_frequency = 25000,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 12.0,
    .max_current = 2.0,
    .rated_voltage = 12.0,
    .collet_size = 2.35,  // Dremel-style
    .spinup_time = 800,
    .spindown_time = 1500,
    .min_on_time = 100,
    .cooldown_time = 0,
    .safety = {false, false, false, false, false, false, false, false},
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 75.0,
    .max_duty_cycle = 70
};

// Brushless Spindles - Better quality, ESC control
const ToolSpec ToolProfiles::BLDC_ER11_300W = {
    .type = TOOL_SPINDLE_BRUSHLESS,
    .name = "ER11 300W Brushless",
    .max_speed = 12000.0,
    .min_speed = 3000.0,
    .idle_speed = 5000.0,
    .control_mode = CONTROL_RC_ESC,
    .pwm_frequency = 50,  // 50Hz for RC ESC
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 24.0,
    .max_current = 12.5,  // 300W / 24V
    .rated_voltage = 24.0,
    .collet_size = 3.175,
    .spinup_time = 2000,
    .spindown_time = 3000,
    .min_on_time = 500,
    .cooldown_time = 5000,
    .safety = {
        .requires_coolant = false,
        .requires_air_assist = true,  // Recommended for chip clearing
        .requires_torch_height = false,
        .requires_interlock = false,
        .has_tachometer = true,
        .has_temperature_sensor = false,
        .has_current_sensor = false,
        .requires_fume_extraction = false
    },
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 60.0,
    .max_duty_cycle = 100
};

const ToolSpec ToolProfiles::BLDC_ER11_500W = {
    .type = TOOL_SPINDLE_BRUSHLESS,
    .name = "ER11 500W Brushless",
    .max_speed = 15000.0,
    .min_speed = 3000.0,
    .idle_speed = 5000.0,
    .control_mode = CONTROL_RC_ESC,
    .pwm_frequency = 50,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 48.0,
    .max_current = 10.4,  // 500W / 48V
    .rated_voltage = 48.0,
    .collet_size = 3.175,
    .spinup_time = 2000,
    .spindown_time = 3000,
    .min_on_time = 500,
    .cooldown_time = 5000,
    .safety = {false, true, false, false, true, false, false, false},
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 65.0,
    .max_duty_cycle = 100
};

const ToolSpec ToolProfiles::BLDC_ER20_1000W = {
    .type = TOOL_SPINDLE_BRUSHLESS,
    .name = "ER20 1000W Brushless",
    .max_speed = 18000.0,
    .min_speed = 5000.0,
    .idle_speed = 6000.0,
    .control_mode = CONTROL_RC_ESC,
    .pwm_frequency = 50,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 48.0,
    .max_current = 20.8,
    .rated_voltage = 48.0,
    .collet_size = 6.35,  // 1/4"
    .spinup_time = 3000,
    .spindown_time = 4000,
    .min_on_time = 1000,
    .cooldown_time = 10000,
    .safety = {false, true, false, false, true, true, false, false},
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 70.0,
    .max_duty_cycle = 100
};

// VFD Water-Cooled Spindles - Professional quality
const ToolSpec ToolProfiles::VFD_1_5KW_WATER = {
    .type = TOOL_SPINDLE_VFD,
    .name = "1.5kW Water-Cooled VFD",
    .max_speed = 24000.0,
    .min_speed = 6000.0,
    .idle_speed = 8000.0,
    .control_mode = CONTROL_ANALOG,  // 0-10V to VFD
    .pwm_frequency = 1000,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 10.0,
    .max_current = 7.5,  // 1500W / 220V AC
    .rated_voltage = 220.0,
    .collet_size = 6.35,
    .spinup_time = 3000,
    .spindown_time = 5000,
    .min_on_time = 2000,
    .cooldown_time = 30000,  // 30 seconds cooldown
    .safety = {
        .requires_coolant = true,   // Water cooling required!
        .requires_air_assist = true,
        .requires_torch_height = false,
        .requires_interlock = true,
        .has_tachometer = true,
        .has_temperature_sensor = true,
        .has_current_sensor = true,
        .requires_fume_extraction = true
    },
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 50.0,  // Water-cooled, should stay cool
    .max_duty_cycle = 100
};

const ToolSpec ToolProfiles::VFD_2_2KW_WATER = {
    .type = TOOL_SPINDLE_VFD,
    .name = "2.2kW Water-Cooled VFD",
    .max_speed = 24000.0,
    .min_speed = 6000.0,
    .idle_speed = 8000.0,
    .control_mode = CONTROL_ANALOG,
    .pwm_frequency = 1000,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 10.0,
    .max_current = 10.0,
    .rated_voltage = 220.0,
    .collet_size = 6.35,
    .spinup_time = 3000,
    .spindown_time = 5000,
    .min_on_time = 2000,
    .cooldown_time = 30000,
    .safety = {true, true, false, true, true, true, true, true},
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 50.0,
    .max_duty_cycle = 100
};

const ToolSpec ToolProfiles::VFD_3_0KW_WATER = {
    .type = TOOL_SPINDLE_VFD,
    .name = "3.0kW Water-Cooled VFD",
    .max_speed = 24000.0,
    .min_speed = 8000.0,
    .idle_speed = 10000.0,
    .control_mode = CONTROL_ANALOG,
    .pwm_frequency = 1000,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 10.0,
    .max_current = 13.6,
    .rated_voltage = 220.0,
    .collet_size = 6.35,
    .spinup_time = 4000,
    .spindown_time = 6000,
    .min_on_time = 2000,
    .cooldown_time = 60000,  // 1 minute cooldown
    .safety = {true, true, false, true, true, true, true, true},
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 55.0,
    .max_duty_cycle = 100
};

// DIY Router Conversions - Popular for hobbyist CNCs
const ToolSpec ToolProfiles::MAKITA_RT0700 = {
    .type = TOOL_SPINDLE_DC,
    .name = "Makita RT0700 Router",
    .max_speed = 30000.0,
    .min_speed = 10000.0,
    .idle_speed = 12000.0,
    .control_mode = CONTROL_RELAY,  // Usually just on/off
    .pwm_frequency = 0,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 230.0,
    .max_current = 3.0,
    .rated_voltage = 230.0,
    .collet_size = 6.35,
    .spinup_time = 2000,
    .spindown_time = 4000,
    .min_on_time = 1000,
    .cooldown_time = 0,
    .safety = {
        .requires_coolant = false,
        .requires_air_assist = true,
        .requires_torch_height = false,
        .requires_interlock = false,
        .has_tachometer = false,
        .has_temperature_sensor = false,
        .has_current_sensor = false,
        .requires_fume_extraction = true
    },
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 80.0,
    .max_duty_cycle = 100
};

const ToolSpec ToolProfiles::DEWALT_611 = {
    .type = TOOL_SPINDLE_DC,
    .name = "DeWalt 611 Router",
    .max_speed = 27000.0,
    .min_speed = 16000.0,
    .idle_speed = 18000.0,
    .control_mode = CONTROL_RELAY,
    .pwm_frequency = 0,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 120.0,
    .max_current = 7.0,
    .rated_voltage = 120.0,
    .collet_size = 6.35,
    .spinup_time = 2000,
    .spindown_time = 4000,
    .min_on_time = 1000,
    .cooldown_time = 0,
    .safety = {false, true, false, false, false, false, false, true},
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 85.0,
    .max_duty_cycle = 100
};

// Plasma Cutters
const ToolSpec ToolProfiles::PLASMA_CUT50_PILOT = {
    .type = TOOL_PLASMA_TORCH,
    .name = "50A Pilot Arc Plasma",
    .max_speed = 100.0,  // Using as power percentage
    .min_speed = 30.0,
    .idle_speed = 0.0,
    .control_mode = CONTROL_TTL,
    .pwm_frequency = 0,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 5.0,
    .max_current = 50.0,
    .rated_voltage = 220.0,
    .collet_size = 0.0,  // N/A
    .spinup_time = 500,
    .spindown_time = 500,
    .min_on_time = 100,
    .cooldown_time = 5000,
    .safety = {
        .requires_coolant = false,
        .requires_air_assist = true,  // Compressed air required
        .requires_torch_height = true,
        .requires_interlock = true,
        .has_tachometer = false,
        .has_temperature_sensor = false,
        .has_current_sensor = true,
        .requires_fume_extraction = true
    },
    .pierce_height = 3.8,  // 3.8mm pierce height
    .cut_height = 1.5,     // 1.5mm cut height
    .pierce_delay = 500,   // 500ms pierce delay
    .max_temperature = 100.0,
    .max_duty_cycle = 60  // 60% duty cycle typical
};

const ToolSpec ToolProfiles::PLASMA_CUT60_PILOT = {
    .type = TOOL_PLASMA_TORCH,
    .name = "60A Pilot Arc Plasma",
    .max_speed = 100.0,
    .min_speed = 40.0,
    .idle_speed = 0.0,
    .control_mode = CONTROL_TTL,
    .pwm_frequency = 0,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 5.0,
    .max_current = 60.0,
    .rated_voltage = 220.0,
    .collet_size = 0.0,
    .spinup_time = 500,
    .spindown_time = 500,
    .min_on_time = 100,
    .cooldown_time = 5000,
    .safety = {false, true, true, true, false, false, true, true},
    .pierce_height = 4.0,
    .cut_height = 1.5,
    .pierce_delay = 600,
    .max_temperature = 100.0,
    .max_duty_cycle = 60
};

const ToolSpec ToolProfiles::PLASMA_HYPERTHERM_45 = {
    .type = TOOL_PLASMA_TORCH,
    .name = "Hypertherm Powermax 45",
    .max_speed = 100.0,
    .min_speed = 30.0,
    .idle_speed = 0.0,
    .control_mode = CONTROL_TTL,
    .pwm_frequency = 0,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 5.0,
    .max_current = 45.0,
    .rated_voltage = 220.0,
    .collet_size = 0.0,
    .spinup_time = 300,
    .spindown_time = 300,
    .min_on_time = 100,
    .cooldown_time = 3000,
    .safety = {false, true, true, true, false, false, true, true},
    .pierce_height = 3.8,
    .cut_height = 1.5,
    .pierce_delay = 400,
    .max_temperature = 100.0,
    .max_duty_cycle = 100  // Hypertherm has better duty cycle
};

// Other Tools
const ToolSpec ToolProfiles::DRAG_KNIFE_STANDARD = {
    .type = TOOL_DRAG_KNIFE,
    .name = "Drag Knife",
    .max_speed = 100.0,  // Pressure percentage
    .min_speed = 0.0,
    .idle_speed = 0.0,
    .control_mode = CONTROL_PWM,
    .pwm_frequency = 1000,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 5.0,
    .max_current = 1.0,
    .rated_voltage = 5.0,
    .collet_size = 0.0,
    .spinup_time = 0,
    .spindown_time = 0,
    .min_on_time = 0,
    .cooldown_time = 0,
    .safety = {false, false, false, false, false, false, false, false},
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 50.0,
    .max_duty_cycle = 100
};

const ToolSpec ToolProfiles::PEN_PLOTTER_STANDARD = {
    .type = TOOL_PEN_PLOTTER,
    .name = "Pen Plotter",
    .max_speed = 100.0,
    .min_speed = 0.0,
    .idle_speed = 0.0,
    .control_mode = CONTROL_PWM,
    .pwm_frequency = 50,  // Servo control
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 5.0,
    .max_current = 0.5,
    .rated_voltage = 5.0,
    .collet_size = 0.0,
    .spinup_time = 0,
    .spindown_time = 0,
    .min_on_time = 0,
    .cooldown_time = 0,
    .safety = {false, false, false, false, false, false, false, false},
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 40.0,
    .max_duty_cycle = 100
};

const ToolSpec ToolProfiles::HOT_WIRE_STANDARD = {
    .type = TOOL_HOT_WIRE,
    .name = "Hot Wire Foam Cutter",
    .max_speed = 100.0,  // Temperature/power percentage
    .min_speed = 20.0,
    .idle_speed = 30.0,
    .control_mode = CONTROL_PWM,
    .pwm_frequency = 100,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 24.0,
    .max_current = 5.0,
    .rated_voltage = 24.0,
    .collet_size = 0.0,
    .spinup_time = 5000,  // Heating time
    .spindown_time = 10000,
    .min_on_time = 1000,
    .cooldown_time = 30000,
    .safety = {
        .requires_coolant = false,
        .requires_air_assist = false,
        .requires_torch_height = false,
        .requires_interlock = false,
        .has_tachometer = false,
        .has_temperature_sensor = true,
        .has_current_sensor = true,
        .requires_fume_extraction = true  // Foam fumes!
    },
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 400.0,  // Wire temperature
    .max_duty_cycle = 80
};

const ToolSpec ToolProfiles::VACUUM_PICKUP_STANDARD = {
    .type = TOOL_VACUUM_PICKUP,
    .name = "Vacuum Pick and Place",
    .max_speed = 100.0,  // Vacuum strength
    .min_speed = 0.0,
    .idle_speed = 0.0,
    .control_mode = CONTROL_TTL,
    .pwm_frequency = 0,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 24.0,
    .max_current = 2.0,
    .rated_voltage = 24.0,
    .collet_size = 0.0,
    .spinup_time = 500,
    .spindown_time = 500,
    .min_on_time = 100,
    .cooldown_time = 0,
    .safety = {false, true, false, false, false, false, false, false},
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 50.0,
    .max_duty_cycle = 100
};

// ============================================================================
// TOOL CONTROLLER IMPLEMENTATION
// ============================================================================

ToolController::ToolController() 
    : pwm_pin(255), dir_pin(255), enable_pin(255), analog_pin(255),
      tach_pin(255), temp_pin(255), coolant_pin(255), air_pin(255),
      pwm_channel(0), enabled(false), direction_cw(true),
      current_speed(0), target_speed(0), measured_rpm(0), temperature(0),
      last_update(0), enable_time(0), last_tach_time(0), tach_pulse_count(0),
      alarm_system(nullptr), coolant_flow_ok(true), air_pressure_ok(true),
      temperature_ok(true), interlock_ok(true), ramping_enabled(true), ramp_rate(1000.0) {
    
    // Initialize with a safe default spec
    spec = ToolProfiles::DC_775_12V;
}

void ToolController::begin() {
    // Initialize PWM pin
    if (pwm_pin != 255) {
        pinMode(pwm_pin, OUTPUT);
        ledcSetup(pwm_channel, spec.pwm_frequency, 8);  // 8-bit resolution
        ledcAttachPin(pwm_pin, pwm_channel);
        ledcWrite(pwm_channel, 0);
    }
    
    // Initialize direction pin
    if (dir_pin != 255) {
        pinMode(dir_pin, OUTPUT);
        digitalWrite(dir_pin, direction_cw ? HIGH : LOW);
    }
    
    // Initialize enable pin
    if (enable_pin != 255) {
        pinMode(enable_pin, OUTPUT);
        digitalWrite(enable_pin, LOW);
    }
    
    // Initialize analog output (DAC)
    if (analog_pin != 255 && spec.control_mode == CONTROL_ANALOG) {
        pinMode(analog_pin, OUTPUT);
        dacWrite(analog_pin, 0);
    }
    
    // Initialize tachometer input
    if (tach_pin != 255 && spec.safety.has_tachometer) {
        pinMode(tach_pin, INPUT_PULLUP);
    }
    
    // Initialize temperature sensor
    if (temp_pin != 255 && spec.safety.has_temperature_sensor) {
        pinMode(temp_pin, INPUT);
    }
    
    // Initialize coolant control/sensing
    if (coolant_pin != 255) {
        pinMode(coolant_pin, spec.safety.requires_coolant ? INPUT_PULLUP : OUTPUT);
        if (!spec.safety.requires_coolant) {
            digitalWrite(coolant_pin, LOW);
        }
    }
    
    // Initialize air assist
    if (air_pin != 255) {
        pinMode(air_pin, spec.safety.requires_air_assist ? INPUT_PULLUP : OUTPUT);
        if (!spec.safety.requires_air_assist) {
            digitalWrite(air_pin, LOW);
        }
    }
    
    Serial.print("Tool controller initialized: ");
    Serial.println(spec.name);
}

void ToolController::setPins(uint8_t pwm, uint8_t dir, uint8_t enable, 
                              uint8_t analog, uint8_t tach) {
    pwm_pin = pwm;
    dir_pin = dir;
    enable_pin = enable;
    analog_pin = analog;
    tach_pin = tach;
}

void ToolController::setToolSpec(const ToolSpec& tool_spec) {
    spec = tool_spec;
    
    // Reconfigure PWM frequency if needed
    if (pwm_pin != 255 && spec.pwm_frequency > 0) {
        ledcSetup(pwm_channel, spec.pwm_frequency, 8);
        ledcAttachPin(pwm_pin, pwm_channel);
    }
}

void ToolController::loadProfile(const String& profile_name) {
    if (profile_name == "DC_775_12V") spec = ToolProfiles::DC_775_12V;
    else if (profile_name == "DC_775_24V") spec = ToolProfiles::DC_775_24V;
    else if (profile_name == "DC_555_12V") spec = ToolProfiles::DC_555_12V;
    else if (profile_name == "BLDC_ER11_300W") spec = ToolProfiles::BLDC_ER11_300W;
    else if (profile_name == "BLDC_ER11_500W") spec = ToolProfiles::BLDC_ER11_500W;
    else if (profile_name == "BLDC_ER20_1000W") spec = ToolProfiles::BLDC_ER20_1000W;
    else if (profile_name == "VFD_1_5KW_WATER") spec = ToolProfiles::VFD_1_5KW_WATER;
    else if (profile_name == "VFD_2_2KW_WATER") spec = ToolProfiles::VFD_2_2KW_WATER;
    else if (profile_name == "VFD_3_0KW_WATER") spec = ToolProfiles::VFD_3_0KW_WATER;
    else if (profile_name == "MAKITA_RT0700") spec = ToolProfiles::MAKITA_RT0700;
    else if (profile_name == "DEWALT_611") spec = ToolProfiles::DEWALT_611;
    else if (profile_name == "PLASMA_CUT50_PILOT") spec = ToolProfiles::PLASMA_CUT50_PILOT;
    else if (profile_name == "PLASMA_CUT60_PILOT") spec = ToolProfiles::PLASMA_CUT60_PILOT;
    else if (profile_name == "PLASMA_HYPERTHERM_45") spec = ToolProfiles::PLASMA_HYPERTHERM_45;
    else if (profile_name == "DRAG_KNIFE_STANDARD") spec = ToolProfiles::DRAG_KNIFE_STANDARD;
    else if (profile_name == "PEN_PLOTTER_STANDARD") spec = ToolProfiles::PEN_PLOTTER_STANDARD;
    else if (profile_name == "HOT_WIRE_STANDARD") spec = ToolProfiles::HOT_WIRE_STANDARD;
    else if (profile_name == "VACUUM_PICKUP_STANDARD") spec = ToolProfiles::VACUUM_PICKUP_STANDARD;
    
    // Reconfigure for new profile
    if (pwm_pin != 255 && spec.pwm_frequency > 0) {
        ledcSetup(pwm_channel, spec.pwm_frequency, 8);
    }
}

void ToolController::update() {
    unsigned long now = millis();
    
    if (now - last_update < 100) return;  // Update at 10Hz
    last_update = now;
    
    if (enabled) {
        updateRamping();
        applySpeed();
        updateTachometer();
        updateTemperature();
        checkSafetyLimits();
    }
}

void ToolController::setSpeed(float speed) {
    target_speed = constrain(speed, spec.min_speed, spec.max_speed);
    
    if (!ramping_enabled) {
        current_speed = target_speed;
    }
}

void ToolController::setSpeedRPM(float rpm) {
    setSpeed(rpm);
}

void ToolController::setSpeedPercent(float percent) {
    float speed = spec.min_speed + (percent / 100.0) * (spec.max_speed - spec.min_speed);
    setSpeed(speed);
}

void ToolController::setDirection(bool clockwise) {
    direction_cw = clockwise;
    if (dir_pin != 255) {
        digitalWrite(dir_pin, clockwise ? HIGH : LOW);
    }
}

void ToolController::enable() {
    // Safety checks before enabling
    if (!checkSafety()) {
        raiseToolAlarm("Safety check failed - cannot enable tool");
        return;
    }
    
    enabled = true;
    enable_time = millis();
    
    if (enable_pin != 255) {
        digitalWrite(enable_pin, HIGH);
    }
    
    // Start coolant/air if needed
    if (coolant_pin != 255 && !spec.safety.requires_coolant) {
        digitalWrite(coolant_pin, HIGH);
    }
    if (air_pin != 255 && !spec.safety.requires_air_assist) {
        digitalWrite(air_pin, HIGH);
    }
}

void ToolController::disable() {
    enabled = false;
    current_speed = 0;
    target_speed = 0;
    
    if (enable_pin != 255) {
        digitalWrite(enable_pin, LOW);
    }
    
    applySpeed();  // Stop immediately
    
    // Cooldown period for tools that need it
    if (spec.cooldown_time > 0) {
        delay(spec.cooldown_time);
    }
    
    // Turn off coolant/air after cooldown
    if (coolant_pin != 255 && !spec.safety.requires_coolant) {
        digitalWrite(coolant_pin, LOW);
    }
    if (air_pin != 255 && !spec.safety.requires_air_assist) {
        digitalWrite(air_pin, LOW);
    }
}

void ToolController::emergencyStop() {
    enabled = false;
    current_speed = 0;
    target_speed = 0;
    
    if (enable_pin != 255) {
        digitalWrite(enable_pin, LOW);
    }
    
    if (pwm_pin != 255) {
        ledcWrite(pwm_channel, 0);
    }
    
    if (analog_pin != 255) {
        dacWrite(analog_pin, 0);
    }
    
    raiseToolAlarm("EMERGENCY STOP");
}

void ToolController::setTorchHeight(float height_mm) {
    // For plasma torches - control Z-axis height
    // This would integrate with motion controller
    // Placeholder for now
}

void ToolController::pierce() {
    if (spec.type != TOOL_PLASMA_TORCH) return;
    
    // Move to pierce height
    setTorchHeight(spec.pierce_height);
    delay(100);
    
    // Start arc
    enable();
    
    // Wait for pierce delay
    delay(spec.pierce_delay);
    
    // Move to cut height
    setTorchHeight(spec.cut_height);
}

void ToolController::arcOn() {
    if (spec.type == TOOL_PLASMA_TORCH) {
        enable();
    }
}

void ToolController::arcOff() {
    if (spec.type == TOOL_PLASMA_TORCH) {
        disable();
    }
}

bool ToolController::isReady() {
    if (!enabled) return false;
    if (spec.spinup_time > 0 && (millis() - enable_time) < spec.spinup_time) return false;
    if (!checkSafety()) return false;
    return true;
}

bool ToolController::checkSafety() {
    bool safe = true;
    
    // Check coolant flow
    if (spec.safety.requires_coolant) {
        checkCoolantFlow();
        safe &= coolant_flow_ok;
    }
    
    // Check air pressure
    if (spec.safety.requires_air_assist) {
        checkAirPressure();
        safe &= air_pressure_ok;
    }
    
    // Check temperature
    if (spec.safety.has_temperature_sensor) {
        checkTemperature();
        safe &= temperature_ok;
    }
    
    // Check interlock
    if (spec.safety.requires_interlock) {
        checkInterlock();
        safe &= interlock_ok;
    }
    
    return safe;
}

String ToolController::getStatusJSON() {
    String json = "{";
    json += "\"tool\":\"" + spec.name + "\",";
    json += "\"type\":" + String(spec.type) + ",";
    json += "\"enabled\":" + String(enabled ? "true" : "false") + ",";
    json += "\"speed\":" + String(current_speed, 1) + ",";
    json += "\"target\":" + String(target_speed, 1) + ",";
    json += "\"rpm\":" + String(measured_rpm, 0) + ",";
    json += "\"temp\":" + String(temperature, 1) + ",";
    json += "\"ready\":" + String(isReady() ? "true" : "false") + ",";
    json += "\"coolant\":" + String(coolant_flow_ok ? "true" : "false") + ",";
    json += "\"air\":" + String(air_pressure_ok ? "true" : "false");
    json += "}";
    return json;
}

void ToolController::enableRamping(bool enable, float rate) {
    ramping_enabled = enable;
    ramp_rate = rate;
}

void ToolController::setAlarmSystem(AlarmSystem* alarms) {
    alarm_system = alarms;
}

// Private methods

void ToolController::applySpeed() {
    float speed_percent = speedToPercent(current_speed);
    
    switch (spec.control_mode) {
        case CONTROL_PWM:
            applyPWM(speed_percent / 100.0);
            break;
            
        case CONTROL_ANALOG:
            applyAnalog(spec.analog_min_voltage + 
                       (speed_percent / 100.0) * (spec.analog_max_voltage - spec.analog_min_voltage));
            break;
            
        case CONTROL_TTL:
            if (pwm_pin != 255) {
                digitalWrite(pwm_pin, current_speed > 0 ? HIGH : LOW);
            }
            break;
            
        case CONTROL_RC_ESC:
            applyESC(speed_percent / 100.0);
            break;
            
        case CONTROL_RELAY:
            if (enable_pin != 255) {
                digitalWrite(enable_pin, enabled ? HIGH : LOW);
            }
            break;
            
        default:
            break;
    }
}

void ToolController::updateRamping() {
    if (!ramping_enabled || current_speed == target_speed) return;
    
    float delta = target_speed - current_speed;
    float max_change = (ramp_rate * 0.1);  // 100ms update rate
    
    if (abs(delta) <= max_change) {
        current_speed = target_speed;
    } else {
        current_speed += (delta > 0) ? max_change : -max_change;
    }
}

void ToolController::updateTachometer() {
    if (tach_pin == 255 || !spec.safety.has_tachometer) return;
    
    // Simple frequency measurement
    unsigned long now = micros();
    if (digitalRead(tach_pin) == LOW) {
        if (tach_pulse_count == 0) {
            last_tach_time = now;
        }
        tach_pulse_count++;
        
        if (tach_pulse_count >= 10) {  // Measure over 10 pulses
            unsigned long period = (now - last_tach_time) / tach_pulse_count;
            measured_rpm = 60000000.0 / period;  // Convert to RPM
            tach_pulse_count = 0;
        }
    }
}

void ToolController::updateTemperature() {
    if (temp_pin == 255 || !spec.safety.has_temperature_sensor) return;
    
    // Read temperature sensor (assuming thermistor or similar)
    int raw = analogRead(temp_pin);
    temperature = raw * 0.1;  // Simple conversion - would need calibration
}

void ToolController::checkSafetyLimits() {
    // Check temperature
    if (spec.safety.has_temperature_sensor && temperature > spec.max_temperature) {
        emergencyStop();
        raiseToolAlarm("Temperature exceeded: " + String(temperature, 1) + "°C");
    }
    
    // Check duty cycle
    if (enabled && spec.max_duty_cycle < 100) {
        unsigned long on_time = millis() - enable_time;
        if (on_time > (spec.max_duty_cycle * 600)) {  // Rough duty cycle check
            disable();
            raiseToolAlarm("Duty cycle limit reached");
        }
    }
}

void ToolController::applyPWM(float duty_cycle) {
    if (pwm_pin == 255) return;
    uint8_t pwm = constrain(duty_cycle * 255, 0, 255);
    ledcWrite(pwm_channel, pwm);
}

void ToolController::applyAnalog(float voltage) {
    if (analog_pin == 255) return;
    uint8_t dac = constrain((voltage / 3.3) * 255, 0, 255);  // ESP32 DAC is 0-3.3V
    dacWrite(analog_pin, dac);
}

void ToolController::applyModbus(float rpm) {
    // Would implement Modbus RTU communication here
    // Placeholder for VFD control
}

void ToolController::applyESC(float throttle) {
    if (pwm_pin == 255) return;
    
    // RC ESC expects 1000-2000μs pulse at 50Hz
    // 1000μs = off, 2000μs = full
    uint16_t pulse_us = 1000 + (throttle * 1000);
    
    // Using ledcWrite with 50Hz frequency
    uint8_t duty = (pulse_us / 20000.0) * 255;  // 20000μs = 50Hz period
    ledcWrite(pwm_channel, duty);
}

float ToolController::speedToPercent(float speed) {
    if (spec.max_speed == spec.min_speed) return 0;
    return ((speed - spec.min_speed) / (spec.max_speed - spec.min_speed)) * 100.0;
}

float ToolController::percentToSpeed(float percent) {
    return spec.min_speed + (percent / 100.0) * (spec.max_speed - spec.min_speed);
}

float ToolController::rpmToPWM(float rpm) {
    return speedToPercent(rpm) / 100.0;
}

float ToolController::pwmToRPM(float pwm) {
    return percentToSpeed(pwm * 100.0);
}

void ToolController::checkCoolantFlow() {
    if (coolant_pin == 255) {
        coolant_flow_ok = true;
        return;
    }
    coolant_flow_ok = (digitalRead(coolant_pin) == HIGH);
    if (!coolant_flow_ok && enabled) {
        raiseToolAlarm("Coolant flow failure");
    }
}

void ToolController::checkAirPressure() {
    if (air_pin == 255) {
        air_pressure_ok = true;
        return;
    }
    air_pressure_ok = (digitalRead(air_pin) == HIGH);
    if (!air_pressure_ok && enabled) {
        raiseToolAlarm("Air pressure failure");
    }
}

void ToolController::checkTemperature() {
    updateTemperature();
    temperature_ok = (temperature < spec.max_temperature);
}

void ToolController::checkInterlock() {
    // Would check interlock pin here
    interlock_ok = true;  // Placeholder
}

void ToolController::raiseToolAlarm(const String& message) {
    if (alarm_system) {
        alarm_system->raiseAlarm(ALARM_TOOL_FAULT, message, SEVERITY_HIGH);
    }
    Serial.print("TOOL ALARM: ");
    Serial.println(message);
}

// ============================================================================
// PLASMA CONTROLLER IMPLEMENTATION
// ============================================================================

PlasmaController::PlasmaController()
    : arc_start_pin(255), arc_ok_pin(255), torch_height_pin(255), ohmic_sense_pin(255),
      plasma_type(PLASMA_PILOT_ARC), pierce_height(3.8), cut_height(1.5),
      current_height(0), pierce_delay(500), arc_on(false), arc_ok(false),
      ohmic_contact(false), alarm_system(nullptr), arc_start_time(0), last_height_update(0) {
}

void PlasmaController::begin() {
    if (arc_start_pin != 255) {
        pinMode(arc_start_pin, OUTPUT);
        digitalWrite(arc_start_pin, LOW);
    }
    
    if (arc_ok_pin != 255) {
        pinMode(arc_ok_pin, INPUT_PULLUP);
    }
    
    if (torch_height_pin != 255) {
        pinMode(torch_height_pin, OUTPUT);
    }
    
    if (ohmic_sense_pin != 255) {
        pinMode(ohmic_sense_pin, INPUT_PULLUP);
    }
    
    Serial.println("Plasma controller initialized");
}

void PlasmaController::setPins(uint8_t arc_start, uint8_t arc_ok, uint8_t height, uint8_t ohmic) {
    arc_start_pin = arc_start;
    arc_ok_pin = arc_ok;
    torch_height_pin = height;
    ohmic_sense_pin = ohmic;
}

void PlasmaController::setPlasmaType(PlasmaType type) {
    plasma_type = type;
}

void PlasmaController::update() {
    unsigned long now = millis();
    
    if (now - last_height_update > 10) {  // Update at 100Hz
        updateHeight();
        checkArcStatus();
        last_height_update = now;
    }
}

void PlasmaController::startArc() {
    if (arc_start_pin == 255) return;
    
    digitalWrite(arc_start_pin, HIGH);
    arc_on = true;
    arc_start_time = millis();
    
    // Wait for arc OK signal
    delay(100);
    checkArcStatus();
    
    if (!arc_ok) {
        raiseArcAlarm("Arc failed to start");
        stopArc();
    }
}

void PlasmaController::stopArc() {
    if (arc_start_pin == 255) return;
    
    digitalWrite(arc_start_pin, LOW);
    arc_on = false;
    arc_ok = false;
}

void PlasmaController::pierce() {
    // Move to pierce height
    setHeight(pierce_height);
    delay(100);
    
    // Start arc
    startArc();
    
    // Wait for pierce
    delay(pierce_delay);
    
    // Move to cut height
    setHeight(cut_height);
}

void PlasmaController::setHeight(float height_mm) {
    current_height = height_mm;
    // Would control Z-axis or torch height controller here
}

void PlasmaController::touchOff() {
    if (ohmic_sense_pin == 255) return;
    
    // Slowly lower torch until ohmic contact
    while (!ohmic_contact) {
        ohmic_contact = (digitalRead(ohmic_sense_pin) == LOW);
        if (ohmic_contact) break;
        
        current_height -= 0.1;
        setHeight(current_height);
        delay(10);
        
        if (current_height < -5.0) {
            raiseArcAlarm("Ohmic touch-off failed - no contact");
            break;
        }
    }
    
    // Set zero at contact point
    current_height = 0;
}

String PlasmaController::getStatusJSON() {
    String json = "{";
    json += "\"arc_on\":" + String(arc_on ? "true" : "false") + ",";
    json += "\"arc_ok\":" + String(arc_ok ? "true" : "false") + ",";
    json += "\"height\":" + String(current_height, 2) + ",";
    json += "\"ohmic\":" + String(ohmic_contact ? "true" : "false");
    json += "}";
    return json;
}

void PlasmaController::setAlarmSystem(AlarmSystem* alarms) {
    alarm_system = alarms;
}

void PlasmaController::checkArcStatus() {
    if (arc_ok_pin == 255) {
        arc_ok = arc_on;  // Assume OK if no feedback
        return;
    }
    
    arc_ok = (digitalRead(arc_ok_pin) == HIGH);
    
    // If arc should be on but isn't OK
    if (arc_on && !arc_ok && (millis() - arc_start_time) > 1000) {
        raiseArcAlarm("Arc lost during cut");
        stopArc();
    }
}

void PlasmaController::updateHeight() {
    // Would implement voltage-based height control here
    // Read arc voltage and adjust height to maintain constant voltage
}

void PlasmaController::raiseArcAlarm(const String& message) {
    if (alarm_system) {
        alarm_system->raiseAlarm(ALARM_TOOL_FAULT, message, SEVERITY_HIGH);
    }
    Serial.print("PLASMA ALARM: ");
    Serial.println(message);
}

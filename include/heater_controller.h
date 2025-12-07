/*
 * Encoder3D CNC Controller - Heater Controller
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 * 
 * Temperature control with PID and thermal runaway protection
 */

#ifndef HEATER_CONTROLLER_H
#define HEATER_CONTROLLER_H

#include <Arduino.h>
#include "config.h"

// Thermistor lookup table and calculations
class Thermistor {
private:
    uint8_t pin;
    float beta;
    float r0;
    float t0;
    
public:
    Thermistor(uint8_t analog_pin);
    float readTemperature();
    
private:
    float analogToResistance(int analog_value);
    float resistanceToTemperature(float resistance);
};

// PID-controlled heater
class Heater {
private:
    uint8_t id;
    HeaterPins pins;
    Thermistor* thermistor;
    
    float current_temp;
    float target_temp;
    float max_temp;
    
    // PID variables
    float kp, ki, kd;
    float integral;
    float prev_error;
    unsigned long last_time;
    
    // Safety
    bool enabled;
    unsigned long safety_timer;
    float last_temp;
    bool thermal_runaway_detected;
    
    uint8_t pwm_channel;
    
public:
    Heater(uint8_t heater_id, const HeaterPins& heater_pins, float max_temperature, float p, float i, float d);
    ~Heater();
    
    void begin();
    void update();  // Call periodically
    
    void setTargetTemperature(float temp);
    float getTargetTemperature();
    float getCurrentTemperature();
    
    void enable();
    void disable();
    bool isEnabled();
    
    bool isAtTarget(float tolerance = 2.0);
    bool isThermalRunaway();
    
    void setPID(float p, float i, float d);
    void emergencyShutdown();
    
private:
    float computePID();
    void checkThermalRunaway();
    void applyPower(float power);
};

// Heater Controller manages all heaters
class HeaterController {
private:
    Heater* heaters[NUM_HEATERS];
    TaskHandle_t control_task_handle;
    bool is_running;
    
public:
    HeaterController();
    ~HeaterController();
    
    void begin();
    void startControlLoop();
    void stopControlLoop();
    
    // Control functions
    void setTemperature(uint8_t heater_id, float temp);
    float getTemperature(uint8_t heater_id);
    float getTargetTemperature(uint8_t heater_id);
    bool isAtTarget(uint8_t heater_id, float tolerance = 2.0);
    
    void enableHeater(uint8_t heater_id);
    void disableHeater(uint8_t heater_id);
    void emergencyShutdownAll();
    
    bool anyThermalRunaway();
    
    Heater* getHeater(uint8_t heater_id);
    
private:
    static void controlLoopTask(void* parameter);
    void controlLoop();
};

#endif // HEATER_CONTROLLER_H

/*
 * Encoder3D CNC Controller - Motor Controller
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 * 
 * Motor control with PID feedback and encoder support
 */

#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>
#include <Encoder.h>
#include "config.h"

// PID Controller class
class PIDController {
private:
    float kp, ki, kd;
    float integral;
    float prev_error;
    float output_limit;
    unsigned long last_time;
    
public:
    PIDController(float p, float i, float d, float limit);
    void reset();
    float compute(float setpoint, float measured);
    void setTunings(float p, float i, float d);
};

// Motor class with encoder feedback
class Motor {
private:
    uint8_t id;
    MotorPins pins;
    Encoder* encoder;
    PIDController* pid;
    
    float current_position;  // in mm
    float target_position;   // in mm
    float current_velocity;  // in mm/s
    float target_velocity;   // in mm/s
    
    int32_t encoder_count;
    int32_t last_encoder_count;
    unsigned long last_update_time;
    
    float steps_per_mm;
    float max_speed;
    float max_accel;
    
    bool enabled;
    int direction;  // 1 or -1
    
public:
    Motor(uint8_t motor_id, const MotorPins& motor_pins, float steps_mm, float max_spd, float max_acc);
    ~Motor();
    
    void begin();
    void enable();
    void disable();
    bool isEnabled();
    
    void update();  // Called from control loop
    void setTargetPosition(float pos_mm);
    void setTargetVelocity(float vel_mm_s);
    
    float getCurrentPosition();
    float getTargetPosition();
    float getCurrentVelocity();
    int32_t getEncoderCount();
    
    void resetPosition(float pos = 0.0);
    void emergencyStop();
    
private:
    void updateEncoder();
    void applyMotorControl(int pwm_value);
    float encoderCountsToMM(int32_t counts);
    int32_t mmToEncoderCounts(float mm);
};

// Motor Controller manages all motors
class MotorController {
private:
    Motor* motors[NUM_MOTORS];
    TaskHandle_t control_task_handle;
    bool is_running;
    
    // Motion planning
    struct MotionState {
        float position[NUM_MOTORS];
        float velocity[NUM_MOTORS];
        float acceleration[NUM_MOTORS];
    } current_state, target_state;
    
public:
    MotorController();
    ~MotorController();
    
    void begin();
    void startControlLoop();
    void stopControlLoop();
    
    // Motion control
    void moveAbsolute(uint8_t motor_id, float position);
    void moveRelative(uint8_t motor_id, float distance);
    void setVelocity(uint8_t motor_id, float velocity);
    void home(uint8_t motor_id);
    void homeAll();
    
    // Coordinated motion (for G-code)
    void linearMove(float x1, float x2, float y1, float y2, float z, float e, float feedrate);
    void rapidMove(float x1, float x2, float y1, float y2, float z, float e);
    
    // Status
    float getPosition(uint8_t motor_id);
    float getVelocity(uint8_t motor_id);
    bool isMoving();
    void waitForMotion();
    
    // Emergency
    void emergencyStop();
    void reset();
    
    // Enable/disable
    void enableMotor(uint8_t motor_id);
    void disableMotor(uint8_t motor_id);
    void enableAll();
    void disableAll();
    
    Motor* getMotor(uint8_t motor_id);
    
private:
    static void controlLoopTask(void* parameter);
    void controlLoop();
    void planMotion();
};

#endif // MOTOR_CONTROLLER_H

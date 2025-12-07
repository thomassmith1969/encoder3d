/*
 * Encoder3D CNC Controller - Motor Controller Implementation
 * 
 * Copyright (c) 2025 Encoder3D Project Contributors
 * Licensed under the MIT License. See LICENSE file for details.
 */

#include "motor_controller.h"

// ============================================================================
// PID Controller Implementation
// ============================================================================

PIDController::PIDController(float p, float i, float d, float limit) 
    : kp(p), ki(i), kd(d), output_limit(limit), integral(0), prev_error(0), last_time(0) {
}

void PIDController::reset() {
    integral = 0;
    prev_error = 0;
    last_time = millis();
}

float PIDController::compute(float setpoint, float measured) {
    unsigned long now = millis();
    float dt = (now - last_time) / 1000.0;  // Convert to seconds
    
    if (dt <= 0) dt = 0.001;  // Prevent division by zero
    
    float error = setpoint - measured;
    
    // Proportional term
    float p_term = kp * error;
    
    // Integral term with anti-windup
    integral += error * dt;
    if (integral > output_limit) integral = output_limit;
    if (integral < -output_limit) integral = -output_limit;
    float i_term = ki * integral;
    
    // Derivative term
    float derivative = (error - prev_error) / dt;
    float d_term = kd * derivative;
    
    // Compute output
    float output = p_term + i_term + d_term;
    
    // Limit output
    if (output > output_limit) output = output_limit;
    if (output < -output_limit) output = -output_limit;
    
    prev_error = error;
    last_time = now;
    
    return output;
}

void PIDController::setTunings(float p, float i, float d) {
    kp = p;
    ki = i;
    kd = d;
}

// ============================================================================
// Motor Implementation
// ============================================================================

Motor::Motor(uint8_t motor_id, const MotorPins& motor_pins, float steps_mm, float max_spd, float max_acc)
    : id(motor_id), pins(motor_pins), steps_per_mm(steps_mm), max_speed(max_spd), max_accel(max_acc),
      current_position(0), target_position(0), current_velocity(0), target_velocity(0),
      encoder_count(0), last_encoder_count(0), enabled(false), direction(1),
      alarm_system(nullptr), pid_tuner(nullptr), position_tolerance(0.5), velocity_tolerance(10.0),
      last_alarm_check(0) {
    
    encoder = new Encoder(pins.enc_a, pins.enc_b);
    pid = new PIDController(MOTOR_PID_KP, MOTOR_PID_KI, MOTOR_PID_KD, PID_OUTPUT_LIMIT);
    last_update_time = micros();
}

Motor::~Motor() {
    delete encoder;
    delete pid;
}

void Motor::begin() {
    pinMode(pins.in1, OUTPUT);
    pinMode(pins.in2, OUTPUT);
    
    // Setup PWM Channel
    ledcSetup(id, 20000, 8);  // 20kHz, 8-bit resolution
    
    // Initial state: Stop
    digitalWrite(pins.in1, LOW);
    digitalWrite(pins.in2, LOW);
    
    encoder->write(0);
    pid->reset();
}

void Motor::enable() {
    enabled = true;
    // No enable pin to control (Hardwired)
}

void Motor::disable() {
    enabled = false;
    applyMotorControl(0);
}

bool Motor::isEnabled() {
    return enabled;
}

void Motor::update() {
    if (!enabled) return;
    
    updateEncoder();
    
    // Calculate error
    float position_error = target_position - current_position;
    
    // Check alarms periodically (every 100ms)
    if (alarm_system && (millis() - last_alarm_check > 100)) {
        checkAlarms();
        last_alarm_check = millis();
    }
    
    // Update PID tuner if active
    if (pid_tuner && pid_tuner->isAutoTuning()) {
        pid_tuner->update();
    }
    
    // Compute PID output
    float pid_output = pid->compute(target_position, current_position);
    
    // Apply motor control
    applyMotorControl((int)pid_output);
}

void Motor::updateEncoder() {
    encoder_count = encoder->read();
    
    unsigned long now = micros();
    float dt = (now - last_update_time) / 1000000.0;  // Convert to seconds
    
    if (dt > 0) {
        int32_t delta_counts = encoder_count - last_encoder_count;
        float delta_mm = encoderCountsToMM(delta_counts);
        
        current_position += delta_mm;
        current_velocity = delta_mm / dt;
        
        last_encoder_count = encoder_count;
        last_update_time = now;
    }
}

void Motor::setTargetPosition(float pos_mm) {
    target_position = pos_mm;
}

void Motor::setTargetVelocity(float vel_mm_s) {
    target_velocity = vel_mm_s;
    // Velocity mode: continuously update target position
    // This would be handled in a more sophisticated motion planner
}

float Motor::getCurrentPosition() {
    return current_position;
}

float Motor::getTargetPosition() {
    return target_position;
}

float Motor::getCurrentVelocity() {
    return current_velocity;
}

int32_t Motor::getEncoderCount() {
    return encoder_count;
}

void Motor::resetPosition(float pos) {
    encoder->write(0);
    encoder_count = 0;
    last_encoder_count = 0;
    current_position = pos;
    target_position = pos;
    pid->reset();
}

void Motor::emergencyStop() {
    target_position = current_position;
    target_velocity = 0;
    applyMotorControl(0);
}

void Motor::applyMotorControl(int pwm_value) {
    if (!enabled) {
        ledcDetachPin(pins.in1);
        ledcDetachPin(pins.in2);
        digitalWrite(pins.in1, LOW);
        digitalWrite(pins.in2, LOW);
        return;
    }
    
    // Limit PWM
    if (pwm_value > 255) pwm_value = 255;
    if (pwm_value < -255) pwm_value = -255;
    
    int speed = abs(pwm_value);
    
    // Set direction and PWM
    if (pwm_value > 0) {
        // Forward: IN1=PWM, IN2=LOW
        ledcDetachPin(pins.in2);
        digitalWrite(pins.in2, LOW);
        ledcAttachPin(pins.in1, id);
        ledcWrite(id, speed);
    } else if (pwm_value < 0) {
        // Reverse: IN1=LOW, IN2=PWM
        ledcDetachPin(pins.in1);
        digitalWrite(pins.in1, LOW);
        ledcAttachPin(pins.in2, id);
        ledcWrite(id, speed);
    } else {
        // Stop
        ledcDetachPin(pins.in1);
        ledcDetachPin(pins.in2);
        digitalWrite(pins.in1, LOW);
        digitalWrite(pins.in2, LOW);
        ledcWrite(id, 0);
    }
}

float Motor::encoderCountsToMM(int32_t counts) {
    return (float)counts / steps_per_mm;
}

int32_t Motor::mmToEncoderCounts(float mm) {
    return (int32_t)(mm * steps_per_mm);
}

// ============================================================================
// Motor Controller Implementation
// ============================================================================

MotorController::MotorController() : is_running(false), control_task_handle(NULL) {
    // Initialize motors with their configurations
    motors[MOTOR_X] = new Motor(MOTOR_X, MOTOR_PINS[MOTOR_X], STEPS_PER_MM_X, MAX_SPEED_X, MAX_ACCEL_X);
    motors[MOTOR_Y] = new Motor(MOTOR_Y, MOTOR_PINS[MOTOR_Y], STEPS_PER_MM_Y, MAX_SPEED_Y, MAX_ACCEL_Y);
    motors[MOTOR_Z] = new Motor(MOTOR_Z, MOTOR_PINS[MOTOR_Z], STEPS_PER_MM_Z, MAX_SPEED_Z, MAX_ACCEL_Z);
    motors[MOTOR_E] = new Motor(MOTOR_E, MOTOR_PINS[MOTOR_E], STEPS_PER_MM_E, MAX_SPEED_E, MAX_ACCEL_E);
}

MotorController::~MotorController() {
    stopControlLoop();
    for (int i = 0; i < NUM_MOTORS; i++) {
        delete motors[i];
    }
}

void MotorController::begin() {
    for (int i = 0; i < NUM_MOTORS; i++) {
        motors[i]->begin();
    }
}

void MotorController::startControlLoop() {
    if (!is_running) {
        is_running = true;
        xTaskCreatePinnedToCore(
            controlLoopTask,
            "MotorControl",
            4096,
            this,
            1,
            &control_task_handle,
            1  // Run on core 1
        );
    }
}

void MotorController::stopControlLoop() {
    if (is_running) {
        is_running = false;
        if (control_task_handle != NULL) {
            vTaskDelete(control_task_handle);
            control_task_handle = NULL;
        }
    }
}

void MotorController::controlLoopTask(void* parameter) {
    MotorController* controller = (MotorController*)parameter;
    
    const TickType_t xFrequency = pdMS_TO_TICKS(1000 / MOTOR_CONTROL_FREQ);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (controller->is_running) {
        controller->controlLoop();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void MotorController::controlLoop() {
    // Update all motors
    for (int i = 0; i < NUM_MOTORS; i++) {
        motors[i]->update();
    }
    
    // Plan future motion (trajectory planning)
    planMotion();
}

void MotorController::planMotion() {
    // Implement trajectory planning here
    // This could include acceleration/deceleration profiles, jerk limiting, etc.
}

void MotorController::moveAbsolute(uint8_t motor_id, float position) {
    if (motor_id < NUM_MOTORS) {
        motors[motor_id]->setTargetPosition(position);
    }
}

void MotorController::moveRelative(uint8_t motor_id, float distance) {
    if (motor_id < NUM_MOTORS) {
        float current = motors[motor_id]->getCurrentPosition();
        motors[motor_id]->setTargetPosition(current + distance);
    }
}

void MotorController::setVelocity(uint8_t motor_id, float velocity) {
    if (motor_id < NUM_MOTORS) {
        motors[motor_id]->setTargetVelocity(velocity);
    }
}

void MotorController::linearMove(float x1, float x2, float y1, float y2, float z, float e, float feedrate) {
    // Coordinated linear motion
    // Calculate the time needed based on feedrate and distance
    // Set target positions for all motors to reach simultaneously
    
    motors[MOTOR_X1]->setTargetPosition(x1);
    motors[MOTOR_X2]->setTargetPosition(x2);
    motors[MOTOR_Y1]->setTargetPosition(y1);
    motors[MOTOR_Y2]->setTargetPosition(y2);
    motors[MOTOR_Z]->setTargetPosition(z);
    motors[MOTOR_E]->setTargetPosition(e);
}

void MotorController::rapidMove(float x1, float x2, float y1, float y2, float z, float e) {
    // Rapid move at maximum speed
    linearMove(x1, x2, y1, y2, z, e, 0);  // 0 means max speed
}

float MotorController::getPosition(uint8_t motor_id) {
    if (motor_id < NUM_MOTORS) {
        return motors[motor_id]->getCurrentPosition();
    }
    return 0;
}

float MotorController::getVelocity(uint8_t motor_id) {
    if (motor_id < NUM_MOTORS) {
        return motors[motor_id]->getCurrentVelocity();
    }
    return 0;
}

bool MotorController::isMoving() {
    for (int i = 0; i < NUM_MOTORS; i++) {
        if (abs(motors[i]->getTargetPosition() - motors[i]->getCurrentPosition()) > 0.01) {
            return true;
        }
    }
    return false;
}

void MotorController::waitForMotion() {
    // Deprecated: This is a blocking operation that should not be used.
    // Callers should instead check isMoving() periodically in their own loop.
    // Keeping this for API compatibility but logging warning.
    if (Serial.availableForWrite() > 80) {
        Serial.println("Warning: waitForMotion() is blocking - use isMoving() instead");
    }
}

void MotorController::emergencyStop() {
    for (int i = 0; i < NUM_MOTORS; i++) {
        motors[i]->emergencyStop();
    }
}

void MotorController::reset() {
    emergencyStop();
    for (int i = 0; i < NUM_MOTORS; i++) {
        motors[i]->resetPosition();
    }
}

void MotorController::enableMotor(uint8_t motor_id) {
    if (motor_id < NUM_MOTORS) {
        motors[motor_id]->enable();
    }
}

void MotorController::disableMotor(uint8_t motor_id) {
    if (motor_id < NUM_MOTORS) {
        motors[motor_id]->disable();
    }
}

void MotorController::enableAll() {
    for (int i = 0; i < NUM_MOTORS; i++) {
        motors[i]->enable();
    }
}

void MotorController::disableAll() {
    for (int i = 0; i < NUM_MOTORS; i++) {
        motors[i]->disable();
    }
}

void MotorController::home(uint8_t motor_id) {
    // Implement homing routine for specific motor
    // This would typically involve moving until an endstop is triggered
}

// ============================================================================
// Motor Alarm and Tuning Support
// ============================================================================

void Motor::setAlarmSystem(AlarmSystem* alarms) {
    alarm_system = alarms;
}

void Motor::setPIDTuner(PIDTuner* tuner) {
    pid_tuner = tuner;
}

void Motor::setPositionTolerance(float tolerance) {
    position_tolerance = tolerance;
}

void Motor::setVelocityTolerance(float tolerance) {
    velocity_tolerance = tolerance;
}

void Motor::checkAlarms() {
    if (!alarm_system || !enabled) return;
    
    float position_error = abs(target_position - current_position);
    float velocity_error = abs(target_velocity - current_velocity);
    
    // Check position tolerance
    if (position_error > position_tolerance) {
        String msg = "Motor " + String(id) + " position error: " + String(position_error, 2) + "mm";
        alarm_system->raiseAlarm(
            ALARM_MOTOR_POSITION_ERROR,
            position_error > (position_tolerance * 2) ? ALARM_ERROR : ALARM_WARNING,
            position_error,
            position_tolerance,
            msg
        );
    } else {
        alarm_system->clearAlarm(ALARM_MOTOR_POSITION_ERROR);
    }
    
    // Check velocity tolerance (only if moving)
    if (abs(target_velocity) > 0.1 && velocity_error > velocity_tolerance) {
        String msg = "Motor " + String(id) + " velocity error: " + String(velocity_error, 2) + "mm/s";
        alarm_system->raiseAlarm(
            ALARM_MOTOR_VELOCITY_ERROR,
            ALARM_WARNING,
            velocity_error,
            velocity_tolerance,
            msg
        );
    } else {
        alarm_system->clearAlarm(ALARM_MOTOR_VELOCITY_ERROR);
    }
    
    // Check for stall (encoder not moving but should be)
    if (abs(target_velocity) > 1.0 && abs(current_velocity) < 0.1) {
        alarm_system->raiseAlarm(
            ALARM_MOTOR_STALL,
            ALARM_ERROR,
            current_velocity,
            target_velocity,
            "Motor " + String(id) + " stall detected"
        );
    } else {
        alarm_system->clearAlarm(ALARM_MOTOR_STALL);
    }
    
    // Check for overspeed
    if (abs(current_velocity) > max_speed) {
        alarm_system->raiseAlarm(
            ALARM_MOTOR_OVERSPEED,
            ALARM_CRITICAL,
            abs(current_velocity),
            max_speed,
            "Motor " + String(id) + " overspeed: " + String(abs(current_velocity), 2) + "mm/s"
        );
        emergencyStop();
    } else {
        alarm_system->clearAlarm(ALARM_MOTOR_OVERSPEED);
    }
}

float Motor::getPositionError() {
    return target_position - current_position;
}

void MotorController::homeAll() {
    // Home all axes in sequence
}

Motor* MotorController::getMotor(uint8_t motor_id) {
    if (motor_id < NUM_MOTORS) {
        return motors[motor_id];
    }
    return nullptr;
}

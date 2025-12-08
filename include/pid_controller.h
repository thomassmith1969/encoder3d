#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>

class PIDController {
private:
    float kp, ki, kd;
    float integral;
    float prevError;
    long lastTime;

public:
    PIDController(float p, float i, float d) {
        kp = p;
        ki = i;
        kd = d;
        integral = 0;
        prevError = 0;
        lastTime = 0;
    }

    int compute(long setpoint, long input) {
        long now = micros();
        float dt = (now - lastTime) / 1000000.0; // Seconds
        if (dt <= 0) dt = 0.001; // Prevent div by zero on first run
        lastTime = now;

        float error = setpoint - input;
        integral += error * dt;
        float derivative = (error - prevError) / dt;
        prevError = error;

        float output = (kp * error) + (ki * integral) + (kd * derivative);
        
        // Clamp output for PWM (8-bit)
        if (output > 255) output = 255;
        if (output < -255) output = -255;

        return (int)output;
    }

    void reset() {
        integral = 0;
        prevError = 0;
        lastTime = micros();
    }
};

#endif

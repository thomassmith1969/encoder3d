#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <Arduino.h>
#include "config.h"

class MotorDriver {
private:
    uint8_t pinA;
    uint8_t pinB;
    uint8_t pwmChannel;
    int8_t attachedPin; // -1: None, 0: PinA, 1: PinB

public:
    MotorDriver(uint8_t pA, uint8_t pB, uint8_t channel) {
        pinA = pA;
        pinB = pB;
        pwmChannel = channel;
        attachedPin = -1;
    }

    void begin() {
        pinMode(pinA, OUTPUT);
        pinMode(pinB, OUTPUT);
        digitalWrite(pinA, LOW);
        digitalWrite(pinB, LOW);
        ledcSetup(pwmChannel, PWM_FREQ, PWM_RES);
    }

    // The "One Channel Switched" Logic
    void setSpeed(int speed) {
        // Constrain speed to -255 to 255
        if (speed > 255) speed = 255;
        if (speed < -255) speed = -255;

        if (speed > 0) {
            // FORWARD: PWM on Pin A, Ground Pin B
            if (attachedPin == 1) {
                ledcDetachPin(pinB);
                pinMode(pinB, OUTPUT);
                attachedPin = -1;
            }
            digitalWrite(pinB, LOW);
            
            if (attachedPin != 0) {
                ledcAttachPin(pinA, pwmChannel);
                attachedPin = 0;
            }
            ledcWrite(pwmChannel, abs(speed));
        } 
        else if (speed < 0) {
            // REVERSE: PWM on Pin B, Ground Pin A
            if (attachedPin == 0) {
                ledcDetachPin(pinA);
                pinMode(pinA, OUTPUT);
                attachedPin = -1;
            }
            digitalWrite(pinA, LOW);
            
            if (attachedPin != 1) {
                ledcAttachPin(pinB, pwmChannel);
                attachedPin = 1;
            }
            ledcWrite(pwmChannel, abs(speed));
        } 
        else {
            // STOP: Ground both
            if (attachedPin == 0) {
                ledcDetachPin(pinA);
                pinMode(pinA, OUTPUT);
                attachedPin = -1;
            }
            if (attachedPin == 1) {
                ledcDetachPin(pinB);
                pinMode(pinB, OUTPUT);
                attachedPin = -1;
            }
            digitalWrite(pinA, LOW);
            digitalWrite(pinB, LOW);
        }
    }
};

#endif

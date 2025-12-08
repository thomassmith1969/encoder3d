#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <Arduino.h>
#include "config.h"

class MotorDriver {
private:
    uint8_t pinA;
    uint8_t pinB;
    int8_t attachedPin; // -1: None, 0: PinA, 1: PinB

public:
    MotorDriver(uint8_t pA, uint8_t pB, uint8_t channel) {
        pinA = pA;
        pinB = pB;
        attachedPin = -1;
    }

    void begin() {
        pinMode(pinA, OUTPUT);
        pinMode(pinB, OUTPUT);
        digitalWrite(pinA, LOW);
        digitalWrite(pinB, LOW);
    }

    // The "One Channel Switched" Logic
    void setSpeed(int speed) {
        // Constrain speed to -255 to 255
        if (speed > 255) speed = 255;
        if (speed < -255) speed = -255;

        if (speed > 0) {
            // FORWARD: PWM on Pin A, Ground Pin B
            if (attachedPin == 1) {
                ledcDetach(pinB);
                attachedPin = -1;
            }
            digitalWrite(pinB, LOW);
            
            if (attachedPin != 0) {
                if (ledcAttach(pinA, PWM_FREQ, PWM_RES)) {
                    attachedPin = 0;
                }
            }
            ledcWrite(pinA, abs(speed));
        } 
        else if (speed < 0) {
            // REVERSE: PWM on Pin B, Ground Pin A
            if (attachedPin == 0) {
                ledcDetach(pinA);
                attachedPin = -1;
            }
            digitalWrite(pinA, LOW);
            
            if (attachedPin != 1) {
                if (ledcAttach(pinB, PWM_FREQ, PWM_RES)) {
                    attachedPin = 1;
                }
            }
            ledcWrite(pinB, abs(speed));
        } 
        else {
            // STOP: Ground both
            if (attachedPin == 0) {
                ledcDetach(pinA);
                attachedPin = -1;
            }
            if (attachedPin == 1) {
                ledcDetach(pinB);
                attachedPin = -1;
            }
            digitalWrite(pinA, LOW);
            digitalWrite(pinB, LOW);
        }
    }
};

#endif

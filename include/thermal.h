#ifndef THERMAL_H
#define THERMAL_H

#include <Arduino.h>
#include "config.h"

class ThermalManager {
private:
    int extPin, bedPin;
    int extPwm, bedPwm;
    int extPwmChan, bedPwmChan;
    double targetExt, targetBed;
    double currentExt, currentBed; // Cache readings

    // Simple Steinhart-Hart implementation for 100k Thermistor
    // Beta 3950, 4.7k Pullup
    double readThermistor(int pin) {
        int raw = analogRead(pin);
        if (raw == 0) return 0; // Safety
        
        double resistance = 4700.0 * ((4095.0 / raw) - 1);
        double logR = log(resistance);
        double tempK = 1.0 / (0.001129148 + (0.000234125 * logR) + (0.0000000876741 * logR * logR * logR));
        return tempK - 273.15; // Celsius
    }
    
public:
    ThermalManager() {
        extPin = PIN_TEMP_EXT;
        bedPin = PIN_TEMP_BED;
        extPwm = PIN_HEATER_EXT;
        bedPwm = PIN_HEATER_BED;
        extPwmChan = PWM_CHAN_HEAT_E;
        bedPwmChan = PWM_CHAN_HEAT_B;
        targetExt = 0;
        targetBed = 0;
        currentExt = 0;
        currentBed = 0;
    }

    void begin() {
        // 1kHz for heaters, 8-bit resolution
        ledcSetup(extPwmChan, 1000, 8);
        ledcSetup(bedPwmChan, 1000, 8);
        ledcAttachPin(extPwm, extPwmChan);
        ledcAttachPin(bedPwm, bedPwmChan);
    }

    void setExtruderTarget(double temp) {
        targetExt = temp;
    }

    void setBedTarget(double temp) {
        targetBed = temp;
    }

    void setTargets(double ext, double bed) {
        targetExt = ext;
        targetBed = bed;
    }

    double getExtruderTemp() { return currentExt; }
    double getBedTemp() { return currentBed; }
    double getExtruderTarget() { return targetExt; }
    double getBedTarget() { return targetBed; }

    void update() {
        // Read current temperatures
        currentExt = readThermistor(extPin);
        currentBed = readThermistor(bedPin);

        // Extruder (Simple P-Control for now, full PID later)
        if (currentExt < targetExt) {
            int drive = (targetExt - currentExt) * 20; // P-gain
            if (drive > 255) drive = 255;
            if (drive < 0) drive = 0;
            ledcWrite(extPwmChan, drive);
        } else {
            ledcWrite(extPwmChan, 0);
        }

        // Bed (Bang-Bang with Hysteresis)
        if (currentBed < targetBed - 1.0) {
            ledcWrite(bedPwmChan, 255);
        } else if (currentBed > targetBed) {
            ledcWrite(bedPwmChan, 0);
        }
    }
};

#endif

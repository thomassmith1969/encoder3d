#ifndef THERMAL_H
#define THERMAL_H

#include <Arduino.h>
#include "config.h"

class ThermalManager {
private:
    int extPin, bedPin;
    int extPwm, bedPwm;
    double targetExt, targetBed;

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
        targetExt = 0;
        targetBed = 0;
    }

    void begin() {
        // 1kHz for heaters, 8-bit resolution
        ledcAttach(extPwm, 1000, 8);
        ledcAttach(bedPwm, 1000, 8);
    }

    void setTargets(double ext, double bed) {
        targetExt = ext;
        targetBed = bed;
    }

    void update() {
        // Extruder (Simple P-Control for now, full PID later)
        double currentExt = readThermistor(extPin);
        if (currentExt < targetExt) {
            int drive = (targetExt - currentExt) * 20; // P-gain
            if (drive > 255) drive = 255;
            if (drive < 0) drive = 0;
            ledcWrite(extPwm, drive);
        } else {
            ledcWrite(extPwm, 0);
        }

        // Bed (Bang-Bang with Hysteresis)
        double currentBed = readThermistor(bedPin);
        if (currentBed < targetBed - 1.0) {
            ledcWrite(bedPwm, 255);
        } else if (currentBed > targetBed) {
            ledcWrite(bedPwm, 0);
        }
    }
};

#endif

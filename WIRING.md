# Hardware Wiring Guide

## ESP32 Lolin32 Lite Pinout Reference

### Motor Connections

Each motor requires 5 connections:
1. **PWM** - Motor speed control
2. **DIR** - Direction control
3. **ENABLE** - Motor enable (active LOW)
4. **ENC_A** - Encoder channel A
5. **ENC_B** - Encoder channel B

#### X1 Motor (Primary X-axis)
- PWM: GPIO 25
- DIR: GPIO 26
- ENABLE: GPIO 27
- ENC_A: GPIO 34 (input only)
- ENC_B: GPIO 35 (input only)

#### X2 Motor (Secondary X-axis)
- PWM: GPIO 32
- DIR: GPIO 33
- ENABLE: GPIO 14
- ENC_A: GPIO 36 (input only)
- ENC_B: GPIO 39 (input only)

#### Y1 Motor (Primary Y-axis)
- PWM: GPIO 12
- DIR: GPIO 13
- ENABLE: GPIO 4
- ENC_A: GPIO 15
- ENC_B: GPIO 2

#### Y2 Motor (Secondary Y-axis)
- PWM: GPIO 16
- DIR: GPIO 17
- ENABLE: GPIO 19
- ENC_A: GPIO 5
- ENC_B: GPIO 18

#### Z Motor
- PWM: GPIO 21
- DIR: GPIO 22
- ENABLE: GPIO 27
- ENC_A: GPIO 23
- ENC_B: GPIO 19

#### Extruder Motor
- PWM: GPIO 0
- DIR: GPIO 4
- ENABLE: GPIO 5
- ENC_A: GPIO 16
- ENC_B: GPIO 17

### Heater Connections

#### Hotend Heater
- Control Output: GPIO 25 (PWM)
- Thermistor Input: GPIO 34 (ADC)
- **IMPORTANT**: Use appropriate MOSFET or SSR rated for your heater

#### Heated Bed
- Control Output: GPIO 26 (PWM)
- Thermistor Input: GPIO 35 (ADC)
- **IMPORTANT**: Use appropriate MOSFET or SSR rated for your heater

### Thermistor Wiring

Standard 100kΩ NTC thermistor with 4.7kΩ pullup:

```
3.3V ----[4.7kΩ]---- ADC Pin ---- [Thermistor] ---- GND
```

## Motor Driver Connections

### Typical Motor Driver (e.g., L298N, TB6612, BTS7960)

```
ESP32           Motor Driver        Motor
-----           ------------        -----
PWM     --->    PWM/SPEED           
DIR     --->    IN1/DIR             
ENABLE  --->    EN                  
GND     --->    GND                 
                                    M+  --->  Motor +
                                    M-  --->  Motor -
```

### Encoder Connections

```
Encoder         ESP32
-------         -----
VCC     --->    3.3V (or 5V if encoder is 5V)
GND     --->    GND
A       --->    ENC_A (with level shifter if 5V encoder)
B       --->    ENC_B (with level shifter if 5V encoder)
```

**Note**: If using 5V encoders, use a level shifter or voltage divider for ESP32 compatibility (ESP32 is 3.3V).

## Power Supply Recommendations

### ESP32 Board
- 5V via USB or VIN pin
- Current: ~500mA peak

### Motors
- Voltage: Depends on your motors (typically 12V or 24V)
- Current: Calculate based on motor specs
- Use separate power supply from ESP32

### Heaters
- Hotend: Typically 12V/24V @ 3-5A
- Bed: Typically 12V/24V @ 10-15A
- **Must** use separate high-current power supply

### Power Supply Architecture

```
Main PSU (24V) ---+--- Motor Drivers (24V)
                  |
                  +--- Heater MOSFETs (24V)
                  |
                  +--- Buck Converter (24V to 5V) --- ESP32

Separate USB (5V) --- ESP32 (for programming/debug)
```

## Safety Considerations

1. **Always use proper fuses** on heater circuits
2. **Use thermal fuses** on heaters as backup
3. **Verify all ground connections** are common
4. **Use appropriate wire gauge** for high current (heaters)
5. **Add flyback diodes** on motor drivers if not built-in
6. **Heat sinks** on MOSFETs/drivers as needed
7. **Emergency stop button** wired to kill all power
8. **Enclosure** for all high voltage connections

## Testing Procedure

### 1. Power Up Test (No Motors/Heaters)
- Connect ESP32 via USB only
- Upload firmware
- Verify WiFi connection
- Access web interface

### 2. Encoder Test
- Connect encoders (no motor power)
- Manually rotate motor shafts
- Monitor encoder counts in web interface or serial

### 3. Motor Test (Low Power)
- Connect one motor with driver
- Set low PWM values (25%)
- Test direction control
- Verify encoder feedback

### 4. Full Motor Test
- Connect all motors
- Test jog controls
- Verify coordinated motion
- Check emergency stop

### 5. Heater Test (With Caution)
- Connect thermistors first
- Verify temperature readings at room temp
- Set low target temperature (40°C)
- Monitor temperature rise
- Test thermal runaway protection
- Gradually increase to operational temps

## Common Issues

### Encoders not counting
- Check 3.3V/5V levels
- Verify pullup resistors if needed
- Swap A/B channels if counting backwards
- Check ground connections

### Motors running backwards
- Swap motor wires, or
- Invert direction in firmware, or
- Change DIR pin logic

### Temperature readings incorrect
- Verify thermistor type (100kΩ NTC assumed)
- Check pullup resistor value (4.7kΩ)
- Ensure good ADC pin selection
- Calibrate beta value in code

### WiFi unstable
- Add capacitors near ESP32 power pins
- Use quality power supply
- Reduce PWM frequency if interference
- Move antenna away from motors

## Schematic Notes

**This is a reference implementation.** Your specific hardware may require:
- Different motor drivers
- Different power supplies
- Additional protection circuits
- Different pin assignments
- Level shifters for 5V components

Always consult datasheets for your specific components and verify connections before powering on.

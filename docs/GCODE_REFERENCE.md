# Encoder3D G-Code Quick Reference

## Standard G-Codes
- `G0/G1` - Linear move (X Y Z E F)
- `G28` - Home axes (X Y Z or all)
- `G90` - Absolute positioning
- `G91` - Relative positioning  
- `G92` - Set position (X Y Z E)

## Standard M-Codes
- `M3` - Spindle on CW (S<speed>)
- `M4` - Spindle on CCW (S<speed>)
- `M5` - Spindle off
- `M82` - Absolute extrusion mode
- `M83` - Relative extrusion mode
- `M104` - Set hotend temp (S<temp>)
- `M105` - Report temperatures
- `M106` - Fan/Laser on (S<power>)
- `M107` - Fan/Laser off
- `M109` - Set hotend temp and wait (S<temp>)
- `M112` - Emergency stop
- `M114` - Get current position
- `M119` - Get endstop status
- `M140` - Set bed temp (S<temp>)
- `M190` - Set bed temp and wait (S<temp>)

## Mode Selection
- `M450` - 3D Printer mode
- `M451` - CNC mode
- `M452` - Laser mode

## Laser Extended Commands (M460-M469)
- `M460` - Set laser type (P<type_id>)
- `M461` - Load laser profile (S<name>)
- `M462` - Set laser power in watts (S<watts>)
- `M463` - Set laser power in percent (S<percent>)
- `M464` - Enable/disable ramping (S<0|1> P<rate>)
- `M465` - Set pulse mode (F<freq_Hz> S<duty_%>)
- `M466` - Laser safety check
- `M467` - Emergency laser stop

## Tool Control Commands (M470-M479)
- `M470` - Set tool type (P<type_id>)
- `M471` - Load tool profile (S<name>)
- `M472` - Set tool speed in RPM (S<rpm>)
- `M473` - Set tool speed in percent (S<percent>)
- `M474` - Enable/disable ramping (S<0|1> P<rate_rpm_per_s>)
- `M475` - Tool safety check (reports coolant, air, temp, interlocks)
- `M476` - Emergency tool stop
- `M477` - Set torch height (S<height_mm>) - plasma only
- `M478` - Check plasma safety (air pressure, arc OK)
- `M479` - Plasma pierce sequence (auto pierce with delays)

## SD Card Commands
- `M20` - List files on SD card
- `M21` - Initialize SD card
- `M22` - Release SD card
- `M23` - Select file (filename in comment)
- `M24` - Start/resume SD print
- `M25` - Pause SD print
- `M27` - Report SD print status
- `M30` - Delete SD file

## Alarm Management (M700-M799)
- `M700` - Get alarm status (JSON)
- `M701` - Clear all alarms
- `M702` - Acknowledge all alarms
- `M703` - Set tolerances (S<pos_mm> P<vel_mm/s> T<temp_C>)
- `M704` - Get system health (JSON)

## PID Tuning (M800-M899)
- `M800` - Set motor PID (P<id> S<Kp> E<Ki> F<Kd>)
- `M801` - Set heater PID (P<id> S<Kp> E<Ki> F<Kd>)
- `M802` - Auto-tune motor PID (P<motor_id>)
- `M803` - Auto-tune heater PID (P<heater_id>)
- `M804` - Load PID preset (S<name>)
- `M805` - Save PID preset (S<name>)

## Diagnostics (M900-M999)
- `M900` - Run full diagnostics
- `M901` - Calibrate motors (reset positions)
- `M902` - Test motor (P<motor_id>)
- `M903` - Test heater (P<heater_id> S<temp>)
- `M999` - Reset controller (reboot)

## Common Usage Examples

### Check System Health
```gcode
M704         ; Get health JSON
M700         ; Get active alarms
```

### Configure for Precision Work
```gcode
M703 S0.1 P5.0 T1.0    ; Tight tolerances
M804 S"conservative"   ; Conservative PID
```

### Configure for Fast Production
```gcode
M703 S0.5 P20.0 T3.0   ; Relaxed tolerances
M804 S"aggressive"     ; Aggressive PID
```

### Laser: Blue Diode Wood Engraving
```gcode
M461 S"DIODE_BLUE_10W" ; Load 10W blue diode profile
M463 S30               ; Set 30% power
M466                   ; Check safety
M3 S7200               ; Laser on (30%)
G1 X50 Y0 F1000        ; Engrave at 1000mm/min
M5                     ; Laser off
```

### Laser: CO2 Acrylic Cutting
```gcode
M461 S"CO2_40W"        ; Load CO2 40W profile
M463 S80               ; Set 80% power  
M3 S19200              ; Laser on
G1 X100 F200           ; Cut at 200mm/min
M5                     ; Laser off
```

### Auto-Tune All Heaters
```gcode
M803 P0        ; Tune hotend
M803 P1        ; Tune bed
```

### Motor Diagnostics
```gcode
M902 P0        ; Test X1 motor
M902 P1        ; Test X2 motor
; ... etc
```

### Clear Alarms After Fix
```gcode
M702           ; Acknowledge alarms
M701           ; Clear all alarms
```

### Emergency Procedures
```gcode
M112           ; EMERGENCY STOP
M999           ; Reboot after emergency
```

---

## Usage Examples

### Spindle Control (CNC Mode)
```gcode
; 775 DC Motor spindle
M451                       ; CNC mode
M471 S"DC_775_12V"        ; Load DC motor profile
M3 S8000                  ; Start at 8000 RPM
G1 X10 Y10 F300           ; Cut move
M5                        ; Stop spindle

; VFD Water-cooled spindle
M471 S"VFD_2_2KW_WATER"   ; Load VFD profile
M475                      ; Check coolant flow
M3 S18000                 ; Start at 18000 RPM
; ... cutting operations ...
M5                        ; Stop with cooldown

; Brushless ER11 spindle
M471 S"BLDC_ER11_500W"    ; Load brushless profile
M474 S1 P1000             ; Enable ramping at 1000 RPM/s
M3 S12000                 ; Ramp to 12000 RPM
; ... operations ...
M5                        ; Ramp down and stop
```

### Plasma Cutting
```gcode
; Setup plasma torch
M451                           ; CNC mode
M471 S"PLASMA_CUT50_PILOT"    ; Load 50A plasma
M478                           ; Check air pressure and arc
G0 Z10                         ; Safe height
G0 X50 Y50                     ; Position over pierce point
M479                           ; Auto pierce (height + delay)
M3                             ; Arc on
G1 X100 Y100 F2000            ; Cut at 2000mm/min
G1 X100 Y50                   ; Continue cutting
M5                             ; Arc off
G0 Z10                         ; Safe height
```

---

## Parameter Legend
- `P` - Peripheral ID (motor, heater, etc.)
- `S` - Speed/Power/Setpoint/Primary value
- `E` - Extruder/Ki gain (context-dependent)
- `F` - Feedrate/Kd gain (context-dependent)
- `T` - Temperature/Tolerance (context-dependent)
- `X Y Z` - Axis coordinates
- `I J K` - Arc centers (future use)

---

*For detailed information, see [ALARM_SYSTEM_GUIDE.md](ALARM_SYSTEM_GUIDE.md)*

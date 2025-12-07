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

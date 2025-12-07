# Alarm System and PID Tuning - User Guide

## Overview

The Encoder3D controller includes a comprehensive alarm system and adaptive PID tuning capabilities designed for robust operation in AI-driven manufacturing environments. These systems monitor tolerances, detect anomalies, and optimize control performance automatically.

## Features

### Alarm System

The alarm system provides real-time monitoring of:

- **Motor Performance**: Position errors, velocity tracking, stall detection, overspeed protection
- **Temperature Control**: Overshoot, undershoot, thermal runaway, sensor faults, settling time
- **System Health**: Memory usage, communication timeouts, emergency stops, limit switches
- **Quality Metrics**: Dimensional tolerances, surface quality (for future AI integration)

#### Alarm Severity Levels

- **INFO**: Informational only, no action needed
- **WARNING**: Monitor closely, may require attention
- **ERROR**: Affects quality, intervention recommended
- **CRITICAL**: Immediate action required, system may halt

### PID Tuning

Automatic and manual PID tuning capabilities:

- **Auto-Tuning Methods**:
  - Ziegler-Nichols: Classic method, fast response
  - Tyreus-Luyben: Less overshoot, more stable
  - Cohen-Coon: Better for processes with lag
  - Adaptive: Continuous optimization during operation

- **Performance Metrics**:
  - Rise time
  - Settling time
  - Overshoot percentage
  - Steady-state error
  - Control effort (efficiency)

### System Monitor

Centralized health monitoring and diagnostics:

- Real-time system health score (0-100%)
- Performance metrics tracking
- CPU usage monitoring
- Memory leak detection
- Automated diagnostics

---

## G-Code Commands

### Alarm Management (M700-M799)

#### M700 - Get Alarm Status
```gcode
M700
```
Returns JSON with all active alarms, severity levels, and system health score.

**Response Example**:
```json
{
  "active_alarms": [
    {
      "type": 1,
      "severity": 2,
      "message": "Motor 0 position error: 0.75mm",
      "value": 0.75,
      "threshold": 0.50,
      "duration": 1500,
      "acknowledged": false,
      "count": 3
    }
  ],
  "total_count": 1,
  "health_score": 85.0
}
```

#### M701 - Clear All Alarms
```gcode
M701
```
Clears all active alarms. Use after resolving issues.

#### M702 - Acknowledge All Alarms
```gcode
M702
```
Acknowledges alarms without clearing them. Useful for logging that operator is aware.

#### M703 - Set Alarm Tolerances
```gcode
M703 S<motor_pos_tol> P<motor_vel_tol> T<temp_tol>
```

**Parameters**:
- `S` - Motor position tolerance (mm)
- `P` - Motor velocity tolerance (mm/s)
- `T` - Temperature tolerance (°C)

**Examples**:
```gcode
M703 S0.5           ; Set motor position tolerance to 0.5mm
M703 P10.0          ; Set motor velocity tolerance to 10mm/s
M703 T2.0           ; Set temperature tolerance to ±2°C
M703 S0.3 P5.0 T1.5 ; Set all tolerances
```

#### M704 - Get System Health
```gcode
M704
```
Returns comprehensive system health report in JSON format.

**Response Example**:
```json
{
  "health": {
    "score": 92.5,
    "status": "good"
  },
  "alarms": {
    "active": 1,
    "critical": 0
  },
  "motors": {
    "max_pos_error": 0.15,
    "avg_pos_error": 0.08,
    "max_vel_error": 2.3
  },
  "temperature": {
    "max_error": 1.2,
    "avg_error": 0.8,
    "thermal_runaway": false
  },
  "system": {
    "uptime": 3600,
    "cpu_usage": 45.2,
    "free_heap": 125000,
    "min_free_heap": 118000
  }
}
```

---

### PID Tuning (M800-M899)

#### M800 - Set Motor PID
```gcode
M800 P<motor_id> S<Kp> I<Ki> D<Kd>
```

**Parameters**:
- `P` - Motor ID (0-5)
- `S` - Proportional gain (Kp)
- `I` - Integral gain (Ki) [uses E parameter]
- `D` - Derivative gain (Kd) [uses F parameter]

**Example**:
```gcode
M800 P0 S2.5 E0.5 F0.5  ; Set motor 0 PID values
```

#### M801 - Set Heater PID
```gcode
M801 P<heater_id> S<Kp> E<Ki> F<Kd>
```

**Parameters**:
- `P` - Heater ID (0-1, where 0=hotend, 1=bed)
- `S` - Proportional gain (Kp)
- `E` - Integral gain (Ki)
- `F` - Derivative gain (Kd)

**Example**:
```gcode
M801 P0 S10.0 E0.5 F5.0  ; Set hotend PID
M801 P1 S5.0 E0.2 F2.0   ; Set bed PID
```

#### M802 - Auto-Tune Motor PID
```gcode
M802 P<motor_id>
```

Automatically tunes PID for specified motor using relay feedback test.

**Example**:
```gcode
M802 P0  ; Auto-tune motor 0
```

**Note**: Motor should be unloaded during tuning.

#### M803 - Auto-Tune Heater PID
```gcode
M803 P<heater_id>
```

Automatically tunes heater PID. Takes 5-10 minutes.

**Example**:
```gcode
M803 P0  ; Auto-tune hotend
```

**Warning**: Heater will cycle during tuning. Monitor temperature.

#### M804 - Load PID Preset
```gcode
M804 S<preset_name>
```

**Presets**:
- `conservative` - Low gains, minimal overshoot, slow response
- `balanced` - Moderate gains, good general-purpose
- `aggressive` - High gains, fast response, may overshoot

**Example**:
```gcode
M804 S"balanced"  ; Load balanced PID preset
```

#### M805 - Save PID Preset
```gcode
M805 S<preset_name>
```

Saves current PID values as a named preset.

---

### Diagnostics (M900-M999)

#### M900 - Run Full Diagnostics
```gcode
M900
```

Performs comprehensive system check:
- Alarm system status
- Motor positions and velocities
- Heater temperatures
- Memory statistics
- System uptime and health

Output sent to serial console.

#### M901 - Calibrate Motors
```gcode
M901
```

Resets all motor positions to zero. Use after homing.

#### M902 - Test Motor
```gcode
M902 P<motor_id>
```

Runs a simple test sequence on specified motor:
1. Enables motor
2. Moves 10mm forward
3. Returns to start position

**Example**:
```gcode
M902 P0  ; Test motor 0
```

#### M903 - Test Heater
```gcode
M903 P<heater_id> S<temperature>
```

Tests heater by setting to specified temperature.

**Parameters**:
- `P` - Heater ID
- `S` - Test temperature (°C), default 50°C

**Example**:
```gcode
M903 P0 S100  ; Heat hotend to 100°C
M903 P1       ; Heat bed to 50°C (default)
```

#### M999 - Reset Controller
```gcode
M999
```

Performs a soft reset of the ESP32 controller.

**Warning**: All current operations will be stopped.

---

## Configuration Examples

### Setting Up for High-Precision Work

```gcode
; Tight tolerances for precision machining
M703 S0.1 P5.0 T1.0  ; 0.1mm pos, 5mm/s vel, ±1°C temp
M804 S"conservative" ; Use conservative PID
```

### Setting Up for Fast Production

```gcode
; Relaxed tolerances for speed
M703 S0.5 P20.0 T3.0  ; 0.5mm pos, 20mm/s vel, ±3°C temp
M804 S"aggressive"    ; Use aggressive PID
```

### Routine Maintenance Check

```gcode
M900  ; Run full diagnostics
M704  ; Check system health
M700  ; Review any alarms
```

### After Assembly or Repair

```gcode
M901  ; Calibrate motors
M902 P0  ; Test each motor individually
M902 P1
M902 P2
M902 P3
M902 P4
M902 P5
M903 P0 S100  ; Test hotend
M903 P1 S60   ; Test bed
```

---

## Integration with AI Manufacturing

The alarm system is designed for seamless integration with AI-driven manufacturing:

### Quality Metrics

Future firmware updates will support:
- Dimensional tolerance alarms based on vision system feedback
- Surface quality monitoring
- Layer adhesion detection
- Extrusion consistency tracking

### Example AI Integration Workflow

1. **Camera captures part image** → Processes dimensions
2. **AI compares to CAD model** → Detects deviations
3. **If out of tolerance** → Raises `ALARM_DIMENSIONAL_TOLERANCE`
4. **System responds**:
   - Pauses print
   - Notifies operator
   - Adjusts PID if needed
   - Logs event for learning

### Alarm Callback for External Systems

```cpp
// Example: Custom alarm handler for external notification
void myAlarmHandler(const Alarm& alarm) {
    if (alarm.severity == ALARM_CRITICAL) {
        // Send notification to AI control system
        notifyAI(alarm.type, alarm.value);
        
        // Log to database
        logToDatabase(alarm);
    }
}

// Register callback
alarm_system.setAlarmCallback(myAlarmHandler);
```

---

## Tolerance Recommendations

### 3D Printing
- Position tolerance: 0.3-0.5mm
- Velocity tolerance: 10-15mm/s
- Temperature tolerance: ±2-3°C

### CNC Milling
- Position tolerance: 0.05-0.1mm
- Velocity tolerance: 5-10mm/s
- Temperature: N/A

### Laser Cutting
- Position tolerance: 0.1-0.2mm
- Velocity tolerance: 10-20mm/s
- Temperature: N/A (unless laser temp monitoring added)

---

## Troubleshooting

### Constant Position Error Alarms

**Problem**: Motor can't maintain position within tolerance

**Solutions**:
1. Increase tolerance: `M703 S1.0`
2. Check mechanical binding
3. Tune PID: `M802 P<motor_id>`
4. Reduce acceleration limits in config

### Temperature Oscillation

**Problem**: Temperature swings ±5°C

**Solutions**:
1. Auto-tune PID: `M803 P<heater_id>`
2. Use conservative preset: `M804 S"conservative"`
3. Check for drafts or cooling fan interference
4. Verify thermistor connection

### Low System Health Score

**Problem**: Health score below 70%

**Solutions**:
1. Run diagnostics: `M900`
2. Check all alarms: `M700`
3. Acknowledge resolved alarms: `M702`
4. Clear old alarms: `M701`
5. Check memory: Look for `free_heap` in `M704` response

### Memory Warnings

**Problem**: Low heap memory alarms

**Solutions**:
1. Restart controller: `M999`
2. Reduce command queue size in firmware
3. Clear old alarm history: `M701`
4. Check for memory leaks in custom code

---

## Advanced Topics

### Adaptive PID Tuning

Enable continuous PID optimization:

```cpp
// In main loop
pid_tuner.enableAdaptiveTuning(true);

// PID will auto-adjust every 60 seconds based on performance
pid_tuner.adaptGains();
```

### Custom Alarm Types

Add project-specific alarms:

```cpp
// Define in alarm_system.h
enum AlarmType {
    // ... existing types ...
    ALARM_CUSTOM_CAMERA_FAULT = 100,
    ALARM_CUSTOM_MATERIAL_FLOW = 101
};

// Raise in your code
alarm_system.raiseAlarm(
    ALARM_CUSTOM_CAMERA_FAULT,
    ALARM_ERROR,
    0,
    1,
    "Camera connection lost"
);
```

### Performance Monitoring

Track PID performance:

```cpp
pid_tuner.startPerformanceTest(target_temp);
// ... run for a while ...
pid_tuner.stopPerformanceTest();

PIDPerformance perf = pid_tuner.getPerformance();
Serial.printf("Overshoot: %.1f%%\n", perf.overshoot);
Serial.printf("Settling time: %.1fs\n", perf.settling_time / 1000.0);
Serial.printf("Score: %.1f/100\n", pid_tuner.getPerformanceScore());
```

---

## API Reference

See source code documentation:
- `include/alarm_system.h` - Alarm system API
- `include/pid_tuner.h` - PID tuning API
- `include/system_monitor.h` - System monitoring API

---

## Support

For issues, feature requests, or contributions, visit:
https://github.com/thomassmith1969/encoder3d

---

*Document version 1.0 - December 2025*

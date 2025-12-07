# Robust Control System Architecture

## Overview

The Encoder3D controller implements a comprehensive, production-ready control system designed for AI-driven manufacturing environments. The system provides real-time monitoring, adaptive control, and robust fault detection suitable for semi-autonomous production lines.

## System Components

```
┌─────────────────────────────────────────────────────────────────┐
│                        User Interfaces                          │
├─────────────┬────────────────┬─────────────────┬───────────────┤
│ Web Browser │ Telnet Client  │ Serial Console  │ REST API      │
│ (Port 80)   │ (Port 23)      │ (USB)           │ (HTTP/WS)     │
└─────────────┴────────────────┴─────────────────┴───────────────┘
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                     G-Code Parser                               │
│  • Command interpretation  • Motion planning                    │
│  • Queue management       • Error handling                      │
└─────────────────────────────────────────────────────────────────┘
                                ▼
┌────────────────────────────┬────────────────────────────────────┐
│      Motor Controller      │     Heater Controller              │
│  • 6 encoder motors        │  • Hotend + bed                    │
│  • PID feedback loops      │  • PID temperature control         │
│  • Position/velocity       │  • Thermal runaway protection      │
└────────────────────────────┴────────────────────────────────────┘
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Monitoring Layer                             │
├─────────────────┬──────────────────┬───────────────────────────┤
│  Alarm System   │   PID Tuner      │   System Monitor          │
│  • Tolerances   │  • Auto-tune     │  • Health scoring         │
│  • Detection    │  • Adaptive      │  • Diagnostics            │
│  • Notification │  • Metrics       │  • Resource tracking      │
└─────────────────┴──────────────────┴───────────────────────────┘
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Hardware Abstraction Layer                    │
│  • PWM generation  • Encoder reading  • ADC sampling            │
│  • GPIO control    • SPI/I2C buses    • WiFi stack              │
└─────────────────────────────────────────────────────────────────┘
```

## Control Flow

### 1. Command Processing
```
User Input → Parser → Validation → Queue → Execution
                                             ↓
                                    Update Controllers
                                             ↓
                                    Check Tolerances
                                             ↓
                                    Raise Alarms (if needed)
```

### 2. Real-Time Control Loop (1kHz)
```
Loop Start (1ms)
├── Read all encoder positions
├── Read all temperatures
├── Motor PID calculations (6x)
├── Heater PID calculations (2x)
├── Apply PWM outputs
├── Check critical alarms
└── Update telemetry
```

### 3. Monitoring Loop (Configurable, default 1Hz)
```
Monitor Update
├── Calculate system health score
├── Check position tolerances
├── Check velocity tolerances
├── Check temperature tolerances
├── Update performance metrics
├── Adaptive PID adjustments (if enabled)
└── Send WebSocket updates
```

## Alarm System Architecture

### Alarm Types & Severity

```
Motor Alarms (ALARM_MOTOR_*)
├── POSITION_ERROR → WARNING/ERROR
├── VELOCITY_ERROR → WARNING
├── STALL → ERROR
├── ENCODER_FAULT → CRITICAL
└── OVERSPEED → CRITICAL (triggers E-stop)

Temperature Alarms (ALARM_TEMP_*)
├── OVERSHOOT → WARNING/ERROR
├── UNDERSHOOT → WARNING
├── THERMAL_RUNAWAY → CRITICAL (emergency shutdown)
├── SENSOR_FAULT → CRITICAL
├── SETTLING_TIMEOUT → WARNING
└── OSCILLATION → WARNING

System Alarms (ALARM_*)
├── COMMUNICATION_TIMEOUT → WARNING
├── BUFFER_OVERFLOW → WARNING/CRITICAL
├── POWER_FLUCTUATION → WARNING
└── EMERGENCY_STOP → CRITICAL
```

### Alarm Processing Pipeline

```
1. Condition Detected
   └→ Check debounce timer (prevent spam)
      └→ Create or update alarm record
         └→ Determine severity level
            └→ Update system health score
               └→ Execute callback (if registered)
                  └→ Notify via WebSocket
                     └→ Log to console
```

### Health Scoring Algorithm

```
Base Score: 100%

Deductions:
├── Each CRITICAL alarm: -25 points
├── Each ERROR alarm: -10 points
├── Each WARNING alarm: -5 points
├── Each INFO alarm: -1 point
├── Low memory: -10 points
├── High position error: -15 points
├── High temperature error: -15 points
└── Thermal runaway: Score = 0 (critical failure)

Final Score: max(0, min(100, calculated_score))

Health Status:
├── 90-100%: EXCELLENT
├── 70-89%:  GOOD
├── 50-69%:  FAIR
├── 30-49%:  POOR
└── 0-29%:   CRITICAL
```

## PID Tuning System

### Auto-Tuning Process

```
1. Initialize Relay Test
   ├── Disable other controllers
   ├── Set relay output parameters
   └── Reset data collectors

2. Run Relay Feedback Test
   ├── Toggle output based on error
   ├── Measure oscillation amplitude
   ├── Measure oscillation period
   ├── Collect multiple cycles (10+)
   └── Calculate ultimate gain (Ku) and period (Pu)

3. Apply Tuning Method
   ├── Ziegler-Nichols
   │   ├── Kp = 0.6 * Ku
   │   ├── Ki = 1.2 * Ku / Pu
   │   └── Kd = 0.075 * Ku * Pu
   │
   ├── Tyreus-Luyben (less overshoot)
   │   ├── Kp = 0.45 * Ku
   │   ├── Ki = 0.54 * Ku / (2.2 * Pu)
   │   └── Kd = 0.45 * Ku * Pu / 6.3
   │
   └── Cohen-Coon (for lag systems)
       ├── Kp = 0.9 * Ku
       ├── Ki = Kp / (1.2 * Pu)
       └── Kd = 0.5 * Kp * Pu

4. Validate Results
   ├── Clamp to safety limits
   ├── Measure performance
   └── Calculate score
```

### Adaptive Tuning Process

```
Every 60 seconds:
1. Analyze error history (last 10 samples)
   ├── Calculate mean error
   └── Calculate standard deviation

2. Adjust gains
   ├── High steady-state error → Increase Ki (1.05x)
   ├── Low steady-state error → Decrease Ki (0.98x)
   ├── High oscillation (σ > 2.0) → Reduce Kp (0.95x), Increase Kd (1.05x)
   └── Low oscillation (σ < 0.5) → Increase Kp (1.02x)

3. Apply and monitor
```

## Performance Metrics

### Motor Control Metrics
- **Position Error**: Target vs. actual position (mm)
- **Velocity Error**: Target vs. actual velocity (mm/s)
- **Encoder Counts**: Raw encoder feedback
- **Control Output**: PWM duty cycle (0-255)
- **Settling Time**: Time to reach target ±tolerance

### Temperature Control Metrics
- **Temperature Error**: Target vs. actual temp (°C)
- **Rise Time**: Time to reach 90% of setpoint
- **Overshoot**: Peak temp - target temp (%)
- **Oscillation**: Standard deviation of temp
- **Control Effort**: Average PWM output

### System Metrics
- **Health Score**: 0-100%
- **Active Alarms**: Count by severity
- **CPU Usage**: Estimated from task timing
- **Free Heap**: Available RAM (bytes)
- **Uptime**: Seconds since boot

## Integration Points for AI Systems

### Alarm Callback Registration

```cpp
void aiAlarmHandler(const Alarm& alarm) {
    // Send to AI control system
    sendToAI({
        "type": alarm.type,
        "severity": alarm.severity,
        "value": alarm.value,
        "timestamp": alarm.timestamp
    });
    
    // AI decision making
    if (alarm.type == ALARM_DIMENSIONAL_TOLERANCE) {
        adjustPrintParameters();
    }
}

alarm_system.setAlarmCallback(aiAlarmHandler);
```

### REST API Endpoints for AI Integration

```
GET  /api/health              → System health JSON
GET  /api/alarms              → Active alarms JSON
GET  /api/metrics             → Performance metrics JSON
POST /api/tolerances          → Update tolerances
POST /api/pid/tune            → Trigger auto-tune
GET  /api/motors/status       → All motor states
GET  /api/heaters/status      → All heater states
```

### WebSocket Events

```javascript
// Real-time notifications to AI systems
ws.on('alarm', (data) => {
    // Process alarm in real-time
    if (data.severity === 'CRITICAL') {
        pauseProduction();
        notifyOperator();
    }
});

ws.on('health', (data) => {
    // Monitor system health
    if (data.score < 50) {
        scheduleMainte();
    }
});
```

## Future Enhancements for AI Manufacturing

### Planned Features
1. **Vision System Integration**
   - Camera-based quality inspection
   - Dimensional verification alarms
   - Surface quality monitoring

2. **Machine Learning Integration**
   - Predictive maintenance based on metrics
   - Automated PID optimization from historical data
   - Anomaly detection

3. **Multi-Machine Coordination**
   - Production line orchestration
   - Resource allocation
   - Quality tracking across batches

4. **Advanced Analytics**
   - Long-term performance trending
   - Failure mode analysis
   - Process optimization recommendations

## Safety Considerations

### Critical Safety Features
1. **Hardware E-Stop** (external button)
2. **Software E-Stop** (M112, triggered on critical alarms)
3. **Thermal Runaway Protection** (automatic shutdown)
4. **Overspeed Protection** (immediate motor cutoff)
5. **Watchdog Timer** (reset if firmware hangs)
6. **Position Limits** (software endstops)

### Safety Response Matrix

| Alarm Type | Severity | Response |
|------------|----------|----------|
| Motor Stall | ERROR | Pause, notify |
| Overspeed | CRITICAL | E-stop, disable motor |
| Thermal Runaway | CRITICAL | Emergency shutdown, disable heaters |
| Position Error (high) | ERROR | Slow motion, notify |
| Sensor Fault | CRITICAL | Disable affected system |
| Memory Low | WARNING | Clear buffers, notify |

## Configuration Best Practices

### Tolerance Settings by Application

**High-Precision Machining**
```gcode
M703 S0.05 P5.0 T1.0  ; ±0.05mm, ±5mm/s, ±1°C
```

**Standard 3D Printing**
```gcode
M703 S0.3 P10.0 T2.0  ; ±0.3mm, ±10mm/s, ±2°C
```

**Fast Prototyping**
```gcode
M703 S0.5 P20.0 T3.0  ; ±0.5mm, ±20mm/s, ±3°C
```

### PID Presets by System

**Temperature Control (Slow Thermal Mass)**
```gcode
M804 S"conservative"  ; Low overshoot, stable
```

**Motor Control (Fast Response)**
```gcode
M804 S"balanced"      ; Good response, minimal overshoot
```

**High-Speed Applications**
```gcode
M804 S"aggressive"    ; Fast response, may overshoot
```

## Monitoring Dashboard Recommendations

### Essential Metrics to Display
1. System health score with trend graph
2. Active alarm count by severity
3. Motor position errors (all axes)
4. Temperature errors (all heaters)
5. CPU and memory usage
6. Print progress (if applicable)

### Alert Thresholds
- Health < 70%: Warning notification
- Health < 50%: Urgent notification
- Any CRITICAL alarm: Immediate alert
- Temperature error > 5°C: Warning
- Position error > 1mm: Warning

---

## Technical Implementation Notes

### Memory Management
- Alarm history: 50 max entries (circular buffer)
- Active alarms: 20 max concurrent
- Temperature history: 20 samples per heater
- Error history: 10 samples for adaptive tuning

### Timing Constraints
- Motor PID loop: 1kHz (1ms period)
- Heater PID loop: 10Hz (100ms period)
- Alarm checking: 10Hz motors, 2Hz heaters
- Health monitoring: 1Hz
- Adaptive tuning: Every 60 seconds

### Communication Protocols
- Serial: 115200 baud, 8N1
- Telnet: Raw TCP, port 23
- HTTP: Port 80, REST API
- WebSocket: Port 80, real-time updates

---

*For implementation details, see source code and documentation*

# Quick Start Guide

## First Time Setup

### 1. Hardware Assembly
- Wire motors according to `WIRING.md`
- Connect encoders to ESP32
- Wire thermistors with 4.7kΩ pullups
- Connect motor drivers
- **DO NOT** connect heaters yet

### 2. Software Installation

```bash
# Install PlatformIO if not already installed
# See: https://platformio.org/install

# Navigate to project directory
cd encoder3d

# Edit configuration
nano include/config.h
# Set your WiFi credentials
# Verify pin assignments match your wiring

# Build firmware
pio run

# Upload firmware to ESP32
pio run --target upload

# Upload web interface files
pio run --target uploadfs

# Open serial monitor
pio device monitor -b 115200
```

### 3. Initial Testing

Watch the serial monitor output. You should see:

```
Encoder3D CNC Controller
Initializing motor controller...
Initializing heater controller...
...
System Initialization Complete!
Web Interface: http://192.168.4.1
```

### 4. Access Web Interface

**AP Mode (default):**
1. Connect to WiFi: `CNC_Controller` (password: `encoder3d`)
2. Open browser: `http://192.168.4.1`

**Station Mode:**
1. Check serial monitor for IP address
2. Open browser to that IP

### 5. First Motion Test

In the web interface:
1. Verify position shows 0,0,0,0
2. Set jog distance to 1mm
3. Click any jog button
4. Watch for motor movement
5. Verify encoder count changes

### 6. Temperature Test (If heaters connected)

1. Set hotend target to 40°C
2. Monitor temperature rise
3. Verify PID control stabilizes
4. Test emergency stop
5. Gradually increase to operating temperature

## Default Controls

### Web Interface
- **Jog Controls**: Manual positioning
- **Home**: Return to zero position
- **Temperature**: Set hotend/bed temperatures
- **Console**: Send G-code commands
- **Emergency Stop**: Halt everything immediately

### Telnet
```bash
telnet 192.168.4.1 23
```
Then send G-code commands directly.

### USB Serial
Connect via USB, 115200 baud, send G-code commands.

## Common First Commands

```gcode
G28          ; Home all axes
M114         ; Report current position
M105         ; Report temperatures
G92 X0 Y0 Z0 ; Set current position as zero
G1 X10 Y10   ; Move to X=10, Y=10
M104 S200    ; Set hotend to 200°C (3D printing mode)
M140 S60     ; Set bed to 60°C
M84          ; Disable motors
```

## Configuration Checklist

- [ ] WiFi credentials set in `config.h`
- [ ] Pin assignments verified for your board
- [ ] Motor direction correct (swap if needed)
- [ ] Encoder feedback working (counts increasing)
- [ ] Steps per mm calibrated
- [ ] Temperature readings accurate
- [ ] PID tuned for your heaters
- [ ] Safety limits configured
- [ ] Emergency stop tested

## Calibration

### Steps Per MM

1. Mark a position on the axis
2. Move exactly 100mm using G-code: `G91 G1 X100 F1000`
3. Measure actual distance moved
4. Calculate: `new_steps = old_steps * (100 / actual_distance)`
5. Update `STEPS_PER_MM_X` in `config.h`

### PID Tuning

For best temperature control, tune PID values:

1. Heat to target temperature
2. Observe oscillation
3. Adjust Kp, Ki, Kd values
4. Reload firmware

Or use auto-tuning G-code (future feature).

## Troubleshooting

**Motors don't move:**
- Check enable pin (try inverting logic)
- Verify motor driver power
- Check PWM output on oscilloscope

**Wrong direction:**
- Invert DIR pin logic in code, or
- Swap motor wires

**Encoders not working:**
- Check voltage levels (3.3V vs 5V)
- Verify ground connection
- Test encoder separately

**WiFi won't connect:**
- Verify credentials
- Try AP mode first
- Check antenna connection
- Monitor serial output

**Web interface doesn't load:**
- Verify filesystem upload: `pio run --target uploadfs`
- Clear browser cache
- Try different browser
- Check serial for errors

## Next Steps

1. Read full `README.md`
2. Review `WIRING.md` for your specific setup
3. Calibrate all axes
4. Tune PID controllers
5. Test emergency stop thoroughly
6. Set up end stops (if using)
7. Configure machine limits
8. Create test G-code programs

## Safety Reminders

⚠️ **ALWAYS:**
- Keep emergency stop accessible
- Monitor first runs closely
- Start with low power/temperature
- Verify wiring before powering on
- Use appropriate fuses
- Follow electrical safety practices

⚠️ **NEVER:**
- Leave heating elements unattended
- Bypass safety features
- Use in production without extensive testing
- Operate without emergency stop

## Getting Help

Check documentation:
- `README.md` - Complete documentation
- `WIRING.md` - Hardware connections
- `config.h` - All configuration options

Serial monitor shows detailed debug information.

---

**Have fun building! And remember: safety first!** 🔧🤖

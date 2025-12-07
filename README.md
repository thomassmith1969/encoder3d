# Encoder3D CNC Controller

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-blue.svg)](https://platformio.org/)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](CONTRIBUTING.md)

A comprehensive ESP32-based CNC controller firmware for encoder motor-driven 3D printers, CNC machines, and laser cutters.

> **⚠️ SAFETY WARNING**: This firmware controls motors and heaters that can cause physical harm if improperly configured. Always implement proper safety measures including emergency stops, thermal protection, and careful testing. Use at your own risk.

## Features

### Hardware Support
- **6-Axis Encoder Motor Control**: Dual X motors, dual Y motors, Z axis, and extruder
- **Real-time PID Feedback**: Closed-loop control using quadrature encoders
- **Dual Heater Control**: Hotend and heated bed with PID temperature control
- **SD Card Storage**: Direct microSD card support for G-code files
- **Safety Features**: Thermal runaway protection, emergency stop, position limits
- **Multi-Mode Operation**: 3D Printer, CNC Spindle, and Laser Cutter modes

### Software Features
- **G-code Interpreter**: Standard G-code and M-code support
- **Web Interface**: Modern, responsive web UI for control and monitoring
- **REST API**: HTTP endpoints for integration with external software
- **WebSocket Support**: Real-time status updates
- **Telnet Interface**: Traditional command-line G-code interface
- **SD Card G-code**: Execute files from microSD card (M20-M30 commands)
- **File Upload**: Upload and execute G-code files via web or SD card
- **WiFi Connectivity**: Access Point or Station mode

## Hardware Requirements

### Board
- **Lolin32 Lite** (ESP32-based development board)
- Dual-core processor for real-time control
- WiFi connectivity
- Sufficient GPIO pins for 6 motors + heaters

### Motors
- 6x DC motors with quadrature encoders (600 PPR recommended)
- Motor drivers compatible with PWM + direction control
- Encoder interface: 2 channels per motor (A and B)

### Heaters
- Hotend heater with thermistor
- Heated bed with thermistor
- MOSFETs or SSRs for heater control

### Optional
- Endstops for homing
- MAX31865 for thermocouple support
- Spindle/laser module
- MicroSD card + adapter for G-code storage (see SD_CARD_WIRING.md)

## Pin Configuration

See `include/config.h` for detailed pin assignments. Default configuration:

### Motor Pins (PWM, DIR, ENC_A, ENC_B, ENABLE)
- X1: 25, 26, 34, 35, 27
- X2: 32, 33, 36, 39, 14
- Y1: 12, 13, 15, 2, 4
- Y2: 16, 17, 5, 18, 19
- Z: 21, 22, 23, 19, 27
- E: 0, 4, 16, 17, 5

### Heater Pins
- Hotend: Output 25, Thermistor 34
- Bed: Output 26, Thermistor 35

### SD Card Pins (SPI)
- CS: GPIO 5
- MOSI: GPIO 23
- MISO: GPIO 19
- CLK: GPIO 18

**Note**: Adjust these pin assignments in `config.h` to match your specific wiring.

## Software Setup

### Prerequisites
- [PlatformIO](https://platformio.org/) installed
- USB cable for programming
- ESP32 board definitions installed

### Installation

1. **Clone or download this project**
   ```bash
   cd encoder3d
   ```

2. **Configure WiFi settings** in `include/config.h`:
   ```cpp
   #define WIFI_SSID "YourNetworkName"
   #define WIFI_PASSWORD "YourPassword"
   #define WIFI_AP_MODE true  // false for station mode
   ```

3. **Adjust pin assignments** in `include/config.h` to match your hardware

4. **Build and upload**
   ```bash
   pio run --target upload
   ```

5. **Monitor serial output**
   ```bash
   pio device monitor
   ```

6. **Upload web files to LittleFS**
   ```bash
   pio run --target uploadfs
   ```

## Usage

### Web Interface

1. Connect to the WiFi network:
   - **AP Mode**: Connect to "CNC_Controller" network
   - **Station Mode**: Find IP address in serial monitor

2. Open web browser to: `http://<IP_ADDRESS>`

3. Features available:
   - Real-time position monitoring
   - Jog controls for manual movement
   - Temperature control and monitoring
   - G-code console
   - File upload and execution
   - Mode switching (3D Printer/CNC/Laser)
   - Emergency stop

### Telnet Interface

Connect using any telnet client:

```bash
telnet <IP_ADDRESS> 23
```

Send G-code commands directly:
```
G28          ; Home all axes
G1 X10 Y10   ; Move to position
M104 S200    ; Set hotend temperature
M105         ; Report temperatures
```

### Serial Interface

Connect via USB and use a serial terminal (115200 baud):
- Send G-code commands
- Monitor system status
- View debug information

## Supported G-codes

### Motion Commands
- `G0/G1` - Linear move
- `G28` - Home axes
- `G90` - Absolute positioning
- `G91` - Relative positioning
- `G92` - Set position

### Temperature Commands
- `M104 S<temp>` - Set hotend temperature
- `M109 S<temp>` - Set hotend temperature and wait
- `M140 S<temp>` - Set bed temperature
- `M190 S<temp>` - Set bed temperature and wait
- `M105` - Report temperatures

### Motor Commands
- `M82` - Absolute extrusion
- `M83` - Relative extrusion
- `M84` - Disable motors

### CNC/Laser Commands
- `M3 S<rpm>` - Spindle on (CW)
- `M4 S<rpm>` - Spindle on (CCW)
- `M5` - Spindle off
- `M106 S<power>` - Laser/fan on
- `M107` - Laser/fan off

### Mode Selection
- `M450` - 3D Printer mode
- `M451` - CNC Spindle mode
- `M452` - Laser Cutter mode

### SD Card Commands
- `M20` - List SD card files
- `M21` - Initialize SD card
- `M22` - Release SD card
- `M23 <filename>` - Select SD file
- `M24` - Start/resume SD print
- `M25` - Pause SD print
- `M27` - Report SD print status
- `M30 <filename>` - Delete SD file

### System Commands
- `M112` - Emergency stop
- `M114` - Report position
- `M119` - Report endstop status

## Configuration

### Motor Tuning

Edit `include/config.h`:

```cpp
// Steps per mm (adjust for your mechanics)
#define STEPS_PER_MM_X 80.0
#define STEPS_PER_MM_Y 80.0
#define STEPS_PER_MM_Z 400.0

// PID tuning for motors
#define MOTOR_PID_KP 2.0
#define MOTOR_PID_KI 0.5
#define MOTOR_PID_KD 0.1
```

### Temperature Tuning

PID autotune recommended, or manually adjust:

```cpp
#define HOTEND_PID_KP 22.2
#define HOTEND_PID_KI 1.08
#define HOTEND_PID_KD 114.0
```

### Machine Limits

```cpp
#define MAX_X 200.0  // mm
#define MAX_Y 200.0
#define MAX_Z 200.0
```

## API Reference

### REST Endpoints

- `GET /api/status` - Get complete system status
- `GET /api/position` - Get current position
- `GET /api/temperature` - Get temperature readings
- `POST /api/command?cmd=<gcode>` - Execute G-code command
- `POST /api/upload` - Upload G-code file
- `POST /api/emergency` - Trigger emergency stop

### WebSocket

Connect to `ws://<IP>/ws` for real-time updates.

Send JSON messages:
```json
{
  "type": "gcode",
  "command": "G28"
}
```

Receive status updates:
```json
{
  "position": {"x": 0, "y": 0, "z": 0, "e": 0},
  "temperatures": {
    "hotend": {"current": 25, "target": 0},
    "bed": {"current": 23, "target": 0}
  },
  "moving": false
}
```

## Architecture

### Control Loops

The firmware uses FreeRTOS tasks for real-time control:

- **Motor Control Loop** (1000 Hz): Updates PID controllers, reads encoders, applies motor outputs
- **Heater Control Loop** (10 Hz): Updates temperature PID controllers, safety checks
- **Main Loop**: Handles communication, command parsing, status updates

### File Structure

```
encoder3d/
├── include/
│   ├── config.h              # Hardware and system configuration
│   ├── motor_controller.h    # Motor control and PID
│   ├── heater_controller.h   # Temperature control
│   ├── gcode_parser.h        # G-code interpreter
│   ├── web_server.h          # Web server and API
│   └── telnet_server.h       # Telnet interface
├── src/
│   ├── main.cpp              # Main application
│   ├── motor_controller.cpp
│   ├── heater_controller.cpp
│   ├── gcode_parser.cpp
│   ├── web_server.cpp
│   └── telnet_server.cpp
├── data/www/                 # Web interface files
│   ├── index.html
│   ├── style.css
│   └── app.js
├── platformio.ini            # PlatformIO configuration
└── README.md                 # This file
```

## Safety Features

1. **Thermal Runaway Protection**: Monitors temperature rise/fall rates
2. **Maximum Temperature Limits**: Prevents overheating
3. **Emergency Stop**: Immediately halts all motion and heaters
4. **Position Limits**: Software endstops (configurable)
5. **Watchdog Timer**: ESP32 built-in watchdog
6. **Encoder Fault Detection**: Detects encoder disconnection

## Troubleshooting

### Motors not moving
- Check motor driver connections
- Verify enable pins are configured correctly (active low/high)
- Check encoder connections
- Monitor serial output for encoder counts

### Temperature reading errors
- Verify thermistor connections
- Check ADC pin configuration
- Ensure proper voltage divider (4.7kΩ pullup)
- Test with known good thermistor

### WiFi connection issues
- Verify SSID and password in config.h
- Check WiFi mode (AP vs. Station)
- Monitor serial output for connection status
- Try AP mode if station mode fails

### Web interface not loading
- Ensure LittleFS filesystem is uploaded
- Check serial monitor for IP address
- Verify web server started successfully
- Try different browser

### SD card issues
- See [SD_CARD_WIRING.md](SD_CARD_WIRING.md) for detailed troubleshooting
- Verify card is FAT32 formatted
- Check solder connections
- Ensure 3.3V power supply (NOT 5V)

## Future Enhancements

- [ ] Advanced motion planning (look-ahead, jerk control)
- [x] SD card support for G-code storage ✅
- [ ] Web-based 3D model viewer
- [ ] Slicer integration
- [ ] Mesh bed leveling
- [ ] Multi-language support
- [ ] OTA (Over-The-Air) firmware updates
- [ ] Advanced kinematics (CoreXY, Delta, etc.)

## License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

This means you are free to:
- ✅ Use commercially
- ✅ Modify
- ✅ Distribute
- ✅ Use privately

With the following conditions:
- 📄 Include original license and copyright notice
- ⚖️ No liability or warranty provided

## Contributing

We welcome contributions from the community! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines.

**Quick Guidelines:**
- 🐛 **Bug Reports**: Use GitHub Issues with detailed reproduction steps
- 💡 **Feature Requests**: Describe use case and proposed implementation
- 🔧 **Pull Requests**: 
  - Fork the repository
  - Create a feature branch
  - Follow existing code style
  - Add comments and documentation
  - Test on actual hardware when possible
  - Update README if adding features
- 📝 **Documentation**: Improvements always welcome

**Code of Conduct**: Be respectful and constructive. This is a collaborative project.

## Credits and Acknowledgments

### Project Lead
Encoder3D CNC Controller - An open-source ESP32-based multi-mode CNC controller

### Dependencies and Libraries
This project builds upon excellent open-source libraries:

- **[ArduinoJson](https://github.com/bblanchon/ArduinoJson)** by Benoit Blanchon - JSON parsing and serialization (MIT License)
- **[ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)** by me-no-dev - Async HTTP and WebSocket server (LGPL-3.0)
- **[AsyncTCP](https://github.com/me-no-dev/AsyncTCP)** by me-no-dev - Asynchronous TCP library (LGPL-3.0)
- **[Encoder](https://github.com/PaulStoffregen/Encoder)** by Paul Stoffregen (PJRC) - Quadrature encoder library (MIT License)
- **[PlatformIO](https://platformio.org/)** - Development platform (Apache-2.0)
- **[Arduino-ESP32](https://github.com/espressif/arduino-esp32)** by Espressif - Arduino core for ESP32 (LGPL-2.1)

### Inspiration
Inspired by open-source CNC and 3D printer controller projects:
- Marlin Firmware
- GRBL
- RepRap project

### Hardware Platform
- **ESP32** by Espressif Systems
- **Lolin32 Lite** development board

### Community
Thanks to all contributors, testers, and users who help improve this project.

## Support and Community

### Documentation
- 📖 [README.md](README.md) - Complete project documentation
- 🔌 [WIRING.md](WIRING.md) - Hardware wiring guide
- 💾 [SD_CARD_WIRING.md](SD_CARD_WIRING.md) - SD card installation guide
- 🚀 [QUICKSTART.md](QUICKSTART.md) - Quick start guide
- 🏗️ [ARCHITECTURE.md](ARCHITECTURE.md) - System architecture and design
- 🤝 [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
- 📜 [LICENSE](LICENSE) - MIT License text
- 🏛️ [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md) - Third-party library licenses and attributions

### Getting Help
1. **Check Documentation**: Most questions are answered in the docs
2. **Search Issues**: Someone may have had the same problem
3. **GitHub Issues**: For bug reports and feature requests
4. **Discussions**: For questions and general discussion

### Reporting Issues
When reporting bugs, please include:
- Hardware configuration (board, motors, drivers)
- Firmware version/commit
- Steps to reproduce
- Expected vs actual behavior
- Serial output or logs
- Photos/videos if relevant

---

**Warning**: This is experimental firmware for CNC machinery. Always:
- Test thoroughly before use
- Have emergency stop readily accessible
- Monitor first runs closely
- Follow all safety precautions for your specific machine

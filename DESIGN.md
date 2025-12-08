# Encoder3D Controller Design Specification

## 1. Hardware Architecture
**Target Platform**: ESP32 Lolin32 Lite
**Kinematics**: 4-Axis (X, Y, Z, E)
**Motor Type**: DC Motors with Quadrature Encoders (6-wire)

### 1.1 Pin Mapping (20 Pins Used)
The design utilizes **ADC1** for thermistors to ensure WiFi stability and avoids strapping pins for critical motor outputs.

| Subsystem | Function | Pin A | Pin B | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **X-Axis** | Motor Drive | GPIO 4 | GPIO 13 | |
| | Encoder Input | GPIO 34 | GPIO 35 | Input-Only Pins |
| **Y-Axis** | Motor Drive | GPIO 12 | GPIO 14 | GPIO 12 safe (internal pulldown) |
| | Encoder Input | GPIO 36 | GPIO 39 | Input-Only Pins |
| **Z-Axis** | Motor Drive | GPIO 27 | GPIO 5 | Moved from 15/16 to avoid boot conflicts |
| | Encoder Input | GPIO 19 | GPIO 21 | |
| **E-Axis** | Motor Drive | GPIO 17 | GPIO 18 | |
| | Encoder Input | GPIO 22 | GPIO 23 | |
| **Thermal** | Extruder Heater | GPIO 25 | - | PWM Output |
| | Bed Heater | GPIO 26 | - | PWM Output |
| | Extruder Sensor | GPIO 32 | - | ADC1_CH4 |
| | Bed Sensor | GPIO 33 | - | ADC1_CH5 |

**Spares**: GPIO 0 (Boot), GPIO 2 (LED), GPIO 15, GPIO 16.

---

## 2. Motor Control Strategy: "One Channel Switched"
To conserve hardware PWM resources, the system uses a dynamic attachment strategy. Each motor is assigned **one** LEDC channel. The channel is dynamically attached to Pin A or Pin B depending on direction, while the opposing pin is grounded.

### 2.1 Logic Flow
**Function**: `setMotorSpeed(axis, speed)`
1.  **Direction Check**: Determine if `speed` is positive (Forward) or negative (Reverse).
2.  **Forward State**:
    *   `ledcDetach(PinB)`
    *   `digitalWrite(PinB, LOW)` (Ground Low Side)
    *   `ledcAttach(PinA, Channel)` (PWM High Side)
    *   `ledcWrite(Channel, abs(speed))`
3.  **Reverse State**:
    *   `ledcDetach(PinA)`
    *   `digitalWrite(PinA, LOW)` (Ground Low Side)
    *   `ledcAttach(PinB, Channel)` (PWM High Side)
    *   `ledcWrite(Channel, abs(speed))`

---

## 3. Software Architecture (RTOS)
The system is divided across the two cores of the ESP32 to ensure real-time motor control is not interrupted by network traffic.

### 3.1 Core 0: Communications & Thermal Manager
**Primary Responsibility**: User Interaction and Slow Control Loops.

1.  **Network Task**:
    *   **Telnet Server (Port 23)**: Accepts raw G-Code streams.
    *   **Web Server (Port 80)**: Serves UI, accepts WebSocket commands.
    *   **Action**: Pushes received data into `GCodeStream` (FreeRTOS StreamBuffer).

2.  **Thermal Task (10Hz)**:
    *   **Input**: Reads ADC values from GPIO 32 & 33.
    *   **Process**: Converts voltage to temperature (Steinhart-Hart).
    *   **Control**:
        *   Extruder: PID Loop.
        *   Bed: Bang-Bang Control (Hysteresis).
    *   **Output**: Writes PWM duty cycle to GPIO 25 & 26.
    *   **Safety**: Monitors for `MAX_TEMP` and `MINTEMP` faults.

### 3.2 Core 1: Real-Time Motion Control
**Primary Responsibility**: High-speed signal processing and trajectory execution.

1.  **Parser Task**:
    *   **Input**: Blocks waiting for data from `GCodeStream`.
    *   **Process**: Parses G-Code (G0, G1, M-codes). Calculates target positions.
    *   **Output**: Pushes `MotionCommand` structs into `MotionQueue`.

2.  **Control Loop (1kHz High Priority)**:
    *   **Input**: Reads Quadrature Encoders (ISR based counting).
    *   **Process**:
        *   Retrieves current target from `MotionQueue`.
        *   Calculates Position Error.
        *   Runs PID Algorithm: `Output = (Kp * Error) + (Ki * Integral) + (Kd * Derivative)`.
    *   **Output**: Calls `setMotorSpeed()` to drive H-Bridges.

### 3.3 Inter-Process Communication (IPC)
*   **GCodeStream**: `StreamBuffer` connecting Network Task -> Parser Task. Handles variable length strings.
*   **MotionQueue**: `Queue` connecting Parser Task -> Control Loop. Stores fixed-size movement commands.

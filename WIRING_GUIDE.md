# Encoder3D Wiring Guide (Lolin32 Lite + L298N)

## ✅ Configuration Fixed: 4-Axis Mode (Full Quadrature)

Per your instructions, we have simplified the design to **4 Axes (X, Y, Z, E)**.
This allows us to use **Full Quadrature Encoders (A+B)** for all motors and fit everything onto just **2 L298N Drivers** and the Lolin32 Lite.

### Key Concepts:
1.  **4 Motors**: X, Y, Z, E. (Dual X/Y motors must be wired in parallel to the single driver output if physically present).
2.  **2 Drivers**:
    *   **Driver 1**: Controls X and Y.
    *   **Driver 2**: Controls Z and E.
3.  **Full Encoders**: All motors use both A and B channels for maximum precision.
4.  **2-Pin Drive**: We use the efficient IN1/IN2 drive method (Enable hardwired to 5V).

---

## Wiring Diagram

See `pcb/wiring_diagram.svg` for the visual layout.

## Pinout Reference (Updated)

| Component | Function | GPIO Pin | Notes |
|-----------|----------|----------|-------|
| **X Axis**| IN1      | **22**   | Driver 1 IN1 |
|           | IN2      | **21**   | Driver 1 IN2 |
|           | Enc A    | 34       | Input Only |
|           | Enc B    | 35       | Input Only |
| **Y Axis**| IN1      | **19**   | Driver 1 IN3 |
|           | IN2      | **23**   | Driver 1 IN4 |
|           | Enc A    | 36       | Input Only |
|           | Enc B    | 39       | Input Only |
| **Z Axis**| IN1      | **18**   | Driver 2 IN1 |
|           | IN2      | **5**    | Driver 2 IN2 |
|           | Enc A    | 25       | |
|           | Enc B    | 26       | |
| **E Axis**| IN1      | **17**   | Driver 2 IN3 |
|           | IN2      | **16**   | Driver 2 IN4 |
|           | Enc A    | 27       | |
|           | Enc B    | 14       | |
| **Heaters**| Hotend PWM | **4** | MOSFET Gate |
|           | Bed PWM    | **13** | MOSFET Gate |
|           | Hotend Temp| 32     | Thermistor (ADC1) |
|           | Bed Temp   | 33     | Thermistor (ADC1) |

### L298N Driver Connections

**Driver 1 (X & Y)**
- **ENA**: 5V (Jumper)
- **IN1**: GPIO 22
- **IN2**: GPIO 21
- **ENB**: 5V (Jumper)
- **IN3**: GPIO 19
- **IN4**: GPIO 23
- **OUT1/2**: X Motor
- **OUT3/4**: Y Motor

**Driver 2 (Z & E)**
- **ENA**: 5V (Jumper)
- **IN1**: GPIO 18
- **IN2**: GPIO 5
- **ENB**: 5V (Jumper)
- **IN3**: GPIO 17
- **IN4**: GPIO 16
- **OUT1/2**: Z Motor
- **OUT3/4**: E Motor


# Encoder3D Master Wiring List

**Date:** December 7, 2025
**Configuration:** 4-Axis (X, Y, Z, E), Full Quadrature Encoders, 2-Pin Drive (Bi-Directional PWM).

---

## 1. ESP32 Lolin32 Lite Pinout (Left Side)

| Pin Label | GPIO | Function | Connect To | Wire Color |
| :--- | :--- | :--- | :--- | :--- |
| **3V3** | - | Logic Power | Encoders (VCC) | Red |
| **GND** | - | Ground | Common Ground | Black |
| **22** | 22 | X Motor IN1 | Driver 1 (IN1) | Purple |
| **21** | 21 | X Motor IN2 | Driver 1 (IN2) | Orange |
| **17** | 17 | E Motor IN1 | Driver 2 (IN3) | Purple |
| **16** | 16 | E Motor IN2 | Driver 2 (IN4) | Orange |
| **4** | 4 | Hotend PWM | Hotend MOSFET (SIG) | Red |
| **0** | 0 | *Boot* | *Do Not Connect* | - |
| **2** | 2 | *LED* | *Do Not Connect* | - |
| **15** | 15 | *Unused* | - | - |
| **13** | 13 | Bed PWM | Bed MOSFET (SIG) | Red |
| **12** | 12 | *Unused* | - | - |
| **14** | 14 | E Encoder B | E Motor (Enc B) | White |
| **27** | 27 | E Encoder A | E Motor (Enc A) | Green |
| **26** | 26 | Z Encoder B | Z Motor (Enc B) | White |
| **25** | 25 | Z Encoder A | Z Motor (Enc A) | Green |
| **33** | 33 | Bed Temp | Bed Thermistor | Yellow |
| **32** | 32 | Hotend Temp | Hotend Thermistor | Yellow |

## 2. ESP32 Lolin32 Lite Pinout (Right Side)

| Pin Label | GPIO | Function | Connect To | Wire Color |
| :--- | :--- | :--- | :--- | :--- |
| **35** | 35 | X Encoder B | X Motor (Enc B) | White |
| **34** | 34 | X Encoder A | X Motor (Enc A) | Green |
| **39** | 39 | Y Encoder B | Y Motor (Enc B) | White |
| **36** | 36 | Y Encoder A | Y Motor (Enc A) | Green |
| **19** | 19 | Y Motor IN1 | Driver 1 (IN3) | Purple |
| **23** | 23 | Y Motor IN2 | Driver 1 (IN4) | Orange |
| **18** | 18 | Z Motor IN1 | Driver 2 (IN1) | Purple |
| **5** | 5 | Z Motor IN2 | Driver 2 (IN2) | Orange |

---

## 3. Motor Driver 1 (X & Y Axis)
*Type: L298N Dual H-Bridge*

| Terminal | Function | Connect To | Notes |
| :--- | :--- | :--- | :--- |
| **12V/24V** | Power In | PSU V+ | Main Motor Power |
| **GND** | Ground | PSU V- / Common GND | |
| **5V** | Logic Power | *See Note* | If <12V in, output 5V. If >12V, input 5V. |
| **ENA** | Enable A | **5V** | **MUST JUMPER TO 5V** |
| **IN1** | Input 1 | ESP32 GPIO **22** | X Axis Control |
| **IN2** | Input 2 | ESP32 GPIO **21** | X Axis Control |
| **IN3** | Input 3 | ESP32 GPIO **19** | Y Axis Control |
| **IN4** | Input 4 | ESP32 GPIO **23** | Y Axis Control |
| **ENB** | Enable B | **5V** | **MUST JUMPER TO 5V** |
| **OUT1** | Output A1 | X Motor (**M+**) | |
| **OUT2** | Output A2 | X Motor (**M-**) | |
| **OUT3** | Output B1 | Y Motor (**M+**) | |
| **OUT4** | Output B2 | Y Motor (**M-**) | |

## 4. Motor Driver 2 (Z & E Axis)
*Type: L298N Dual H-Bridge*

| Terminal | Function | Connect To | Notes |
| :--- | :--- | :--- | :--- |
| **12V/24V** | Power In | PSU V+ | Main Motor Power |
| **GND** | Ground | PSU V- / Common GND | |
| **5V** | Logic Power | *See Note* | |
| **ENA** | Enable A | **5V** | **MUST JUMPER TO 5V** |
| **IN1** | Input 1 | ESP32 GPIO **18** | Z Axis Control |
| **IN2** | Input 2 | ESP32 GPIO **5** | Z Axis Control |
| **IN3** | Input 3 | ESP32 GPIO **17** | E Axis Control |
| **IN4** | Input 4 | ESP32 GPIO **16** | E Axis Control |
| **ENB** | Enable B | **5V** | **MUST JUMPER TO 5V** |
| **OUT1** | Output A1 | Z Motor (**M+**) | |
| **OUT2** | Output A2 | Z Motor (**M-**) | |
| **OUT3** | Output B1 | E Motor (**M+**) | |
| **OUT4** | Output B2 | E Motor (**M-**) | |

---

## 5. Motor Wiring (6-Pin Connector)

**Repeat for all 4 Motors (X, Y, Z, E)**

| Pin # | Function | Connect To |
| :--- | :--- | :--- |
| **1** | Motor + | Driver OUT1 (or OUT3) |
| **2** | Motor - | Driver OUT2 (or OUT4) |
| **3** | GND | Common Ground |
| **4** | 5V | 5V Logic Power |
| **5** | Encoder A | ESP32 Input (See Pinout) |
| **6** | Encoder B | ESP32 Input (See Pinout) |

---

## 6. Heater & Thermistor Wiring

### Hotend
1.  **Heater Wires**: Connect to **Hotend MOSFET** Output Terminals.
2.  **MOSFET Signal**: Connect to ESP32 GPIO **4**.
3.  **Thermistor**: Connect one leg to **GND**, other leg to ESP32 GPIO **32** (with 4.7k Pullup to 3.3V).

### Heated Bed
1.  **Heater Wires**: Connect to **Bed MOSFET** Output Terminals.
2.  **MOSFET Signal**: Connect to ESP32 GPIO **13**.
3.  **Thermistor**: Connect one leg to **GND**, other leg to ESP32 GPIO **33** (with 4.7k Pullup to 3.3V).

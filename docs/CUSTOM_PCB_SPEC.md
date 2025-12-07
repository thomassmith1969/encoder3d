# Encoder3D Custom PCB Design Specification

## Overview

Custom PCB to integrate the Encoder3D ESP32 controller with L298N motor drivers, MOSFET-based heater controls, and expansion capabilities. This design consolidates power distribution, motor control, and safety features on a single board.

---

## Board Specifications

**Dimensions:** 150mm × 120mm (5.9" × 4.7")  
**Layers:** 2-layer PCB  
**Copper Weight:** 2oz (70μm) for high-current traces  
**Finish:** HASL or ENIG  
**Soldermask:** Green (or black for professional look)  
**Silkscreen:** White component labels and warnings

---

## Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         POWER INPUT                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │ 24V MAIN │  │ 12V AUX  │  │  5V USB  │  │ 3.3V REG │       │
│  │  XT60    │  │  Barrel  │  │USB-C/µUSB│  │ AMS1117  │       │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘       │
└───────┼─────────────┼─────────────┼─────────────┼──────────────┘
        │             │             │             │
        │             │             │             └──→ ESP32 3.3V
        │             │             └────────────────→ ESP32 5V (via USB)
        │             └──────────────────────────────→ Fans, Aux
        │
        ├──→ L298N Module 1 (X-axis motors)
        ├──→ L298N Module 2 (Y-axis motors)
        ├──→ L298N Module 3 (Z + E motors)
        ├──→ Hotend MOSFET (IRF540N or IRLZ44N)
        └──→ Bed Heater MOSFET (IRF540N or IRLZ44N)

┌─────────────────────────────────────────────────────────────────┐
│                      ESP32 DevKit v1                            │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  GPIO Breakout:                                          │  │
│  │  - Motor Step/Dir/Enable pins (18 pins)                 │  │
│  │  - Encoder inputs (12 pins)                             │  │
│  │  - Heater PWM (2 pins)                                   │  │
│  │  - Thermistor ADC (2-3 pins)                            │  │
│  │  - Endstop inputs (6 pins)                              │  │
│  │  - Expansion header (remaining GPIO)                    │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                    OUTPUT CONNECTORS                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │ Motors   │  │ Encoders │  │ Heaters  │  │Endstops  │       │
│  │ JST-XH4  │  │ JST-XH4  │  │XT60/Screw│  │ JST-XH3  │       │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘       │
└─────────────────────────────────────────────────────────────────┘
```

---

## Detailed Component Layout

### 1. Power Input Section (Top Left)

**Main Power Input (24V):**
```
┌─────────────────────────────────────┐
│  XT60 Connector (24V Main)          │
│  ┌─────┐                            │
│  │ (+) │───→ Fuse 15A ──→ Power LED │
│  │ (-) │───→ Common Ground          │
│  └─────┘                            │
│                                     │
│  Polarity Protection: P-Channel     │
│  MOSFET (reverse polarity)          │
│  OR: Schottky diode (10A)           │
│                                     │
│  Bulk Capacitor: 1000µF/35V         │
│  Ceramic: 100nF (×4 distributed)    │
└─────────────────────────────────────┘
```

**12V Auxiliary (Optional - for fans):**
```
┌─────────────────────────────────────┐
│  DC Barrel Jack (12V, 2A)           │
│  Or LM2596 Buck Converter           │
│  (24V → 12V stepdown)               │
│                                     │
│  Output: JST-XH2 connector          │
│  For: Cooling fans, auxiliary       │
└─────────────────────────────────────┘
```

**5V Regulator (ESP32 Power):**
```
┌─────────────────────────────────────┐
│  LM2596S Buck Module (24V→5V, 3A)   │
│  OR: Discrete design with           │
│      MP2315 or similar              │
│                                     │
│  Input: 24V from main               │
│  Output: 5V, 3A                     │
│  → USB Micro/Type-C for ESP32       │
│                                     │
│  OR: AMS1117-5.0 (if <1A needed)    │
└─────────────────────────────────────┘
```

**3.3V Regulator (Logic Level):**
```
┌─────────────────────────────────────┐
│  AMS1117-3.3 SOT-223                │
│  Input: 5V                          │
│  Output: 3.3V, 1A                   │
│  → ESP32 3.3V pin (backup)          │
│                                     │
│  Caps: 10µF in, 22µF out            │
└─────────────────────────────────────┘
```

### 2. ESP32 Socket (Center)

```
┌─────────────────────────────────────────────┐
│  ESP32 DevKit v1 Socket                     │
│  ┌────────────────────────────────────┐     │
│  │  2×19 pin female header             │     │
│  │  2.54mm pitch                       │     │
│  │                                     │     │
│  │  Pinout clearly labeled on silk    │     │
│  │  All GPIO broken out to headers    │     │
│  │                                     │     │
│  │  USB port accessible from edge     │     │
│  └────────────────────────────────────┘     │
│                                             │
│  Supporting components:                    │
│  - 100nF decoupling caps (×8)              │
│  - 10µF bulk cap near power pins           │
│  - Reset button (optional)                 │
│  - Boot mode jumper (optional)             │
└─────────────────────────────────────────────┘
```

### 3. Motor Driver Section (Right Side)

**Three L298N Modules (using L298N breakout boards):**

```
┌─────────────────────────────────────────────────────────────┐
│  L298N MODULE 1 - X-AXIS (Dual Motors)                      │
│  ┌────────────────────────────────────────────────────┐     │
│  │  L298N Breakout Board (Red PCB)                    │     │
│  │  - VCC: 24V from main power                       │     │
│  │  - GND: Common ground                             │     │
│  │  - ENA: ESP32 GPIO13 (X1 Step - actually PWM)    │     │
│  │  - IN1: ESP32 GPIO12 (X1 Dir)                    │     │
│  │  - IN2: ESP32 GPIO14 (X1 Enable)                 │     │
│  │  - ENB: ESP32 GPIO26 (X2 Step - actually PWM)    │     │
│  │  - IN3: ESP32 GPIO25 (X2 Dir)                    │     │
│  │  - IN4: ESP32 GPIO33 (X2 Enable)                 │     │
│  │                                                    │     │
│  │  Output:                                          │     │
│  │  - OUT1/OUT2: Motor X1 (JST-XH4)                 │     │
│  │  - OUT3/OUT4: Motor X2 (JST-XH4)                 │     │
│  └────────────────────────────────────────────────────┘     │
│                                                             │
│  Heatsink: Required, 40×40mm aluminum                      │
│  Cooling: 12V fan optional                                  │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│  L298N MODULE 2 - Y-AXIS (Dual Motors)                      │
│  ┌────────────────────────────────────────────────────┐     │
│  │  L298N Breakout Board                              │     │
│  │  - VCC: 24V                                        │     │
│  │  - ENA: ESP32 GPIO4  (Y1 Step)                    │     │
│  │  - IN1: ESP32 GPIO16 (Y1 Dir)                     │     │
│  │  - IN2: ESP32 GPIO17 (Y1 Enable)                  │     │
│  │  - ENB: ESP32 GPIO18 (Y2 Step)                    │     │
│  │  - IN3: ESP32 GPIO19 (Y2 Dir)                     │     │
│  │  - IN4: ESP32 GPIO21 (Y2 Enable)                  │     │
│  │                                                    │     │
│  │  Output:                                          │     │
│  │  - OUT1/OUT2: Motor Y1 (JST-XH4)                 │     │
│  │  - OUT3/OUT4: Motor Y2 (JST-XH4)                 │     │
│  └────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│  L298N MODULE 3 - Z-AXIS + EXTRUDER                         │
│  ┌────────────────────────────────────────────────────┐     │
│  │  L298N Breakout Board                              │     │
│  │  - VCC: 24V                                        │     │
│  │  - ENA: ESP32 GPIO23 (Z Step)                     │     │
│  │  - IN1: ESP32 GPIO15 (Z Dir)                      │     │
│  │  - IN2: ESP32 GPIO2  (Z Enable)                   │     │
│  │  - ENB: ESP32 GPIO22 (E Step)                     │     │
│  │  - IN3: ESP32 GPIO1  (E Dir - TX pin, careful!)  │     │
│  │  - IN4: ESP32 GPIO3  (E Enable - RX, careful!)   │     │
│  │                                                    │     │
│  │  Output:                                          │     │
│  │  - OUT1/OUT2: Motor Z  (JST-XH4)                 │     │
│  │  - OUT3/OUT4: Motor E  (JST-XH4)                 │     │
│  └────────────────────────────────────────────────────┘     │
│                                                             │
│  NOTE: TX/RX pins require boot mode consideration          │
└─────────────────────────────────────────────────────────────┘
```

**Alternative: Discrete L298N Design (if using chips directly):**

```
Each L298N chip footprint:
┌────────────────────────────────┐
│  L298N (Multiwatt15 package)   │
│  - Heat sink required          │
│  - Flyback diodes (built-in)   │
│  - Sense resistors (0.5Ω 2W)   │
│  - Input caps (100nF ceramic)  │
│  - Decoupling caps (100µF)     │
│                                │
│  Screw terminals for motors   │
│  JST-XH4 alternative           │
└────────────────────────────────┘
```

### 4. Heater MOSFET Section (Bottom)

**Hotend Heater MOSFET:**
```
┌─────────────────────────────────────────────────────────────┐
│  HOTEND MOSFET CIRCUIT                                      │
│  ┌────────────────────────────────────────────────────┐     │
│  │  Q1: IRF540N or IRLZ44N (N-channel MOSFET)        │     │
│  │     TO-220 package with heatsink                  │     │
│  │                                                    │     │
│  │  Gate ←─ R1 (10kΩ pulldown) ─→ GND              │     │
│  │  Gate ←─ R2 (220Ω) ←─ ESP32 GPIO23 (PWM)        │     │
│  │                                                    │     │
│  │  Drain ←─ 24V Main Power                         │     │
│  │  Source → Hotend + (screw terminal)              │     │
│  │           Hotend - → GND                          │     │
│  │                                                    │     │
│  │  Protection:                                      │     │
│  │  - Flyback diode: 1N5822 (3A Schottky)           │     │
│  │  - Fuse: 5A inline                                │     │
│  │                                                    │     │
│  │  LED Indicator: Red LED + 1kΩ resistor           │     │
│  │  (shows when heater is ON)                        │     │
│  │                                                    │     │
│  │  Output: Screw terminal 5mm pitch                │     │
│  │  Rating: 24V @ 40W (1.7A max)                    │     │
│  └────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

**Bed Heater MOSFET (High Current):**
```
┌─────────────────────────────────────────────────────────────┐
│  BED HEATER MOSFET CIRCUIT                                  │
│  ┌────────────────────────────────────────────────────┐     │
│  │  Q2: IRF540N (N-channel MOSFET)                   │     │
│  │     TO-220 with LARGE heatsink                    │     │
│  │     OR: Parallel 2× IRF540N for high current      │     │
│  │                                                    │     │
│  │  Gate ←─ R3 (10kΩ pulldown) ─→ GND              │     │
│  │  Gate ←─ R4 (220Ω) ←─ ESP32 GPIO22 (PWM)        │     │
│  │                                                    │     │
│  │  Drain ←─ 24V Main Power (thick trace!)          │     │
│  │  Source → Bed Heater + (XT60 or screw)           │     │
│  │           Bed Heater - → GND                      │     │
│  │                                                    │     │
│  │  Protection:                                      │     │
│  │  - Flyback diode: 1N5822 or 10A Schottky         │     │
│  │  - Fuse: 15A automotive blade fuse                │     │
│  │  - Thermal fuse: 150°C on heatsink (optional)    │     │
│  │                                                    │     │
│  │  LED Indicator: Red LED + 1kΩ resistor           │     │
│  │                                                    │     │
│  │  Output: XT60 connector OR                        │     │
│  │          Heavy-duty screw terminal (10A rated)   │     │
│  │  Rating: 24V @ 200W (8.3A max)                   │     │
│  └────────────────────────────────────────────────────┘     │
│                                                             │
│  CRITICAL: Use 2oz copper, minimum 3mm trace width         │
│  Consider external SSR for >200W bed heaters               │
└─────────────────────────────────────────────────────────────┘
```

### 5. Thermistor Input Section (Bottom Right)

```
┌─────────────────────────────────────────────────────────────┐
│  THERMISTOR INPUTS (with filtering)                         │
│  ┌────────────────────────────────────────────────────┐     │
│  │  HOTEND THERMISTOR (NTC 100K)                      │     │
│  │  ┌──────────────────────────┐                     │     │
│  │  │  3.3V ──┬── R5 (4.7kΩ) ──┬── ESP32 GPIO36     │     │
│  │  │         │                 │    (ADC1_0)        │     │
│  │  │         │   Thermistor    │                    │     │
│  │  │         └─────┤├─────────┴── GND              │     │
│  │  │                                                │     │
│  │  │  Filter: C1 (100nF) parallel to thermistor   │     │
│  │  │  Connector: JST-XH2                           │     │
│  │  └──────────────────────────┘                     │     │
│  │                                                    │     │
│  │  BED THERMISTOR (NTC 100K)                        │     │
│  │  ┌──────────────────────────┐                     │     │
│  │  │  3.3V ──┬── R6 (4.7kΩ) ──┬── ESP32 GPIO39     │     │
│  │  │         │                 │    (ADC1_3)        │     │
│  │  │         │   Thermistor    │                    │     │
│  │  │         └─────┤├─────────┴── GND              │     │
│  │  │                                                │     │
│  │  │  Filter: C2 (100nF)                           │     │
│  │  │  Connector: JST-XH2                           │     │
│  │  └──────────────────────────┘                     │     │
│  │                                                    │     │
│  │  Optional: Chamber thermistor on GPIO34           │     │
│  └────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

### 6. Encoder Input Section (Left Side)

```
┌─────────────────────────────────────────────────────────────┐
│  ENCODER INPUTS (Quadrature, 5V tolerant with protection)  │
│  ┌────────────────────────────────────────────────────┐     │
│  │  For each encoder (×6):                            │     │
│  │  ┌──────────────────────────────┐                 │     │
│  │  │  Encoder A ──┬── R (1kΩ) ──┬── ESP32 GPIO     │     │
│  │  │              │              │                   │     │
│  │  │              │   Zener 3.6V │                   │     │
│  │  │              └──────┤├──────┴── GND            │     │
│  │  │                                                 │     │
│  │  │  Encoder B ──┬── R (1kΩ) ──┬── ESP32 GPIO     │     │
│  │  │              │              │                   │     │
│  │  │              │   Zener 3.6V │                   │     │
│  │  │              └──────┤├──────┴── GND            │     │
│  │  │                                                 │     │
│  │  │  +5V from encoder                              │     │
│  │  │  GND                                            │     │
│  │  │                                                 │     │
│  │  │  Connector: JST-XH4 (A, B, +5V, GND)          │     │
│  │  └──────────────────────────────┘                 │     │
│  │                                                    │     │
│  │  Encoder Connectors (×6):                         │     │
│  │  - X1: GPIO36, GPIO39                             │     │
│  │  - X2: GPIO34, GPIO35                             │     │
│  │  - Y1: GPIO32, GPIO33                             │     │
│  │  - Y2: GPIO25, GPIO26                             │     │
│  │  - Z:  GPIO27, GPIO14                             │     │
│  │  - E:  GPIO12, GPIO13                             │     │
│  └────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

### 7. Endstop Section (Top Right)

```
┌─────────────────────────────────────────────────────────────┐
│  ENDSTOP INPUTS (with debounce)                             │
│  ┌────────────────────────────────────────────────────┐     │
│  │  For each endstop (×6):                            │     │
│  │  ┌──────────────────────────────┐                 │     │
│  │  │  3.3V ──┬── R (10kΩ) ──┬── ESP32 GPIO         │     │
│  │  │         │               │                       │     │
│  │  │         │    Switch     │                       │     │
│  │  │         └──────/ ───────┴── GND                │     │
│  │  │         (NC or NO)                              │     │
│  │  │                                                 │     │
│  │  │  Debounce: C (100nF) parallel                  │     │
│  │  │  LED: Optional indicator                       │     │
│  │  │  Connector: JST-XH3 (Signal, +3.3V, GND)      │     │
│  │  └──────────────────────────────┘                 │     │
│  │                                                    │     │
│  │  Endstop Pins:                                    │     │
│  │  - X_MIN: GPIO36  X_MAX: GPIO39                   │     │
│  │  - Y_MIN: GPIO34  Y_MAX: GPIO35                   │     │
│  │  - Z_MIN: GPIO32  Z_MAX: GPIO33                   │     │
│  └────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

### 8. Expansion Headers (Bottom Right)

```
┌─────────────────────────────────────────────────────────────┐
│  EXPANSION / FUTURE USE                                     │
│  ┌────────────────────────────────────────────────────┐     │
│  │  EXPANSION HEADER 1 (I2C/SPI)                      │     │
│  │  ┌──────────────────────────────┐                 │     │
│  │  │  Pin 1: 3.3V                 │                 │     │
│  │  │  Pin 2: GND                  │                 │     │
│  │  │  Pin 3: GPIO21 (SDA/I2C)     │                 │     │
│  │  │  Pin 4: GPIO22 (SCL/I2C)     │                 │     │
│  │  │  Pin 5: GPIO19 (MISO/SPI)    │                 │     │
│  │  │  Pin 6: GPIO23 (MOSI/SPI)    │                 │     │
│  │  │  Pin 7: GPIO18 (SCK/SPI)     │                 │     │
│  │  │  Pin 8: GPIO5  (CS/SPI)      │                 │     │
│  │  │  Connector: 2×4 pin header   │                 │     │
│  │  └──────────────────────────────┘                 │     │
│  │                                                    │     │
│  │  EXPANSION HEADER 2 (General GPIO)                │     │
│  │  ┌──────────────────────────────┐                 │     │
│  │  │  8× spare GPIO pins           │                 │     │
│  │  │  With 3.3V and GND           │                 │     │
│  │  │  For:                         │                 │     │
│  │  │  - Additional sensors         │                 │     │
│  │  │  - RGB LEDs                   │                 │     │
│  │  │  - Laser control              │                 │     │
│  │  │  - Spindle control            │                 │     │
│  │  │  - Extra motors (TMC drivers) │                 │     │
│  │  │  Connector: 2×6 pin header    │                 │     │
│  │  └──────────────────────────────┘                 │     │
│  │                                                    │     │
│  │  UART HEADER (for external modules)               │     │
│  │  ┌──────────────────────────────┐                 │     │
│  │  │  TX, RX, GND, 3.3V            │                 │     │
│  │  │  For: WiFi modules, displays  │                 │     │
│  │  │  Connector: 1×4 pin header    │                 │     │
│  │  └──────────────────────────────┘                 │     │
│  └────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

---

## Component Placement Recommendations

### Top Side Components
```
┌───────────────────────────────────────────────────────┐
│                                                       │
│  Power Input    ESP32       L298N #1    Endstops    │
│  [XT60]         [Socket]    [Module]    [JST×6]     │
│                                                       │
│  Regulators     Encoders    L298N #2    Expansion   │
│  [Buck+Linear]  [JST×6]     [Module]    [Headers]   │
│                                                       │
│  Fuses          Therm.      L298N #3    LEDs        │
│  [Blade×2]      [JST×2]     [Module]    [Status]    │
│                                                       │
│  Hotend FET     Bed FET     Motor Out   Fan Out     │
│  [MOSFET+HS]    [MOSFET+HS] [JST×6]     [JST×3]     │
│                                                       │
└───────────────────────────────────────────────────────┘
```

### Bottom Side Components
- SMD decoupling capacitors (100nF, 10µF)
- Pull-up/pull-down resistors (0805 or 1206 package)
- Small signal components
- Keep bottom relatively clear for mounting

---

## Trace Width Guidelines

### Power Traces (2oz copper):
```
Current | Trace Width (2oz) | Notes
--------|-------------------|---------------------------
1A      | 0.5mm (20mil)     | Logic power, fans
3A      | 1.0mm (40mil)     | Motor phase current
5A      | 2.0mm (80mil)     | Hotend heater
10A     | 3.5mm (140mil)    | Bed heater, main 24V
15A     | 5.0mm (200mil)    | Main power bus

Use trace width calculator for verification.
Consider copper pour for ground plane.
```

### Signal Traces:
```
Type          | Width      | Notes
--------------|------------|---------------------------
GPIO          | 0.25mm     | Default signal traces
Encoder       | 0.3mm      | Keep short, shielded
Thermistor    | 0.25mm     | Low noise routing
USB D+/D-     | 0.25mm     | 90Ω differential pair
```

### Ground Plane:
- Use copper pour on both layers
- Connect with multiple vias (thermal relief for hand soldering)
- Star ground configuration for sensitive analog

---

## Connector Pinouts

### Motor Connectors (JST-XH4):
```
Pin 1: Motor A+
Pin 2: Motor A-
Pin 3: Motor B+
Pin 4: Motor B-
```

### Encoder Connectors (JST-XH4):
```
Pin 1: Encoder A
Pin 2: Encoder B
Pin 3: +5V
Pin 4: GND
```

### Thermistor Connectors (JST-XH2):
```
Pin 1: Signal (to ADC)
Pin 2: GND
```

### Endstop Connectors (JST-XH3):
```
Pin 1: Signal
Pin 2: +3.3V (or +5V)
Pin 3: GND
```

---

## Bill of Materials (BOM)

### Major Components

| Ref | Component | Package | Qty | Est. Cost | Notes |
|-----|-----------|---------|-----|-----------|-------|
| U1 | ESP32 DevKit v1 | Module | 1 | $8 | Main controller |
| U2 | L298N Module #1 | Breakout | 1 | $3 | X-axis motors |
| U3 | L298N Module #2 | Breakout | 1 | $3 | Y-axis motors |
| U4 | L298N Module #3 | Breakout | 1 | $3 | Z + E motors |
| U5 | LM2596S Module | Buck Module | 1 | $2 | 24V→5V, 3A |
| U6 | AMS1117-3.3 | SOT-223 | 1 | $0.50 | 3.3V regulator |
| Q1 | IRF540N | TO-220 | 1 | $1 | Hotend FET |
| Q2 | IRF540N | TO-220 | 2 | $2 | Bed FET (parallel) |
| D1-D3 | 1N5822 | DO-201 | 3 | $1 | Flyback diodes |
| F1 | Blade Fuse 15A | ATC | 1 | $1 | Main power |
| F2 | Blade Fuse 5A | ATC | 1 | $1 | Hotend |

### Passives

| Ref | Component | Package | Qty | Est. Cost |
|-----|-----------|---------|-----|-----------|
| C1-C8 | 100nF Ceramic | 0805 | 20 | $2 |
| C9-C12 | 10µF Ceramic | 1206 | 10 | $3 |
| C13 | 1000µF/35V | Radial | 2 | $2 |
| R1-R20 | 10kΩ | 0805 | 20 | $1 |
| R21-R30 | 1kΩ | 0805 | 10 | $0.50 |
| R31-R34 | 220Ω | 0805 | 10 | $0.50 |
| R35-R36 | 4.7kΩ | 0805 | 2 | $0.10 |

### Connectors

| Ref | Component | Type | Qty | Est. Cost |
|-----|-----------|------|-----|-----------|
| J1 | XT60 Male | Power | 1 | $1 |
| J2-J7 | JST-XH4 | Motor | 6 | $3 |
| J8-J13 | JST-XH4 | Encoder | 6 | $3 |
| J14-J15 | JST-XH2 | Thermistor | 2 | $0.50 |
| J16-J21 | JST-XH3 | Endstop | 6 | $3 |
| J22 | USB Micro/C | SMD | 1 | $1 |
| J23-J24 | Screw Term 5mm | Through-hole | 2 | $1 |
| J25-J27 | Pin Header 2×4 | 2.54mm | 3 | $0.50 |

### Misc

| Item | Qty | Est. Cost | Notes |
|------|-----|-----------|-------|
| Heatsink 40×40mm | 4 | $4 | L298N + MOSFETs |
| Thermal pad | 4 | $2 | For heatsinks |
| Standoffs M3×10mm | 4 | $1 | PCB mounting |
| Status LEDs | 5 | $1 | Power, heaters |
| Fuse holders | 2 | $2 | Blade fuse type |

### **Total Estimated Cost: ~$60-80** (excluding PCB fabrication)

---

## PCB Fabrication Notes

### Design Rules (JLCPCB/PCBWay standard):
```
Minimum track width:     0.15mm (6mil)
Minimum clearance:       0.15mm (6mil)
Minimum drill:           0.3mm
Minimum annular ring:    0.15mm
Solder mask clearance:   0.05mm
Silkscreen line width:   0.15mm
```

### Recommended PCB Specs:
```
Layers:           2
Dimensions:       150mm × 120mm
Thickness:        1.6mm
Copper weight:    2oz (70μm) - IMPORTANT for high current!
Surface finish:   HASL or ENIG
Soldermask:       Green or Black
Silkscreen:       White
Min. hole size:   0.3mm
Castellated:      No
```

### Cost Estimate:
- 5× PCBs from JLCPCB: ~$15-25 (2oz copper)
- Shipping: ~$10-20
- **Total: ~$25-45 for 5 boards**

---

## Safety Features

### Over-Current Protection:
1. **Main Fuse**: 15A blade fuse on 24V input
2. **Hotend Fuse**: 5A inline fuse
3. **Bed current**: Monitor via firmware (optional shunt resistor)

### Over-Voltage Protection:
1. **Reverse polarity**: P-channel MOSFET or diode
2. **Zener clamps**: 3.6V on all encoder inputs
3. **TVS diodes**: Optional on power input

### Thermal Protection:
1. **Heatsinks**: Required on L298N and MOSFETs
2. **Thermal fuse**: Optional 150°C on bed MOSFET heatsink
3. **Firmware**: Thermal runaway detection

### Visual Indicators:
1. **Power LED**: Green - main power OK
2. **Hotend LED**: Red - when heating
3. **Bed LED**: Red - when heating
4. **Status LED**: Blue - ESP32 activity
5. **Error LED**: Red - fault condition

---

## Assembly Instructions

### Step 1: PCB Inspection
- Check for shorts (continuity test)
- Verify power traces are intact
- Check all connector pads

### Step 2: SMD Components First
- Solder regulators (U6, etc.)
- Solder resistors and capacitors (0805/1206)
- Reflow or hand solder

### Step 3: Through-Hole Components
- Solder connectors (JST, screw terminals)
- Solder fuse holders
- Install pin headers for ESP32

### Step 4: Power Section
- Install XT60 connector
- Solder power MOSFETs (Q1, Q2)
- Add heatsinks with thermal paste
- Install L298N modules (if using breakouts, just add headers)

### Step 5: Testing (NO MOTORS YET!)
1. **Visual inspection**: No solder bridges
2. **Continuity test**: Check power rails
3. **Power on test**: 
   - Connect 24V, check for smoke/heat
   - Measure 5V output
   - Measure 3.3V output
4. **ESP32 test**: 
   - Install ESP32
   - Program with test firmware
   - Check all GPIO outputs

### Step 6: Motor Connection
- Connect motors one at a time
- Test each axis individually
- Verify encoder feedback

---

## Firmware Configuration

Update `include/config.h` for this PCB:

```cpp
// Custom PCB Pin Definitions
// Motor Step/Dir/Enable pins connected to L298N ENA/IN1/IN2

// X-AXIS (L298N Module 1)
#define MOTOR_X1_STEP_PIN 13   // Actually PWM to ENA
#define MOTOR_X1_DIR_PIN 12    // To IN1
#define MOTOR_X1_EN_PIN 14     // To IN2

#define MOTOR_X2_STEP_PIN 26   // To ENB
#define MOTOR_X2_DIR_PIN 25    // To IN3
#define MOTOR_X2_EN_PIN 33     // To IN4

// Y-AXIS (L298N Module 2)
#define MOTOR_Y1_STEP_PIN 4
#define MOTOR_Y1_DIR_PIN 16
#define MOTOR_Y1_EN_PIN 17

#define MOTOR_Y2_STEP_PIN 18
#define MOTOR_Y2_DIR_PIN 19
#define MOTOR_Y2_EN_PIN 21

// Z-AXIS (L298N Module 3)
#define MOTOR_Z_STEP_PIN 23
#define MOTOR_Z_DIR_PIN 15
#define MOTOR_Z_EN_PIN 2

// E-AXIS (Extruder - L298N Module 3)
#define MOTOR_E_STEP_PIN 22
#define MOTOR_E_DIR_PIN 1      // TX pin - be careful!
#define MOTOR_E_EN_PIN 3       // RX pin - be careful!

// Encoders
#define ENCODER_X1_A_PIN 36
#define ENCODER_X1_B_PIN 39
// ... etc

// Heaters
#define HEATER_HOTEND_PIN 23   // PWM to MOSFET gate
#define HEATER_BED_PIN 22      // PWM to MOSFET gate

// Thermistors
#define TEMP_HOTEND_PIN 36     // ADC1_0
#define TEMP_BED_PIN 39        // ADC1_3

// L298N uses PWM for speed control, not step pulses
// Update motor controller to use PWM mode instead of step/dir
```

---

## Expansion Ideas

### Future Enhancements:
1. **TMC2209 Headers**: Replace L298N with silent stepper drivers
2. **SD Card Socket**: Add microSD for G-code storage
3. **Display Header**: I2C connector for OLED/LCD
4. **WiFi Antenna**: External antenna connector
5. **Current Sensing**: Shunt resistors + INA219 modules
6. **Laser Control**: PWM + enable outputs
7. **Spindle Control**: 0-10V DAC output for VFD
8. **Auto Bed Leveling**: BLTouch connector
9. **Filament Sensor**: Dedicated input with LED
10. **RGB LEDs**: WS2812B connector for status

---

## Design Files Checklist

When creating PCB files, include:

- [ ] Schematic (KiCad/Eagle/Altium)
- [ ] PCB Layout with copper pours
- [ ] 3D model for visualization
- [ ] BOM (CSV export)
- [ ] Gerber files for fabrication
- [ ] Pick-and-place file (if using SMD assembly)
- [ ] Assembly drawing (PDF)
- [ ] Wiring diagram (this document)
- [ ] Test procedure document

---

## Known Issues / Limitations

1. **L298N Heat**: L298N modules get HOT under load. Ensure good heatsinking and ventilation.

2. **TX/RX Pins**: Using GPIO1/3 for extruder requires boot mode consideration. May need to disconnect during programming.

3. **Current Limit**: L298N limited to ~2A per motor. For higher current, consider TMC drivers.

4. **Bed Heater**: 200W maximum on-board. For larger beds (>200W), use external SSR.

5. **Encoder 5V**: If encoders provide 5V signals, Zener protection is CRITICAL.

---

This PCB design provides a solid foundation with room to grow. Start with basic functionality, test thoroughly, then expand as needed!

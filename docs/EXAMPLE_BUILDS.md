# Encoder3D Example Implementations

Complete hardware configurations for popular DIY CNC builds using the Encoder3D firmware.

---

## Table of Contents

1. [Micro PCB Etcher (100mm x 100mm)](#1-micro-pcb-etcher)
2. [Medium 3D Printer (610mm / 24" cube)](#2-medium-3d-printer-24x24x24)
3. [Rotary Wood Carver (200mm x 250mm x 100mm)](#3-rotary-wood-carver-8x10x4)
4. [Large Format Laser Cutter (800mm x 800mm)](#4-large-format-laser-cutter-800x800)

---

## 1. Micro PCB Etcher (100mm x 100mm x 50mm)

**Perfect for:** Circuit board etching, PCB drilling, fine engraving

### Hardware Specifications

**Frame:**
- Working area: 100mm x 100mm x 50mm (4" x 4" x 2")
- Frame: 2020 aluminum extrusion or 3D printed
- Linear rails: MGN9H (9mm) or MGN12H (12mm)
- Lead screws: T8 (8mm, 2mm pitch) or ballscrews

**Motors:**
- Type: NEMA 17 with 600 PPR optical encoders
- All axes: 1.8° stepper + encoder
- Gearing: Direct drive on Z, 1:1 on X/Y

**End Effector:**
- 555 DC motor spindle (12V, 3000-12000 RPM)
- ER11 collet adapter (0.5-3.175mm bits)
- Alternative: Dremel 3000 conversion

**Electronics:**
- ESP32: Lolin32 Lite or DevKit v1
- Motor drivers: 6x TMC2208/2209 (silent, stall detection)
- Power: 24V 5A supply
- Spindle: L298N motor driver

**Sensors:**
- Encoders: 6x 600 PPR quadrature (X1, X2, Y1, Y2, Z, E)
- Z-probe: Conductive PCB probe or mechanical probe
- Limit switches: 6x optical endstops

### Wiring Diagram

```
ESP32 Lolin32 Lite Pin Assignment:
┌─────────────────────────────────────────────────────┐
│ MOTORS (via TMC2208/2209 drivers)                   │
├─────────────────────────────────────────────────────┤
│ X1: Step=13, Dir=12, EN=14, UART=27                 │
│ X2: Step=26, Dir=25, EN=33, UART=32                 │
│ Y1: Step=4,  Dir=16, EN=17, UART=5                  │
│ Y2: Step=18, Dir=19, EN=21, UART=22                 │
│ Z:  Step=23, Dir=15, EN=2,  UART=0                  │
│ E:  Step=34, Dir=35, EN=32, UART=33 (not used)      │
├─────────────────────────────────────────────────────┤
│ ENCODERS (quadrature)                               │
├─────────────────────────────────────────────────────┤
│ X1: A=36, B=39                                      │
│ X2: A=34, B=35                                      │
│ Y1: A=25, B=26                                      │
│ Y2: A=32, B=33                                      │
│ Z:  A=27, B=14                                      │
│ E:  A=12, B=13 (optional)                           │
├─────────────────────────────────────────────────────┤
│ SPINDLE                                             │
├─────────────────────────────────────────────────────┤
│ PWM: GPIO15 → L298N Enable A                        │
│ DIR: GPIO2  → L298N Input 1                         │
│ EN:  GPIO4  → L298N Input 2                         │
├─────────────────────────────────────────────────────┤
│ ENDSTOPS                                            │
├─────────────────────────────────────────────────────┤
│ X_MIN: GPIO36, X_MAX: GPIO39                        │
│ Y_MIN: GPIO34, Y_MAX: GPIO35                        │
│ Z_MIN: GPIO32 (Z-probe), Z_MAX: GPIO33              │
└─────────────────────────────────────────────────────┘
```

### Configuration File (config.h)

```cpp
// PCB Etcher Configuration
#define MACHINE_NAME "PCB Etcher 100x100"
#define DEFAULT_MODE MODE_CNC_SPINDLE

// Mechanical specifications
#define STEPS_PER_MM_X 80.0    // 200 steps/rev * 16 microsteps / 40mm/rev (T8 lead screw)
#define STEPS_PER_MM_Y 80.0
#define STEPS_PER_MM_Z 400.0   // Finer Z for precise depth control
#define STEPS_PER_MM_E 95.0

#define ENCODER_PPR_X1 600
#define ENCODER_PPR_X2 600
#define ENCODER_PPR_Y1 600
#define ENCODER_PPR_Y2 600
#define ENCODER_PPR_Z 600
#define ENCODER_PPR_E 600

// Build volume (mm)
#define MAX_X_POS 100.0
#define MAX_Y_POS 100.0
#define MAX_Z_POS 50.0

// Speed limits (mm/min)
#define MAX_FEEDRATE_X 3000
#define MAX_FEEDRATE_Y 3000
#define MAX_FEEDRATE_Z 500    // Slow Z for precision
#define MAX_FEEDRATE_E 1000

// Acceleration (mm/s²)
#define MAX_ACCEL_X 500
#define MAX_ACCEL_Y 500
#define MAX_ACCEL_Z 100    // Gentle Z acceleration
#define MAX_ACCEL_E 200

// PID Tuning (conservative for precision)
#define DEFAULT_KP_X 2.0
#define DEFAULT_KI_X 0.5
#define DEFAULT_KD_X 0.8

#define DEFAULT_KP_Y 2.0
#define DEFAULT_KI_Y 0.5
#define DEFAULT_KD_Y 0.8

#define DEFAULT_KP_Z 3.0    // Stiffer Z control
#define DEFAULT_KI_Z 0.8
#define DEFAULT_KD_Z 1.2

// Tool configuration
#define DEFAULT_TOOL_PROFILE "DC_555_12V"
#define TOOL_PWM_PIN 15
#define TOOL_DIR_PIN 2
#define TOOL_ENABLE_PIN 4

// Homing
#define HOME_FEEDRATE_X 1000
#define HOME_FEEDRATE_Y 1000
#define HOME_FEEDRATE_Z 300
```

### Example G-code (PCB Etching)

```gcode
; PCB isolation routing - Single sided
G21                        ; mm mode
G90                        ; absolute positioning
M451                       ; CNC mode
M471 S"DC_555_12V"        ; Load spindle profile
G28 Z                      ; Home Z first (safety)
G28 X Y                    ; Home X and Y

; Setup
G0 Z5                      ; Safe height
M3 S8000                   ; Spindle to 8000 RPM
G4 P2000                   ; Wait 2 seconds for spindle

; Begin isolation milling (0.2mm depth)
G0 X10 Y10                 ; Start position
G1 Z-0.2 F100             ; Plunge slowly
G1 X90 F800               ; Cut at 800mm/min
G1 Y90
G1 X10
G1 Y10
G0 Z5                      ; Safe height

; Drilling (1mm holes)
G0 X20 Y20                 ; First hole
G1 Z-1.8 F80              ; Drill through (1.6mm PCB + margin)
G0 Z5
G0 X40 Y20                 ; Next hole
G1 Z-1.8 F80
G0 Z5

; Cleanup
M5                         ; Stop spindle
G0 Z50                     ; Full retract
G28 X Y                    ; Home
M84                        ; Motors off
```

### Bill of Materials

| Item | Quantity | Est. Cost | Notes |
|------|----------|-----------|-------|
| ESP32 Lolin32 Lite | 1 | $5 | Controller |
| NEMA 17 + Encoder | 6 | $180 | $30 each with 600 PPR encoder |
| TMC2208 Driver | 6 | $30 | $5 each |
| 555 DC Motor | 1 | $8 | 12V spindle |
| L298N Driver | 1 | $3 | Spindle control |
| MGN9H Rail 150mm | 3 | $45 | $15 each |
| T8 Lead Screw 150mm | 3 | $15 | $5 each |
| 2020 Extrusion | 2m | $20 | Frame |
| Power Supply 24V 5A | 1 | $15 | |
| Optical Endstops | 6 | $12 | $2 each |
| Z-probe (PCB contact) | 1 | $5 | DIY or purchased |
| Misc (wires, connectors) | - | $20 | |
| **TOTAL** | | **~$358** | |

---

## 2. Medium 3D Printer (24" x 24" x 24" / 610mm cube)

**Perfect for:** Large format PLA/ABS/PETG printing, props, cosplay

### Hardware Specifications

**Frame:**
- Working volume: 610mm x 610mm x 610mm (24" cube)
- Frame: 4040 aluminum extrusion (CoreXY or Cartesian)
- Linear rails: MGN12H (12mm) on X/Y, MGN15H on Z
- Lead screws: T8 lead screws or dual Z ballscrews

**Motors:**
- Type: NEMA 17 (X/Y) and NEMA 23 (Z - for heavy bed)
- All with 1000 PPR encoders
- CoreXY: 2x motors for XY, 2x for Z leveling
- Cartesian: 2x X, 2x Y, 2x Z

**Hotend:**
- E3D V6 or similar all-metal hotend
- Volcano hotend optional (faster printing)
- Direct drive extruder with encoder feedback

**Heated Bed:**
- 600mm x 600mm silicone heater (750W)
- Magnetic PEI spring steel build plate
- Thermistor: NTC 100K or PT1000

**Electronics:**
- ESP32: DevKit v1 or WROOM-32
- Motor drivers: TMC2209 with UART
- Power: 24V 30A supply (720W)
- Heated bed: SSR (Solid State Relay) for high current

**Sensors:**
- Encoders: 6x 1000 PPR
- BLTouch or CR Touch for bed leveling
- Filament runout sensor
- Chamber thermistor (optional)

### Wiring Diagram

```
ESP32 DevKit v1 Extended Pin Assignment:
┌─────────────────────────────────────────────────────┐
│ MOTORS (TMC2209 with UART, StealthChop)            │
├─────────────────────────────────────────────────────┤
│ X1 (or CoreXY A): Step=13, Dir=12, EN=14, UART=27  │
│ X2 (or CoreXY B): Step=26, Dir=25, EN=33, UART=32  │
│ Y1: Step=4,  Dir=16, EN=17, UART=5                 │
│ Y2: Step=18, Dir=19, EN=21, UART=22                │
│ Z1: Step=23, Dir=15, EN=2,  UART=0                 │
│ Z2: Step=34, Dir=35, EN=32, UART=33                │
│ E:  Step=19, Dir=18, EN=5,  UART=17                │
├─────────────────────────────────────────────────────┤
│ ENCODERS                                            │
├─────────────────────────────────────────────────────┤
│ X1: A=36, B=39 (interrupt pins)                    │
│ X2: A=34, B=35                                      │
│ Y1: A=32, B=33                                      │
│ Y2: A=25, B=26                                      │
│ Z1: A=27, B=14                                      │
│ Z2: A=12, B=13                                      │
│ E:  A=15, B=2                                       │
├─────────────────────────────────────────────────────┤
│ HEATERS (via MOSFET/SSR)                           │
├─────────────────────────────────────────────────────┤
│ Hotend: GPIO23 → MOSFET → 24V 40W                  │
│ Bed:    GPIO22 → SSR → 220V 750W or 24V 750W       │
├─────────────────────────────────────────────────────┤
│ THERMISTORS (ADC with voltage divider)             │
├─────────────────────────────────────────────────────┤
│ Hotend: GPIO36 (ADC1_0)                             │
│ Bed:    GPIO39 (ADC1_3)                             │
│ Chamber: GPIO34 (ADC1_6) - optional                 │
├─────────────────────────────────────────────────────┤
│ AUTO BED LEVELING                                   │
├─────────────────────────────────────────────────────┤
│ BLTouch Signal: GPIO21                              │
│ BLTouch Servo:  GPIO19 (PWM)                        │
├─────────────────────────────────────────────────────┤
│ COOLING FANS                                        │
├─────────────────────────────────────────────────────┤
│ Part Fan:   GPIO16 (PWM)                            │
│ Hotend Fan: GPIO4  (always on or thermostatic)      │
└─────────────────────────────────────────────────────┘
```

### Configuration File (config.h)

```cpp
// Large Format 3D Printer Configuration
#define MACHINE_NAME "BigBox 24x24x24"
#define DEFAULT_MODE MODE_3D_PRINTER

// Mechanical specifications (CoreXY example)
#define STEPS_PER_MM_X 80.0    // 200 * 16 / 40 (20 tooth GT2 pulley)
#define STEPS_PER_MM_Y 80.0
#define STEPS_PER_MM_Z 400.0   // T8 lead screw
#define STEPS_PER_MM_E 140.0   // BMG dual drive extruder

#define ENCODER_PPR_X1 1000
#define ENCODER_PPR_X2 1000
#define ENCODER_PPR_Y1 1000
#define ENCODER_PPR_Y2 1000
#define ENCODER_PPR_Z 1000
#define ENCODER_PPR_E 1000

// Build volume (mm)
#define MAX_X_POS 610.0
#define MAX_Y_POS 610.0
#define MAX_Z_POS 610.0

// Speed limits (mm/min)
#define MAX_FEEDRATE_X 12000   // Fast for large prints
#define MAX_FEEDRATE_Y 12000
#define MAX_FEEDRATE_Z 600
#define MAX_FEEDRATE_E 3000

// Acceleration (mm/s²)
#define MAX_ACCEL_X 1500
#define MAX_ACCEL_Y 1500
#define MAX_ACCEL_Z 100
#define MAX_ACCEL_E 500

// PID Tuning (balanced for quality + speed)
#define DEFAULT_KP_X 1.5
#define DEFAULT_KI_X 0.3
#define DEFAULT_KD_X 0.6

#define DEFAULT_KP_Y 1.5
#define DEFAULT_KI_Y 0.3
#define DEFAULT_KD_Y 0.6

#define DEFAULT_KP_Z 2.5
#define DEFAULT_KI_Z 0.5
#define DEFAULT_KD_Z 1.0

// Heater PID (tune these!)
#define DEFAULT_HOTEND_KP 22.0
#define DEFAULT_HOTEND_KI 1.08
#define DEFAULT_HOTEND_KD 114.0

#define DEFAULT_BED_KP 70.0
#define DEFAULT_BED_KI 10.0
#define DEFAULT_BED_KD 200.0

// Temperature limits
#define MAX_HOTEND_TEMP 285    // All-metal hotend
#define MAX_BED_TEMP 120       // For ABS/ASA
#define THERMAL_RUNAWAY_PERIOD 40  // seconds
#define THERMAL_RUNAWAY_HYSTERESIS 4  // °C

// Filament diameter
#define FILAMENT_DIAMETER 1.75

// Bed leveling (if using BLTouch)
#define ENABLE_AUTO_BED_LEVELING
#define ABL_PROBE_POINTS_X 5
#define ABL_PROBE_POINTS_Y 5
```

### Example G-code (Large PLA Print)

```gcode
; BigBox 610mm cube PLA print start
M140 S60                   ; Set bed temp (don't wait)
M104 S205                  ; Set hotend temp (don't wait)
G21                        ; mm
G90                        ; absolute
M82                        ; absolute extrusion
M450                       ; 3D printer mode

; Wait for temperatures
M190 S60                   ; Wait for bed
M109 S205                  ; Wait for hotend

; Home and level
G28                        ; Home all
G29                        ; Auto bed level (if enabled)

; Prime nozzle
G1 Z0.3 F3000             ; Raise to 0.3mm
G1 X10 Y10 F3000          ; Move to corner
G1 E10 F200               ; Extrude 10mm
G1 X10 Y150 E20 F1000     ; Prime line
G1 X11 Y150 F3000
G1 X11 Y10 E30 F1000      ; Second line
G92 E0                     ; Reset extruder

; Start print
G1 Z2.0 F3000             ; Lift
; ... actual print moves ...
```

### Bill of Materials

| Item | Quantity | Est. Cost | Notes |
|------|----------|-----------|-------|
| ESP32 DevKit v1 | 1 | $8 | Controller |
| NEMA 17 + Encoder 1000PPR | 5 | $200 | X, Y, E ($40 each) |
| NEMA 23 + Encoder 1000PPR | 2 | $120 | Dual Z ($60 each) |
| TMC2209 Driver | 7 | $42 | $6 each |
| E3D V6 Hotend | 1 | $60 | All-metal |
| BMG Extruder | 1 | $45 | Dual drive |
| MGN12H Rail 700mm | 2 | $80 | X/Y rails |
| MGN15H Rail 700mm | 2 | $100 | Z rails |
| T8 Lead Screw 700mm | 2 | $30 | Dual Z |
| GT2 Belt 10m | 1 | $15 | CoreXY |
| GT2 Pulleys 20T | 8 | $16 | $2 each |
| 4040 Extrusion | 12m | $150 | Frame |
| 600x600mm Heated Bed | 1 | $80 | Silicone 750W |
| Build Plate (PEI) | 1 | $50 | Magnetic |
| Power Supply 24V 30A | 1 | $45 | 720W |
| SSR 40A | 1 | $8 | Bed control |
| BLTouch | 1 | $25 | Auto leveling |
| Filament Sensor | 1 | $5 | Runout detection |
| Fans | 3 | $15 | Part + hotend + PSU |
| Misc (wires, MOSFET, etc) | - | $50 | |
| **TOTAL** | | **~$1,144** | |

---

## 3. Rotary Wood Carver (8" x 10" x 4" with rotary axis)

**Perfect for:** 3D wood carving, cylindrical objects, decorative work

### Hardware Specifications

**Frame:**
- Working area: 200mm x 250mm x 100mm (8" x 10" x 4")
- Plus rotary axis (A-axis) for cylinders up to 150mm diameter
- Frame: Steel tube or 3030 extrusion
- Linear rails: HGR15 or SBR16
- Ball screws: 1605 (16mm, 5mm pitch)

**Motors:**
- X, Y, Z: NEMA 23 with 1000 PPR encoders (high torque)
- Rotary (A): NEMA 17 with 1:5 gearing and encoder
- E axis: Not used (can repurpose as second rotary)

**Spindle:**
- ER11 300W brushless spindle (3000-12000 RPM)
- ESC control (RC hobby ESC)
- Water cooling optional, air cooling standard

**Electronics:**
- ESP32: DevKit v1
- Motor drivers: TMC2209 for X/Y/Z/A
- Spindle ESC: 30A brushless ESC
- Power: 24V 15A for motors, 24V 15A for spindle

**Sensors:**
- Encoders: 5x 1000 PPR (X, Y, Z, A, E-repurposed)
- Limit switches: 8x mechanical (X±, Y±, Z±, A home)
- Touch probe for work offset

### Wiring Diagram

```
ESP32 DevKit v1 - Rotary Carver:
┌─────────────────────────────────────────────────────┐
│ MOTORS                                              │
├─────────────────────────────────────────────────────┤
│ X:  Step=13, Dir=12, EN=14, UART=27 (NEMA 23)      │
│ Y:  Step=26, Dir=25, EN=33, UART=32 (NEMA 23)      │
│ Z:  Step=4,  Dir=16, EN=17, UART=5  (NEMA 23)      │
│ A:  Step=18, Dir=19, EN=21, UART=22 (NEMA 17)      │
│     (Rotary axis - repurposed from Y2)              │
├─────────────────────────────────────────────────────┤
│ ENCODERS                                            │
├─────────────────────────────────────────────────────┤
│ X: A=36, B=39                                       │
│ Y: A=34, B=35                                       │
│ Z: A=32, B=33                                       │
│ A: A=25, B=26 (rotary)                             │
├─────────────────────────────────────────────────────┤
│ SPINDLE (Brushless ESC)                            │
├─────────────────────────────────────────────────────┤
│ PWM: GPIO15 → ESC Signal (1-2ms pulse, 50Hz)       │
├─────────────────────────────────────────────────────┤
│ ENDSTOPS                                            │
├─────────────────────────────────────────────────────┤
│ X_MIN: GPIO23, X_MAX: GPIO22                        │
│ Y_MIN: GPIO21, Y_MAX: GPIO19                        │
│ Z_MIN: GPIO18 (also touch probe), Z_MAX: GPIO5     │
│ A_HOME: GPIO17 (index position)                     │
├─────────────────────────────────────────────────────┤
│ PROBE                                               │
├─────────────────────────────────────────────────────┤
│ Touch Probe: GPIO18 (shared with Z_MIN)            │
└─────────────────────────────────────────────────────┘
```

### Configuration File (config.h)

```cpp
// Rotary Wood Carver Configuration
#define MACHINE_NAME "RotaryCarver 8x10"
#define DEFAULT_MODE MODE_CNC_SPINDLE

// Mechanical specifications
#define STEPS_PER_MM_X 320.0   // 200 * 16 / 10 (1605 ballscrew = 10mm/rev)
#define STEPS_PER_MM_Y 320.0
#define STEPS_PER_MM_Z 320.0
#define STEPS_PER_MM_A 17.78   // Rotary: 200 * 16 / 180 = steps per degree

#define ENCODER_PPR_X1 1000
#define ENCODER_PPR_Y1 1000
#define ENCODER_PPR_Z 1000
#define ENCODER_PPR_A 1000     // Rotary axis (using Y2 encoder)

// Build volume
#define MAX_X_POS 200.0        // mm
#define MAX_Y_POS 250.0
#define MAX_Z_POS 100.0
#define MAX_A_POS 36000.0      // degrees (100 rotations)

// Speed limits
#define MAX_FEEDRATE_X 3000    // mm/min
#define MAX_FEEDRATE_Y 3000
#define MAX_FEEDRATE_Z 1000
#define MAX_FEEDRATE_A 3600    // degrees/min (10 RPM max)

// Acceleration
#define MAX_ACCEL_X 800
#define MAX_ACCEL_Y 800
#define MAX_ACCEL_Z 300
#define MAX_ACCEL_A 600        // degrees/s²

// PID Tuning (aggressive for wood cutting forces)
#define DEFAULT_KP_X 3.5
#define DEFAULT_KI_X 0.8
#define DEFAULT_KD_X 1.5

#define DEFAULT_KP_Y 3.5
#define DEFAULT_KI_Y 0.8
#define DEFAULT_KD_Y 1.5

#define DEFAULT_KP_Z 4.0       // Stiffest for cutting
#define DEFAULT_KI_Z 1.0
#define DEFAULT_KD_Z 2.0

#define DEFAULT_KP_A 2.5       // Rotary
#define DEFAULT_KI_A 0.5
#define DEFAULT_KD_A 1.0

// Spindle configuration
#define DEFAULT_TOOL_PROFILE "BLDC_ER11_300W"
#define TOOL_PWM_PIN 15
#define MAX_SPINDLE_RPM 12000
#define MIN_SPINDLE_RPM 3000

// Rotary axis enable
#define ENABLE_ROTARY_AXIS
#define ROTARY_AXIS A          // Using A-axis
```

### Example G-code (Cylindrical Carving)

```gcode
; Rotary carving - decorative pattern on cylinder
G21                        ; mm
G90                        ; absolute
M451                       ; CNC mode
M471 S"BLDC_ER11_300W"    ; Load spindle

; Home all axes including rotary
G28 X Y Z A

; Setup
G0 Z10                     ; Safe height
M3 S8000                   ; Spindle 8000 RPM
G4 P3000                   ; Spindle warmup

; Carve helical pattern (cylindrical stock)
; Cylinder: 50mm diameter, 150mm length
; Pattern: Spiral groove, 0.5mm deep, 10mm pitch

G0 X0 Y0 Z0 A0            ; Start position
G1 Z-0.5 F100             ; Plunge depth
G1 X150 A3600 F800        ; Helical move: 150mm along, 10 rotations
                          ; (3600 degrees = 10 full rotations)

; Retract
G0 Z10
G0 A0                      ; Return rotary to zero

; Multiple passes for deeper cut
G0 X0 Y0 Z0
G1 Z-1.0 F100
G1 X150 A3600 F800

G0 Z10
G0 A0

; Cleanup
M5                         ; Stop spindle
G28 X Y A                  ; Home (leave Z for part removal)
M84                        ; Motors off
```

### Bill of Materials

| Item | Quantity | Est. Cost | Notes |
|------|----------|-----------|-------|
| ESP32 DevKit v1 | 1 | $8 | Controller |
| NEMA 23 + Encoder 1000PPR | 3 | $210 | X, Y, Z ($70 each) |
| NEMA 17 + Encoder 1000PPR | 1 | $40 | Rotary axis |
| TMC2209 Driver | 4 | $24 | $6 each |
| ER11 300W Spindle | 1 | $70 | Brushless |
| 30A ESC | 1 | $15 | Spindle control |
| SBR16 Rail 300mm | 3 | $60 | X, Y, Z rails |
| 1605 Ballscrew 300mm | 3 | $60 | $20 each |
| Rotary Axis Kit | 1 | $80 | Chuck + tailstock |
| 3030 Extrusion | 4m | $50 | Frame |
| Power Supply 24V 15A | 2 | $60 | Motors + spindle |
| Limit Switches | 8 | $16 | $2 each |
| Touch Probe | 1 | $15 | Work offset |
| Misc (wires, couplers) | - | $40 | |
| **TOTAL** | | **~$748** | |

---

## 4. Large Format Laser Cutter (800mm x 800mm)

**Perfect for:** Wood engraving/cutting, acrylic cutting, leather work

### Hardware Specifications

**Frame:**
- Working area: 800mm x 800mm x 100mm (31.5" x 31.5")
- Frame: 4040 aluminum extrusion (very rigid)
- Linear rails: MGN15H (heavy duty)
- Belt drive: GT2 9mm belts (high strength)

**Motors:**
- X/Y: NEMA 23 with 1000 PPR encoders (high torque for fast acceleration)
- Z: NEMA 17 with encoder (focus adjustment)
- CoreXY or gantry configuration

**Laser:**
- 10W blue diode laser (445nm) for engraving/light cutting
- Or 80W CO2 laser tube for cutting
- PWM control + TTL
- Air assist required
- Exhaust fan required

**Electronics:**
- ESP32: DevKit v1 or WROOM-32
- Motor drivers: TMC2209
- Laser PSU: Dedicated laser power supply
- Power: 24V 10A for motors, laser has own PSU

**Safety:**
- Lid interlock switch
- Emergency stop button
- Air assist flow sensor
- Fume extraction interlock
- Laser safety glasses required!

### Wiring Diagram

```
ESP32 DevKit v1 - Laser Cutter:
┌─────────────────────────────────────────────────────┐
│ MOTORS (CoreXY configuration)                       │
├─────────────────────────────────────────────────────┤
│ X1 (CoreXY A): Step=13, Dir=12, EN=14, UART=27     │
│ X2 (CoreXY B): Step=26, Dir=25, EN=33, UART=32     │
│ Z (Focus):     Step=4,  Dir=16, EN=17, UART=5      │
├─────────────────────────────────────────────────────┤
│ ENCODERS                                            │
├─────────────────────────────────────────────────────┤
│ X1: A=36, B=39                                      │
│ X2: A=34, B=35                                      │
│ Z:  A=32, B=33                                      │
├─────────────────────────────────────────────────────┤
│ LASER (10W Blue Diode Example)                     │
├─────────────────────────────────────────────────────┤
│ PWM:    GPIO15 → Laser PSU PWM input               │
│ ENABLE: GPIO2  → Laser PSU TTL input               │
├─────────────────────────────────────────────────────┤
│ SAFETY INTERLOCKS                                   │
├─────────────────────────────────────────────────────┤
│ Lid Switch:     GPIO23 (NC - opens when lid open)  │
│ E-Stop:         GPIO22 (NC)                         │
│ Air Flow:       GPIO21 (Air pressure switch)        │
│ Fume Extract:   GPIO19 (Exhaust fan tach)          │
├─────────────────────────────────────────────────────┤
│ AIR ASSIST & EXHAUST                                │
├─────────────────────────────────────────────────────┤
│ Air Solenoid:   GPIO18 (relay/MOSFET)              │
│ Exhaust Fan:    GPIO5  (relay - 220V AC fan)       │
└─────────────────────────────────────────────────────┘
```

### Configuration File (config.h)

```cpp
// Large Format Laser Cutter Configuration
#define MACHINE_NAME "LaserCut 800x800"
#define DEFAULT_MODE MODE_LASER_CUTTER

// Mechanical specifications (CoreXY)
#define STEPS_PER_MM_X 80.0    // GT2 belt, 20 tooth pulley
#define STEPS_PER_MM_Y 80.0
#define STEPS_PER_MM_Z 400.0   // Fine Z for focus

#define ENCODER_PPR_X1 1000
#define ENCODER_PPR_X2 1000
#define ENCODER_PPR_Z 1000

// Build volume
#define MAX_X_POS 800.0
#define MAX_Y_POS 800.0
#define MAX_Z_POS 100.0        // Focus range

// Speed limits (FAST for laser!)
#define MAX_FEEDRATE_X 24000   // 400mm/s for raster
#define MAX_FEEDRATE_Y 24000
#define MAX_FEEDRATE_Z 300

// Acceleration (high for fast direction changes)
#define MAX_ACCEL_X 3000
#define MAX_ACCEL_Y 3000
#define MAX_ACCEL_Z 200

// PID Tuning (tight for precise positioning)
#define DEFAULT_KP_X 2.0
#define DEFAULT_KI_X 0.4
#define DEFAULT_KD_X 0.8

#define DEFAULT_KP_Y 2.0
#define DEFAULT_KI_Y 0.4
#define DEFAULT_KD_Y 0.8

#define DEFAULT_KP_Z 3.0
#define DEFAULT_KI_Z 0.6
#define DEFAULT_KD_Z 1.2

// Laser configuration
#define DEFAULT_LASER_PROFILE "DIODE_BLUE_10W"
#define LASER_PWM_PIN 15
#define LASER_ENABLE_PIN 2
#define LASER_PWM_FREQUENCY 20000  // 20kHz

// Safety interlocks (CRITICAL!)
#define LASER_INTERLOCK_PIN 23     // Lid switch
#define LASER_ENCLOSURE_PIN 22     // E-stop
#define LASER_AIR_ASSIST_PIN 21    // Air pressure
#define LASER_FUME_EXTRACT_PIN 19  // Exhaust fan

#define LASER_INTERLOCK_ENABLED true
#define LASER_ENCLOSURE_REQUIRED true
#define LASER_AIR_ASSIST_REQUIRED true
#define LASER_FUME_EXTRACTION_REQUIRED true

// Air assist control
#define AIR_ASSIST_SOLENOID_PIN 18
#define EXHAUST_FAN_RELAY_PIN 5
```

### Example G-code (Wood Engraving)

```gcode
; Wood engraving - photo raster
G21                        ; mm
G90                        ; absolute
M452                       ; Laser mode
M461 S"DIODE_BLUE_10W"    ; Load laser profile
M466                       ; Safety check (ALL interlocks)

; Home
G28 X Y
G28 Z                      ; Focus to surface

; Setup
G0 X10 Y10 Z0             ; Start position
M3 S0                      ; Laser standby (0 power)

; Raster engraving (simulated - real would be image data)
; Line 1
G0 X10 Y10
M463 S20                   ; 20% power
G1 X200 F12000            ; Engrave line at 200mm/s
M463 S0                    ; Power off
G0 Y11                     ; Next line (0.1mm step)

; Line 2
G0 X200                    ; Return (right to left)
M463 S30                   ; 30% power
G1 X10 F12000
M463 S0

; ... more raster lines ...

; Vector cutting (outline)
M464 S1 P50               ; Enable ramping (50W/s)
G0 X50 Y50
M463 S100                  ; Full power for cutting
G1 Z-0.5 F100             ; Focus slightly below surface
G1 X250 F800              ; Cut at 800mm/min
G1 Y250
G1 X50
G1 Y50
G0 Z0                      ; Return focus

; Cleanup
M5                         ; Laser off
G0 X0 Y0                   ; Home position
M84                        ; Motors off
```

### Safety Checklist

**CRITICAL - READ BEFORE OPERATING:**

- [ ] Laser safety glasses worn (OD6+ @ 445nm for blue diode)
- [ ] Lid closed (interlock active)
- [ ] Air assist connected and flowing
- [ ] Exhaust fan running
- [ ] Fire extinguisher nearby
- [ ] Never leave laser unattended
- [ ] Test fire at low power first
- [ ] Material is laser-safe (NO PVC, NO POLYCARBONATE)

### Bill of Materials

| Item | Quantity | Est. Cost | Notes |
|------|----------|-----------|-------|
| ESP32 DevKit v1 | 1 | $8 | Controller |
| NEMA 23 + Encoder 1000PPR | 2 | $140 | X/Y CoreXY ($70 each) |
| NEMA 17 + Encoder | 1 | $40 | Z focus |
| TMC2209 Driver | 3 | $18 | $6 each |
| 10W Blue Diode Laser | 1 | $120 | Complete with PSU |
| MGN15H Rail 1000mm | 2 | $100 | X/Y rails |
| MGN15H Rail 200mm | 1 | $20 | Z rail |
| GT2 Belt 9mm (20m) | 1 | $30 | High strength |
| GT2 Pulleys 20T | 8 | $20 | |
| 4040 Extrusion | 12m | $180 | Rigid frame |
| T8 Lead Screw 200mm | 1 | $8 | Z axis |
| Power Supply 24V 10A | 1 | $30 | Motors |
| Safety Switches | 4 | $12 | Interlocks |
| Air Pump + Solenoid | 1 | $35 | Air assist |
| Exhaust Fan 220V | 1 | $40 | Fume extraction |
| Laser Safety Glasses | 1 | $25 | OD6+ @ 445nm |
| Acrylic Panels | - | $60 | Enclosure |
| Misc (wires, relays) | - | $50 | |
| **TOTAL** | | **~$936** | |

---

## Common Features Across All Builds

### Encoder Feedback Benefits

1. **Position Verification**: Real-time position error detection
2. **Stall Detection**: Automatic detection of missed steps
3. **Closed-Loop Control**: PID maintains exact position
4. **Adaptive Control**: System adjusts to load changes

### PID Tuning Tips

**Conservative (High Precision, Slower Response):**
```cpp
Kp = 1.5, Ki = 0.3, Kd = 0.6
```

**Balanced (Good for Most Applications):**
```cpp
Kp = 2.5, Ki = 0.5, Kd = 1.0
```

**Aggressive (Fast Response, May Oscillate):**
```cpp
Kp = 4.0, Ki = 1.0, Kd = 1.5
```

**Auto-Tuning via G-code:**
```gcode
M802 P0        ; Auto-tune X axis (motor 0)
M802 P1        ; Auto-tune Y axis (motor 1)
; Wait 2-3 minutes per axis
M800 P0        ; Display tuned values
```

### Web Interface Access

All builds include WiFi web interface:

```
Default AP Mode:
SSID: Encoder3D_XXXXXX
Password: encoder3d
IP: http://192.168.4.1

Station Mode (after WiFi config):
IP: http://encoder3d.local
or check your router for assigned IP
```

### Firmware Upload

1. Install PlatformIO in VS Code
2. Clone repository
3. Edit `include/config.h` for your machine
4. Connect ESP32 via USB
5. Click "Upload" in PlatformIO
6. Monitor serial output (115200 baud)

---

## Conversion from Existing Hardware

### From 3D Printer → CNC Mill
- Add spindle (remove hotend)
- Change to `MODE_CNC_SPINDLE`
- Adjust speeds/accels for cutting
- Add coolant/air blast

### From CNC → Laser Cutter
- Add laser module
- Change to `MODE_LASER_CUTTER`
- **ADD SAFETY INTERLOCKS!**
- Increase max feedrate
- Add air assist

### From Laser → 3D Printer
- Remove laser, add hotend
- Change to `MODE_3D_PRINTER`
- Add heated bed
- Tune heater PID
- Adjust for print speeds

---

## Support & Resources

- **Firmware:** github.com/encoder3d/firmware
- **Documentation:** `/docs/` folder in repository
- **Web Interface:** Built-in at `http://encoder3d.local`
- **G-code Reference:** `docs/GCODE_REFERENCE.md`
- **Wiring Guide:** `WIRING.md`

---

**Safety Disclaimer:**

All CNC machines are potentially dangerous. Lasers can cause blindness. Spindles can cause severe injury. Hot ends can burn. Always:
- Implement proper safety interlocks
- Use appropriate PPE
- Never bypass safety features
- Keep fire extinguisher nearby
- Never leave machines unattended
- Follow all local safety regulations

**Use at your own risk. The developers assume no liability for injuries or damage.**

---

*Build amazing things safely!* 🛠️

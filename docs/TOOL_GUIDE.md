# CNC Tool Support Guide

## Overview

The Encoder3D firmware provides comprehensive support for a wide variety of CNC end effectors, from budget DIY setups to professional industrial tools.

---

## Supported Tool Types

### ✅ Spindles
- **DC Motor Spindles** - Cheap and cheerful (775/555 motors)
- **Brushless Spindles** - Better performance, ESC control
- **VFD Spindles** - Professional water-cooled systems
- **Router Conversions** - Makita RT0700, DeWalt 611

### ✅ Plasma Torches
- **Pilot Arc** - CUT50, CUT60, Hypertherm
- **HF Start** - High-frequency start systems
- **Blowback Start** - Touch-start systems

### ✅ Other Tools
- **Drag Knives** - Vinyl cutting, paper
- **Pen Plotters** - Drawing, plotting
- **Hot Wire Cutters** - Foam cutting
- **Vacuum Pickups** - Pick-and-place
- **Pneumatic Drills** - Air-powered tools
- **3D Printer Extruders** - Basic support

---

## Spindle Comparison

| Spindle Type | Power | RPM Range | Cost | DIY Friendly | Cooling | Best For |
|--------------|-------|-----------|------|--------------|---------|----------|
| **775 DC Motor** | 80-150W | 3K-15K | $ | ⭐⭐⭐⭐⭐ | Air | Light wood, PCB |
| **555 DC Motor** | 50-100W | 5K-12K | $ | ⭐⭐⭐⭐⭐ | Air | PCB, soft materials |
| **ER11 BLDC** | 300-500W | 3K-15K | $$ | ⭐⭐⭐⭐ | Air | Wood, aluminum |
| **ER20 BLDC** | 1000W | 5K-18K | $$$ | ⭐⭐⭐ | Air | Production work |
| **1.5kW VFD** | 1500W | 6K-24K | $$$$ | ⭐⭐ | Water | Aluminum, steel |
| **2.2kW VFD** | 2200W | 6K-24K | $$$$$ | ⭐⭐ | Water | Steel, production |
| **Makita RT0700** | 710W | 10K-30K | $$$ | ⭐⭐⭐⭐ | Air | Popular DIY choice |
| **DeWalt 611** | 1HP | 16K-27K | $$$ | ⭐⭐⭐⭐ | Air | Popular in US |

**Cost Legend:**
- $ = Under $50
- $$ = $50-150
- $$$ = $150-400
- $$$$ = $400-800
- $$$$$ = $800+

---

## Quick Start Examples

### Example 1: 775 DC Motor Spindle (Budget CNC)

**Perfect For:** First CNC, PCB milling, soft wood

**Hardware Needed:**
- 775 DC motor (~$5-15)
- L298N motor driver (~$3)
- 12V or 24V power supply
- 1/8" collet or chuck
- Mounting bracket

**Wiring:**
```
ESP32 Pin 32 (PWM)  →  L298N Enable A
ESP32 Pin 33 (DIR)  →  L298N Input 1
ESP32 Pin 14 (EN)   →  L298N Input 2
L298N Output A      →  775 Motor +/-
```

**Configuration:**
```cpp
// config.h
#define DEFAULT_TOOL_PROFILE "DC_775_12V"
#define TOOL_PWM_PIN 32
#define TOOL_DIR_PIN 33
#define TOOL_ENABLE_PIN 14
```

**G-code Usage:**
```gcode
M470 S"DC_775_12V"    ; Load 775 motor profile
M3 S8000              ; Start spindle at 8000 RPM
G1 X10 Y10 F300       ; Cut move
M5                    ; Stop spindle
```

**Pros:**
- ✅ Very cheap ($20 total)
- ✅ Easy to find
- ✅ Simple wiring
- ✅ Good for learning

**Cons:**
- ❌ Limited power
- ❌ Noisy
- ❌ Short bearing life
- ❌ Not for hard materials

---

### Example 2: ER11 Brushless Spindle (Mid-Range)

**Perfect For:** Serious hobby CNC, aluminum milling, production

**Hardware Needed:**
- ER11 300W or 500W spindle (~$60-120)
- ESC (Electronic Speed Controller) (~$15-30)
- 24V or 48V power supply
- ER11 collets (various sizes)
- Dust shoe / air blast

**Wiring:**
```
ESP32 Pin 32 (PWM)  →  ESC Signal (white/yellow)
Ground              →  ESC Ground (black/brown)
24-48V PSU          →  ESC Power (red)
ESC Motor Wires     →  Spindle 3-phase wires
```

**Configuration:**
```cpp
// config.h
#define DEFAULT_TOOL_PROFILE "BLDC_ER11_500W"
#define TOOL_PWM_PIN 32
#define TOOL_AIR_ASSIST_PIN 26  // Optional air solenoid
```

**G-code Usage:**
```gcode
M470 S"BLDC_ER11_500W"  ; Load brushless profile
M471 S1                  ; Enable ramping (smooth start)
M3 S12000                ; Start at 12000 RPM
; ... cutting moves ...
M5                       ; Stop (ramps down smoothly)
```

**Pros:**
- ✅ Good power-to-weight
- ✅ Quiet operation
- ✅ Long bearing life
- ✅ Can handle aluminum
- ✅ Affordable

**Cons:**
- ❌ Needs ESC calibration
- ❌ No true RPM feedback
- ❌ Air cooling only

---

### Example 3: 2.2kW Water-Cooled VFD Spindle (Professional)

**Perfect For:** Production work, steel, hard materials

**Hardware Needed:**
- 2.2kW water-cooled spindle (~$250-400)
- VFD (Variable Frequency Drive) (~$150-250)
- Water pump + radiator (~$50-100)
- 0-10V signal isolator (~$10)
- Flow sensor (~$15)
- Temperature sensor (~$5)

**Wiring:**
```
ESP32 DAC Pin 25    →  0-10V Isolator Input
Isolator Output     →  VFD Analog Input (AI1)
VFD Relay Output    →  ESP32 Pin 27 (Run status)
Flow Sensor         →  ESP32 Pin 26 (Safety)
Temp Sensor         →  ESP32 Pin 35 (ADC)
Water Pump          →  VFD Aux Relay Output
```

**Configuration:**
```cpp
// config.h
#define DEFAULT_TOOL_PROFILE "VFD_2_2KW_WATER"
#define TOOL_ANALOG_PIN 25      // DAC output
#define TOOL_COOLANT_PIN 26     // Flow sensor
#define TOOL_TEMP_PIN 35        // Temperature
#define TOOL_ENABLE_PIN 27      // VFD run status
```

**VFD Setup:**
```
P00.00 = 1      ; Control source: Terminal
P00.01 = 1      ; Frequency source: Analog AI1
P00.06 = 400    ; Max frequency (400 Hz = 24000 RPM)
P00.07 = 100    ; Min frequency (100 Hz = 6000 RPM)
P08.00 = 10.0   ; AI1 max voltage (10V)
P08.01 = 0.0    ; AI1 min voltage (0V)
```

**G-code Usage:**
```gcode
M470 S"VFD_2_2KW_WATER"  ; Load VFD profile
M475                      ; Check coolant flow
M3 S18000                 ; Start at 18000 RPM
; ... heavy cutting in steel ...
M5                        ; Stop with 30s cooldown
```

**Pros:**
- ✅ Professional quality
- ✅ Consistent performance
- ✅ True RPM feedback from VFD
- ✅ Can cut steel
- ✅ Quiet operation
- ✅ Long service life

**Cons:**
- ❌ Expensive (~$500+ complete)
- ❌ Complex wiring
- ❌ Needs water cooling system
- ❌ 220V AC required
- ❌ VFD programming needed

---

### Example 4: Makita RT0700 Router (Popular DIY)

**Perfect For:** DIY CNC routers, wood, aluminum

**Hardware Needed:**
- Makita RT0700 router (~$100-150)
- IoT relay (for safe 230V switching) (~$15)
- Optional: speed controller module

**Wiring:**
```
ESP32 Pin 14  →  IoT Relay Control
IoT Relay     →  Makita Router Power
```

**Configuration:**
```cpp
// config.h
#define DEFAULT_TOOL_PROFILE "MAKITA_RT0700"
#define TOOL_ENABLE_PIN 14  // Relay control
// Manual speed dial on router itself
```

**G-code Usage:**
```gcode
M470 S"MAKITA_RT0700"  ; Load Makita profile
M3                     ; Turn on (speed set manually on router)
; ... cutting moves ...
M5                     ; Turn off
```

**Pros:**
- ✅ Proven reliability
- ✅ Easy to find worldwide
- ✅ Good power (710W)
- ✅ Soft start built-in
- ✅ Large community support

**Cons:**
- ❌ No automatic speed control
- ❌ Loud
- ❌ Heavy
- ❌ High minimum RPM (10K)

---

## Plasma Torch Support

### Example 5: 50A Pilot Arc Plasma Cutter

**Perfect For:** Steel cutting, artistic metalwork

**Hardware Needed:**
- 50A plasma cutter with CNC port (~$300-500)
- Torch height controller (THC) (~$100-200)
- Voltage divider (arc voltage sensing) (~$10)
- Limit switches for Z-axis
- Slat table or water table

**Wiring:**
```
ESP32 Pin 32 (PWM)      →  THC Height Control
ESP32 Pin 14 (START)    →  Plasma CNC Start
ESP32 Pin 27 (ARC OK)   →  Plasma Arc OK Signal
ESP32 Pin 26 (AIR)      →  Air Pressure Switch
ESP32 Pin 35 (VOLTAGE)  →  Arc Voltage Divider
```

**Configuration:**
```cpp
// config.h
#define DEFAULT_TOOL_PROFILE "PLASMA_CUT50_PILOT"
#define PLASMA_ARC_START_PIN 14
#define PLASMA_ARC_OK_PIN 27
#define PLASMA_HEIGHT_PIN 32
#define PLASMA_AIR_PIN 26
#define PLASMA_PIERCE_HEIGHT 3.8    // mm
#define PLASMA_CUT_HEIGHT 1.5       // mm
#define PLASMA_PIERCE_DELAY 500     // ms
```

**G-code Usage:**
```gcode
M470 S"PLASMA_CUT50_PILOT"  ; Load plasma profile
M478                         ; Check air pressure
G0 Z10                       ; Rapid to safe height
G0 X50 Y50                   ; Position for pierce
M479                         ; Pierce (auto height + delay)
M3                           ; Arc on
G1 X100 Y100 F2000          ; Cut move at 2000mm/min
M5                           ; Arc off
G0 Z10                       ; Safe height
```

**Material Settings:**

| Material | Thickness | Cut Height | Pierce Delay | Feed Rate |
|----------|-----------|------------|--------------|-----------|
| Mild Steel | 1mm | 1.0mm | 300ms | 4000mm/min |
| Mild Steel | 3mm | 1.5mm | 500ms | 2500mm/min |
| Mild Steel | 6mm | 1.8mm | 800ms | 1500mm/min |
| Stainless | 3mm | 1.5mm | 600ms | 2000mm/min |
| Aluminum | 3mm | 1.2mm | 400ms | 3000mm/min |

**Pros:**
- ✅ Cuts thick steel easily
- ✅ Fast cutting
- ✅ Relatively affordable
- ✅ Pilot arc = no touch start

**Cons:**
- ❌ Consumables cost (tips, electrodes)
- ❌ Rough edge quality
- ❌ Dangerous (arc flash, fumes)
- ❌ Requires compressed air
- ❌ Complex THC setup

**SAFETY WARNING:**
- ⚠️ Plasma arc produces UV radiation - eye protection required
- ⚠️ Metal fumes are toxic - ventilation required
- ⚠️ Arc flash can cause burns
- ⚠️ Fire hazard - have extinguisher ready
- ⚠️ Never cut galvanized steel (toxic zinc fumes)

---

## Other Tool Types

### Drag Knife (Vinyl Cutting)

**Hardware:**
- Servo-actuated drag knife (~$50)
- Servo motor (SG90 or similar)

**Configuration:**
```cpp
#define DEFAULT_TOOL_PROFILE "DRAG_KNIFE_STANDARD"
#define TOOL_PWM_PIN 32  // Servo control
```

**G-code:**
```gcode
M470 S"DRAG_KNIFE_STANDARD"
M3 S50     ; Lower knife (50% pressure)
G1 X100    ; Cut
M5         ; Raise knife
```

---

### Pen Plotter

**Hardware:**
- Servo to lift pen (~$5)
- Pen holder

**Configuration:**
```cpp
#define DEFAULT_TOOL_PROFILE "PEN_PLOTTER_STANDARD"
#define TOOL_PWM_PIN 32
```

**G-code:**
```gcode
M470 S"PEN_PLOTTER_STANDARD"
M3         ; Pen down
G1 X50 Y50 ; Draw
M5         ; Pen up
```

---

### Hot Wire Foam Cutter

**Hardware:**
- Nichrome wire (0.5-0.8mm)
- 24V power supply
- MOSFET driver

**Configuration:**
```cpp
#define DEFAULT_TOOL_PROFILE "HOT_WIRE_STANDARD"
#define TOOL_PWM_PIN 32
#define TOOL_TEMP_PIN 35  // Optional thermistor
```

**G-code:**
```gcode
M470 S"HOT_WIRE_STANDARD"
M3 S60     ; 60% power - preheat
G4 P5000   ; Wait 5 seconds
G1 X100 F500  ; Cut slowly
M5         ; Cool down
```

---

## Complete Tool Profile List

### Available Profiles

The firmware includes these predefined profiles:

**DC Spindles:**
- `DC_775_12V` - 775 motor at 12V (3K-10K RPM)
- `DC_775_24V` - 775 motor at 24V (3K-15K RPM)
- `DC_555_12V` - 555 motor at 12V (2K-12K RPM)

**Brushless Spindles:**
- `BLDC_ER11_300W` - 300W ER11 (3K-12K RPM)
- `BLDC_ER11_500W` - 500W ER11 (3K-15K RPM)
- `BLDC_ER20_1000W` - 1000W ER20 (5K-18K RPM)

**VFD Spindles:**
- `VFD_1_5KW_WATER` - 1.5kW water-cooled (6K-24K RPM)
- `VFD_2_2KW_WATER` - 2.2kW water-cooled (6K-24K RPM)
- `VFD_3_0KW_WATER` - 3.0kW water-cooled (8K-24K RPM)

**Router Conversions:**
- `MAKITA_RT0700` - Makita trim router (10K-30K RPM)
- `DEWALT_611` - DeWalt trim router (16K-27K RPM)

**Plasma Torches:**
- `PLASMA_CUT50_PILOT` - 50A pilot arc
- `PLASMA_CUT60_PILOT` - 60A pilot arc
- `PLASMA_HYPERTHERM_45` - Hypertherm Powermax 45

**Other Tools:**
- `DRAG_KNIFE_STANDARD` - Vinyl drag knife
- `PEN_PLOTTER_STANDARD` - Pen plotter
- `HOT_WIRE_STANDARD` - Foam cutter
- `VACUUM_PICKUP_STANDARD` - Pick-and-place

---

## G-Code Commands

### Tool Selection & Control

| Command | Description | Example |
|---------|-------------|---------|
| **M470** | Set tool type by index | `M470 P3` |
| **M471** | Load tool profile | `M471 S"VFD_2_2KW_WATER"` |
| **M472** | Set speed in RPM | `M472 S12000` |
| **M473** | Set speed in percent | `M473 S75` |
| **M474** | Enable/disable ramping | `M474 S1 P1000` (enable, 1000 RPM/s) |
| **M475** | Tool safety check | `M475` (reports status) |
| **M476** | Emergency tool stop | `M476` |
| **M3** | Start tool CW | `M3 S12000` |
| **M4** | Start tool CCW | `M4 S10000` |
| **M5** | Stop tool | `M5` |

### Plasma-Specific Commands

| Command | Description | Example |
|---------|-------------|---------|
| **M477** | Set torch height | `M477 S1.5` (1.5mm) |
| **M478** | Check plasma safety | `M478` (air, arc OK) |
| **M479** | Pierce sequence | `M479` (auto pierce) |

---

## Safety Features

### Spindle Safety

1. **Coolant Flow Monitoring** (VFD spindles)
   - Flow sensor detects water circulation
   - Auto-shutdown if flow stops
   - 30-60 second cooldown period

2. **Temperature Monitoring**
   - Thermistor on spindle body
   - Alarm at configurable threshold
   - Emergency stop if overtemp

3. **RPM Monitoring** (if equipped)
   - Tachometer feedback
   - Detect stalls
   - Speed verification

4. **Current Monitoring**
   - Detect overload conditions
   - Tool breakage detection
   - Power consumption tracking

### Plasma Safety

1. **Air Pressure Interlock**
   - Requires minimum air pressure
   - Cannot start without air
   - Auto-stop if pressure drops

2. **Arc OK Monitoring**
   - Detects arc presence
   - Alerts if arc lost during cut
   - Prevents missed cuts

3. **Torch Height Control**
   - Maintains consistent cut height
   - Prevents torch crashes
   - Voltage-based feedback

4. **Ohmic Sensing** (optional)
   - Touch-off for material detection
   - Automatic Z-zero setting

---

## Custom Tool Profiles

### Creating Your Own Profile

```cpp
// In your code or config
ToolSpec my_custom_spindle = {
    .type = TOOL_SPINDLE_DC,
    .name = "My Custom Spindle",
    .max_speed = 20000.0,      // Max RPM
    .min_speed = 5000.0,       // Min RPM
    .idle_speed = 8000.0,      // Idle RPM
    .control_mode = CONTROL_PWM,
    .pwm_frequency = 25000,    // 25kHz PWM
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 24.0,
    .max_current = 5.0,        // 5A max
    .rated_voltage = 24.0,
    .collet_size = 3.175,      // 1/8"
    .spinup_time = 2000,       // 2 second spinup
    .spindown_time = 3000,     // 3 second spindown
    .min_on_time = 100,
    .cooldown_time = 0,
    .safety = {
        .requires_coolant = false,
        .requires_air_assist = true,
        .requires_torch_height = false,
        .requires_interlock = false,
        .has_tachometer = true,
        .has_temperature_sensor = false,
        .has_current_sensor = false,
        .requires_fume_extraction = true
    },
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 80.0,
    .max_duty_cycle = 100
};

// Load the custom profile
tool_controller.setToolSpec(my_custom_spindle);
```

---

## Troubleshooting

### Spindle Won't Start

1. ✅ Check enable signal (M475)
2. ✅ Verify power supply voltage
3. ✅ Check ESC calibration (BLDC)
4. ✅ Verify VFD parameters (VFD)
5. ✅ Check safety interlocks
6. ✅ Inspect wiring connections

### Spindle Speed Unstable

1. ✅ Check PWM frequency (try M474)
2. ✅ Verify power supply capacity
3. ✅ Check for loose connections
4. ✅ Inspect brushes (DC motors)
5. ✅ Verify load isn't too high
6. ✅ Check for electrical noise

### VFD Spindle Issues

1. ✅ Verify 0-10V signal level
2. ✅ Check VFD parameters (P00.xx)
3. ✅ Verify water flow
4. ✅ Check three-phase output
5. ✅ Inspect cable shielding
6. ✅ Review VFD error codes

### Plasma Problems

1. ✅ Check air pressure (60-80 PSI)
2. ✅ Inspect consumables (tip, electrode)
3. ✅ Verify ground clamp connection
4. ✅ Check torch height
5. ✅ Clean nozzle
6. ✅ Verify CNC port wiring

---

## Recommended Upgrades

### Budget → Mid-Range
- Replace 775 DC with ER11 brushless
- Add air assist system
- Install RPM tachometer
- Add temperature monitoring

### Mid-Range → Professional
- Upgrade to water-cooled VFD spindle
- Add automatic tool changer
- Install current monitoring
- Professional THC for plasma

---

## Additional Resources

- **Source Code:** `include/tool_controller.h`
- **Implementation:** `src/tool_controller.cpp`
- **G-Code Reference:** `docs/GCODE_REFERENCE.md`
- **Configuration:** `include/config.h`

---

## Legal & Safety

**WARNING:** 
- Power tools are dangerous
- Plasma cutters can cause severe burns and eye damage
- High voltage can be lethal
- Always follow manufacturer safety guidelines
- Wear appropriate PPE
- Ensure proper ventilation
- Keep fire extinguisher nearby

**The developers are not responsible for injuries, property damage, or equipment damage resulting from use of this software.**

Use at your own risk. Consult professionals for industrial applications.

---

*For additional tool support requests, submit an issue or pull request to the repository.*

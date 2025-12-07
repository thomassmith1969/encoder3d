# Laser Controller Guide

## Overview

The Encoder3D controller provides comprehensive support for a wide variety of laser types, from hobbyist diode lasers to industrial fiber lasers and high-power laser welders. The system includes advanced safety features, power control modes, and material-optimized profiles.

## Supported Laser Types

### 1. CO2 IR Lasers (10.6µm)
**Traditional cutting and engraving lasers**

- **CO2 40W** - Hobby/small business
  - Typical uses: Wood, acrylic, leather, paper engraving
  - Cutting: Up to 6mm acrylic, 3mm wood
  - Focus distance: 50.8mm (2")
  
- **CO2 100W** - Professional
  - Typical uses: Thicker materials, faster cutting
  - Cutting: Up to 12mm acrylic, 8mm wood
  - Focus distance: 63.5mm (2.5")

**Characteristics:**
- Requires mirrors and lens maintenance
- Best for non-metallic materials
- Excellent cut quality
- Air assist strongly recommended

### 2. Blue Diode Lasers (405-450nm)
**Popular for DIY and small CNC conversions**

- **Blue Diode 5W** - Entry level
  - Typical uses: Wood engraving, light cutting
  - Cutting: Up to 2mm basswood/balsa
  - Very affordable
  
- **Blue Diode 10W** - Mid-range
  - Typical uses: Wood cutting, acrylic engraving
  - Cutting: Up to 4mm wood, 2mm acrylic
  - Good power/price ratio
  
- **Blue Diode 20W** - High power
  - Typical uses: Faster cutting, thicker materials
  - Cutting: Up to 6mm wood, 3mm acrylic
  - Requires active cooling

**Characteristics:**
- Easy to mount on existing CNC
- No mirrors needed (direct beam)
- Works on wood, plastics, painted metals
- NOT suitable for transparent acrylic (passes through)
- Eye protection absolutely critical (445nm extremely dangerous)

### 3. Red Diode Lasers (635-650nm)
**Lower power, safer for alignment**

- **Red Diode 500mW** - Engraving only
  - Typical uses: Light engraving, marking
  - Often used as alignment laser
  - Very safe at low powers

**Characteristics:**
- Visible beam (easier to see)
- Good for paper, light wood engraving
- Commonly used for "aim and mark" systems

### 4. Fiber Lasers (1064nm)
**Industrial marking and metal cutting**

- **Fiber 20W** - Metal marking
  - Typical uses: Anodized aluminum, stainless steel marking
  - High-speed engraving
  - Permanent marks on metals
  
- **Fiber 50W** - Metal cutting
  - Typical uses: Thin metal cutting, deep engraving
  - Cuts: Up to 1mm stainless steel
  - Very precise spot size

**Characteristics:**
- Infrared (invisible beam - DANGEROUS)
- Excellent for metals
- Very fine detail possible
- Requires proper beam dump and safety enclosure
- High initial cost but low maintenance

### 5. Laser Welders (1064nm)
**High-power metal welding and cutting**

- **Welder 1000W** - Medium industrial
  - Typical uses: Sheet metal welding, thick cutting
  - Welds: Up to 3mm steel
  - Precision welding applications
  
- **Welder 2000W** - Heavy industrial
  - Typical uses: Thick plate welding, rapid cutting
  - Welds: Up to 6mm steel
  - Production environments

**Characteristics:**
- Extremely dangerous - invisible infrared beam
- Requires full safety enclosure
- Water cooling mandatory
- Interlock systems required
- Professional training recommended

## Safety Systems

### Hardware Safety Features

The controller supports multiple safety interlocks:

1. **Hardware Interlock**
   - Emergency stop circuit
   - Breaks laser power if triggered
   - Typically door switch or safety key
   
2. **Enclosure Detection**
   - Detects if protective enclosure is open
   - Disables laser if door opened during operation
   
3. **Air Assist Monitoring**
   - Ensures air assist is flowing
   - Critical for preventing fire and improving cut quality
   
4. **Water Cooling Flow**
   - Monitors water flow for high-power lasers
   - Prevents thermal damage to laser tube/diode
   
5. **Beam Detection** (future)
   - Detects unexpected beam reflections
   - Additional safety layer

### Software Safety Features

1. **Maximum Duty Cycle Limiting**
   - Prevents overheating
   - Configurable per laser type
   
2. **Maximum Continuous Fire Time**
   - Auto-shutoff after extended firing
   - Default: 60 seconds continuous
   
3. **Power Ramping**
   - Gradual power changes
   - Reduces thermal shock
   - Prevents material flash-burning

## Configuration

### Quick Start

#### 1. Select Your Laser Type

Edit `include/config.h`:

```cpp
// For CO2 40W laser
#define DEFAULT_LASER_PROFILE "CO2_40W"

// For blue diode 10W
#define DEFAULT_LASER_PROFILE "DIODE_BLUE_10W"

// For fiber 20W
#define DEFAULT_LASER_PROFILE "FIBER_20W"

// For laser welder 1000W
#define DEFAULT_LASER_PROFILE "WELDER_1000W"
```

#### 2. Configure Pins

```cpp
#define LASER_PWM_PIN 32        // PWM control signal
#define LASER_ENABLE_PIN 14     // Enable/disable
#define LASER_ANALOG_PIN 25     // 0-10V analog (if supported)
#define LASER_TTL_PIN 255       // TTL on/off (255 = not used)
```

#### 3. Configure Safety

```cpp
// Safety interlock pins (255 = not used)
#define LASER_INTERLOCK_PIN 22      // E-stop or key switch
#define LASER_ENCLOSURE_PIN 23      // Door sensor
#define LASER_AIR_ASSIST_PIN 24     // Air flow sensor
#define LASER_WATER_FLOW_PIN 25     // Water flow sensor

// Safety requirements
#define LASER_INTERLOCK_ENABLED true
#define LASER_ENCLOSURE_REQUIRED true
#define LASER_AIR_ASSIST_REQUIRED true      // Highly recommended for CO2
#define LASER_WATER_COOLING_REQUIRED true   // Required for high power
```

### Advanced Configuration

#### Custom Laser Profile

```cpp
LaserSpec my_custom_laser = {
    .type = LASER_CUSTOM,
    .max_power = 15.0,              // 15W max
    .min_power = 0.5,               // 0.5W min stable
    .wavelength = 445.0,            // 445nm blue
    .min_pwm_freq = 1000,           // 1kHz min
    .max_pwm_freq = 50000,          // 50kHz max
    .optimal_pwm_freq = 20000,      // 20kHz optimal
    .supports_ttl = true,
    .supports_analog = false,
    .supports_pwm = true,
    .focus_distance = 12.0,         // 12mm focal length
    .spot_size = 0.1                // 0.1mm spot
};

laser_controller.setLaserSpec(my_custom_laser);
```

## G-Code Commands

### Laser Control (M-Codes)

#### M3 - Laser On Clockwise (Continuous)
```gcode
M3 S1000        ; Turn on laser at "1000 RPM" (interpreted as power)
M3 S12000       ; 50% power (12000/24000)
M3 S24000       ; 100% power
```

#### M4 - Laser On Counter-clockwise (PWM Mode)
```gcode
M4 S1000        ; Turn on laser with PWM
```

#### M5 - Laser Off
```gcode
M5              ; Turn off laser
```

#### M106 - Set Laser Power (0-255)
```gcode
M106 S128       ; 50% power (128/255)
M106 S255       ; 100% power
M106 S0         ; Off
```

#### M107 - Laser Off
```gcode
M107            ; Turn off laser (same as M5)
```

### Laser-Specific Extended Commands

#### M460 - Set Laser Type
```gcode
M460 P0         ; CO2 40W
M460 P1         ; CO2 100W
M460 P2         ; Blue Diode 5W
M460 P3         ; Blue Diode 10W
M460 P4         ; Blue Diode 20W
M460 P5         ; Red Diode 500mW
M460 P6         ; Fiber 20W
M460 P7         ; Fiber 50W
M460 P8         ; Welder 1000W
M460 P9         ; Welder 2000W
```

#### M461 - Load Laser Profile
```gcode
M461 S"CO2_40W"          ; Load CO2 40W profile
M461 S"DIODE_BLUE_10W"   ; Load blue diode 10W profile
M461 S"FIBER_20W"        ; Load fiber 20W profile
M461 S"WELDER_1000W"     ; Load welder 1000W profile
```

#### M462 - Set Laser Power (Watts)
```gcode
M462 S5.0       ; Set to 5 watts
M462 S10.5      ; Set to 10.5 watts
```

#### M463 - Set Laser Power (Percent)
```gcode
M463 S50.0      ; Set to 50%
M463 S75.5      ; Set to 75.5%
```

#### M464 - Enable/Disable Ramping
```gcode
M464 S1         ; Enable power ramping
M464 S0         ; Disable power ramping
M464 S1 P50     ; Enable ramping at 50W/second
```

#### M465 - Set Pulse Mode
```gcode
M465 F5000 S50  ; 5kHz frequency, 50% duty cycle
M465 F10000 S25 ; 10kHz frequency, 25% duty cycle
```

#### M466 - Laser Safety Check
```gcode
M466            ; Check all safety interlocks
                ; Returns status of interlock, enclosure, air, water
```

#### M467 - Emergency Laser Stop
```gcode
M467            ; Immediate laser shutdown
```

## Usage Examples

### Example 1: Simple Wood Engraving (Blue Diode)

```gcode
; Setup
M461 S"DIODE_BLUE_10W"  ; Load 10W blue diode profile
M463 S30                ; Set 30% power for engraving
G0 F3000                ; Rapid move speed

; Engrave a square
G0 X0 Y0                ; Move to start (laser off)
M3 S7200                ; Laser on (30% = 7200/24000)
G1 X50 Y0 F1000         ; Engrave line at 1000mm/min
G1 X50 Y50
G1 X0 Y50
G1 X0 Y0
M5                      ; Laser off

G0 X0 Y0                ; Return home
```

### Example 2: Acrylic Cutting (CO2)

```gcode
; Setup for cutting 3mm acrylic
M461 S"CO2_40W"         ; Load CO2 40W profile
M463 S80                ; 80% power for cutting
G0 F5000                ; Fast positioning

; Enable air assist (external control)
M106 P1 S255            ; Turn on air assist fan

; Cut circle (simplified)
G0 X100 Y100            ; Position to center
G1 Z-3                  ; Lower to material surface
M3 S19200               ; Laser on (80% power)
G2 X100 Y100 I25 J0 F200 ; Cut 25mm radius circle at 200mm/min
M5                      ; Laser off
G0 Z10                  ; Raise head

M107 P1                 ; Air assist off
```

### Example 3: Metal Marking (Fiber Laser)

```gcode
; Setup for stainless steel marking
M461 S"FIBER_20W"       ; Load fiber 20W profile
M463 S100               ; 100% power for marking
M464 S0                 ; Disable ramping for sharp edges

; Check safety
M466                    ; Verify all interlocks

; Mark text (simplified - would use actual font data)
G0 X10 Y10
M3 S24000               ; Laser on full power
; ... actual marking moves ...
M5                      ; Laser off
```

### Example 4: Laser Welding

```gcode
; Setup for 1mm steel welding
M461 S"WELDER_1000W"    ; Load 1000W welder profile
M463 S60                ; 60% power (600W)
M464 S1 P100            ; Enable ramping at 100W/s

; Safety critical - verify all systems
M466                    ; Check interlocks

; Weld bead
G0 X0 Y0 Z0.5           ; Position above workpiece
G1 Z0                   ; Touch off
M3 S14400               ; Laser on (60% power, ramps up)
G1 X50 F100             ; Weld at 100mm/min
M5                      ; Laser off (ramps down)
G0 Z10                  ; Retract

; Cool down period
G4 P5000                ; Wait 5 seconds
```

## Material Settings

### Recommended Settings by Material

#### Wood (Blue Diode or CO2)

| Material | Laser | Power | Speed | Passes |
|----------|-------|-------|-------|--------|
| Basswood engrave | Blue 10W | 30-40% | 1000-2000 mm/min | 1 |
| Basswood cut 3mm | Blue 10W | 90-100% | 200-400 mm/min | 1-2 |
| Plywood engrave | CO2 40W | 20-30% | 2000-3000 mm/min | 1 |
| Plywood cut 3mm | CO2 40W | 70-90% | 300-500 mm/min | 1 |

#### Acrylic (CO2 only)

| Thickness | Power | Speed | Passes | Air Assist |
|-----------|-------|-------|--------|------------|
| 3mm cut | 80-90% | 150-250 mm/min | 1 | Required |
| 6mm cut | 90-100% | 80-120 mm/min | 1-2 | Required |
| 3mm engrave | 20-30% | 1500-2500 mm/min | 1 | Optional |

**Note:** Blue lasers don't cut clear acrylic (passes through). Use CO2.

#### Leather (Blue Diode or CO2)

| Operation | Laser | Power | Speed | Notes |
|-----------|-------|-------|-------|-------|
| Engrave | Blue 5W | 40-60% | 800-1200 mm/min | Light touch |
| Cut thin | Blue 10W | 80-100% | 200-400 mm/min | Max 2mm |
| Engrave | CO2 40W | 15-25% | 1500-2500 mm/min | Excellent detail |

#### Metal (Fiber only)

| Material | Operation | Power | Speed | Passes |
|----------|-----------|-------|-------|--------|
| Anodized aluminum | Mark | 80-100% | 1000-2000 mm/min | 1 |
| Stainless steel | Mark | 100% | 500-1000 mm/min | 1-2 |
| Brass | Mark | 70-90% | 800-1500 mm/min | 1 |
| Steel 1mm | Cut | 100% | 50-100 mm/min | 1 |

## Safety Guidelines

### ⚠️ CRITICAL SAFETY RULES

1. **NEVER operate laser without proper eye protection**
   - CO2: OD6+ at 10600nm
   - Blue diode: OD6+ at 445nm
   - Fiber/welder: OD7+ at 1064nm

2. **NEVER operate laser without enclosure**
   - Fire risk
   - Reflection hazards
   - Fume containment

3. **ALWAYS use fume extraction**
   - Toxic fumes from many materials
   - Especially critical: PVC, ABS, polycarbonate

4. **NEVER leave laser unattended**
   - Fire can start quickly
   - Keep fire extinguisher nearby

5. **TEST safety interlocks before operation**
   - Verify door switches work
   - Test emergency stop
   - Check water flow (if applicable)

### Materials NEVER to Laser Cut

❌ **DO NOT CUT:**
- PVC / Vinyl (releases chlorine gas - TOXIC)
- Polycarbonate (catches fire, releases toxic fumes)
- ABS (melts, terrible fumes)
- Fiberglass (glass particles, toxic resin)
- Coated/treated materials (unless verified safe)

## Troubleshooting

### Weak Cut/Engrave

**Possible Causes:**
1. Dirty lens/mirrors - Clean with proper lens cleaner
2. Out of focus - Adjust focus height
3. Power too low - Increase power setting
4. Speed too fast - Reduce feed rate
5. Tired laser tube - Check with power meter

### Burning/Charring

**Solutions:**
1. Reduce power
2. Increase speed
3. Add/increase air assist
4. Use masking tape on material

### Safety Interlock Triggering

**Check:**
1. Door switches properly aligned
2. Water flowing (check pump and filters)
3. Air pressure adequate
4. Wiring connections secure

### Inconsistent Power

**Possible Causes:**
1. PWM frequency incorrect for laser type
2. Power supply issues
3. Thermal throttling (needs cooling time)
4. Control signal interference

## Advanced Topics

### Custom PWM Frequencies

Different lasers respond better to different frequencies:

```cpp
// Low frequency for large CO2 tubes
laser_controller.setPulseMode(1000, 0.5);  // 1kHz

// High frequency for fast diodes
laser_controller.setPulseMode(25000, 0.5);  // 25kHz
```

### Power Ramping for Welding

Gradual power changes prevent thermal shock:

```cpp
laser_controller.enableRamping(true);
laser_controller.setRampRate(200.0);  // 200W per second
```

### Multiple Laser Heads

To support multiple lasers (e.g., marking + cutting):

```cpp
LaserController laser1(PWM1, EN1);  // Marking laser
LaserController laser2(PWM2, EN2);  // Cutting laser

// Switch between them with M452/M453 custom codes
```

## API Reference

See `include/laser_controller.h` for complete API documentation.

Key methods:
- `setLaserType(LaserType)` - Set laser type
- `setPower(float, PowerUnit)` - Set output power
- `fire()` / `stopFire()` - Control laser
- `checkSafety()` - Verify safety interlocks
- `emergencyStop()` - Immediate shutdown

---

## Support and Resources

- **Documentation**: See this guide and source code
- **Safety Datasheets**: Consult your laser manufacturer
- **Training**: Seek professional training for industrial lasers
- **Community**: GitHub discussions and issues

---

## Legal Disclaimer

⚠️ **Laser systems can cause serious injury or death if improperly used.**

- Follow all local laws and regulations
- Obtain required permits and training
- Use appropriate safety equipment
- Never bypass safety interlocks
- Keep fire extinguisher accessible
- Have emergency procedures in place

**The developers of this software are not responsible for injuries, property damage, or legal consequences resulting from use of this system. Use at your own risk.**

---

*Document Version 1.0 - December 2025*

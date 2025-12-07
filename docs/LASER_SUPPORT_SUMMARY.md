# Laser Support Summary

## Supported Laser Systems

The Encoder3D firmware provides comprehensive support for DIY and industrial laser systems:

### ✅ Fully Supported Laser Types

| Laser Type | Power Range | Typical Use | DIY Friendly | Status |
|------------|-------------|-------------|--------------|--------|
| **CO2 IR** | 40W - 100W+ | Wood, acrylic, leather cutting | ⭐⭐⭐ Moderate | ✅ Full Support |
| **Blue Diode** | 5W - 20W | Wood engraving, light cutting | ⭐⭐⭐⭐⭐ Very Easy | ✅ Full Support |
| **Red Diode** | 500mW - 5W | Light engraving, marking | ⭐⭐⭐⭐⭐ Very Easy | ✅ Full Support |
| **Fiber Laser** | 20W - 50W | Metal marking/cutting | ⭐⭐ Advanced | ✅ Full Support |
| **Laser Welder** | 1000W - 2000W | Industrial welding | ⭐ Professional | ✅ Full Support |

## Control Modes

All laser types support multiple control methods:

1. **PWM Control** - Standard pulse-width modulation (most common)
2. **TTL Control** - Simple on/off control
3. **Analog 0-10V** - Professional systems (requires DAC)
4. **Pulse Mode** - Configurable frequency and duty cycle

## Safety Features

### Hardware Interlocks
- ✅ Emergency stop / safety key
- ✅ Enclosure door sensors
- ✅ Air assist flow monitoring
- ✅ Water cooling flow sensors
- ✅ Beam detection (future)

### Software Safety
- ✅ Maximum duty cycle limiting
- ✅ Continuous fire time limits
- ✅ Power ramping (prevents thermal shock)
- ✅ Automatic shutdown on interlock failure
- ✅ Safety check before firing

## DIY Integration Examples

### Example 1: Add Blue Diode to 3D Printer
**Difficulty: Easy**

**Hardware Needed:**
- 5W-10W blue laser module (~$30-80)
- Mounting bracket
- Safety glasses (OD6+ @ 445nm)
- Optional: Air assist pump

**Setup:**
```cpp
// config.h
#define DEFAULT_LASER_PROFILE "DIODE_BLUE_5W"
#define LASER_PWM_PIN 32
#define LASER_ENABLE_PIN 14
```

**Usage:**
```gcode
M461 S"DIODE_BLUE_5W"  ; Load profile
M463 S50               ; 50% power
M3 S12000              ; Turn on
G1 X100 F1000          ; Move and engrave
M5                     ; Turn off
```

### Example 2: Add CO2 Laser to CNC Router
**Difficulty: Moderate**

**Hardware Needed:**
- 40W CO2 laser tube (~$150-300)
- Power supply
- Mirrors and lens
- Air assist system (required)
- Enclosure with extraction

**Setup:**
```cpp
// config.h
#define DEFAULT_LASER_PROFILE "CO2_40W"
#define LASER_AIR_ASSIST_REQUIRED true
#define LASER_ENCLOSURE_REQUIRED true
```

**Usage:**
```gcode
M461 S"CO2_40W"        ; Load CO2 profile
M466                   ; Check safety
M463 S80               ; 80% for cutting
M3 S19200              ; Turn on
; ... cutting moves ...
M5                     ; Turn off
```

### Example 3: K40 Laser Cutter Conversion
**Difficulty: Moderate**

**Hardware:**
- K40 laser cutter
- ESP32 board
- Optocoupler for laser control
- Safety sensors

**Benefits of Conversion:**
- WiFi control
- Better motion control
- Real-time monitoring
- Safety features
- Web interface

**Setup:**
```cpp
#define DEFAULT_LASER_PROFILE "CO2_40W"
#define LASER_INTERLOCK_ENABLED true
#define LASER_WATER_COOLING_REQUIRED true
```

### Example 4: Fiber Laser Marker
**Difficulty: Advanced**

**Hardware:**
- 20W fiber laser source (~$2000+)
- Galvo mirrors or gantry system
- Safety enclosure (required)
- Beam dump

**Setup:**
```cpp
#define DEFAULT_LASER_PROFILE "FIBER_20W"
#define LASER_INTERLOCK_ENABLED true
#define LASER_ENCLOSURE_REQUIRED true
```

**Safety Note:** Fiber lasers are **invisible infrared** and extremely dangerous. Professional training required.

## Popular DIY Laser Modules

### Budget-Friendly ($30-$100)
1. **5W Blue Diode**
   - Brands: NEJE, Ortur, AtomStack
   - Good for: Wood engraving, light cutting
   - Easy to integrate

2. **500mW Red Diode**
   - Good for: Paper engraving, marking
   - Very safe at low powers
   - Often used for alignment

### Mid-Range ($100-$500)
1. **10W-20W Blue Diode**
   - Brands: Opt Lasers, J Tech Photonics
   - Good for: Production engraving, cutting
   - Faster than lower power

2. **40W CO2 Tube**
   - Generic Chinese tubes
   - Good for: Acrylic, wood, leather
   - Requires full setup

### Professional ($500+)
1. **100W CO2**
   - Production cutting
   - Thick materials
   - Full enclosure needed

2. **Fiber Lasers**
   - Metal marking/cutting
   - Extremely precise
   - Professional only

## Software Features

### Power Control
```cpp
// Set by percentage
laser.setPowerPercent(50.0);

// Set by watts  
laser.setPowerWatts(5.0);

// Set by M-code
M463 S50  // 50%
M462 S5   // 5 watts
```

### Ramping (for Welding)
```cpp
// Gradual power changes
laser.enableRamping(true);
laser.setRampRate(100.0);  // 100W/second

// G-code
M464 S1 P100  // Enable ramping at 100W/s
```

### Pulse Mode
```cpp
// High-frequency pulsing
laser.setPulseMode(10000, 0.5);  // 10kHz, 50% duty

// G-code
M465 F10000 S50  // 10kHz @ 50%
```

## Material Compatibility

### Blue Diode Lasers
✅ **Works Well:**
- Wood (engraving and cutting)
- Plywood
- Cardboard
- Leather
- Painted metals
- Anodized aluminum
- Some plastics

❌ **Does NOT Work:**
- Clear/transparent acrylic (passes through)
- Metals (reflects)
- Glass (passes through)

### CO2 Lasers
✅ **Works Well:**
- All woods
- Acrylic (excellent)
- Leather
- Paper/cardboard
- Fabric
- Rubber
- Some ceramics

❌ **Does NOT Work:**
- Metals (reflects)
- PVC (toxic!)
- Polycarbonate (fire hazard)

### Fiber Lasers
✅ **Works Well:**
- All metals
- Anodized aluminum
- Stainless steel
- Brass/copper
- Plastics (marking)

❌ **Does NOT Work:**
- Transparent materials
- Highly reflective metals (dangerous)

## Pre-configured Profiles

The firmware includes these ready-to-use profiles:

```cpp
// Access via M461 command or API
LaserProfiles::CO2_40W
LaserProfiles::CO2_100W
LaserProfiles::DIODE_BLUE_5W
LaserProfiles::DIODE_BLUE_10W
LaserProfiles::DIODE_BLUE_20W
LaserProfiles::DIODE_RED_500MW
LaserProfiles::FIBER_20W
LaserProfiles::FIBER_50W
LaserProfiles::WELDER_1000W
LaserProfiles::WELDER_2000W
```

Each profile includes:
- Optimal PWM frequency
- Power range
- Wavelength
- Supported control modes
- Recommended focus distance

## Custom Profiles

Create your own profile:

```cpp
LaserSpec my_laser = {
    .type = LASER_CUSTOM,
    .max_power = 15.0,           // Your laser's max power
    .min_power = 0.5,
    .wavelength = 445.0,         // Wavelength in nm
    .min_pwm_freq = 1000,
    .max_pwm_freq = 50000,
    .optimal_pwm_freq = 20000,   // Tune for your laser
    .supports_ttl = true,
    .supports_analog = false,
    .supports_pwm = true,
    .focus_distance = 12.0,
    .spot_size = 0.08
};

laser_controller.setLaserSpec(my_laser);
```

## Integration Checklist

### Before Adding a Laser:

- [ ] Read laser safety guide
- [ ] Obtain proper eye protection (correct wavelength and OD rating)
- [ ] Plan enclosure/shielding
- [ ] Consider fume extraction
- [ ] Check local regulations
- [ ] Install fire suppression (extinguisher minimum)
- [ ] Plan emergency stop accessibility
- [ ] Test interlock systems

### Hardware Setup:

- [ ] Mount laser securely
- [ ] Connect PWM control signal
- [ ] Wire enable/disable
- [ ] Install safety interlocks
- [ ] Test water cooling (if applicable)
- [ ] Test air assist (if applicable)
- [ ] Verify focus mechanism
- [ ] Check for beam reflections

### Software Configuration:

- [ ] Select correct laser profile
- [ ] Configure control pins
- [ ] Set safety requirements
- [ ] Test power control
- [ ] Calibrate power levels
- [ ] Verify safety interlocks in software
- [ ] Test emergency stop

### First Tests:

- [ ] Low power (10%) on safe material
- [ ] Verify power control works
- [ ] Test safety interlocks
- [ ] Verify beam path is safe
- [ ] Check focus quality
- [ ] Monitor for smoke/fire
- [ ] Verify extraction working

## Troubleshooting

### Laser Won't Fire
1. Check enable signal
2. Verify safety interlocks
3. Check power supply
4. Verify M452 (laser mode) selected
5. Check M466 safety status

### Weak/Inconsistent Power
1. Check focus distance
2. Verify PWM frequency (try M465)
3. Clean optics
4. Check power supply voltage
5. Verify no thermal throttling

### Safety Interlock False Triggers
1. Check sensor wiring
2. Verify pull-up resistors
3. Test individual sensors
4. Check for electrical noise
5. Verify pin configuration

## Resources

- **Documentation:** `docs/LASER_GUIDE.md`
- **Source Code:** `include/laser_controller.h`
- **G-Code Reference:** `docs/GCODE_REFERENCE.md`
- **Configuration:** `include/config.h`

## Safety Warning

⚠️ **LASERS ARE DANGEROUS**

- Can cause instant permanent eye damage
- Can cause fires
- Can release toxic fumes
- Invisible beams (IR lasers) are especially dangerous
- Reflections can be as dangerous as direct beam

**ALWAYS:**
- Wear appropriate safety glasses
- Use in enclosed area
- Have fire extinguisher ready
- Never bypass safety interlocks
- Get proper training

---

**The developers are not responsible for injuries or property damage. Use at your own risk.**

---

*Support for additional laser types can be added via custom profiles. Contact the development team or submit a pull request for new laser profiles.*

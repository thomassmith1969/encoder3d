# CNC Tool Support Summary

## Supported CNC End Effectors

The Encoder3D firmware provides comprehensive support for virtually any CNC tool attachment:

---

## ✅ Spindles - Complete Support Matrix

| Type | Power | RPM Range | Cooling | Control Mode | Cost | DIY Level | Status |
|------|-------|-----------|---------|--------------|------|-----------|--------|
| **775 DC Motor** | 80-150W | 3K-15K | Air | PWM | $ | ⭐⭐⭐⭐⭐ | ✅ Full |
| **555 DC Motor** | 50-100W | 2K-12K | Air | PWM | $ | ⭐⭐⭐⭐⭐ | ✅ Full |
| **ER11 BLDC 300W** | 300W | 3K-12K | Air | ESC | $$ | ⭐⭐⭐⭐ | ✅ Full |
| **ER11 BLDC 500W** | 500W | 3K-15K | Air | ESC | $$ | ⭐⭐⭐⭐ | ✅ Full |
| **ER20 BLDC 1kW** | 1000W | 5K-18K | Air | ESC | $$$ | ⭐⭐⭐ | ✅ Full |
| **1.5kW VFD** | 1500W | 6K-24K | Water | 0-10V | $$$$ | ⭐⭐ | ✅ Full |
| **2.2kW VFD** | 2200W | 6K-24K | Water | 0-10V | $$$$$ | ⭐⭐ | ✅ Full |
| **3.0kW VFD** | 3000W | 8K-24K | Water | 0-10V | $$$$$ | ⭐ | ✅ Full |
| **Makita RT0700** | 710W | 10K-30K | Air | Relay | $$$ | ⭐⭐⭐⭐ | ✅ Full |
| **DeWalt 611** | 1HP | 16K-27K | Air | Relay | $$$ | ⭐⭐⭐⭐ | ✅ Full |

---

## ✅ Plasma Torches

| Type | Current | Start Method | Control | Applications | Status |
|------|---------|--------------|---------|--------------|--------|
| **CUT50 Pilot Arc** | 50A | Pilot Arc | TTL | Steel ≤12mm | ✅ Full |
| **CUT60 Pilot Arc** | 60A | Pilot Arc | TTL | Steel ≤16mm | ✅ Full |
| **Hypertherm 45** | 45A | Pilot Arc | TTL | Professional | ✅ Full |
| **HF Start** | Varies | High Freq | TTL | Budget systems | ✅ Full |
| **Blowback Start** | Varies | Touch | TTL | Simple systems | ✅ Full |

**Plasma Features:**
- ✅ Automatic torch height control (THC)
- ✅ Ohmic sensing for touch-off
- ✅ Pierce height & delay control
- ✅ Arc OK monitoring
- ✅ Air pressure interlock
- ✅ Material thickness compensation

---

## ✅ Other Tool Types

### Drag Knives
- **Use:** Vinyl cutting, paper, thin materials
- **Control:** Servo positioning
- **Status:** ✅ Full Support

### Pen Plotters
- **Use:** Drawing, plotting, artwork
- **Control:** Servo lift mechanism
- **Status:** ✅ Full Support

### Hot Wire Cutters
- **Use:** Foam cutting (EPS, XPS)
- **Control:** PWM power control
- **Features:** Temperature monitoring, duty cycle limiting
- **Status:** ✅ Full Support

### Vacuum Pickups
- **Use:** Pick-and-place operations
- **Control:** Vacuum pump + solenoid
- **Status:** ✅ Full Support

### Pneumatic Tools
- **Use:** Air-powered drills, engravers
- **Control:** Pressure regulation
- **Status:** ✅ Full Support

### 3D Printer Extruders
- **Use:** Basic 3D printing
- **Control:** Heater + thermistor
- **Status:** ⚠️ Basic support (use dedicated 3D printer firmware for full features)

---

## Control Modes Supported

The firmware supports **6 different control modes**:

1. **PWM Control** - Standard pulse-width modulation
   - DC motors, hot wires, drag knives
   - Configurable frequency (100Hz - 50kHz)

2. **0-10V Analog** - Professional standard
   - VFD spindles
   - Industrial controllers
   - Uses ESP32 DAC

3. **TTL On/Off** - Simple digital control
   - Plasma torches
   - Vacuum pumps
   - Relay-switched tools

4. **RC ESC** - Hobby brushless control
   - 1-2ms pulse at 50Hz
   - Brushless spindles
   - Self-calibrating

5. **Modbus RTU** - Industrial protocol
   - VFD spindles (RS485)
   - Parameter reading/writing
   - Future implementation

6. **Relay** - AC power switching
   - Routers (Makita, DeWalt)
   - High-power tools
   - IoT relay compatible

---

## Safety Systems

### Spindle Safety Features
- ✅ **Water Cooling Monitoring** - Flow sensors for VFD spindles
- ✅ **Temperature Monitoring** - Thermistor integration
- ✅ **RPM Feedback** - Tachometer support
- ✅ **Current Monitoring** - Overload detection
- ✅ **Duty Cycle Limiting** - Prevents overheating
- ✅ **Cooldown Timer** - Safe shutdown sequences
- ✅ **Interlock Support** - Emergency stop integration

### Plasma Safety Features
- ✅ **Air Pressure Interlock** - Won't start without air
- ✅ **Arc OK Monitoring** - Detects arc loss
- ✅ **Torch Height Sensing** - Prevents crashes
- ✅ **Ohmic Sensing** - Material touch-off
- ✅ **Consumable Life Tracking** - Maintenance alerts
- ✅ **Fume Extraction Interlock** - Safety requirement

---

## Quick Comparison: Which Tool for Your Project?

### PCB Milling
**Best:** 555 DC Motor or ER11 300W  
**Why:** Precision, low runout, quiet

### Wood Engraving
**Best:** 775 DC Motor or Makita RT0700  
**Why:** Good power, affordable, proven

### Aluminum Milling
**Best:** ER11 500W or 1.5kW VFD  
**Why:** Power needed, cooling, rigidity

### Steel Milling
**Best:** 2.2kW+ VFD  
**Why:** High torque, water-cooled, professional

### Steel Cutting
**Best:** Plasma CUT50/60  
**Why:** Fast, thick material capability

### Vinyl Cutting
**Best:** Drag Knife  
**Why:** Purpose-built, clean cuts

### Foam Shaping
**Best:** Hot Wire  
**Why:** Smooth cuts, no dust

---

## DIY Integration Examples

### Example 1: Convert 3D Printer to CNC Mill
**Add:** 775 DC spindle + L298N driver  
**Cost:** ~$25  
**Difficulty:** Easy  
**Materials:** Soft wood, PCB, acrylic

### Example 2: Add Plasma to Router
**Add:** CUT50 plasma + THC  
**Cost:** ~$500  
**Difficulty:** Moderate  
**Materials:** Steel up to 12mm

### Example 3: Multi-Tool Setup
**Tools:** Spindle + Laser + Drag Knife  
**Switch:** Quick-change tool holder  
**Difficulty:** Advanced  
**Benefit:** One machine, multiple processes

---

## Performance Specifications

### DC Spindle (775 Motor)
```
Power: 80-150W
RPM: 3,000 - 15,000
Torque: ~0.1 Nm
Runout: 0.05-0.1mm
Noise: 70-80 dB
Life: 500-1000 hours
Materials: Wood, PCB, soft plastics
Max Cut Depth: 2-3mm per pass
```

### Brushless Spindle (ER11 500W)
```
Power: 500W
RPM: 3,000 - 15,000
Torque: ~0.3 Nm
Runout: 0.01-0.02mm
Noise: 50-60 dB
Life: 2000-5000 hours
Materials: Wood, aluminum, plastics
Max Cut Depth: 3-5mm per pass
```

### VFD Spindle (2.2kW)
```
Power: 2200W
RPM: 6,000 - 24,000
Torque: 0.9 Nm @ 24k RPM
Runout: <0.005mm
Noise: 40-50 dB
Life: 10,000+ hours
Materials: All metals, wood, plastics
Max Cut Depth: 5-8mm per pass (steel)
```

### Plasma Torch (CUT50)
```
Power: 50A @ 220V
Cut Capacity: 12mm mild steel
Pierce Capacity: 8mm
Cut Speed: 1000-4000 mm/min
Kerf Width: 1.5-2mm
Duty Cycle: 60% @ max
Consumable Life: 100-200 starts
Air Pressure: 60-80 PSI
Air Consumption: 4-5 CFM
```

---

## Material Compatibility

### Spindle Applications

**775 DC Motor:**
- ✅ Soft wood (pine, balsa)
- ✅ PCB (FR4, copper clad)
- ✅ Soft plastics (acrylic, HDPE)
- ✅ Wax, foam
- ❌ Hard wood (oak, maple) - too slow
- ❌ Aluminum - not enough power
- ❌ Steel - impossible

**ER11 BLDC 500W:**
- ✅ All woods
- ✅ PCB
- ✅ All plastics
- ✅ Aluminum (with care)
- ✅ Brass, copper
- ⚠️ Steel (very light cuts only)

**2.2kW VFD:**
- ✅ All woods
- ✅ All plastics
- ✅ Aluminum (production speeds)
- ✅ Brass, copper
- ✅ Mild steel
- ✅ Stainless steel
- ✅ Titanium (with care)

### Plasma Applications

**CUT50 (50A):**
- ✅ Mild steel: 1-12mm
- ✅ Stainless steel: 1-10mm
- ✅ Aluminum: 1-8mm
- ❌ Copper - difficult, poor quality
- ❌ Brass - difficult

---

## Wiring Diagrams

### DC Spindle (PWM Control)
```
ESP32        L298N         Spindle
-----        ------        -------
Pin 32 ───→ ENA           
Pin 33 ───→ IN1           
Pin 14 ───→ IN2           
              │            
         OUT1 ├───────→ Motor +
         OUT2 ├───────→ Motor -
              │
12-24V ──→ +12V
GND ─────→ GND
```

### BLDC Spindle (ESC Control)
```
ESP32        ESC          Spindle
-----        ----         -------
Pin 32 ───→ Signal       
GND ──────→ Ground       
              │           
24-48V ───→ Power        
              │           
         Motor U ──────→ Phase U
         Motor V ──────→ Phase V
         Motor W ──────→ Phase W
```

### VFD Spindle (0-10V Control)
```
ESP32       Isolator      VFD
-----       --------      ---
DAC 25 ───→ Input        
GND ──────→ GND          
              │           
         Output ────────→ AI1 (0-10V)
              │           
              └─────────→ GND
```

### Plasma Torch
```
ESP32           Plasma Cutter
-----           -------------
Pin 14 ──────→ CNC Start
Pin 27 ←────── Arc OK
Pin 26 ←────── Air Pressure OK
GND ──────────→ GND

Optional THC:
Pin 32 (PWM) ─→ THC Up/Down
Pin 35 (ADC) ←─ Arc Voltage (via divider)
```

---

## G-Code Quick Reference

### Load Tool Profile
```gcode
M471 S"DC_775_12V"         ; Load DC motor
M471 S"BLDC_ER11_500W"     ; Load brushless
M471 S"VFD_2_2KW_WATER"    ; Load VFD
M471 S"PLASMA_CUT50_PILOT" ; Load plasma
```

### Start Tool
```gcode
M3 S12000    ; Start spindle at 12000 RPM (CW)
M4 S10000    ; Start spindle at 10000 RPM (CCW)
M3           ; Start plasma torch
```

### Stop Tool
```gcode
M5           ; Stop tool (with cooldown if configured)
M476         ; Emergency stop (immediate)
```

### Safety Check
```gcode
M475         ; Check all safety interlocks
M478         ; Check plasma safety (air, arc)
```

### Plasma Operations
```gcode
M477 S1.5    ; Set torch height to 1.5mm
M479         ; Auto pierce sequence
```

---

## Custom Tool Creation

### Define Your Own Tool

```cpp
ToolSpec my_custom_tool = {
    .type = TOOL_CUSTOM,
    .name = "My Special Tool",
    .max_speed = 10000.0,
    .min_speed = 1000.0,
    .idle_speed = 2000.0,
    .control_mode = CONTROL_PWM,
    .pwm_frequency = 20000,
    .analog_min_voltage = 0.0,
    .analog_max_voltage = 24.0,
    .max_current = 3.0,
    .rated_voltage = 24.0,
    .collet_size = 3.175,
    .spinup_time = 1000,
    .spindown_time = 2000,
    .min_on_time = 100,
    .cooldown_time = 5000,
    .safety = {
        .requires_coolant = false,
        .requires_air_assist = true,
        .requires_torch_height = false,
        .requires_interlock = false,
        .has_tachometer = false,
        .has_temperature_sensor = true,
        .has_current_sensor = false,
        .requires_fume_extraction = false
    },
    .pierce_height = 0,
    .cut_height = 0,
    .pierce_delay = 0,
    .max_temperature = 80.0,
    .max_duty_cycle = 100
};

tool_controller.setToolSpec(my_custom_tool);
```

---

## Budget Planning

### Entry Level ($50-150)
- 775 DC motor + driver: $25
- ER11 300W brushless + ESC: $80
- Basic drag knife: $30
- **Best for:** Learning, soft materials

### Mid-Range ($150-500)
- ER11 500W brushless: $120
- ER20 1kW brushless: $200
- Makita RT0700: $120
- **Best for:** Serious hobby, production

### Professional ($500-2000)
- 1.5kW VFD + spindle: $600
- 2.2kW VFD + spindle: $800
- CUT50 plasma + THC: $600
- **Best for:** Business, steel work

---

## Troubleshooting Quick Reference

| Problem | Check | Solution |
|---------|-------|----------|
| Won't start | M475 safety | Fix interlock issue |
| Speed erratic | PWM frequency | Adjust M474 |
| Overheating | Coolant flow | Check M475 |
| VFD no response | 0-10V signal | Verify DAC output |
| Plasma no arc | Air pressure | Check M478 |
| Plasma arc lost | Consumables | Replace tip |
| Spindle vibration | Collet/bit | Check runout |
| ESC beeping | Calibration | Recalibrate ESC |

---

## Upgrade Paths

### Path 1: Budget → Performance
1. Start: 775 DC ($25)
2. Upgrade: ER11 500W BLDC ($120)
3. Final: 1.5kW VFD ($600)

### Path 2: 3D Printer → CNC
1. Start: 3D printer only
2. Add: 775 DC spindle ($25)
3. Add: Drag knife ($30)
4. Add: Laser module ($80)
5. Result: Multi-function machine

### Path 3: Router → Plasma
1. Start: CNC router with spindle
2. Add: CUT50 plasma ($400)
3. Add: THC ($200)
4. Result: Router + plasma table

---

## Resources

- **Documentation:** `docs/TOOL_GUIDE.md`
- **Source Code:** `include/tool_controller.h`
- **Implementation:** `src/tool_controller.cpp`
- **G-Code Reference:** `docs/GCODE_REFERENCE.md`
- **Configuration:** `include/config.h`

---

## Safety Warnings

### ⚠️ SPINDLES
- Rotating tools can cause severe injury
- Always wear eye protection
- Secure workpiece properly
- Never reach over spinning tool
- Disconnect power before changing bits

### ⚠️ PLASMA TORCHES
- Intense UV radiation - blindness risk
- Arc flash can cause severe burns
- Metal fumes are toxic
- Fire hazard - have extinguisher ready
- Never cut galvanized steel (toxic zinc)
- Proper ventilation required

### ⚠️ HIGH VOLTAGE
- VFD spindles use 220V AC - lethal voltage
- Only qualified electricians should wire AC
- Use proper circuit breakers
- Ensure proper grounding
- Use RCD/GFCI protection

---

**The developers are not responsible for injuries, property damage, or equipment damage.**  
**Use at your own risk. Professional installation recommended for high-voltage systems.**

---

*For additional tool support, submit feature requests or contribute profiles to the repository.*

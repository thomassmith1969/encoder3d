# Encoder3D: Complete End Effector Support

## Overview

The Encoder3D firmware transforms your ESP32-based CNC controller into a **universal machine control platform** supporting virtually any end effector from budget DIY tools to professional industrial equipment.

---

## 🎯 Supported End Effector Categories

### 1. ✅ Lasers (10 Types)
From hobby diodes to industrial welders

### 2. ✅ Spindles (10 Types)  
From $5 DC motors to 3kW VFD systems

### 3. ✅ Plasma Torches (5 Types)
Budget to professional CNC plasma cutting

### 4. ✅ Specialty Tools (6+ Types)
Drag knives, plotters, hot wire, vacuum, and more

---

## 📊 Quick Comparison Matrix

| Category | Power Range | Cost Range | DIY Level | Status |
|----------|-------------|------------|-----------|--------|
| **Lasers** | 500mW - 2000W | $30 - $5000 | ⭐-⭐⭐⭐⭐⭐ | ✅ Full |
| **Spindles** | 50W - 3000W | $5 - $2000 | ⭐-⭐⭐⭐⭐⭐ | ✅ Full |
| **Plasma** | 40A - 60A+ | $300 - $3000 | ⭐⭐-⭐⭐⭐⭐ | ✅ Full |
| **Other Tools** | Varies | $5 - $500 | ⭐⭐⭐⭐⭐ | ✅ Full |

---

## 🔦 Laser Support Detail

### Types Supported
1. **CO2 IR Lasers** (40W, 100W)
   - Wood, acrylic, leather cutting
   - Popular in K40 conversions

2. **Blue Diode Lasers** (5W, 10W, 20W)
   - Most popular DIY laser
   - Wood engraving, light cutting
   - Brands: NEJE, Ortur, AtomStack

3. **Red Diode Lasers** (500mW)
   - Light engraving, alignment
   - Very safe, affordable

4. **Fiber Lasers** (20W, 50W)
   - Metal marking/engraving
   - Professional applications

5. **Laser Welders** (1000W, 2000W)
   - Industrial metal welding
   - High-power applications

### Laser Features
- ✅ PWM/TTL/Analog (0-10V) control
- ✅ Power ramping (prevents flash-burns)
- ✅ Pulse mode (frequency + duty cycle)
- ✅ 4-layer safety interlocks
- ✅ Multiple power units (%, watts, mW)
- ✅ Material-specific profiles
- ✅ G-code commands M460-M467

### Laser Applications
- Wood engraving & cutting
- Acrylic cutting (CO2 only)
- Leather engraving
- Metal marking (fiber only)
- Metal welding (laser welders)
- PCB etching

**Documentation:** [docs/LASER_GUIDE.md](docs/LASER_GUIDE.md)

---

## 🛠️ Spindle Support Detail

### Types Supported

#### Budget DC Motors
1. **775 Motor** (12V/24V) - $5-15
   - 3,000-15,000 RPM
   - Perfect for: PCB milling, soft wood
   - DIY Level: ⭐⭐⭐⭐⭐

2. **555 Motor** (12V) - $5-10
   - 2,000-12,000 RPM
   - Perfect for: PCB, Dremel-style work

#### Brushless Spindles
3. **ER11 300W** - $60-100
   - 3,000-12,000 RPM
   - Perfect for: Wood, soft aluminum

4. **ER11 500W** - $100-150
   - 3,000-15,000 RPM
   - Perfect for: Production wood, aluminum

5. **ER20 1000W** - $180-250
   - 5,000-18,000 RPM
   - Perfect for: Heavy aluminum work

#### VFD Water-Cooled (Professional)
6. **1.5kW VFD** - $500-700
   - 6,000-24,000 RPM
   - Perfect for: Aluminum, mild steel

7. **2.2kW VFD** - $700-1000
   - 6,000-24,000 RPM
   - Perfect for: Steel production work

8. **3.0kW VFD** - $1000-1500
   - 8,000-24,000 RPM
   - Perfect for: Heavy steel milling

#### Popular Router Conversions
9. **Makita RT0700** - $100-150
   - 10,000-30,000 RPM
   - Most popular worldwide DIY choice

10. **DeWalt 611** - $120-180
    - 16,000-27,000 RPM
    - Popular in North America

### Spindle Control Modes
- ✅ PWM (DC motors, brushless)
- ✅ RC ESC (brushless spindles)
- ✅ 0-10V Analog (VFD spindles)
- ✅ Relay switching (routers)
- ✅ Modbus RTU (future - VFD)

### Spindle Safety Features
- ✅ Water cooling flow monitoring
- ✅ Temperature monitoring
- ✅ RPM tachometer feedback
- ✅ Current overload detection
- ✅ Duty cycle limiting
- ✅ Controlled ramp up/down
- ✅ Cooldown timer

**Documentation:** [docs/TOOL_GUIDE.md](docs/TOOL_GUIDE.md)

---

## ⚡ Plasma Torch Support

### Types Supported
1. **CUT50 Pilot Arc** (~$300-400)
   - 50A cutting capacity
   - Cuts steel up to 12mm
   - Pilot arc = no touch start

2. **CUT60 Pilot Arc** (~$400-500)
   - 60A cutting capacity
   - Cuts steel up to 16mm

3. **Hypertherm Powermax 45** (~$1500-2000)
   - Professional quality
   - Better duty cycle
   - Superior cut quality

4. **HF Start Systems**
   - High-frequency start
   - Budget systems

5. **Blowback Start**
   - Touch-start systems
   - Simple, reliable

### Plasma Features
- ✅ Automatic Torch Height Control (THC)
- ✅ Ohmic sensing (touch-off)
- ✅ Pierce height & delay control
- ✅ Arc OK monitoring
- ✅ Air pressure interlock
- ✅ Consumable life tracking
- ✅ G-code commands M477-M479

### Plasma Applications
- Mild steel cutting (1-16mm)
- Stainless steel cutting
- Aluminum cutting
- Artistic metalwork
- Production cutting

### Material Capacity (CUT50)
| Material | Max Thickness | Cut Speed |
|----------|---------------|-----------|
| Mild Steel | 12mm | 1000-4000mm/min |
| Stainless | 10mm | 800-2500mm/min |
| Aluminum | 8mm | 1500-3500mm/min |

**Documentation:** [docs/TOOL_GUIDE.md](docs/TOOL_GUIDE.md)

---

## 🎨 Other Tool Types

### 1. Drag Knife
- **Use:** Vinyl cutting, paper, cardboard
- **Control:** Servo positioning
- **Cost:** $30-100
- **Status:** ✅ Full Support

### 2. Pen Plotter  
- **Use:** Drawing, plotting, artwork
- **Control:** Servo lift
- **Cost:** $10-50
- **Status:** ✅ Full Support

### 3. Hot Wire Cutter
- **Use:** Foam cutting (EPS, XPS)
- **Control:** PWM power + temperature
- **Cost:** $50-200
- **Status:** ✅ Full Support

### 4. Vacuum Pickup
- **Use:** Pick-and-place operations
- **Control:** Vacuum pump + solenoid
- **Cost:** $50-150
- **Status:** ✅ Full Support

### 5. Pneumatic Tools
- **Use:** Air drills, engravers
- **Control:** Pressure regulator
- **Cost:** Varies
- **Status:** ✅ Full Support

### 6. 3D Printer Extruder
- **Use:** Basic 3D printing
- **Control:** Heater + stepper
- **Cost:** Varies
- **Status:** ⚠️ Basic (use dedicated firmware for full features)

---

## 🎛️ Control Capabilities

### 6 Control Modes Supported

1. **PWM (Pulse Width Modulation)**
   - Frequency: 100Hz - 50kHz (configurable)
   - Resolution: 8-bit (0-255)
   - Used for: DC motors, lasers, hot wire

2. **0-10V Analog**
   - Uses ESP32 DAC
   - Used for: VFD spindles, professional tools

3. **TTL On/Off**
   - Simple digital control
   - Used for: Plasma, vacuum, relays

4. **RC ESC (1-2ms pulse)**
   - 50Hz servo-style control
   - Used for: Brushless spindles

5. **Modbus RTU** (future)
   - RS485 industrial protocol
   - Used for: VFD parameter control

6. **Relay Switching**
   - AC power control
   - Used for: Routers, high-power tools

---

## 📋 G-Code Command Summary

### Universal Commands
```gcode
M3 S<speed>   ; Start tool (CW for spindles)
M4 S<speed>   ; Start tool (CCW for spindles)
M5            ; Stop tool
```

### Laser Commands (M460-M467)
```gcode
M461 S"DIODE_BLUE_5W"  ; Load laser profile
M463 S50               ; Set 50% power
M466                   ; Safety check
M467                   ; Emergency stop
```

### Tool Commands (M470-M479)
```gcode
M471 S"VFD_2_2KW_WATER"  ; Load spindle profile
M472 S18000              ; Set 18000 RPM
M475                     ; Safety check (coolant, air, temp)
M476                     ; Emergency stop
M478                     ; Check plasma safety
M479                     ; Plasma pierce sequence
```

---

## 💡 Use Case Examples

### Use Case 1: Budget CNC Router
**Budget:** $100-200  
**Tools:** 
- 775 DC spindle ($15)
- 5W blue laser ($80)
- Drag knife ($30)

**Result:** Multi-function machine for wood, engraving, vinyl

---

### Use Case 2: Aluminum Milling
**Budget:** $500-800  
**Tools:**
- ER20 1kW brushless ($200)
- Air blast system ($50)
- Tachometer ($20)

**Result:** Production-quality aluminum work

---

### Use Case 3: Plasma Cutting Table
**Budget:** $800-1200  
**Tools:**
- CUT50 plasma torch ($400)
- Torch height controller ($200)
- Slat table ($200)

**Result:** Steel cutting up to 12mm

---

### Use Case 4: Professional Multi-Tool CNC
**Budget:** $2000-3000  
**Tools:**
- 2.2kW VFD spindle ($800)
- 20W fiber laser ($1500)
- Automatic tool changer ($500)

**Result:** Steel milling + metal marking

---

## 🔒 Safety Systems

### Hardware Safety
- ✅ Emergency stop integration
- ✅ Safety interlocks (4 types for lasers)
- ✅ Coolant flow monitoring (VFD)
- ✅ Air pressure monitoring (plasma)
- ✅ Temperature monitoring
- ✅ Current monitoring

### Software Safety
- ✅ Alarm system integration
- ✅ Health scoring (0-100%)
- ✅ Automatic shutdown on fault
- ✅ Duty cycle limiting
- ✅ Cooldown timers
- ✅ Safety checks before operation

### Safety Commands
```gcode
M466  ; Laser safety check
M475  ; Tool safety check
M478  ; Plasma safety check
M476  ; Emergency tool stop
M467  ; Emergency laser stop
```

---

## 📈 Upgrade Paths

### Path 1: 3D Printer → Multi-Tool
1. Start: 3D printer firmware
2. Add: 775 DC spindle ($15)
3. Add: 5W laser ($80)
4. Add: Drag knife ($30)
**Total:** $125 for 4 processes

### Path 2: Hobby → Professional CNC
1. Start: 775 DC ($15)
2. Upgrade: ER11 BLDC ($120)
3. Upgrade: 1.5kW VFD ($600)
**Growth:** From hobby to professional

### Path 3: Router → Plasma Table
1. Start: CNC router with spindle
2. Add: CUT50 plasma ($400)
3. Add: THC ($200)
**Result:** Wood + metal capability

---

## 🎓 Skill Level Requirements

### Beginner (⭐⭐⭐⭐⭐)
- 775 DC motor spindle
- 5W blue diode laser
- Drag knife
- Pen plotter

### Intermediate (⭐⭐⭐)
- Brushless spindles (ER11/ER20)
- 10-20W blue lasers
- Hot wire cutter
- Basic plasma (with guidance)

### Advanced (⭐⭐)
- VFD water-cooled spindles
- Fiber lasers
- Professional plasma with THC
- Custom tool integration

### Expert (⭐)
- High-power laser welders
- 3kW+ spindles
- Multi-tool automatic changers
- Industrial automation

---

## 📚 Documentation Structure

### Quick Start
1. **LASER_SUPPORT_SUMMARY.md** - Quick laser overview
2. **TOOL_SUPPORT_SUMMARY.md** - Quick tool overview

### Detailed Guides
1. **LASER_GUIDE.md** - Complete laser guide (485 lines)
   - All 10 laser types
   - Material settings
   - Safety rules
   - Configuration examples

2. **TOOL_GUIDE.md** - Complete tool guide (extensive)
   - All spindle types
   - Plasma torch setup
   - Specialty tools
   - Wiring diagrams

### Technical Reference
1. **GCODE_REFERENCE.md** - All commands
2. **include/laser_controller.h** - Laser API
3. **include/tool_controller.h** - Tool API

---

## 🔧 Configuration Examples

### Laser Configuration
```cpp
// config.h
#define DEFAULT_LASER_PROFILE "DIODE_BLUE_5W"
#define LASER_PWM_PIN 32
#define LASER_ENABLE_PIN 14
#define LASER_INTERLOCK_PIN 27
```

### Spindle Configuration
```cpp
// config.h
#define DEFAULT_TOOL_PROFILE "VFD_2_2KW_WATER"
#define TOOL_ANALOG_PIN 25      // DAC for 0-10V
#define TOOL_COOLANT_PIN 26     // Flow sensor
#define TOOL_TEMP_PIN 35        // Temperature
```

### Plasma Configuration
```cpp
// config.h
#define DEFAULT_TOOL_PROFILE "PLASMA_CUT50_PILOT"
#define PLASMA_ARC_START_PIN 14
#define PLASMA_ARC_OK_PIN 27
#define PLASMA_AIR_PIN 26
#define PLASMA_PIERCE_HEIGHT 3.8
#define PLASMA_CUT_HEIGHT 1.5
```

---

## 🌟 Key Advantages

### 1. Universal Platform
One controller for multiple tools - no rewiring needed

### 2. Budget Friendly
Support for $5 motors to $5000 lasers

### 3. Safety First
Multiple layers of protection for dangerous tools

### 4. Easy Upgrades
Start cheap, upgrade as you go

### 5. Open Source
Community-driven development, free to use

### 6. Proven Profiles
20+ predefined profiles ready to use

### 7. Flexible Control
6 different control modes supported

### 8. Professional Features
VFD spindles, plasma THC, fiber lasers

---

## ⚠️ Safety Warnings

### Lasers
- Can cause instant permanent blindness
- Fire hazard
- Require proper eye protection (OD6+)
- Never bypass interlocks

### Spindles
- Rotating tools can cause severe injury
- Flying debris hazard
- Proper bit securing critical
- Emergency stop required

### Plasma Torches
- Arc flash causes severe burns
- UV radiation hazard
- Toxic metal fumes
- Fire hazard
- Requires ventilation

### High Voltage
- VFD spindles use 220V AC (lethal)
- Professional installation required
- Proper grounding essential
- RCD/GFCI protection required

**The developers are not responsible for injuries or property damage. Use at your own risk.**

---

## 🚀 Getting Started

### Step 1: Choose Your Tool
Review the comparison matrices and decide what you need

### Step 2: Read the Guide
- Lasers: [docs/LASER_GUIDE.md](docs/LASER_GUIDE.md)
- Spindles/Plasma: [docs/TOOL_GUIDE.md](docs/TOOL_GUIDE.md)

### Step 3: Gather Hardware
Follow the wiring diagrams in the guides

### Step 4: Configure
Set appropriate pins and profile in config.h

### Step 5: Safety Check
Use M466/M475/M478 commands to verify safety

### Step 6: Test
Start with low power, safe materials

---

## 📞 Support

- **Documentation:** See links above
- **GitHub Issues:** For bugs and features
- **Community:** Discussions tab
- **Safety:** Read ALL warnings before use

---

**Built for the DIY community, tested by professionals, ready for production.**

*Transform your CNC into a multi-tool powerhouse.* 🚀

# SD Card Wiring Guide

This guide explains how to directly solder a microSD-to-SD adapter to the ESP32 for G-code file storage.

## Overview

The ESP32 communicates with SD cards using the SPI (Serial Peripheral Interface) protocol. By soldering a microSD adapter directly to the ESP32, you can store and execute G-code files without a USB connection.

## Materials Needed

- **MicroSD-to-SD adapter sleeve**: A spare adapter dedicated to this project
- **Thin stranded wire**: For flexible, reliable connections (22-26 AWG recommended)
- **Soldering iron and solder**: Lead-free solder requires higher temperatures (~350°C)
- **Wire strippers and flux**: For clean connections
- **MicroSD card**: Formatted as FAT32, Class 10 or better recommended
- **Hot glue or epoxy**: To strain-relieve the connections

## ESP32 SPI Pinout

The firmware uses the default ESP32 hardware SPI pins for maximum compatibility:

| SD Card Function | SD Pin No. | ESP32 GPIO | Wire Color Suggestion |
|-----------------|------------|------------|----------------------|
| CS (Chip Select) | 1 | GPIO 5 | Yellow |
| DI (MOSI - Data In) | 2 | GPIO 23 | Green |
| VSS (Ground) | 3 | GND | Black |
| VCC (3.3V Power) | 4 | 3.3V | Red |
| CLK (Clock) | 5 | GPIO 18 | Blue |
| VSS (Ground) | 6 | GND | Black |
| DO (MISO - Data Out) | 7 | GPIO 19 | Orange |
| RSV (Reserved) | 8 | No connection | - |

**Note**: Pin numbers refer to the full-size SD card contact pads on the adapter, not the microSD slot. Refer to SD card pinout diagrams when soldering.

## Configuration

The pins are defined in `include/config.h`:

```cpp
#define SD_CS_PIN 5
#define SD_MOSI_PIN 23
#define SD_MISO_PIN 19
#define SD_CLK_PIN 18
#define SD_ENABLED true
```

If you need to use different pins, modify these values before compiling.

## Soldering Instructions

### 1. Prepare the Adapter

- Carefully open the SD adapter casing (gently pry with a flathead screwdriver in the microSD slot)
- Expose the internal metal contact pads
- Clean the pads with isopropyl alcohol

### 2. Prepare Wires

- Cut 7 wires to ~30cm length (adjust based on your enclosure)
- Strip 2-3mm of insulation from each end
- "Tin" both ends by applying a small amount of solder
- Label wires with tape or heat shrink according to the color suggestions above

### 3. Solder to Adapter

- Apply flux to each pad on the adapter
- Carefully solder tinned wire ends to the pads
- Work quickly to avoid overheating the adapter
- Check for short circuits between adjacent pads with a multimeter
- Ensure good mechanical connections (gently tug each wire)

### 4. Solder to ESP32

Connect wires to the ESP32 Lolin32 Lite:

- **GPIO 5** (CS) - Usually labeled on the board
- **GPIO 23** (MOSI) - SPI data out from ESP32
- **GPIO 19** (MISO) - SPI data in to ESP32
- **GPIO 18** (CLK) - SPI clock
- **GND** - Connect both ground wires to GND
- **3.3V** - Power supply (NOT 5V!)

### 5. Strain Relief

- Use hot glue or epoxy to secure wires to the adapter housing
- Prevent stress on the solder joints
- Route wires neatly to avoid interference with moving parts

## MicroSD Card Preparation

1. **Format as FAT32**: 
   - Most cards come pre-formatted
   - For cards >32GB, use a tool like SD Card Formatter
   
2. **File Structure**:
   ```
   /
   ├── test.gcode
   ├── calibration.gcode
   └── prints/
       ├── part1.gcode
       └── part2.gcode
   ```

3. **File Naming**:
   - Use standard 8.3 naming (filename.ext) for best compatibility
   - Avoid spaces and special characters
   - Use .gcode or .g extensions

## G-Code Commands

The firmware supports standard SD card G-codes:

| Command | Description | Example |
|---------|-------------|---------|
| **M20** | List files | `M20` |
| **M21** | Initialize SD card | `M21` |
| **M22** | Release SD card | `M22` |
| **M23** | Select file | `M23 /test.gcode` |
| **M24** | Start/resume print | `M24` |
| **M25** | Pause print | `M25` |
| **M27** | Report print status | `M27` |
| **M30** | Delete file | `M30 /old.gcode` |

## Usage Examples

### Print a File

```gcode
M21              ; Initialize SD card
M20              ; List files
M23 /test.gcode  ; Select file
M24              ; Start printing
```

### Check Progress

```gcode
M27              ; Returns percentage complete
```

### Pause and Resume

```gcode
M25              ; Pause
; ... do something ...
M24              ; Resume
```

## Web Interface Integration

The web interface includes SD card file management:

- **Upload G-code files** via the web UI
- **Browse and select files** from the SD card
- **Start/pause/stop** prints remotely
- **Monitor progress** in real-time

Files uploaded through the web interface are saved directly to the SD card.

## Troubleshooting

### Card Not Detected

- Check 3.3V power supply voltage
- Verify all solder connections
- Ensure card is formatted as FAT32
- Try a different microSD card (some are incompatible)

### Read/Write Errors

- Check for loose connections
- Verify wire routing doesn't cross high-current motor traces
- Try lower SPI speed (modify in code if needed)
- Ensure card is not write-protected

### Files Not Listed

- Check file system format (must be FAT32)
- Verify files are in root directory or accessible path
- Check filename compatibility (8.3 format recommended)

### Print Starts Then Stops

- Check for corrupt G-code files
- Verify sufficient power supply for ESP32
- Monitor serial output for error messages
- Ensure motor commands are non-blocking

## Performance Considerations

- **Read speed**: ~1-2 MB/s typical
- **Command rate**: Limited by G-code execution speed (~10 lines/sec)
- **Buffering**: 128-byte chunks read non-blocking
- **File size**: No limit (100MB max configured, adjustable)

## Safety Notes

- **Never use 5V** for SD card power (will damage card and possibly ESP32)
- **Short circuits** can damage the ESP32 or SD card permanently
- **Static electricity** can damage the SD card - use ESD precautions
- **Test connections** with multimeter before powering on
- **Strain relief** is critical - broken solder joints are difficult to repair

## Advanced: Alternative Pins

If you need to use different GPIO pins (e.g., due to conflicts), you can configure alternative SPI pins. Modify `config.h`:

```cpp
#define SD_CS_PIN 15    // Any GPIO
#define SD_MOSI_PIN 13  // Hardware SPI recommended
#define SD_MISO_PIN 12  // Hardware SPI recommended
#define SD_CLK_PIN 14   // Hardware SPI recommended
```

**Note**: Using non-hardware SPI pins may reduce performance.

## Testing

After wiring, test the SD card:

1. Insert formatted microSD card
2. Upload firmware
3. Open serial monitor (115200 baud)
4. Look for "SD Card initialized" message
5. Send `M21` to initialize
6. Send `M20` to list files
7. Create a simple test.gcode file with a few G0 commands
8. Execute with `M23 /test.gcode` then `M24`

## References

- [SD Card Specification](https://www.sdcard.org/downloads/pls/)
- [ESP32 SPI Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html)
- [Arduino SD Library](https://www.arduino.cc/en/Reference/SD)

---

**Copyright (c) 2025 Encoder3D Project Contributors**  
Licensed under the MIT License

# Third-Party Licenses and Attributions

This project uses the following open-source libraries and components. We are grateful to their authors and contributors.

## Runtime Dependencies

### ArduinoJson
- **Author**: Benoit Blanchon
- **License**: MIT License
- **URL**: https://github.com/bblanchon/ArduinoJson
- **Description**: A C++ JSON library for embedded systems
- **Copyright**: Copyright © 2014-2024, Benoit Blanchon

### ESPAsyncWebServer
- **Author**: me-no-dev
- **License**: LGPL-3.0 License
- **URL**: https://github.com/me-no-dev/ESPAsyncWebServer
- **Description**: Asynchronous HTTP and WebSocket Server Library for ESP32
- **Copyright**: Copyright (c) 2016 me-no-dev

### AsyncTCP
- **Author**: me-no-dev
- **License**: LGPL-3.0 License
- **URL**: https://github.com/me-no-dev/AsyncTCP
- **Description**: Asynchronous TCP Library for ESP32
- **Copyright**: Copyright (c) 2016 me-no-dev

### Encoder Library
- **Author**: Paul Stoffregen (PJRC)
- **License**: MIT License
- **URL**: https://github.com/PaulStoffregen/Encoder
- **Description**: Quadrature Encoder Library for Arduino
- **Copyright**: Copyright (c) 2011-2018 PJRC.COM, LLC - Paul Stoffregen

## Development Dependencies

### PlatformIO
- **License**: Apache License 2.0
- **URL**: https://platformio.org/
- **Description**: Professional collaborative platform for embedded development
- **Copyright**: Copyright (c) 2014-present PlatformIO

### Arduino-ESP32
- **Author**: Espressif Systems
- **License**: LGPL-2.1 License
- **URL**: https://github.com/espressif/arduino-esp32
- **Description**: Arduino core for ESP32 WiFi chip
- **Copyright**: Copyright 2015-2024 Espressif Systems (Shanghai) PTE LTD

### ESP-IDF (Espressif IoT Development Framework)
- **Author**: Espressif Systems
- **License**: Apache License 2.0
- **URL**: https://github.com/espressif/esp-idf
- **Description**: Official development framework for ESP32
- **Copyright**: Copyright 2015-2024 Espressif Systems (Shanghai) PTE LTD

## Inspiration and Reference

This project was inspired by and references concepts from:

### Marlin Firmware
- **License**: GPL-3.0 License
- **URL**: https://github.com/MarlinFirmware/Marlin
- **Description**: Optimized 3D printer firmware
- **Note**: No code directly copied; concepts and G-code standards referenced

### GRBL
- **License**: GPL-3.0 License
- **URL**: https://github.com/grbl/grbl
- **Description**: High performance CNC firmware
- **Note**: G-code interpretation standards referenced

### RepRap Project
- **License**: GPL-2.0 License
- **URL**: https://reprap.org/
- **Description**: Open-source 3D printer project
- **Note**: 3D printer concepts and standards

## Hardware Platforms

### ESP32
- **Manufacturer**: Espressif Systems
- **URL**: https://www.espressif.com/en/products/socs/esp32
- **Description**: Low-cost, low-power system on a chip microcontroller

### Lolin32 Lite
- **Manufacturer**: Wemos/Lolin
- **URL**: https://www.wemos.cc/
- **Description**: ESP32 development board

## Fonts and Icons

- Emoji characters used in UI are from standard Unicode emoji set

## License Compatibility

This project (Encoder3D) is licensed under the **MIT License**, which is compatible with:
- ✅ MIT License (ArduinoJson, Encoder)
- ✅ Apache 2.0 License (PlatformIO, ESP-IDF)
- ⚠️ LGPL-3.0 and LGPL-2.1 Licenses (ESPAsyncWebServer, AsyncTCP, Arduino-ESP32)

**Note on LGPL Dependencies**: The LGPL libraries (ESPAsyncWebServer, AsyncTCP, Arduino-ESP32) are used as unmodified libraries. According to LGPL terms, you can use LGPL libraries in your own projects (even commercial ones) as long as:
1. You provide a way to replace the LGPL library (dynamic linking or providing object files)
2. You include the LGPL library's license and copyright notices
3. Any modifications to the LGPL library itself must be released under LGPL

Since these are used as external libraries and not modified, the main Encoder3D project can remain MIT licensed while properly attributing the LGPL dependencies.

## Full License Texts

Full license texts for all dependencies can be found in their respective repositories:
- MIT License: https://opensource.org/licenses/MIT
- Apache 2.0: https://www.apache.org/licenses/LICENSE-2.0
- LGPL-3.0: https://www.gnu.org/licenses/lgpl-3.0.html
- LGPL-2.1: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
- GPL-3.0: https://www.gnu.org/licenses/gpl-3.0.html
- GPL-2.0: https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

## Contributing

If you add new dependencies to this project, please update this file with appropriate attribution.

---

**Last Updated**: December 7, 2025

We thank all the open-source contributors whose work makes projects like this possible! 🙏

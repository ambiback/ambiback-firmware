# AmbiBack Firmware

**English** · [Deutsch](README.de.md)

ESP32 firmware for AmbiBack. It drives LED strips to match the screen content and connects DreamScreen, Philips Hue, a camera and network clients.

It is the core of the [AmbiBack project](https://github.com/ambiback). [AmbiBack UI](https://github.com/ambiback/ambiback-ui) and [AmbiBack Manager](https://github.com/ambiback/ambiback-manager) configure and control the device, and [AmbiBack PC](https://github.com/ambiback/ambiback-pc) can send it color data.

## Status

- Version in the source: 0.5.231.
- **The firmware has not been compiled since the migration changes and has not been tested on real hardware.** Treat the build steps below as untested.
- AmbiBack devices have not been on sale for around five years. This repository is for DIY use and further development, with no product support. More at [ambiback.de](https://www.ambiback.de/).

## Operating modes

- **VIDEO:** receives color data over UDP or TCP from external applications.
- **CAM:** determines colors using the module's camera.
- **AMBIENT:** shows static colors and scenes.
- **RELAY:** forwards LED data to other AmbiBack devices.
- **DEMO:** for testing.

## Supported device types

The target is selected through `ESP32_DEVICE_TYPE` in `globalincludes.h`. Exactly one type is active per build:

`ESP32_BRIDGE`, `ESP32_NAKED`, `ESP32_WROOVER_CAM`, `ESP32_TTGO_CAM`, `ESP32_TTGO_CAM_MIC`, `ESP32_TTGO_CAM_JOURNAL`, `ESP32_TTGO_CAM_PLUS`, `ESP32_M5ST_CAM`, `ESP32_M5ST_CAM_X`, `ESP32_M5ST_ATOMLITE`, `ESP32_M5ST_ATOMMATRIX`, `ESP32_AMBIBACK_LEDV1`, `ESP32_M5ST_CAM_TIMER_X`

## Building

You need the Arduino IDE or `arduino-cli` with ESP32 board support, plus Node.js and Python for the web interface. External libraries include AsyncTCP, ESPAsyncWebServer, FastLED and IRremoteESP8266.

1. Install the web dependencies in `web/`: `npm install`
2. Generate the embedded web headers: `bash build-web.sh` (Windows: `build-web.bat`)
3. Select the target device in `globalincludes.h`.
4. Compile and flash `AmbiBack.Controller.ino`.

`ambiwebserver_html_raw.h` and `ambiwebserver_html_raw_min.h` are generated and not checked in.

### Build settings

These values can be overridden as compiler flags:

- `AMBIBACK_FIRMWARE_BASE_URL`: base address for firmware updates. The device path is appended automatically. The availability of the default server is not guaranteed, so set your own address for your own builds.
- `AMBIBACK_SYSLOG_SERVER`: IP address of the syslog server. Empty by default, and it can also be set on the device.
- `AMBIBACK_AP_PASSWORD_SEED`: 32-bit seed for the generated access point password, defined in `globalincludes.h`. The default `0x0000FFFF` keeps the password of existing devices. For your own builds, set your own value, do not commit it, and use the same value in AmbiBack Manager.

## Security

The generated access point password is derived from the device's MAC address. A custom seed is not full protection: anyone who knows one password from a build can calculate the others. The device web interface and API have no login. Set your own access point password and run devices only on a trusted network.

## Trademarks

Philips Hue and DreamScreen are trademarks of their respective owners. This project is not affiliated with them.

## License

Copyright (C) 2020-2026 Andre Lorbach

This program is free software under the [GNU General Public License version 3](LICENSE). It comes with no warranty.

Included third-party code keeps its own license, see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

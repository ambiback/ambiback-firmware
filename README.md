# AmbiBack Firmware

**English** · [Deutsch](README.de.md)

ESP32 firmware for AmbiBack. It drives LED strips to match the screen content and connects DreamScreen, Philips Hue, a camera and network clients.

It is the core of the [AmbiBack project](https://github.com/ambiback). [AmbiBack UI](https://github.com/ambiback/ambiback-ui) and [AmbiBack Manager](https://github.com/ambiback/ambiback-manager) configure and control the device, and [AmbiBack PC](https://github.com/ambiback/ambiback-pc) can send it color data.

## Status

- Version in the source: 0.5.231.
- Eight of the 13 device types compile with the customized ESP32 core 2.0.0 used for earlier releases. That core is not published yet. `ttgo`, `ttgotm`, `ttgojr`, `wroover` and `ttgopl` do not build yet, and the camera types exceed the app partition with the public core 3.3.7.
- **The firmware has not been tested on real hardware since the migration.**
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

Devices with an access point open their setup window only when no Wi-Fi is configured, or after a physical request: hold the button for 4 to 7 seconds, or switch the power on three times within a few seconds each. AmbiBack Manager then pairs with the device and stores a device key. After pairing, Wi-Fi setup, access point and debug settings, reboot, WPS and firmware updates require this key.

Remaining limits: the first setup is not protected against someone in radio range who pairs first. The generated access point password is still derived from the MAC address, a custom seed is not full protection, and traffic is unencrypted. LED and mode control stay open on the local network, and device types without an access point have no pairing. Run devices only on a trusted network.

## Trademarks

Philips Hue and DreamScreen are trademarks of their respective owners. This project is not affiliated with them.

## License

Copyright (C) 2020-2026 Andre Lorbach

This program is free software under the [GNU General Public License version 3](LICENSE). It comes with no warranty.

Included third-party code keeps its own license, see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

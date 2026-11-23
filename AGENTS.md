# AmbiBack Controller - Agent Guide

**Quick links:** [Overview](#project-overview) · [Repository Split](#repository-split-main-vs-ambiback-ui) · [Architecture](#core-architecture) · [Key concepts](#key-concepts) · [Common tasks](#common-tasks) · [Parameter ranges](#parameter-ranges-and-preset-setters)

## Project Overview

**AmbiBack Controller** is an ESP32-based ambient backlighting system that synchronizes RGB LED strips with on-screen content. It can operate in multiple modes:
- **VIDEO Mode**: Receives color data over network (UDP/TCP) from external applications
- **CAM Mode**: Uses onboard camera to capture screen content and extract colors in real-time
- **AMBIENT Mode**: Static color, themed scenes, or health scenes (12–15: 40 Hz Gamma, 10 Hz Alpha, 34 Hz Gamma Red, Rotlicht Abend)
- **RELAY Mode**: Forwards LED data to other AmbiBack devices
- **DEMO Mode**: Test/demo functionality

**Current Version**: 0.5.230

### Multibuild Project

This is a **multibuild project** that produces different firmware binaries based on preprocessor options defined in `globalincludes.h`. Only one `ESP32_DEVICE_TYPE` is active per build (uncomment the desired device, comment others). The preprocessor conditionals (`#if`, `#elif`, `#ifdef`) throughout the codebase select which modules, pins, and features are compiled for each device.

**All device types listed in `globalincludes.h` (lines 95–107) are valid targets:**
- `ESP32_BRIDGE` – Bridge/relay device
- `ESP32_NAKED` – Naked module
- `ESP32_WROOVER_CAM` – ESP32 Wroover with camera
- `ESP32_TTGO_CAM` – TTGO camera module
- `ESP32_TTGO_CAM_MIC` – TTGO camera + microphone
- `ESP32_TTGO_CAM_JOURNAL` – TTGO T-Journal
- `ESP32_TTGO_CAM_PLUS` – TTGO camera Plus
- `ESP32_M5ST_CAM` – M5Stack ESP32CAM
- `ESP32_M5ST_CAM_X` – M5Stack CAM X (OV2660)
- `ESP32_M5ST_ATOMLITE` – M5Stack Atom Lite
- `ESP32_M5ST_ATOMMATRIX` – M5Stack Atom Matrix
- `ESP32_AMBIBACK_LEDV1` – AmbiBack LED module
- `ESP32_M5ST_CAM_TIMER_X` – M5Stack CAM Timer X (OV3660)

**Important for code optimization:** All code paths guarded by `ESP32_DEVICE_TYPE` or related feature flags are valid and in use for at least one build target. Do **not** remove or simplify these paths during optimization, dead-code elimination, or refactoring. Code that appears unused for the currently selected device may be required for other device types.

### Repository Split: Main vs ambiback-ui

The project is split into **two repositories** (or logically two areas when committing):

| Location | Contents |
|----------|----------|
| **Main repo** (`AmbiBack.Controller`) | Firmware (Arduino/C++), embedded web UI (`web/`), API handlers, firmware proxy endpoints, `AGENTS.md` |
| **ambiback-ui** | Standalone React/Vite UI; capabilities gating; Firmware tab; API client (`getFirmwareIndex`, `autoUpdate`, etc.) |

- **Firmware and device API** changes (e.g. `ambiwebserver_*.cpp`, `firmwarebaseurl`, `getfirmwareindex`, `web/HTML_MIN/index.html`) → commit to the **main** repo.
- **React UI** changes (e.g. `FirmwarePage.jsx`, `useCapabilities.js`, `ModeControls.jsx`, `api/client.js`, `Layout.jsx`) → commit to the **ambiback-ui** repo.
- **Default rule for UI-only work**: if the request is about the modern UI, frontend, page layout, React components, or other standalone app behavior, update **only** `ambiback-ui`.
- **Legacy embedded UI**: `web/HTML/` and `web/HTML_MIN/` are the firmware-hosted legacy UI. Do **not** change them for UI-only work unless the user explicitly asks for the embedded web UI, firmware-hosted pages, `web/`, or legacy parity.
- When adding new API endpoints or parameters that the React UI will use, implement them in the main repo first, then consume them in ambiback-ui.
- See `ambiback-ui/AGENTS.md` for ambiback-ui–specific workflows and structure.

### UI Ownership Decision Rule

- Requests mentioning `UI`, `frontend`, `page`, `layout`, `React`, or the modern app default to **`ambiback-ui` only**.
- Requests mentioning embedded web UI, firmware-hosted pages, `web/`, `web/HTML/`, or `web/HTML_MIN/` target the **legacy embedded UI** in the main repo.
- If a change needs new firmware/API support for the modern UI, update the API in the main repo first, then update `ambiback-ui`.
- Only update both UIs when the request explicitly asks for legacy parity or otherwise makes both surfaces part of scope.

## Agent Workflow

### KEY WORD SUMMARIZE

When the user requests **"SUMMARIZE"** (or uses the keyword `SUMMARIZE`), the agent should:

1. **Analyze all changes made in the current session**:
   - Review all file modifications (edits, creations, deletions)
   - Identify the scope and nature of changes
   - Understand the purpose and impact of modifications

2. **Create a squashed commit message**:
   - Format: Conventional commit style preferred
   - Structure:
     ```
     <type>(<scope>): <subject>
     
     <body>
     
     <footer>
     ```
   - Types: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`, etc.
   - Scope: Component affected (e.g., `led`, `camera`, `wifi`, `web`, `docs`)

3. **Output the commit message copy-ready**:
   - Print the complete commit message in a code block
   - Ensure it's formatted for direct copy-paste into git
   - Include all relevant details about what was changed and why
   - List affected files if helpful for context

**Example Usage:**
```
User: SUMMARIZE

Agent: [Analyzes session changes and outputs:]

```
feat(docs): Add comprehensive AGENTS.md guide for future AI agents

Added detailed documentation covering:
- Project architecture and component overview
- Build configuration and device types
- Key concepts (LED modes, camera processing, WiFi)
- Code patterns and common tasks
- Network protocols and integration points
- Debugging tips and known limitations

This guide will help future AI agents quickly understand
the codebase structure and common workflows.

Files changed:
- AGENTS.md (created, 406 lines)
```
```

**Important Notes:**
- The commit message should be **concise but descriptive**
- Focus on **what** was changed and **why** (if clear from context)
- Group related changes together
- Use present tense ("Add" not "Added")
- Keep subject line under 72 characters when possible
- Body can be more detailed, explaining the rationale

## Core Architecture

### Main Entry Point
- **`AmbiBack.Controller.ino`**: Main Arduino sketch containing `setup()` and `loop()`
  - Initializes all subsystems
  - Handles WiFi, button presses, system checks
  - Main event loop coordinates all components

### Key Components

#### 1. LED Management (`ledhelper.h/cpp`)
- Controls WS2812B RGB LED strips via FastLED library
- Handles color processing, smoothing, brightness control
- Manages LED modes (OFF, VIDEO, CAM, AMBIENT, RELAY, DEMO)
- **Ambient mode**: When `AmbientPreset` ≠ 0, **TaskAmbientCode** runs; `UpdateAmbientFrame()` writes to `m_leds_next` (presets 1–11: scenes; 12–15: health modes). Health presets use fixed per-preset cadence (40/10/34 Hz or breathing) with the unified `ambientAlphaStyleValue()` intensity model.
- **Cam-to-LED mapping**: maps sector CRGB buffer to strip (`SetLedBufferFromCamSectors`, `camCalcNextLedStep*`); uses `global::m_pCamProcessor` for cam mode/detail
- Frame smoothing and color boost algorithms

#### 2. Web Server (`ambiwebserver.h/cpp`)
- AsyncWebServer on port 80
- RESTful API for configuration and control
- Serves web interface (mobile-responsive)
- Handles OTA firmware updates
- Camera image streaming (JPEG, BMP, MJPEG)

#### 3. Global State (`global.h/cpp`)
- Centralized state management
- WiFi configuration and management
- Settings persistence coordination
- System-wide helper functions
- Debug/logging infrastructure

#### 4. EEPROM Storage (`eepromhelper.h/cpp`)
- Persistent settings storage using ESP32 Preferences API
- Settings include: WiFi credentials, LED configuration, camera settings, presets
- Auto-save mechanism (5 second timeout)

#### 5. Camera Processing (when `ENABLE_CAM` defined)
- **CamProcessor** (`camprocessor.h/cpp`): Image → sector colors. JPEG decode, sector geometry (`camCalcCamPointAreas`), color extraction (`camCalcColorsFromCamPoints`), frame stats, cam smoothing, mutex for capture/snapshot. Output: sector CRGB buffer; no dependency on LEDHelper.
- **LEDHelper** provides cam-to-LED mapping only (sector buffer → `m_leds_next`) and calls `CamTask_Start()` when in CAM mode.
- **Cam task** (`camtask.h` + implementation in `camprocessor.cpp`): owns the camera capture FreeRTOS task and handle; **TaskCamCode** orchestrates capture → CamProcessor (decode + sector colors) → LEDHelper::SetLedBufferFromCamSectors → wake ProcessLedNextStep. `CamTask_Start()` / `CamTask_Delete()` are implemented in camprocessor.cpp so builds that do not compile a separate camtask.cpp (e.g. USBFlasher) still link.
- **sectorgeometry.h**: shared types (`campoint`, `rectangle`, `CAMPOINTS`, `CAM_SECTOR_COUNT`, etc.) used by CamProcessor, LEDHelper (cam-to-LED mapping), and Hue.
- ESP32-CAM integration (OV2640/OV3660 sensors), adaptive exposure/gain, black detection.

#### 6. Network Communication
- **UDP Server** (`udpserver.h/cpp`): Port 1337 for LED data packets
- **TCP Server** (`tcpserver.h/cpp`): Port 1337 for LED sync (optional)
- **Serial Input** (`serialhelper.h/cpp`): Serial port LED data (optional)

#### 7. Smart Home Integration
- **Hue Helper** (`huehelper.h/cpp`): Philips Hue bridge integration (DTLS support)
- **DreamScreen** (`dreamscreen.h/cpp`): DreamScreen protocol emulation/compatibility

## File Structure

```
AmbiBack.Controller/
├── AmbiBack.Controller.ino    # Main sketch entry point
├── global.h/cpp               # Global state and utilities
├── globalincludes.h           # Build configuration, defines, includes
├── sectorgeometry.h           # Shared sector/cam types (campoint, rectangle, CAMPOINTS, etc.)
├── ledhelper.h/cpp            # LED strip control, cam-to-LED mapping (SetLedBufferFromCamSectors), calls CamTask_Start in CAM mode
├── camtask.h                  # Camera capture task API (CamTask_Start, CamTask_Delete); implementation in camprocessor.cpp
├── camprocessor.h/cpp         # Camera image → sector colors (when ENABLE_CAM)
├── ambiwebserver.h/cpp        # Web server and API
├── ambiwebserver_api.cpp      # setparam/setparameter handlers, JSON endpoints (validate value ranges here)
├── global_presets.cpp         # SetXxxPreset() setters; many clamp min/max (e.g. SetAmbientPreset 0–12)
├── eepromhelper.h/cpp         # Settings persistence
├── udpserver.h/cpp             # UDP network server
├── tcpserver.h/cpp             # TCP network server (optional)
├── serialhelper.h/cpp          # Serial input handler (optional)
├── irhelper.h/cpp              # IR remote support (optional)
├── huehelper.h/cpp             # Philips Hue integration (optional)
├── dreamscreen.h/cpp           # DreamScreen protocol (optional)
├── syslog.h/cpp                # Syslog debugging (optional)
├── ambiwebserver_html.h        # Wrapper for embedded web UI (includes generated header)
├── AI/                         # AI-generated task files and documentation
│   └── CODE_REVIEW.md          # Code review and analysis documents
├── ambiback-ui/                # Optional external React UI (talks to device API; separate build)
└── web/                        # Web interface assets (source of truth for embedded UI)
    ├── HTML/                   # Full UI source (edit here)
    ├── HTML_MIN/               # Minimal UI source (edit here)
    ├── convert.py, convert.sh  # Generate headers from HTML/ and HTML_MIN/
    └── AGENTS.md               # Web build workflow
```

### AI Tasks Folder

The `AI/` folder is designated for AI-generated task files, documentation, and analysis reports. When AI agents create:
- Code reviews
- Analysis documents
- Task lists
- Generated documentation
- Other AI-generated artifacts

These files should be placed in the `AI/` folder to keep the project root organized and clearly distinguish AI-generated content from source code.

## Build Configuration

### Device Type Selection
Set `ESP32_DEVICE_TYPE` in `globalincludes.h` (lines 95–107) to target specific hardware. Exactly one device define should be active per build. All listed device types are valid; see the **Multibuild Project** section above for the full list and the warning about preserving device-specific code paths during optimization.

### Feature Flags
Key compile-time features controlled by defines:
- `ENABLE_CAM`: Camera support
- `ENABLE_VIDEO`: Video mode (network LED data)
- `ENABLE_HUE`: Philips Hue integration
- `ENABLE_DREAMSCREEN`: DreamScreen protocol
- `ENABLE_IR`: IR remote support
- `ENABLE_SERIAL_INPUT`: Serial port LED data
- `ENABLE_TCP`: TCP server support
- `ENABLE_WIFI_AP`: Access Point mode
- `ENABLE_WIFI_NAT`: NAT for AP mode
- `ENABLE_SSD1306`: OLED display support
- `ENABLE_RELAY`: Relay mode (forward to other devices)
- `DISABLE_BLUETOOTH`: Disable BT to free memory

### Pin Definitions
Each device type has specific pin assignments in `globalincludes.h`:
- `LEDCTRL_PIN`: Primary LED data pin
- `LEDCTRL_PIN_SEC`: Secondary LED data pin (optional)
- `BTN_PIN`: Button input
- `PWRLED_PIN`: Power indicator LED
- `CTLLED_PIN`: Control indicator LED
- `IRRECV_PIN`: IR receiver pin
- Camera pins defined in `globaldefinesttgo.h` for TTGO modules

## Key Concepts

### LED Modes
- **LED_MODE_OFF (0)**: LEDs disabled
- **LED_MODE_VIDEO (1)**: Network-driven (UDP/TCP/Serial)
- **LED_MODE_AMBIENT (2)**: Ambient lighting (behavior depends on **Ambient Preset**, see below)
- **LED_MODE_CAM (3)**: Camera-based color extraction
- **LED_MODE_RELAY (4)**: Forward data to other devices
- **LED_MODE_DEMO (5)**: Demo/test mode

### Ambient Presets (when in AMBIENT mode)
Controlled by `AmbientPreset` (0–15). **TaskAmbient** runs in `ledhelper.cpp` when preset ≠ 0.

- **0 (Solid)**: No animation; uses stored ambient RGB (sliders / setcolor). TaskAmbient is suspended.
- **1–11 (Scenes)**: Themed animations (Fire, Christmas, Ocean, Rainbow, etc.). `UpdateAmbientFrame()` in `ledhelper.cpp`; frame rate set by **AmbientSpeed**; **AmbientIntensity** scales colors.
- **12–15 (Health Scenes)**: All use the unified `ambientAlphaStyleValue(intensity, wave)` helper. **AmbientIntensity** sets the floor of the pulse: `1` means the pulse decays only to ~99% of peak (minimal change), `100` means it decays all the way to black. Peak brightness is always bounded by the global **Brightness** setting. Speed is ignored for 12–14 (fixed cadence); preset 15 still uses Speed.
  - **12 (40 Hz Gamma)**: Blue square-wave pulse at **40 Hz** (12.5 ms half-period). To verify the rate: enable **Ultra Debug** in the web UI; Serial will print e.g. `40Hz Gamma: 40.0 Hz (target 40), half-period 12.50 ms`. See [Debugging Tips](#debugging-tips).
  - **13 (10 Hz Alpha)**: Red sine-wave pulse at **10 Hz** (~8.3 ms per frame, 12 frames/cycle).
  - **14 (34 Hz Gamma Red)**: Red square-wave pulse at **34 Hz** (14.7 ms half-period). Based on Lee et al. (2021) finding that 34–38 Hz red light is effective for gamma entrainment.
  - **15 (Rotlicht Abend)**: Red breathing scene; **Speed** controls the rate (`periodMs = 5000 / speed`; default speed 5 = 1 Hz).

### Health Ambient Modes (research background)

Presets 12–14 are inspired by flickering light stimulation (FLS) and brain-wave entrainment research. **Not medical advice**; intended for informational and experimental use only.

- **Gamma entrainment**: Studies link reduced gamma oscillations to conditions such as Alzheimer’s disease, and suggest that flickering light may entrain gamma waves. In mice, 40 Hz FLS reduced amyloid burden; in humans, optimal parameters may differ.
- **Lee et al. (2021)** investigated optimal color, intensity, and frequency for gamma entrainment in humans. Key findings: red light entrained gamma most effectively; 34–38 Hz entrained stronger and more widespread gamma than 40–50 Hz; 400 cd/m² white light at 34–38 Hz was most optimal with tolerable adverse effects.
- **Reference**: Lee, K., Park, Y., Suh, S.W. et al. Optimal flickering light stimulation for entraining gamma waves in the human brain. *Sci Rep* 11, 16206 (2021). https://www.nature.com/articles/s41598-021-95550-1

### Camera Modes (when in CAM mode)
- **LED_MODE_CAM_SINGLE (0)**: Single color from entire screen
- **LED_MODE_CAM_BAR (1)**: Light bar (top/bottom zones)
- **LED_MODE_CAM_DETAIL (2)**: Detailed multi-zone color mapping

### WiFi Modes
- **Station Mode**: Connects to existing WiFi network
- **Access Point Mode**: Creates own network for configuration
- **WPS Mode**: WiFi Protected Setup for easy connection
- **WiFi Direct**: Direct connection to relay devices

### Button Functions
Button press duration determines action:
- **< 1.5s**: Short press (configurable function)
- **1.5-7.5s**: Reboot
- **7.5-15s**: Enter WPS mode
- **> 15s**: Factory reset
- **Hold on startup**: Firmware reset

### Color Processing Pipeline
1. **Capture**: Camera frame or network data
2. **Extract**: Color from screen zones (top/bottom/left/right)
3. **Pre-process**: Color correction, boost, minimum luminosity
4. **Smooth**: Frame smoothing to reduce flicker
5. **Post-process**: Side brightness, fade effects
6. **Display**: Send to LED strip via FastLED

## Important Code Patterns

### Settings Management
```cpp
// Get setting with default
byte value = global::m_pEepromHelper->GetSettingByte("SettingName", defaultValue);

// Set setting (auto-saves after timeout)
global::m_pLedHelper->SetBrightness(255, true); // true = save to EEPROM
```

### LED Color Setting
```cpp
// Set individual LED
pLedhelper->SetLedColorByIdx(index, white, red, green, blue);

// Update all LEDs
pLedhelper->ShowLedColors();
```

### Debug Output
```cpp
_DBGPRINTF(PSTR("Message: %d\n"), value);           // Normal debug
_DBGPRINTF_ULTRA(PSTR("Verbose: %d\n"), value);      // Ultra verbose
_ERRPRINTF(PSTR("Error: %d\n"), value);              // Error message
```

### WiFi Event Handling
WiFi events handled in `handleWiFiEvents()` function:
- Connection/disconnection
- WPS success/failure
- IP address assignment
- AP client connections

## Configuration System

### EEPROM Storage
- Uses ESP32 Preferences API
- 4096 bytes total storage
- Auto-save after 5 seconds of changes
- Factory reset clears all settings

### Settings Categories
1. **WiFi**: SSID, password, AP settings
2. **LED**: Count, brightness, color correction, setup geometry
3. **Ambient**: Preset (0–12), speed (1–10), intensity (1–100)
4. **Camera**: Resolution, exposure, color thresholds, points
5. **Hue**: Bridge IP, user, light mappings
6. **DreamScreen**: Group, emulation settings
7. **System**: Device name, ID, debug settings

### Web API Endpoints
- `GET /`: Main web interface
- `GET /api/status`: System status JSON
- `GET /api/parameters` or `GET /getparamatersasjson`: All settings JSON
- `GET /setparam?param=<name>&value=<value>`: Parameter set (e.g. `param=setambientpreset&value=12`). Handled by `handleSetParameter` in `ambiwebserver_api.cpp`.
- `GET /setpreset?param=<name>&value=<value>`: Preset set for color/brightness/motion/cam/hue (not ambient). Handled by `handleSetParameterPresets`.
- `POST /api/setparameter`: Update setting (JSON body); same validation as setparam in `ambiwebserver_api.cpp`
- `POST /api/setcolor`: Set LED color
- `POST /api/reboot`: Reboot device
- `POST /api/wps`: Start WPS
- `GET /cam.jpg`: Camera JPEG snapshot
- `GET /cam.mjpeg`: MJPEG stream
- `POST /update`: OTA firmware update

### Parameter ranges and preset setters
When adding new valid values (e.g. ambient preset 12), update **all** of the following or the device may reject the value or store the wrong one:
1. **ambiwebserver_api.cpp**: range checks in both code paths that handle the param (e.g. `iNew <= 12` for `setambientpreset`). Missing this → "ERROR, invalid value".
2. **global_presets.cpp**: the setter’s clamp (e.g. `SetAmbientPreset`: `if (iIndex > 12) iIndex = 12`). If the API passes 12 but the setter still clamps to 11, the response can say "Successfully set new value 12" while the stored value is 11. Always extend the clamp when extending the preset range.

## Network Protocols

### UDP LED Data Packet Format
```
[Header: 8 bytes]
- Sequence number
- LED count
- Mode flags
- Reserved

[LED Data: 3 bytes per LED]
- Red, Green, Blue
```

### TCP LED Sync
Similar format to UDP, used for reliable transmission.

### Serial LED Data
Binary protocol with header and chunked data for reliable transmission.

## Memory Management

### Critical Memory Constraints
- **Minimum Free Memory**: 20KB (triggers warnings)
- **OTA Minimum**: 40KB (for firmware updates)
- **PSRAM**: Used for camera buffers when available
- **Heap Monitoring**: Regular checks every 5 seconds

### Memory Optimization
- Bluetooth disabled by default (`DISABLE_BLUETOOTH`)
- Camera buffer count configurable (2-5 buffers)
- Frame smoothing uses PSRAM when available
- Dynamic allocation with `heap_caps_malloc()` for PSRAM

## Common Tasks

### Adding a New Device Type
1. Define device constant in `globalincludes.h` (e.g., `ESP32_MY_DEVICE 40`)
2. Add `#elif ESP32_DEVICE_TYPE == ESP32_MY_DEVICE` block
3. Define pins, features, device name, network name
4. Set appropriate feature flags

### Adding a New LED Mode
1. Define mode constant in `globalincludes.h`
2. Add case in `LEDHelper::SetLedMode()`
3. Implement mode logic in `LEDHelper::HandleLeds()`
4. Add web UI controls in `ambiwebserver.cpp`

### Adding a New Setting
1. Add getter/setter in `global.h` (or `ledhelper.h` if LED-specific)
2. Implement in corresponding `.cpp` file
3. Add EEPROM key in getter/setter implementation
4. Add web API handler in `ambiwebserver.cpp` and/or `ambiwebserver_api.cpp`
5. **If the setting has a numeric range**: (a) ensure both handler paths in `ambiwebserver_api.cpp` accept the new range; (b) if there is a setter in `global_presets.cpp` (or similar) that clamps the value, extend that clamp too—otherwise the API may report success but the stored value is wrong.
6. Update the correct UI surface if needed:
   - `ambiback-ui` for modern React UI work.
   - `web/HTML/` or `web/HTML_MIN/` only when the embedded legacy UI is explicitly in scope.

### Adding a New Ambient Preset
1. Add a new `case` (e.g. 13) in `UpdateAmbientFrame()` in `ledhelper.cpp`; implement RGB logic per LED/frame.
2. If the preset needs fixed timing (e.g. 40 Hz), in `TaskAmbientCode()` branch on preset and use `_delayMicroseconds(...)` instead of `_taskDelay(frameMs)`.
3. **Extend API validation** in `ambiwebserver_api.cpp`: search for `setambientpreset` and increase the allowed range (e.g. `iNew <= 12` → `iNew <= 13`). Both code paths that handle this param must allow the new value.
4. **Extend the setter clamp** in `global_presets.cpp`: in `SetAmbientPreset()`, update the upper clamp (e.g. `if (iIndex > 12)` → `if (iIndex > 13)`). If you skip this, the API can return success but the stored value will be clamped to the old max (e.g. 12 sent, 11 stored).
5. If the preset must be exposed in the modern UI, update `ambiback-ui/src/pages/AmbientPage.jsx` (`AMBIENT_PRESETS`).
6. Update `web/HTML/include_ambient.html` only if the embedded legacy UI is explicitly part of scope, then run `web/convert.sh` to regenerate embedded HTML headers.
7. Optionally update comments in `global.h` / `global.cpp` for `m_iAmbientPreset`.

### Debugging WiFi Issues
- Check `handleWiFiEvents()` for connection events
- Enable `ENABLE_NORMAL_DEBUG` for verbose output
- Use WPS mode for easy setup
- Check `DEFAULT_WIFI_CONNECT_TIMEOUT` (20 seconds)
- Monitor `g_timeWIFITryConnectCount` for retry attempts

### Camera Troubleshooting
- Check camera initialization in `global::InitCam()`
- Verify sensor type (`CAM_SENSOR_OV2640` or `CAM_SENSOR_OV3660`)
- Monitor frame rate via `GetCurrentCamFps()`
- Adjust resolution/performance settings if memory issues
- Check black detection thresholds if colors not activating

## Integration Points

### External Applications
- Send UDP packets to port 1337 with LED color data
- Format: Header (8 bytes) + LED data (3 bytes per LED)
- Sequence numbers for packet ordering
- Supports up to 256 LEDs per strip

### Philips Hue
- Requires Hue bridge IP and username
- Supports DTLS for secure communication
- Maps LED zones to Hue lights
- Configurable via web interface

### DreamScreen
- Emulates DreamScreen protocol on UDP port 8888
- Compatible with DreamScreen-compatible applications
- Group-based control

## Build and Deployment

### Prerequisites
- Arduino IDE with ESP32 board support
- Required libraries (see dependencies)
- Appropriate ESP32 board selected

### Dependencies
- FastLED (LED control)
- ESPAsyncWebServer (web server)
- AsyncTCP (async networking)
- ESP32-CAM libraries (camera support)
- Preferences (EEPROM storage)

### Generate web headers (before first compile)
The embedded web UI is built from `web/HTML/` and `web/HTML_MIN/`; the generated headers are not in the repo. **Before compiling firmware**, generate them:

- From the `web/` directory run: `./convert.sh` (or `bash convert.sh` on Windows).
- This creates `ambiwebserver_html_raw.h` and `ambiwebserver_html_raw_min.h` in the project root.
- Requires Python 3 and Node.js (for minification). Use `--no-minify` with `convert.py` if Node is unavailable (larger firmware).
- If the headers are missing, the build fails on `#include "ambiwebserver_html_raw.h"` (or `_min`). See `web/AGENTS.md` for the full web build workflow.

### Compilation
1. Generate web headers (see above) if not already done.
2. Set `ESP32_DEVICE_TYPE` in `globalincludes.h`
3. Configure feature flags as needed
4. Compile in Arduino IDE
5. Upload via USB or OTA

### Compiler Size Optimization
Copy `platform.local.txt` to Arduino ESP32 package:
`C:\Users\<User>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\<version>\platform.local.txt`
Or locate path from Arduino IDE verbose compile output.
Flags: -Os, -ffunction-sections, -fdata-sections, -Wl,--gc-sections
Estimated savings: 5-15 KB.

### OTA Updates
- Web interface: `/update` endpoint
- HTTP server: `http://firmware.ambiback.com/firmware/...`
- Automatic version checking
- Rollback on failure

## Testing

### LED Testing
- Use DEMO mode for basic LED functionality
- Test each side independently
- Verify color accuracy and brightness

### Camera Testing
- Check frame capture rate
- Verify color extraction accuracy
- Test different resolutions
- Validate black detection

### Network Testing
- UDP packet reception
- TCP connection handling
- Web interface responsiveness
- OTA update process

## Known Limitations

1. **Memory**: Limited by ESP32 RAM (varies by board)
2. **LED Count**: Maximum 256 LEDs per strip
3. **Camera FPS**: Limited by processing power (typically 10-30 FPS)
4. **Network Range**: WiFi range limitations
5. **Concurrent Connections**: Single TCP client at a time

## Future Enhancements

Potential areas for improvement:
- Multi-strip support (currently single strip)
- Higher camera frame rates
- Additional smart home integrations
- Machine learning for better color extraction
- WebRTC for remote camera viewing

## Debugging Tips

1. **Enable Debug Output**: Set `ENABLE_NORMAL_DEBUG` in `globalincludes.h`
2. **Serial Monitor**: Use 115200 baud rate
3. **Syslog**: Configure syslog server for remote logging
4. **Memory Monitoring**: Watch for low memory warnings
5. **WiFi Events**: Monitor connection state changes
6. **Frame Rate**: Check FPS counters in status output
7. **"Success but wrong value"**: If setparam returns success but the device uses the wrong value (e.g. preset 12 becomes 11), the setter in `global_presets.cpp` is likely still clamping to the old max. Update the clamp in the corresponding `SetXxxPreset()` (or similar) function.
8. **Verify 40 Hz Gamma rate**: Set preset to 12 (40 Hz Gamma), enable **Ultra Debug** (web UI or debug settings). Open Serial at 115200 baud; about once per second you should see e.g. `40Hz Gamma: 40.0 Hz (target 40), half-period 12.50 ms`. Significant deviation (e.g. &lt;38 or &gt;42 Hz) may indicate task starvation or timing drift.

## Code Style Notes

- Uses `PSTR()` macro for string constants (program memory)
- Hungarian notation for variables (e.g., `m_bVariable`, `m_iVariable`)
- Static class members for global state
- Extensive use of preprocessor conditionals for feature flags
- Error handling via return values and status flags

## Important Constants

- `UDP_PORT`: 1337
- `TCP_PORT`: 1337
- `HTTP_PORT`: 80
- `MAX_LEDS`: 256
- `EEPROM_SIZE`: 4096 bytes
- `DEFAULT_SERIAL_RATE`: 115200
- `DEFAULT_FRAME_LENGTH`: 10 (LEDs per frame)
- `DEFAULT_DELAY_LENGTH`: 40ms

---

**Last Updated**: Based on version 0.4.227
**Maintainer**: See git history for contributors

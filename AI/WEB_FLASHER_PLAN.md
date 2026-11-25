# AmbiBack Web Flasher – Project Plan

Status: Phase 1 and 2 (firmware CI, Improv Serial, boot banner, symbol maps) implemented in ambiback-firmware; the web app lives in `ambiback/ambiback-web-flasher`. Written 2026-11-25.

## 1. Goal

A browser tool, hosted on GitHub Pages like `ambiback-ui`, that installs AmbiBack firmware on an ESP32 over USB and helps with debugging. No Arduino, no esptool and no Windows app needed.

Decisions taken:

| Topic | Decision |
|---|---|
| Firmware build | **CI prebuilt only.** GitHub Actions builds all device types; the browser only picks and flashes. No build in the browser, no custom builds. |
| Stack | **React + Vite**, same tooling and look as `ambiback-ui`. Flashing with `esptool-js`. |
| Scope v1 | Flashing, serial monitor, device control commands, crash backtrace decoding, Wi-Fi setup via Improv Serial. |
| Desktop flasher | **Keep both.** The web flasher covers plain installs (today's Easy mode). `ambiback-flasher` stays for local builds and development. |

Non-goals: compiling in the browser, custom build options, Firefox/Safari/iOS support (no Web Serial there), uploading to an update server.

## 2. Architecture overview

```text
ambiback-firmware (tag vX.Y.Z)
  └─ GitHub Actions "release-builds"
       matrix: 13 device types × default core
       → per device: firmware.bin, partitions.bin, bootloader.bin, boot_app0.bin, symbols.json, buildinfo.json
       → manifest.json (all devices, this version)
       → one zip ambiback-firmware-<version>.zip: attached to the GitHub Release (tags)
         and uploaded as workflow artifact "ambiback-firmware" (every run); ELFs as separate artifacts

ambiback-web-flasher (new repo)
  └─ GitHub Actions "deploy"
       1. fetch firmware release assets (server side, no CORS issue)
       2. copy them into public/firmware/<version>/<device>/…
       3. merge into public/firmware/index.json (all versions)
       4. vite build → GitHub Pages
  └─ Browser app
       Web Serial → esptool-js (flash) / line reader (monitor, commands, Improv)
       fetch same-origin /firmware/… (no CORS problem)
```

Why copy the binaries into Pages: GitHub release asset downloads redirect to a storage host that does not send CORS headers, so browser code cannot fetch them directly. Same-origin files on Pages avoid this. Size check: ~1.3 MB × 4 files × 13 devices ≈ 25–35 MB per version incl. ELF; keep the latest N versions (e.g. 5) to stay well under the 1 GB Pages limit. ELF files are the biggest part; strip them to a symbol map if size becomes a problem (see 5.4).

## 3. Phase 1 – Firmware repo: CI release builds

New file `.github/workflows/release-builds.yml` in `ambiback-firmware`.

### 3.1 Build matrix

Source of truth today is the device table in `ambiback-flasher/CDeviceHelper.cs` (lines 42–55). Move it into the firmware repo as `build/devices.json` so CI, the desktop flasher and the web flasher share one list:

| id | label | define | board (3.x core) | extra FQBN options | upload baud | PSRAM |
|---|---|---|---|---|---|---|
| bridge | AmbiBack Bridge | ESP32_BRIDGE | esp32 | CPUFreq=240,FlashMode=qio,FlashFreq=80,FlashSize=4M | 921600 | no |
| ambiledv1 | AmbiBack LED V1 | ESP32_AMBIBACK_LEDV1 | m5stick-c | – | 1500000 | no |
| naked | AmbiBack Naked ESP32 | ESP32_NAKED | esp32 | (as bridge) | 921600 | no |
| wroover | AmbiBack Wroover Cam | ESP32_WROOVER_CAM | esp32 | (as bridge) | 921600 | yes |
| ttgo | AmbiBack TTGO Cam | ESP32_TTGO_CAM | esp32 | (as bridge) | 921600 | yes |
| ttgotm | AmbiBack TTGO Cam + Mic | ESP32_TTGO_CAM_MIC | esp32 | (as bridge) | 921600 | yes |
| ttgojr | AmbiBack TTGO Journal | ESP32_TTGO_CAM_JOURNAL | esp32 | (as bridge) | 921600 | no |
| ttgopl | AmbiBack TTGO Cam Plus | ESP32_TTGO_CAM_PLUS | esp32 | (as bridge) | 921600 | yes |
| m5esp32 | M5Stack ESP32CAM | ESP32_M5ST_CAM | esp32 | (as bridge) | 921600 | no |
| m5camx | M5Stack Camera X OV2640 | ESP32_M5ST_CAM_X | esp32 | (as bridge) | 921600 | yes |
| m5camtx | M5Stack Camera Timer X OV3660 | ESP32_M5ST_CAM_TIMER_X | m5stack-timer-cam | – | 1500000 | yes |
| m5alit | M5Stack Atom Lite | ESP32_M5ST_ATOMLITE | m5stick-c | – | 1500000 | no |
| m5almx | M5Stack Atom Matrix | ESP32_M5ST_ATOMMATRIX | m5stick-c | – | 1500000 | no |

Follow-up for the desktop flasher (separate change): read `build/devices.json` from the downloaded source instead of the hard-coded table.

### 3.2 Build steps (per matrix entry)

Mirror what `ambiback-flasher` does in `frmMain.cs` (`BuildWithCli`, `GetExtraFlags`):

1. `arduino/setup-arduino-cli` action, `arduino-cli core install esp32:esp32@3.3.6` (default core; cache `~/.arduino15`).
2. Install libraries at the fixed tags (FastLED 3.10.3, AsyncTCP v3.4.10, ESPAsyncWebServer v3.9.5, WiFiClientSecureUdp 1.0.0, esp8266-oled-ssd1306 4.6.2). Put the list in `build/libraries.json` (shared with the desktop flasher later).
3. Generate web headers: `python web/convert.py` (with Node for minify).
4. Compile (`build/ci_build.py`):
   ```sh
   arduino-cli compile \
     --fqbn "esp32:esp32:<board3>:<fqbnOptions filtered against board details>" \
     --build-property "compiler.cpp.extra_flags=-DESP32_DEVICE_TYPE=<define> -DAMBIBACK_BUILD_ID_TOKEN=<id> [-DBOARD_HAS_PSRAM]" \
     --build-property "compiler.c.extra_flags=<same>" \
     --warnings none <sketch>
   ```
   Differences from the desktop flasher: `compiler.*.extra_flags` instead of `build.extra_flags`, so the core keeps its own defines (CORE_DEBUG_LEVEL, ARDUINO_BOARD, PSRAM). The build ID is passed as an identifier token (`AMBIBACK_BUILD_ID_TOKEN`) and stringified in `globalincludes.h`, which avoids quoting problems. Board options a board does not offer are dropped, like `FilterBoardOptions` does.
5. Collect into `dist/<id>/`: `firmware.bin` (`AmbiBack.Controller.ino.bin`), `partitions.bin`, `bootloader.bin`, `boot_app0.bin` (from the core), `symbols.json` (`build/make-symbols.py`), `buildinfo.json` (version from `AMBIBACK_VERSION`, build ID, core, commit SHA, size and sha256 of each file). The ELF goes to a separate workflow artifact `elf-<id>`.

### 3.3 Release job

- Trigger: tag `v*` and `workflow_dispatch` (for test builds without a release).
- After the matrix: `build/make-manifest.py` writes `manifest.json`; everything is zipped into `ambiback-firmware-<version>.zip` (manifest at the root), uploaded as artifact `ambiback-firmware` and, on tags, attached to the GitHub Release.
- The workflow also runs on pull requests that touch firmware sources, so every PR proves all 13 builds.
- Optional: a `repository_dispatch` to `ambiback-web-flasher` so its deploy runs on new releases (needs a fine-grained token stored as a secret; otherwise the web flasher polls on a schedule).

### 3.4 Manifest format

`manifest.json` (one per firmware version):

```json
{
  "schema": 1,
  "version": "0.5.231",
  "commit": "15f4ad8",
  "core": "3.3.6",
  "date": "2026-11-25",
  "channel": "stable",
  "devices": {
    "naked": {
      "label": "AmbiBack Naked ESP32",
      "define": "ESP32_NAKED",
      "chip": "ESP32",
      "baud": 921600,
      "psram": false,
      "buildId": "ci20261125_15f4ad8_r12",
      "flash": { "mode": "dio", "freq": "80m", "size": "keep" },
      "parts": [
        { "offset": 4096,  "path": "naked/bootloader.bin", "size": 0, "sha256": "…" },
        { "offset": 32768, "path": "naked/partitions.bin", "size": 0, "sha256": "…" },
        { "offset": 57344, "path": "naked/boot_app0.bin",  "size": 0, "sha256": "…" },
        { "offset": 65536, "path": "naked/firmware.bin",   "size": 0, "sha256": "…" }
      ],
      "symbols": "naked/symbols.json"
    }
  }
}
```

Offsets match the desktop flasher's esptool call: `0x1000` bootloader, `0x8000` partitions, `0xe000` boot_app0, `0x10000` app.

`index.json` (web flasher side, merged at deploy): list of versions with channel and date, newest first.

**Acceptance:** a tag produces 13 device folders plus `manifest.json` on the release; flashing one of them with desktop `esptool` boots the device.

## 4. Phase 2 – Firmware repo: Improv Serial

Add [Improv Wi-Fi Serial](https://www.improv-wifi.com/serial/) so the web flasher (and ESP Web Tools / Home Assistant) can set Wi-Fi in a standard way.

- New `improvserial.h/.cpp`, guarded by a feature flag (e.g. `ENABLE_IMPROV_SERIAL`), on by default for all device types.
- Hook into `serialhelper.cpp`: the existing parser reads binary packets starting with `%`; Improv packets start with `IMPROV`. Dispatch on the first byte so both protocols live side by side.
- Commands to support: *Request current state*, *Request device info* (firmware `AmbiBack`, version, chip `ESP32`, device name), *Send Wi-Fi settings* (reuse the same code path as the `%WIFI:` command), *Request scanned Wi-Fi networks* (optional but nice).
- States: `Authorized` (no button needed) → `Provisioning` → `Provisioned` with the device URL `http://<ip>/` as redirect.
- Builds where `ENABLE_SERIAL_INPUT` uses the port for LED data: check there is no conflict at boot (Improv runs only until Wi-Fi is set, or always listens for the `IMPROV` header).

**Acceptance:** the Improv test page or ESP Web Tools sets Wi-Fi on a flashed device and shows its URL.

## 5. Phase 3 – New repo `ambiback-web-flasher`

### 5.1 Setup

- Copy tooling from `ambiback-ui`: Vite 7, React 19, ESLint config, GPL-3.0 license, `deploy.yml` (GitHub Pages), English/German texts.
- Dependencies: `esptool-js` (Espressif, Apache-2.0), nothing else heavy. Backtrace decoding is done in our own code (see 5.4).
- Add `THIRD_PARTY_NOTICES.md` (esptool-js, React).

### 5.2 Folder layout

```text
src/
  serial/
    port.js          Web Serial wrapper: request, open/close, baud change, one reader shared by the features
    flasher.js       esptool-js wrapper: connect, chip detect, write parts, progress, hard reset
    ambiCommands.js  AmbiBack serial commands (%INFO:, %WIFI:, %REBOOT:, %FACTORYDEFAULT:, LED test)
    improv.js        Improv Serial client
    lineReader.js    splits the stream into lines for the monitor, keeps raw bytes for protocols
  debug/
    backtrace.js     find "Backtrace: 0x… 0x…" lines, map addresses to symbols
    symbols.js       load the symbol map for the flashed device/version
  firmware/
    catalog.js       load index.json and manifest.json, verify sha256 after download
  pages/
    InstallPage.jsx  device picker → version → port → flash → Wi-Fi
    MonitorPage.jsx  serial monitor + decoded crashes
    DevicePage.jsx   device control
  components/        DeviceCard, ProgressBar, BrowserCheck, DriverHelp, BootButtonHelp
public/
  firmware/          filled by CI at deploy time (not in git)
  devices/           device pictures (reuse those of ambiback-flasher)
```

### 5.3 Flows

**Install (main flow):**
1. Browser check: no `navigator.serial` → show which browsers work.
2. Pick the device from pictures (labels from the manifest).
3. Pick the version (default: newest stable).
4. Connect: `navigator.serial.requestPort()` with USB filters for common bridges (CP210x, CH340, FTDI, ESP32 native USB).
5. Flash with esptool-js: baud from the manifest with fallback to 115200 on errors, write all parts, verify, hard reset. Offer **Erase all first** (checkbox, default off; clears settings).
6. Wi-Fi: after reboot try Improv; if the firmware is too old for Improv, fall back to the `%WIFI:` command.
7. Done: show the device URL and a link to `ambiback-ui`.

**Monitor:** baud selector (115200 default), auto scroll, filter, pause, clear, save as `.txt`, timestamps. Crash lines decoded inline.

**Device control:** Info, Reboot, Factory defaults (with confirm), LED test, Wi-Fi.

Only one feature may own the port's reader at a time; `port.js` hands it over between flasher, monitor and protocols.

### 5.4 Serial command format (port from the desktop flasher)

Firmware side (`serialhelper.cpp`): the parser skips bytes until `%`, reads the command name up to `:`, space, `0x00` or `0x04`, then reads command data. For `WIFI` it reads SSID and key, each ending with `0x00`, and **drops bytes ≤ 0x1F**.

Desktop side (`frmMain.cs`, lines 769–880): plain commands (`%INFO:`, `%REBOOT:`, `%FACTORYDEFAULT:`, `%STARTLEDDATA:`) are sent as plain ASCII. `%WIFI:`, `%LEDDATA:` and `%SETRELAY:` are built with .NET `BinaryWriter.Write(string)`, which puts a **length byte in front of each string**. The firmware ignores this byte only because it is ≤ 0x1F.

**Latent bug in the desktop flasher:** an SSID of exactly 32 characters, or a Wi-Fi key of 32–63 characters, gets a length byte ≥ 0x20 (a printable character). The firmware then copies that byte into the SSID or key, and the device stores the wrong password. The web flasher must send raw bytes: `"%WIFI:" + ssid + 0x00 + key + 0x00` (UTF-8, no length prefix). Fix the desktop flasher the same way (separate change).

Write the byte layout of every command down in a small spec (`docs/serial-protocol.md` in the firmware repo) that both tools follow.

### 5.5 Backtrace decoding

- ESP32 panic output contains `Backtrace: 0x400d1234:0x3ffb1230 0x…`.
- CI produces a compact symbol map per device and version: `nm -n -C --defined-only` plus `addr2line` line info for the function start addresses → `symbols.json` (sorted address ranges → function, file, line). This is much smaller than the ELF and avoids an ELF/DWARF parser in the browser.
- The monitor matches PC values against the map with binary search and shows `function (file:line)`.
- Version mismatch guard: the device prints its version and build ID at boot (`AMBIBACK_BUILD_ID`); only decode when they match the loaded map, otherwise warn.

### 5.6 Deploy workflow

- Triggers: push to `main`, `repository_dispatch` from the firmware release, weekly schedule as fallback, manual.
- Steps: `gh release list/download` on `ambiback/ambiback-firmware` (latest N), unpack into `public/firmware/`, write `index.json`, check the sha256 values, `npm ci && npm run build`, deploy to Pages.

## 6. Phase 4 – Docs and links

- README (English + German): supported browsers, drivers (CP210x, CH340), BOOT button for boards without auto reset (ESP32-CAM style boards), privacy (everything runs locally in the browser).
- `ambiback-flasher` README: point plain users to the web flasher.
- `ambiback-ui`: a link "Install firmware over USB".
- Firmware README: link in the install section.

## 7. Milestones

| # | Milestone | Repo | Result |
|---|---|---|---|
| M1 | `build/devices.json` + `release-builds.yml` | firmware | tag → 13 images + manifest on the release |
| M2 | Serial protocol spec + Improv Serial | firmware | Wi-Fi via Improv works |
| M3 | Repo scaffold + deploy with firmware copy | web-flasher | Pages site lists versions and devices |
| M4 | Install flow (flash + Wi-Fi) | web-flasher | end-to-end install on real hardware |
| M5 | Monitor + device control | web-flasher | parity with the desktop flasher's serial features |
| M6 | Symbol maps in CI + backtrace decoding | both | crash lines show function and file |
| M7 | Docs and cross links | all | users find the tool |

M1 and M2 can run in parallel. M3 needs M1 (or a hand-made test manifest).

## 8. Risks and open points

| Risk | Mitigation |
|---|---|
| Web Serial only in Chromium desktop browsers | Clear browser check; desktop flasher stays as fallback. |
| Board without auto reset (some camera boards) | Show the "hold BOOT, press RESET" help when esptool-js cannot sync. |
| Missing USB driver on Windows | Driver help page with links to Silicon Labs / WCH. |
| High baud rates (1500000) fail on some cables/bridges | Retry at 460800 then 115200 automatically. |
| Pages size grows with versions | Keep latest N versions; symbol maps instead of full ELF. |
| Device table drifts between tools | Single `build/devices.json` in the firmware repo. |
| Old firmware without Improv | Fallback to `%WIFI:` command. |
| Never tested on real hardware so far (also true for the migrated desktop flasher) | M4 acceptance requires a real device of at least two board families (ESP32 DevKit style and M5Stack). |

Open points to settle later:
- Repo name (`ambiback-web-flasher` assumed) and URL (`ambiback.github.io/ambiback-web-flasher/` or a custom domain).
- Channels: stable only, or also beta/dev builds from `workflow_dispatch`.
- Whether the web flasher should also run as a PWA offline (would cache the firmware files).

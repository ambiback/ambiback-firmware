# AmbiBack serial protocol

The firmware listens on the USB serial port at **115200 baud** (8N1). Two protocols share the port:

1. **AmbiBack `%` commands**, used by the AmbiBack USB Flasher and the AmbiBack Web Flasher.
2. **Improv Wi-Fi Serial**, a standard protocol for setting Wi-Fi from a browser.

The parser lives in `serialhelper.cpp` (`ENABLE_SERIAL_INPUT`, on for all device types except the Wroover Cam) and in `improvserial.cpp` (`ENABLE_IMPROV_SERIAL`, on for all device types; define `DISABLE_IMPROV_SERIAL` to leave it out). Builds without `ENABLE_SERIAL_INPUT` still read Improv packets from the main loop.

Debug output is written to the same port as plain text lines. Clients must skip lines they do not understand.

## Boot banner

Right after `Serial.begin()` the firmware prints one line, whatever the debug settings:

```text
[AmbiBack] version=0.5.231 build=ci20261125_15f4ad8_r12 device=ESP32_NAKED
```

- `version`: `AMBIBACK_VERSION`
- `build`: `AMBIBACK_BUILD_ID` (`dev` for local builds without a build ID)
- `device`: the `ESP32_DEVICE_TYPE` name (`AMBIBACK_DEVICE_DEFINE`), the same string as `define` in the CI `manifest.json`

Browser tools use it to match crash backtraces with the right symbol map.

## `%` commands

All text is sent as **raw UTF-8 bytes, without any length prefix**. A command is:

```text
'%' NAME ':' [data]
```

The parser skips every byte until `%`, then reads `NAME` (up to 15 characters) until `:`, a space, `0x00` or `0x04`.

String parameters end with `0x00`. Inside a string parameter, bytes `<= 0x1F` are dropped, so control characters cannot be part of an SSID or key. Older ambiback-flasher versions sent a .NET length byte in front of each string; this was harmless for strings shorter than 32 bytes and corrupted longer ones (fixed in ambiback-flasher PR #2).

| Command | Data after `:` | Effect |
|---|---|---|
| `%INFO:` | – | Replies `%info: {json}` with DeviceType, DeviceVersion, DeviceID, DeviceMAC, DeviceName, DeviceIP, LedStatus (and relay state on camera builds) |
| `%REBOOT:` | – | Reboots |
| `%FACTORYDEFAULT:` | – | Clears all settings and reboots |
| `%WIFI:` | `ssid 0x00 key 0x00` | Connects to the network. On success it stores SSID and key and reboots. SSID and key must not be empty. |
| `%RESETWIFI:` | – | Clears SSID and key and reboots |
| `%LEDMODE:` | `mode 0x00` (decimal text, e.g. `2`) | Sets the LED mode (0 off, 1 video, 2 ambient, 3 cam, 4 relay, 5 demo) |
| `%SETRELAY:` | `relayId 0x00` | Camera builds only (`ENABLE_RELAY`): selects the AmbiBack device to relay to |
| `%STARTLEDDATA:` | – | Switches the port to `MAX_SERIAL_RATE` (921600, 1500000 on some boards) for LED streaming |
| `%ENDLEDDATA:` | – | Switches back to 115200 |
| `%LEDDATA:` | binary, see below | One frame of LED colors |

### `%LEDDATA:` frame

```text
count      1 byte   number of LEDs (1..255), 0 is invalid
start      1 byte   start index (not used yet)
crcCheck   1 byte   0x00 = no check, 0x01 = check crc
crc        1 byte   CRC-8 over the LED data
data       count × 4 bytes: white, red, green, blue
```

### Example: set Wi-Fi

SSID `Home`, key `secret`:

```text
25 57 49 46 49 3A  48 6F 6D 65 00  73 65 63 72 65 74 00
 %  W  I  F  I  :   H  o  m  e      s  e  c  r  e  t
```

## Improv Wi-Fi Serial

Implements [Improv Wi-Fi Serial](https://www.improv-wifi.com/serial/) version 1.

```text
"IMPROV" | version (0x01) | type | length | data[length] | checksum
```

`checksum` is the sum of all previous bytes (mod 256). The device adds `\n` after each packet it sends.

The `%` parser hands every byte that is not part of a `%` command to the Improv parser. Once an `IMPROV` header has been received, the rest of that packet goes to Improv even if it contains `%`.

| RPC command | Response |
|---|---|
| `0x01` Send Wi-Fi settings | State `0x03` (provisioning). On success: state `0x04` (provisioned), RPC result with `http://<ip>/`, then reboot. On failure: error `0x03` (unable to connect), state `0x02` (authorized). |
| `0x02` Request current state | State `0x02` (authorized) or `0x04` (provisioned, connected as Wi-Fi station). If provisioned, also an RPC result with `http://<ip>/`. |
| `0x03` Request device info | RPC result: `AmbiBack`, firmware version, `ESP32`, device name |
| `0x04` Request scanned networks | One RPC result per network (`ssid`, `rssi`, `YES`/`NO` for secured), then an empty result |

No authorization step is needed (the device starts in state `authorized`). Unknown commands return error `0x02`, malformed packets or wrong checksums return error `0x01`.

The Wi-Fi command uses the same code path as `%WIFI:` (`global::ConnectWifiConfigured`). An empty key is passed through, so open networks work over Improv.

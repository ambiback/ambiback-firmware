# AmbiBack Scripts

## decode_backtrace

Decode ESP32 Guru Meditation backtrace using `xtensa-esp32-elf-addr2line`.

### Requirements

- `xtensa-esp32-elf-addr2line` in PATH, or ESP-IDF / Arduino ESP32 toolchain installed
- The `.elf` file from the **exact build** that produced the crash

**WSL/Linux**: Add the toolchain to PATH, or install PlatformIO (`pio run` will pull the toolchain to `~/.platformio`). The script searches `~/.espressif`, `~/.platformio`, and Arduino ESP32 package dirs.

### Usage

**With explicit ELF path:**
```bash
# From file
./decode_backtrace.sh path/to/AmbiBack.Controller.ino.elf crashdump.txt

# From stdin
echo "Backtrace:0x4009165d:0x3ffb8910..." | ./decode_backtrace.sh path/to/elf.elf

# With addresses only
./decode_backtrace.sh path/to/elf.elf 0x4009165d 0x4008ff4a 0x4008dbe0
```

**With build path (enumerates ELF files, prompts to choose):**
```bash
AMIBACK_BUILD_PATH="D:/path/to/build/_build" ./decode_backtrace.sh < crashdump.txt
./decode_backtrace.sh --build-path "D:\path\to\build\_build" crashdump.txt
```

**PowerShell:**
```powershell
.\decode_backtrace.ps1 -Elf "D:\path\to\elf.elf" -CrashFile crashdump.txt
.\decode_backtrace.ps1 -BuildPath "C:\AmbiBack\Flasher\build\_build"
```

### Configuration

Copy `decode_backtrace.conf.example` to `decode_backtrace.conf` and set your default build path:

```
AMIBACK_BUILD_PATH=C:\AmbiBack\Flasher\build\_build
```

Or set the `AMIBACK_BUILD_PATH` environment variable.

### Crash format

Supports the real ESP32 panic output, including compact formats:

```
Backtrace:0x4009165d:0x3ffb89100x4008ff4a:0x3ffb8950 0x4008dbe0:0x3ffb8990 0x4008db92:0xa5a5a5a5  |<-CORRUPTED
```

Corrupted frames (SP = 0xa5a5a5a5, 0xcdcdcdcd, etc.) are automatically skipped.

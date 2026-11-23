#!/usr/bin/env bash
# Decode ESP32 Guru Meditation backtrace using addr2line.
# Supports: Backtrace:0x4009165d:0x3ffb89100x4008ff4a:0x3ffb8950 0x4008dbe0:0x3ffb8990 0x4008db92:0xa5a5a5a5
# Usage:
#   ./decode_backtrace.sh <elf> [<crash_dump_file>]
#   ./decode_backtrace.sh <elf> 0x4009165d 0x4008ff4a 0x4008dbe0
#   ./decode_backtrace.sh --build-path "D:\path\to\build\_build" [<crash_dump_file>]
#   AMIBACK_BUILD_PATH=/path/to/build ./decode_backtrace.sh [<crash_dump_file>]

set -e

# Run with DEBUG=1 to trace (e.g. DEBUG=1 ./decode_backtrace.sh ...)
if [ -n "$DEBUG" ]; then set -x; fi

# Corrupted stack fill patterns (skip frames with these SP values)
CORRUPTED_PATTERNS="0xa5a5a5a5 0xcdcdcdcd 0xabababab 0xdeadbeef 0xffffffff"

read_toolchain_bin() {
  if [ -n "$AMIBACK_TOOLCHAIN_BIN" ]; then
    echo "$AMIBACK_TOOLCHAIN_BIN"
    return 0
  fi
  local conf
  conf="$(cd "$(dirname "$0")" && pwd)/decode_backtrace.conf"
  if [ -f "$conf" ]; then
    local val
    val=$(grep -E '^AMIBACK_TOOLCHAIN_BIN=' "$conf" 2>/dev/null | head -1 | cut -d= -f2- | tr -d '\r\n"' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    if [ -n "$val" ]; then
      echo "$val"
      return 0
    fi
  fi
  return 1
}

find_addr2line() {
  local addr2line d base
  # Config/env toolchain path (first priority)
  local toolchain_bin
  toolchain_bin=$(read_toolchain_bin 2>/dev/null)
  if [ -n "$toolchain_bin" ]; then
    # Convert Windows path to WSL: D:\foo -> /mnt/d/foo, normalize backslashes
    toolchain_bin="${toolchain_bin//\\//}"
    if [[ "$toolchain_bin" =~ ^([A-Za-z]):(.*) ]]; then
      toolchain_bin="/mnt/${BASH_REMATCH[1],,}${BASH_REMATCH[2]}"
    fi
    for exe in "$toolchain_bin/xtensa-esp32-elf-addr2line" "$toolchain_bin/xtensa-esp32-elf-addr2line.exe"; do
      [ -f "$exe" ] && { echo "$exe"; return 0; }
    done
  fi
  addr2line=$(command -v xtensa-esp32-elf-addr2line 2>/dev/null)
  if [ -n "$addr2line" ]; then
    echo "$addr2line"
    return 0
  fi
  # IDF_PATH
  if [ -n "$IDF_PATH" ]; then
    for d in "$IDF_PATH"/tools/xtensa-esp32-elf/*/xtensa-esp32-elf/bin \
             "$IDF_PATH"/.espressif/tools/xtensa-esp32-elf/*/xtensa-esp32-elf/bin; do
      [ -f "$d/xtensa-esp32-elf-addr2line" ] 2>/dev/null && { echo "$d/xtensa-esp32-elf-addr2line"; return 0; }
    done
  fi
  # ~/.espressif (WSL/Linux - glob only if parent exists)
  if [ -d "$HOME/.espressif/tools/xtensa-esp32-elf" ]; then
    for d in "$HOME/.espressif/tools/xtensa-esp32-elf"/*/xtensa-esp32-elf/bin; do
      [ -f "$d/xtensa-esp32-elf-addr2line" ] 2>/dev/null && { echo "$d/xtensa-esp32-elf-addr2line"; return 0; }
    done
  fi
  # PlatformIO (WSL/Linux)
  for base in "$HOME/.platformio/packages/toolchain-xtensa-esp32" "/home/$USER/.platformio/packages/toolchain-xtensa-esp32"; do
    [ -f "$base/bin/xtensa-esp32-elf-addr2line" ] 2>/dev/null && { echo "$base/bin/xtensa-esp32-elf-addr2line"; return 0; }
  done
  # Arduino packages (WSL/Linux)
  for base in "$HOME/.arduino15/packages/esp32/hardware/esp32" "/usr/share/arduino/hardware/esp32"; do
    [ -d "$base" ] || continue
    for d in "$base"/*/tools/xtensa-esp32-elf-gcc/*/xtensa-esp32-elf/bin; do
      [ -f "$d/xtensa-esp32-elf-addr2line" ] 2>/dev/null && { echo "$d/xtensa-esp32-elf-addr2line"; return 0; }
    done
  done
  # Arduino Windows paths (when run from WSL with Windows Arduino)
  for base in "$HOME/AppData/Local/Arduino15" "/mnt/c/Users/$USER/AppData/Local/Arduino15"; do
    [ -d "$base/packages/esp32/hardware/esp32" ] || continue
    for d in "$base/packages/esp32/hardware/esp32"/*/tools/xtensa-esp32-elf-gcc/*/xtensa-esp32-elf/bin; do
      [ -f "$d/xtensa-esp32-elf-addr2line" ] 2>/dev/null && { echo "$d/xtensa-esp32-elf-addr2line"; return 0; }
    done
  done
  # Fallback: find in toolchain dirs
  for search_dir in "$HOME/.espressif" "$HOME/.platformio" /opt/esp; do
    [ -d "$search_dir" ] || continue
    addr2line=$(find "$search_dir" -name "xtensa-esp32-elf-addr2line" -type f 2>/dev/null | head -1)
    [ -n "$addr2line" ] && { echo "$addr2line"; return 0; }
  done
  return 1
}

read_build_path() {
  if [ -n "$AMIBACK_BUILD_PATH" ]; then
    echo "$AMIBACK_BUILD_PATH"
    return 0
  fi
  local conf
  conf="$(cd "$(dirname "$0")" && pwd)/decode_backtrace.conf"
  if [ -f "$conf" ]; then
    local val
    val=$(grep -E '^AMIBACK_BUILD_PATH=' "$conf" 2>/dev/null | head -1 | cut -d= -f2- | tr -d '\r\n"' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    if [ -n "$val" ]; then
      echo "$val"
      return 0
    fi
  fi
  return 1
}

find_elf_in_build() {
  local build_root="$1"
  local elfs=()
  local d f

  if [ ! -d "$build_root" ]; then
    echo "Build path not found: $build_root" >&2
    return 1
  fi

  for d in "$build_root"/*/; do
    [ -d "$d" ] || continue
    for f in "$d"*.ino.elf "$d"*.elf; do
      if [ -f "$f" ]; then
        # Avoid duplicates (both *.ino.elf and *.elf match .ino.elf)
        [[ " ${elfs[*]} " = *" $f "* ]] || elfs+=("$f")
      fi
    done
  done

  if [ ${#elfs[@]} -eq 0 ]; then
    # Also check build_root directly
    for f in "$build_root"/*.ino.elf "$build_root"/*.elf; do
      if [ -f "$f" ]; then
        [[ " ${elfs[*]} " = *" $f "* ]] || elfs+=("$f")
      fi
    done
  fi

  if [ ${#elfs[@]} -eq 0 ]; then
    echo "No .elf files found under $build_root" >&2
    return 1
  fi

  if [ ${#elfs[@]} -eq 1 ]; then
    echo "${elfs[0]}"
    return 0
  fi

  echo "Select ELF file:" >&2
  local i=1
  for f in "${elfs[@]}"; do
    # Show subfolder name prominently: m5camtx
    local subdir
    subdir=$(basename "$(dirname "$f")")
    echo "  $i) [$subdir] $(basename "$f")" >&2
    ((i++)) || true
  done
  echo -n "Choice [1-${#elfs[@]}]: " >&2
  if [ -c /dev/tty ]; then
    read -r choice </dev/tty
  else
    read -r choice
  fi
  choice=${choice:-1}
  if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le ${#elfs[@]} ]; then
    echo "${elfs[$((choice-1))]}"
    return 0
  fi
  echo "${elfs[0]}"
  return 0
}

extract_addresses() {
  # Match addr:sp pairs; filter where sp is corrupted
  grep -oE '0x[0-9a-fA-F]+:0x[0-9a-fA-F]+' | while read -r pair; do
    pair="${pair//$'\r'/}"
    local addr sp
    addr="${pair%%:*}"
    sp="${pair#*:}"
    if [[ ! "$sp" =~ ^(0xa5a5a5a5|0xcdcdcdcd|0xabababab|0xdeadbeef|0xffffffff)$ ]]; then
      echo "$addr"
    fi
  done
}

ELF=""
BUILD_PATH=""
CRASH_FILE=""
CRASH_CONTENT=""
ADDR_ARGS=()

# Parse arguments
while [ $# -gt 0 ]; do
  case "$1" in
    --build-path)
      BUILD_PATH="$2"
      shift 2
      ;;
    -*)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
    *)
      if [[ "$1" =~ ^0x[0-9a-fA-F]+$ ]]; then
        ADDR_ARGS+=("$1")
      elif [ -f "$1" ] && [ -z "$CRASH_FILE" ] && [[ "$1" != *".elf" ]]; then
        CRASH_FILE="$1"
      elif [ -d "$1" ] && [ -z "$BUILD_PATH" ]; then
        BUILD_PATH="$1"
      elif [[ "$1" == *"Backtrace"* || "$1" =~ 0x[0-9a-fA-F]+:0x[0-9a-fA-F]+ ]]; then
        # Looks like crash dump content (e.g. "Backtrace:0x4009165d:0x3ffb8910...")
        CRASH_CONTENT="${CRASH_CONTENT:+$CRASH_CONTENT }$1"
      elif ([ -z "$ELF" ] && [ -z "$BUILD_PATH" ]) && [[ "$1" == *".elf" || -f "$1" ]]; then
        ELF="$1"
      elif [ -z "$ELF" ] && [ -z "$BUILD_PATH" ]; then
        ELF="$1"
      fi
      shift
      ;;
  esac
done

# If no ELF yet, try build path
if [ -z "$ELF" ] && [ -z "$BUILD_PATH" ]; then
  BUILD_PATH=$(read_build_path)
fi

if [ -n "$BUILD_PATH" ]; then
  # Run in main shell so read from terminal works (no command substitution subshell)
  _elf_list=()
  for d in "$BUILD_PATH"/*/; do
    [ -d "$d" ] || continue
    for f in "$d"*.ino.elf "$d"*.elf; do
      [ -f "$f" ] || continue
      [[ " ${_elf_list[*]} " = *" $f "* ]] || _elf_list+=("$f")
    done
  done
  if [ ${#_elf_list[@]} -eq 0 ]; then
    for f in "$BUILD_PATH"/*.ino.elf "$BUILD_PATH"/*.elf; do
      [ -f "$f" ] || continue
      [[ " ${_elf_list[*]} " = *" $f "* ]] || _elf_list+=("$f")
    done
  fi
  if [ ${#_elf_list[@]} -eq 0 ]; then
    echo "No .elf files found under $BUILD_PATH" >&2
    exit 1
  fi
  if [ ${#_elf_list[@]} -eq 1 ]; then
    ELF="${_elf_list[0]}"
  else
    echo "Select ELF file:" >&2
    i=1
    for f in "${_elf_list[@]}"; do
      subdir=$(basename "$(dirname "$f")")
      echo "  $i) [$subdir] $(basename "$f")" >&2
      ((i++)) || true
    done
    echo -n "Choice [1-${#_elf_list[@]}]: " >&2
    if [ -c /dev/tty ]; then read -r choice </dev/tty; else read -r choice; fi
    choice=${choice:-1}
    if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le ${#_elf_list[@]} ]; then
      ELF="${_elf_list[$((choice-1))]}"
    else
      ELF="${_elf_list[0]}"
    fi
  fi
  unset _elf_list
fi

if [ -z "$ELF" ] || [ ! -f "$ELF" ]; then
  echo "Usage: $0 <elf_file> [crash_dump_file|crash_dump_text]
       $0 --build-path <build_root> [crash_dump_file]
       $0 'Backtrace:0x4009165d:0x3ffb8910...'  (uses config for ELF)
       AMIBACK_BUILD_PATH=/path ./decode_backtrace.sh
       (with config: run with no args, paste dump when prompted)

ELF file required. Set AMIBACK_BUILD_PATH or use scripts/decode_backtrace.conf" >&2
  exit 1
fi

ADDR2LINE=$(find_addr2line) || true
if [ -z "$ADDR2LINE" ]; then
  echo "xtensa-esp32-elf-addr2line not found. Install ESP32 toolchain or add to PATH." >&2
  echo "  On WSL: install espressif-esp32, or add Arduino/ESP32 toolchain bin to PATH." >&2
  exit 1
fi

# Collect addresses: from explicit args, crash content, file, or stdin (prompt if interactive)
ADDRESSES=("${ADDR_ARGS[@]}")
if [ ${#ADDRESSES[@]} -eq 0 ]; then
  if [ -n "$CRASH_FILE" ]; then
    ADDRESSES=($(cat "$CRASH_FILE" | extract_addresses))
  elif [ -n "$CRASH_CONTENT" ]; then
    ADDRESSES=($(printf '%s' "$CRASH_CONTENT" | extract_addresses))
  else
    input=""
    if [ -t 0 ]; then
      # Interactive: no stdin, prompt user
      echo "Paste crash dump (or path to file), then Ctrl+D:" >&2
      input=$(cat)
      # If single line and it's a file path, use as crash file
      if [[ "$input" =~ ^[^[:space:]]+$ ]] && [ -f "$input" ]; then
        ADDRESSES=($(cat "$input" | extract_addresses))
      else
        ADDRESSES=($(echo "$input" | extract_addresses))
      fi
    else
      ADDRESSES=($(extract_addresses))
    fi
  fi
fi

if [ ${#ADDRESSES[@]} -eq 0 ]; then
  echo "No backtrace addresses found. Paste crash dump, pass a file, or pass addresses." >&2
  exit 1
fi

echo "Decoding ${#ADDRESSES[@]} frames (${ADDRESSES[*]}) with $ELF" >&2
if ! "$ADDR2LINE" -pfiaC -e "$ELF" "${ADDRESSES[@]}"; then
  echo "addr2line failed (exit $?). Check that ELF matches the crashed firmware." >&2
  exit 1
fi

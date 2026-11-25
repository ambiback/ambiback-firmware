#!/usr/bin/env python3
"""Build a compact symbol map from a firmware ELF for crash backtrace decoding.

Output (JSON):
  {"version": "...", "buildId": "...", "device": "...",
   "symbols": [[start, size, "function", "file:line"], ...]}   sorted by start

Only code symbols (nm types T/t/W/w) in the ESP32 instruction address space are kept.
Usage:
  make-symbols.py --elf fw.elf --nm xtensa-esp32-elf-nm --addr2line xtensa-esp32-elf-addr2line \
                  --out symbols.json --version 0.5.231 --build-id ci_x --device naked
"""
import argparse
import json
import os
import subprocess
import sys

CODE_TYPES = set("TtWw")
CODE_MIN = 0x40000000
CODE_MAX = 0x50000000
CHUNK = 400


def short_path(path):
    """Keep the useful tail of a source path (sketch file or library/file)."""
    path = path.replace("\\", "/")
    for marker in ("/libraries/", "/sketch/", "/cores/esp32/", "/AmbiBack.Controller/"):
        idx = path.rfind(marker)
        if idx >= 0:
            return path[idx + len(marker):]
    parts = [p for p in path.split("/") if p]
    return "/".join(parts[-2:]) if parts else path


def read_symbols(nm, elf):
    out = subprocess.run([nm, "-n", "-S", "-C", "--defined-only", elf],
                         check=True, capture_output=True, text=True, errors="replace").stdout
    symbols = {}
    for line in out.splitlines():
        # address size type name   (size missing for some symbols)
        parts = line.split(None, 3)
        if len(parts) < 4:
            continue
        addr_s, size_s, typ, name = parts
        if typ not in CODE_TYPES:
            continue
        try:
            addr = int(addr_s, 16)
            size = int(size_s, 16)
        except ValueError:
            continue
        if not (CODE_MIN <= addr < CODE_MAX) or size == 0:
            continue
        # Keep the first (or largest) symbol per address
        if addr not in symbols or size > symbols[addr][0]:
            symbols[addr] = (size, name)
    return symbols


def resolve_lines(addr2line, elf, addresses):
    result = {}
    for i in range(0, len(addresses), CHUNK):
        chunk = addresses[i:i + CHUNK]
        out = subprocess.run([addr2line, "-e", elf] + ["0x%08x" % a for a in chunk],
                             check=True, capture_output=True, text=True, errors="replace").stdout
        lines = out.splitlines()
        for addr, loc in zip(chunk, lines):
            loc = loc.split(" (discriminator")[0].strip()
            if loc.startswith("??"):
                result[addr] = ""
            else:
                file_part, _, line_part = loc.rpartition(":")
                result[addr] = (short_path(file_part) + ":" + line_part) if file_part else loc
    return result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--elf", required=True)
    ap.add_argument("--nm", required=True)
    ap.add_argument("--addr2line", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--version", required=True)
    ap.add_argument("--build-id", required=True)
    ap.add_argument("--device", required=True)
    args = ap.parse_args()

    symbols = read_symbols(args.nm, args.elf)
    addresses = sorted(symbols)
    lines = resolve_lines(args.addr2line, args.elf, addresses)
    table = [[a, symbols[a][0], symbols[a][1], lines.get(a, "")] for a in addresses]

    doc = {"version": args.version, "buildId": args.build_id, "device": args.device, "symbols": table}
    with open(args.out, "w", encoding="utf-8") as f:
        json.dump(doc, f, separators=(",", ":"))
    print("make-symbols: %d symbols, %d KB -> %s" % (len(table), os.path.getsize(args.out) // 1024, args.out))
    return 0


if __name__ == "__main__":
    sys.exit(main())

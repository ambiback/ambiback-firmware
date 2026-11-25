#!/usr/bin/env python3
"""Build one AmbiBack device type with arduino-cli and collect the flash files.

Used by .github/workflows/release-builds.yml; also runs locally if arduino-cli,
the ESP32 core and the libraries are installed.

Writes:
  <out>/<id>/firmware.bin, partitions.bin, bootloader.bin, boot_app0.bin,
             symbols.json, buildinfo.json
  <elf-out>/<id>.elf
"""
import argparse
import glob
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SKETCH_NAME = "AmbiBack.Controller"


def run(cmd, **kw):
    print("+ " + " ".join(cmd), flush=True)
    return subprocess.run(cmd, check=True, **kw)


def read_version(repo):
    with open(os.path.join(repo, "globalincludes.h"), encoding="utf-8", errors="replace") as f:
        m = re.search(r'#define\s+AMBIBACK_VERSION\s+"([^"]+)"', f.read())
    if not m:
        sys.exit("AMBIBACK_VERSION not found in globalincludes.h")
    return m.group(1)


def filter_options(cli, fqbn_base, wanted):
    """Keep only options (key=value) the board offers; arduino-cli stops on unknown ones."""
    out = subprocess.run([cli, "board", "details", "-b", fqbn_base, "--format", "json"],
                         check=True, capture_output=True, text=True).stdout
    details = json.loads(out)
    offered = {}
    for opt in details.get("config_options", []):
        offered[opt["option"]] = {v["value"] for v in opt.get("values", [])}
    keep = []
    for pair in [p for p in wanted.split(",") if p]:
        key, _, value = pair.partition("=")
        if key in offered and value in offered[key]:
            keep.append(pair)
        else:
            print("::notice::%s: board option %s not offered, dropped" % (fqbn_base, pair))
    return ",".join(keep)


def find_one(pattern):
    hits = sorted(glob.glob(pattern, recursive=True))
    if not hits:
        sys.exit("not found: " + pattern)
    return hits[-1]


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(65536), b""):
            h.update(block)
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--device", required=True, help="device id from build/devices.json")
    ap.add_argument("--repo", default=os.path.dirname(HERE))
    ap.add_argument("--libraries", required=True, help="folder with the libraries")
    ap.add_argument("--work", required=True, help="scratch folder")
    ap.add_argument("--out", required=True)
    ap.add_argument("--elf-out", required=True)
    ap.add_argument("--build-id", required=True, help="identifier chars only: [A-Za-z_][A-Za-z0-9_]*")
    ap.add_argument("--commit", default="")
    ap.add_argument("--cli", default="arduino-cli")
    ap.add_argument("--data-dir", default=os.path.expanduser("~/.arduino15"))
    args = ap.parse_args()

    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", args.build_id):
        sys.exit("build id must be a C identifier (it is passed as a token and stringified)")

    with open(os.path.join(HERE, "devices.json"), encoding="utf-8") as f:
        doc = json.load(f)
    core = doc["core"]
    dev = next((d for d in doc["devices"] if d["id"] == args.device), None)
    if dev is None:
        sys.exit("unknown device " + args.device)
    version = read_version(args.repo)

    # Sketch folder must be named like the .ino
    sketch = os.path.join(args.work, "sketch", SKETCH_NAME)
    shutil.rmtree(os.path.dirname(sketch), ignore_errors=True)
    shutil.copytree(args.repo, sketch, ignore=shutil.ignore_patterns(
        ".git", ".github", ".claude", "node_modules", "build", "AI", "docs", "web", "scripts"))
    build_dir = os.path.join(args.work, "out-" + dev["id"])
    shutil.rmtree(build_dir, ignore_errors=True)

    fqbn_base = "esp32:esp32:" + dev["board3"]
    options = filter_options(args.cli, fqbn_base, dev["fqbnOptions"])
    fqbn = fqbn_base + (":" + options if options else "")

    # compiler.*.extra_flags are empty in the core and meant for users, so the
    # core's own defines (CORE_DEBUG_LEVEL, PSRAM, ARDUINO_BOARD, ...) stay intact.
    flags = "-DESP32_DEVICE_TYPE=%s -DAMBIBACK_BUILD_ID_TOKEN=%s" % (dev["define"], args.build_id)
    if dev["psram"]:
        flags += " -DBOARD_HAS_PSRAM"
    if dev.get("psramFix"):
        flags += " -mfix-esp32-psram-cache-issue"

    run([args.cli, "compile", "--fqbn", fqbn,
         "--build-path", build_dir,
         "--libraries", args.libraries,
         "--build-property", "compiler.cpp.extra_flags=" + flags,
         "--build-property", "compiler.c.extra_flags=" + flags,
         "--warnings", "none", sketch])

    dest = os.path.join(args.out, dev["id"])
    os.makedirs(dest, exist_ok=True)
    os.makedirs(args.elf_out, exist_ok=True)
    pairs = {
        "firmware.bin": SKETCH_NAME + ".ino.bin",
        "partitions.bin": SKETCH_NAME + ".ino.partitions.bin",
        "bootloader.bin": SKETCH_NAME + ".ino.bootloader.bin",
    }
    for target, source in pairs.items():
        shutil.copyfile(os.path.join(build_dir, source), os.path.join(dest, target))
    boot_app0 = find_one(os.path.join(args.data_dir, "packages", "esp32", "hardware", "esp32", core,
                                      "tools", "partitions", "boot_app0.bin"))
    shutil.copyfile(boot_app0, os.path.join(dest, "boot_app0.bin"))
    elf = os.path.join(args.elf_out, dev["id"] + ".elf")
    shutil.copyfile(os.path.join(build_dir, SKETCH_NAME + ".ino.elf"), elf)

    tools = os.path.join(args.data_dir, "packages", "esp32", "tools")
    nm = find_one(os.path.join(tools, "**", "bin", "xtensa-esp32-elf-nm*"))
    addr2line = find_one(os.path.join(tools, "**", "bin", "xtensa-esp32-elf-addr2line*"))
    run([sys.executable, os.path.join(HERE, "make-symbols.py"), "--elf", elf, "--nm", nm,
         "--addr2line", addr2line, "--out", os.path.join(dest, "symbols.json"),
         "--version", version, "--build-id", args.build_id, "--device", dev["id"]])

    files = {name: {"size": os.path.getsize(os.path.join(dest, name)), "sha256": sha256(os.path.join(dest, name))}
             for name in ("bootloader.bin", "partitions.bin", "boot_app0.bin", "firmware.bin")}
    info = {"device": dev["id"], "label": dev["label"], "define": dev["define"], "version": version,
            "buildId": args.build_id, "commit": args.commit, "core": core, "fqbn": fqbn, "files": files}
    with open(os.path.join(dest, "buildinfo.json"), "w", encoding="utf-8") as f:
        json.dump(info, f, indent=2)
    print("built %s %s (%d bytes)" % (dev["id"], version, files["firmware.bin"]["size"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())

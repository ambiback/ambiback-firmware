#!/usr/bin/env python3
"""Combine the per-device build folders into manifest.json (schema 1).

Usage: make-manifest.py --dist dist --channel stable --date 2026-11-25
Reads dist/<id>/buildinfo.json for every device in build/devices.json and writes dist/manifest.json.
"""
import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

# Flash layout, same as ambiback-flasher (esptool write_flash)
PARTS = [
    (0x1000, "bootloader.bin"),
    (0x8000, "partitions.bin"),
    (0xE000, "boot_app0.bin"),
    (0x10000, "firmware.bin"),
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dist", required=True)
    ap.add_argument("--channel", default="stable")
    ap.add_argument("--date", required=True)
    args = ap.parse_args()

    with open(os.path.join(HERE, "devices.json"), encoding="utf-8") as f:
        devices = json.load(f)

    manifest = {"schema": 1, "version": None, "commit": None, "core": devices["core"],
                "date": args.date, "channel": args.channel, "devices": {}}
    missing = []
    for dev in devices["devices"]:
        info_path = os.path.join(args.dist, dev["id"], "buildinfo.json")
        if not os.path.exists(info_path):
            missing.append(dev["id"])
            continue
        with open(info_path, encoding="utf-8") as f:
            info = json.load(f)
        manifest["version"] = manifest["version"] or info["version"]
        manifest["commit"] = manifest["commit"] or info["commit"]
        if info["version"] != manifest["version"]:
            sys.exit("version mismatch in " + dev["id"])
        manifest["devices"][dev["id"]] = {
            "label": dev["label"],
            "define": dev["define"],
            "chip": "ESP32",
            "baud": dev["baud"],
            "psram": dev["psram"],
            "buildId": info["buildId"],
            "flash": {"mode": "dio", "freq": "80m", "size": "keep"},
            "parts": [{"offset": off, "path": dev["id"] + "/" + name,
                       "size": info["files"][name]["size"], "sha256": info["files"][name]["sha256"]}
                      for off, name in PARTS],
            "symbols": dev["id"] + "/symbols.json",
        }
    if missing:
        sys.exit("missing builds: " + ", ".join(missing))
    with open(os.path.join(args.dist, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
    print("manifest: version %s, %d devices" % (manifest["version"], len(manifest["devices"])))
    return 0


if __name__ == "__main__":
    sys.exit(main())

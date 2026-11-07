#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Verify the built plugin APK really contains a valid RendererPlugin v2 config.

Why this exists: aapt2's resource-string parser strips unescaped double quotes.
Using &quot; in strings.xml therefore compiles to a JSON string with *no quotes*,
which FCL's parseV2() silently rejects -- the plugin then has no v2 entry and
the ANGLE toggle never appears. The source-level checker cannot see this; it
only sees the pre-aapt2 XML. This checker reads the compiled resources.arsc.

Usage:
    python native/tools/check_plugin_apk.py --apk build/plugin/glesmod-renderer-plugin.apk

LGPL-3.0-or-later
"""
from __future__ import annotations

import argparse
import sys
import zipfile
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_APK = ROOT / "build" / "plugin" / "glesmod-renderer-plugin.apk"

errors: list[str] = []
notes: list[str] = []


def fail(msg: str) -> None:
    errors.append(msg)


def contains_any(data: bytes, texts: list[str]) -> bool:
    for text in texts:
        for enc in ("utf-8", "utf-16le"):
            if text.encode(enc) in data:
                return True
    return False


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--apk", default=str(DEFAULT_APK))
    args = ap.parse_args()
    apk = Path(args.apk)

    if not apk.is_file():
        fail(f"APK not found: {apk}")
        return report()

    try:
        with zipfile.ZipFile(apk) as z:
            names = set(z.namelist())
            manifest = z.read("AndroidManifest.xml")
            arsc = z.read("resources.arsc")

            # --- v2 metadata in the binary manifest ---------------------
            if contains_any(manifest, ["fclPlugin_V2"]):
                notes.append("manifest: fclPlugin_V2 present")
            else:
                fail("manifest: fclPlugin_V2 is missing from the compiled APK")
            if contains_any(manifest, ["gles_driver_angle"]):
                notes.append("manifest: gles_driver_angle present")
            else:
                fail("manifest: gles_driver_angle is missing from the compiled APK")

            # --- JSON string in resources.arsc --------------------------
            # The exact prefix is the cheapest proof that aapt2 kept quotes.
            if contains_any(arsc, ['{"displayName"', '{ "displayName"']):
                notes.append("resources.arsc: renderer_config JSON keeps quotes")
            else:
                fail(
                    "resources.arsc: renderer_config JSON has no quotes; "
                    "use \\\" escapes in strings.xml, not &quot;"
                )

            for key in ('"rendererId"', '"POJAVEXEC_EGL"', '"libEGL_angle.so"'):
                if contains_any(arsc, [key]):
                    notes.append(f"resources.arsc: {key} present")
                else:
                    fail(f"resources.arsc: {key} missing")

            # --- native libraries --------------------------------------
            for abi in ("arm64-v8a", "armeabi-v7a"):
                so = f"lib/{abi}/libgl_gles.so"
                if so not in names:
                    fail(f"APK is missing {so}")
                    continue
                data = z.read(so)
                if b"[GLESMod] proxy texture" not in data:
                    fail(f"{so}: proxy texture marker is missing")
                else:
                    notes.append(f"{so}: proxy texture marker present")
    except zipfile.BadZipFile as e:
        fail(f"APK is not a valid zip: {e}")
    except KeyError as e:
        fail(f"APK is missing entry: {e}")

    return report()


def report() -> int:
    print("=" * 74)
    print("compiled plugin APK")
    print("=" * 74)
    for n in notes:
        print(f"  OK   {n}")
    if errors:
        for e in errors:
            print(f"  FAIL {e}")
        print("=" * 74)
        print("verdict: FAIL")
        return 1
    print("=" * 74)
    print("verdict: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

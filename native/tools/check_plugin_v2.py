#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Validate the FCL RendererPlugin v2 config embedded in the plugin APK resources.

Why this exists: FCL's parseV2() is silent on failure. A malformed JSON string,
a missing title resource, or a typo in the ToggleableEnv key means the plugin
simply does not appear in the renderer list -- with no error shown to the user.
The v1 metadata remains as a fallback for old FCL, but new FCL prefers v2, so
v2 must be checked before packaging.

This is a static gate: it reads fcl-plugin/res/values/strings.xml and
fcl-plugin/AndroidManifest.xml, decodes the JSON, and checks the contract.

LGPL-3.0-or-later
"""
from __future__ import annotations

import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")

ROOT = Path(__file__).resolve().parents[2]
PLUGIN = ROOT / "fcl-plugin"
STRINGS = PLUGIN / "res" / "values" / "strings.xml"
MANIFEST = PLUGIN / "AndroidManifest.xml"
ANDROID_NS = "{http://schemas.android.com/apk/res/android}"

errors: list[str] = []
notes: list[str] = []


def fail(msg: str) -> None:
    errors.append(msg)


def read(path: Path) -> str:
    if not path.is_file():
        fail(f"missing file: {path.relative_to(ROOT)}")
        return ""
    return path.read_text(encoding="utf-8", errors="replace")


def android_unescape(text: str) -> str:
    """Approximate aapt2's string unescaping for the JSON resource.

    aapt2 does NOT treat &quot; as a literal quote: it decodes the entity to
    a quote, then the resource string parser strips unescaped quotes. The
    source must therefore use backslash-escaped quotes (\\"). This helper
    turns those back into real quotes for json.loads().
    """
    out: list[str] = []
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == "\\" and i + 1 < len(text):
            nxt = text[i + 1]
            if nxt == "u" and i + 5 < len(text):
                try:
                    out.append(chr(int(text[i + 2:i + 6], 16)))
                    i += 6
                    continue
                except ValueError:
                    pass
            mapping = {'"': '"', "'": "'", "\\": "\\", "n": "\n", "t": "\t", "r": "\r"}
            if nxt in mapping:
                out.append(mapping[nxt])
                i += 2
                continue
        out.append(ch)
        i += 1
    return "".join(out)


def main() -> int:
    manifest = read(MANIFEST)
    strings_xml = read(STRINGS)
    if not manifest or not strings_xml:
        return report()

    # --- manifest metadata -------------------------------------------------
    if "fclPlugin_V2" not in manifest:
        fail("AndroidManifest.xml: fclPlugin_V2 metadata is missing")
    else:
        notes.append("manifest: fclPlugin_V2 present")

    if "gles_driver_angle" not in manifest:
        fail("AndroidManifest.xml: gles_driver_angle title metadata is missing")
    else:
        notes.append("manifest: gles_driver_angle title metadata present")

    # v1 must stay as the fallback for old FCL.
    for field in ("fclPlugin", "renderer", "des", "boatEnv", "pojavEnv"):
        if field not in manifest:
            fail(f"AndroidManifest.xml: v1 fallback field {field} is missing")

    # --- string resources --------------------------------------------------
    try:
        root = ET.fromstring(strings_xml)
    except ET.ParseError as e:
        fail(f"strings.xml: XML parse failed: {e}")
        return report()

    config_text = None
    title_text = None
    for el in root.findall("string"):
        name = el.get("name")
        text = "".join(el.itertext()).strip()
        if name == "renderer_config":
            config_text = text
        elif name == "gles_driver_angle":
            title_text = text

    if config_text is None:
        fail("strings.xml: renderer_config string resource is missing")
    if not title_text:
        fail("strings.xml: gles_driver_angle title string is empty")

    if config_text is not None:
        try:
            cfg = json.loads(android_unescape(config_text))
        except json.JSONDecodeError as e:
            fail(f"renderer_config: JSON parse failed: {e}")
            return report()

        for key in ("displayName", "rendererId", "rendererGLPath",
                    "rendererEGLPath", "env", "minMCVer", "maxMCVer"):
            if key not in cfg:
                fail(f"renderer_config: missing key {key}")

        if not str(cfg.get("rendererId", "")).strip():
            fail("renderer_config: rendererId is empty")
        gl_path = str(cfg.get("rendererGLPath", ""))
        if not gl_path.endswith("libgl_gles.so"):
            fail(f"renderer_config: rendererGLPath must point at libgl_gles.so, got {gl_path!r}")
        if cfg.get("rendererEGLPath") != "libEGL.so":
            fail(
                "renderer_config: rendererEGLPath must stay libEGL.so; "
                "ANGLE is enabled only by the POJAVEXEC_EGL toggle"
            )

        env = cfg.get("env")
        if not isinstance(env, list):
            fail("renderer_config: env must be a list")
            return report()

        normal: dict[str, str] = {}
        toggles: list[dict] = []
        for item in env:
            if not isinstance(item, dict):
                fail("renderer_config: env entry is not an object")
                continue
            typ = item.get("type")
            if typ == "NormalEnv":
                normal[str(item.get("key"))] = str(item.get("value"))
            elif typ == "ToggleableEnv":
                toggles.append(item)

        if normal.get("POJAV_RENDERER") != "opengles3":
            fail("renderer_config: POJAV_RENDERER NormalEnv must be opengles3")
        if normal.get("LIBGL_ES") != "3":
            fail("renderer_config: LIBGL_ES NormalEnv must be 3")

        angle = next((t for t in toggles if t.get("key") == "POJAVEXEC_EGL"), None)
        if angle is None:
            fail("renderer_config: missing ToggleableEnv POJAVEXEC_EGL")
        else:
            if angle.get("value") != "libEGL_angle.so":
                fail("renderer_config: POJAVEXEC_EGL toggle must use libEGL_angle.so")
            if angle.get("toggle") is not False:
                fail("renderer_config: ANGLE toggle must default to false")
            title = angle.get("title")
            if not isinstance(title, dict) or title.get("key") != "gles_driver_angle":
                fail("renderer_config: ANGLE toggle title.key must be gles_driver_angle")
            else:
                notes.append("config: ANGLE toggle -> POJAVEXEC_EGL=libEGL_angle.so (default off)")

    return report()


def report() -> int:
    print("=" * 74)
    print("FCL RendererPlugin v2 config")
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

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Static gate: proxy-texture emulation must stay wired into the native layer.

Why a gate instead of a comment: `glGetTexLevelParameteriv/fv` used to be plain
`F` forwarders. If someone regenerates `symbols.def` or `generated_forwarders.c`
without keeping them in `CUSTOM_SYMBOLS`, the ANGLE startup crash comes back and
the failure is a native SIGSEGV -- exactly the kind of regression that is easy
to miss until a device test.

The gate is intentionally static and offline: it checks the wiring, not the
runtime behaviour. The runtime behaviour is covered by the device test matrix.

LGPL-3.0-or-later
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")

ROOT = Path(__file__).resolve().parents[2]

SYMBOLS = ROOT / "native" / "symbols.def"
CUSTOM = ROOT / "native" / "src" / "custom.c"
FORWARDERS = ROOT / "native" / "src" / "generated_forwarders.c"
BACKEND_H = ROOT / "native" / "include" / "gles_backend.h"

PROXY_SYMBOLS = ("glGetTexLevelParameteriv", "glGetTexLevelParameterfv")

errors: list[str] = []
notes: list[str] = []


def read(path: Path) -> str:
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")
        return ""
    return path.read_text(encoding="utf-8", errors="replace")


def check_symbols() -> None:
    text = read(SYMBOLS)
    for name in PROXY_SYMBOLS:
        m = re.search(rf"^([FCS])\s+.+?\s+{re.escape(name)}\s*\(", text, re.M)
        if not m:
            errors.append(f"symbols.def: {name} not declared")
        elif m.group(1) != "C":
            errors.append(f"symbols.def: {name} is {m.group(1)}, expected C")
        else:
            notes.append(f"symbols.def: {name} = C")


def check_custom_definitions() -> None:
    text = read(CUSTOM)
    for name in PROXY_SYMBOLS:
        # A definition line, not a call: "void glGet...(...)"
        if not re.search(rf"^void\s+{re.escape(name)}\s*\(", text, re.M):
            errors.append(f"custom.c: missing definition for {name}")

    if "glesmod_is_emulated_proxy_target" not in text:
        errors.append("custom.c: glesmod_is_emulated_proxy_target() is missing")
    if "glesmod_log_proxy_unsupported" not in text:
        errors.append("custom.c: glesmod_log_proxy_unsupported() is missing")

    if "glesmod_proxy_action(target)" not in text:
        errors.append("custom.c: glesmod_proxy_action(target) is not used")
    else:
        n = text.count("glesmod_proxy_action(target)")
        notes.append(f"custom.c: proxy policy helper used {n}x")
        if n < 4:
            errors.append(
                "custom.c: expected glesmod_proxy_action() in glTexImage2D, "
                "glTexImage3D, glGetTexLevelParameteriv and "
                "glGetTexLevelParameterfv"
            )

    for action in ("GLESMOD_PROXY_EMULATE", "GLESMOD_PROXY_REJECT"):
        if action not in text:
            errors.append(f"custom.c: {action} is missing")

    if "glesmod_proxy_emu_enabled()" not in text:
        errors.append("custom.c: glesmod_proxy_emu_enabled() is not used")
    if "glesmod_proxy_record(" not in text:
        errors.append("custom.c: glesmod_proxy_record() is not called")
    if "glesmod_proxy_query(" not in text:
        errors.append("custom.c: glesmod_proxy_query() is not used")

    if "GLESMOD_ENV_PROXY_EMU" not in text:
        errors.append("custom.c: GLESMOD_ENV_PROXY_EMU is not read")
    else:
        notes.append("custom.c: GLESMOD_PROXY_EMU is wired")


def check_forwarders() -> None:
    text = read(FORWARDERS)
    for name in PROXY_SYMBOLS:
        # The generated forwarder would define the function; its body contains
        # glesym_resolve("name"). If present, the symbol is duplicated.
        if re.search(rf"^GLESMOD_EXPORT\s+void\s+{re.escape(name)}\s*\(", text, re.M):
            errors.append(
                f"generated_forwarders.c: still defines {name}; "
                "regenerate after changing symbols.def"
            )
        else:
            notes.append(f"generated_forwarders.c: {name} not duplicated")


def check_env_definition() -> None:
    text = read(BACKEND_H)
    if "GLESMOD_ENV_PROXY_EMU" not in text:
        errors.append("gles_backend.h: GLESMOD_ENV_PROXY_EMU is not defined")


def main() -> int:
    check_symbols()
    check_custom_definitions()
    check_forwarders()
    check_env_definition()

    print("=" * 74)
    print("proxy texture emulation wiring")
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

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Check whether the built native libraries / APK actually contain the
latest converter fix.

Self-locating, no args.  Reads the .so files directly (they are the artifact
that performs shader conversion at runtime) and the copy inside the APK.
"""
import glob
import os
import sys
import zipfile

sys.stdout.reconfigure(encoding="utf-8")

NEEDLES = [
    (b"v10", "probe banner v10 (当前源码版本)"),
    (b"v9", "probe banner v9 (旧版本)"),
    (b"isamplerBuffer", "采样器类型表 isamplerBuffer (本次修复新增)"),
    (b"removed desktop-only #extension", "normalize_preprocessor_directives (#extension)"),
    (b"GLESMOD_ERROR_DIRECTIVE_NORMALIZED", "normalize_preprocessor_directives (#error)"),
    (b"GLESMOD_MULTIDRAW_NATIVE", "GL_EXT_multi_draw_indirect native path"),
    (b"precision highp ", "采样器精度注入代码"),
    (b"depth_readback", "旧深度回读实验 (v10 应已移除)"),
]

targets = []
for p in ("build/native/arm64-v8a/libgl_gles.so",
          "build/native/armeabi-v7a/libgl_gles.so"):
    if os.path.isfile(p):
        targets.append(("FILE", p))

apk = "build/plugin/glesmod-renderer-plugin.apk"
if os.path.isfile(apk):
    targets.append(("APK", apk))

if not targets:
    print("no artifacts found")
    sys.exit(1)


def scan_bytes(label, data):
    print("  %s  (%d bytes)" % (label, len(data)))
    for needle, desc in NEEDLES:
        found = needle in data
        mark = "YES" if found else " -- "
        print("    [%s] %-45s %s" % (mark, desc, needle.decode("utf-8", "replace")))


for kind, path in targets:
    print("=" * 74)
    print("%s: %s" % (kind, path))
    print("=" * 74)
    if kind == "FILE":
        scan_bytes(os.path.basename(path), open(path, "rb").read())
    else:
        with zipfile.ZipFile(path) as z:
            for n in z.namelist():
                if n.endswith("libgl_gles.so"):
                    scan_bytes(n, z.read(n))
    print()

print("mtime:")
for kind, path in targets:
    import datetime
    print("  %-24s %s" % (os.path.basename(path),
                          datetime.datetime.fromtimestamp(
                              os.path.getmtime(path))))

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Run every regression gate and print one compact verdict line each.

Rationale: this terminal mangles inline python containing parentheses or quotes,
and PowerShell 5.1 redirection writes UTF-16. A script file that captures
subprocess output itself is the only reliable path -- same reasoning as
run_mc_gate.py.

LGPL-3.0-or-later
"""
import os
import subprocess
import sys

sys.stdout.reconfigure(encoding="utf-8")

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PY = sys.executable


def run(args, cwd=ROOT):
    r = subprocess.run(args, capture_output=True, text=True,
                       encoding="utf-8", errors="replace", cwd=cwd)
    return r.returncode, (r.stdout or "") + (r.stderr or "")


def line_of(text, keys):
    for l in text.splitlines():
        s = l.strip()
        if any(k in s for k in keys):
            return s
    return "(no verdict line)"


def main():
    gates = []

    # 1. roundtrip (convert fixtures -> glslang)
    rc, out = run([PY, "-X", "utf8", os.path.join(HERE, "roundtrip_validate.py")])
    gates.append(("roundtrip", rc, line_of(out, ["结果"])))

    # 2. Veil pinwheel audit
    rc, out = run([PY, "-X", "utf8", os.path.join(HERE, "veil_audit.py")])
    rep = os.path.join(ROOT, "veil-audit-report.txt")
    verdict = "(no report)"
    if os.path.isfile(rep):
        txt = open(rep, encoding="utf-8", errors="replace").read()
        verdict = line_of(txt, ["结果:"])
    gates.append(("veil pinwheel", rc, verdict))

    # 3. MC core shaders (PowerShell wrapper)
    rc, out = run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
                   "-File", os.path.join(ROOT, "audit-mc-shaders.ps1")])
    gates.append(("mc core", rc, line_of(out, ["通过 ", "全部着色器"])))

    # 4. BSL pack
    rc, out = run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
                   "-File", os.path.join(ROOT, "validate-pack-all.ps1")])
    gates.append(("bsl pack", rc, line_of(out, ["通过 "])))

    # 5. Flywheel device dumps
    rc, out = run([PY, "-X", "utf8",
                   os.path.join(HERE, "rebuild_and_verify_flywheel.py")])
    npass = out.count("verdict: PASS")
    gates.append(("flywheel dumps", rc, "verdict PASS x%d" % npass))

    # 6. Sable: splice Sable's Flywheel overrides into a real device dump and
    #    convert + judge. Offline proxy for "does Sable's shader work on us".
    rc, out = run([PY, "-X", "utf8",
                   os.path.join(HERE, "sable_splice_audit.py")])
    sable = []
    for l in out.splitlines():
        if "corruption:" not in l:
            continue          # skips the "baseline dump lines: 973" banner
        for tag in ("baseline", "sable "):
            if l.startswith(tag):
                tail = l.split("corruption:", 1)[-1].strip()
                sable.append("%s corruption:%s" % (tag.strip(), tail))
                break
    gates.append(("sable splice", rc, " | ".join(sable) if sable else "(no output)"))

    # 7. .gitignore: third-party material must never reach git history.
    #    History is permanent, so this is checked by machine, not by eye.
    rc, out = run([PY, "-X", "utf8", os.path.join(HERE, "check_gitignore.py")])
    gates.append(("gitignore", rc, line_of(out, ["verdict:"])))

    # 8. Real device fixtures replayed offline.
    #    These are NOT synthetic probes: they are the exact shader sources our
    #    backend dumped in native.log when Simulated's shaders failed to compile
    #    on the phone (latest.log 14:20:03 / 14:20:04). Keeping them as a gate is
    #    the only way the gap that let defect A/B through stays closed -- no other
    #    corpus in this repo used `textureSize()` at all.
    rc, out = run([PY, "-X", "utf8",
                   os.path.join(HERE, "verify_device_fixtures.py")])
    nfix = out.count("我们修对了")
    nbad = out.count("回归") + out.count("仍有错误")
    gates.append(("device fixtures", rc,
                  "%d 修复 / %d 异常" % (nfix, nbad)))

    # 9. Built-in int-argument probes (defect A's narrow cases).
    #
    #    IMPORTANT: regenerate the conversions first. attrib_probe_builtin.py
    #    only judges .conv files that probe_builtin_int_args.py wrote -- run it
    #    alone and it reports whatever is left on disk, which after a converter
    #    change is a STALE artifact. That mistake already produced one phantom
    #    "regression 4" gate line. Generating here keeps it self-consistent.
    run([PY, "-X", "utf8", os.path.join(HERE, "probe_builtin_int_args.py")])
    rc, out = run([PY, "-X", "utf8",
                   os.path.join(HERE, "attrib_probe_builtin.py")])
    gates.append(("builtin int args", rc, line_of(out, ["verdict"])))

    # 10. Artifact freshness. A green code fix means nothing if the APK the
    #     device loads was never rebuilt -- that is exactly what happened on
    #     2026-10-09 21:23, where the phone reported the SAME errors as before
    #     because build-all.ps1 was run without -WithPlugin. Compare markers,
    #     not timestamps: only markers prove the patch is inside the binary.
    rc, out = run([PY, "-X", "utf8",
                   os.path.join(HERE, "check_artifact_freshness.py")])
    # count only the per-blob verdict lines (they end with OK / 缺标记 N 个)
    nblob = sum(1 for l in out.splitlines()
                if l.startswith("  ") and (l.endswith("OK") or "缺标记" in l))
    gates.append(("artifact freshness", rc,
                  "%d 产物 | %s" % (nblob, line_of(out, ["verdict"]))))

    # 11. Documentation facts. Docs drift silently: nobody notices that a figure
    #     like "20 custom implementations" became wrong until a reader is misled.
    #     This gate cross-checks the numbers the docs assert against the actual
    #     source -- it has already caught four stale claims in these guides.
    rc, out = run([PY, "-X", "utf8",
                   os.path.join(HERE, "verify_doc_claims.py")])
    gates.append(("doc facts", rc, line_of(out, ["verdict"])))

    # 12. Proxy texture emulation wiring. This is the guard that keeps the
    #     ANGLE startup fix from being silently undone by a symbols.def /
    #     forwarder regeneration. It is static on purpose: the failure mode is
    #     a native SIGSEGV, which a host-only test cannot catch.
    rc, out = run([PY, "-X", "utf8",
                   os.path.join(HERE, "check_proxy_emulation.py")])
    gates.append(("proxy emu", rc, line_of(out, ["verdict"])))

    # 13. FCL RendererPlugin v2 config. parseV2() is silent on failure: a
    #     malformed JSON or missing title resource makes the plugin disappear
    #     from the renderer list without an error. Check it before packaging.
    rc, out = run([PY, "-X", "utf8",
                   os.path.join(HERE, "check_plugin_v2.py")])
    gates.append(("plugin v2", rc, line_of(out, ["verdict"])))

    print("=" * 74)
    for name, rc, v in gates:
        print("%-16s rc=%-3s %s" % (name, rc, v[:150]))
    print("=" * 74)
    bad = [g for g in gates if g[1] != 0]
    print("failing gates: %d" % len(bad))
    for b in bad:
        print("  - %s" % b[0])
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())

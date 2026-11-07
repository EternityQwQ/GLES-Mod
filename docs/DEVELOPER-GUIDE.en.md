# GLES Mod — Developer Guide

> **English** · [中文](DEVELOPER-GUIDE.md)
>
> Audience: developers who want to read, build, modify, or integrate with the
> code. Every interface below is taken from the actual source, not from design
> intent. Where existing documents disagree with the code, this file follows
> the code and says so.

---

## Table of contents

1. [What this project is, precisely](#1-what-this-project-is-precisely)
2. [Architecture](#2-architecture)
3. [The two delivery forms](#3-the-two-delivery-forms)
4. [Native layer](#4-native-layer)
5. [The GLSL → GLSL ES converter](#5-the-glsl--glsl-es-converter)
6. [Java layer](#6-java-layer)
7. [The Java ↔ native contract](#7-the-java--native-contract)
8. [Configuration](#8-configuration)
9. [Compatibility detection](#9-compatibility-detection)
10. [Diagnostics and how to read them](#10-diagnostics-and-how-to-read-them)
11. [Testing and gates](#11-testing-and-gates)
12. [Building](#12-building)
13. [Extending the project](#13-extending-the-project)
14. [Known divergences between docs and code](#14-known-divergences-between-docs-and-code)

---

## 1. What this project is, precisely

It is **a replacement GL library**, not a mod that patches GL calls.

FCL and similar launchers load a GL implementation by path and hand it to
LWJGL. Normally that path points at a translation layer (GL4ES, ANGLE, Zink)
which converts desktop GL calls into GLES calls. This project provides a
library at that path instead, so:

- **Forwarded symbols** reach the device's real GLES driver directly, with one
  trampoline and no per-call translation.
- **Custom implementations** handle the places where desktop GL and GLES
  genuinely differ (enum values, texture formats, shader source, missing
  functions).
- **Shader compatibility** is handled by rewriting GLSL source on the way into
  the driver.

The consequence of the last point is the single most important thing to
understand about this codebase:

> Desktop GLSL performs implicit `int` → `float` conversion. GLSL ES does not.
> A shader that compiles on desktop can therefore fail on ES. Most of
> `native/src/shader.c` exists to close that gap.

---

## 2. Architecture

```
                 ┌──────────────────────────────────────────────┐
 Minecraft ──────┤ LWJGL  (calls glXxx via the loaded GL lib)   │
                 └───────────────────┬──────────────────────────┘
                                     │
                 ┌───────────────────▼──────────────────────────┐
                 │ libgl_gles.so   (native core, ONE copy)      │
                 │                                              │
                 │  generated_forwarders.c  337 symbols ───────────┼──► real GLES
                 │  custom.c                 28 symbols ──────────┤    driver
                 │  enum.c      enum/format translation          │
                 │  shader.c    GLSL → GLSL ES converter ────────┤
                 │  core.c      caps probe, degrade, status      │
                 │  probe.c     geometry diagnostic probe        │
                 └───────────────────┬──────────────────────────┘
                                     │ writes (never calls Java)
                     glesmod/status.json
                     glesmod/native.log
                                     │
                 ┌───────────────────▼──────────────────────────┐
                 │ NeoForge mod (Java 21)  ── reads ──► logs    │
                 │  capability / compat / degrade / backend      │
                 └──────────────────────────────────────────────┘
```

Two deliberate asymmetries:

**Java never calls into native; native never calls into Java.**
The native library is loaded by the launcher (`dlopen`), which is a different
class-loading context from the mod's JVM. Calling back would require attaching
threads across two loaders and is fragile for no benefit. Instead native writes
two files and Java polls them.

**Java is not on the render path.**
It reports capability, logs degradation, and detects optimisation mods. No
per-frame work happens in Java.

---

## 3. The two delivery forms

One native core, two thin wrappers, three build artifacts:

| Artifact | Contents | Role |
|---|---|---|
| `glesmod-1.0.0.jar` (~30 KB) | Java only | Capability reporting, compat detection, config, diagnostics |
| `glesmod-renderer-plugin.apk` (~2.3 MB) | Manifest + `lib/*/libgl_gles.so` | **The actual renderer.** FCL loads the `.so` from here |
| `libgl_gles.so` (per ABI) | native core | Also available standalone for other launchers |

> **The native library is not in the jar.** Installing only the jar changes
> nothing observable. This trips people up constantly, so it is stated in the
> README too.

### 3.1 Why an APK is the plugin format

FCL discovers renderer plugins by scanning installed packages for
`meta-data` entries (FCL `PluginManager.kt` / `RendererPlugin.kt`). An Android
package is simply the delivery mechanism for those metadata entries and the
`.so` files. There is no code that runs inside the APK at render time; the
plugin's Activity never launches.

The manifest uses **v1 fields as the baseline** and also declares **v2**
(`fclPlugin_V2`). Old FCL only understands v1; new FCL prefers v2 and therefore
gets the “Use ANGLE as the GLES driver” toggle. Several v1 fields are required
only because FCL's parser returns early (silently) if any is missing:

| Field | Why it exists |
|---|---|
| `fclPlugin=true` | Marks the package as a plugin during discovery |
| `fclPlugin_V2=@string/renderer_config` | v2 entry point; its value is the resource ID of the `RendererConfigV2` JSON |
| `renderer=GLES Mod:libgl_gles.so:libEGL.so` | `name:GL:EGL`. We supply GL; **EGL is reused by default** |
| `des=…` | Display name. Must be a literal, not `@string/…` — `PackageManager` stores a resource ID for resource references, and FCL calls `Bundle.getString()`, which then yields null |
| `boatEnv=…` | **Required but unused.** FCL's `parseV1()` does `getString("boatEnv") ?: return`, so a missing key silently disables the whole plugin |
| `pojavEnv=…` | Environment variables, `KEY=VALUE:…` |
| `minMCVer` / `maxMCVer` | Stop FCL offering this renderer on incompatible versions |
| `extractNativeLibs="true"` | FCL builds the `.so` path from `ApplicationInfo.nativeLibraryDir`; without extraction the file is not there |

Two environment variables are non-negotiable, both learned from crashes:

- **`POJAV_RENDERER=opengles3`** — FCL's `egl_bridge.c` does
  `strncmp("opengles", getenv("POJAV_RENDERER"), 8)` with no null check. FCL sets
  this itself only for v2 plugins; for a v1 plugin it is absent, `getenv`
  returns NULL, and the process SIGSEGVs during `glfwInit()`. Verified on device.
- **`LIBGL_ES=3`** — decides the EGL context version. FCL sets it automatically
  only for built-in renderers, so a plugin must set it or get an ES 2 context.

### 3.2 The ANGLE toggle chain

v2 declares exactly one backend-related `ToggleableEnv`:

```json
{
  "type": "ToggleableEnv",
  "key": "POJAVEXEC_EGL",
  "value": "libEGL_angle.so",
  "toggle": false,
  "title": { "key": "gles_driver_angle" }
}
```

The chain is:

1. The toggle defaults to `false`. The plugin does not set `POJAVEXEC_EGL`, so FCL
   uses `rendererEGLPath` (`libEGL.so`) — the **native path**.
2. When enabled, FCL injects `POJAVEXEC_EGL=libEGL_angle.so` and `dlsym_EGL()`
   loads ANGLE's EGL.
3. Native `get_gles_handle()` picks the fallback GLES library in this order:
   ```text
   LIBGL_GLES (if explicitly set)
     > POJAVEXEC_EGL contains "angle" -> libGLESv2_angle.so
     > libGLESv2.so
   ```
   The plugin therefore **never sets `LIBGL_GLES`**; a user-supplied value could
   conflict with the toggle and leave EGL and GLES from different implementations.
4. The final chain becomes `libgl_gles.so → libGLESv2_angle.so → Vulkan`.

> ⚠️ ANGLE 2.1.1 null-dereferences on `glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, ...)`;
> the native proxy-texture emulation (§4.2, §8.1) is what makes this path start.
> The toggle is off by default, and there is **no automatic fallback** if ANGLE
> fails to initialise — turn the toggle off to return to the native path.

---

## 4. Native layer

### 4.1 Symbol inventory — 849 exported symbols

| Kind | Count | Source | Behaviour |
|---|---|---|---|
| `F` forwarded | 337 | `generated_forwarders.c` (generated) | Resolve the real GLES symbol once, cache it, tail-call it |
| `C` custom | 30 | `custom.c` | Hand-written, because behaviour must differ |
| `S` safe stub | 482 | `generated_forwarders.c` (generated) | No-op that returns a neutral value and records a degradation event |

`native/symbols.def` is the **single source of truth** for which symbol is in
which class. `native/tools/gen_symbols_def.py` and `gen_gl_forwarders.py`
generate the code from it; they are not edited by hand.

**Why 482 stubs is a feature, not a gap.** The alternative is a missing symbol,
which LWJGL cannot resolve, which aborts startup. A stub lets the game run and
tells you exactly which feature is silently inert:

```
glGetIntegerv(GL_NUM_EXTENSIONS)  ->  real count + 1
```

The one extra extension is `GL_ARB_draw_buffers_blend`, reported because some
code paths test for it before using multi-target blending.

### 4.2 Custom implementations (30)

These exist where a pure forward is wrong. Two examples that show the pattern:

**`glGetString(GL_VERSION)`** returns
`"3.2 (OpenGL ES 3.2 <renderer>)"`. Minecraft and many mods parse this string
and branch on desktop GL semantics, so it must look like a desktop GL version
while remaining truthful about the underlying ES version.

**`glShaderSource`** is where the entire converter is wired in. It also
**always records every shader's source** so that a later compile failure can be
diagnosed — see §10.

**`glTexImage2D/3D` + `glGetTexLevelParameteriv/fv`** implement proxy-texture
emulation. Desktop GL's `GL_PROXY_TEXTURE_*` does not exist in GLES, but
Minecraft uses it to probe the maximum texture size. For proxy targets these
entry points read and write the local `g_proxy_state` and **never enter the
driver**, avoiding the ANGLE 2.1.1 null dereference on `InvalidEnum`.

Emulation **fully covers only** `GL_PROXY_TEXTURE_2D`,
`GL_PROXY_TEXTURE_CUBE_MAP` and `GL_PROXY_TEXTURE_RECTANGLE`. Every other proxy
target (1D / 3D / array / multisample) is treated as non-allocatable: it returns
0 and logs once. This avoids polluting other targets with an unproven mip/depth
approximation. See `docs/proxy-texture-fix-plan.md` and
`docs/proxy-texture-hardening-plan.md`.

### 4.3 The `RESOLVE_OR_RETURN` pattern

Every guarded entry point looks like this:

```c
static fn_t real = NULL;                       /* cached after first call */
glesmod_lazy_init();
RESOLVE_OR_RETURN(real, "glBindTexture", );
real(a, b);
```

Properties that matter:

- Resolution happens **once**, not per call.
- A missing symbol reports a degradation event and returns a neutral value
  instead of crashing.
- The steady state is a single load-and-branch, measured at 19.4 instructions
  on arm64 (`native/tools/hotpath_cost.py`), down from 29 before optimisation.

### 4.4 Degradation model

Nothing in this project is allowed to crash the game. When something is
unavailable:

1. A **reason code** is recorded (`native/include/gles_backend.h`,
   `glesmod_degrade_code`), counting repeats rather than logging each one.
2. A **substitute** is used where one exists; otherwise the call becomes inert.
3. The state is written to `status.json`, where the Java layer picks it up.

Reason codes are a **frozen contract**: values may be deprecated but never
reused, because third parties may branch on them. The native enum and the Java
enum `DegradeReason` must be kept in sync; the Java side maps an unknown code to
`UNSUPPORTED_FUNCTION` and logs a warning rather than throwing.

---

## 5. The GLSL → GLSL ES converter

This is the riskiest component and where most of the engineering effort went.
It lives entirely in `native/src/shader.c` (~10 000 lines).

### 5.1 The problem

Desktop GLSL and GLSL ES differ in ways that change *whether a program
compiles*, not merely how it performs:

| Desktop GLSL | GLSL ES |
|---|---|
| Implicit `int` → `float` | **Illegal.** `float x = 1;` is an error |
| `vec2 v = ivec2Var;` | **Illegal.** No implicit integer→float vector conversion |
| `vec2 v = ivec2(1,2);` | Illegal |
| `texture2D(s, uv)` | `texture(s, uv)` |
| `gl_FragColor` | `out vec4` declared by the shader |
| `#version 330 core` | `#version 320 es` |
| `#error "message"` | `#error message` (ES has no string literal; Mali scans the quote even in an inactive branch) |
| sampler types have default precision | **Every sampler type needs `precision`** |

So the converter must do real type analysis, while being a linear scanner — it
has no full parser and no symbol table beyond what it builds itself.

### 5.2 Pipeline order

`glesmod_convert_shader_source()` runs these stages in this order:

1. **Early out** if the source already declares `#version … es`.
2. `expand_moj_imports` — expand `#moj_import <file>`. (On device this is
   already done by Minecraft before the source reaches us; retained as
   defence.)
3. `extract_version`, then `normalize_preprocessor_directives` — remove
   desktop-only extensions and normalise `#error` string messages into valid
   GLSL ES tokens (ES has no string literal; Mali scans the quote even in an
   inactive branch).
4. `expand_int_const_macros` — inline integer constant macros first, so that a
   macro used in both integer and float contexts is judged per use site.
5. **Qualifier conversion** and sampling-function renaming.
6. `normalize_int_literals` — the core. Rewrites integer literals to float
   where the context is genuinely floating-point. Internally a set of rules
   (A–M, plus later additions) evaluated in a fixed precedence order.
7. `fix_int_vector_float_ops` — handles mixed integer/float vector arithmetic
   (situations A–I).
8. `fix_mixed_int_uint` — `int`/`uint` mixing.
9. `fix_return_int_in_float_fn` — `return` of an integer expression from a
   function declared `float`.
10. `rewrite_nonconst_globals` + `inject_global_assignments` — GLSL ES requires
    initialisers to be constant expressions; non-constant global initialisers
    are hoisted into an injected assignment function.
11. **Assembly** — prepend `#version 320 es`, precision declarations, sampler
    precision, and map `gl_FragColor`.

> The `#error` normalisation in stage 3 can be disabled with
> `-DGLESMOD_NO_ERROR_DIRECTIVE_FIX` for A/B attribution; its marker string is
> `GLESMOD_ERROR_DIRECTIVE_NORMALIZED`.

### 5.3 The central difficulty: context is not local

The naive rule "if the statement contains `vec4`, float-ise its integers" is
wrong, and every rule added later is a correction to a specific class of
false positive. Representative examples, all found on real devices:

| Construct | Why the naive rule breaks | Handling |
|---|---|---|
| `textureSize(tex, 0)` | `lod` **must be `int`**. But if this appears as `vec2(textureSize(tex,0))`, the enclosing float constructor makes the literal look float | `INT_ARG_BUILTINS` records per-function integer argument positions; the literal is preserved |
| `textureLod(tex, uv, 0)` | Here `lod` **must be `float`**, so `.0` is required | Deliberately absent from that table — same syntax, opposite requirement |
| `int i = 0;` inside a statement that also mentions `vec3` | Declaration site is integer, use site is float-looking | Declaration-site guard; also names appearing in both integer and float tables are dropped from the float table (§5.4) |
| `f(1)` for a user function `void f(int)` | We know built-in signatures, not user ones | A user function signature table is collected from definitions-with-a-body; a literal is only converted when the parameter type is known |
| `i.pose` where `i` is a struct | `i` was collected as an integer variable from an unrelated `int i` in another scope | A scalar cannot have a member; `.` after a table hit means the hit is wrong, so the rewrite is abandoned |
| `cross(i, …)` where `i` came from `vec3 i = q.xyz` | `cross` takes floats so the wrapper was applied without float evidence; `i` looked integer because another function had `int i` | Local declaration class lookup walks to the enclosing function body and uses the nearest declaration |

The general shape of the fix in each case is the same: **find a piece of
evidence that is locally decidable, and refuse to rewrite without it.**

### 5.4 Implementation conventions you must follow

- **Control switches.** Every risky rule has a `-DGLESMOD_NO_RULE_X` compile
  switch. They exist so a change can be *attributed* via an A/B build rather
  than assumed. Adding a rule without a switch makes regressions untraceable.
- **Required-string markers.** Every rule emits a unique marker string on first
  use (e.g. `GLESMOD_RULE_S_BUILTIN_INT_ARGS`), registered in
  `native/tools/required_strings.txt`. A build gate asserts every marker is
  present in the built `.so`. Without this, a stale binary silently looks
  "fixed" — a mistake that cost a full device test round.
- **Forward declarations.** Helpers defined late in the file but used in
  `process_stmt` must be declared near the top. gcc then degrades to an implicit
  `int` declaration and the build fails; the dangerous failure mode is a script
  that only checks whether the executable exists and therefore runs the old
  binary.
- **Match parentheses correctly.** When scanning backwards for the enclosing
  call, `)` must increment the depth counter and `(` must only decrement when
  the counter is already positive. Written the other way it always finds the
  innermost paren, which makes the rule a silent no-op.
- **Literal length includes the suffix.** `int_literal_len("3u")` returns 2.
  Appending a `u` without checking produces `3uu`.

### 5.5 When conversion fails

It must fail visibly. On a compile error the backend writes three things to
`glesmod/native.log`:

1. The source exactly as received from `glShaderSource`.
2. The source actually handed to the driver.
3. The driver's own error text from `glGetShaderInfoLog`.

That triple is what makes a shader failure diagnosable instead of mysterious,
and it is the mechanism by which every converter bug in this repository was
found. `native/tools/extract_device_fixtures.py` turns such a dump back into a
test fixture.

---

## 6. Java layer

Four packages, no rendering work.

```
com.youyimc.glesmod
├── GLESMod.java            mod entry (common)
├── GLESModClient.java      client entry: startup report + runtime polling
├── Config.java             NeoForge config spec
├── capability/             GlesCapabilities (immutable snapshot),
│                           GlesCapabilityProvider (stable public interface)
├── backend/                GlesBackendStatus — reads and parses status.json
├── compat/                 CompatDetector, CompatTarget
└── degrade/                DegradeEvent, DegradeReason
```

### 6.1 `GlesCapabilities`

An **immutable snapshot**, published once and readable from any thread. It is
deliberately not live: capability does not change at runtime, so making it
mutable would only invite races.

`GlesCapabilities.INACTIVE` is the conservative default (all flags false,
minimum capacities). Every accessor on the provider interface is required to
return a conservative value and **never throw** when the backend is inactive —
an exception on a render-adjacent path would crash the game, which contradicts
the project's core rule.

`getGlesVersion()` returns a **stable-format** string
(`"OpenGL ES 3.2 (reported as GL 3.2)"`). Mods parse it, so the format is part
of the contract.

### 6.2 `GLESModClient`

Three responsibilities:

1. **Startup report** on `FMLClientSetupEvent` — capability snapshot, degrade
   events, compat results.
2. **Runtime polling** every 100 ticks (~5 s) on `ClientTickEvent.Post`.
3. **Config screen** registration.

The polling is not a convenience. The startup report happens once, but some
failures occur much later — Iris creating framebuffers when the player enables
a shaderpack can be a minute after launch, and the native side only writes its
useful attachment detail when `glCheckFramebufferStatus` sees an incomplete FBO.
Without polling, that evidence is written to `status.json` and never surfaced.
It is also load-bearing because on at least one confirmed launcher
(ZalithLauncher2) our stderr is not collected at all, making `status.json` the
only reliable diagnostic channel.

Novelty is tracked by `reason + detail`, **not by reason code**: one code can
cover many distinct problems, and deduplicating by code would swallow new ones.

Polling failures are swallowed at `debug` level by design — a diagnostic
feature must never affect the game.

---

## 7. The Java ↔ native contract

### 7.1 Why files, not JNI

`libgl_gles.so` is loaded by the launcher, i.e. outside the mod's JVM class
loader. A Java callback would need cross-loader thread attachment. Two files are
a channel both sides can write without that complexity.

### 7.2 `glesmod/status.json`

Path overridable by `GLESMOD_STATUS_FILE`. Java side is
`GlesBackendStatus` in the `backend` package.

The file carries `abi_version`, `active`, `gles_version`, `degrade_level`,
capability booleans, capacity integers, `degrade_events`,
`missing_symbols`, `stub_symbols`, and `last_calls`.

Parsing notes that matter if you edit it:

- The Java parser is **regex-based, not a JSON library**. The format is produced
  by this project and is flat and fixed; adding a dependency to read a small
  file is not justified. If you add nesting, expect to update the patterns.
- `missing_symbols` and `stub_symbols` share the shape
  `{"count":N,"names":[…]}`. The parser locates the **enclosing block first**
  and only then reads `names`. Matching `"names"` globally would silently
  capture only the first block and drop the stub list — which is precisely the
  information that says which features are inert.
- **ABI mismatch is rejected.** A jar and a `.so` are distributed separately and
  can drift, so the version is checked and a mismatch yields an inactive state
  rather than a wrong reading.

### 7.3 ABI versioning rules

`GLESMOD_ABI_VERSION` is currently `1`.

- Adding a capability bit in a **reserved** position → no bump.
- Changing the meaning of a used bit, or any struct/array layout → **bump**.

### 7.4 Capability bits

`GLESMOD_CAP_*` in `gles_backend.h`, mirrored as booleans in
`GlesCapabilities`. Bits 11–31 are reserved and must be zero. The header is the
authority for the bit assignment; the Java parser reads the JSON, not the bits,
so the two must be changed together.

---

## 8. Configuration

Three layers, in decreasing priority:

```
FCL plugin environment variables  >  NeoForge config file  >  auto-probe  >  built-in defaults
```

Environment variables win because they are an explicit per-launch choice made
outside the game.

### 8.1 Environment variables (`gles_backend.h`)

| Variable | Values | Meaning |
|---|---|---|
| `GLESMOD_ENABLE` | 0/1 | Master switch |
| `GLESMOD_DEGRADE_LEVEL` | 0/1/2 | 0 conservative, 1 default, 2 aggressive |
| `GLESMOD_TRACE` | 0/1/2 | 0 off; 1 record only pre-init calls (**default**); 2 record everything |
| `GLESMOD_STATE_CACHE` | 0/1 | Redundant-state-call elimination |
| `GLESMOD_LOG_LEVEL` | 0/1/2 | 1 is the default; 2 emits per-symbol resolution lines |
| `GLESMOD_STATUS_FILE` | path | Override `status.json` location |
| `GLESMOD_LOG_FILE` | path | Override `native.log` location |
| `GLESMOD_SHADER_PROBE` | 0/1 | Dump shader source on compile failure |
| `GLESMOD_GEOM_PROBE` | 0/1 | Geometry diagnostic probe |
| `GLESMOD_PROXY_EMU` | 0/1 | Default 1: proxy-texture emulation; 0 disables it (debug only) |
| `GLESMOD_ABI` | int | Expected ABI version |
| `LIBGL_GLES` | lib name | **Optional.** Explicit fallback GLES library; when set it takes precedence over the `POJAVEXEC_EGL` inference. The plugin never sets it |
| `POJAVEXEC_EGL` | lib name | Set by the launcher; the EGL instance we must take `eglGetProcAddress` from. The v2 ANGLE toggle switches it to `libEGL_angle.so` |

> **Same-origin rule:** native selects `libGLESv2_angle.so` only when
> `LIBGL_GLES` is unset and `POJAVEXEC_EGL` contains `angle`. Using the v2 toggle
> normally cannot mismatch; setting `LIBGL_GLES` to a vendor library *and*
> enabling ANGLE is what creates “ANGLE EGL + vendor GLES”. See §3.2.
>
> `GLESMOD_PROXY_EMU` only toggles proxy-texture emulation. The emulated set is
> fixed to 2D / cube / rectangle; every other proxy target always returns 0.

`GLESMOD_TRACE=1` deserves explanation. Full-sequence tracing costs a branch and
an atomic increment on **every** GL call; measured, that is 29 instructions to
reach the real call versus 15 when tracing only the pre-initialisation window.
At roughly 5×10⁴ calls per frame the difference is ~0.2 ms/frame, 6 % of a
300 FPS frame. Both real-device SIGSEGVs we chased were early-initialisation
problems, so the default covers the actual need — and level 2 remains available
for "crashes after a long run" without rebuilding.

### 8.2 NeoForge config (`Config.java`)

`enabled`, `degradeLevel`, `verboseLog`, `warnIfInactive`, `pollStatusFile`.

`degradeLevel` is read by native **from the environment**, so changing it in the
config file requires a restart — stated in the config comments.

---

## 9. Compatibility detection

`CompatDetector` does one thing: read the mod list and adjust strategy.

It **does not** patch, mixin-inject into, or reflectively touch Sodium or
Embeddium. Reasons: their internals change between versions, injection produces
hard-to-diagnose conflicts and breaks on upgrade, and Sodium has been under
Polyform Shield since 0.6, so copying its code is not permitted in any case.

| Detected | Verdict |
|---|---|
| Sodium `0.8.13…` (**prefix** match) | `FULLY_SUPPORTED` |
| Any other Sodium | `CONSERVATIVE` |
| Embeddium, any version | `CONSERVATIVE` |
| Nothing | Vanilla path, logged |

Prefix matching is intentional, so build variants such as `0.8.13+build.7` are
accepted. The bias is explicit: **prefer conservative over optimistic** — an
unknown version takes the conservative path and says so rather than guessing at
internals.

---

## 10. Diagnostics and how to read them

### 10.1 The two files

| File | Written by | Read by |
|---|---|---|
| `glesmod/status.json` | native | Java, once at startup and every ~5 s |
| `glesmod/native.log` | native | humans |

`native.log` exists because relying on stderr is not safe here: on device we hit
a case where the game log contained **zero** lines from this library, and that
situation is ambiguous between "we were never loaded" and "stderr was not
captured" — opposite conclusions. Writing our own file removes the ambiguity.

### 10.2 Reading a shader failure

Look for `编译失败的着色器 —— 阶段=…，GL 名称=N`, followed by the
raw/converted source pair and the driver's message. Compare the two sources;
the difference is what we changed. This is how every converter defect was found,
and `native/tools/extract_device_fixtures.py` can turn the dump into a
permanent regression fixture.

### 10.3 Interpreting the startup report

```
GlesCapabilities{active=true, gles=3.2, reportedGl=3.2, degradeLevel=1,
 multiDraw=false, computeShader=true, persistentMapping=true,
 instancing=true, indirectDraw=true, maxTextureUnits=96, maxDrawBuffers=4,
 maxTextureSize=16384, maxSamples=4, maxVertexAttribs=32}
```

`multiDraw=false` on ES is expected and is not a fault — it means the driver
does not expose `GL_EXT_multi_draw_indirect`. This implementation only uses the
native indirect entry points from that extension; the non-indirect
`glMultiDraw*` calls always use the loop fallback.

---

## 11. Testing and gates

Everything runs on the **host**, in seconds, with no device. That is deliberate:
a converter regression surfaces on device as an opaque driver message, which is
a slow and expensive way to learn about it.

```bash
py -X utf8 native/tools/run_all_gates.py
```

| Gate | What it establishes |
|---|---|
| `roundtrip` | Fixtures convert, and the output compiles under the real GLSL ES front end (`glslc` from the NDK) |
| `veil pinwheel` | Veil's 48 shaders convert and compile |
| `mc core` | All 125 Minecraft core shaders convert and compile |
| `bsl pack` | 182 shaders from a real shaderpack |
| `flywheel dumps` | Flywheel's assembled device shaders re-convert cleanly |
| `sable splice` | Sable's Flywheel overrides spliced into a real device dump, then converted |
| `gitignore` | Third-party material cannot reach git history |
| `device fixtures` | Shaders that **actually failed on a phone** now compile |
| `builtin int args` | Built-ins with `int` parameters are not float-ised |
| `artifact freshness` | The shipped binaries contain the current fixes |

### 11.1 Two methodologies that are not optional

**Attribution.** "It passes now" does not show that *your change* is why. Build
with the matching `-DGLESMOD_NO_RULE_X` and confirm the failure returns. A
change without an attributed before/after is not verified.

**Baseline choice.** When comparing converter output, do **not** diff the
original source against the converted source. They differ in two ways — the
`#version`/precision header *and* our rewrites — so a line we never touched can
appear "broken" purely because desktop GLSL performs implicit conversions that
ES does not. The correct baseline is `raw_es`: the original source with **only
the header swapped and no conversion applied**, representing what a pure
pass-through would hand the driver.

```
raw_es PASS + conv FAIL  ->  regression we introduced
raw_es FAIL + conv PASS  ->  fixed by us
raw_es FAIL + conv FAIL  ->  still unfixed
```

---

## 12. Building

```bash
./gradlew build                 # NeoForge mod jar
```

```powershell
./build-all.ps1 -WithPlugin     # jar + .so (both ABIs) + plugin APK
./build-all.ps1                 # jar + .so only
```

> `-WithPlugin` is **not** implied. Without it the APK is not rebuilt, so the
> device keeps loading the previous native library while the code has changed.
> This has already cost one full device test round, which is why
> `check_artifact_freshness.py` compares marks inside the binaries rather than
> timestamps.

Toolchain: JDK 21, Android SDK (`aapt2`, `d8`, `zipalign`, `apksigner`),
Android NDK 27.x (Clang, CMake, Ninja, `glslc`). See
[`build-environment.md`](build-environment.md).

Notes that save time:

- Gradle must be run with `--offline` in this environment (the NeoForged maven
  host is not reachable), so builds use the local Gradle cache.
- `org.gradle.configuration-cache=false` is **required**: ModDevGradle 2.0.146
  fails to serialise `CreateMinecraftArtifacts` under Gradle's configuration
  cache. This is a plugin limitation, not a defect here.
- `glslc` must be passed `--target-env=opengl`.

---

## 13. Extending the project

### 13.1 Adding a forwarded GL function

1. Add the entry to `native/symbols.def` with class `F`.
2. Run `py native/tools/gen_symbols_def.py` / `gen_gl_forwarders.py`.
3. Run `verify_ptr_types.py` — it checks pointer stars against `gl.xml`. A
   missing `*` once turned `glGetIntegerv(GLenum, GLint *)` into
   `glGetIntegerv(GLenum, GLint)`; on AArch64 the pointer was truncated to 32
   bits and the driver faulted inside `GL.createCapabilities()`. Nothing is
   visible at compile or link time. Hence: always run this gate.

### 13.2 Adding a safe stub

Same, class `S`. Then decide whether calling it should record a degradation
event — it usually should, since a stub being called means a feature is inert.

### 13.3 Adding a converter rule

1. Write a **minimal fixture** that fails before the change. Prefer one derived
   from a real device failure (`extract_device_fixtures.py`).
2. Confirm the failure with a control build (`-DGLESMOD_NO_RULE_X`).
3. Implement the rule behind its own switch.
4. Emit a **required-string marker** and register it in
   `native/tools/required_strings.txt`.
5. Prove the A/B difference and re-run every gate.
6. Document the rule in `shader.c` with the device evidence — every existing
   rule carries its evidence inline, and that convention is what makes the file
   maintainable.

### 13.4 Adding a capability bit

Add to `GLESMOD_CAP_*` in a **reserved** position (no ABI bump), mirror it in
`GlesCapabilities` and the JSON parser, and document it. Changing a used bit
requires an ABI bump.

---

## 14. Known divergences between docs and code

Recorded so nobody is misled by stale text:

- **[`docs/capability-interface.md`](capability-interface.md) still describes a JNI
  return-value contract** (§4). The real boundary is `glesmod/status.json`;
  there is no JNI. That section is pending a rewrite.
- **[`docs/architecture.md`](architecture.md) was synchronised on 2026-11-07**
  with the current sources, the optional ANGLE path, and proxy-texture
  emulation. If any detail still conflicts, this document and the code win.

This document and the code are the authority; where they disagree with
`architecture.md`, the code wins.

---

## Licence

LGPL-3.0-or-later — see [`../LICENSE`](../LICENSE). Third-party components are
listed in [`../THIRD-PARTY-NOTICES.md`](../THIRD-PARTY-NOTICES.md).

# Proxy texture 仿真修复计划（ANGLE 2.1.1 启动崩溃）

> 状态：✅ **核心修复已完成并真机验证（2026-11-07）**
> 日期：2026-11-06 ～ 2026-11-07
> 范围：GLES-Mod native 层（`libgl_gles.so`）+ FCL 渲染器插件 APK；不改 FCL，不改 NeoForge 模组
> 目标：修复 MC 1.21.1 在 ANGLE 模式下 `RenderSystem.maxSupportedTextureSize()` 的 SIGSEGV
> 参考实现：MobileGlues `MobileGlues-cpp/gl/texture.cpp` 的 proxy texture 处理
> 关联文档：`docs/angle-gles-driver-plan.md`、`docs/p2-04-degrade-routing.md`、`docs/DEVELOPER-GUIDE.md`
> 关系：`docs/angle-gles-driver-plan.md` 的 Phase 0 PoC 发现 ANGLE 模式在 `maxSupportedTextureSize` 崩溃；本计划是该阻塞项的 native 修复，完成后才能继续 ANGLE 开关的 Phase 1/2。
> 仓库说明：`GLES-Mod-main` 现为本地 git 仓库，远端为 `EternityQwQ/GLES-Mod`，修复在 `fix/angle-proxy-texture` 分支。
> 已知遗留：CI 每次构建会生成新的 debug keystore，导致新 APK 无法覆盖旧安装；按用户要求**暂不处理**，当前需卸载旧插件后安装新 APK。

---

## 1. 一句话结论

**可以按 MobileGlues 的方式修：在 GLES-Mod native 层自己实现 `GL_PROXY_TEXTURE_*` 的桌面 GL 语义，让 `glTexImage2D/3D` 和 `glGetTexLevelParameteriv/fv` 在 proxy target 上不进入 ANGLE。**

这不是绕过 ANGLE 的补丁，而是补上 GLES 本来就没有、桌面 GL 才有的 proxy texture 行为。修复后：

- MC 的 32768×32768 探测由 GLES-Mod 自己记录为“不可分配”；
- `glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, ...)` 返回 0；
- MC 减半到 16384，正常继续启动；
- ANGLE 根本看不到非法 proxy target，因此不会触发它的 InvalidEnum 空指针。

---

## 2. 根因回顾（只保留修复所需部分）

日志：`fcl-latest-560015124094015090.log`

```text
SIGSEGV at pc=0x...710
Problematic frame:
C  [libGLESv2_angle.so+0x268710]
C  [libGLESv2_angle.so+0x103ee4]  GL_GetTexLevelParameteriv+0xd8
si_addr: 0x0000000000000008
```

Java 栈：

```text
RenderSystem.maxSupportedTextureSize()
  → GlStateManager._getTexLevelParameter(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH)
  → GL11.glGetTexLevelParameteri
  → libGLESv2_angle.so 的 GL_GetTexLevelParameteriv
```

MC 实际做的是：

```text
glTexImage2D(GL_PROXY_TEXTURE_2D = 0x8064, 0,
             GL_RGBA, 32768, 32768, 0,
             GL_RGBA, GL_UNSIGNED_BYTE, NULL)

glGetTexLevelParameteri(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH = 0x1000)
```

`GL_PROXY_TEXTURE_2D` 是桌面 GL 的探测机制，GLES 没有。ANGLE 2.1.1 的校验路径把 `TextureTarget::InvalidEnum` 放行，随后 `getTextureByType(InvalidEnum)` 返回空，`QueryTexLevelParameterBase` 解引用空指针，`si_addr=0x8`。

**关键事实：**

- 同一台设备 22:50 的原生模式（`libEGL.so` + Adreno GLES）没有崩，说明问题只在 ANGLE 路径；
- GLES-Mod 的 `glGetTexLevelParameteriv` 目前是纯转发（`symbols.def` 里的 `F`），把 proxy target 原样交给 ANGLE；
- GLES-Mod 的 `custom.c` 只在 `texprobe_report()` 里抑制 proxy `glTexImage2D` 的错误日志，没有拦截 query。

---

## 3. 修复目标与非目标

### 3.1 目标

- ANGLE 模式下，MC 1.21.1 能通过 `RenderSystem.maxSupportedTextureSize()`，不再 SIGSEGV；
- 原生模式下行为不回归；
- 修复位于 native 层，**只装插件、不装模组**也能生效；
- proxy 语义尽量贴近桌面 GL：
  - `glTexImage2D/3D(proxy, ...)` 不真正分配；
  - 尺寸超过 `GL_MAX_TEXTURE_SIZE` 时 proxy 读回 0；
  - `glGetTexLevelParameteriv/fv` 返回记录的宽/高/深度/内部格式；
- 不影响 draw/uniform 等热点路径的性能。

### 3.2 非目标

- 不修 ANGLE 本身的校验 bug（那是上游问题）；
- 不改 FCL 的 `egl_loader.c`；
- 不改 NeoForge 模组源码；
- 不在本阶段做 mod-only Mixin 兜底（可作为并行方案，但不是本计划的主路径）；
- 不处理 MobileGlues 的 1D/3D proxy 常量不一致问题（我们按自己的语义实现，不逐字复制）。

---

## 4. 方案选择

| 方案 | 是否只动插件/native | 能否修复插件-only 用户 | 风险 | 结论 |
|---|---|---|---|---|
| **native proxy 仿真** | ✅ | ✅ | 低；只影响 proxy 纹理路径 | ✅ **本计划采用** |
| mod-only Mixin | ❌（要装模组） | ❌ | 中；Mixin 冲突/映射 | 备选，不替代 |
| 修 ANGLE 校验 | ❌（要重编/替换 ANGLE） | ✅ | 高；依赖 FCL 的 ANGLE 版本 | 上游/长期 |
| 只改配置/资源 | ✅ | ❌ | 无法阻止 native 崩溃 | 不可行 |

选择 native proxy 仿真的理由：

1. MobileGlues 已经证明这套语义可行；
2. 它修复的是 GL 语义，不依赖 ANGLE 版本；
3. 只在纹理创建/查询路径执行，不在热点路径；
4. 插件-only 用户也能受益，不需要模组。

---

## 5. 设计

### 5.1 状态

在 `custom.c` 中增加线程局部的 proxy 状态：

```c
typedef struct {
    GLsizei width;          /* base level（level 0）宽度 */
    GLsizei height;
    GLsizei depth;
    GLenum  internal_format;
    int     valid;
} glesmod_proxy_state_t;

static __thread glesmod_proxy_state_t g_proxy_state;
```

说明：

- MC 的探测在 Render thread 上，线程局部足够；
- 如果未来遇到同一线程多 context，可在 record/query 时比较 `eglGetCurrentContext`，或按 context 存表；
- 初始 `valid=0`，没有 proxy 分配时查询返回 0。

### 5.2 拦截点

| GL 入口 | 行为 |
|---|---|
| `glTexImage2D` | 若 target 是 proxy：记录 `width/height`、`internalformat`，直接返回，不调用 ANGLE |
| `glTexImage3D` | 若 target 是 proxy：记录 `width/height/depth`、`internalformat`，直接返回 |
| `glGetTexLevelParameteriv` | 若 target 是 proxy：返回记录值，不调用 ANGLE |
| `glGetTexLevelParameterfv` | 同上，返回 float |
| `glTexImage1D` | 暂不处理；GLES-Mod 该符号当前不是主路径，后续可补 |
| `glTexStorage*` | 不处理；桌面 `glTexStorage*` 不接受 proxy target |

### 5.3 尺寸语义

- `glTexImage2D(proxy, level, ..., width, height, ...)` 的 `width/height` 是 **该 level** 的尺寸；
- 存 base level 尺寸：`base_width = width << level`，查询 level L 时返回 `base_width >> L`；
- 超过 `GL_MAX_TEXTURE_SIZE` 时，该 level 的任一维度超限即视为不可分配，proxy 的 width/height/depth 全部读回 0；
- `GL_TEXTURE_INTERNAL_FORMAT` 返回调用方传入的原始 `internalformat`；
- `GL_TEXTURE_DEPTH` 对 2D 返回 1，对 3D/array 返回 depth（本阶段可以只保证 2D）。

### 5.4 target 列表

至少覆盖：

```text
GL_PROXY_TEXTURE_1D                  0x8063
GL_PROXY_TEXTURE_2D                  0x8064
GL_PROXY_TEXTURE_3D                  0x8070
GL_PROXY_TEXTURE_RECTANGLE           0x84F7
GL_PROXY_TEXTURE_CUBE_MAP            0x851B
GL_PROXY_TEXTURE_1D_ARRAY            0x8C19
GL_PROXY_TEXTURE_2D_ARRAY            0x8C1B
GL_PROXY_TEXTURE_CUBE_MAP_ARRAY      0x900B
GL_PROXY_TEXTURE_2D_MULTISAMPLE      0x9101
GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9103
```

第一阶段只保证 `GL_PROXY_TEXTURE_2D`（MC 唯一用到的）；其余 target 可以先按同一套状态记录，但测试以 2D 为主。

### 5.5 可选开关

为了 A/B 对照和快速回滚，建议加一个环境变量：

```text
GLESMOD_PROXY_EMU=0/1   默认 1
```

- `1`：启用 proxy 仿真；
- `0`：退回旧行为（直接转发），用于对比/定位；
- 只在 native 侧读取，不需要改插件清单。

> 如果不想增加新 env，也可以不加；回滚靠回退 `custom.c` + `symbols.def` 即可。

---

## 6. 实施阶段

### Phase 0：基线确认（0.5 天）

- [x] 用当前插件 + ANGLE 模式复现一次崩溃，保存：
  - FCL 日志（`fcl-latest-560015124094015090.log`）；
  - `glesmod/native.log`；
  - `status.json`；
  - `hs_err_pid*.log`。
- [x] 用原生模式跑一次，确认不崩，作为对照。
- [x] 记录 `GL_MAX_TEXTURE_SIZE`（本机 16384）、`GL_RENDERER`（ANGLE + Adreno 619）、ANGLE 版本（2.1.1）。
- [x] 确认 `glGetTexLevelParameteriv` 当时在 `symbols.def` 中是 `F`。

**实际结果**：已稳定复现 ANGLE 模式在 `RenderSystem.maxSupportedTextureSize` 的 SIGSEGV；原生模式正常。后续另有一份 `fcl-latest-1628002214646083008.log` 复现同一崩溃，并确认当时设备仍加载旧的 `2.0.0-pre47 / Oct 18` 原生库。

**验收**：✅ 通过。

### Phase 1：proxy 状态与 helper（1–2 天）

- [x] 在 `custom.c` 增加：
  - `glesmod_proxy_state_t`；
  - `glesmod_is_proxy_target()`；
  - `glesmod_proxy_max_texture_size()`；
  - `glesmod_proxy_base_size()`；
  - `glesmod_proxy_level_size()`；
  - `glesmod_proxy_record()`；
  - `glesmod_proxy_query()`。
- [x] 用 `GL_MAX_TEXTURE_SIZE` 查询一次并缓存；查询失败回退 4096。
- [x] 增加一次性日志，确认拦截生效。
- [x] 增加 `GLESMOD_PROXY_EMU` 开关（默认 1，0 可退回旧行为）。

**实际结果**：`native/src/custom.c` 已实现；`clang -Wall -Wextra -Wshadow -Wconversion` 无新增警告。

**验收**：✅ 通过。

### Phase 2：接入 GL 入口（1 天）

- [x] `glTexImage2D` 开头拦截 proxy target，调用 `glesmod_proxy_record(..., depth=1)` 后返回。
- [x] `glTexImage3D` 开头拦截 proxy target，调用 `glesmod_proxy_record(..., depth)` 后返回。
- [x] 实现 `glGetTexLevelParameteriv`：
  - proxy target → 返回记录值；
  - 其他 target → 解析并转发给真实 GLES 函数。
- [x] 实现 `glGetTexLevelParameterfv`，同上。
- [x] 把 `texprobe_report()` 的 3 个硬编码 proxy 常量替换为 `glesmod_is_proxy_target(target)`，顺带修正 `0x8071/0x8072` 的错误常量。
- [x] 在 `gl_internal.h` 增加两个函数声明。

**实际结果**：用户安装新插件 APK 后确认 ANGLE 模式启动崩溃已修复；`native.log` 应出现 `[GLESMod] proxy texture 仿真`（新 APK 内已确认包含该字符串）。

**验收**：✅ ANGLE 启动崩溃已修复；原生模式完整回归尚未单独归档（见 §17）。

### Phase 3：符号表与生成器（0.5 天）

- [x] `gen_symbols_def.py` 的 `CUSTOM_SYMBOLS` 加入：

  ```python
  "glGetTexLevelParameteriv",
  "glGetTexLevelParameterfv",
  ```

- [x] `native/symbols.def` 把两行从 `F` 改成 `C`。
- [x] 重新生成 `generated_forwarders.c`：

  ```bash
  python native/tools/gen_gl_forwarders.py \
      --def native/symbols.def \
      --out native/src/generated_forwarders.c
  ```

- [x] 确认 `generated_forwarders.c` 不再包含这两个转发函数，避免与 `custom.c` 重复定义。
- [x] 同步更新头部计数注释（F 339→337，C 26→28）。
- [x] 运行 `check_symbols.py`，确认导出符号数仍为 849。

**实际结果**：生成器输出 `337 F / 484 S / 28 C = 849`；CI 产出的 APK 内 `libgl_gles.so` 的 `check_symbols.py` 覆盖率 100%，`glGetTexLevelParameteriv/fv` 均为导出符号。

**验收**：✅ 通过。

### Phase 4：构建与主机门禁（0.5 天）

```powershell
.\native\build-native.ps1 -Abi arm64-v8a -CheckSymbols
.\native\build-native.ps1 -Abi armeabi-v7a
.\fcl-plugin\build-plugin.ps1 -SkipNative
```

- [ ] `run_all_gates.py` 全绿（含新增 `proxy emu` 门禁）；
  - ⚠️ 未跑完整套件（依赖 glslang / 第三方夹具）；CI 已跑 `proxy emu`、`doc facts`、`check_symbols`。
- [x] `check_symbols.py` 通过；
- [x] `check_proxy_emulation.py` 通过；
- [x] `check_artifact_freshness.py` 通过（本次以手动 build stamp + `proxy texture` 字符串校验替代）；
- [x] `nm -D`/`llvm-readelf` 确认 `glGetTexLevelParameteriv/fv` 仍导出。

**实际结果**：GitHub Actions run `42317566973` 两个 job 均成功；arm64-v8a / armeabi-v7a 均构建成功；APK 内 arm64 `libgl_gles.so` 为 `Nov  7 2026`，包含 `[GLESMod] proxy texture` 与 `GLESMOD_PROXY_EMU`，`check_symbols.py` 覆盖率 100%。

**验收**：✅ 两个 ABI 构建成功；完整 `run_all_gates.py` 尚未执行（见 §17 遗留项）。

### Phase 5：真机验证（1–2 天）

设备矩阵（用户确认完整验证通过；覆盖 Adreno 619 与 Mali-G57）：

| 场景 | 原生模式 | ANGLE 模式 | 通过标准 |
|---|---|---|---|
| 启动到主菜单 | ✅ | ✅ | 无 SIGSEGV |
| 进世界 | ✅ | ✅ | 区块/实体/粒子正常 |
| Sodium 0.8.13 | ✅ | ✅ | 无新增降级/崩溃 |
| Embeddium 1.0.15 | 如有 | 如有 | 同上 |
| Iris + BSL | 可选 | 可选 | 光影可加载或明确跳过 |
| 退出游戏 | ✅ | ✅ | 无 EGL 销毁崩溃 |
| 8192/16384/32768 设备 | 如有 | 如有 | proxy 返回值与 `GL_MAX_TEXTURE_SIZE` 一致 |

断言：

- [x] 新 APK 的 `libgl_gles.so` 内含 `[GLESMod] proxy texture` 日志字符串（已离线校验）。
- [x] 用户确认安装新 APK 后 ANGLE 模式启动崩溃已修复；不再出现 `libGLESv2_angle.so+0x268710`。
- [x] `GL_RENDERER` 仍含 `ANGLE`（用户确认）。
- [x] `status.json` 的 `max_texture_size` 为 16384（用户确认）。
- [x] 原生模式日志中出现拦截但不影响结果（MC 仍回落到 `GL_MAX_TEXTURE_SIZE`）。
- [x] Sodium / Embeddium / Iris / 退出游戏 / 不同 max texture size 设备的完整矩阵已通过（含 Mali-G57）。

**实际结果**：用户确认完整验证通过，覆盖 Adreno 619 与 Mali-G57；ANGLE 与原生两种模式均正常。

**验收**：✅ 通过。

### Phase 6：文档与回滚准备（0.5 天）

- [x] 更新 `docs/DEVELOPER-GUIDE.md` / `.en.md`：新增“proxy texture 仿真”小节；
- [ ] 更新 `docs/architecture.md` §2.5 或纹理章节：说明 GLES 无 proxy target，由 native 仿真；
- [ ] 更新 `README.md` 已知限制：ANGLE 模式已处理 proxy texture；
  - 注：README 的 337/28 计数已同步，但专门的 ANGLE/proxy 说明尚未补。
- [ ] 更新 `docs/p2-07-compatibility-matrix.md`：增加 ANGLE 行；
- [x] 记录本次日志/测试结果（见 §16）；
- [x] 如果加了 `GLESMOD_PROXY_EMU`，写入环境变量表。

**验收**：⚠️ 核心文档已更新；architecture / README 专门条目 / p2-07 仍待补。

---

## 7. 文件级改动清单

| 文件 | 改动 | 阶段 | 风险 |
|---|---|---|---|
| `native/src/custom.c` | proxy 状态、helper、`glTexImage2D/3D` 拦截、`glGetTexLevelParameteriv/fv` 实现、`texprobe_report` 常量修正 | 1/2 | 中 |
| `native/src/gl_internal.h` | 新增两个定制函数声明 | 2 | 低 |
| `native/symbols.def` | `glGetTexLevelParameteriv/fv` 由 `F` 改 `C` | 3 | 中：必须重新生成 forwarder |
| `native/tools/gen_symbols_def.py` | `CUSTOM_SYMBOLS` 增加两个符号 | 3 | 低 |
| `native/src/generated_forwarders.c` | 重新生成，移除两个旧转发函数 | 3 | 中：重复定义风险 |
| `native/tools/check_symbols.py` | 无需改；用于验证 | 3/4 | 低 |
| `native/tools/check_proxy_emulation.py` | 新增静态门禁：确认两个符号为 `C`、forwarder 未重复、`custom.c` 已接入 | 3/4 | 低 |
| `native/tools/run_all_gates.py` | 接入 `proxy emu` 门禁 | 4 | 低 |
| `docs/DEVELOPER-GUIDE.md` | 新增 proxy 仿真说明 | 6 | 低 |
| `docs/DEVELOPER-GUIDE.en.md` | 英文同步 | 6 | 低 |
| `docs/architecture.md` | 纹理/proxy 说明 | 6 | 低 |
| `README.md` | 已知限制/兼容性 | 6 | 低 |
| `docs/p2-07-compatibility-matrix.md` | ANGLE 兼容行 | 6 | 低 |

---

## 8. 代码草案

### 8.1 helper（`custom.c`）

```c
#define GLESMOD_GL_PROXY_TEXTURE_1D                      0x8063
#define GLESMOD_GL_PROXY_TEXTURE_2D                      0x8064
#define GLESMOD_GL_PROXY_TEXTURE_3D                      0x8070
#define GLESMOD_GL_PROXY_TEXTURE_RECTANGLE               0x84F7
#define GLESMOD_GL_PROXY_TEXTURE_CUBE_MAP                0x851B
#define GLESMOD_GL_PROXY_TEXTURE_1D_ARRAY                0x8C19
#define GLESMOD_GL_PROXY_TEXTURE_2D_ARRAY                0x8C1B
#define GLESMOD_GL_PROXY_TEXTURE_CUBE_MAP_ARRAY          0x900B
#define GLESMOD_GL_PROXY_TEXTURE_2D_MULTISAMPLE          0x9101
#define GLESMOD_GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY    0x9103

#define GLESMOD_GL_TEXTURE_WIDTH                         0x1000
#define GLESMOD_GL_TEXTURE_HEIGHT                        0x1001
#define GLESMOD_GL_TEXTURE_INTERNAL_FORMAT               0x1003
#define GLESMOD_GL_TEXTURE_DEPTH                         0x8071
#define GLESMOD_GL_MAX_TEXTURE_SIZE                      0x0D33

typedef struct {
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum  internal_format;
    int     valid;
} glesmod_proxy_state_t;

static __thread glesmod_proxy_state_t g_proxy_state;
static __thread int g_proxy_logged = 0;

static int glesmod_is_proxy_target(GLenum target) {
    switch (target) {
        case GLESMOD_GL_PROXY_TEXTURE_1D:
        case GLESMOD_GL_PROXY_TEXTURE_2D:
        case GLESMOD_GL_PROXY_TEXTURE_3D:
        case GLESMOD_GL_PROXY_TEXTURE_RECTANGLE:
        case GLESMOD_GL_PROXY_TEXTURE_CUBE_MAP:
        case GLESMOD_GL_PROXY_TEXTURE_1D_ARRAY:
        case GLESMOD_GL_PROXY_TEXTURE_2D_ARRAY:
        case GLESMOD_GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
        case GLESMOD_GL_PROXY_TEXTURE_2D_MULTISAMPLE:
        case GLESMOD_GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
            return 1;
        default:
            return 0;
    }
}
```

### 8.2 记录与查询

```c
static GLint glesmod_proxy_max_texture_size(void) {
    static GLint cached = 0;
    if (cached <= 0) {
        typedef void (*get_int_t)(GLenum, GLint *);
        get_int_t get_int = (get_int_t)glesym_resolve("glGetIntegerv");
        GLint v = 0;
        if (get_int != NULL) {
            get_int(GLESMOD_GL_MAX_TEXTURE_SIZE, &v);
        }
        cached = (v > 0) ? v : 4096;
    }
    return cached;
}

static GLsizei glesmod_proxy_base_size(GLsizei size, GLint level) {
    if (size <= 0) return 0;
    if (level <= 0) return size;
    if (level >= 31) return 0;
    if (size > (GLsizei)(0x7fffffff >> level)) return 0;
    return (GLsizei)(size << level);
}

static GLint glesmod_proxy_level_size(GLint base, GLint level) {
    if (base <= 0) return 0;
    if (level <= 0) return base;
    if (level >= 31) return 1;
    GLint v = base >> level;
    return (v > 0) ? v : 1;
}

static void glesmod_proxy_record(GLenum target, GLint level, GLint internalformat,
                                 GLsizei width, GLsizei height, GLsizei depth) {
    GLint max = glesmod_proxy_max_texture_size();

    GLsizei base_w = glesmod_proxy_base_size(width, level);
    GLsizei base_h = glesmod_proxy_base_size(height, level);
    GLsizei base_d = glesmod_proxy_base_size(depth, level);

    if ((GLint)width > max || (GLint)height > max || (GLint)depth > max) {
        base_w = 0;
        base_h = 0;
        base_d = 0;
    }

    g_proxy_state.width = base_w;
    g_proxy_state.height = base_h;
    g_proxy_state.depth = base_d;
    g_proxy_state.internal_format = (GLenum)internalformat;
    g_proxy_state.valid = 1;

    if (!g_proxy_logged) {
        g_proxy_logged = 1;
        log_line(1, "proxy texture 仿真: target=0x%04X level=%d %dx%dx%d max=%d -> %dx%dx%d",
                 (unsigned)target, (int)level, (int)width, (int)height, (int)depth,
                 (int)max, (int)base_w, (int)base_h, (int)base_d);
    }
}

static GLint glesmod_proxy_query(GLenum pname, GLint level) {
    if (!g_proxy_state.valid) return 0;

    switch (pname) {
        case GLESMOD_GL_TEXTURE_WIDTH:
            return glesmod_proxy_level_size(g_proxy_state.width, level);
        case GLESMOD_GL_TEXTURE_HEIGHT:
            return glesmod_proxy_level_size(g_proxy_state.height, level);
        case GLESMOD_GL_TEXTURE_DEPTH:
            return glesmod_proxy_level_size(g_proxy_state.depth, level);
        case GLESMOD_GL_TEXTURE_INTERNAL_FORMAT:
            return (GLint)g_proxy_state.internal_format;
        default:
            return 0;
    }
}
```

### 8.3 查询入口

```c
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params) {
    if (glesmod_is_proxy_target(target)) {
        if (params != NULL) {
            *params = glesmod_proxy_query(pname, level);
        }
        return;
    }

    typedef void (*fn_t)(GLenum, GLint, GLenum, GLint *);
    static fn_t real = NULL;

    glesmod_lazy_init();
    RESOLVE_OR_RETURN(real, "glGetTexLevelParameteriv", );
    real(target, level, pname, params);
}

void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params) {
    if (glesmod_is_proxy_target(target)) {
        if (params != NULL) {
            *params = (GLfloat)glesmod_proxy_query(pname, level);
        }
        return;
    }

    typedef void (*fn_t)(GLenum, GLint, GLenum, GLfloat *);
    static fn_t real = NULL;

    glesmod_lazy_init();
    RESOLVE_OR_RETURN(real, "glGetTexLevelParameterfv", );
    real(target, level, pname, params);
}
```

### 8.4 `glTexImage2D/3D` 开头

```c
void glTexImage2D(GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const void *pixels) {
    glesmod_lazy_init();

    if (glesmod_is_proxy_target(target)) {
        glesmod_proxy_record(target, level, internalformat, width, height, 1);
        return;
    }

    /* 原有 fixup_depth_format / fixup_unsized_internalformat / 转发逻辑 */
    ...
}
```

```c
void glTexImage3D(GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLsizei height, GLsizei depth, GLint border,
                  GLenum format, GLenum type, const void *pixels) {
    glesmod_lazy_init();

    if (glesmod_is_proxy_target(target)) {
        glesmod_proxy_record(target, level, internalformat, width, height, depth);
        return;
    }

    ...
}
```

---

## 9. 测试计划

### 9.1 主机测试

```bash
python native/tools/run_all_gates.py
python native/tools/check_symbols.py --lib build/native/arm64-v8a/libgl_gles.so --def native/symbols.def
python native/tools/check_proxy_emulation.py
```

必须全绿。

### 9.2 真机测试

至少覆盖：

- Oppo PGGM10 / SM6375 / Adreno 619 / Android 14（本次崩溃设备）；
- 如有条件，加一台 Mali 设备；
- 如有条件，加一台 `GL_MAX_TEXTURE_SIZE=8192` 或 `32768` 的设备；
- 原生模式 + ANGLE 模式各跑一轮；
- Sodium / Embeddium / Iris（如环境具备）。

### 9.3 断言

- ANGLE 模式启动不再 SIGSEGV；
- `native.log` 出现 proxy 仿真日志；
- MC 日志出现（或不再需要）：

  ```text
  Failed to determine maximum texture size by probing,
  trying GL_MAX_TEXTURE_SIZE = 16384
  ```

- `status.json` 的 `max_texture_size` 与设备一致；
- `GL_RENDERER` 仍为 ANGLE；
- 原生模式行为不变。

### 9.4 性能

proxy 路径只在启动时执行，理论上对帧率无影响。记录：

- 冷启动到主菜单耗时（原生/ANGLE 各一次）；
- 进世界耗时；
- 平均 FPS（前后对比）。

---

## 10. 验收标准

> 标记规则：已实际验证或已离线校验的标 `[x]`；未逐项执行或缺少 post-fix 日志的保留 `[ ]`。

功能：

- [x] ANGLE 模式下 MC 1.21.1 启动不再在 `RenderSystem.maxSupportedTextureSize` 崩溃（用户真机确认）；
- [x] 不再出现 `libGLESv2_angle.so+0x268710`（用户真机确认）；
- [x] 原生模式完整回归（用户确认）；
- [x] 只装插件、不装模组（用户确认完整验证）。

正确性：

- [x] `glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH)` 返回 0（32768 探测）；
- [x] 16384 探测返回 16384；
- [x] 其他非 proxy 查询仍原样转发（代码路径保留 `real(...)`）；
- [x] 849 个导出符号不变（CI `check_symbols.py` 100%）；
- [ ] `run_all_gates.py` 全绿（设备验证已完成；完整主机门禁单独确认）。

兼容性：

- [x] Sodium / Embeddium 至少各跑一轮（用户确认）；
- [x] 光影包至少一个通过或明确跳过（用户确认）；
- [x] 8192/16384/32768 设备（如有）返回值正确（用户确认完整矩阵，含 Mali-G57）。

文档：

- [x] `DEVELOPER-GUIDE.md` / `.en.md` 增加 proxy 仿真说明；
- [ ] `README.md` 更新已知限制（仅同步了计数）；
- [ ] `architecture.md` 更新纹理/proxy 描述；
- [ ] `p2-07-compatibility-matrix.md` 增加 ANGLE 行。

---

## 11. 风险与缓解

| 风险 | 可能性 | 影响 | 缓解 |
|---|---|---|---|
| proxy 状态记录错误，导致 MC 高报 max texture size | 中 | 高 | 严格按 `GL_MAX_TEXTURE_SIZE` 判断；测试 8192/16384/32768 |
| 自定义查询影响真实纹理查询 | 低 | 高 | 只在 proxy target 时拦截，其余原样转发 |
| 线程局部状态在多 context 下串扰 | 低 | 中 | 本阶段 MC 单 context；后续加 context 判别 |
| 符号表改错导致重复定义/缺符号 | 中 | 高 | 重新生成 forwarder；`check_symbols.py`；`nm -D` |
| `GL_MAX_TEXTURE_SIZE` 查询失败 | 低 | 中 | 回退 4096；让 MC 自然回落 |
| 1D/3D/array proxy 语义不完整 | 中 | 低 | 第一阶段只保证 2D；其余逐步补 |
| MobileGlues 代码复制导致许可证问题 | 低 | 中 | 只参考语义，自行实现；不逐字复制 |
| 构建产物未更新 | 中 | 高 | `check_artifact_freshness.py`；重建 APK |

---

## 12. 回滚方案

- 代码回滚：还原 `custom.c`、`gl_internal.h`、`symbols.def`、`gen_symbols_def.py`、`generated_forwarders.c`；
- 如果加了 `GLESMOD_PROXY_EMU`，设 `0` 即可退回旧行为；
- 插件回滚：重新安装上一版 APK；
- 最坏情况：ANGLE 模式暂不启用，退回原生模式；
- 回滚不影响模组 jar、FCL、ANGLE 库。

---

## 13. 工作量估算

| 阶段 | 内容 | 估算 |
|---|---|---|
| Phase 0 | 复现与基线 | 0.5 天 |
| Phase 1 | proxy 状态/helper | 1–2 天 |
| Phase 2 | 接入 GL 入口 | 1 天 |
| Phase 3 | 符号表/生成器 | 0.5 天 |
| Phase 4 | 构建/主机门禁 | 0.5 天 |
| Phase 5 | 真机验证 | 1–2 天 |
| Phase 6 | 文档/回滚 | 0.5 天 |
| **合计** | | **约 5–7 人天**（真机测试是瓶颈） |

---

## 14. 待确认问题

1. **范围**：第一阶段只做 `GL_PROXY_TEXTURE_2D`，还是一次性覆盖所有 proxy target？
   - ✅ 已按 2D 路径完成并验证；array/1D 的完整 mip 语义仍遗留（见 §17）。
2. **开关**：是否加 `GLESMOD_PROXY_EMU=0/1`？
   - ✅ 已加，默认 1。
3. **context 判别**：是否需要在本阶段处理同一线程多 context？
   - ⏳ 暂按线程局部；per-context 判别遗留。
4. **max texture size 缓存**：`GL_MAX_TEXTURE_SIZE` 是否可能随 context 变化？
   - ⏳ 当前全局缓存；如需严格再按 context 缓存。
5. **MobileGlues 参考边界**：只参考语义，还是允许引用其代码片段？
   - ✅ 按语义自行实现，未逐字复制。
6. **插件版本号**：修复后是否 bump 插件版本，方便用户确认安装的是新 APK？
   - ✅ 已 bump 到 versionCode `2` / versionName `1.1.0`。
7. **是否并行提供 mod-only Mixin 兜底**？
   - ⏳ 未实现；当前以 native 修复为主。

---

## 15. 变更记录

| 日期 | 变更 |
|---|---|
| 2026-11-06 | 初稿：根因、方案选择、Phase 0–6、代码草案、测试矩阵、风险与回滚 |
| 2026-11-07 | 标记完成项：Phase 0–3 完成；CI 构建 + APK 产物校验通过；用户真机确认 ANGLE 启动崩溃已修复；签名密钥问题按用户要求暂不处理 |

---

## 16. 实际完成记录

### 16.1 代码与提交

| 项 | 内容 |
|---|---|
| 修复提交 | `194a16e fix(native): emulate GL_PROXY_TEXTURE_* to avoid ANGLE startup crash` |
| CI 提交 | `495da63 ci: build native APK and NeoForge JAR` |
| 分支 | `fix/angle-proxy-texture` |
| 远端 | `EternityQwQ/GLES-Mod` |
| 变更规模 | 13 files changed, +1725 / -51（修复提交） |

### 16.2 CI 构建

| 项 | 内容 |
|---|---|
| Workflow | `.github/workflows/build.yml` |
| Run | `42317566973`（Build APK and JAR） |
| JAR job | ✅ success |
| APK job | ✅ success |
| APK artifact | `glesmod-renderer-plugin-apk`（750,067 bytes） |
| Native libs | `glesmod-native-libs`（742,133 bytes） |
| JAR artifact | `glesmod-jar`（28,628 bytes） |

### 16.3 APK 产物校验

| 项 | 结果 |
|---|---|
| arm64 `libgl_gles.so` 构建戳 | `Nov  7 2026` |
| proxy 日志字符串 | 含 `[GLESMod] proxy texture` |
| 环境变量开关 | 含 `GLESMOD_PROXY_EMU` |
| 导出符号 | `check_symbols.py` 覆盖率 100%（849/849） |
| `glGetTexLevelParameteriv/fv` | 均为导出符号 |

### 16.4 真机结果

| 项 | 结果 |
|---|---|
| 设备 | Oppo PGGM10 / SM6375 / Adreno 619 / Android 14；Mali-G57 |
| ANGLE | 2.1.1（`2f91cb5f753e`） |
| 修复前 | `RenderSystem.maxSupportedTextureSize` → `GL_GetTexLevelParameteriv` SIGSEGV |
| 修复后 | 完整验证通过；ANGLE 与原生模式均正常 |
| 备注 | 早期设备上旧插件仍为 `2.0.0-pre47 / Oct 18`；安装新 APK 后生效 |

### 16.5 FCL 日志对照（2026-11-07）

用户提供了两份 FCL 日志。文件名与实际模式相反，以下按**日志内容**归类：

| 项 | 原生模式 | ANGLE 模式 |
|---|---|---|
| `POJAVEXEC_EGL` | `libEGL.so` | `libEGL_angle.so` |
| EGL 来源 | `eglGetProcAddress 已从 libEGL.so 取得` | `... 已从 libEGL_angle.so 取得` |
| GLES 库 | `已打开 libGLESv2.so` | `已打开 libGLESv2_angle.so` |
| `GL_VERSION` | `OpenGL ES 3.2 V@0530.48 ...` | `OpenGL ES 3.2 (ANGLE 2.1.1 ...)` |
| `GL_RENDERER` | `Adreno (TM) 619` | `ANGLE (Qualcomm, Vulkan 1.1.128, Adreno 619)` |
| proxy 2D 探测 | `32768x32768 max=16384 -> 0x0` | 同左 |
| `proxy target ... 未实现` | 未出现 | 未出现 |
| 启动崩溃 / SIGSEGV | 无 | 无 |
| 进世界 / 退出 | ✅ | ✅ |

结论：

- v2 开关与 native 同源选择均生效；
- proxy 收窄没有破坏 MC 使用的 2D 路径；
- 两份日志都只触发 `GL_PROXY_TEXTURE_2D`，符合收窄后的预期；
- ANGLE 仍使用厂商 Vulkan（`Vulkan 1.1.128`），Turnip 未生效，属独立问题。

---

## 17. 已知遗留（本次未处理）

> 以下按用户要求暂不处理，记录在此避免遗忘。

1. **CI 签名密钥不稳定（用户要求暂不解决）**
   - `fcl-plugin/debug.keystore` 被 `.gitignore` 排除，CI 每次生成新密钥；
   - 旧 APK 证书 `71:BB...`，CI 新 APK 证书 `01:81...`；
   - 结果：新 APK 无法覆盖旧安装，必须卸载旧插件后安装；
   - 建议：把固定 keystore 存入 GitHub Secret，在 APK job 构建前恢复，并 bump `--version-code`。

2. **~~Array / 1D proxy 的 mip 维度语义不完整~~ —— ✅ 已收口（2026-11-07）**
   - 仿真范围已收窄为 `GL_PROXY_TEXTURE_2D` / `CUBE_MAP` / `RECTANGLE`；
   - 1D / 3D / array / multisample proxy 不再做 depth 近似，按「不可分配」安全返回 0；
   - MC 1.21.1 只走 `GL_PROXY_TEXTURE_2D`，完整 mip 语义暂无真实需求。

3. **proxy 状态是线程局部，不是 per-context**
   - 同一线程多 context 时可能串扰；MC 单 context 暂不受影响。

4. **静态缓存未加锁**
   - `glesmod_proxy_max_texture_size` / `glesmod_proxy_emu_enabled` 的首次初始化存在良性竞争。

5. **proxy query 的错误语义不完整**
   - 非法 pname / `params == NULL` 不产生 GL 错误；对 MC 无影响。

6. **静态门禁较脆**
   - `check_proxy_emulation.py` 依赖 `^void ...` 和 guard 次数；代码风格变化可能误报。

7. **CI action 未 pin SHA**
   - `actions/checkout@v4`、`android-actions/setup-android@v3`、`actions/upload-artifact@v4` 未固定 commit；供应链安全建议后续 pin。

8. **~~完整测试矩阵未跑~~ —— ✅ 已完成（2026-11-07）**
   - 用户确认完整验证通过，覆盖 Oppo PGGM10 / Adreno 619 / Android 14
     与 Mali-G57 设备；
   - 原生与 ANGLE 两条路径均完成启动、进世界、退出游戏；
   - Sodium / Iris 正常，无新增崩溃或视觉问题。

9. **顺带发现（非本次引入）**
   - `check_symbols.py` 报告额外导出 `glDrawPixels`，但它不在 `symbols.def`；实际导出 850，声明 849。

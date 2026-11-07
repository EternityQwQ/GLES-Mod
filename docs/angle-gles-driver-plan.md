# ANGLE 作为 GLES 驱动可选后端 —— 实施计划

> 状态：✅ **核心已完成（Phase 0 阻塞项已修复；v2 ANGLE 开关已在 Adreno 619 + Mali-G57 验证；Phase 4 自动回退未实现）**
> 日期：2026-11-06 ～ 2026-11-07
> 范围：GLES-Mod 的 FCL 渲染器插件 + native 后端；FCL 侧默认不改
> 目标：在渲染器插件设置中增加「使用 ANGLE 作为 GLES 驱动」开关，默认关闭
> 关联文档：`docs/architecture.md` §2.5、`docs/DEVELOPER-GUIDE.md` §3/§8、`docs/p2-04-degrade-routing.md`、`docs/p3-01-hotpath-optimization.md`
> 关联任务：`开发任务书.txt` 的 P3-05（设备兼容性数据库与回退开关）、P3-07（FCL 插件可配置环境变量）
> 备注：本仓库远端为 `EternityQwQ/GLES-Mod`，实现位于 `fix/angle-proxy-texture` 分支。
> 路径约定：`FCL/...` 指同级的 `FoldCraftLauncher-main` 仓库；`native/...`、`fcl-plugin/...`、`src/...` 指本仓库。

---

## 1. 一句话结论

**可行。** 推荐做在 **GLES-Mod 的渲染器插件**里，而不是 NeoForge 模组里；开关只需切换 `POJAVEXEC_EGL` 一个环境变量，FCL 与 GLES-Mod 就能同时落到 ANGLE 的 EGL + GLES 上。

但必须同时满足两个前提：

1. **EGL 与 GLES 必须一起切到 ANGLE。** 只 `dlopen("libGLESv2_angle.so")`、继续用系统 `libEGL.so` 建上下文，会因每上下文状态（TLS）不共享而崩溃。
2. **ANGLE 模式只能定位为兼容/统一驱动模式，默认关闭。** 它把「桌面 GL → 原生 GLES」变成「桌面 GL → GLES → Vulkan」，热路径上多一层翻译；性能目标是原生模式，不是 ANGLE 模式。

---

## 2. 背景与目标

### 2.1 问题

Android 设备的 GLES 驱动碎片化严重：不同 SoC/ROM 的扩展支持、GLSL 编译器严格程度、有状态调用的实现细节都不一致。GLES-Mod 目前直接转发到厂商 GLES 驱动，因此继承了全部驱动差异。

### 2.2 目标

- 在 GLES-Mod 的 FCL 渲染器插件设置里提供一个用户可切换的开关：
  - **关闭（默认）**：MC 桌面 GL → `libgl_gles.so` → 厂商 GLES 驱动；
  - **开启**：MC 桌面 GL → `libgl_gles.so` → ANGLE（`libEGL_angle.so` + `libGLESv2_angle.so`）→ Vulkan 驱动。
- 开关状态持久化，下次启动生效。
- 不改变默认行为；不开启时与当前版本完全一致。
- 为后续「设备兼容性数据库 / 自动建议开启 ANGLE」留出接口。

### 2.3 非目标

- 不把 ANGLE 做成独立渲染器条目（GLES-Mod 仍是渲染器，ANGLE 只是它下面的 GLES 实现）。
- 不在 NeoForge 模组里切换驱动（时序不允许，见 §3.4）。
- 不做 FCL 全局「GLES 驱动」开关（会破坏 Zink / MobileGlues 等自带 EGL 的渲染器）。
- 不在本阶段改 shader converter 的语义；ANGLE 模式先复用现有转换器，再按测试结果调整。
- 不在本阶段做 ANGLE 初始化失败的自动回退（需要改 FCL，放到可选阶段）。

---

## 3. 现状与可行性依据

### 3.1 GLES-Mod 当前链路

```
Minecraft Java（桌面 GL 3.2+）
        │
        ▼
libgl_gles.so（849 个导出符号：转发 / 定制 / 安全 stub）
        │  glesym_resolve()
        ├─ 1) eglGetProcAddress（首选，与当前上下文同实例）
        ├─ 2) dlsym(libGLESv2.so)（兜底）
        └─ 3) dlsym(RTLD_DEFAULT)（最后兜底，过滤自身符号）
        │
        ▼
厂商 GLES 驱动（由系统 libEGL.so 建上下文）
```

关键代码：

- `native/src/core.c:586-594`：从 `POJAVEXEC_EGL` 取 EGL 库名，`dlopen` 后拿 `eglGetProcAddress`。
- `native/src/core.c:630-655`：兜底 `dlopen("libGLESv2.so")`。
- `native/src/core.c:713-760`：解析顺序 `eglGetProcAddress` → `libGLESv2` dlsym → `RTLD_DEFAULT`。
- `native/include/gles_backend.h:88`：`GLESMOD_ENV_EXEC_EGL = "POJAVEXEC_EGL"`。
- `fcl-plugin/AndroidManifest.xml:79`：`renderer=GLES Mod:libgl_gles.so:libEGL.so`。
- `fcl-plugin/AndroidManifest.xml:170`：v1 `pojavEnv` 注入 `POJAV_RENDERER=opengles3:LIBGL_ES=3:...`。

设计约束（`gles_backend.h` 文件头）：**本层不接管 EGL，复用启动器已建好的上下文。** 本方案不违背这一约束：GLES-Mod 仍然不导出/不创建 EGL，只是让启动器改为加载 ANGLE 的 EGL。

### 3.2 FCL 已经具备的基础设施

**（1）ANGLE 预编译库已经随 FCL 打包**

```
FCL/src/main/jniLibs/arm64-v8a/libEGL_angle.so
FCL/src/main/jniLibs/arm64-v8a/libGLESv2_angle.so
FCL/src/main/jniLibs/armeabi-v7a/...
FCL/src/main/jniLibs/x86/...
FCL/src/main/jniLibs/x86_64/...
```

二进制核对结果：

- `libEGL_angle.so` 是薄加载器，内部 `dlopen("libGLESv2_angle")`，并导出 `eglGetProcAddress` / `eglGetDisplay` / `eglInitialize` / `eglCreateContext` / `eglMakeCurrent` / `eglSwapBuffers`。
- `libGLESv2_angle.so` 同时包含 `libANGLE/renderer/gl` 与 `libANGLE/renderer/vulkan`，导出 `glBindTexture` / `glDrawElements` / `glGetString` 等核心函数。
- 字符串中存在 `ANGLE Requires a minimum Vulkan device version of 1.1`，说明默认 Vulkan 后端有 Vulkan 1.1 下限。

**（2）FCL 的 EGL 选择已经是环境变量驱动**

`FCL/src/main/jni/ctxbridges/egl_loader.c:44-47`：

```c
char* gles = getenv("LIBGL_GLES");
char* eglName = (strncmp(gles ? gles : "", "libGLESv2_angle.so", 18) == 0)
                ? "libEGL_angle.so"
                : getenv("POJAVEXEC_EGL");
void* dl_handle = loader_dlopen(eglName, "libEGL.so", RTLD_LOCAL|RTLD_LAZY);
```

也就是说：

- 设置 `LIBGL_GLES=libGLESv2_angle.so` 会让 FCL 强制加载 `libEGL_angle.so`；
- 或者直接设置 `POJAVEXEC_EGL=libEGL_angle.so`，FCL 也会加载 ANGLE 的 EGL；
- 两者都不设置时，按渲染器的 `rendererEGLPath` / 系统 `libEGL.so` 走。

补充两个实现细节：

- FCL 的 `appendCommonPaths()` 会把主 APK 的 `nativeLibraryDir` 追加到 `LD_LIBRARY_PATH`（`FCLauncher.java`），所以 `dlopen("libEGL_angle.so")` 能按名字找到 FCL 打包的 ANGLE，不要求插件自己带库。
- Android 链接器对已加载的 soname 会返回同一实例；FCL 先加载 `libEGL_angle.so`，GLES-Mod 再按 `POJAVEXEC_EGL` `dlopen` 同名库时，拿到的就是同一个 EGL 实例。这正是「与当前上下文同实例」保证的来源。

**（3）GLES-Mod 与 FCL 读的是同一个变量**

GLES-Mod 的 `get_egl_get_proc_address()` 从 `POJAVEXEC_EGL` 取 EGL 库名。因此，**只要把 `POJAVEXEC_EGL` 切成 `libEGL_angle.so`，FCL 的上下文创建和 GLES-Mod 的 `eglGetProcAddress` 就自动同源**。

**（4）FCL 的 v2 插件配置原生支持开关**

- `RendererConfigV2.kt:72-84`：`ToggleableEnv`（key / value / title / toggle）。
- `RendererPlugin.kt:595-632`：`buildPojavEnv()` 在开关启用时把 `KEY=VALUE` 放进 `pojavEnv`。
- `RendererPlugin.kt:623-626`：`ToggleableEnv` 仅在 `enabled` 时注入。
- `FCLauncher.java:370-375`：先写 `POJAVEXEC_EGL`，再遍历 `pojavEnv` 覆盖，顺序正好。
- `PluginManageAdapter.kt:104-107` / `PluginManagePage.kt:105-117`：v2 插件在插件管理页显示齿轮，点开就是 `RendererEnvDialog`。

### 3.3 ANGLE 的 `eglGetProcAddress` 覆盖核心函数

ANGLE 源码 `src/libGLESv2/egl_stubs_getprocaddress_autogen.cpp` 的表中包含 `glBindTexture` 等核心入口，不是只返回扩展。这意味着 GLES-Mod 的主解析路径（`eglGetProcAddress`）在 ANGLE 模式下可以正常工作。

> 注意：这是源码核对结论；最终仍需真机日志确认没有 `libGLESv2-dlsym` 兜底警告。

### 3.4 为什么模组侧不可行

1. `FCLauncher.setupGraphicAndSoundEngine()` 在 JVM 启动前就 `dlopen` 了渲染器的 GL 库（`FCLauncher.java:640-666`）。
2. `egl_loader.c` 在 `gl_init()` / `glfwInit()` 之前就选好 EGL 并创建上下文。
3. 等 NeoForge 模组加载时，`GL.createCapabilities()` 已经把 LWJGL 的函数表固定到 `libgl_gles.so`，EGL 上下文也已存在。
4. 模组目前只读取 `status.json` 并上报（`src/main/java/com/youyimc/glesmod/GLESModClient.java` 的 `reportBackendStatus()`），没有驱动选择能力。
5. 强行在运行时替换函数表属于 `开发任务书.txt` / `architecture.md` 已放弃的 B2 路线。

结论：模组最多做「状态显示 + 建议开关」，不能真正切换驱动。

---

## 4. 方案设计

### 4.1 目标链路

**原生模式（默认）**

```
MC 桌面 GL
   → libgl_gles.so
   → eglGetProcAddress（来自系统 libEGL.so）
   → 厂商 GLES 驱动
```

**ANGLE 模式（开关开启）**

```
MC 桌面 GL
   → libgl_gles.so
   → eglGetProcAddress（来自 libEGL_angle.so）
   → libGLESv2_angle.so
   → ANGLE Vulkan 后端
   → Vulkan 驱动（可选：FCL 的 Turnip / panvk）
```

### 4.2 单一事实源：`POJAVEXEC_EGL`

选择 `POJAVEXEC_EGL` 作为唯一开关键，理由：

- FCL 的 `egl_loader.c` 读它；
- GLES-Mod 的 `core.c` 读它；
- 它是 FCL 对插件渲染器已经注入的既有变量，不需要新增 FCL API；
- 开关关闭时不会覆盖 FCL 已设置的 `POJAVEXEC_EGL=libEGL.so`（FCL 会先按 `rendererEGLPath` 写入默认值），因此 native 始终能拿到一个有效的 EGL 名。

不选 `LIBGL_GLES` 的原因：它只影响 FCL 的 EGL 选择，GLES-Mod 的 native 不读它；只设它会造成「FCL 用 ANGLE EGL、GLES-Mod 用系统 EGL」的错配。若未来要兼容其他启动器，可以在 native 侧同时识别 `LIBGL_GLES`，但不作为第一阶段的开关键。

### 4.3 方案对比

| 方案 | EGL 切换 | GLES 切换 | 开关位置 | 改动量 | 结论 |
|---|---|---|---|---|---|
| **A. v2 `ToggleableEnv POJAVEXEC_EGL`** | `POJAVEXEC_EGL` | 同一个变量 | 插件管理页齿轮 | 插件 v2 + native 约 10 行 | ✅ **推荐** |
| B. v2 `ToggleableEnv LIBGL_GLES` | FCL 的 `LIBGL_GLES` 分支 | native 需新增识别 | 插件管理页齿轮 | 插件 v2 + native 约 20 行 | 可行，但不推荐作为首选 |
| C. 静态 `rendererEGLPath=libEGL_angle.so` | 固定 ANGLE | 固定 ANGLE | 无开关 | 最小 | 只能做「ANGLE 专用插件」，无开关 |
| D. NeoForge 模组切换 | ❌ | ❌ | 模组配置 | — | 时序不允许 |
| E. FCL 全局 GLES 驱动选项 | 全局 | 全局 | FCL 设置 | 大 | 会破坏 Zink/MG 等自带 EGL 的渲染器 |

### 4.4 UX 位置

- **第一阶段**：v2 插件在 **设置 → 插件 → GLES Mod → 齿轮** 中显示开关。FCL 现有代码已经支持，零 FCL 改动。
- **可选第二阶段**：如果希望开关出现在「渲染器选择」列表的齿轮里，需要让 `RendererSelectItemAdapter` 对 `renderer.path` 非空的 v2 插件也调用 `hasConfigurableEnvs(packageName)`，并让 `RendererSelectDialog` 的 `configCallback` 分支到 `getConfigurableEnvs` / `updateEnvConfigs`。约 15 行 FCL 改动，不是必须。

开关文案建议：

- 标题：`使用 ANGLE 作为 GLES 驱动（实验性）`
- 说明：`统一 GLES 前端，减少厂商驱动差异；需要 Vulkan 1.1，可能降低帧率。默认关闭。`

---

## 5. 实施阶段

### Phase 0：PoC（0 行代码，先验证 ANGLE 能跑）

**目的**：在投入 v2 迁移之前，确认 FCL 内置 ANGLE 在目标设备上能创建上下文、能进世界。

**改动**：只改 `fcl-plugin/AndroidManifest.xml` 的 `pojavEnv`，加：

```
POJAVEXEC_EGL=libEGL_angle.so
```

可选再加：

```
LIBGL_GLES=libGLESv2_angle.so
```

> 注意：只加 `LIBGL_GLES` 不加 `POJAVEXEC_EGL` 会导致 FCL 与 GLES-Mod 不同源，必须避免。

**验收**：

- 游戏能启动，`glfwInit` / EGL 初始化不崩；
- `logcat` 或 `glesmod/native.log` 出现「eglGetProcAddress 已从 libEGL_angle.so 取得」；
- `GL_RENDERER` 含 `ANGLE`（形如 `ANGLE (Qualcomm, Adreno (TM) 750, OpenGL ES 3.2)`）；
- 不出现「符号 X 来自 libGLESv2-dlsym（非 EGL）」的警告；
- 能进世界，Sodium/Embeddium 基础区块渲染正常；
- 记录启动耗时、进世界耗时、大致 FPS，与原生模式对比。

**风险**：如果 `libEGL_angle.so` **加载失败**，FCL 的 `loader_dlopen` 会回落到 `libEGL.so`，而 GLES-Mod 仍在尝试 `libEGL_angle.so`，可能出现错配；如果是 `eglInitialize` / `eglCreateContext` **初始化失败**，当前链路没有自动回退，会直接启动失败。PoC 阶段先看日志区分这两种情况；正式阶段再决定是否在 FCL 侧加回退（Phase 4）。

> ✅ **2026-11-07 更新**：Phase 0 发现的 `RenderSystem.maxSupportedTextureSize` 崩溃已由 `docs/proxy-texture-fix-plan.md` 的 native proxy texture 仿真修复，用户真机确认 ANGLE 模式可启动；修复提交为 `194a16e`，CI 构建为 `495da63`。
>
> ✅ **2026-11-07 继续**：Phase 2 的 v2 可配置开关已实现——插件清单增加 `fclPlugin_V2`，`renderer_config` 中提供 `ToggleableEnv: POJAVEXEC_EGL=libEGL_angle.so`（默认关闭）；native `get_gles_handle()` 会按 `LIBGL_GLES` / `POJAVEXEC_EGL` 选择同源 GLES 库。开关关闭/打开两种路径已在 Adreno 619 与 Mali-G57 上验证通过。Phase 4 的自动回退仍未实现。

### Phase 1：native 后端选择与可观测性（约 0.5–1 天）

**目标**：让 native 明确知道当前是原生还是 ANGLE，并修掉「eglGetProcAddress 取不到时退回系统 libGLESv2」的跨实例风险。

**改动 1：`get_gles_handle()` 兜底使用 ANGLE 的 GLES 库**

`native/src/core.c:630-655` 草案：

```c
static void *get_gles_handle(void) {
    if (g_gles_handle_tried) return g_gles_handle;
    g_gles_handle_tried = 1;

    const char *name = glesmod_env_str("LIBGL_GLES");
    if (name == NULL || name[0] == '\0') {
        const char *egl = glesmod_env_str(GLESMOD_ENV_EXEC_EGL);
        name = (egl != NULL && strstr(egl, "angle") != NULL)
             ? "libGLESv2_angle.so"
             : "libGLESv2.so";
    }

    g_gles_handle = dlopen(name, RTLD_NOW | RTLD_LOCAL);
    if (g_gles_handle == NULL) {
        log_line(0, "无法 dlopen %s: %s", name, dlerror());
        return NULL;
    }
    log_line(1, "已打开 %s (%p)", name, g_gles_handle);
    return g_gles_handle;
}
```

**改动 2：日志与状态文件记录后端**

- 在 `get_egl_get_proc_address()` 成功时，日志已经是「已从 `<egl_name>` 取得」，ANGLE 模式会自然打印 `libEGL_angle.so`。
- 可选：在 `glesmod_write_status()` 增加一行：

  ```c
  fprintf(f, "  \"gles_backend\": \"%s\",\n", g_backend_is_angle ? "angle" : "native");
  ```

  并在 Java 侧 `GlesBackendStatus` 增加可选解析字段。此项不影响 ABI，旧版 Java 忽略未知字段即可。

**改动 3：`gles_backend.h` 注释同步**

文件头目前写「EGL 由启动器按 rendererEGLPath 自行加载」。改为「由启动器按其 EGL 选择机制加载（`rendererEGLPath` 或 `POJAVEXEC_EGL`）」，避免文档与实现矛盾。

**验收**：

- 原生模式下 `g_gles_handle` 仍为 `libGLESv2.so`，行为不变；
- ANGLE 模式下 `g_gles_handle` 为 `libGLESv2_angle.so`，日志可见；
- 无 `libGLESv2-dlsym` 警告（或至少确认该警告只出现在 ANGLE 不提供的符号上）；
- 两种模式下 849 个符号的解析路径与现有测试一致。

### Phase 2：插件 v1 → v2，加入开关（约 1–2 天）

**目标**：把静态 `pojavEnv` 迁移为 v2 配置，并在插件管理页显示开关。

**2.1 保留 v1 作为旧 FCL 回退**

清单同时声明 v1 与 v2：

```xml
<!-- 旧版 FCL 回退：无 ANGLE 开关，仅原生模式 -->
<meta-data android:name="fclPlugin" android:value="true" />
<meta-data android:name="renderer"
    android:value="GLES Mod:libgl_gles.so:libEGL.so" />
<meta-data android:name="des" android:value="GLES Mod (native OpenGL ES)" />
<meta-data android:name="boatEnv" android:value="GLESMOD_BOAT_ENV_NOOP=1" />
<meta-data android:name="pojavEnv"
    android:value="POJAV_RENDERER=opengles3:LIBGL_ES=3:GLESMOD_ABI=1:GLESMOD_ENABLE=1:..." />

<!-- v2：支持可配置开关 -->
<meta-data android:name="fclPlugin_V2" android:resource="@string/renderer_config" />
```

FCL 的 `RendererPlugin.parse()` 优先 v2；旧版 FCL 不认识 `fclPlugin_V2`，会走 v1。这样不需要为了新功能放弃旧启动器。

**2.2 v2 配置 JSON 草案**

放在 `fcl-plugin/res/values/strings.xml` 的字符串资源里（FCL 用 `resources.getString(configResId)` 读取，不能用 `res/raw` 的 InputStream）：

```json
{
  "displayName": "GLES Mod",
  "rendererId": "glesmod",
  "rendererGLPath": "**|libgl_gles.so",
  "rendererEGLPath": "libEGL.so",
  "env": [
    { "type": "NormalEnv", "key": "POJAV_RENDERER", "value": "opengles3" },
    { "type": "NormalEnv", "key": "LIBGL_ES", "value": "3" },
    { "type": "NormalEnv", "key": "GLESMOD_ABI", "value": "1" },
    { "type": "NormalEnv", "key": "GLESMOD_ENABLE", "value": "1" },
    { "type": "NormalEnv", "key": "GLESMOD_DEGRADE_LEVEL", "value": "1" },
    { "type": "NormalEnv", "key": "GLESMOD_LOG_LEVEL", "value": "1" },
    { "type": "NormalEnv", "key": "GLESMOD_TRACE", "value": "1" },
    { "type": "NormalEnv", "key": "GLESMOD_SHADER_PROBE", "value": "0" },
    { "type": "NormalEnv", "key": "GLESMOD_GEOM_PROBE", "value": "0" },

    {
      "type": "ToggleableEnv",
      "key": "POJAVEXEC_EGL",
      "value": "libEGL_angle.so",
      "toggle": false,
      "title": { "key": "gles_driver_angle" }
    }
  ],
  "minMCVer": "1.21.1",
  "maxMCVer": "1.21.1"
}
```

说明：

- `rendererId` 用稳定 ID `glesmod`，不依赖 FCL 把 `rendererId` 当 `POJAV_RENDERER` 的实现细节；`POJAV_RENDERER=opengles3` 由 `NormalEnv` 显式提供。`addRendererEnvInner()` 会先写 `rendererId` 对应的值，再被 `NormalEnv` 覆盖，顺序正确。
- `rendererGLPath` 用 `**|libgl_gles.so`（v2 规范里「插件 nativeLibraryDir 下的库」占位符）。
- `rendererEGLPath` 保持 `libEGL.so`（系统 EGL）。**不要**写 `**|libEGL_angle.so`，因为插件 APK 里没有 ANGLE 库；ANGLE 在 FCL 自己的 `nativeLibraryDir` 里。
- **不要**在 `NormalEnv` 里再写 `POJAVEXEC_EGL=libEGL.so`，否则会和 `ToggleableEnv` 形成顺序依赖。默认值交给 `rendererEGLPath`。
- 标题 `gles_driver_angle` 通过 meta-data 资源 id 本地化：

  ```xml
  <meta-data android:name="gles_driver_angle"
      android:resource="@string/gles_driver_angle" />
  ```

**2.3 字符串资源**

`fcl-plugin/res/values/strings.xml`：

```xml
<string name="gles_driver_angle">使用 ANGLE 作为 GLES 驱动（实验性）</string>
```

`fcl-plugin/res/values-zh/strings.xml`（当前不存在，需新建；如采用英文默认值，则默认 values 设英文、values-zh 设中文）：

```xml
<string name="gles_driver_angle">使用 ANGLE 作为 GLES 驱动（实验性）</string>
```

> FCL 的 v2 标题只显示标题，不显示说明文字（`EnvSpec.description` 对 v2 插件为空）。如果需要说明，只能在标题里简短表达，或在文档/插件说明里写。

**2.4 构建脚本**

`fcl-plugin/build-plugin.ps1` 目前已经用 `aapt2 compile --dir res`，新增字符串资源不需要改脚本。需要确认：

- `fclPlugin_V2` 的 `android:resource` 能被 `aapt2 link` 正确解析；
- JSON 字符串里的引号在 `strings.xml` 中做 XML 转义（`&quot;`）或使用 CDATA；
- 构建后 `aapt2 dump xmltree` 确认 meta-data 的 resource id 非 0。

**2.5 验收**

- 设置 → 插件 → GLES Mod 显示齿轮；
- 点开显示「使用 ANGLE 作为 GLES 驱动（实验性）」开关；
- 默认关闭；关闭时 `POJAVEXEC_EGL=libEGL.so`，行为与当前版本一致；
- 打开后启动，日志显示 ANGLE 路径；
- 开关状态持久化（DataStore `rendererEnvPrefs[packageName]["POJAVEXEC_EGL@enabled"]`）；
- 旧版 FCL 仍能通过 v1 识别插件（无开关）。

### Phase 3：能力面与兼容性重测（约 2–3 天）

ANGLE 会改变 `GL_VERSION` / `GL_RENDERER` / 扩展集合，必须重新验证：

1. **主机门禁**：`python native/tools/run_all_gates.py` 全绿（roundtrip、veil、mc core、bsl、flywheel、sable、device fixtures、builtin int args、artifact freshness）。
2. **符号解析**：ANGLE 模式下确认 `eglGetProcAddress` 覆盖 849 个符号中的转发项；统计 `libGLESv2-dlsym` / `RTLD_DEFAULT` 兜底次数。
3. **能力探测**：记录 `status.json` 的 `gles_version` / `caps`；确认 ANGLE 暴露的 compute / SSBO / tessellation 是否改变降级决策。如果 ANGLE 暴露了现有策略未验证的能力，先按保守路径处理（`GLESMOD_DEGRADE_LEVEL=0` 或显式屏蔽）。
4. **渲染回归**：
   - 原版 1.21.1 进世界、UI、区块、实体、粒子；
   - Sodium 0.8.13 / Embeddium 1.0.15；
   - Iris + 光影包（至少 BSL）；
   - Flywheel / Veil / Sable（如测试环境具备）。
5. **性能对比**：同一设备、同一场景、同一设置，原生 vs ANGLE 的 FPS / 帧时间 / 启动时间。ANGLE 模式预期更慢；记录差距作为产品文案依据。
6. **设备矩阵**：
   - Adreno 750 / Android 16（已验证设备）；
   - 至少一台 Mali（G 系列）设备；
   - 一台只有 Vulkan 1.0 或 Vulkan 不可用的设备（预期 ANGLE 模式失败，验证回退/提示）；
   - arm64-v8a 必测；armeabi-v7a 如目标仍支持则抽测。

### Phase 4（可选）：设备探测与自动回退（约 1–2 天）

- **Vulkan 1.1 预检**：在插件/启动器侧检测 `vkEnumerateInstanceVersion` / 设备 `apiVersion`；不满足时禁用开关或提示。
- **ANGLE 初始化失败回退**：修改 FCL 的 `egl_loader.c`，在 `libEGL_angle.so` 加载/初始化失败时回退 `POJAVEXEC_EGL` / 系统 `libEGL.so`。这需要 FCL 改动，建议单独评审。
- **渲染器选择列表齿轮**：让 v2 插件在渲染器选择对话框也能配置（约 15 行 FCL 改动）。
- **自动建议**：基于设备兼容性数据库（P3-05），对已知 GLES 驱动有问题的设备提示开启 ANGLE；不自动开启。

---

## 6. 文件级改动清单

| 文件 | 改动 | 阶段 | 风险 |
|---|---|---|---|
| `fcl-plugin/AndroidManifest.xml` | PoC 加 `POJAVEXEC_EGL=libEGL_angle.so`；正式阶段加 `fclPlugin_V2` 与标题 meta-data，保留 v1 回退 | 0/2 | 中：meta-data 资源 id 解析 |
| `fcl-plugin/res/values/strings.xml` | 新增 `renderer_config` JSON 字符串、`gles_driver_angle` 标题 | 2 | 中：XML 转义 |
| `fcl-plugin/res/values-zh/strings.xml` | 中文标题（如采用英文默认） | 2 | 低 |
| `fcl-plugin/build-plugin.ps1` | 通常无需改；确认 aapt2 资源编译 | 2 | 低 |
| `native/src/core.c` | `get_gles_handle()` 识别 ANGLE；可选 status 字段 | 1 | 低 |
| `native/include/gles_backend.h` | 注释同步；可选新增 env 常量 | 1 | 低 |
| `native/tools/` | 可选新增 ANGLE 符号解析审计脚本 | 3 | 低 |
| `docs/DEVELOPER-GUIDE.md` | 新增「ANGLE 模式」章节，更新「不接管 EGL」表述 | 3 | 低 |
| `docs/DEVELOPER-GUIDE.en.md` | 英文同步 | 3 | 低 |
| `README.md` | 功能列表/已知限制补充 ANGLE 兼容模式 | 3 | 低 |
| `docs/architecture.md` | §2.5 补充 ANGLE 可选路径 | 3 | 低 |
| `docs/p2-07-compatibility-matrix.md` | 增加 ANGLE 模式兼容矩阵 | 3 | 低 |
| `THIRD-PARTY-NOTICES.md` | 如打包 ANGLE，补 BSD-3-Clause 声明 | 4 | 低 |

FCL 侧（可选，非本阶段必须）：

| 文件 | 改动 | 阶段 |
|---|---|---|
| `FCL/src/main/java/com/mio/ui/adapter/RendererSelectItemAdapter.kt` | v2 插件也显示齿轮 | 4 |
| `FCL/src/main/java/com/mio/ui/dialog/RendererSelectDialog.kt` | `configCallback` 分支 v2 插件 | 4 |
| `FCL/src/main/jni/ctxbridges/egl_loader.c` | ANGLE 加载失败回退 | 4 |

---

## 7. 测试计划

### 7.1 主机测试

```bash
python native/tools/run_all_gates.py
```

必须全绿。若新增 ANGLE 专用规则/探针，再补：

- `native/tools/check_symbols.py`（导出符号仍为 849）；
- `native/tools/check_artifact_freshness.py`（APK 内的 `.so` 与源码一致）；
- 新增 `native/tools/audit_angle_resolution.py`（可选）：从真机日志统计 `via eglGetProcAddress` / `libGLESv2-dlsym` / `RTLD_DEFAULT` 的比例。

### 7.2 真机测试矩阵

| 场景 | 原生模式 | ANGLE 模式 | 通过标准 |
|---|---|---|---|
| 启动到主菜单 | ✅ | ✅ | 无崩溃、无 ANR |
| 进世界（原版） | ✅ | ✅ | 区块/实体/粒子正常 |
| Sodium 0.8.13 | ✅ | ✅ | 无渲染错误；降级日志符合预期 |
| Embeddium 1.0.15 | ✅ | ✅ | 无渲染错误 |
| Iris + BSL | ✅ | ⚠️ 待测 | 光影可加载或明确跳过 |
| 退出游戏 | ✅ | ✅ | 无 EGL 销毁崩溃 |
| Vulkan 1.0 设备 | ✅ | ❌ 预期失败 | 有明确提示/回退，不静默崩溃 |
| armeabi-v7a | ✅ | ⚠️ 抽测 | 至少能启动 |

### 7.3 断言日志

ANGLE 模式启动后检查：

- `glesmod/native.log` 含 `eglGetProcAddress 已从 libEGL_angle.so 取得`；
- 不含 `警告: eglGetProcAddress 取自 RTLD_DEFAULT`；
- 不含（或仅少量）`注意: 符号 X 来自 libGLESv2-dlsym（非 EGL）`；
- `status.json` 的 `gles_version` 为 `3.2`，`reported_gl_version` 符合预期；
- `GL_RENDERER` 含 `ANGLE`。

### 7.4 性能记录

每个模式至少记录：

- 冷启动到主菜单耗时；
- 进世界耗时；
- 静止/移动/战斗三个场景的平均 FPS 与 1% low；
- 峰值内存 / 显存（如可测）。

建议用同一存档、同一视距、同一光影设置；ANGLE 模式不承诺性能，只记录差距。

---

## 8. 风险与缓解

| 风险 | 可能性 | 影响 | 缓解 |
|---|---|---|---|
| EGL/GLES 不同源导致有状态调用崩溃 | 中 | 高 | 只切 `POJAVEXEC_EGL`，确保 FCL 与 native 同源；PoC 日志确认 |
| ANGLE 需要 Vulkan 1.1，目标设备不满足 | 中 | 高 | 默认关闭；Phase 4 预检；文档标注设备要求 |
| ANGLE 模式性能明显下降 | 高 | 中 | 定位为兼容模式；默认关闭；文案说明；保留原生模式 |
| ANGLE 能力面变化导致降级策略误判 | 中 | 中 | Phase 3 重测；必要时对 ANGLE 使用保守 profile |
| ANGLE 着色器编译器与厂商 GLSL 编译器差异 | 中 | 中 | 复用现有转换器；跑 MC/BSL 门禁；记录失败样本 |
| FCL 的 ANGLE 库版本随 FCL 升级变化 | 中 | 中 | 依赖 `eglGetProcAddress` 而非固定符号；记录 FCL 版本；必要时插件自带 ANGLE |
| SDL / 未来 MC 版本不走 `POJAVEXEC_EGL` | 低 | 中 | 1.21.1 不受影响；未来版本单独评估（可能需要插件目录内 ANGLE 或 FCL 改动） |
| `${egl_lib_name}` 仍指向插件目录的 `libEGL.so` | 低 | 低 | 1.21.1 不走 LWJGL EGL；如需要，单独修 FCL |
| 打包 ANGLE 的许可证/体积 | 低 | 低 | 优先复用 FCL 已分发库；如打包，补 `THIRD-PARTY-NOTICES` |
| 旧版 FCL 不认识 v2 | 中 | 低 | 保留 v1 meta-data 回退；旧版无开关但可用 |
| 开关状态与 FCL 版本升级不兼容 | 低 | 低 | 使用稳定的 env key；文档记录 DataStore 键 |

---

## 9. 验收标准

功能：

- [ ] 插件管理页出现「使用 ANGLE 作为 GLES 驱动（实验性）」开关；
- [ ] 默认关闭，关闭时行为与当前版本一致；
- [ ] 打开后 `POJAVEXEC_EGL=libEGL_angle.so`，FCL 与 GLES-Mod 同源；
- [ ] 开关持久化，重启生效；
- [ ] 旧版 FCL 仍能识别插件（v1 回退）。

正确性：

- [ ] ANGLE 模式能启动、进世界、退出，无崩溃；
- [ ] `GL_RENDERER` 含 `ANGLE`；
- [ ] 无 `RTLD_DEFAULT` 警告；`libGLESv2-dlsym` 兜底为 0 或仅限已知非核心符号；
- [ ] 849 个导出符号不变；
- [ ] `run_all_gates.py` 全绿。

兼容性：

- [ ] 原版 / Sodium / Embeddium 至少各跑一轮；
- [ ] 光影包至少一个通过或明确跳过；
- [ ] Vulkan 1.0 / 无 Vulkan 设备有明确失败提示，不静默崩溃。

文档：

- [ ] `DEVELOPER-GUIDE.md` / `.en.md` 增加 ANGLE 模式章节；
- [ ] `README.md` 更新「不接管 EGL」「已知限制」表述；
- [ ] `architecture.md` §2.5 补充可选 ANGLE 路径；
- [ ] `p2-07-compatibility-matrix.md` 增加 ANGLE 列。

---

## 10. 回滚方案

- 开关默认关闭，最坏情况用户不开启即可回到当前行为。
- 如果 v2 配置导致插件无法识别：保留 v1 meta-data，FCL 会走 v1；或直接回退 `fcl-plugin` 到当前版本。
- 如果 native 兜底改动引入回归：`get_gles_handle()` 的改动是局部函数，回退该函数即可；不涉及 ABI/符号表。
- 如果 ANGLE 库在某些设备上导致启动失败：Phase 4 之前不做自动回退，先通过文档/开关提示；必要时在 FCL 侧加回退（单独 PR）。

---

## 11. 工作量估算

| 阶段 | 内容 | 估算 |
|---|---|---|
| Phase 0 | PoC（manifest 改一行 + 真机验证） | 0.5 天 |
| Phase 1 | native 兜底 + 日志/status | 0.5–1 天 |
| Phase 2 | 插件 v2 迁移 + 开关 + 构建验证 | 1–2 天 |
| Phase 3 | 能力/兼容/性能重测 + 文档 | 2–3 天 |
| Phase 4 | 可选：Vulkan 预检 / 自动回退 / FCL UI | 1–2 天 |
| **合计** | | **约 5–8 人天**（真机测试是主要瓶颈） |

---

## 12. 待确认问题

1. **目标 FCL 最低版本**：v2 支持从哪个 FCL 版本开始？是否需要兼容更老的 v1-only 版本？（当前方案保留 v1 回退。）
2. **是否允许依赖 FCL 内置 ANGLE**：如果目标用户的 FCL 版本较老、没有 `libEGL_angle.so`，是否需要在插件 APK 内自带 ANGLE？自带会增加约 6 MB 和许可证义务。
3. **`POJAVEXEC_EGL` 的稳定性**：它目前是 FCL 对插件渲染器注入的既有变量，但没有正式文档承诺；是否需要同时在 native 侧支持 `LIBGL_GLES` 作为备用键？
4. **目标设备范围**：只保证 Adreno 750，还是需要覆盖 Mali / Vulkan 1.0 / 32 位设备？这决定 Phase 3 的测试矩阵和 Phase 4 的预检策略。
5. **是否要求 ANGLE 模式支持光影包**：如果只要求原版/Sodium 可用，测试范围可以缩小；如果要求 BSL/Iris，需要更多着色器回归。
6. **开关位置**：插件管理页即可，还是必须出现在渲染器选择列表？后者需要 FCL 侧小改。
7. **是否纳入 Turnip 组合测试**：ANGLE + Turnip 在 Adreno 上可能形成更统一的栈，但 Turnip 的兼容范围有限，是否作为本阶段目标？
8. **ANGLE 失败时的产品行为**：静默回退、明确报错、还是禁用开关？这决定是否需要 FCL 侧改动。
9. **许可证与分发**：如果插件 APK 自带 ANGLE，是否接受 BSD-3-Clause 声明与体积增加？
10. **任务编号**：本文档是否登记为 P3-08，还是并入 P3-05/P3-07？

---

## 13. 决策记录（待评审）

| 编号 | 决策 | 状态 |
|---|---|---|
| D1 | 做在渲染器插件，不做在 NeoForge 模组 | ✅ 已确认 |
| D2 | 开关只切 `POJAVEXEC_EGL`，默认 `libEGL.so`，开启时 `libEGL_angle.so` | ✅ 推荐 |
| D3 | 默认关闭，定位为兼容/统一驱动模式 | ✅ 推荐 |
| D4 | 插件 v2 + 保留 v1 回退 | ✅ 推荐 |
| D5 | 优先复用 FCL 内置 ANGLE，不在插件内自带 | ⏳ 待确认 |
| D6 | Phase 1–3 不改 FCL；Phase 4 可选改 | ✅ 推荐 |
| D7 | ANGLE 模式暂不承诺性能，只承诺兼容性 | ✅ 推荐 |

---

## 14. 附录：证据索引

| 结论 | 证据 |
|---|---|
| FCL 内置 ANGLE 库（4 ABI） | `FCL/src/main/jniLibs/*/libEGL_angle.so`、`libGLESv2_angle.so` |
| FCL 按 `LIBGL_GLES` 切 ANGLE EGL | `FCL/src/main/jni/ctxbridges/egl_loader.c:44-47` |
| FCL 按 `POJAVEXEC_EGL` 选 EGL | 同上 |
| GLES-Mod 从 `POJAVEXEC_EGL` 取 `eglGetProcAddress` | `native/src/core.c:586-594` |
| GLES-Mod 兜底 `libGLESv2.so` | `native/src/core.c:630-655` |
| 符号解析优先级 | `native/src/core.c:713-760` |
| v2 支持 `ToggleableEnv` | `FCL/src/main/java/com/mio/plugin/RendererConfigV2.kt:72-84` |
| v2 开关注入 env | `FCL/src/main/java/com/mio/plugin/RendererPlugin.kt:595-632` |
| `pojavEnv` 覆盖 `POJAVEXEC_EGL` | `FCL/src/main/java/com/tungsten/fclauncher/FCLauncher.java:370-375` |
| v2 插件齿轮入口 | `FCL/src/main/java/com/mio/ui/adapter/PluginManageAdapter.kt:104-107` |
| 渲染器选择列表齿轮只对内置 | `FCL/src/main/java/com/mio/ui/adapter/RendererSelectItemAdapter.kt` |
| 模组只读 status | `src/main/java/com/youyimc/glesmod/GLESModClient.java` 的 `reportBackendStatus()` |
| 模组无法选驱动 | `FCL/src/main/java/com/tungsten/fclauncher/FCLauncher.java:640-666`（JVM 前 dlopen GL） |
| ANGLE `eglGetProcAddress` 含核心函数 | ANGLE 源码 `src/libGLESv2/egl_stubs_getprocaddress_autogen.cpp` |
| ANGLE 需要 Vulkan 1.1 | `libGLESv2_angle.so` 字符串 |
| FCL 已放弃 B1/B2 驱动注入 | `docs/architecture.md` §2.4、`开发任务书.txt` |

---

## 15. 变更记录

| 日期 | 变更 |
|---|---|
| 2026-11-06 | 初稿：可行性结论、方案对比、Phase 0–4、测试计划、风险与待确认项 |

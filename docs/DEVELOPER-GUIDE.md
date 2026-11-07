# GLES Mod — 开发者指南

> [English](DEVELOPER-GUIDE.en.md) · **中文**
>
> 面向：需要阅读、构建、修改或对接本项目的开发者。
> 以下所有接口均取自**实际源码**，不是设计意图。凡本文与既有文档冲突之处，
> 本文以代码为准，并会在冲突处明确指出。

---

## 目录

1. [准确地说，这个项目是什么](#1-准确地说这个项目是什么)
2. [架构](#2-架构)
3. [两种交付形态](#3-两种交付形态)
4. [native 层](#4-native-层)
5. [GLSL → GLSL ES 转换器](#5-glsl--glsl-es-转换器)
6. [Java 层](#6-java-层)
7. [Java ↔ native 契约](#7-java--native-契约)
8. [配置](#8-配置)
9. [兼容性检测](#9-兼容性检测)
10. [诊断信息与阅读方法](#10-诊断信息与阅读方法)
11. [测试与门禁](#11-测试与门禁)
12. [构建](#12-构建)
13. [如何扩展本项目](#13-如何扩展本项目)
14. [已知的文档与代码不一致之处](#14-已知的文档与代码不一致之处)

---

## 1. 准确地说，这个项目是什么

它**是一个替换用的 GL 库**，不是一个去 patch GL 调用的模组。

FCL 之类的启动器按路径加载 GL 实现并交给 LWJGL。通常那个路径指向兼容层
（GL4ES、ANGLE、Zink），由它把桌面 GL 调用翻译成 GLES 调用。
本项目**在那个路径上放自己的库**，于是：

- **转发符号**直接抵达设备真实的 GLES 驱动，只有一层跳板，没有逐次翻译。
- **定制实现**处理桌面 GL 与 GLES 真正不同的地方（枚举值、纹理格式、
  着色器源码、缺失函数）。
- **着色器兼容**通过在源码进入驱动之前改写 GLSL 来解决。

最后一点带来一个理解本项目最关键的事实：

> 桌面 GLSL 会做 `int` → `float` 的**隐式转换**，GLSL ES **不会**。
> 因此一个在桌面能编译的着色器，在 ES 上可能编译失败。
> `native/src/shader.c` 的绝大部分代码，就是为了填上这个鸿沟。

---

## 2. 架构

```
                 ┌──────────────────────────────────────────────┐
 Minecraft ──────┤ LWJGL（通过被加载的 GL 库调用 glXxx）        │
                 └───────────────────┬──────────────────────────┘
                                     │
                 ┌───────────────────▼──────────────────────────┐
                 │ libgl_gles.so    （native 核心，只有一份）    │
                 │                                              │
                 │  generated_forwarders.c  337 符号 ───────────┼──► 真实 GLES
                 │  custom.c                 28 符号 ────────────┤    驱动
                 │  enum.c      枚举/格式转换                    │
                 │  shader.c    GLSL → GLSL ES 转换器 ───────────┤
                 │  core.c      能力探测 / 降级 / 状态输出        │
                 │  probe.c     几何诊断探针                     │
                 └───────────────────┬──────────────────────────┘
                                     │ 只写文件，从不回调 Java
                           glesmod/status.json
                           glesmod/native.log
                                     │
                 ┌───────────────────▼──────────────────────────┐
                 │ NeoForge 模组（Java 21）── 读取 ──► 写日志     │
                 │  capability / compat / degrade / backend      │
                 └──────────────────────────────────────────────┘
```

两个刻意保留的不对称性：

**Java 从不调用 native，native 也从不高调用 Java。**
native 库由启动器 `dlopen` 加载，与模组的 JVM 处于**不同的类加载上下文**。
回调需要跨两个加载器 attach 线程，复杂且脆弱，且没有收益。
因此 native 写两个文件，Java 定期读取。

**Java 不在渲染路径上。**
它只做能力上报、降级记录、优化模组检测。每帧没有任何 Java 开销。

---

## 3. 两种交付形态

一份 native 核心 + 两层薄包装 = 三个构建产物：

| 产物 | 内容 | 作用 |
|---|---|---|
| `glesmod-1.0.0.jar`（约 30 KB） | 仅 Java | 能力上报、兼容检测、配置、诊断 |
| `glesmod-renderer-plugin.apk`（约 2.3 MB） | 清单 + `lib/*/libgl_gles.so` | **真正的渲染器**。FCL 从这里加载 `.so` |
| `libgl_gles.so`（每 ABI 一份） | native 核心 | 也可单独取出供其它启动器使用 |

> **原生库不在 jar 里。** 只装 jar 不会有任何可观察的变化。
> 这一点极容易踩坑，所以 README 里也专门写了。

### 3.1 为什么插件形态是 APK

FCL 通过扫描已安装包的 `meta-data` 来发现渲染器插件（FCL 的
`PluginManager.kt` / `RendererPlugin.kt`）。APK 只是承载这些元数据与 `.so`
的**投递载体**。渲染时没有任何代码在 APK 内运行 —— 插件的 Activity 从不启动。

清单以 **v1 字段为基线**，同时声明 **v2**（`fclPlugin_V2`）。旧版 FCL 只认识 v1；
新版 FCL 优先解析 v2，从而获得「使用 ANGLE 作为 GLES 驱动」的开关。若干 v1 字段
之所以必需，只是因为 FCL 的解析器在字段缺失时会**静默 return**：

| 字段 | 存在理由 |
|---|---|
| `fclPlugin=true` | 发现阶段标记该包为插件 |
| `fclPlugin_V2=@string/renderer_config` | v2 配置入口；值是 `RendererConfigV2` JSON 的资源 ID |
| `renderer=GLES Mod:libgl_gles.so:libEGL.so` | 格式 `名称:GL:EGL`。我们提供 GL，**EGL 默认复用系统** |
| `des=…` | 显示名。必须是**字面量**而非 `@string/…` —— 资源引用会被 `PackageManager` 存成资源 ID，而 FCL 调用 `Bundle.getString()`，于是得到 null |
| `boatEnv=…` | **必需但本项目不用。** FCL 的 `parseV1()` 执行 `getString("boatEnv") ?: return`，缺这一项会静默禁用整个插件 |
| `pojavEnv=…` | 环境变量，格式 `KEY=VALUE:…` |
| `minMCVer` / `maxMCVer` | 避免 FCL 在不兼容版本上提供本渲染器 |
| `extractNativeLibs="true"` | FCL 用 `ApplicationInfo.nativeLibraryDir` 拼 `.so` 路径；不提取则该目录下没有这个文件 |

两个环境变量属于**不可妥协**，都由崩溃换来的：

- **`POJAV_RENDERER=opengles3`** —— FCL 的 `egl_bridge.c` 里
  `strncmp("opengles", getenv("POJAV_RENDERER"), 8)` **没有判空**。
  FCL 只为 v2 插件自动设置它；v1 插件不设，`getenv` 返回 NULL，
  进程在 `glfwInit()` 阶段 SIGSEGV。已真机验证。
- **`LIBGL_ES=3`** —— 决定 EGL 上下文版本。FCL 只为内置渲染器自动设置，
  插件必须自己设，否则拿到 ES 2 上下文。

### 3.2 ANGLE 开关链路

v2 配置里只有一个与后端有关的 `ToggleableEnv`：

```json
{
  "type": "ToggleableEnv",
  "key": "POJAVEXEC_EGL",
  "value": "libEGL_angle.so",
  "toggle": false,
  "title": { "key": "gles_driver_angle" }
}
```

链路如下：

1. 开关默认 `false`，插件不设置 `POJAVEXEC_EGL`，FCL 使用 `rendererEGLPath`
   （`libEGL.so`）——**原生路径**。
2. 用户打开开关后，FCL 注入 `POJAVEXEC_EGL=libEGL_angle.so`，`dlsym_EGL()`
   加载 ANGLE 的 EGL。
3. native 的 `get_gles_handle()` 按以下顺序选择兜底 GLES 库：
   ```text
   LIBGL_GLES（若显式设置）
     > POJAVEXEC_EGL 含 "angle" -> libGLESv2_angle.so
     > libGLESv2.so
   ```
   因此插件**绝不设置 `LIBGL_GLES`**；否则用户手动配置可能与开关冲突，
   造成 EGL 与 GLES 不同源。
4. 最终调用链变为 `libgl_gles.so → libGLESv2_angle.so → Vulkan`。

> ⚠️ ANGLE 2.1.1 在 `glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, ...)` 上会空指针
> 崩溃；native 的 proxy texture 仿真（§4.2、§8.1）是该路径能启动的前提。
> 开关默认关闭，ANGLE 初始化失败时也**没有自动回退**——关闭开关即可恢复原生路径。

---

## 4. native 层

### 4.1 符号清单 —— 849 个导出符号

| 类别 | 数量 | 来源 | 行为 |
|---|---|---|---|
| `F` 转发 | 337 | `generated_forwarders.c`（生成） | 首次调用解析真实 GLES 符号并缓存，之后直接跳转 |
| `C` 定制 | 30 | `custom.c` | 手写，因为行为必须不同 |
| `S` 安全桩 | 482 | `generated_forwarders.c`（生成） | 空实现，返回中性值并记录一次降级事件 |

`native/symbols.def` 是「哪个符号属于哪一类」的**唯一真源**。
`native/tools/gen_symbols_def.py` 与 `gen_gl_forwarders.py` 依它生成代码，
不要手改生成产物。

**482 个桩是特性，不是缺漏。** 另一种做法是让符号缺失，而 LWJGL 解析不到符号
会直接中断启动。用桩，游戏能跑起来，并且能准确告诉你哪些功能是静默失效的：

```
glGetIntegerv(GL_NUM_EXTENSIONS)  ->  真实数量 + 1
```

那多出来的一个扩展是 `GL_ARB_draw_buffers_blend`，因为有些代码路径会先检测
它再决定是否使用多目标混合。

### 4.2 30 个定制实现

它们存在的场合是「单纯转发会得到错误结果」。两个能说明模式的例子：

**`glGetString(GL_VERSION)`** 返回
`"3.2 (OpenGL ES 3.2 <渲染器名>)"`。Minecraft 与许多模组会解析这个字符串并
按**桌面 GL 语义**分支，所以它必须看起来像桌面 GL 版本，同时又要诚实反映底层
的 ES 版本。

**`glShaderSource`** 是整个转换器的接入点。它还会**无条件记录每一个着色器的
源码**，以便后续编译失败时可诊断 —— 见 §10。

**`glTexImage2D/3D` + `glGetTexLevelParameteriv/fv`** 负责 proxy texture 仿真。
桌面 GL 的 `GL_PROXY_TEXTURE_*` 在 GLES 中不存在，而 Minecraft 用它探测最大
纹理尺寸。命中 proxy target 时直接读写本地 `g_proxy_state`，**不进入驱动**，
从而避免 ANGLE 2.1.1 对 `InvalidEnum` 的空指针崩溃。

仿真**只完整覆盖** `GL_PROXY_TEXTURE_2D`、`GL_PROXY_TEXTURE_CUBE_MAP`、
`GL_PROXY_TEXTURE_RECTANGLE`；其余 proxy target（1D / 3D / array / multisample）
按「不可分配」返回 0，并记录一次日志。这样不会用未经证明的 mip/depth 近似
去污染其他 target。详见 `docs/proxy-texture-fix-plan.md` 与
`docs/proxy-texture-hardening-plan.md`。

### 4.3 `RESOLVE_OR_RETURN` 模式

每一个被守护的入口都长这样：

```c
static fn_t real = NULL;                       /* 首次调用后缓存 */
glesmod_lazy_init();
RESOLVE_OR_RETURN(real, "glBindTexture", );
real(a, b);
```

三个要点：

- 解析只发生**一次**，不是每次调用。
- 符号缺失时记录降级事件并返回中性值，而不是崩溃。
- 稳态只有一次 load 加一次分支，arm64 实测 19.4 条指令
  （`native/tools/hotpath_cost.py`），优化前是 29 条。

### 4.4 降级模型

本项目不允许任何环节导致游戏崩溃。当某个能力不可用时：

1. 记录**原因码**（`native/include/gles_backend.h` 的
   `glesmod_degrade_code`），**累加计数**而不是每次都打日志。
2. 有替代方案就用替代方案；没有就让该调用变成惰性的。
3. 状态写入 `status.json`，由 Java 层读取。

原因码是**冻结契约**：可以废弃，**不可复用**，因为第三方可能据此分支判断。
native 枚举与 Java 的 `DegradeReason` 必须保持同步；Java 侧遇到未知码值会
回退为 `UNSUPPORTED_FUNCTION` 并打一条警告，而不是抛异常。

---

## 5. GLSL → GLSL ES 转换器

这是风险最高的部件，也是投入工程努力最多的地方。
全部位于 `native/src/shader.c`（约 10 000 行）。

### 5.1 问题所在

桌面 GLSL 与 GLSL ES 的差异会改变**程序能否编译**，而不只是性能：

| 桌面 GLSL | GLSL ES |
|---|---|
| 隐式 `int` → `float` | **非法**。`float x = 1;` 报错 |
| `vec2 v = ivec2Var;` | **非法**。整数→浮点向量无隐式转换 |
| `vec2 v = ivec2(1,2);` | 非法 |
| `texture2D(s, uv)` | `texture(s, uv)` |
| `gl_FragColor` | 由着色器自己声明的 `out vec4` |
| `#version 330 core` | `#version 320 es` |
| `#error "message"` | `#error message`（ES 没有字符串字面量；Mali 连非活动分支也会扫描引号） |
| 采样器类型有默认精度 | **每种采样器类型都要 `precision`** |

因此转换器必须做**真正的类型分析**，却又只是一个线性扫描器 ——
它没有完整语法分析器，也没有符号表，一切都要自己构建。

### 5.2 流水线顺序

`glesmod_convert_shader_source()` 按此顺序执行：

1. **提前返回**：源码已声明 `#version … es` 时直接放行。
2. `expand_moj_imports` —— 展开 `#moj_import <file>`。
   （真机上 Minecraft 在源码到达我们之前就已经展开，此项作为防御保留。）
3. `extract_version` 与 `normalize_preprocessor_directives` —— 去掉仅桌面存在的扩展，
   并把 `#error` 的字符串消息规范化为 GLSL ES 合法 token
   （GLSL ES 没有字符串字面量；Mali 的非活动分支也会扫描引号）。
4. `expand_int_const_macros` —— **先**展开整数常量宏，
   这样同一个宏在整数与浮点上下文中的两次使用能分别判断。
5. **限定符转换**与采样函数改名。
6. `normalize_int_literals` —— **核心**。在确实属于浮点的上下文里把整数字面量
   改写为浮点。内部是一组规则（A–M 及后续增补），按固定优先级顺序求值。
7. `fix_int_vector_float_ops` —— 处理整数/浮点向量混算（情形 A–I）。
8. `fix_mixed_int_uint` —— `int`/`uint` 混用。
9. `fix_return_int_in_float_fn` —— 从声明为 `float` 的函数里 `return` 整数表达式。
10. `rewrite_nonconst_globals` + `inject_global_assignments` ——
    GLSL ES 要求初始化器必须是常量表达式，非恒定的全局初始化器会被
    提升进一个注入的赋值函数。
11. **组装** —— 前置 `#version 320 es`、精度声明、采样器精度，并映射
    `gl_FragColor`。

> 第 3 步的 `#error` 规范化可用 `-DGLESMOD_NO_ERROR_DIRECTIVE_FIX` 关闭，
> 用于 A/B 归因；对应标记字符串为 `GLESMOD_ERROR_DIRECTIVE_NORMALIZED`。

### 5.3 核心难点：上下文不是局部信息

朴素规则「语句里出现 `vec4` 就把其中的整数浮点化」是错的，
而且**后加的每一条规则，都是对某一类具体误判的修正**。以下均为真机实证：

| 构造 | 朴素规则为何出错 | 处理方式 |
|---|---|---|
| `textureSize(tex, 0)` | `lod` **必须是 `int`**。但写成 `vec2(textureSize(tex,0))` 后，外层的浮点构造函数让这个字面量看起来是浮点 | `INT_ARG_BUILTINS` 按函数记录整型实参位置，命中则保留整数 |
| `textureLod(tex, uv, 0)` | 这里 `lod` **必须是 `float`**，**必须**补 `.0` | **刻意不收录**进上表 —— 语法相同，要求相反 |
| `int i = 0;` 出现在同时提到 `vec3` 的语句里 | 声明处是整数，使用处看起来是浮点 | 声明处守卫；同时「在整数表与浮点表中都出现」的名字会从浮点表剔除（见 §5.4） |
| `f(1)`，而用户函数是 `void f(int)` | 我们只知道内建函数签名，不知道用户函数 | 从「带函数体的定义」收集用户函数签名表；只有形参类型已知时才转换字面量 |
| `i.pose`，而 `i` 是结构体 | `i` 被另一个作用域的 `int i` 污染，被当作整数变量收录 | **标量不可能有成员**。表格命中后紧跟 `.` 说明命中错了，放弃改写 |
| `cross(i, …)`，而 `i` 来自 `vec3 i = q.xyz` | `cross` 要求浮点，所以没等浮点证据就包了转换；而 `i` 看起来是整数，因为另一个函数里有 `int i` | 局部声明类别查找会走到所在函数体，取**最近**的声明 |

这些修法在形态上是一致的：**找到一处可局部判定的证据，没有证据就不改写。**

### 5.4 必须遵守的实现约定

- **控制开关。** 每条有风险的规则都配 `-DGLESMOD_NO_RULE_X` 编译开关。
  它存在的意义是让改动可以被**归因**（A/B 对照），而不是靠假定。
  加规则却不加开关，会让回归无法定位。
- **标记字符串。** 每条规则首次生效时输出一个唯一标记串
  （如 `GLESMOD_RULE_S_BUILTIN_INT_ARGS`），登记在
  `native/tools/required_strings.txt`。构建门禁会断言每个标记都存在于
  产物 `.so` 中。没有这道关，**陈旧的二进制会伪装成已修复** ——
  这个错误已经实实在在浪费过一整轮真机测试。
- **前置声明。** 定义在文件后段、但被 `process_stmt` 使用的辅助函数，
  必须在文件顶部声明。否则 gcc 退化为隐式 `int` 声明而构建失败；
  真正危险的失败模式是：脚本只检查可执行文件是否存在，于是**继续用旧二进制**。
- **括号配对要写对。** 向左扫描包含该实参的调用时，`)` 必须使深度计数**加一**，
  而 `(` 必须在计数**已经大于零**时才减一。写反了会永远找到最内层括号，
  规则变成静默的空操作。
- **字面量长度包含后缀。** `int_literal_len("3u")` 返回 2。
  不检查就再追加 `u`，会得到非法的 `3uu`。

### 5.5 转换失败时

必须**可见地**失败。编译出错时，后端会把三样东西写进 `glesmod/native.log`：

1. 从 `glShaderSource` 收到的**原始源码**。
2. **实际送入驱动**的源码。
3. 驱动自己的错误原文（来自 `glGetShaderInfoLog`）。

这三者正是让着色器故障变得**可诊断**而非成谜的机制，也是本仓库里每一个转换器
缺陷被找出来的途径。`native/tools/extract_device_fixtures.py` 可以把这样的
转储还原成测试夹具。

---

## 6. Java 层

四个包，不做任何渲染工作。

```
com.youyimc.glesmod
├── GLESMod.java            模组入口（common）
├── GLESModClient.java      客户端入口：启动报告 + 运行期轮询
├── Config.java             NeoForge 配置定义
├── capability/             GlesCapabilities（不可变快照）、
│                           GlesCapabilityProvider（稳定对外接口）
├── backend/                GlesBackendStatus —— 读取并解析 status.json
├── compat/                 CompatDetector、CompatTarget
└── degrade/                DegradeEvent、DegradeReason
```

### 6.1 `GlesCapabilities`

**不可变快照**，发布一次，任意线程可安全读取。它刻意不是「活的」：
能力在运行时不会变化，做成可变只会招来竞态。

`GlesCapabilities.INACTIVE` 是保守默认值（所有标志位为 false、容量取最小值）。
对外接口的每个方法在后端未激活时都必须返回保守值且**绝不抛异常** ——
在贴近渲染路径的地方抛异常会导致游戏崩溃，这与本项目的核心原则冲突。

`getGlesVersion()` 返回**格式稳定**的字符串
（`"OpenGL ES 3.2 (reported as GL 3.2)"`）。模组会解析它，因此格式属于契约的一部分。

### 6.2 `GLESModClient`

三项职责：

1. **启动报告**（`FMLClientSetupEvent`）—— 能力快照、降级事件、兼容性结果。
2. **运行期轮询**（`ClientTickEvent.Post`，每 100 tick ≈ 5 秒）。
3. **配置界面**注册。

轮询不是便利功能。启动报告只发生一次，但有些故障发生得**晚得多** ——
玩家点「应用光影」时 Iris 才去创建 FBO，可能已是启动后一分钟；
而 native 侧只有在 `glCheckFramebufferStatus` 看到不完整 FBO 时才会写下
那条有用的附件明细。没有轮询，这些证据会写进 `status.json` 却**永远不被呈现**。
它同样是**承重**的：在至少一个已确认的启动器（ZalithLauncher2）上，
我们的 stderr 完全不被收集，`status.json` 因此是唯一的可靠诊断通道。

去重按 **`原因 + 明细`** 而非按原因码：一个原因码可以覆盖许多互不相同的问题，
按码去重会把真正的新问题一起吞掉。

轮询失败会被静默在 `debug` 级别 —— 诊断功能绝不能影响游戏运行。

---

## 7. Java ↔ native 契约

### 7.1 为什么用文件而不是 JNI

`libgl_gles.so` 由启动器加载，即处于模组 JVM 的类加载器**之外**。
Java 回调需要跨加载器 attach 线程。两个文件是双方都能稳定写入的通道，
且没有这层复杂度。

### 7.2 `glesmod/status.json`

路径可由 `GLESMOD_STATUS_FILE` 覆盖。Java 侧是 `backend` 包的
`GlesBackendStatus`。

文件中包含 `abi_version`、`active`、`gles_version`、`degrade_level`、
能力布尔位、容量整数、`degrade_events`、`missing_symbols`、
`stub_symbols`、`last_calls`。

**改动这个文件时必须知道的解析细节**：

- Java 侧是**正则解析，不是 JSON 库**。格式由本项目产出，扁平且固定；
  为读一个小文件引入依赖并不划算。若你增加嵌套，就要同步更新那些正则。
- `missing_symbols` 与 `stub_symbols` 结构相同，都是
  `{"count":N,"names":[…]}`。解析器**先定位所属外层块**，再在块内取 `names`。
  若全局匹配 `"names"`，只会命中第一个块并静默丢掉 stub 列表 ——
  而 stub 列表恰恰是「哪些功能实际失效」的信息。
- **ABI 不匹配会被拒绝。** jar 与 `.so` 分开分发、可能版本漂移，
  因此会校验版本；不匹配时返回未激活状态，而不是给出错误解读。

### 7.3 ABI 版本演进规则

`GLESMOD_ABI_VERSION` 当前为 `1`。

- 在**保留位**新增能力位 → **不**递增。
- 修改已用位的含义，或任何结构体/数组布局 → **必须**递增。

### 7.4 能力位

`gles_backend.h` 里的 `GLESMOD_CAP_*`，在 `GlesCapabilities` 中镜像为布尔字段。
第 11–31 位保留，必须为 0。位的分配以头文件为准；
Java 侧解析的是 JSON 而非位，因此两处必须同步修改。

---

## 8. 配置

三层，优先级递减：

```
FCL 插件环境变量  >  NeoForge 配置文件  >  自动探测  >  内置默认值
```

环境变量优先，因为它是**在游戏之外做出的、每次启动的显式选择**。

### 8.1 环境变量（`gles_backend.h`）

| 变量 | 取值 | 含义 |
|---|---|---|
| `GLESMOD_ENABLE` | 0/1 | 总开关 |
| `GLESMOD_DEGRADE_LEVEL` | 0/1/2 | 0 保守，1 默认，2 激进 |
| `GLESMOD_TRACE` | 0/1/2 | 0 关闭；1 **只记录初始化前的调用（默认）**；2 全量记录 |
| `GLESMOD_STATE_CACHE` | 0/1 | 冗余状态调用消除 |
| `GLESMOD_LOG_LEVEL` | 0/1/2 | 默认 1；2 会输出逐符号解析日志 |
| `GLESMOD_STATUS_FILE` | 路径 | 覆盖 `status.json` 位置 |
| `GLESMOD_LOG_FILE` | 路径 | 覆盖 `native.log` 位置 |
| `GLESMOD_SHADER_PROBE` | 0/1 | 编译失败时转储着色器源码 |
| `GLESMOD_GEOM_PROBE` | 0/1 | 几何诊断探针 |
| `GLESMOD_PROXY_EMU` | 0/1 | 默认 1：proxy texture 仿真；0 退回旧行为（调试用） |
| `GLESMOD_ABI` | 整数 | 期望的 ABI 版本 |
| `LIBGL_GLES` | 库名 | **可选**。显式指定兜底 GLES 库；设置后优先于 `POJAVEXEC_EGL` 推断。插件自身不设置它 |
| `POJAVEXEC_EGL` | 库名 | 由启动器设置；我们必须从**同一个实例**取 `eglGetProcAddress`。v2 的 ANGLE 开关把它切到 `libEGL_angle.so` |

> **同源规则**：native 只有在 `LIBGL_GLES` 未设置、且 `POJAVEXEC_EGL` 含
> `angle` 时才会选择 `libGLESv2_angle.so`。因此正常使用 v2 开关不会错配；
> 手动把 `LIBGL_GLES` 设成厂商库、同时打开 ANGLE 开关，才会制造
> 「ANGLE EGL + 厂商 GLES」的错配。详见 §3.2。
>
> `GLESMOD_PROXY_EMU` 只影响 proxy texture 仿真开关；完整仿真范围固定为
> 2D / cube / rectangle，其他 proxy target 始终安全返回 0。

`GLESMOD_TRACE=1` 值得解释。全量序列追踪要在**每一次** GL 调用上付一次分支
和一次原子自增；实测到达真实调用需要 29 条指令，而只追踪初始化阶段是 15 条。
按每帧约 5×10⁴ 次调用估算，差约 0.2 ms/帧，在 300 FPS 下占 6 %。
我们追查的两起真机 SIGSEGV 都是**早期初始化**问题，所以默认档已覆盖实际需求；
而第 2 档仍保留，供「长时间运行后才崩」的场景使用，无需重新构建。

### 8.2 NeoForge 配置（`Config.java`）

`enabled`、`degradeLevel`、`verboseLog`、`warnIfInactive`、`pollStatusFile`。

`degradeLevel` 由 native **从环境变量**读取，因此在配置文件里改动它需要重启 ——
配置注释中已说明。

---

## 9. 兼容性检测

`CompatDetector` 只做一件事：读模组列表并调整策略。

它**不**对 Sodium / Embeddium 做 patch、Mixin 注入或反射。理由：
它们的内部实现随版本变动；注入会造成难以诊断的冲突且升级即失效；
并且 Sodium 自 0.6 起采用 Polyform Shield，复制其代码在任何情况下都不被允许。

| 检测到 | 判定 |
|---|---|
| Sodium `0.8.13…`（**前缀**匹配） | `FULLY_SUPPORTED` |
| 其它 Sodium 版本 | `CONSERVATIVE` |
| Embeddium 任意版本 | `CONSERVATIVE` |
| 都没有 | 原版路径，并记录日志 |

前缀匹配是刻意的，这样 `0.8.13+build.7` 这类构建变体也能被接受。
取向是明确的：**宁可保守，不可激进** —— 未知版本走保守路径并**明说**，
而不是去猜测内部实现。

---

## 10. 诊断信息与阅读方法

### 10.1 两个文件

| 文件 | 写入方 | 读取方 |
|---|---|---|
| `glesmod/status.json` | native | Java：启动时一次，之后每约 5 秒 |
| `glesmod/native.log` | native | 人 |

`native.log` 存在的原因是**依赖 stderr 在这里不安全**：
真机上遇到过游戏日志里本库输出为**零行**的情况，而那种情况在
「我们根本没被加载」与「stderr 没被收集」之间是**有歧义**的 ——
两者排查方向完全相反。自己写文件就消除了这个歧义。

### 10.2 阅读着色器失败

查找 `编译失败的着色器 —— 阶段=…，GL 名称=N`，其后是
「原始源码 / 转换后源码」两段，以及驱动原话。**对比两份源码，差异就是我们所改的
内容。** 每一个转换器缺陷都是这样被找到的，
`native/tools/extract_device_fixtures.py` 可以把转储变成永久回归夹具。

### 10.3 解读启动报告

```
GlesCapabilities{active=true, gles=3.2, reportedGl=3.2, degradeLevel=1,
 multiDraw=false, computeShader=true, persistentMapping=true,
 instancing=true, indirectDraw=true, maxTextureUnits=96, maxDrawBuffers=4,
 maxTextureSize=16384, maxSamples=4, maxVertexAttribs=32}
```

ES 上 `multiDraw=false` 是**预期行为，不是故障** —— 它表示驱动没有暴露
`GL_EXT_multi_draw_indirect`。本实现只对该扩展的 indirect 入口做原生加速；
非 indirect 的 `glMultiDraw*` 始终走循环拆解。

---

## 11. 测试与门禁

一切都在**主机**上、几秒内完成，**不需要设备**。这是刻意的：
转换器回归在真机上只表现为一条难以理解的驱动消息，
用那种方式发现问题既慢又贵。

```bash
py -X utf8 native/tools/run_all_gates.py
```

| 门禁 | 它确立了什么事 |
|---|---|
| `roundtrip` | 夹具可转换，且输出能通过**真正的 GLSL ES 前端**（NDK 的 `glslc`）编译 |
| `veil pinwheel` | Veil 的 48 个着色器可转换且可编译 |
| `mc core` | Minecraft 全部 125 个核心着色器可转换且可编译 |
| `bsl pack` | 真实光影包的 182 个着色器 |
| `flywheel dumps` | Flywheel 在真机上装配后的着色器重新转换无污染 |
| `sable splice` | 把 Sable 的 Flywheel 覆盖文件拼接进真机 dump 后再转换 |
| `gitignore` | 第三方素材无法进入 git 历史 |
| `device fixtures` | **真机上确实失败过**的着色器现在能编译 |
| `builtin int args` | 实参为 `int` 的内建函数不会被浮点化 |
| `artifact freshness` | 交付的二进制确实包含当前修复 |

### 11.1 两条不可选的方法论

**归因。** 「现在通过了」不能证明**是你的改动**使它通过。
用对应的 `-DGLESMOD_NO_RULE_X` 构建并确认故障重现。
没有做过前后对照的改动，不算已验证。

**基准选择。** 对比转换器输出时，**不要**拿原始源码直接与转换后源码做 diff。
两者之间有**两处**差异 —— `#version`/精度头 **以及**我们的改写 ——
于是一条我们从未触碰的语句，也可能仅仅因为「桌面 GLSL 有隐式转换、ES 没有」
而显得「被改坏了」。正确的基准是 `raw_es`：原始源码**只换头、不做任何转换**，
代表一个纯粹的原样转发者会交给驱动的源码。

```
raw_es PASS + conv FAIL  ->  我们引入的回归
raw_es FAIL + conv PASS  ->  我们修好了
raw_es FAIL + conv FAIL  ->  仍未修
```

---

## 12. 构建

```bash
./gradlew build                 # NeoForge 模组 jar
```

```powershell
./build-all.ps1 -WithPlugin     # jar + 两个 ABI 的 .so + 插件 APK
./build-all.ps1                 # 只构建 jar + .so
```

> `-WithPlugin` **不会**自动生效。不带它时 APK 不会被重建，
> 于是代码改了，设备加载的仍是上一个原生库。
> 这个坑已经浪费过一整轮真机测试，所以有了
> `check_artifact_freshness.py` —— 它比对二进制里的**标记字符串**，而不是时间戳。

工具链：JDK 21、Android SDK（`aapt2`、`d8`、`zipalign`、`apksigner`）、
Android NDK 27.x（Clang、CMake、Ninja、`glslc`）。
见 [`build-environment.md`](build-environment.md)。

省时间的几点：

- 本环境下 Gradle 必须带 `--offline`（NeoForged 的 maven 主机不可达），
  因此构建依赖本地 Gradle 缓存。
- `org.gradle.configuration-cache=false` 是**必需**的：ModDevGradle 2.0.146 在
  Gradle 配置缓存下无法序列化 `CreateMinecraftArtifacts`。
  这是插件的限制，不是本项目的缺陷。
- `glslc` 必须传 `--target-env=opengl`。

---

## 13. 如何扩展本项目

### 13.1 新增一个转发的 GL 函数

1. 在 `native/symbols.def` 加条目，类别 `F`。
2. 运行 `py native/tools/gen_symbols_def.py` / `gen_gl_forwarders.py`。
3. 运行 `verify_ptr_types.py` —— 它按 `gl.xml` 校验指针星号。
   曾经漏掉一个 `*`，把 `glGetIntegerv(GLenum, GLint *)` 变成
   `glGetIntegerv(GLenum, GLint)`；在 AArch64 上指针被截断为 32 位，
   驱动在 `GL.createCapabilities()` 内部发生段错误。
   **编译期与链接期都看不出任何异常。** 因此这道门禁必须跑。

### 13.2 新增一个安全桩

同上，类别 `S`。随后决定调用它是否应记录降级事件 ——
通常应该，因为桩被调用意味着某个功能是惰性的。

### 13.3 新增一条转换规则

1. 先写一个**最小夹具**，它在改动前失败。优先从真机故障派生
   （`extract_device_fixtures.py`）。
2. 用对照构建（`-DGLESMOD_NO_RULE_X`）确认故障确实存在。
3. 实现规则，并配自己的开关。
4. 输出**标记字符串**并登记到 `native/tools/required_strings.txt`。
5. 证明 A/B 差异，并重跑全部门禁。
6. 在 `shader.c` 中连同**真机证据**一起写进注释 ——
   现有每条规则都内联携带自己的证据，正是这一约定让该文件可维护。

### 13.4 新增一个能力位

在**保留位**加 `GLESMOD_CAP_*`（不需递增 ABI），
在 `GlesCapabilities` 与 JSON 解析器中镜像，并写文档。
改动已用位则必须递增 ABI。

---

## 14. 已知的文档与代码不一致之处

记录下来，以免有人被过时文字误导：

- **[`docs/capability-interface.md`](capability-interface.md) 仍描述 JNI 返回值契约**（§4）。
  实际边界是 `glesmod/status.json`，没有 JNI；该节待重写。
- **[`docs/architecture.md`](architecture.md) 已于 2026-11-07 同步**当前源码结构、
  ANGLE 可选路径与 proxy texture 仿真；若仍有细节冲突，以本文与代码为准。

以本文与代码为准；本文与 `architecture.md` 冲突时，以代码为准。

---

## 许可证

LGPL-3.0-or-later —— 见 [`../LICENSE`](../LICENSE)。
第三方成分列在 [`../THIRD-PARTY-NOTICES.md`](../THIRD-PARTY-NOTICES.md)。

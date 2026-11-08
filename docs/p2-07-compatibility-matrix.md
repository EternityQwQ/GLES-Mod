# P2-07：Sodium / Embeddium 兼容矩阵与已知问题

> 状态：✅ **已完成**（真机验证基线；已含 Embeddium 实机结果）
> 日期：2026-09-29
> 对应任务：P2-07
> 关联文档：`docs/p2-04-degrade-routing.md`、`docs/p2-05-embeddium-assessment.md`、`docs/o-02-symbol-inventory.md`
> 验证设备：Xiaomi houji 23127PN0CC / SM8650 / **Adreno 750** / Android 16 / GLES 3.2

> ### 修订记录
>
> | 版本 | 日期 | 变更 |
> |---|---|---|
> | v1 | 2026-09-29 | 初版：基于 Sodium 真机验证结果 |
> | v2 | 2026-09-29 | §4.1 高危项已修复；新增构建期导出符号校验门禁 |
> | v3 | 2026-09-29 | **新增 Embeddium 实机结果**：无视觉问题、帧率与 Sodium 相当；§4.1 查明该风险路径**实测未被触发** |
> | v4 | 2026-11-07 | 新增 **ANGLE 可选后端**验证：Adreno 619 / Mali-G57，原生与 ANGLE 双路径通过；补充 proxy texture 修复记录 |
> | v5 | 2026-11-07 | 新增 Mali `#error "..."` 词法错误修复记录（Sodium + Iris 进世界崩溃的根因） |
> | v6 | 2026-11-08 | 新增 ANGLE Multi-Draw 检测验证：`GL_EXT_multi_draw_indirect=1`，`multiDraw=true` |

---

## 1. 兼容矩阵总表

| 优化模组 | 版本 | MC | Loader | 状态 | 验证方式 | 备注 |
|---|---|---|---|---|---|---|
| **Sodium** | `0.8.13+mc1.21.1` | 1.21.1 | NeoForge | ✅ **FULLY_SUPPORTED** | **真机实测** | 主目标 |
| **Embeddium** | `1.0.15+mc1.21.1` | 1.21.1 | NeoForge | ✅ **真机验证通过**（1 轮） | **真机实测** | 无视觉问题，帧率与 Sodium 相当；见 `p2-05` §9 |
| Sodium | 其他 0.8.x | 1.21.1 | NeoForge | ⚠️ 保守策略 | 未测 | 版本号前缀不匹配即降级为保守 |
| Sodium | < 0.8 | 1.21.1 | NeoForge | ❌ 不支持 | — | 内部实现差异过大 |
| Embeddium | 1.0.15+mc1.21.1 | 1.21.1 | NeoForge | ✅ 真机通过 | **已测 1 轮** | 建议再测 2 轮后提升至 FULLY_SUPPORTED |
| Embeddium | < 1.0.15 | 1.21.1 | NeoForge | ⚠️ 保守策略 | 未测 | 同上 |
| — | 无优化模组 | 1.21.1 | NeoForge | ✅ 原版路径 | **真机实测** | 原版不使用任何 ES 受限功能 |

### 1.1 Sodium 版本判定逻辑

`CompatDetector.checkOne()` 的实际行为：

```java
public static final String KNOWN_SODIUM_VERSION = "0.8.13";
...
if (version.startsWith(KNOWN_SODIUM_VERSION)) {
    return new CompatTarget(modId, version,
            CompatTarget.SupportLevel.FULLY_SUPPORTED,
            "已验证版本，使用专用策略");
}
return new CompatTarget(modId, version,
        CompatTarget.SupportLevel.CONSERVATIVE,
        "未经测试的版本，使用保守策略");
```

| 检测到的版本 | 判定 |
|---|---|
| `0.8.13...`（前缀匹配） | `FULLY_SUPPORTED` |
| 其他任意版本 | `CONSERVATIVE` |
| Embeddium 任意版本 | `CONSERVATIVE` |

**设计原则：宁可保守，不可激进。** 未知版本一律走保守路径并明确告知用户，
避免因猜测内部实现而导致崩溃。这也是为什么用**前缀匹配**而非精确匹配（允许 `0.8.13+build.xyz` 这类变体）。

### 1.2 未做 Mixin 注入——这是刻意的

本模组**不修改** Sodium / Embeddium 的代码，也不反射调用其内部类。只读取模组列表中的公开版本信息。

理由（`CompatDetector` 文件头原文）：

> Sodium 的内部实现随时可能随版本变动，注入会导致难以诊断的冲突，且升级即失效。
> 软依赖 + 保守策略更可持续。

**许可证约束**：Sodium 自 0.6 起采用 **Polyform Shield** 许可证，**不得复制其代码**。本项目只做运行时检测。

### 1.3 渲染后端：原生 GLES vs 可选 ANGLE

| 后端 | 启用方式 | 链路 | 默认 | 状态 |
|---|---|---|---|---|
| 原生 GLES | 默认 | 桌面 GL → `libgl_gles.so` → 厂商 GLES 驱动 | ✅ | ✅ 已验证 |
| ANGLE | 插件 v2 开关（`POJAVEXEC_EGL=libEGL_angle.so`） | 桌面 GL → `libgl_gles.so` → ANGLE → Vulkan | ❌ | ✅ 已验证（Adreno 619 / Mali-G57） |

ANGLE 是**第二层翻译**，只在原生 GLES 驱动异常、或需要 ANGLE 的 Vulkan 路径时
才建议打开。两者共用同一份 native 核心和同一套 proxy texture 仿真；ANGLE 只是把
最终执行层从「厂商 GLES」换成「ANGLE → Vulkan」。

已知后端差异（实测，非缺陷）：

| 能力 | 原生 Adreno 619 | ANGLE 2.1.1 |
|---|---|---|
| `GL_RENDERER` | `Adreno (TM) 619` | `ANGLE (Qualcomm, Vulkan 1.1.128, Adreno 619)` |
| `maxTextureUnits` | 96 | 192 |
| `maxVertexAttribs` | 32 | 16 |
| proxy texture 2D 探测 | 仿真，32768 → 0 | 仿真，32768 → 0 |

> Turnip 目前**不作用于 ANGLE**：即使设置 `DRIVER_LIB=libvulkan_freedreno.so`，
> ANGLE 仍使用厂商 Vulkan 驱动。这是独立问题，不在本次收口范围。

---

## 2. ES 能力 vs 优化模组需求的交叉矩阵

| 功能 | ES 3.2 核心 | Adreno 750 实测 | 原版 MC | Sodium | Embeddium | 我们的处理 |
|---|---|---|---|---|---|---|
| **Multi-Draw** | ❌ 无 | 仅 indirect 扩展时原生，否则循环 | ❌ 不用 | ✅ 会用 | ✅ 会用 | `GL_EXT_multi_draw_indirect` 原生优先，其余循环拆解 |
| **计算着色器** | ✅ 3.1+ | ✅ `true` | ❌ 不用 | 部分特性 | 未确认 | 直接转发 |
| **间接绘制** | ✅ 3.1+ | ✅ `true` | ❌ 不用 | 会用 | 未确认 | 直接转发 |
| **持久映射** | ⚠️ 需扩展 | ✅ `true`（`glMapBufferRange`） | ❌ 不用 | 会用 | ✅ **会用** | ⚠️ **见 §4 已知问题** |
| **缓冲存储**（`glBufferStorage`） | ❌ 无（GL 4.4） | ❌ | ❌ 不用 | 未确认 | ✅ 会用（但**实测未被触发**） | ✅ **已修复：真实分配 + 降级** |
| **实例化** | ✅ 3.0+ | ✅ `true` | ❌ 不用 | 会用 | 会用 | 直接转发 |
| **MRT** | ✅ 3.0+ | ✅ `maxDrawBuffers=4` | 会用 | 会用 | 会用 | 直接转发 |
| **`glFenceSync`** | ✅ 3.0+ | ✅ | ❌ 不用 | 会用 | 会用 | 直接转发 |
| **纹理存储**（`glTexStorage2D`） | ✅ 3.0+ | ✅ `true` | ❌ | 会用 | 未确认 | 直接转发 |
| **几何/细分着色器** | ❌ 无 | ❌ | ❌ | ❌ | ❌ | **策略允许的失败范围** |
| **`glLogicOp`** | ❌ 无 | ❌ | \(\approx\)不用 | ❌ | ❌ | 忽略 + 降级 |
| **`glPolygonMode`** | ❌ 无 | ❌ | \(\approx\)不用 | ❌ | ❌ | 仅 `GL_FILL` 静默 |

> **"原版 MC" 一列的结论来自 `docs/o-02-symbol-inventory.md`**：MC 1.21.1 只引用到 `GL30` 绑定类，未引用 `GL31`/`GL32` 或更高。
> 因此 **ES 3.0 即可满足原版**，ES 3.2 是充裕的。

---

## 3. 真机验证记录

### 3.1 环境

| 项 | 值 |
|---|---|
| 设备 | Xiaomi houji 23127PN0CC |
| SoC | SM8650（骁龙 8 Gen 3） |
| GPU | **Adreno 750**（`libGLESv2_adreno.so`） |
| 架构 | arm64 |
| Android | 16（SDK 36） |
| 面板 | 120 Hz |
| MC / NeoForge | 1.21.1 / 21.1.241 |
| JVM | OpenJDK 21.0.1 |
| Sodium | 0.8.13+mc1.21.1 |
| 启动器 | FCL 1.3.1.6（versionCode 1316）、ZL2 2.4.9_hotfix1 |
| 分辨率参数 | `--width 2670 --height 1200` |

### 3.2 通过项（Sodium 路径）

```
[INFO] GLES 后端已激活，GLES 3.2，无功能降级。
[INFO] 后端能力: GlesCapabilities{
         active=true, gles=3.2, reportedGl=3.2, degradeLevel=1,
         multiDraw=false, computeShader=true, persistentMapping=true,
         instancing=true, indirectDraw=true,
         maxTextureUnits=96, maxDrawBuffers=4, maxTextureSize=16384,
         maxSamples=4, maxVertexAttribs=32}
[INFO] 降级事件 (0 类):
[INFO] 优化模组: sodium 0.8.13+mc1.21.1 -> FULLY_SUPPORTED（已验证版本，使用专用策略）
[INFO]   - Multi-Draw（Sodium 区块渲染会降级为逐次绘制，性能略降）
```

| 验收项 | 结果 |
|---|---|
| 主菜单可达 | ✅ |
| 进入世界 | ✅ |
| 区块渲染正常 | ✅ |
| 崩溃 | ❌ 无 |
| 着色器编译失败 | ❌ 无 |
| 缺失符号（`missing_symbols`） | ✅ 0 |
| 被调用的 stub（`stub_symbols`） | ✅ 0 |
| 渲染错误 | ✅ 两个启动器下均为 0 |

### 3.3 Sodium 侧的工作区配合

Sodium 自身也应用了一个 workaround（日志原文）：

```
[WARN] Sodium has applied one or more workarounds to prevent crashes
       or other issues on your system: [NO_ERROR_CONTEXT_UNSUPPORTED]
```

`NO_ERROR_CONTEXT_UNSUPPORTED` = Sodium 检测到环境不支持"无错误上下文"设置（`KHR_no_error` / `GL_CONTEXT_FLAG_NO_ERROR_BIT`），
因此关闭了该优化。这是 **Sodium 自己的能力协商**，属预期行为，非缺陷。

### 3.4 性能

| 启动器 / 渲染器 | FPS | 判定 |
|---|---|---|
| **ZL2 + 本项目 + Sodium** | **300 ~ 400** | 远高于验收线 |
| **ZL2 + MobileGlues + Sodium** | 与本项目**几乎无差别** | 同一层次 |
| **ZL2 + Zink + Sodium** | 与本项目**几乎无差别** | 同一层次 |
| ZL2 + 本项目 + **Embeddium** | 与 Sodium **基本一致** | — |
| FCL 1.3.1.6 + 本项目 + Sodium | 120（=面板刷新率） | 非本项目问题，见 §5.1 |

> **条件**：以上对比均在**原版环境**（无光影、无大型模组包）下测得。
> 其他负载（光影、大模组包、高视距）**未测**。

#### 3.4.1 如何正确解读「几乎无差别」

| 维度 | 判读 |
|---|---|
| 与其他主流渲染器**并列** | ✅ 可声称 |
| **超越**其他渲染器 | ❌ **不可声称** |

**为什么「没差别」是好消息**：

MobileGlues（GL→ES 转发）与 Zink（GL→Vulkan）都在 **native 层**，
与本项目处于**同一层次**，且都直接调用 GPU 驱动。
在**原版 + Sodium** 这类负载下表现相近，说明：

- **链路健康** —— 没有翻译层被重复叠加，没有引入额外开销
- 本项目**不引入中间层**（直接输出 ES 3.2；Zink 需经 Vulkan 再翻译一层）
  的预期优势，在这类负载下**恰好测不出来**（因为瓶颈不在翻译层）

⇒ 差异应体现在**更极端负载**下。这是后续需要验证的方向。

### 3.5 ANGLE 可选后端验证（2026-11-07）

在两台设备上分别验证「原生 GLES」与「ANGLE」两条路径，均包含启动、进世界、
退出游戏：

| 项 | 原生模式 | ANGLE 模式 |
|---|---|---|
| `POJAVEXEC_EGL` | `libEGL.so` | `libEGL_angle.so` |
| GLES 库 | `libGLESv2.so` | `libGLESv2_angle.so` |
| `GL_RENDERER` | `Adreno (TM) 619` | `ANGLE (Qualcomm, Vulkan 1.1.128, Adreno 619)` |
| proxy 2D 探测 | `32768x32768 max=16384 -> 0x0` | 同左 |
| 启动崩溃 | 无 | 无 |
| 进世界 / 退出 | ✅ | ✅ |
| Sodium / Iris | ✅ | ✅ |

验证设备：

| 设备 | SoC | GPU | Android | 结果 |
|---|---|---|---|---|
| Oppo PGGM10 | SM6375 | Adreno 619 | 14（SDK 34） | ✅ 原生 + ANGLE |
| Mali-G57 设备 | — | Mali-G57 | — | ✅ 原生 + ANGLE |

结论：

- v2 开关只切换 `POJAVEXEC_EGL`，native 自动选择同源 GLES，**没有 EGL/GLES 错配**；
- proxy texture 仿真在两种后端下都生效，ANGLE 2.1.1 的
  `RenderSystem.maxSupportedTextureSize` 崩溃不再出现；
- 本轮 MC/Sodium 只使用 `GL_PROXY_TEXTURE_2D`，没有触发「未实现 proxy target」
  日志，符合收窄后的预期。

### 3.6 ANGLE Multi-Draw 检测（2026-11-08）

同一构建 `v10 / b88b129` 分别跑 ANGLE 与原生模式，启动日志对比：

| 项 | ANGLE | 原生 |
|---|---|---|
| `POJAVEXEC_EGL` | `libEGL_angle.so` | `libEGL.so` |
| `GL_EXT_multi_draw_indirect` | **1** | 0 |
| `arrays` | **1** | 0 |
| `elements` | **1** | 0 |
| `multiDraw` | **true** | false |
| Multi-Draw 降级提示 | 无 | 有 |

ANGLE 日志原文：

```text
[GLESMod] Multi-Draw: GL_EXT_multi_draw_indirect=1, arrays=1, elements=1
multiDraw=true
ES 能力满足优化模组的全部功能需求，无降级。
```

原生日志：

```text
[GLESMod] Multi-Draw: GL_EXT_multi_draw_indirect=0, arrays=0, elements=0
multiDraw=false
Multi-Draw（Sodium 区块渲染会降级为逐次绘制，性能略降）
```

结论：

- ANGLE 2.1.1 暴露 `GL_EXT_multi_draw_indirect`，两个 indirect 入口均可解析；
- GLES-Mod 在 ANGLE 模式下启用原生 indirect 路径，不再记录 Multi-Draw 降级；
- Adreno 原生 GLES 驱动未暴露该扩展，保持逐条回退；
- 两种模式均正常进世界、退出，无崩溃。

---

## 4. 已知问题清单

### 4.1 ✅ 已真机验证：`glBufferStorage` 路径**未被触发**

**机制**：`glBufferStorage` 在 `symbols.def` 中曾是 `S`（安全 stub，只记降级不做事）。
Embeddium 的 `MappedStagingBuffer` 依赖它**真实分配存储**，否则后续 `glMapBufferRange`
会失败并抛 `RuntimeException`。

**详细证据链见 `docs/p2-05-embeddium-assessment.md` §4。**

**实测结果（2026-09-29，ZalithLauncher2）**：**该路径未被走到**，无崩溃、无视觉问题。
三条件证据：

1. 日志出现 `EsxBufferObject::Map - Ignoring EsxBufferMapUnsyncedBit`
   → `GL_MAP_UNSYNCHRONIZED_BIT` 是 **`FallbackStagingBuffer` 路径独有**的标志，
   `MappedStagingBuffer` 不含它；
2. `Pending memory %d (%3d%%)` 的 `%d` **未被替换** → 印证走的是回退路径；
3. 根因：`pickBest()` 返回 `NONE`，因 LWJGL 在我们的 ES 后端下**不上报**
   `OpenGL44` / `GL_ARB_buffer_storage`（`glGetString(GL_VERSION)` 是直接转发，
   返回驱动原话 `"OpenGL ES 3.2 ..."`）。

**仍已修复**（理由与是否被触发无关）：

| 符号 | 原类型 | 新类型 | 新行为 |
|---|---|---|---|
| `glBufferStorage` | `S` 静默 stub | **`C` 定制实现** | 用 `glBufferData` 真实分配存储 + 记降级 |
| `glMapBufferRange` | `F` 直接转发 | **`C` 定制实现** | 剥离 ES 非法的持久/一致映射位后转发 + 记降级 |

**验证**：两个 ABI 的 `libgl_gles.so` 均导出这两个符号（`llvm-nm` 实测，导出函数 869 个）；
该校验已接入 `build-all.ps1` 的 `[4/5]` 门禁。

**状态**：✅ **已修复 + 已真机确认为未触发路径**（新构建待测）

---

### 4.2 ℹ️ 低危：`GL_TEXTURE_LOD_BIAS` 被过滤

**问题**：MC 的 `TextureUtil` 会调用 `glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f)`，而该 pname 在 ES 中非法。

**判定**：值恒为默认 0.0f、MC 从不回读、全字节码仅 `TextureUtil` 引用该枚举 → **丢弃语义等价**。

**修复**：`glTexParameterf/i` 过滤该 pname。**真机验证：70 → 0 次。**

**附带教训**：过滤后**不**排空 GL 错误队列。`glGetError` 是 FIFO 队列，排空会连带丢弃 MC 真正关心的错误，**掩盖真实故障**。
原则：**不清理 MC 的 GL 状态，只避免自己制造污染。**

---

### 4.3 ℹ️ 低危：Adreno 忽略 `GL_MAP_UNSYNCHRONIZED_BIT`

**日志**：
```
1 × EsxBufferObject::Map - Ignoring EsxBufferMapUnsyncedBit
```

**判定**：Adreno 驱动**忽略**该位，可能导致隐式同步停顿（潜在性能损失），但**不产生错误**。
属驱动侧行为，**不视为缺陷**。

**可观测性说明**：该停顿在帧率被外部因素限制时**无法观测**。

---

### 4.4 ℹ️ 无害：驱动侧噪声

| 日志 | 次数 | 判定 |
|---|---|---|
| `Abnormally high render area` | 7 | Adreno 功耗管理，自愈 |
| `Reset max power request` | 6 | 同上 |
| `Namespace collision detected, using slow path` | 7 | 见下方说明 |
| `unable to generate a triangle primitive because there are less than 3 vertices` | 4 | 退化绘制，MC 侧行为 |
| `Warning: high level of unsubmitted work` | 1 | 驱动提示 |

**`Namespace collision detected` 的演变**：最初为 **27229** 次，现为 **7** 次。
这个改善消除了原本计划的"全量改用 `eglGetProcAddress`"优化项的必要性。

### 4.5 ✅ 已修复：驱动端日志通道

**历史问题**：native 层最初只写 `stderr`，依赖启动器重定向。但真机出现过"游戏日志里一行本库输出都没有"，
此时**无法区分**：

| 情形 | 排查方向 |
|---|---|
| 库根本没被加载进进程 | 排查 FCL 插件配置 |
| 库被加载并调用了，但 `stderr` 没被重定向 | 排查日志通道 |

两者排查方向**完全相反**。因此改为**同时**写 `glesmod/native.log` 文件，绕开重定向的不确定性。

**相关教训（值得单独记）**：
> **给用户用的诊断信息，必须走用户实际收集的那个通道。**

曾发生过一次：构建戳（build stamp）最初只写文件，结果在用户提交的启动器日志里 **0 命中**，
导致我误判"新构建没生效"。修复方式：`log_load_banner()` 与 `glesmod_trace_dump()` 同时输出到 `stderr`。

---

### 4.6 ⚠️ 中等：测试陈旧二进制导致的假阴性

**现象**：修好着色器转换后仍崩溃，日志显示探针 banner 是 **v9**，而源码已是 **v10**。

**根因**：**Android 每次安装都会重新生成 `~~<random>==` 签名**。
日志中的插件安装路径签名 `ZctRY6-tdW5YcQzmCEhP-A` 与上次**完全相同** ⇒ 插件 APK **根本没有被重新安装**。

**对策**：加入**构建戳**（版本号 + `__DATE__` + `__TIME__`），使"跑的到底是哪个构建"可被直接读出。

**教训**：改完代码后，**必须确认设备上跑的确实是新构建**，否则会浪费时间排查已修好的问题。

### 4.7 ✅ 已修复：Mali 拒绝 `#error "..."` 的字符串消息

**现象**：Mali-G57 进入世界时崩溃，栈在 Sodium `GlShader.<init>`：

```text
Shader compilation log for sodium:blocks/block_layer_opaque.vsh:
2:65: L0001: Unknown character '"'(34)
```

**根因**：Sodium 的 `chunk_vertex.glsl` 在非活动 `#else` 分支里有：

```glsl
#error "Vertex compression must be enabled"
```

GLSL ES 没有字符串字面量。Mali 的词法器即使在非活动分支里也会扫描到引号，
于是 `"` 被报为未知字符。Adreno 能容忍，所以此前未暴露。

**修复**：转换器的 `normalize_preprocessor_directives()` 现在把 `#error`
消息规范化为合法 token，同时保留指令与行号：

```glsl
#error Vertex compression must be enabled
```

**验证**：主机端用例 `[23]`、A/B 开关 `GLESMOD_NO_ERROR_DIRECTIVE_FIX`、
标记字符串 `GLESMOD_ERROR_DIRECTIVE_NORMALIZED`；已用真机日志中的 163 行原始
着色器离线回放，输出为 `#error Vertex compression must be enabled`（无引号）；
Mali 真机待装新 APK 复测。

---

## 5. 与启动器相关的已知事项（非本项目缺陷）

### 5.1 FCL < 1.3.3.2 帧率被锁在面板刷新率

| 项 | 内容 |
|---|---|
| **现象** | FCL 下帧率恒为 120（=120Hz 面板刷新率），关闭游戏内垂直同步无效 |
| **根因** | FCL 旧版 BufferQueue 处于**同步模式**，`dequeueBuffer` 按 vsync 阻塞 |
| **证据** | FCL CHANGELOG `[1.3.3.2] - 2026-09-13` 新增"解除帧率锁定"；用户版本 `versionCode 1316` = **1.3.1.6**（早于该版本） |
| **涉及文件** | `FCL/src/main/jni/ctxbridges/swap_interval_no_egl.c` |
| **处置** | **升级 FCL 到 ≥ 1.3.3.2**。我方零改动 |
| **旁证** | ZL2 2.4.9_hotfix1 有等价实现 → 300~400 FPS |

> **我此前的错误结论**：一度判为"120 = 硬件上限"。错因是**对照实验只换了渲染器，启动器始终固定** ——
> 启动器是那个实验的共同因子，**在原理上不可能被排除**。
> 教训：**对照实验必须覆盖所有可疑因子；「换掉被测对象」只能排除该对象本身。**

### 5.2 ZL2 不重定向 native 的 `stderr`

ZL2 日志中 `[GLESMod]` 的 `stderr` 行为 **0 条**，但 Java 侧 `status.json` 显示 `加载成功: true`。

**判定**：native 库**确实执行且工作正常**，只是该启动器不重定向此通道 —— 这正是 `glesmod/native.log` 文件通道存在的价值（见 §4.5）。

**交叉验证价值**：同一后端 + 两个不同启动器 + **均为零渲染错误**，是很强的正确性证据。

---

## 6. 设备兼容性（初步）

| 设备 | SoC | GPU | GLES | 状态 |
|---|---|---|---|---|
| Xiaomi houji 23127PN0CC | SM8650 | Adreno 750 | 3.2 | ✅ 原生已验证 |
| Oppo PGGM10 | SM6375 | Adreno 619 | 3.2 | ✅ 原生 + ANGLE 已验证 |
| Mali-G57 设备 | — | Mali-G57 | 3.2 | ✅ 原生 + ANGLE 已验证 |
| 其他 Adreno | — | — | — | ⚠️ 未测（同类驱动，预计可用） |
| 其他 Mali | — | — | — | ⚠️ 未测（**注意**：Mali 对 `glGetError` 性能敏感，且误差行为与 Adreno 不同） |
| PowerVR | — | — | — | ⚠️ 未测 |

> **P3-05（设备兼容性数据库与回退开关）将扩展此表。** 当前仅有单设备结论，不构成"广泛兼容"的承诺。

---

## 7. 已知问题汇总速查

| # | 严重度 | 问题 | 影响范围 | 状态 |
|---|---|---|---|---|
| 1 | ✅ 已解决 | `glBufferStorage` 静默 stub | Embeddium（**已实测未触发**） | **已修复 + 已确认路径不可达** |
| 2 | 🟡 中 | 陈旧二进制导致假阴性（测试方法问题） | 我方流程 | ✅ 已用构建戳缓解 |
| 3 | 🟢 低 | `GL_TEXTURE_LOD_BIAS` ES 非法 | MC 自身 | ✅ 已过滤（70→0） |
| 4 | 🟢 低 | Adreno 忽略 `GL_MAP_UNSYNCHRONIZED_BIT` | 驱动侧 | ℹ️ 记录，不修 |
| 5 | 🟢 低 | 驱动功耗/命名冲突噪声 | 驱动侧 | ℹ️ 记录，不修 |
| 6 | ⚪ 非本项目 | FCL < 1.3.3.2 帧率锁定 | 启动器 | ⚠️ 升级 FCL |
| 7 | ⚪ 非本项目 | ZL2 不重定向 `stderr` | 启动器 | ℹ️ 已用文件通道绕过 |
| 8 | ✅ 已修复 | ANGLE 2.1.1 在 `GL_PROXY_TEXTURE_2D` 空指针崩溃 | ANGLE 模式启动 | ✅ native proxy 仿真 + 真机验证 |
| 9 | ✅ 已修复 | Mali 拒绝 `#error "..."` 字符串（L0001） | Mali + Sodium/Iris 进世界 | ✅ 转换器规范化 + 主机用例 |

---

## 8. 结论

1. **Sodium 0.8.13+mc1.21.1 已通过完整真机验证**：`FULLY_SUPPORTED`，零降级事件，零缺失符号，零被调用 stub，零渲染错误。
2. **Embeddium 1.0.15+mc1.21.1 已通过真机验证（1 轮）**：无任何视觉问题，帧率与 Sodium 基本一致。
   其 `glBufferStorage` 路径**实测未被触发**（走的是 `FallbackStagingBuffer`），机制见 §4.1 与 `p2-05` §9。
3. **性能与其他主流渲染器相当**（原版环境），高于验收指标。
4. **保留在 `CONSERVATIVE` 级别**，直到再测 2~3 轮；届时可提升为 `FULLY_SUPPORTED`。
5. **所有其他残留噪声均已定性**，无一是渲染正确性问题。
6. **ANGLE 可选后端已通过 Adreno 619 与 Mali-G57 验证**：默认关闭，开启后 EGL/GLES
   同源切换，proxy texture 仿真正常，无启动崩溃；原生路径无回归。

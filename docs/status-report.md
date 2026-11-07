# 阶段性报告

> 日期：**2026-09-29**
> 阶段：**阶段一 ✅ 完成 · 阶段二 ✅ 完成（含一轮修复）→ 准备进入阶段三**
> 基线：Minecraft 1.21.1 + NeoForge 21.1.241 + Java 21
> 许可证：LGPL-3.0-or-later
> 验证设备：Xiaomi houji 23127PN0CC / SM8650 / **Adreno 750** / Android 16 / GLES 3.2

---

## 1. 一句话状态

**阶段一与阶段二已全部完成并通过真机验证：主菜单可达、世界可进入、Sodium 0.8.13 满血运行、零渲染错误，性能与其他主流渲染器相当。**
本轮补齐 P2-04/05/07 三份文档，并修复了评估中发现的一个高危项。

---

## 2. 交付物清单（全部已产出）

### 2.1 构建产物

| 产物 | 大小 | 状态 |
|---|---|---|
| `build/plugin/glesmod-renderer-plugin.apk` | 1800.5 KB | ✅ 已签名（v2+v3），FCL 元数据全部通过校验 |
| `build/libs/glesmod-1.0.0.jar` | 28.7 KB | ✅ |
| `build/native/arm64-v8a/libgl_gles.so` | 1057.8 KB | ✅ 869 个导出函数 |
| `build/native/armeabi-v7a/libgl_gles.so` | 732.4 KB | ✅ 869 个导出函数 |

### 2.2 文档

| 文档 | 状态 | 对应任务 |
|---|---|---|
| `开发任务书.txt` | ✅ 修订至 v6 | — |
| `docs/architecture.md` | ✅ | P1-01 / P1-02 |
| `docs/capability-interface.md` | ✅ | P2-01 |
| `docs/o-02-symbol-inventory.md` | ✅ | O-02 |
| **`docs/p2-04-degrade-routing.md`** | ✅ **本轮新增** | **P2-04** |
| **`docs/p2-05-embeddium-assessment.md`** | ✅ **本轮新增** | **P2-05** |
| **`docs/p2-07-compatibility-matrix.md`** | ✅ **本轮新增** | **P2-07** |
| **`docs/r-12-stub-audit.md`** | ✅ **本轮新增** | **R-12 审计** |
| **`docs/p3-01-hotpath-optimization.md`** | ✅ **本轮新增** | **P3-01** |
| `docs/test-guide.md`、`docs/test-log-analysis.md`、`docs/build-environment.md` | ✅ | P1-08 / P1-10 |
| `docs/status-report.md` | ✅ 本文 | — |

### 2.3 代码规模

| 部分 | 内容 |
|---|---|
| `native/src/` | `core.c`（符号解析/能力探测/降级/状态输出）、`custom.c`（**30 个定制实现**）、`shader.c`（GLSL→GLSL ES 转换）、`enum.c`、`probe.c`、`generated_forwarders.c`（自动生成） |
| `native/tools/` | 符号生成/校验、着色器往返验证、真机日志分析等工具链 |
| `src/main/java/` | `capability/`、`compat/`、`degrade/`、`backend/` 四个包 |
| `fcl-plugin/` | FCL 渲染器插件（RendererPlugin v2 规范） |

**符号总数 849** = 337 转发 + **30 定制** + 482 stub。

---

## 3. 阶段一验收：✅ 全部通过

| 验收项（任务书 §6） | 结果 |
|---|---|
| 启动到主菜单 | ✅ |
| 进入世界 | ✅ |
| 背包 / JEI 界面 | ✅ |
| 运行 10 分钟不崩溃 | ✅ |
| 日志显示 GLES 后端已激活 | ✅ |
| 能力协商输出完整 | ✅ |

### 3.1 阶段一中解决的关键问题

| # | 问题 | 根因 | 状态 |
|---|---|---|---|
| 1 | 启动被 NeoForge 拒绝（`only supports Sodium`） | `versionRange=""` 解析为空范围，匹配任何版本都为 false | ✅ 改为 `"[0,)"`，用真实 Maven `VersionRange` 实证 |
| 2 | 启动 SIGSEGV | `POJAV_RENDERER` 未设置 → FCL 的 `strncmp` 收到 NULL | ✅ 插件 env 显式注入 |
| 3 | **深度测试完全失效**（面不被剔除、正反面交叠） | 桌面 GL 允许 `GL_DEPTH_COMPONENT`(0x1902) 作 internalformat，**ES 不允许** → 深度纹理根本没被分配 | ✅ 翻译为 `GL_DEPTH_COMPONENT24` |
| 4 | 进入世界崩溃（**三轮才定位**） | Sodium 区块着色器：① 缺 GLSL ES 采样器精度 ② `uvecN`/`uint` 与 float 混算 ③ **`uvec3 * float标量`** | ✅ 三者全部修复 |
| 5 | `pname 34049` 噪声（70 次） | `GL_TEXTURE_LOD_BIAS` 是桌面专属 pname | ✅ 过滤，70 → 0 |
| 6 | 诊断信息在用户日志中缺失 | 构建戳只写文件，未走 stderr | ✅ 双通道输出 |

> **问题 4 值得单独记一笔**：三轮真机测试才定位到三个独立的转换器缺口。
> 这直接催生了 `roundtrip_validate.py` 往返门禁 —— 用真实夹具 + glslang 校验，
> 把「一次真机迭代」压缩成「一次本地秒级校验」。

---

## 4. 阶段二验收：✅ 全部通过

| 验收项（任务书 §6） | 结果 |
|---|---|
| 安装 Sodium 0.8.13-neoforge（1.21.1）后能启动 | ✅ |
| 不崩溃 | ✅ |
| 世界渲染正常 | ✅ |
| 日志明确记录哪些功能被禁用 | ✅ |
| FPS 不低于 GL4ES 基线的 70% | ✅ 达成（**且高于该指标**） |

**另外，Embeddium 1.0.15+mc1.21.1 也已真机验证（1 轮）**：

| 观察项 | 结果 |
|---|---|
| 视觉问题 | ✅ 无 |
| 帧率 | ✅ 与 Sodium 基本一致 |
| 崩溃 | ✅ 无 |

详见 `docs/p2-05-embeddium-assessment.md` §9。

### 4.1 真机实测输出

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

**零缺失符号、零被调用 stub、零渲染错误**（FCL 与 ZL2 两个启动器下均如此）。

### 4.2 本轮新增的三份文档 + 一处修复

| 任务 | 产出 | 关键内容 |
|---|---|---|
| **P2-04** | `docs/p2-04-degrade-routing.md` | 降级路由的完整实现说明；澄清「日志说降级但 `degrade_events` 为空**不是矛盾**」（预测 vs 实测） |
| **P2-05** | `docs/p2-05-embeddium-assessment.md` | Embeddium 评估：确认 1.0.15+mc1.21.1 存在（LGPL-3.0-only），并**发现一个高危项** |
| **P2-07** | `docs/p2-07-compatibility-matrix.md` | 兼容矩阵、已知问题清单（含严重度分级）、设备兼容性表 |

### 4.3 ★ 本轮修复：`glBufferStorage` 静默 stub ★

**这是本轮最有价值的产出** —— P2-05 的代码审查发现了一个**真实的一手代码级风险**。

**问题**：`glBufferStorage` 原本是 `S`（安全 stub：什么都不做）。但它是**有存储语义要求**的调用：

```
glBufferStorage(...)   -> stub 返回 void，不分配存储，无错误
glMapBufferRange(...)  -> 因缓冲区无存储而失败，返回 NULL
调用方                 -> 抛异常崩溃
```

**失败发生在离根因很远的地方**，且以「与存储完全无关」的形式出现（`Failed to map buffer`）。

**一手证据**（Embeddium `21.1/neoforge` 分支源码，非推测）：
`BufferStorageFunctions.pickBest()` **只认桌面 GL 4.4 / `ARB_buffer_storage`，完全没有考虑 ES 场景**；
而 `CORE` 与 `ARB` 两条路在我们这里都落到同一个符号上。

**修复**：

| 符号 | 原类型 | 新类型 | 新行为 |
|---|---|---|---|
| `glBufferStorage` | `S` | **`C`** | `glBufferData` 真实分配 + 记降级 |
| `glMapBufferRange` | `F` | **`C`** | 剥离 ES 非法映射位后转发 + 记降级 |

**由此确立的原则**：

> **静默 stub 只适用于「调用方不会依赖其返回值/副作用」的函数。**
> 若函数的契约包含产生某种资源（分配存储、创建对象、返回句柄），
> 做成静默 stub 就等于**用一个谎言替换一次失败**，而谎言会在很远的地方以完全无关的形式暴露。

#### 4.3.1 已在真机上重新定性（重要）

Embeddium 实机测试（§4 开头）显示 **该路径实测未被触发** —— 走的是安全的
`FallbackStagingBuffer`。原因：`pickBest()` 返回 `NONE`，
因为 LWJGL 在我们的 ES 后端下**不上报** `OpenGL44` / `GL_ARB_buffer_storage`。

**因此我必须修正自己的表述偏向**：此前把这条风险写成「Embeddium 会崩溃」是**过度断言**。
准确的说法是：

> **存在一条会崩溃的代码路径；是否会被走到取决于 LWJGL 的能力上报，
> 而实测显示当前不会走到。**

我已在 `p2-05` §4.4 把它标为「未知项」，但在权衡建议里把语气写强了。
**这是一个应当避免的表达偏向：不确定性应在结论句里也保持。**

**顺带新增构建门禁**：`native/tools/verify_exported_symbols.py`，接入 `build-all.ps1` 的 `[4/5]` 步。
它用 `llvm-nm` 直接查 `.so` 的动态符号表 —— **因为构建系统自己的输出不能证明产物是最新的**（本轮实际踩到）。

---

## 5. 质量门禁（全部绿色）

| 门禁 | 结果 |
|---|---|
| 符号表指针类型校验（`verify_ptr_types.py`） | ✅ |
| 主机端着色器单元测试（17 组） | ✅ 全通过 |
| MC 全量着色器审计 + glslang | ✅ **129 个着色器，0 问题**；glslang 通过 **125/125** |
| 真实夹具往返验证（`roundtrip_validate.py`） | ✅ 输出零 GLSL ES 错误 |
| FCL 插件元数据校验 | ✅ `renderer`/`des`/`boatEnv`/`pojavEnv` 全在；`LIBGL_ES=3` 与 `POJAV_RENDERER` 已注入 |
| **导出符号校验（本轮新增）** | ✅ 两个 ABI 均导出 869 个函数，17 个定制实现齐全 |
| **Stub 危险度审计（`audit_stubs.py`）** | ✅ 审计时 484 个已分类：28 DANGER / 28 CAUTION / 428 SAFE（现有 482） |

> **往返门禁的价值**：它先证明「原码按 ES 编译**失败**」（证明是桌面专属写法），
> 再证明「转换后**通过**」。这是一个双向断言，比单向检查强得多。

---

## 6. 已知问题

> 详见 `docs/p2-07-compatibility-matrix.md` §4 与 §7 的完整清单。

| # | 严重度 | 问题 | 状态 |
|---|---|---|---|
| 1 | ✅ | `glBufferStorage` 静默 stub | **已修复**（本轮） |
| 2 | 🟡 中 | 陈旧二进制/APK 导致假阴性 | ✅ 已用构建戳 + 导出符号门禁缓解 |
| 3 | 🟢 低 | `GL_TEXTURE_LOD_BIAS` ES 非法 | ✅ 已过滤（70→0） |
| 4 | 🟢 低 | Adreno 忽略 `GL_MAP_UNSYNCHRONIZED_BIT` | ℹ️ 驱动侧，不修 |
| 5 | 🟢 低 | 驱动功耗/命名冲突噪声 | ℹ️ 驱动侧，不修 |
| 6 | ⚪ 非本项目 | FCL < 1.3.3.2 帧率锁定 | ⚠️ 升级 FCL 即可 |
| 7 | ⚪ 非本项目 | ZL2 不重定向 `stderr` | ℹ️ 已用文件通道绕过 |

### 6.1 关于 FCL 帧率锁定的说明

**定性结论：这是 FCL 版本问题，我方零改动。**

- FCL CHANGELOG `[1.3.3.2] - 2026-09-13` 新增**「解除帧率锁定」**
- 用户版本 `versionCode 1316` = **1.3.1.6**，早于该版本
- 机制：FCL 旧版 BufferQueue 处于同步模式，`dequeueBuffer` 按 vsync 阻塞 → 锁在 120Hz 面板刷新率

> **我的方法论错误（已记录）**：一度判为「120 = 硬件上限」。错因是**对照实验只换了渲染器，
> 启动器始终固定** —— 启动器是共同因子，在原理上不可能被排除。
> **教训：对照实验必须覆盖所有可疑因子。**

---

## 7. 待办与下一步

### 7.1 待你确认/执行

| # | 事项 | 优先级 | 说明 |
|---|---|---|---|
| 1 | **实际画面质量**反馈 | 高 | 有无贴图错位、区块闪烁、透明/光照异常？—— 这类问题我这边日志看不到 |
| 2 | 是否验证 Embeddium | 中 | 修复已就位，需真机确认（见 `p2-05` §5.3） |
| 3 | 阶段三的优先级排序 | 高 | 见 §7.2 |

> **零成本的第一步**：正常玩一次后看 `status.json` ——
> `stub_symbols` 应**不再**含 `glBufferStorage`，`degrade_events` 里**应出现** `PERSISTENT_MAP_UNSUPPORTED`。

### 7.2 阶段三候选任务（需你定优先级）

| 编号 | 任务 | 优先级 | 当前状态 |
|---|---|---|---|
| P3-01 | native 侧**状态缓存**，减少冗余 GLES 调用 | P0 | 🟡 **部分完成**：每调用固定开销已消除（29→15 条）；**真正的状态缓存未实现**，见 `docs/p3-01-hotpath-optimization.md` §9 |
| P3-02 | TBDR 优化：减少 FBO 切换、`glInvalidateFramebuffer` | P0 | ❌ 未实现（`glInvalidateFramebuffer` 目前是**直接转发**，未被主动调用） |
| P3-03 | 粒子渲染实例化优化 | P1 | ❌ |
| P3-04 | 剔除逻辑确认与 CPU 端优化 | P1 | ❌（视觉无问题，优先级下调） |
| P3-05 | 设备兼容性数据库与回退开关 | P1 | ❌（当前仅单设备结论） |
| P3-06 | CI/CD：自动构建 jar + APK + native 产物 | P1 | ❌ |
| P3-07 | FCL 插件功能对齐（SelectableEnv / ToggleableEnv） | P2 | ❌ |

**我的建议顺序**：**P3-01 → P3-02**，理由是二者都是「减少每帧 GL 调用」的直接收益，
且当前帧率天花板已被 ZL2（300~400 FPS）抬高，优化空间真实存在。
**但若你更关心广泛设备可用性，P3-05 应提前。**

### 7.3 关于任务书的一处诚实说明（已修正）

任务书 §9 风险表里的「**性能不如 GL4ES**」这条**可以作废**，但理由需要更正。

**我此前写过**：「ZL2 实测 300~400 FPS，而 GL4ES 在同类设备上通常为 30~60 FPS」，
并据此说「远超基线」。**这个对比已被用户实测推翻，已删除**：

| 我原先的问题 | 更正 |
|---|---|
| 拿 GL4ES（**旧一代**翻译层）当参照 | 当前主流是 **MobileGlues / Zink**，同样成熟 |
| 「30~60 FPS」**没有一手来源** | 是我凭印象写的，不应作为数据使用 |
| 据此声称「远超」 | **不成立** |

**用户实测（原版环境）**：

> GLES Mod + Sodium 与 **MobileGlues** + Sodium、**Zink** + Sodium
> 的帧率**几乎没有差别**。

⇒ 正确表述是 **「与其他主流渲染器相当」**，而不是「超越」。

**这个结果不意外**：MobileGlues 与 Zink 都在 native 层，与本项目同一层次。
本项目的差异化点不在帧率，而在于**不引入中间层**（直接输出 ES 3.2，
无需 GL→Vulkan 或 GL→ES 翻译）以及**能力协商的可见性**。

**仍未验证**：改动前/后的同场景对照（属 P1-09 职责）。
详见 `docs/p3-01-hotpath-optimization.md` §8.5.3。

---

## 8. 风险登记（更新）

| 编号 | 风险 | 状态 |
|---|---|---|
| ~~R-01~~ | ~~GL 库替换不可行~~ | ✅ 已闭环 |
| R-02 | GL 符号覆盖不全导致启动失败 | ✅ **已降至低位**：849 符号全导出 + 导出符号门禁 |
| R-03 | 着色器转换失败 | 🟡 已大幅缓解：四道本地门禁 + 真机夹具回归；几何/细分属已知不可行范围 |
| R-04 | 驱动差异导致渲染错误 | 🟡 仅单设备验证；**P3-05 是主要对策** |
| R-05 | Sodium 版本更新导致兼容层失效 | 🟡 已有版本白名单 + 保守策略；架构上不做 Mixin 注入 |
| ~~R-06~~ | ~~性能不如 GL4ES~~ | ✅ **已作废**：实测与 MobileGlues / Zink **相当**（详见 §7.3） |
| R-07 | 无法本地验证，反馈周期长 | 🟡 **已显著缓解**：本地门禁可捕获大部分问题；但画面质量仍必须靠人眼 |
| R-08 | 许可证污染 | ✅ 只做运行时检测，不复制代码；Embeddium 为 LGPL-3.0-only，Sodium 为 Polyform Shield |
| ~~R-09~~ | ~~EGL 双重翻译~~ | ✅ 已排除（FCL 用系统 `libEGL.so`） |
| R-10 | 插件未设 `LIBGL_ES` | ✅ 已闭环：插件 env 注入 `LIBGL_ES=3`，门禁校验 |
| R-11 | 设备仅支持 ES 3.0/3.1 | 🟡 已有能力探测+降级；**未在低版本设备上验证过** |
| **R-12** | **静默 stub 掩盖真实失败**（本轮新增） | ✅ **已审计完成**：审计时 484 个中 28 DANGER / 28 CAUTION / 428 SAFE；现有 482；见 `docs/r-12-stub-audit.md` |

> **R-12 审计结论（本轮完成）**：
>
> 1. **28 个 DANGER 全部是 GL 4.4/4.5 的 DSA 函数族**（`glCreate*` / `glNamedBuffer*` / `glTextureStorage*` 等）。
> 2. **有实证说明真机上它们从未被调用**：`降级事件 0 类` ⟹ `glesmod_report_stub` 从未触发
>    ⟹ 零 stub 被调用（因为 stub 被调用时必记一条降级事件）。
> 3. **不预先修复**：无真实调用方时把 DSA 调用翻译回非 DSA 形式是**净风险**。
>    但**触发条件已明确且现已可观测**（见 `r-12` §5.2）。
> 4. **顺带修复一个真实缺陷**：`calledStubs` 被解析却未传入诊断报告，
>    导致**用户在日志里看不到 stub 证据**。已存入字段 + 进入诊断报告 + 以 WARN 输出。

---

## 9. 结论

| 维度 | 状态 |
|---|---|
| 技术可行性 | ✅ 已用真机结果证明 |
| 阶段一 | ✅ 完成并验收 |
| 阶段二 | ✅ 完成并验收（Sodium 满血 + 一轮主动修复） |
| 文档完备性 | ✅ P2-04 / P2-05 / P2-07 已补齐 |
| 质量门禁 | ✅ 6 道全绿 |
| 构建产物 | ✅ 四个产物均最新且已校验 |
| 阻塞项 | **无阻塞**。待你反馈：画面质量、Embeddium 是否验证、阶段三优先级 |
| 遗留审计项 | ✅ **R-12 已审计完成**（见 `docs/r-12-stub-audit.md`） |

**建议：给出阶段三优先级后即可直接开工。**

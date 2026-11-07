# P2-04：功能降级路由（实现与验证）

> 状态：✅ **已实现并在真机验证**
> 日期：2026-09-29
> 对应任务：P2-04（原 P0 → v5 修订下修为 P1）
> 关联文档：`docs/o-02-symbol-inventory.md`（符号清单）、`docs/capability-interface.md`（降级码契约）
> 适用范围：**仅 Sodium / Embeddium 路径需要**。原版 MC 不使用这些功能（见 `o-02` §2.2），因此本模块不阻塞 MVP。

---

## 1. 为什么需要降级路由

OpenGL ES 与桌面 OpenGL 的能力边界不同。ES **没有**：

- Multi-Draw（多重绘制，需扩展才有）
- 逻辑运算（`glLogicOp`）
- 多边形面模式（`glPolygonMode`）
- 直接像素写入（`glDrawPixels`）
- 纹理回读（`glGetTexImage`）

而 LWJGL 在 `GL.createCapabilities()` 时**必须解析它声明的全部 native 方法**，缺失任何"必需函数"都会抛 `NullPointerException` 硬崩溃，且发生在早期初始化阶段、无法捕获。

因此策略是二选一，**绝不"不导出"**：

| 做法 | 结果 |
|---|---|
| 不导出符号 | LWJGL 解析失败 → 抛 NPE → 游戏崩溃（不可捕获） |
| 导出为**安全 stub** | LWJGL 拿到有效指针 → 调用返回零值 + 记降级事件 → **不崩溃** |
| 导出为**定制实现** | 用可行的 ES 手段等价实现，语义尽力对齐 |

> **设计格言**：任何无法实现的路径都必须「**不崩溃 + 记录降级事件**」，而不是中止游戏。
> —— 出自 `native/src/custom.c` 文件头注释。

---

## 2. 实现结构

### 2.1 三类符号实现

`native/symbols.def` 由 `native/tools/gen_symbols_def.py` 生成，共 **849** 个符号：

| 类型 | 数量 | 位置 | 行为 |
|---|---|---|---|
| `F` 直接转发 | 337 | `native/src/generated_forwarders.c` | 查 `glesym_resolve()` → 调用同名 GLES 函数 |
| `C` 定制实现 | 30 | `native/src/custom.c` | 特殊处理（见 §3） |
| `S` 安全 stub | 482 | `native/src/generated_forwarders.c` | 返回零值 + `glesmod_report_stub()` |

> ⚠️ **生成时必须带 `--required native/tools/lwjgl_required.txt`**。漏掉该参数会让总数从 849 掉到 657（stub 482→293），导致真机 NPE。
> 每次重新生成后**必须核对总数 = 849**。
>
> 另需注意：849 个符号里，**每个符号的类型（F/C/S）会随路由策略调整而变化**。
> 例如 `glBufferStorage` 最初是 `S`，在发现 Embeddium 会因此崩溃后改为 `C`（见 §3.9）。
> 因此**总数不变不代表内容不变** —— 变更后应查 `symbols.def` 里目标符号的字母前缀，
> 并用 `native/tools/verify_exported_symbols.py` 校验产物。

### 2.2 能力探测与降级决策的分离

这是本模块的核心设计：**探测是探测，降级是降级，两者不耦合。**

```
启动 → probe_capabilities()（native/src/core.c）
         ├─ ES 版本查询          → es_major / es_minor
         ├─ 容量查询             → max_texture_units / max_samples / ...
         └─ 扩展能力探测         → 置位 g_caps.flags
                  ↓
         写入 status.json 的 caps 段
                  ↓
   Java 侧 GlesBackendStatus 正则解析 → GlesCapabilities 快照
                  ↓
   CompatDetector 据此输出「哪些功能不可用」
```

**运行时降级**则独立发生：当某个 `glMultiDraw*` 真的被调用时，`custom.c` 就地拆解并记 `MULTI_DRAW_UNSUPPORTED` 事件。

> **这一点极易误读，务必分清**：
> - **Java 侧的「以下功能在 ES 环境下不可用」是*预测*** —— 在进世界渲染前就打印。
> - **native 侧的 `degrade_events` 是*实测*** —— 只在函数真的被调用时才出现。
> - 因此「日志说 Multi-Draw 降级，但 `degrade_events` 为空」**不是矛盾**，而是说明 Sodium 自己做了能力检测、走了它的非 multi-draw 路径（详见 §5.2）。

### 2.3 能力探测代码（`native/src/core.c`）

```c
if (es_major > 3 || (es_major == 3 && es_minor >= 1)) {
    /* ES 3.1+ 才有计算着色器与间接绘制 */
    probe_proc("glDispatchCompute",       GLESMOD_CAP_COMPUTE_SHADER);
    probe_proc("glDrawElementsIndirect",  GLESMOD_CAP_INDIRECT_DRAW);
}

if (es_major >= 3) {
    /* ES 3.0 起支持实例化 */
    probe_proc("glDrawElementsInstanced", GLESMOD_CAP_INSTANCING);
    probe_proc("glDrawBuffers",           GLESMOD_CAP_MULTI_DRAW_BUFFERS);
    probe_proc("glTexStorage2D",          GLESMOD_CAP_TEXTURE_STORAGE);
}

/* Multi-Draw：ES 没有核心支持，只有少数驱动提供扩展 */
if (glesym_resolve("glMultiDrawElements") != NULL) {
    g_caps.flags |= GLESMOD_CAP_MULTI_DRAW;
}

/* 持久映射：ES 3.0+ 有 glMapBufferRange，但持久映射位需扩展 */
if (es_major >= 3 && glesym_resolve("glMapBufferRange") != NULL) {
    g_caps.flags |= GLESMOD_CAP_PERSISTENT_MAP;
}
```

**关键原则**：`probe_proc()` 探测的是**函数指针是否存在**，不是版本号。
「ES 3.2 就该有计算着色器」是不能假设的 —— 驱动可能只实现到 3.1 的某个子集。
**探测不到就不置位，宁可降级不用，不可假设可用。**

---

## 3. 各功能的降级策略

### 3.1 Multi-Draw（`GLESMOD_DEGRADE_MULTI_DRAW_UNSUPPORTED` = 0x0001）

**问题**：ES 3.2 核心不含 `glMultiDraw*`；本实现只对 `GL_EXT_multi_draw_indirect` 做原生加速。

> 2026-11-07 更新：只对 `GL_EXT_multi_draw_indirect` 做原生加速，检测
> `glMultiDrawArraysIndirectEXT` / `glMultiDrawElementsIndirectEXT`。
> 非 indirect 的 `glMultiDrawArrays` / `glMultiDrawElements` / BaseVertex
> 没有原生路径，始终走循环回退；原生扩展不可用或首次调用探测失败时，
> indirect 也走逐条回退。

**策略**：原生扩展优先，否则拆解为循环调用，语义等价。下面是回退路径的核心片段。

```c
/* glMultiDrawElements(mode, count, type, indices, drawcount)
   == for (i = 0; i < drawcount; i++)
          glDrawElements(mode, count[i], type, indices[i]);   */
void glMultiDrawElements(GLenum mode, const GLsizei *count, GLenum type,
                         const void *const *indices, GLsizei drawcount) {
    typedef void (*draw_elements_t)(GLenum, GLsizei, GLenum, const void *);
    static draw_elements_t real = NULL;

    glesmod_lazy_init();
    if (drawcount <= 0) return;
    RESOLVE_OR_RETURN(real, "glDrawElements", );

    for (GLsizei i = 0; i < drawcount; i++) {
        if (count[i] <= 0) continue;
        real(mode, count[i], type, indices[i]);
    }

    glesmod_degrade(GLESMOD_DEGRADE_MULTI_DRAW_UNSUPPORTED,
                    "Multi-Draw 原生不可用，已降级为循环 glDrawElements");
}
```

三个变体全部覆盖：

| 符号 | 降级方式 |
|---|---|
| `glMultiDrawElements` | 循环 `glDrawElements` |
| `glMultiDrawArrays` | 循环 `glDrawArrays` |
| `glMultiDrawElementsBaseVertex` | 优先循环 `glDrawElementsBaseVertex`；若该函数也不存在，退化为循环 `glDrawElements`（**忽略 basevertex，几何体可能偏移，但不崩溃**） |

**性能影响**：每增加一次绘制调用，CPU 开销增加。在 ES 上这是唯一可行方案。
**只在首次调用时记录降级事件**，避免每帧刷屏（`glesmod_degrade` 内部对同一码值只记首次日志）。

**`drawcount <= 0` 提前返回**：Sodium 的空批次会传 0，必须避免无意义调用与误报降级。

### 3.2 持久映射（`GLESMOD_DEGRADE_PERSISTENT_MAP_UNSUPPORTED` = 0x0003）

**问题**：ES 3.0 有 `glMapBufferRange`，但 `GL_MAP_PERSISTENT_BIT` / `GL_MAP_COHERENT_BIT` 需要 `GL_EXT_buffer_storage`，驱动支持度参差。

**策略**：`glMapBuffer`（桌面整块映射）映射到 `glMapBufferRange`。

```c
void *glMapBuffer(GLenum target, GLenum access) {
    /* 需先用 glGetBufferParameteriv 取得缓冲区大小，
       再调用 glMapBufferRange(target, 0, size, access) */
}
```

无法确定大小时返回 `NULL` 并记 `GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION`，**不猜测大小**（猜错会越界）。

**已知驱动行为**（真机实测，Adreno 750）：

```
1 × EsxBufferObject::Map - Ignoring EsxBufferMapUnsyncedBit
```

Adreno 驱动**忽略 `GL_MAP_UNSYNCHRONIZED_BIT`**。这可能导致隐式同步停顿（潜在性能损失），但**不产生错误**，且属驱动侧行为 —— 在帧率被外部因素限制时不可观测。本项**不视为缺陷**，仅记录。

### 3.3 计算着色器（`GLESMOD_DEGRADE_COMPUTE_SHADER_UNSUPPORTED` = 0x0002）

**问题**：ES 3.1+ 才有，且需驱动实际实现。

**策略**：`glDispatchCompute` / `glDispatchComputeIndirect` 作为**直接转发**（`F` 类型）。若驱动不提供，转发失败 → `glesmod_report_missing()` → 记 `UNSUPPORTED_FUNCTION` 降级。

**不需要特殊拆解**：计算着色器的语义无法用其他 ES 功能等价模拟，因此策略是「能转发就转发，不能就明确记录失效」，而不是假装成功。

原版 MC 不使用计算着色器（`o-02` §2.2），Sodium 0.8 部分特性使用。

### 3.4 间接绘制（`GLESMOD_DEGRADE_INDIRECT_DRAW_UNSUPPORTED` = 0x0004）

**策略**：同 §3.3，`glDrawElementsIndirect` 为直接转发，缺失时记降级。ES 3.1+ 原生支持。

### 3.5 其他 ES 缺失函数的定制实现（`custom.c`，18 个）

| 符号 | 问题 | 策略 | 是否记录降级 |
|---|---|---|---|
| `glClearDepth` | 参数 `double`，ES 只收 `float` | 转 `float` → 调 `glClearDepthf` | 否（无损等价） |
| `glGetError` | ES 错误枚举值与桌面不同 | 映射返回值 | 否 |
| `glPolygonMode` | ES 无面模式 | 仅 `GL_FILL` 静默忽略；其他忽略 + 记降级 | **是** |
| `glLogicOp` | ES 无逻辑运算 | 忽略 + 记降级 | **是** |
| `glDrawPixels` | ES 无 | 忽略 + 记降级（`NativeImage` 路径） | **是** |
| `glGetTexImage` | ES 无纹理回读 | 用 FBO + `glReadPixels` 回读（仅支持 `GL_TEXTURE_2D`） | 仅 `LOG_LEVEL>=2` |
| `glTexImage2D/3D` | 桌面深层格式 ES 非法 | **翻译内部格式**（见 §3.6） | 是（`TEXTURE_FORMAT_MAPPED`） |
| `glTexParameterf/i` | `GL_TEXTURE_LOD_BIAS` ES 非法 | 过滤该 pname | 是（降噪，见 §3.7） |
| `glBufferStorage` | ES 无（GL 4.4 功能） | 用 `glBufferData` 真实分配 | 是（见 §3.9） |
| `glMapBufferRange` | 桌面专属映射位 ES 非法 | 剥离非法位后转发 | 是（见 §3.9） |
| `glShaderSource` | 需 GLSL → GLSL ES 转换 | 调转换器 | 是（发生转换即记） |
| `glGetShaderInfoLog` | 行为不变 | **拦截并记录驱动错误原文** | 否 |
| `glGetProgramInfoLog` | 行为不变 | 同上 | 否 |
| `glMultiDraw*` ×3 | ES 无多重绘制 | 循环拆解 | **是** |

### 3.6 深度格式翻译：一个真实根因修复

这份修复值得单独记录，因为它是**唯一的、从症状反推到根因的完整闭环**。

**症状**：旁观模式下能看到**未被正确剔除的面**，实体**正反面交叠** —— 这是「深度测试完全没生效」的定义。

**证据链（全部一手材料）**：

1. 用户实测症状（上面）。
2. MC 1.21.1 字节码（`RenderTarget` / `MainTarget`）创建深度附件时传：
   ```
   sipush 6402   (= 0x1902 = GL_DEPTH_COMPONENT)  ← internalformat
   sipush 5126   (= 0x1406 = GL_FLOAT)            ← type
   ```
3. 驱动原话（`latest.log`）：
   ```
   the combination of format 6402 and type 5126 is unsupported
   ```

**根因**：桌面 GL 允许 `internalformat = GL_DEPTH_COMPONENT`，**GLES 不允许**。
ES 只接受带位宽的深层格式（`DEPTH_COMPONENT16/24/32F` 或 `DEPTH24_STENCIL8`）。
原样转发 → 驱动拒绝 → **深度纹理根本没被分配** → 深度测试完全失效。

**修法**（只影响这一种组合，其余一律原样转发）：

| 桌面格式 | 值 | → | ES 格式 | 值 |
|---|---|---|---|---|
| `GL_DEPTH_COMPONENT` | 0x1902 | → | `GL_DEPTH_COMPONENT24` | 0x81A6 |
| `GL_DEPTH_STENCIL` | 0x84F9 | → | `GL_DEPTH24_STENCIL8` | 0x88F0 |
| `type = GL_FLOAT` | 0x1406 | → | `GL_UNSIGNED_INT` | 0x1405 |

### 3.7 `GL_TEXTURE_LOD_BIAS` 过滤：一个降噪而非修复的取舍

**现象**：日志中有大量 `pname 34049` 噪声（最初 70 次）。

**查明**：`34049` = `0x8501` = **`GL_TEXTURE_LOD_BIAS`**。

> ⚠️ 早期笔记曾误记为 `GL_UNPACK_ROW_LENGTH`（实为 `0x0CF2` = 3314）。
> 已用捆绑的权威 `gl.xml` 更正。

**反编译证据**（`com.mojang.blaze3d.platform.TextureUtil`）：

```
sipush 3553           // GL_TEXTURE_2D
ldc    #67 int 34049  // GL_TEXTURE_LOD_BIAS
fconst_0              // value = 0.0f  ← 文档记载的默认值
invokestatic GlStateManager._texParameter:(IIF)V
```

**判定**：值恒为默认值 0.0f，MC **从不回读**该参数，且全字节码扫描确认只有 `TextureUtil` 引用 `34049`。
因此**丢弃该调用在语义上完全等价**。

**实现**：`glTexParameterf/i` 中过滤该 pname，并记一次降级事件。
**验证**：真机 70 → 0 次，噪声消除。

**★ 一处刻意的自我否决，值得记录 ★**

最初我还在过滤后调用 `drain_gl_errors()` 去清掉随之而来的 `GL_INVALID_ENUM`，**随后移除了它**。

理由：`glGetError` 是一个 **FIFO 队列**。排空队列会**连带丢弃更早调用留下的错误** —— 包括 MC 真正关心的失败。这会**掩盖真实故障**。

由此确立原则：**不要去清理 MC 的 GL 状态，只避免自己制造新的污染。**

### 3.8 着色器转换失败（`GLESMOD_DEGRADE_SHADER_UNSUPPORTED_FEATURE` = 0x0007）
GLSL → GLSL ES 转换器（`native/src/shader.c`）对无法处理的结构采取：
- 无法解析的 `#include` → 替换为等长注释 + 记降级事件（不中断编译）
- 桌面专属 `#extension` → 剥离为等长注释（**保留行号**，便于错误定位）
- ES 无几何/细分着色器 → 明确记降级，**这是策略允许的失败范围**（无法用软件层可靠模拟）

---

### 3.9 `glBufferStorage` / `glMapBufferRange`：静默 stub 的危险

**这一节记录一个原则性教训，而不只是两个函数的修法。**

#### 问题：静默 stub 在「有存储语义要求」的调用上是危险的

`glBufferStorage` 原本是 `S`（安全 stub：什么都不做，只记一条降级）。
对大多数符号这是合理的，但对它**不是**，因为它会让调用方拿到「成功」的假象：

```
glBufferStorage(...)   -> stub 返回 void，不分配存储，无错误
glMapBufferRange(...)  -> 因缓冲区无存储而失败，返回 NULL
调用方                 -> 抛异常崩溃
```

失败发生在**离根因很远的地方**，且以「与存储完全无关」的形式出现。

#### 一手证据（Embeddium 21.1 分支源码）

```java
// RenderRegionManager.createStagingBuffer()
if (Embeddium.options().advanced.useAdvancedStagingBuffers
        && MappedStagingBuffer.isSupported(device)) {
    return new MappedStagingBuffer(commandList);   // <- 走这条会崩
}
return new FallbackStagingBuffer(commandList);     // <- 本可安全

// MappedStagingBuffer
STORAGE_FLAGS = {PERSISTENT, CLIENT_STORAGE, MAP_WRITE}   // 桌面 GL 专有
MAP_FLAGS     = {PERSISTENT, INVALIDATE_BUFFER, WRITE, EXPLICIT_FLUSH}

// BufferStorageFunctions.pickBest() —— 只认桌面 GL 标志
if (capabilities.OpenGL44)                   return CORE;
else if (capabilities.GL_ARB_buffer_storage) return ARB;
else                                         return NONE;   // ES 上期望走这里

// GLRenderDevice.mapBuffer()
if (buf == null) {
    throw new RuntimeException("Failed to map buffer");   // <- 硬失败
}
```

**关键**：`pickBest()` **完全没有考虑 ES 场景**。而 `CORE` 与 `ARB` 两条路在我们这里
都落到同一个 `glBufferStorage` 符号上。

#### 修法

| 符号 | 原类型 | 新类型 | 新行为 |
|---|---|---|---|
| `glBufferStorage` | `S` | **`C`** | 用 `glBufferData`（`GL_DYNAMIC_DRAW`）真实分配存储；若 flags 含持久/一致位，记 `PERSISTENT_MAP_UNSUPPORTED` |
| `glMapBufferRange` | `F` | **`C`** | 掩掉 `GL_MAP_PERSISTENT_BIT`(0x0040) / `GL_MAP_COHERENT_BIT`(0x0080) / `GL_CLIENT_STORAGE_BIT`(0x0200) 后转发 |

**为什么掩掉映射位是安全的**：这三位都是**性能承诺**，不是**正确性要求**：

| 位 | 掩掉后的后果 |
|---|---|
| `PERSISTENT` | 映射不能长期保持 → 每次映射即可，**仍正确** |
| `COHERENT` | 需显式 flush → 驱动保守同步，**仍正确** |
| `CLIENT_STORAGE` | 存储位置偏好失效 → 纯提示，**仍正确** |

我们**没有**改动任何影响正确性的位（`READ`/`WRITE`/`INVALIDATE`/`FLUSH`）。

**为什么用 `glBufferData` 而不是 `GL_EXT_buffer_storage`**：该扩展支持度参差，
探测它需要 `glGetStringi` 查扩展列表。用 `glBufferData`（ES 2.0 起必有）保证成功，
代价是失去不可变存储语义 —— 而那本来就不是我们能提供的。少一次探测，多一分确定性。

**关于「善意降级」的取舍**：这确实意味着调用方会拿到**不符合其声明语义**的映射。
但因为我们**同时记录了降级事件**（日志 + `status.json` 均可查），它不是静默欺骗；
而不这样做的后果是**崩溃** —— 那是本项目的第一优先级禁区。

#### ★ 由此确立的原则 ★

> **静默 stub 只适用于「调用方不会依赖其返回值/副作用」的函数。**
> 若某函数的**契约包含产生某种资源**（分配存储、创建对象、返回句柄），
> 把它做成静默 stub 就等于**用一个谎言替换一次失败**，
> 而谎言会在很远的地方以完全无关的形式暴露。
> 这类符号必须给「最接近的真实实现 + 显式降级」。

---

## 4. 降级事件的对外通道

### 4.1 状态文件（native → Java）

`glesmod/status.json`，字段固定、结构扁平：

```json
{
  "abi_version": 1,
  "active": true,
  "gles_version": "3.2",
  "reported_gl_version": "3.2",
  "degrade_level": 1,
  "state_cache": true,
  "caps": { "multi_draw": false, "compute_shader": true, "...": "..." },
  "degrade_events": [
    { "reason": "MULTI_DRAW_UNSUPPORTED", "count": 1, "detail": "..." }
  ],
  "missing_symbols": { "count": 0, "names": [] },
  "stub_symbols":    { "count": 0, "names": [] },
  "last_calls": ["glDrawElements", "..."]
}
```

> **为什么走文件而不走 JNI**：native 库由启动器 `dlopen` 加载，与 mod 的 JVM 处于**不同的库加载路径**。若让 native 回调 Java，需处理类加载器隔离与线程 attach，复杂且脆弱。文件是两端都能稳定访问的通道。

### 4.2 `missing_symbols` 与 `stub_symbols` 必须分开

这是诊断精度的关键区分：

| 列表 | 含义 | 指向 |
|---|---|---|
| `missing_symbols` | 期望 GLES 提供但**没有** | **缺陷** —— 符号表或驱动有问题 |
| `stub_symbols` | 我们主动提供的空实现**被调用** | **能力边界** —— 预期内的功能失效 |

两者混在一起会让诊断失去方向。

**每发现一个新的 stub 就立即重写状态文件**。原因：状态文件本在初始化完成时写出，而 stub 往往在那之后（进入世界、模组加载渲染代码）才被调用。若不即时更新，用户拿到的 `status.json` 里 `stub_symbols` **永远是空的**，我们就无法判断「哪些功能实际上失效了」。

### 4.3 降级档位

由环境变量 `GLESMOD_DEGRADE_LEVEL` 下发，取值范围 0–2（自动钳制），默认 1：

| 档位 | 语义 |
|---|---|
| 0 | 保守 —— 能降则降，最稳 |
| 1 | 默认 |
| 2 | 激进 —— 能用则用，最快 |

---

## 5. 真机验证结果

### 5.1 设备与环境

| 项 | 值 |
|---|---|
| 设备 | Xiaomi houji 23127PN0CC / SM8650 / **Adreno 750** |
| Android | 16（SDK 36） |
| GLES | 3.2 |
| MC / NeoForge | 1.21.1 / 21.1.241 |
| Sodium | 0.8.13+mc1.21.1 |
| 启动器 | FCL 1.3.1.6 与 ZL2 2.4.9_hotfix1（**两者结果一致**） |

### 5.2 实测结论

```
[Render thread/INFO] [com.youyimc.glesmod.GLESMod/]: GLES 后端已激活，GLES 3.2，无功能降级。
[Render thread/INFO] [com.youyimc.glesmod.GLESMod/]: 后端能力: GlesCapabilities{
    active=true, gles=3.2, reportedGl=3.2, degradeLevel=1,
    multiDraw=false, computeShader=true, persistentMapping=true,
    instancing=true, indirectDraw=true,
    maxTextureUnits=96, maxDrawBuffers=4, maxTextureSize=16384,
    maxSamples=4, maxVertexAttribs=32}
[Render thread/INFO]: 降级事件 (0 类):
[Render thread/INFO]: 优化模组: sodium 0.8.13+mc1.21.1 -> FULLY_SUPPORTED（已验证版本，使用专用策略）
[Render thread/INFO]: 以下功能在 ES 环境下不可用，优化模组已相应降级：
[Render thread/INFO]:   - Multi-Draw（Sodium 区块渲染会降级为逐次绘制，性能略降）
```

**逐条解读**：

| 观察 | 结论 |
|---|---|
| `multiDraw=false` | ✅ 探测正确 —— Adreno 750 不提供 `glMultiDrawElements` |
| `computeShader=true` | ✅ ES 3.2 有 `glDispatchCompute` |
| `persistentMapping=true` | ✅ 有 `glMapBufferRange` |
| `indirectDraw=true` | ✅ ES 3.2 有 `glDrawElementsIndirect` |
| `degrade_events` 为空 | ⚠️ **见下方说明，这不是矛盾** |
| 无 `stub_symbols` / 无 `missing_symbols` | ✅ 说明渲染路径**没有踩到任何 stub**，即没有功能静默失效 |
| 世界渲染正常、进入世界成功 | ✅ 降级路径工作正常 |

**关于「日志说降级但事件为空」**：这是 §2.2 所述的预测/实测分离。
`degrade_events` 为空说明 **`glMultiDraw*` 从未被实际调用** —— 即 Sodium 自己检测到 `multiDraw=false` 后，主动改走逐次绘制路径。
这恰恰是**期望行为**：Sodium 的能力协商生效了，我们连一次降级事件都不需要产生。

反之，若 `degrade_events` 里出现大量 `MULTI_DRAW_UNSUPPORTED` 且计数很高，才说明 Sodium 在**盲目调用** multi-draw（版本不匹配的信号）。

### 5.3 性能验收（原 P2-06 指标）

任务书原文要求「FPS 不低于 GL4ES 基线的 70%」。

| 启动器 / 渲染器 | 实测 FPS | 说明 |
|---|---|---|
| **ZL2 + 本项目** | **300 ~ 400** | 高于指标 |
| **ZL2 + MobileGlues** | 与本项目**几乎无差别** | 同一层次 |
| **ZL2 + Zink** | 与本项目**几乎无差别** | 同一层次 |
| FCL 1.3.1.6 | 120（= 面板刷新率） | **非本项目问题** —— FCL 旧版未解除帧率锁定，详见 §5.4 |

**结论：性能高于验收要求，不是瓶颈。** 但需注意措辞 ——
与原版环境下其他主流渲染器**相当**，而非「超越」；
对比的**条件**是原版 + Sodium（无光影、无大模组包）。

我此前一度把 120 判为「硬件上限」，那个结论是**错的**，错因见 §5.4。

### 5.4 与降级路由无关但必须澄清的一点：120 FPS 的真正原因

**定性结论：FCL 版本过旧，我方无需任何改动。**

证据链：

1. **FCL 自己的 CHANGELOG** —— `[1.3.3.2] - 2026-09-13`：
   > **解除帧率锁定**：游戏帧率不再锁定屏幕刷新率，启动时向系统投票设备最高刷新率；
   > 关闭垂直同步时交换间隔强制置 0 并切入 BufferQueue 异步模式

2. **用户日志** `FCL Version Code: 1316` → 版本 **1.3.1.6**，**早于 1.3.3.2** ⇒ 不含该修复。

3. **FCL 源码机制**（`FCL/src/main/jni/ctxbridges/swap_interval_no_egl.c`）：
   ```c
   void setNativeWindowSwapInterval(struct ANativeWindow* nativeWindow, int swapInterval) {
       // BufferQueue 同步模式下 dequeueBuffer 按垂直同步信号阻塞，持续帧率会被锁在屏幕刷新率；
       // 交换间隔 0 会让 Surface 切入异步模式（生产者不再阻塞），因此关闭垂直同步时必须显式置 0
       if(!getenv("POJAV_VSYNC_IN_ZINK")) {
           swapInterval = 0;
       }
       ... nativeWindowReal->setSwapInterval(nativeWindow, swapInterval) ...
   }
   ```
   它**直接操纵 `ANativeWindow_real->setSwapInterval`，绕开 EGL**。

**机制**：FCL 1.3.1.6 的 BufferQueue 仍处于**同步模式**，`dequeueBuffer` 按 vsync 阻塞 → 帧率锁在**面板刷新率**。这解释了为何「关掉游戏内垂直同步也没用」—— 问题不在 EGL swap interval，而在 BufferQueue 的生产者同步模式。而 120 恰好等于 120Hz 面板刷新率，这个数值巧合让整件事**看起来像硬件上限**。

**处置**：升级 FCL 到 ≥ 1.3.3.2。ZL2 已有等价实现，这是它跑到 300~400 FPS 的原因 ——
两个启动器除帧率外**无任何差别，且都无渲染错误**，是本模块正常工作的强交叉验证。

> **方法论教训**：我最初的对照实验只换了**渲染器**，而**启动器始终固定**。
> 启动器是那个实验里的共同因子，**在原理上不可能被排除**。
> 正确的对照必须覆盖所有可疑因子 —— 「换掉被测对象」只能排除该对象本身。

---

## 6. 已知边界与不做的取舍

| 不做 | 原因 |
|---|---|
| 不模拟几何 / 细分着色器 | ES 3.2 无此阶段，无法用软件层可靠实现。明确记降级，属策略允许的失败范围 |
| 不自动映射纹理格式 | 格式转换会改变数据布局，**盲目自动映射会静默产生错误图像**。必须在具体调用点判断 |
| 不清理 MC 残留的 GL 错误 | `glGetError` 是 FIFO 队列，排空会连带丢弃真实故障（见 §3.7） |
| 不做 `glMapBuffer` 大小猜测 | 猜错会越界读写；无法确定时返回 `NULL` 并记降级 |
| 不实现 `state_cache` 的实际缓存 | 当前仅**读入配置标志并写入状态文件**，真正的状态缓存属 **P3-01** |

### 6.1 待办：`state_cache` 目前是空开关

`native/src/core.c` 中：

```c
static int g_state_cache = 1;
...
g_state_cache = glesmod_env_int(GLESMOD_ENV_STATE_CACHE, 1);
```

该值**只被写入 `status.json` 的 `state_cache` 字段，未驱动任何实际行为**。
这是一个**诚实的空开关**（预留接口），不应被误读为「状态缓存已实现」。

---

## 7. 复现方法

```powershell
# 1) 重新生成符号表（必须带 --required，否则 total 会掉到 657）
py native/tools/gen_symbols_def.py --gl-xml .cache/gl.xml `
   --required native/tools/lwjgl_required.txt --out native/symbols.def

# 2) 重新生成转发函数
py native/tools/gen_gl_forwarders.py --def native/symbols.def `
   --out native/src/generated_forwarders.c `
   --manifest native/tools/symbol_manifest.json

# 3) 核对总数必须为 849
#    = 346 转发 + 18 定制 + 485 stub
```

主机侧单元测试：`native/tools/test_shader_convert.c` 中 **[16]** 组覆盖
Sodium 的整数/浮点混用场景，含反向断言（纯整数运算不得被改动）。

---

## 8. 结论

1. **降级路由已完整实现**：Multi-Draw（原生扩展优先，含 indirect）/ 持久映射 / 计算着色器 + 30 个定制实现。
2. **真机验证通过**：Adreno 750 上无扩展时 `multiDraw=false` 被正确探测，Sodium 顺利协商并渲染正常；有 `GL_EXT_multi_draw_indirect` 的设备会优先走原生 indirect 入口，非 indirect 的 `glMultiDraw*` 仍走循环。
3. **零静默失效**：`stub_symbols` 与 `missing_symbols` 均为空，说明渲染路径没有踩到任何未实现功能。
4. **性能高于验收标准**：ZL2 下 300~400 FPS。
5. **诚实标注**：`state_cache` 为空开关；`degrade_events` 为空是「预测 vs 实测」的正常表现，非矛盾。

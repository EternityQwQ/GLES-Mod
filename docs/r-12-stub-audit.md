# R-12：安全 stub 危险度审计报告

> 状态：✅ **审计完成**
> 日期：2026-09-29
> 触发：P2-05 评估中发现 `glBufferStorage` 静默 stub 会导致 Embeddium 硬崩溃
> 关联：`docs/p2-05-embeddium-assessment.md` §4、`docs/p2-04-degrade-routing.md` §3.9
> 工具：`native/tools/audit_stubs.py`（无参，自行定位 `native/symbols.def`）

---

## 1. 审计的起因与判据

`glBufferStorage` 的缺陷揭示了一类问题模式：

```
glBufferStorage(...)   -> 静默 stub：不分配存储，返回 void，无错误
glMapBufferRange(...)  -> 因缓冲区无存储而失败，返回 NULL
调用方                 -> 抛异常崩溃
```

失败发生在**离根因很远的地方**，且以「与存储完全无关」的形式出现。由此确立判据：

> **静默 stub 只适用于「调用方不会依赖其返回值/副作用」的函数。**
> 若函数的契约包含产生某种资源（分配存储、创建对象、返回句柄），
> 做成静默 stub 就等于**用一个谎言替换一次失败**。

`glBufferStorage` 已修复，但**其余 484 个 stub 尚未审计**。本报告补上。

> 2026-11-07 更新：`glMultiDrawArraysIndirect` / `glMultiDrawElementsIndirect`
> 已从 `S` 改为 `C`（原生扩展优先，否则逐条 indirect 回退），当前 stub 总数为
> **482**。下面的分类结果基于当时的 484 个 stub，其余结论不变。

---

## 2. 审计结果

```
Stub 总数: 484

  DANGER  (产生资源，stub 会导致延迟失败) : 28
  CAUTION (返回值被当作数据使用)          : 28
  SAFE    (纯状态设置 / 调用方不依赖)      : 428
```

### 2.1 DANGER 清单（28 个）

**全部属于桌面 GL 4.5 的 DSA（Direct State Access）函数族**，分四类：

| 类别 | 符号 | 为何危险 |
|---|---|---|
| **DSA 对象创建**（8 个） | `glCreateBuffers`、`glCreateFramebuffers`、`glCreateQueries`、`glCreateRenderbuffers`、`glCreateSamplers`、`glCreateTextures`、`glCreateTransformFeedbacks`、`glCreateVertexArrays` | 创建对象并**返回句柄**；stub 后按「成功」返回但句柄为 0（无效） |
| **DSA 缓冲存储**（7 个） | `glNamedBufferStorage`、`glNamedBufferData`、`glMapNamedBuffer`、`glMapNamedBufferRange`、`glFlushMappedNamedBufferRange`、`glUnmapNamedBuffer` | 与 `glBufferStorage` **完全同类**：分配存储/返回映射指针 |
| **DSA 纹理存储**（10 个） | `glTextureStorage1D/2D/2DMultisample/3D/3DMultisample`、`glTextureSubImage1D/2D/3D`、`glTextureBuffer`、`glTextureBufferRange` | 分配纹理存储/上传数据；stub 后纹理内容为空 |
| **其它**（3 个） | `glTexStorage1D`、`glNamedRenderbufferStorage`、`glNamedRenderbufferStorageMultisample`、`glVertexArrayElementBuffer` | 分配存储 / 绑定索引缓冲 |

> **一个共同特征**：这 28 个**全部是 GL 4.4/4.5 才引入的特性**。ES 3.2 核心完全没有它们。

### 2.2 CAUTION 清单（28 个）

返回值的语义是「查询结果」，stub 返回 0 可能被误读为「真实值就是 0」。
例：`glGetNamedBuffer*`、`glGetTexture*`、`glGetVertexArray*`、`glGetFramebuffer*`、
`glGetQuery*`、`glGetSynciv`、`glGetProgramResource*`、`glGetInternalformativ` 等。

**风险低于 DANGER**：返回值被误读最多导致**功能判断错误**，不会像 DANGER 那样
在后续某个无关调用处崩溃（除非调用方据此走了错误分支）。

---

## 3. ★ 关键实证：这些 stub 在真机上从未被调用 ★

### 3.1 结论

**本次真机会话中，没有任何一个 stub 被调用过。**

### 3.2 一手证据

用户日志（`latest(5).log`）原文：

```
加载成功      : true
ABI 版本      : 1
GLES 版本     : 3.2
降级事件 (0 类):
缺失符号 (0):
```

### 3.3 推理链（为什么「降级事件 0 类」能证明「零 stub 被调用」）

1. 读 `native/src/core.c` 中 `glesmod_report_stub()` 的实现：
   它**每发现一个新的 stub 就调用一次** `glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION, ...)`；
2. 该函数只对同一码值记**首次**日志，但**计数一定会累加**；
3. 因此：**只要有任何 stub 被调用，`降级事件` 段落就不可能为空**；
4. 实测为空（0 类）⟹ **没有任何 stub 被调用**。∎

### 3.4 为什么这符合预期

| 理由 | 依据 |
|---|---|
| 原版 MC 只用到 GL 3.0 | `docs/o-02-symbol-inventory.md` §2.1（未引用 `GL31`/`GL32`） |
| Sodium 0.8.13 会做能力探测、主动避开不支持的功能 | 真机 `multiDraw=false` 被正确协商，且 `degrade_events` 为空 |
| 28 个 DANGER 全是 GL 4.4/4.5 特性 | ES 3.2 核心不存在；DSA 需 GL 4.5 |

**即：没有任何一方会去调用它们。** 这解释了为何真机一路跑通。

---

## 4. ★ 顺带发现并修复的缺陷：stub 证据传不出去 ★

### 4.1 缺陷

审计过程中我想查「哪些 stub 被调用过」，但**在日志里找不到任何该项证据**。
追查 `src/main/java/com/youyimc/glesmod/backend/GlesBackendStatus.java` 后发现：

| 事实 | 后果 |
|---|---|
| `calledStubs` **被正则解析出来了** | 数据其实是有的 |
| 但**没有存为字段** | 出了 `parse()` 就丢了 |
| 因此 `diagnosticReport()` 里**从不显示它** | 用户回传的诊断报告里看不到 |
| 只在解析时打了一条 WARN | 且用户日志里实际看不到 |

**⇒ 用户回传的日志里根本看不到 stub 证据，排查无从下手。**

> **这与之前记录过的教训是同一类错误**：
> **给用户用的诊断信息，必须走用户实际收集的那个通道。**

### 4.2 修复

| 改动 | 位置 |
|---|---|
| `calledStubs` 存为 `final` 字段 + 新增 `getCalledStubs()` | `GlesBackendStatus.java` |
| `diagnosticReport()` 新增「被调用的空实现 (N):」段 | 同上 |
| 非空时打 **WARN**（不是 INFO），并提示提交 issue | `GLESModClient.java` |

**为什么与「缺失符号」分段列出**：两者语义不同，混在一起会让诊断失去方向：

| 列表 | 含义 | 指向 |
|---|---|---|
| `missing_symbols` | 期望 GLES 提供但**没有** | **缺陷** |
| `stub_symbols` | 我们主动提供的空实现**被调用** | **能力边界** |

---

## 5. 处置结论：DANGER 项**不预先修复**，但有明确触发条件

### 5.1 为什么不预先修

把 28 个全部改成定制实现，而真机上它们**一次都不会被调用** ——
这是把风险从「理论存在」换成「引入新 bug 的机会」，违反 YAGNI。

更具体的理由：这 28 个全是 DSA 函数，正确的定制实现需要把 DSA 调用
**翻译回非 DSA 形式**（如 `glCreateBuffers` → `glGenBuffers`，
`glTextureStorage2D` → `glTexStorage2D`）。这是有实质工作量的改造，
且每条都有自己的一套语义陷阱 —— **在没有真实调用方的情况下做，几乎必然引入新问题。**

### 5.2 触发条件（何时必须修）

**当且仅当**以下任一情况出现，才需要按 `glBufferStorage` 的方式修复对应符号：

1. 真机 `status.json` 的 `stub_symbols` 里出现了 DANGER 清单中的符号；
2. 或用户日志里出现「被调用的空实现」段落且含 DANGER 项。

此时说明**确有调用方走到了它**，修才有意义，也才有验证手段。

**好消息**：§4 的修复让这个判据**从「不可观测」变成了「可观测」**。

### 5.3 若将来要修，`glBufferStorage` 的修法是模板

| 步骤 | 做法 |
|---|---|
| 1 | 找到「最接近的真实实现」（如 `glBufferData` / `glGenBuffers`） |
| 2 | 用它真实完成「产生资源」这件事 |
| 3 | 对无法满足的语义**显式记降级事件**，而不是假装成功 |
| 4 | 确认符号类型从 `S` 变为 `C`，并重新生成 `symbols.def` + `generated_forwarders.c` |
| 5 | 核对总数仍为 **849**，且 `gen_gl_forwarders.py` 的「定制实现清单」与 `custom.c` 一一对应 |
| 6 | 跑 `native/tools/verify_exported_symbols.py` 确认产物已导出 |

---

## 6. 交付物

| 文件 | 作用 |
|---|---|
| `native/tools/audit_stubs.py` | 无参审计脚本，输出 DANGER/CAUTION/SAFE 分类 |
| `GlesBackendStatus.getCalledStubs()` | 让 stub 证据能被 Java 侧消费 |
| `GLESModClient` 的 WARN 段落 | 让 stub 证据出现在**用户实际收集的日志**里 |
| 本报告 | 审计结论与触发条件 |

### 复现方法

```
py native/tools/audit_stubs.py
```

---

## 7. 结论

1. **审计完成**：484 个 stub 中，**28 个 DANGER**（全为 GL 4.4/4.5 的 DSA 族）、
   **28 个 CAUTION**、**428 个 SAFE**。
2. **有实证说明这些 stub 在真机上从未被调用**：`降级事件 0 类` ⟹
   `glesmod_report_stub` 从未触发 ⟹ 零 stub 被调用。
3. **DANGER 项不预先修复**：无真实调用方时改造 DSA 翻译层是净风险。
   但**触发条件已明确且现已可观测**。
4. **顺带修复一个真实缺陷**：`calledStubs` 被解析却未传出，
   导致用户在日志里看不到 stub 证据。已存入字段、进入诊断报告、并以 WARN 输出。
5. **R-12 状态**：从「未审计」→ **已审计，结论明确，且有可观测的触发条件**。

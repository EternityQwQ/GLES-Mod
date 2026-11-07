# 测试夹具来源说明 / Fixture Provenance

本目录的 `*.vert` / `*.frag` / `*.glsl` 是**回归测试夹具**，供 `roundtrip_validate.py`
把「转换器 → glslang GLSL ES 类型检查」这条链路跑通。

**它们不是产品代码，也不会随任何发布产物分发。** 但因为其中部分内容**源自第三方着色器**，
必须逐项说明来源与授权状态。

---

## 分类

### A. 原创最小复现件（本项目版权）

针对具体转换器缺陷**独立编写**的最小样例。虽受真机错误信息启发，
但代码是原创的，只保留触发缺陷所必需的最小构造。

| 文件 | 覆盖的缺陷 |
|---|---|
| `bsl_fsuffix.frag` | `f` 后缀浮点字面量（`0.25f`）引发的三处类型错误 |
| `bsl_like_deferred.frag` | 全局初始化器与 int/float 混算的综合场景 |
| `int_float_mul.frag` | `float * uniform int`（情形 H） |
| `iris_interface_block.frag` | Iris 注入的 `layout(std140) uniform iris_Fog` 全局接口块 |
| `iris_layout_out.frag` | `layout(location=N) out` 输出声明 |
| `sodium_blit_bitwise.vert` | `gl_VertexID & 1` 的整数位运算被误转为浮点 |
| `veil_pow_int_exponent.vert` | `pow(vecN, int)` —— 向量底数 + 整数指数无重载 |
| `error_directive_quotes.vert` | 非活动 `#else` 分支中的 `#error "..."`；Mali 词法器拒绝引号 |

**授权**：LGPL-3.0-or-later（同本项目）。

---

### B. 真机产物片段（来源为第三方光影包）

从**我们自己设备**上 Iris 导出的着色器（`patched_shaders/`）中摘出的**最小片段**。
导出文件本身是 Iris 的编译产物，其**内容源自对应光影包**。

| 文件 | 来源 | 上游许可证 |
|---|---|---|
| `bsl_assign_int_to_float.vert` | BSL Shaders `shadow.glsl` | 见下方"授权疑点" |
| `bsl_float_eq_int.frag` | BSL `020_terrain_translucent.fsh` | 同上 |
| `bsl_global_multiuniform.frag` | BSL `020_terrain_translucent.fsh` | 同上 |
| `bsl_int_ctor_div.vert` | BSL `018_terrain_solid.vsh` | 同上 |
| `bsl_ternary_int_float.vert` | BSL `031_hand_cutout.vsh` | 同上 |
| `bsl_user_texture2DShadow.frag` | BSL `007_basic.fsh` | 同上 |
| `sodium_blit_bitwise.vert` 的注释 | 真机 `native.log` | — |

> `bsl_*.frag` / `bsl_*.vert` **采用自描述命名**（`bsl_` 前缀）正是为了标明来源。

---

### C. 第三方着色器的**逐字副本** ⚠️

| 文件 | 来源 | 上游许可证 | 状态 |
|---|---|---|---|
| `sodium_chunk_0.8.13.vert` | Sodium 0.8.13 的区块顶点着色器（含 `#import` 展开后的 `fog.glsl` / `chunk_vertex.glsl` / `chunk_matrices.glsl`） | **Polyform Shield License 1.0.0**（**非 FOSS**） | **已从 git 排除** |

**处置**：该文件已加入 `.gitignore`，**不会进入仓库**。

**为什么这样做**：

1. **技术上不必要** —— `fetch_sodium_shaders.ps1` 可从上游随时重新获取：
   ```
   py native/tools/...   或   .\native\tools\fetch_sodium_shaders.ps1
   ```
2. **授权上不安全** —— Polyform Shield 不是开源许可证，**逐字副本的再分发**需要
   另行取得许可，而我们没有。
3. **它是重要的回归证据** —— 2026-09-28 那次真机崩溃正是它触发的，所以**保留在本地**
   用于测试，只是不公开发布。

**首次克隆后如需该用例**：运行 `native/tools/fetch_sodium_shaders.ps1`，
再把产物放成 `sodium_chunk_0.8.13.vert` 即可。缺失时 `roundtrip_validate.py`
会跳过它（夹具数量由 14 变 13），**不影响其它用例**。

---

## 授权疑点（待你决策）

### ⚠️ 关于 B 类（BSL 片段）

**BSL Shaders 未公开发布明确的开源许可证**（其分发页未附许可证文本）。
这意味着严格来说，其衍生片段的再分发需要获得作者许可。

**我的建议（三选一）**：

| 方案 | 做法 | 优点 | 缺点 |
|---|---|---|---|
| **① 改写为原创**（推荐） | 把 B 类改写成**结构等价但表达不同**的独立夹具 —— 例如把 `isRightHanded ? 1 : -1` 换成 `someFlag ? 1 : -1`，把 `weatherCol` 换成 `w` | 彻底消除疑点，测试价值不减 | 需逐个改写（7 个文件，工作量不大） |
| ② 保留 + 声明 | 保留现状，在上游许可澄清前于 README 标注来源 | 省事 | 仍有法律不确定性 |
| ③ 也排除 | 一并加入 `.gitignore` | 最保守 | 开源仓库失去最有价值的回归用例，别人难以复现问题 |

**为什么缺陷的"最小化"已经大幅降低了风险**：这些夹具只保留了触发类型错误所必需的
1~3 行构造（如一个三元表达式、一次 `==` 比较），既不含 BSL 的着色算法，也不含
其光影效果实现。这属于**为兼容性目的的必要引用**，但仍建议走方案 ① 以求稳妥。

**请你决定**（这是你的项目，我不替你拍板）。在你决定前，当前状态是：
**B 类保留在仓库中**，并在本文档与 `THIRD-PARTY-NOTICES.md` 中如实标注来源。

---

## 其它说明

- 夹具**只用于开发期回归**，`roundtrip_validate.py` 不会把它们打进任何产物。
- 复现完整测试需要你自己合法获得的 Minecraft 与光影包（见 `docs/test-guide.md`）。
- 若你认为本文件的分类有误，欢迎提 issue 指正。

---

*最后更新：2026-11-07*

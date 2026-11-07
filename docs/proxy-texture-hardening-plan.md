# Proxy texture 仿真收口计划（含可选 EGL/GLES 同源检查）

> 状态：✅ **Phase 1 已完成并在 Adreno 619 + Mali-G57 验证**
> 日期：2026-11-07
> 基线：`fix/angle-proxy-texture` / `5c395e1`
> 范围：GLES-Mod native 层 + v2 插件配置校验；不改 FCL，不改 NeoForge 模组
> 说明：本计划已随修复提交进入仓库（`faae3e9`），保留不删除。

---

## 1. 一句话结论

ANGLE 开关已经真机验证可用；下一步不是继续加功能，而是收口一处真正的 P1 工程债，外加一个可选的廉价防御：

1. **P1（必做）**：proxy 仿真从“全 target 声明、2D 实现”收窄为“已测试 target + 其他 target 安全降级”；
2. **P3（可选）**：给 `LIBGL_GLES` / `POJAVEXEC_EGL` 加一条同源警告。

推荐方案：**先做窄实现（2D + cube/rectangle），不做全 target 完整语义。**

> **2026-11-07 修正**：`LIBGL_GLES` 错配在 FCL 默认路径下**几乎不存在**——FCL 内置渲染器不设置它，GLES-Mod 的 v1/v2 插件配置也不设置它，v2 开关只设 `POJAVEXEC_EGL`。只有手动 env 或旧 PoC 残留才可能制造错配。因此 Phase 2 降为可选防御性检查，不阻塞本计划。

---

## 2. 当前基线

| 项 | 状态 |
|---|---|
| ANGLE 开关 | ✅ 真机验证可见、可启动 |
| proxy texture 2D 修复 | ✅ 已验证 |
| v2 配置 | ✅ 已进入 APK，编译产物门禁通过 |
| CI | ✅ APK / JAR / native libs 构建成功 |
| 签名密钥 | ⏳ 按用户要求暂不处理 |
| 完整兼容矩阵 | ⏳ 未逐项回归 |

当前实现的风险：

- `glesmod_is_proxy_target()` 接受 2D/3D/array/1D 等全部 proxy target；
- `glesmod_proxy_record()` 把 2D_ARRAY / CUBE_ARRAY / MULTISAMPLE_ARRAY 当 volume 处理，layers 被错误地按 mip 左移；
- 1D_ARRAY 的 height 是 layers，也被错误左移；
- 所有 target 共用一份 `g_proxy_state`，2D/3D 状态会互相覆盖。

MC 1.21.1 只走 `GL_PROXY_TEXTURE_2D`，所以当前修复有效；但代码对外声称覆盖所有 target，属于过度承诺。

---

## 3. 目标与非目标

### 3.1 目标

- 保留 `GL_PROXY_TEXTURE_2D` 的已验证行为；
- 其他 proxy target 不再传入 ANGLE，避免再次触发 `InvalidEnum` 空指针；
- 对未实现的 proxy target，按“该 proxy 分配失败”安全返回 0，并记录一次日志；
- （可选）`LIBGL_GLES` / `POJAVEXEC_EGL` 错配时给出明确警告，并按 EGL 实现族选择 GLES 库；
- `GLESMOD_PROXY_EMU=0` 仍能退回旧行为，便于 A/B；
- 不改变默认原生 / ANGLE 开关行为。

### 3.2 非目标

- 不实现 1D/3D/array proxy 的完整 mip 语义（以后有真实需求再做）；
- 不修 ANGLE 上游校验 bug；
- 不改 FCL 的 `egl_loader.c`；
- 不处理 CI 签名密钥；
- 不创建 PR（发布时另行准备）。

---

## 4. 方案选择

| 方案 | 行为 | 风险 | 结论 |
|---|---|---|---|
| **A. 收窄 + 安全降级** | 只仿真 2D/cube/rectangle；其他 proxy target 不进驱动，返回 0 | 低；诚实 | ✅ **采用** |
| B. 全 target 完整实现 | 按 target 算 width/height/depth 的 mip | 代码多、测试缺口大 | ⏳ 以后再做 |
| 当前实现 | 全 target 都拦，按 volume 近似 | array/1D 语义错误 | ❌ 需要改掉 |

选择 A 的理由：

- MC 只走 `GL_PROXY_TEXTURE_2D`；
- 没有 array/1D proxy 的真实测试夹具；
- 项目原则是“不崩溃 + 记录降级”，不是“猜一个近似语义”；
- 改动小，不破坏已验证路径。

---

## 5. 实施阶段

### Phase 0：基线归档（0.5 天）

- [ ] 保存当前 ANGLE 模式 / 原生模式启动日志；
- [ ] 保存当前 `native.log` 的 `proxy texture 仿真` 输出；
- [ ] 记录 `GL_MAX_TEXTURE_SIZE`、`GL_RENDERER`、ANGLE 版本；
- [ ] 确认当前 `GLESMOD_PROXY_EMU` 默认行为。

**验收**：后续改动可对照当前已验证行为。

### Phase 1：proxy 仿真收窄（0.5–1 天）

#### 5.1 拆分 target 判断

新增：

```c
/* 所有 proxy target：用于日志抑制和安全拦截 */
static int glesmod_is_proxy_target(GLenum target) { ... }

/* 真正实现了语义的 target：只放已测试的 */
static int glesmod_is_emulated_proxy_target(GLenum target) {
    switch (target) {
        case GL_PROXY_TEXTURE_2D:
        case GL_PROXY_TEXTURE_CUBE_MAP:
        case GL_PROXY_TEXTURE_RECTANGLE:
            return 1;
        default:
            return 0;
    }
}
```

#### 5.2 行为矩阵

| target | `GLESMOD_PROXY_EMU=1` | `GLESMOD_PROXY_EMU=0` |
|---|---|---|
| 2D / cube / rectangle | 本地记录 + 查询返回记录值 | 按旧行为转发（A/B 用） |
| 1D / 3D / array / multisample | 不进驱动，查询返回 0，日志一次 | 按旧行为转发 |

#### 5.3 `glTexImage2D/3D`

```c
if (glesmod_proxy_emu_enabled()) {
    if (glesmod_is_emulated_proxy_target(target)) {
        glesmod_proxy_record(...);
        return;
    }
    if (glesmod_is_proxy_target(target)) {
        glesmod_log_proxy_unsupported_once(target);
        return;   /* 不进 ANGLE */
    }
}
/* 旧行为 / 非 proxy */
```

说明：

- `glTexImage3D` 的 3D/array proxy 属于“未实现”，因此只会走安全返回，不记录；
- 2D/cube/rectangle 仍走 `glTexImage2D`，保持现有正确路径。

#### 5.4 `glGetTexLevelParameteriv/fv`

```c
if (glesmod_proxy_emu_enabled()) {
    if (glesmod_is_emulated_proxy_target(target)) {
        *params = glesmod_proxy_query(...);
        return;
    }
    if (glesmod_is_proxy_target(target)) {
        *params = 0;
        glesmod_log_proxy_unsupported_once(target);
        return;
    }
}
/* 非 proxy：原样转发 */
```

#### 5.5 简化状态

- `glesmod_proxy_state_t` 可以去掉 `depth`，只保留 `width/height/internal_format/valid`；
- 删除 `depth_is_volume` 和 3D/array 的 base-depth 逻辑；
- `glesmod_proxy_base_size()` / `level_size()` 只服务 2D/cube/rectangle 的 width/height。

#### 5.6 日志

- 未实现 target 每个 target 只记录一次：
  ```text
  [GLESMod] proxy target 0x8C1B 未实现，按不可分配处理
  ```
- 可用 `GLESMOD_DEGRADE_LEVEL` 控制是否升级为 degrade 事件；默认只写日志。

**验收**：

- MC ANGLE 启动仍正常；
- `GL_PROXY_TEXTURE_2D` 的 32768 探测返回 0，16384 返回 16384；
- `GL_PROXY_TEXTURE_2D_ARRAY` 等不再进入 ANGLE；
- `GLESMOD_PROXY_EMU=0` 可恢复旧行为（仅用于 A/B）。

> **2026-11-07 实现状态**：Phase 1 已实现并验证——`glesmod_is_emulated_proxy_target()` 只覆盖 2D/cube/rectangle；其他 proxy target 走 `glesmod_log_proxy_unsupported()` 安全返回；`depth` 已从状态结构移除。已在 Adreno 619 与 Mali-G57 上通过完整验证。

### Phase 2（可选）：EGL/GLES 同源防御性检查（0.5 天）

> 现实中出现概率极低：FCL 内置渲染器不设置 `LIBGL_GLES`，v1/v2 插件配置也不设置它。
> 本节只作为“如果将来有人手动设置”的廉价防御，不阻塞 Phase 1。

#### 6.1 新增判断

```c
static int glesmod_name_is_angle(const char *name) {
    return name != NULL && strstr(name, "angle") != NULL;
}
```

#### 6.2 `get_gles_handle()` 逻辑

```c
int egl_is_angle  = glesmod_name_is_angle(egl_name);
int gles_is_angle = glesmod_name_is_angle(libgl_gles);

if (libgl_gles && egl_name && egl_is_angle != gles_is_angle) {
    log_line(0, "警告: LIBGL_GLES=%s 与 POJAVEXEC_EGL=%s 不同源，"
                "按 EGL 实现选择 GLES 库", libgl_gles, egl_name);
    chosen = egl_is_angle ? "libGLESv2_angle.so" : "libGLESv2.so";
} else if (libgl_gles && libgl_gles[0]) {
    chosen = libgl_gles;
} else if (egl_is_angle) {
    chosen = "libGLESv2_angle.so";
}
```

原则：

- EGL 已经创建了 context，GLES 兜底必须跟随 EGL 的实现族；
- `LIBGL_GLES` 只允许在同一族内做更细的选择；
- 不 abort，保持“能跑优先 + 明确日志”。

#### 6.3 组合测试

| `LIBGL_GLES` | `POJAVEXEC_EGL` | 预期 |
|---|---|---|
| 未设置 | `libEGL.so` | 系统 GLES |
| 未设置 | `libEGL_angle.so` | ANGLE GLES |
| `libGLESv2_angle.so` | `libEGL_angle.so` | ANGLE GLES |
| `libGLESv2.so` | `libEGL_angle.so` | 警告；选 ANGLE GLES |
| `libGLESv2_angle.so` | `libEGL.so` | 警告；选系统 GLES |
| `libGLESv2.so` | `libEGL.so` | 系统 GLES |

**验收**：错配组合有明确日志；不再出现跨实例兜底；正常开关路径无变化。

### Phase 3：v2 JSON 生成（可选，1–2 天）

当前 `strings.xml` 里是一大段手工转义的 JSON。虽然已有编译产物门禁，但设计仍然脆：

- 建议把 JSON 独立为 `fcl-plugin/renderer_config.json`；
- `build-plugin.ps1` 在构建前生成转义后的 `strings.xml` 条目；
- `check_plugin_v2.py` 直接对 `.json` 做 `json.loads`，不再模拟 aapt2 的 `android_unescape`；
- `check_plugin_apk.py` 继续校验编译产物。

> 这项不是当前修复的阻塞项；如果近期不改 v2 配置，可以延后。

### Phase 4：工程债收口（0.5 天，可选）

- `check_proxy_emulation.py` 去正则化，改为按符号名/定义行检查；
- `CMakeLists.txt` 去掉重复的 `-O2` 和未使用的 `android` 链接；
- CI action pin 到 commit SHA；
- 记录 `glDrawPixels` 额外导出（历史遗留，非本次引入）。

### Phase 5：验证与文档（1 天）

- 真机：ANGLE 开关开/关、原生模式、Sodium/Embeddium/Iris、退出游戏；
- 归档 post-fix `native.log` / `status.json`；
- 更新 `docs/proxy-texture-fix-plan.md` §17；
- 更新 `docs/DEVELOPER-GUIDE.md` 的 proxy / `LIBGL_GLES` 说明；
- 文档随修复提交保留，不创建 PR。

---

## 6. 文件级改动清单

| 文件 | 改动 | 阶段 |
|---|---|---|
| `native/src/custom.c` | target 拆分、未实现 target 安全降级、简化 proxy 状态、日志 | 1 |
| `native/src/core.c` | 可选：`LIBGL_GLES` / `POJAVEXEC_EGL` 同源警告 | 2（可选） |
| `native/include/gles_backend.h` | 可选：新增 `LIBGL_GLES` 说明 | 2（可选） |
| `native/tools/check_proxy_emulation.py` | 可选：去正则化 | 4 |
| `native/CMakeLists.txt` | 可选：清理编译/链接选项 | 4 |
| `.github/workflows/build.yml` | 可选：pin action SHA | 4 |
| `fcl-plugin/renderer_config.json` | 可选：JSON 独立文件 | 3 |
| `fcl-plugin/build-plugin.ps1` | 可选：生成转义 JSON 资源 | 3 |
| `docs/*` | 验证记录与说明 | 5 |

---

## 7. 测试矩阵

### 8.1 主机

- `clang -Wall -Wextra -Wshadow -Wconversion` 无新增警告；
- `check_proxy_emulation.py` 通过；
- `check_plugin_v2.py` / `check_plugin_apk.py` 通过；
- `check_symbols.py` 849/849。

### 8.2 真机

| 场景 | 原生 | ANGLE | 通过标准 |
|---|---|---|---|
| 启动到主菜单 | ✅ | ✅ | 无 SIGSEGV |
| 进世界 | ✅ | ✅ | 正常 |
| Sodium / Embeddium | ✅ | ✅ | 无新增降级 |
| Iris / 光影 | 可选 | 可选 | 可加载或明确跳过 |
| 退出游戏 | ✅ | ✅ | 无 EGL 销毁崩溃 |
| `GLESMOD_PROXY_EMU=0` | — | 预期旧行为 | 仅 A/B，不用于发布 |

### 8.3 断言

- `GL_PROXY_TEXTURE_2D` 32768 探测返回 0；
- 16384 探测返回 16384；
- array/1D/3D proxy 不再进入 ANGLE；
- 错配 env 有 `LIBGL_GLES ... 与 POJAVEXEC_EGL ... 不同源` 日志；
- 正常开关路径无变化。

---

## 8. 验收标准

- [x] MC 1.21.1 ANGLE 模式启动不崩溃；
- [x] 原生模式无回归；
- [x] 2D proxy 返回值正确；
- [x] 未实现 proxy target 安全返回 0，不进入 ANGLE；
- [x] `GLESMOD_PROXY_EMU=0` 可恢复旧行为；
- [ ] （可选）EGL/GLES 错配有明确日志并选择同源库；
- [x] 849 符号、proxy gate、v2 gate 全绿；
- [x] 文档记录完成；
- [x] 计划文档保留在仓库，不删除。

> 验证设备：Oppo PGGM10 / Adreno 619 / Android 14，以及 Mali-G57 设备；
> 原生与 ANGLE 两条路径均通过。可选错配日志未实现，按计划降为 P3。

---

## 9. 风险与缓解

| 风险 | 影响 | 缓解 |
|---|---|---|
| 收窄后某个模组使用 array proxy | 该探测返回 0，功能可能降级 | 记录日志；后续按真实需求补 target |
| `GLESMOD_PROXY_EMU=0` 在 ANGLE 下崩溃 | 仅调试用 | 文档标注“不要在生产 ANGLE 模式关闭” |
| env 同源校验误判 | 选错 GLES 库 | 只按 `angle` 族判断；保留显式 `LIBGL_GLES` 同族选择 |
| 改动破坏已验证 2D 路径 | 启动崩溃回归 | 先保留现有 2D 逻辑，只改 target 判断和状态结构 |

---

## 10. 工作量

| 阶段 | 内容 | 估算 |
|---|---|---|
| Phase 0 | 基线归档 | 0.5 天 |
| Phase 1 | proxy 收窄 | 0.5–1 天 |
| Phase 2 | env 同源检查（可选） | 0.5 天 |
| Phase 3 | JSON 生成（可选） | 1–2 天 |
| Phase 4 | 工程债（可选） | 0.5 天 |
| Phase 5 | 验证/文档 | 1 天 |
| **合计** | 不含可选 Phase 3 | **约 2.5–3.5 天** |

---

## 11. 待确认

1. 是否同意“只支持 2D + cube/rectangle，其他 proxy target 安全返回 0”？
2. `GLESMOD_PROXY_EMU=0` 是否保留“旧行为转发”语义，还是直接禁用整个 proxy 拦截？
3. （可选）env 错配时是否值得加一条警告？由于 FCL 默认路径几乎不会产生错配，建议只加日志、不做强制逻辑。
4. v2 JSON 生成器现在就做，还是延后？
5. 是否要把 `LIBGL_GLES` 写入 `docs/DEVELOPER-GUIDE` 环境变量表？

---

## 12. 变更记录

| 日期 | 变更 |
|---|---|
| 2026-11-07 | 初稿：proxy 收窄、EGL/GLES 同源保护、可选 JSON 生成与工程债收口；文档随修复提交保留，不创建 PR |
| 2026-11-07 | 修正：`LIBGL_GLES`/`POJAVEXEC_EGL` 错配在 FCL 默认路径下几乎不存在，Phase 2 降为可选防御性检查 |

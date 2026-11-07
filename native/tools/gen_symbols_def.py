#!/usr/bin/env python3
"""
从 Khronos gl.xml 生成完整的 symbols.def。

用途：
    覆盖 GL 3.2 core profile 的【全部】316 个符号，而不是只覆盖 Minecraft
    直接调用的那部分。

为什么必须覆盖全集（这是一次真实的教训）：
    最初的做法是扫描 MC 的 class 文件，只导出它直接引用的 89 个符号。
    这在真机上立刻失败了：
        java.lang.NullPointerException: A required function is missing: glGetStringi
            at org.lwjgl.opengl.GL.createCapabilities(GL.java:514)
    LWJGL 在 createCapabilities() 时需要一批函数（如 core profile 下枚举
    扩展所用的 glGetStringi），缺失即抛 NPE 硬崩溃，且无法被上层捕获。
    第三方模组（Sodium 等）也会调用我们没预料到的符号。

关键设计：导出为「安全 stub」比「不导出」更好
    不导出          -> LWJGL 解析失败 -> 抛异常崩溃
    导出为 stub     -> LWJGL 拿到有效指针 -> 调用时返回零值、记降级事件、不崩溃
    因此对于 GLES 不提供的符号，恒生成 stub 而非留空。

符号分类：
    F  转发。GLES 中存在同名函数，直接转发。
    S  Stub。GLES 中不存在，生成安全空实现（记降级 + 返回零值）。
    C  定制。需要映射到不同的 GLES 函数或特殊处理，实现在 custom.c。

用法：
    py gen_symbols_def.py --gl-xml .cache/gl.xml --out ../symbols.def
"""

from __future__ import annotations

import argparse
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

# ---------------------------------------------------------------------
# 手工维护的定制实现清单（定义在 native/src/custom.c）
#
# key   = GL 符号名
# value = 用于 symbols.def 的参数列表（类型 + 参数名）
# ---------------------------------------------------------------------

# 需要映射到不同 GLES 函数、或有特殊语义的符号。
# 参数列表按 gl.xml 的原始签名生成，custom.c 里的实现与之对应。
#
# glGetShaderInfoLog / glGetProgramInfoLog 被列为定制实现，目的不是改变行为，
# 而是【捕获驱动返回的着色器编译错误】：
#   真机上「着色器编译失败」的表现只有一句
#       java.lang.IllegalStateException: could not preload shader position
#   完全看不出 GLSL 哪里错了。而 MC 在判定失败前一定会调用
#   glGetShaderInfoLog 取错误文本 —— 把它拦下来写进日志，
#   就能得到驱动给的确切错误行与原因（这是唯一可靠的一手证据）。
CUSTOM_SYMBOLS = {
    "glClearDepth",
    "glDrawPixels",
    "glGetError",
    "glGetTexImage",
    "glLogicOp",
    "glPolygonMode",
    "glMapBuffer",
    "glMultiDrawElements",
    "glMultiDrawArrays",
    "glMultiDrawElementsBaseVertex",
    "glMultiDrawArraysIndirect",
    "glMultiDrawElementsIndirect",
    "glShaderSource",
    # 仅用于捕获错误信息，行为与原函数完全一致
    "glGetShaderInfoLog",
    "glGetProgramInfoLog",
    # 用于在编译失败时主动转储源码（不能只依赖 MC 去查 info log）
    "glGetShaderiv",
    # 几何错位诊断（见 src/probe.c）
    "glDrawElements",
    # 【深度缓冲修复】桌面 GL 允许把 GL_DEPTH_COMPONENT(0x1902) 当作
    # internalformat，**GLES 不允许**（ES 只接受 DEPTH_COMPONENT16/24/32F
    # 或 DEPTH24_STENCIL8）。MC 的 RenderTarget/MainTarget 正是传
    #   glTexImage2D(..., 0x1902 /*GL_DEPTH_COMPONENT*/, w, h, 0,
    #                 0x1902 /*GL_DEPTH_COMPONENT*/, 0x1406 /*GL_FLOAT*/, NULL)
    # 驱动原话（latest.log）：the combination of format 6402 and type 5126
    # is unsupported。结果：**深度纹理根本没被分配** -> 深度测试形同虚设
    # -> 该被遮的面露出来、实体正反面交叠（用户实测症状）。
    # 需要定制实现来把 internalformat/format 翻译成 ES 合法值。
    "glTexImage2D",
    "glTexImage3D",
    # 【proxy texture 仿真】
    #   GLES 没有 GL_PROXY_TEXTURE_*；桌面 GL 用它探测最大纹理尺寸。
    #   ANGLE 2.1.1 对 proxy target 的 glGetTexLevelParameteriv 会空指针崩溃。
    #   改为定制实现：proxy target 返回本地记录值，不进入驱动。
    "glGetTexLevelParameteriv",
    "glGetTexLevelParameterfv",
    # 【glTexStorage2D/3D：深度格式翻译必须覆盖「不可变存储」这条路】
    #   Iris 建 FBO 报 36055 = INCOMPLETE_MISSING_ATTACHMENT（附件无存储）。
    #   符号表核对显示关键差异：
    #     glTexStorage2D -> F 纯转发（**没有任何格式翻译**）
    #     glTexImage2D   -> C 定制（有深度格式翻译）
    #   => 深度格式的 ES 合法化**只做在了 glTexImage2D 这条路上**。
    #   而 glTexStorage2D 是另一条分配纹理存储的 API，同样只接受
    #   ES 合法的深层格式；传桌面值 0x1902 会被拒 -> 纹理无存储
    #   -> attach 后即 36055。伴随的 'pixel buffer format is not
    #   compatible with level format' 与此完全吻合。
    "glTexStorage2D",
    "glTexStorage3D",
    # 【桌面专属 pname 过滤 + 清错误位】
    #   MC 的 TextureUtil 每次创建纹理都会调
    #       glTexParameterf(GL_TEXTURE_2D, 34049 /*GL_TEXTURE_LOD_BIAS*/, 0.0f)
    #   GL_TEXTURE_LOD_BIAS 是桌面专属 pname，GLES 拒绝它：
    #       'pname 34049 is not supported for this API call'（每轮约 70 次）
    #   值恒为 0.0（等于默认值），MC 也从不读回，因此忽略是语义等价的。
    #   **但驱动会留下 GL_INVALID_ENUM，而本库的 glGetError 是定制实现、
    #     会把真实错误原样交给 MC** -> 必须顺手清掉，否则等于给 MC 喂假错误。
    "glTexParameterf",
    "glTexParameteri",
    # 【glGetString：改写为桌面 GL 兼容的版本串】
    #   真机崩溃（2026-09-30，Iris 1.8.14-beta.1）：
    #     IllegalStateException: Could not parse GL version from
    #       "OpenGL ES 3.2 V@0762.36 ..."
    #   Iris 用正则从 GL_VERSION 抓「主.次」；桌面串 "4.6.0 NVIDIA 536.23"
    #   可匹配，而我们的串以 "OpenGL ES " 开头 -> 匹配失败 -> 光影加载失败。
    #   修法：对外返回 "3.2 (OpenGL ES 3.2 <GL_RENDERER>)"。
    #   声称 3.2 与**实际导出的符号集**（GL 3.2 core 子集）一致，
    #   不是伪造 4.6（那会诱导调用方去用 DSA 等静默 stub 特性）。
    # 【glCheckFramebufferStatus：把「不完整」翻译成可定位的原因】
    #   Iris 建 FBO 失败只报 "Status: 36055"，不足以定位根因。
    #   包一层：状态 != COMPLETE 时主动查询附件对象/层级并写日志。
    #   正常路径只多一次比较，不做 GL 查询，不影响性能。
    # 【glGetStringi / glGetIntegerv：扩展列表追加必须成对，否则是死代码】
    #   Iris 的 supportsBufferBlending() 检查 GL_ARB_draw_buffers_blend || OpenGL40，
    #   两者在 ES 上都不存在 -> 永远 false -> 光影加载抛异常。
    #   而该扩展的全部函数在 ES 3.0 起就是核心，本项目均为真实转发：
    #     glBlendFunci / glBlendFuncSeparatei / glBlendEquationi /
    #     glBlendEquationSeparatei / glColorMaski / glEnablei / glDisablei
    #   => 能力具备，只是扩展名不在驱动列表里。追加它属于「如实」。
    #
    #   ★ 上一轮只改了 glGetStringi，真机上完全没生效 ★
    #   LWJGL 查权威源码后确认：**ES 路径根本不调用 glGetStringi**，
    #   它解析的是 glGetString(GL_EXTENSIONS) 这一个空格分隔的整串：
    #       final StringTokenizer tokenizer = new StringTokenizer(extensions_string);
    #       while ( tokenizer.hasMoreTokens() )
    #           supported_extensions.add(tokenizer.nextToken());
    #   因此生效路径是 glGetString 的 GL_EXTENSIONS 分支（已实现追加）。
    #   glGetStringi 保留给桌面兼容路径；为使它自成一致（计数与实际枚举相符），
    #   glGetIntegerv 的 GL_NUM_EXTENSIONS 也要 +1，否则那条索引永远读不到。
    "glGetStringi",
    "glCheckFramebufferStatus",
    "glGetString",
    "glGetIntegerv",
    # 【glBufferStorage：静默 stub 会让调用方崩在后续 glMapBufferRange】
    #   GLES 3.2 核心没有 glBufferStorage（GL 4.4 / ARB_buffer_storage 的功能），
    #   一开始被生成为安全 stub。但它在**有存储语义要求**的调用上是危险的：
    #     glBufferStorage(...)  -> stub 返回 void，不分配存储、无错误
    #     glMapBufferRange(...) -> 因无存储而失败，返回 NULL
    #     调用方                -> 抛异常崩溃
    #   一手证据（Embeddium 21.1 分支）：
    #     RenderRegionManager.createStagingBuffer() 会在
    #       useAdvancedStagingBuffers && MappedStagingBuffer.isSupported(device)
    #     时选 MappedStagingBuffer，其 STORAGE_FLAGS 含 PERSISTENT + CLIENT_STORAGE；
    #     而 BufferStorageFunctions.pickBest() 只认 OpenGL44 / GL_ARB_buffer_storage，
    #     **完全没有考虑 ES 场景**（CORE 与 ARB 两条路都落到本符号上）。
    #     GLRenderDevice.mapBuffer() 在 glMapBufferRange 返回 NULL 时
    #     直接 throw new RuntimeException("Failed to map buffer")。
    #   => 改为定制实现：用 glBufferData 真实分配存储 + 记录降级。
    #      把「崩溃」转化为「性能降级」，符合本项目的核心原则。
    "glBufferStorage",
    # 【glMapBufferRange：剥离 ES 非法的映射位】
    #   ES 只接受 READ/WRITE/INVALIDATE_RANGE/INVALIDATE_BUFFER/
    #   FLUSH_EXPLICIT/UNSYNCHRONIZED。而桌面 GL 4.4 的
    #     GL_MAP_PERSISTENT_BIT(0x0040) / GL_MAP_COHERENT_BIT(0x0080) /
    #     GL_CLIENT_STORAGE_BIT(0x0200)
    #   在 ES 上非法，原样转发会让驱动返回 GL_INVALID_OPERATION -> 映射失败
    #   -> 调用方崩溃。这三个位只是「性能承诺」而非「正确性要求」，
    #   掩掉后驱动走更保守路径，**不会算错数据**。
    "glMapBufferRange",
}


def parse_version(number: str) -> tuple[int, ...]:
    parts = []
    for seg in number.split("."):
        try:
            parts.append(int(seg))
        except ValueError:
            break
    return tuple(parts)


def normalize_type(raw: str) -> str:
    """把类型字符串规范化为稳定的 C 形式。"""
    # 类型名与 * 之间补空格：`GLchar*` -> `GLchar *`
    s = re.sub(r"(?<=[A-Za-z0-9_])\*", " *", raw)
    return re.sub(r"\s+", " ", s).strip()


def join_type(el: ET.Element) -> str:
    """
    把 <param> / <proto> 的内容还原为 C 类型字符串（保留 *、const、数组）。

    【这是本文件最关键的一处实现，曾导致真机 SIGSEGV】

    指针的 `*` 与 `const` 不属于 <ptype>，而是 <param> 内部的兄弟文本节点：

        <param><ptype>GLuint</ptype> *<name>buffers</name></param>
        <param>const <ptype>GLchar</ptype> *<name>source</name></param>

    只读取 <ptype>.text 会把 `GLuint *` 变成 `GLuint`。后果不是编译错误，
    而是运行时的静默数据损坏：在 AArch64（Android arm64-v8a）上，
    `GLuint` 只占参数寄存器低 32 位，于是 LWJGL 传来的 64 位指针被截断，
    驱动写入时即 SIGSEGV。真机表现：

        SIGSEGV at __memcpy_aarch64_simd+0x84
        si_addr: 0x00000000a0eaf3cc     <- 被截断的指针
        GL.createCapabilities(...)      <- 内部调用 glGetIntegerv(..., IntBuffer)

    但也不能退回「拼 itertext 再删掉变量名」的旧做法：变量名恰好是类型名
    子串时会误删（<ptype>GLintptr</ptype> 配参数名 ptr 会被删成 GLint）。
    因此这里按元素边界重建：跳过 <name> 元素本身，其余文本与子元素按序保留。
    """
    pieces = []
    if el.text:
        pieces.append(el.text)
    for child in el:
        if child.tag == "name":
            # 变量名（或函数名）：整块跳过，只保留其后的文本
            if child.tail:
                pieces.append(child.tail)
            continue
        if child.text:
            pieces.append(child.text)
        if child.tail:
            pieces.append(child.tail)
    return normalize_type("".join(pieces))


def load_commands(root: ET.Element) -> dict[str, dict]:
    """
    建立 command 名称 -> {return, params:[(type, name)]} 的索引。

    类型提取统一走 join_type()，它保留了 *、const 与数组形式。
    细节与教训见 join_type 的文档字符串。
    """
    commands: dict[str, dict] = {}
    for cmd in root.findall("./commands/command"):
        proto = cmd.find("proto")
        if proto is None:
            continue
        name_el = proto.find("name")
        if name_el is None or not name_el.text:
            continue
        name = name_el.text.strip()

        ret = join_type(proto)

        params = []
        for p in cmd.findall("param"):
            p_name_el = p.find("name")
            p_name = p_name_el.text.strip() if (p_name_el is not None
                                                and p_name_el.text) else ""
            params.append((join_type(p), p_name))

        commands[name] = {"return": ret, "params": params}
    return commands


def collect(root: ET.Element, max_version: tuple[int, ...],
            profile: str | None, api: str) -> set[str]:
    """按版本顺序累积 require/remove，返回最终生效的命令名集合。"""
    prefixes = ("GL_VERSION_", "GL_ES_VERSION_")
    features = []
    for feature in root.findall("./feature"):
        if feature.get("api") != api:
            continue
        name = feature.get("name", "")
        number = feature.get("number")
        if not name.startswith(prefixes) or not number:
            continue
        ver = parse_version(number)
        if not ver or ver > max_version:
            continue
        features.append((ver, feature))
    features.sort(key=lambda x: x[0])

    active: set[str] = set()
    for _, feature in features:
        for req in feature.findall("require"):
            prof = req.get("profile")
            if prof is not None and profile is not None and prof != profile:
                continue
            for c in req.findall("command"):
                n = c.get("name")
                if n:
                    active.add(n)
        for rem in feature.findall("remove"):
            prof = rem.get("profile")
            if prof is not None and profile is not None and prof != profile:
                continue
            for c in rem.findall("command"):
                n = c.get("name")
                if n:
                    active.discard(n)
    return active


def load_required_list(path: Path) -> set[str]:
    """
    读取一份「必须导出」的符号名清单（每行一个）。

    目前用途：传入 extract_lwjgl_natives.py 从 LWJGL jar 提取的结果。
    这是确定必需符号的【权威来源】——见 main() 开头的说明。
    """
    if not path.is_file():
        return set()
    out = set()
    # utf-8-sig：容忍 BOM，否则首个符号名会带上 \ufeff 前缀而无法匹配
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        s = line.strip()
        if s and not s.startswith("#"):
            out.add(s)
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description="生成完整的 symbols.def")
    ap.add_argument("--gl-xml", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--required", help=(
        "必须导出的符号清单（每行一个），由 extract_lwjgl_natives.py 生成。"
        "强烈建议提供——这是权威来源。省略时退化为按 GL 版本取并集。"))
    ap.add_argument("--gl-version", default="4.6",
                    help="仅在未提供 --required 时使用。默认取全部版本以策安全")
    ap.add_argument("--gles-version", default="3.2")
    args = ap.parse_args()

    path = Path(args.gl_xml)
    if not path.is_file():
        print(f"错误: 找不到 {path}", file=sys.stderr)
        return 1

    root = ET.parse(path).getroot()
    commands = load_commands(root)

    gles = collect(root, parse_version(args.gles_version), None, "gles2")

    # ---- 确定需要导出的符号集合 ----
    #
    # 【为什么以 LWJGL 清单为准】
    #   曾用「GL core profile 全集」作为目标，仍然反复真机失败：
    #     NullPointerException: A required function is missing: glGetStringi
    #   LWJGL 在 GL.createCapabilities() 时解析它声明的全部 native 方法
    #   （实测 474 个，覆盖 GL 1.0–4.6，含 glBegin 等兼容性函数），
    #   其中「必需函数」缺失即抛 NPE，且早期初始化阶段无法捕获。
    #   LWJGL 自己的声明才是权威清单，任何按 GL 版本推测的做法都会漏。
    required = load_required_list(Path(args.required)) if args.required else set()

    if required:
        # 以 LWJGL 清单为主，并上 GL core 全集作为补充
        # （覆盖 LWJGL 未声明但第三方模组可能直接解析的符号）
        gl_core = collect(root, parse_version(args.gl_version), "core", "gl")
        targets = set(required) | gl_core
        source_note = (f"LWJGL 清单({len(required)}) ∪ GL {args.gl_version} core"
                       f"({len(gl_core)})")
    else:
        gl_core = collect(root, parse_version(args.gl_version), "core", "gl")
        targets = gl_core
        source_note = f"GL {args.gl_version} core（未提供 --required，可能不完整）"

    # egl* 由启动器负责加载，不由本库导出
    targets = {n for n in targets if n.startswith("gl")}

    def render_params(name: str) -> str:
        """
        渲染参数列表。

        参数名统一生成为 p0, p1, ...，不用 gl.xml 的原始名称。原因：

        1. 原始名称偶尔会撞上 C 保留字或平台宏（near/far/type/index）。

        2. 更重要的是解析歧义。转发层生成器靠「最后一个词是否以小写开头」
           区分类型与变量名，而 `GLintptr p0` 这类类型名本身以小写结尾，
           会被误拆成 `GLi` + `ptr`。统一用 p0/p1 可彻底消除。

        参数类型仍然取 gl.xml 的权威值。
        """
        meta = commands.get(name)
        if meta is None:
            return ""
        return ", ".join(
            f"{ptype} p{i}" for i, (ptype, _) in enumerate(meta["params"])
        )

    def render_ret(name: str) -> str:
        meta = commands.get(name)
        return meta["return"] if meta else "void"

    forwards, stubs, customs, unknown = [], [], [], []

    for name in sorted(targets):
        if name not in commands:
            unknown.append(name)
            continue
        if name in CUSTOM_SYMBOLS:
            customs.append(name)
        elif name in gles:
            forwards.append(name)
        else:
            stubs.append(name)

    # ---- 生成文件 ----
    lines = []
    lines.append("/*")
    lines.append(" * symbols.def —— GL 符号清单（由 gen_symbols_def.py 自动生成）")
    lines.append(" *")
    lines.append(" * 请勿手工编辑。重新生成：")
    lines.append(" *   # 1) 从 LWJGL 提取权威需求清单")
    lines.append(" *   py native/tools/extract_lwjgl_natives.py \\")
    lines.append(" *       --jar <lwjgl-opengl.jar> --out native/tools/lwjgl_required.txt")
    lines.append(" *   # 2) 生成符号表")
    lines.append(" *   py native/tools/gen_symbols_def.py --gl-xml .cache/gl.xml \\")
    lines.append(" *       --required native/tools/lwjgl_required.txt --out native/symbols.def")
    lines.append(" *")
    lines.append(f" * 符号来源：{source_note}")
    lines.append(f" * 导出总数：{len(forwards) + len(customs) + len(stubs)}")
    lines.append(" *")
    lines.append(" * 【为什么必须覆盖 LWJGL 声明的全部符号】")
    lines.append(" *   真机教训（两次）：")
    lines.append(" *     NullPointerException: A required function is missing: glGetStringi")
    lines.append(" *         at org.lwjgl.opengl.GL.createCapabilities(GL.java:514)")
    lines.append(" *   LWJGL 在 createCapabilities() 时解析它声明的全部 native 方法，")
    lines.append(" *   缺失「必需函数」即抛 NPE 硬崩溃，早期初始化阶段无法捕获。")
    lines.append(" *   LWJGL 的声明（实测 474 个，含 GL 1.0–4.6 与兼容性函数）")
    lines.append(" *   才是权威清单；任何按 GL 版本推测的做法都会漏。")
    lines.append(" *")
    lines.append(" * 【导出安全 stub 优于不导出】")
    lines.append(" *   不导出      -> LWJGL 解析失败 -> 抛 NPE 崩溃（不可捕获）")
    lines.append(" *   导出为 stub -> 拿到有效指针 -> 返回零值 + 记降级事件，不崩溃")
    lines.append(" *   因此 GLES 不提供的符号一律生成 stub。")
    lines.append(" *")
    lines.append(" * 转发类型：")
    lines.append(" *   F  直接转发（GLES 有同名函数）")
    lines.append(" *   S  安全 stub（GLES 无此函数，返回零值并记降级）")
    lines.append(" *   C  定制实现（映射到其他 GLES 函数，见 src/custom.c）")
    lines.append(" *")
    lines.append(" * LGPL-3.0-or-later")
    lines.append(" */")
    lines.append("")
    lines.append(f"/* ===== 直接转发（{len(forwards)} 个）===== */")
    lines.append("")
    for name in forwards:
        lines.append(f"F {render_ret(name)} {name}({render_params(name)})")
    lines.append("")
    lines.append(f"/* ===== 定制实现（{len(customs)} 个，定义在 src/custom.c）===== */")
    lines.append("")
    for name in customs:
        lines.append(f"C {render_ret(name)} {name}({render_params(name)})")
    lines.append("")
    lines.append(f"/* ===== 安全 stub（{len(stubs)} 个，GLES 不提供）===== */")
    lines.append("")
    for name in stubs:
        lines.append(f"S {render_ret(name)} {name}({render_params(name)})")
    lines.append("")

    Path(args.out).write_text("\n".join(lines), encoding="utf-8")

    total = len(forwards) + len(customs) + len(stubs)
    print(f"符号来源 : {source_note}")
    print(f"GLES {args.gles_version} 提供同名函数: {len(gles)}")
    print()
    print(f"  F 直接转发 : {len(forwards):4d}")
    print(f"  C 定制实现 : {len(customs):4d}")
    print(f"  S 安全 stub: {len(stubs):4d}")
    print(f"  合计       : {total:4d}")
    if unknown:
        print(f"  gl.xml 无定义: {len(unknown):4d}  (跳过)")
        for n in unknown[:10]:
            print(f"      {n}")
    print()
    print(f"输出: {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

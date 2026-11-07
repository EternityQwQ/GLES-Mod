/*
 * shader.c —— 桌面 GLSL -> GLSL ES 320 转换
 *
 * 设计原则（对应任务书 O-06 决策：尽最大努力兼容，无法兼容则降级）：
 *   1. 覆盖常见差异，不追求完整 GLSL 前端
 *   2. 转换失败时返回 NULL，由调用方决定如何处理（回退原源码，让 GL 报错）
 *   3. 绝不静默产生语义错误的着色器：不确定时宁可不转换
 *
 * 覆盖的差异：
 *   - #version 声明改写为 320 es
 *   - 注入 precision 限定符（ES 要求）
 *   - 废弃内置替换：texture2D/textureCube -> texture，gl_FragColor -> out 变量
 *   - varying -> in/out（视阶段而定，保守处理）
 *   - 桌面专属内建变量清理
 *
 * 不做的事：
 *   - 不实现几何/细分着色器（ES 3.2 无条件支持）
 *   - 不处理 layout 之外的复杂语义分析
 *
 * 许可证：LGPL-3.0-or-later
 */

#include "gl_internal.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 输出缓冲区增长策略 */
#define INITIAL_CAP 4096

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
    int failed;
} sbuf;

static void sbuf_init(sbuf *sb, size_t cap) {
    sb->buf = (char *)malloc(cap);
    sb->len = 0;
    sb->cap = cap;
    sb->failed = (sb->buf == NULL);
    if (sb->buf != NULL) sb->buf[0] = '\0';
}

static void sbuf_put(sbuf *sb, const char *s, size_t n) {
    if (sb->failed) return;
    if (sb->len + n + 1 > sb->cap) {
        size_t ncap = sb->cap * 2;
        while (ncap < sb->len + n + 1) ncap *= 2;
        char *nb = (char *)realloc(sb->buf, ncap);
        if (nb == NULL) {
            sb->failed = 1;
            return;
        }
        sb->buf = nb;
        sb->cap = ncap;
    }
    memcpy(sb->buf + sb->len, s, n);
    sb->len += n;
    sb->buf[sb->len] = '\0';
}

static void sbuf_puts(sbuf *sb, const char *s) {
    sbuf_put(sb, s, strlen(s));
}

/* ------------------------------------------------------------------ */
/* 阶段判定                                                            */
/* ------------------------------------------------------------------ */

typedef enum {
    STAGE_UNKNOWN,
    STAGE_VERTEX,
    STAGE_FRAGMENT,
} shader_stage;

/*
 * 通过 GL 的 shader 类型无法在此获取（本函数只拿到源码），
 * 因此用内容启发式判定阶段：
 *   - 含 gl_FragColor / gl_FragData / gl_FragCoord -> 片元
 *   - 含 gl_Position                                -> 顶点
 */
static shader_stage detect_stage(const char *src) {
    if (strstr(src, "gl_FragColor") != NULL ||
        strstr(src, "gl_FragData") != NULL ||
        strstr(src, "gl_FragCoord") != NULL) {
        return STAGE_FRAGMENT;
    }
    if (strstr(src, "gl_Position") != NULL) {
        return STAGE_VERTEX;
    }
    return STAGE_UNKNOWN;
}

/*
 * 把 GL 报告的真实阶段映射为内部表示。
 *
 * 【为什么必须优先用真实阶段，而不是靠源码猜测】
 *   detect_stage() 只能匹配 gl_FragColor / gl_Position 这些传统标志。
 *   现代的桌面着色器（如 FML 早期显示窗口）用的是：
 *       out vec4 fragColor;        <- 片元，但没有 gl_FragColor
 *       in vec2 position;          <- 顶点，但没有 gl_Position 之外的特征
 *   于是会被猜成 UNKNOWN，导致：
 *     - 漏掉片元必需的 precision 声明  -> 驱动编译必然失败
 *     - 不转换 varying/attribute 限定符 -> 同样编译失败
 *   真机上 FML 的片元着色器正是因此无法编译。
 *   调用方用 glGetShaderiv(shader, GL_SHADER_TYPE, &t) 即可拿到确切阶段，
 *   这个信息一直是可得的，没有理由依赖猜测。
 */
static shader_stage stage_from_gl(int stage) {
    if (stage == GLESMOD_SHADER_STAGE_VERTEX)   return STAGE_VERTEX;
    if (stage == GLESMOD_SHADER_STAGE_FRAGMENT) return STAGE_FRAGMENT;
    return STAGE_UNKNOWN;
}

/* ------------------------------------------------------------------ */
/* ES 采样器精度限定符                                                 */
/* ------------------------------------------------------------------ */

/* 定义在后面（词法安全替换区），此处先声明以便复用 */
static int is_ident_char(char c);

/*
 * 【前置声明 —— 缺陷 A 必需的，血泪教训】
 *   `in_builtin_int_arg` 的定义在文件后半部分（约 5700 行），
 *   而 `process_stmt` 在 3600 多行就要调用它。
 *   若不在这里声明，gcc 报 -Wimplicit-function-declaration 并**退化为
 *   返回 int 的隐式声明**，编译虽会失败，但更早的教训是：脚本只看
 *   「exe 是否存在」时会**继续用旧二进制**，症状表现为「补丁看起来实现了
 *   却零效果」——本文件第一次修规则 R 时正是栽在这里。
 *   因此：凡 process_stmt 需要、而定义在后面的辅助函数，一律在此前置声明。
 *
 * 【注意只声明真正需要的那一个】
 *   `inside_float_ctor_args`（情形 B 调用，约 6820 行）与
 *   `collect_builtin_int_returning_funcs`（约 6100 行调用）的定义
 *   都在各自调用点**之前**，无需声明；
 *   而且 collect_builtin_int_returning_funcs 的签名含 `ivec_var`，
 *   该类型要到 3900 多行才定义 —— 在这里声明反而会编译不过。
 */
static int in_builtin_int_arg(const char *p, const char *src);


/*
 * GLSL ES 里，除 sampler2D / samplerCube 之外**所有采样器类型都没有默认精度**，
 * 必须显式声明 `precision <p> <type>;` 才能声明该类型的变量。
 * 桌面 GLSL 完全没有这个要求 —— 于是「在桌面能编译」的着色器
 * 一旦被我们改标成 `#version 320 es` 就必然编译失败。
 *
 * 【这是 Sodium 区块着色器崩溃的根因】
 *   Sodium 的 block_layer_opaque.vsh 里有：
 *       uniform isamplerBuffer u_SectionTimeInfo;   // 区块淡入的时间缓冲
 *   驱动报错：
 *       'sampler/image' : type requires declaration of default precision qualifier
 *   Sodium 编译失败 -> 抛 RuntimeException -> 进入世界时直接崩溃。
 *
 * 【为什么两个阶段都要注入】
 *   顶点着色器同样受此约束（曾经只在片元阶段注入精度，是错的）。
 *   实测（native/tools/es_sampler_precision_probe.py，用 NDK 的 glslang）：
 *     顶点阶段需要显式精度的类型共 26 个，只有 sampler2D / samplerCube 例外；
 *     片元阶段同样。
 *
 * 用 highp：ES 3.x 强制支持，且语义上严格更安全（mediump 的 float 上限
 * 只有 65504，MC 的世界坐标会溢出）。
 *
 * 多声明未使用的采样器精度是**完全合法**的，不影响链接。
 * 列表按「长名在前」排列，避免前缀互相干扰。
 */
static const char *const ES_SAMPLER_PRECISION_TYPES[] = {
    /* 阴影采样器 */
    "samplerCubeArrayShadow",
    "sampler2DArrayShadow",
    "samplerCubeShadow",
    "sampler2DShadow",
    /* 整数采样器 */
    "isampler2DMSArray",
    "isamplerCubeArray",
    "isampler2DArray",
    "isamplerBuffer",
    "isampler2DMS",
    "isamplerCube",
    "isampler3D",
    "isampler2D",
    /* 无符号整数采样器 */
    "usampler2DMSArray",
    "usamplerCubeArray",
    "usampler2DArray",
    "usamplerBuffer",
    "usampler2DMS",
    "usamplerCube",
    "usampler3D",
    "usampler2D",
    /* 浮点采样器 */
    "sampler2DMSArray",
    "samplerCubeArray",
    "sampler2DArray",
    "samplerBuffer",
    "sampler2DMS",
    "sampler3D",
    "sampler2D",
    "samplerCube",
};

/*
 * 判断着色器是否用到了某个采样器类型（按标识符边界匹配，避免误判）。
 * 找不到就跳过该声明，让输出尽量精简。
 */
static int source_uses_sampler_type(const char *src, const char *type) {
    size_t n = strlen(type);
    const char *p = src;
    while ((p = strstr(p, type)) != NULL) {
        int left_ok = (p == src) || !is_ident_char(p[-1]);
        int right_ok = !is_ident_char(p[n]);
        if (left_ok && right_ok) return 1;
        p += n;
    }
    return 0;
}

/*
 * 为源码实际用到的采样器类型注入默认精度声明。
 * 返回写入的声明数量。
 */
static int emit_sampler_precision(sbuf *out, const char *src) {
    int count = 0;
    for (size_t i = 0;
         i < sizeof(ES_SAMPLER_PRECISION_TYPES) /
                 sizeof(ES_SAMPLER_PRECISION_TYPES[0]);
         i++) {
        const char *t = ES_SAMPLER_PRECISION_TYPES[i];
        if (!source_uses_sampler_type(src, t)) continue;
        sbuf_puts(out, "precision highp ");
        sbuf_puts(out, t);
        sbuf_puts(out, ";\n");
        count++;
    }
    return count;
}

/*
 * 移除 GLSL ES 不支持的 #extension 指令。
 *
 * 桌面着色器常写：
 *     #extension GL_ARB_separate_shader_objects : require
 * 该扩展在 ES 中不存在，`require` 语义 = 不支持则报错：
 *     'extension not supported'
 * 而 ES 3.x 本身已原生支持 location 限定符与独立着色器对象语义，
 * 所以直接删掉这行是正确且安全的。
 *
 * 保留 GL_ES / GL_OES / GL_EXT / GL_KHR / GL_ANGLE / GL_NV 开头的
 * 扩展名（可能确实存在于该驱动），让驱动自行判断。
 */
/*
 * 整数常量宏展开。
 *
 * ================== 为什么必须做这件事 ==================
 *
 * 真机 Flywheel 的 flywheel:internal/wavelet.glsl 定义
 *     #define TRANSPARENCY_WAVELET_COEFFICIENT_COUNT 16
 * 然后把它用在**不同数据类型**的上下文里：
 *     int   index   = clamp(int(floor(d)), 0, TRANSPARENCY_WAVELET_COEFFICIENT_COUNT - 1);  // 整数
 *     float depth  *= float(...) / TRANSPARENCY_WAVELET_COEFFICIENT_COUNT;                   // 浮点
 *     float cdepth  = depth * TRANSPARENCY_WAVELET_COEFFICIENT_COUNT;                        // 浮点
 *
 * 桌面 GLSL 靠隐式转换让两种用法都成立。GLSL ES 没有隐式 int<->float 转换，
 * 因此**同一个宏在两类上下文里需要相反的改写**：
 *     整数处：`COUNT - 1` 必须保持 int
 *     浮点处：`* COUNT`  必须变成 float
 *
 * 任何「把宏名一刀切当成 int（或 float）」的做法都必然在另一半出错 ——
 * 这是实测踩到的：登记为 int 后 clamp 修好了，但 `depth * COUNT` 反而
 * 从「隐式转换」变成「确定非法」，错误数只从 38 降到 22。
 *
 * 【正确做法：按定义文本展开，与预处理器语义完全一致】
 *   把宏名就地替换为它的定义 token（`TRANSPARENCY_...` -> `16`），
 *   然后**删除该 #define**，让原本就存在、且已充分验证的字面量规则
 *   在各使用点自然生效：
 *     - 整数上下文：`16 - 1` 因相邻运算符判定保持整数（规则 B 会被
 *       stmt_has_int_ident / 运算符邻接性正确抑制）
 *     - 浮点上下文：`16` 变成 `16.0`
 *   完全等价于预处理器展开后的结果，不需要任何新的类型推断。
 *
 * 【严格约束：只展开「宏体是单一整数字面量」的对象式宏】
 *   - 只认十进制 / 十六进制整数字面量（可带 u 后缀）
 *   - 名字后紧跟 '(' 的是函数式宏，跳过（展开需要处理参数，风险大）
 *   - 宏体含运算符/多 token/续行的跳过
 *   - 如果同一名字在源码里被 #undef 或重复 #define 成不同值，一律跳过
 *     （宁可漏改，不可误改）
 *   - 展开后删除 #define 行。这会让源码里**不再有该名字**，
 *     因此后续所有以「标识符不在已知表里就当作浮点」为判据的逻辑
 *     不再被这个宏名干扰。
 */
#define INT_MACRO_MAX 64
typedef struct {
    char   name[64];
    char   body[32];    /* 只可能是整数字面量，32 字节绰绰有余 */
} int_macro_def;

static int_macro_def g_int_macros[INT_MACRO_MAX];
static int            g_int_macro_count = 0;

/*
 * 两个辅助函数的定义在文件后半部分，而宏展开在本文件靠前处就要用到，
 * 因此在此前向声明（否则会出现 implicit declaration / conflicting types）。
 */
static const char *skip_comment_or_string(const char *p, const char *end);
static int int_literal_token_is_decimal_or_hex(const char *b, size_t n);

/* 行首（允许前导空白）是否为指令；*z 指向 '#'，*le 指向行尾（去 CR/空白） */
static int line_is_directive(const char *p, const char *nl, const char *end,
                             const char **z_out, const char **le_out) {
    const char *le = nl ? nl : end;
    while (le > p && (le[-1] == '\r' || le[-1] == ' ' || le[-1] == '\t')) le--;
    const char *z = p;
    while (z < le && (*z == ' ' || *z == '\t')) z++;
    *z_out = z;
    *le_out = le;
    return (z < le && *z == '#');
}

/* 该名字在源码里是否被 #undef 过，或存在宏体不同的另一个 #define */
static int macro_name_ambiguous(const char *src, const char *end,
                                const char *name, size_t nlen,
                                const char *body, size_t blen) {
    const char *scan = src;
    while (scan < end) {
        const char *nl = memchr(scan, '\n', (size_t)(end - scan));
        const char *z = NULL, *le = NULL;
        line_is_directive(scan, nl, end, &z, &le);
        const char *next = nl ? nl + 1 : end;

        int is_def = ((size_t)(le - z) >= 8 && strncmp(z, "#define", 7) == 0);
        int is_undef = ((size_t)(le - z) >= 6 && strncmp(z, "#undef", 6) == 0);
        if (is_def || is_undef) {
            const char *nm = z + (is_def ? 7 : 6);
            while (nm < le && (*nm == ' ' || *nm == '\t')) nm++;
            const char *ne = nm;
            while (ne < le && is_ident_char(*ne)) ne++;
            if ((size_t)(ne - nm) == nlen && strncmp(nm, name, nlen) == 0) {
                if (is_undef) return 1;
                const char *b = ne;
                while (b < le && (*b == ' ' || *b == '\t')) b++;
                const char *be = b;
                while (be < le && is_ident_char(*be)) be++;
                if ((size_t)(be - b) != blen || strncmp(b, body, blen) != 0) {
                    return 1;
                }
            }
        }
        scan = next;
    }
    return 0;
}

static void expand_int_const_macros(sbuf *out, const char *src) {
    const char *end = src + strlen(src);

    /* ---------------- 阶段 1：收集候选宏 ---------------- */
    g_int_macro_count = 0;
    {
        const char *p = src;
        while (p < end) {
            const char *nl = memchr(p, '\n', (size_t)(end - p));
            const char *z = NULL, *le = NULL;
            const char *next = nl ? nl + 1 : end;
            if (line_is_directive(p, nl, end, &z, &le) &&
                (size_t)(le - z) >= 8 && strncmp(z, "#define", 7) == 0 &&
                (z[7] == ' ' || z[7] == '\t')) {
                const char *nm = z + 7;
                while (nm < le && (*nm == ' ' || *nm == '\t')) nm++;
                const char *ne = nm;
                while (ne < le && is_ident_char(*ne)) ne++;
                if (ne > nm && (size_t)(ne - nm) < sizeof(g_int_macros[0].name)) {
                    const char *k = ne;
                    while (k < le && (*k == ' ' || *k == '\t')) k++;
                    if (k >= le || *k != '(') {       /* 排除函数式宏 */
                        const char *b = k;
                        const char *be = b;
                        while (be < le && is_ident_char(*be)) be++;
                        size_t blen = (size_t)(be - b);
                        const char *t = be;
                        while (t < le && (*t == ' ' || *t == '\t')) t++;
                        if (blen > 0 && blen < sizeof(g_int_macros[0].body) &&
                            t >= le &&
                            int_literal_token_is_decimal_or_hex(b, blen) &&
                            !macro_name_ambiguous(src, end, nm,
                                                  (size_t)(ne - nm), b, blen) &&
                            g_int_macro_count < INT_MACRO_MAX) {
                            int_macro_def *d = &g_int_macros[g_int_macro_count++];
                            memcpy(d->name, nm, (size_t)(ne - nm));
                            d->name[ne - nm] = '\0';
                            memcpy(d->body, b, blen);
                            d->body[blen] = '\0';
                        }
                    }
                }
            }
            p = next;
        }
    }

    if (g_int_macro_count == 0) {
        sbuf_puts(out, src);
        return;
    }

    /*
     * ---------------- 阶段 2：只替换「非指令行」中的宏名 ----------------
     *
     * 【两条不能违反的约束 —— 都是实测踩出来的】
     *
     *  1. **绝不删除 #define 行**。
     *     Sodium 的着色器里有配套的守卫：
     *         #define MAX_TEXTURE_LOD_BIAS 16
     *         ...
     *         #ifndef MAX_TEXTURE_LOD_BIAS
     *         #error "MAX_TEXTURE_LOD_BIAS constant not specified"
     *         #endif
     *     一旦删掉定义，`#ifndef` 变为成立，#error 立刻触发，
     *     整个着色器编译失败 —— 比原缺陷更严重。
     *     保留定义没有任何副作用：展开后已没有其它使用点。
     *
     *  2. **绝不改动任何预处理器指令行**（`#` 开头的行）。
     *     `#ifdef` / `#if` / `#ifndef` 里的宏名由预处理器按定义求值，
     *     若被我们替换成字面量，会产出 `#ifdef 16` 这种非法指令。
     *     只有普通代码行才做文本替换。
     */
    {
        const char *p = src;
        while (p < end) {
            const char *nl = memchr(p, '\n', (size_t)(end - p));
            const char *le = nl ? nl : end;

            /* 指令行：原样复制 */
            const char *z = p;
            while (z < le && (*z == ' ' || *z == '\t')) z++;
            if (z < le && *z == '#') {
                sbuf_put(out, p, (size_t)(le - p));
                if (nl) sbuf_put(out, "\n", 1);
                p = nl ? nl + 1 : end;
                continue;
            }

            /* 普通行：把行内（注释/字符串之外）的宏名替换为字面量 */
            {
                const char *q = p;
                while (q < le) {
                    const char *sk = skip_comment_or_string(q, le);
                    if (sk != NULL) {
                        sbuf_put(out, q, (size_t)(sk - q));
                        q = sk;
                        continue;
                    }
                    if (!is_ident_char(*q)) { sbuf_put(out, q, 1); q++; continue; }
                    const char *qe = q;
                    while (qe < le && is_ident_char(*qe)) qe++;
                    size_t nl2 = (size_t)(qe - q);
                    int replaced = 0;
                    for (int i = 0; i < g_int_macro_count; i++) {
                        if (strlen(g_int_macros[i].name) == nl2 &&
                            strncmp(g_int_macros[i].name, q, nl2) == 0) {
                            sbuf_puts(out, g_int_macros[i].body);
                            replaced = 1;
                            break;
                        }
                    }
                    if (!replaced) sbuf_put(out, q, nl2);
                    q = qe;
                }
            }
            if (nl) sbuf_put(out, "\n", 1);
            p = nl ? nl + 1 : end;
        }
    }
}

/*
 * 把 #error 的消息规范化为 GLSL ES 合法的 token 序列。
 *
 * 桌面 GLSL 习惯写成：
 *     #error "Vertex compression must be enabled"
 * 但 GLSL ES 的预处理器没有字符串字面量。Mali 的词法器即使在
 * 非活动分支里也会扫描到引号：
 *     2:65: L0001: Unknown character '"'(34)
 * 于是整个着色器编译失败（Sodium + Iris 进入世界时必现）。
 *
 * 这里只保留标识符字符与单空格：消息仍然可读，指令本身不变，
 * 活动分支依旧会按原意触发 #error。
 */
#ifndef GLESMOD_NO_ERROR_DIRECTIVE_FIX
static void emit_error_message_tokens(sbuf *out, const char *msg, size_t n) {
    int emitted = 0;
    int pending_space = 0;

    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)msg[i];
        int keep = (c >= 'A' && c <= 'Z') ||
                   (c >= 'a' && c <= 'z') ||
                   (c >= '0' && c <= '9') ||
                   c == '_';
        if (keep) {
            char ch = (char)c;
            if (pending_space && emitted) sbuf_put(out, " ", 1);
            sbuf_put(out, &ch, 1);
            emitted = 1;
            pending_space = 0;
        } else {
            pending_space = 1;
        }
    }
}
#endif  /* GLESMOD_NO_ERROR_DIRECTIVE_FIX */

/*
 * 预处理器指令规范化。
 *
 * 桌面 GLSL 与 GLSL ES 的预处理器并不完全等价。这里只处理两类
 * 已知差异，且都保持「行数不变」，以免破坏 #line 映射：
 *
 *   1. 桌面专属 #extension（例如 GL_ARB_separate_shader_objects）
 *      -> 注释掉；
 *   2. #error 的字符串消息（GLSL ES 没有字符串字面量）
 *      -> 规范化为合法 token。
 *
 * 不做条件分支求值：宏由驱动预处理器负责，我们越权求值只会制造
 * 新的兼容性问题。
 */
static void normalize_preprocessor_directives(sbuf *out, const char *src) {
    const char *p = src;
#ifndef GLESMOD_NO_ERROR_DIRECTIVE_FIX
    int error_marker_emitted = 0;
#endif

    while (*p != '\0') {
        const char *eol = strchr(p, '\n');
        size_t line_len = eol ? (size_t)(eol - p + 1) : strlen(p);
        const char *line_end = p + line_len;

        const char *q = p;
        while (q < line_end && (*q == ' ' || *q == '\t')) q++;

        if (strncmp(q, "#extension", 10) == 0) {
            /* 取出扩展名 */
            const char *name = q + 10;
            while (*name == ' ' || *name == '\t') name++;
            int keep = (strncmp(name, "GL_ES", 5) == 0 ||
                        strncmp(name, "GL_OES", 6) == 0 ||
                        strncmp(name, "GL_EXT", 6) == 0 ||
                        strncmp(name, "GL_KHR", 6) == 0 ||
                        strncmp(name, "GL_ANGLE", 8) == 0 ||
                        strncmp(name, "GL_NV", 5) == 0 ||
                        strncmp(name, "GL_QCOM", 7) == 0 ||
                        strncmp(name, "GL_ARM", 6) == 0);
            if (!keep) {
                /* 换成同样长度的注释，保住后续行号，便于读驱动报错 */
                sbuf_puts(out, "//");
                if (line_len > 2) sbuf_puts(out, " glesmod: removed desktop-only #extension");
                sbuf_put(out, "\n", 1);
                p += line_len;
                continue;
            }
        }

#ifndef GLESMOD_NO_ERROR_DIRECTIVE_FIX
        if (strncmp(q, "#error", 6) == 0 &&
            (q + 6 >= line_end || !is_ident_char(q[6]))) {
            const char *msg = q + 6;
            while (msg < line_end && (*msg == ' ' || *msg == '\t')) msg++;

            const char *msg_end = line_end;
            while (msg_end > msg &&
                   (msg_end[-1] == '\n' || msg_end[-1] == '\r' ||
                    msg_end[-1] == ' ' || msg_end[-1] == '\t')) {
                msg_end--;
            }

            /* 保留前导空白与 #error 本身 */
            sbuf_put(out, p, (size_t)((q + 6) - p));
            if (msg < msg_end) {
                sbuf_put(out, " ", 1);
                emit_error_message_tokens(out, msg, (size_t)(msg_end - msg));
            }
            sbuf_put(out, "\n", 1);

            if (!error_marker_emitted) {
                error_marker_emitted = 1;
                glesmod_log("预处理器: #error 消息已规范化为 GLSL ES 合法 token"
                            "（GLESMOD_ERROR_DIRECTIVE_NORMALIZED）");
            }

            p += line_len;
            continue;
        }
#endif

        sbuf_put(out, p, line_len);
        p += line_len;
    }
}

/* ------------------------------------------------------------------ */
/* 词法安全替换                                                        */
/* ------------------------------------------------------------------ */

/*
 * 判断 pos 处是否为标识符边界。
 * 避免把 mytexture2D 中的 texture2D 也替换掉。
 */
static int is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

/*
 * 全词替换：把 src 中所有独立的 from 替换为 to。
 *
 * 注意：会跳过注释与字符串字面量，避免误改。
 */
static void replace_word(sbuf *out, const char *src, const char *from, const char *to) {
    size_t from_len = strlen(from);
    const char *p = src;

    while (*p != '\0') {
        /* 跳过行注释 */
        if (p[0] == '/' && p[1] == '/') {
            const char *e = strchr(p, '\n');
            size_t n = e ? (size_t)(e - p + 1) : strlen(p);
            sbuf_put(out, p, n);
            p += n;
            continue;
        }
        /* 跳过块注释 */
        if (p[0] == '/' && p[1] == '*') {
            const char *e = strstr(p + 2, "*/");
            size_t n = e ? (size_t)(e - p + 2) : strlen(p);
            sbuf_put(out, p, n);
            p += n;
            continue;
        }
        /* 跳过字符串字面量 */
        if (*p == '"') {
            const char *e = p + 1;
            while (*e != '\0' && *e != '"') {
                if (*e == '\\' && e[1] != '\0') e++;
                e++;
            }
            size_t n = (size_t)(e - p) + (*e == '"' ? 1 : 0);
            sbuf_put(out, p, n);
            p += n;
            continue;
        }

        /* 标识符匹配 */
        if (strncmp(p, from, from_len) == 0) {
            int left_ok  = (p == src) || !is_ident_char(p[-1]);
            int right_ok = !is_ident_char(p[from_len]);
            if (left_ok && right_ok) {
                sbuf_puts(out, to);
                p += from_len;
                continue;
            }
        }

        sbuf_put(out, p, 1);
        p++;
    }
}

/* ------------------------------------------------------------------ */
/* 整数字面量浮点化                                                    */
/* ------------------------------------------------------------------ */

/*
 * 【为什么必须做这一步 —— 这是桌面 GLSL 与 GLSL ES 的核心差异】
 *
 * 桌面 GLSL 允许在二元运算中隐式把 int 转为 float：
 *     vec2 * 2        // 桌面合法（2 隐式当作 2.0）
 * 而 GLSL ES（含 3.00 / 3.20）不允许 —— ES 只允许在「构造函数」里做隐式转换：
 *     vec2(2, 3)      // ES 合法
 *     vec2 * 2        // ES 非法：
 *                     //   no operation '*' exists that takes a left-hand
 *                     //   operand of type '2-component vector of float'
 *                     //   and a right operand of type 'const int'
 *
 * 这正是 FML 早期显示窗口在真机上启动失败的根因。它的顶点着色器
 * （#version 150 core）写着：
 *     gl_Position = vec4((position/screenSize) * 2 - 1, 0.0, 1.0);
 * 该表达式在桌面上完全正常，在 ES 上必然编译失败。
 *
 * 注意：把 #version 提升到 320 es 并不能解决它 —— 这是语言层面的规则差异，
 * 不是版本高低的问题。必须改写字面量本身。
 * 改写是语义等价的：小整数在 float 中可以精确表示。
 *
 * 【安全边界】
 * 改写必须严格限定在「确定是浮点上下文」的位置，否则会把
 *     int count = 0;      // 若 0 变成 0.0，int 声明就出错了
 * 这种正确代码改坏。因此判定依据只有三条：
 *   1. 整数字面量所在的【语句】（以 ; { } 分隔）中出现过浮点字面量
 *   2. 整数字面量所在的语句声明了 float 类型
 *   3. 整数字面量位于 vecN / matN 构造函数的实参列表内（这些必然是浮点）
 * 并且以下位置一律不改写：
 *   - ivecN / uvecN / bvecN 构造函数内
 *   - 数组下标 [ ] 内
 *   - 预处理器行（# 开头）
 *   - 标识符的一部分（vec4、mat3、sampler2D 中的数字）
 */

/* 浮点构造函数名：出现在其实参列表内的整数必然是浮点上下文 */
static const char *const FLOAT_CTORS[] = {
    "vec2", "vec3", "vec4",
    "mat2", "mat3", "mat4",
    "mat2x2", "mat2x3", "mat2x4",
    "mat3x2", "mat3x3", "mat3x4",
    "mat4x2", "mat4x3", "mat4x4",
};

/* 整数构造函数名：其内部绝不改写 */
static const char *const INT_CTORS[] = {
    /*
     * 【必须包含 int / uint 这两个**标量**构造函数】
     *   真机证据（Iris 导出的 patched_shaders/001_deferred1.vsh）：
     *       modifiedWorldDay = int(mod(worldDay, 100) + 5);
     *   `int(...)` 的实参本身就是整数域，其中的 `5` 绝不能浮点化。
     *   原先只列了 ivecN / uvecN / bvecN，漏掉标量 `int(...)`，
     *   于是 `5` 被改成 `5.0f`，反而制造出 `int + float`：
     *       '+' : no operation '+' exists that takes a left-hand
     *             operand of type 'const int' and a right operand of type 'float'
     *   （随后 `int(...)` 的结果推不出来，`mod` 也跟着报无匹配重载。）
     *   这类「标量构造函数漏进整数表」的缺陷与 vecN 完全同源。
     */
    "int", "uint",
    "ivec2", "ivec3", "ivec4",
    "uvec2", "uvec3", "uvec4",
    "bvec2", "bvec3", "bvec4",
};

/*
 * 只接受浮点实参的内建函数名。
 *
 * 【用途】两处都需要它：
 *   1. 判断某个整数变量是否作为**浮点函数的实参**（`min(float, int)`）
 *   2. 判断整数字面量是否与**浮点函数调用**直接相邻（`min(...) + 1`）
 *
 * 刻意只收这一批「参数类型必须一致」的函数，不做通用推断：
 * `max(int, int)` 是合法的，误加 float() 会把合法代码改坏。
 * 宁可漏改，也不能误改。
 */
static const char *const FLOAT_BUILTIN_FNS[] = {
    "min", "max", "clamp", "mix", "step", "smoothstep",
    "pow", "mod", "radians", "degrees", "distance", "length",
    "dot", "normalize", "reflect", "refract", "faceforward",
    "exp", "exp2", "log", "log2", "sqrt", "inversesqrt",
    "abs", "sign", "floor", "ceil", "fract",
    "sin", "cos", "tan", "asin", "acos", "atan",
    "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
    "texture", "textureLod", "textureProj",
    "exp2", "fma",
    /*
     * 【★ 必须有 NULL 终止符 ★ —— 真机 SIGSEGV 的根因】
     *
     * 本表的所有遍历点都写成
     *     for (int i = 0; FLOAT_BUILTIN_FNS[i] != NULL; i++) {
     *         ... strlen(FLOAT_BUILTIN_FNS[i]) ...
     *     }
     * 一旦漏掉终止符，循环就会越过数组末尾继续把相邻内存当作
     * `const char *` 解引用，随后在 `strlen` 上 SIGSEGV。
     *
     * 真机表现（2026-10-06，Flywheel shader=220）：
     *     SIGSEGV (0xb) at pc=...  C  [libc.so+0x6bf90]  __strlen_aarch64+0x10
     * 且崩在 glShaderSource 的入口（dump 都来不及写）。
     *
     * 【为什么宿主机单测没能发现】
     *   同样的遍历在 x86-64/Linux 上也越界了，但那块内存恰好可读，
     *   于是"正常通过"。**这是运气，不是正确性。**
     *   教训：任何以 NULL 结尾的表，都必须有机械化的自检
     *   （见 native/tools/test_table_terminators.c），
     *   不能依赖"跑起来没崩"。
     */
    NULL
};

#define LIST_LEN(a) (sizeof(a) / sizeof((a)[0]))

/*
 * ================== 数组长度宏：NULL 结尾的表要减一 ==================
 *
 * 【血的教训 —— 一次自己制造的 0xC0000005】
 *   本项目有两种表，混用会直接崩：
 *
 *   (A) **以 NULL 结尾**、用 `for (i = 0; table[i] != NULL; i++)` 遍历的表
 *       （FLOAT_BUILTIN_FNS / kFloatOnlyFns / kStrictFloatFns / kTypes ...）
 *   (B) **无终止符**、用 `name_in_list(..., LIST_LEN(table))` 遍历的表
 *       （FLOAT_CTORS / INT_CTORS / KWS ...）
 *
 *   给 (A) 类表补上 NULL 终止符后，**如果它同时也被 LIST_LEN 使用**，
 *   长度就多算了 1：`name_in_list` 会走到那个 NULL 元素上做 `strlen(NULL)`。
 *   Windows/MinGW 上这不是"读到野指针"而是**直接访问违规**
 *   （exit=0xC0000005，日志 0 字节），比真机那次崩溃更容易复现 ——
 *   也算幸运：宿主端终于把这类缺陷暴露出来了。
 *
 * 【两个宏的分工，必须严格遵守】
 *   LIST_LEN(t)      -> 表**没有** NULL 终止符
 *   LIST_LEN_TERM(t) -> 表**有** NULL 终止符（长度不含它）
 *
 *   新增表时先决定用哪种遍历方式，再选对应的宏。
 *   native/tools/test_table_terminators.c 会核对这个对应关系。
 */
#define LIST_LEN_TERM(a) (LIST_LEN(a) - 1)

static int name_in_list(const char *s, size_t len,
                        const char *const *list, size_t n) {
    for (size_t i = 0; i < n; i++) {
        /*
         * 防御：即使调用方把长度算多了（或某张表既带 NULL 又被 LIST_LEN
         * 使用），这里也不会对 NULL 调 strlen —— 后者在 Windows 上是
         * 直接访问违规，会让整个进程静默消失（exit=0xC0000005）。
         */
        if (list[i] == NULL) break;
        size_t li = strlen(list[i]);
        if (li == len && strncmp(s, list[i], len) == 0) return 1;
    }
    return 0;
}

/*
 * 判断 p 处是否为浮点字面量，返回其长度（0 表示不是）。
 * 识别形式：1.0   1.   .5   1e3   1.5e-2
 *
 * ============ 必须识别 `f` / `F` 后缀（BSL 全篇都这么写） ============
 *
 * 【真机证据 —— 一次完整复现】
 *   BSL v10.1.8 的 patched_shaders/001_deferred1.fsh 里，
 *   **所有**浮点字面量都带 f 后缀：
 *       color += 0.25f * lightCol * visibility * (1.0f + 0.25f * isEyeInWater);
 *       ... max(weatherWeight, 1.0E-4f) ...
 *   真机报：
 *       ERROR: 0:1492: '*' : no operation '*' exists that takes a left-hand
 *             operand of type 'const float' and a right operand of type
 *             'uniform int'
 *   原因就是本函数原先**不认识 f 后缀**，解析出 `0.25` 后在 `f` 处停下，
 *   于是 `0.25f` 被当成「0.25 后面跟一个标识符 f」，
 *   整个浮点检测链条全部失效 —— 规则 B/C 都判不出右侧是浮点。
 *
 *   更糟的是 int_literal_len 明确写了 `if (isalpha(*q)) return 0;`，
 *   于是 `3u`（uint 字面量）也被当成「不是整数」，
 *   两个方向的判断同时失效。
 *
 *   GLSL ES 3.20 规范 4.1.3 明确允许 float 字面量的 `f`/`F` 后缀，
 *   以及 uint 字面量的 `u`/`U` 后缀。必须解析它们。
 */
static size_t float_literal_len(const char *p) {
    const char *q = p;
    if (*q == '.') {
        /* .5 形式 */
        q++;
        if (!isdigit((unsigned char)*q)) return 0;
        while (isdigit((unsigned char)*q)) q++;
    } else if (isdigit((unsigned char)*q)) {
        while (isdigit((unsigned char)*q)) q++;
        if (*q != '.') {
            /* 纯整数，但可能是 1e3 这种浮点指数形式 */
            if (*q != 'e' && *q != 'E') return 0;
        } else {
            q++;
            while (isdigit((unsigned char)*q)) q++;
        }
    } else {
        return 0;
    }
    if (*q == 'e' || *q == 'E') {
        const char *e = q + 1;
        if (*e == '+' || *e == '-') e++;
        if (isdigit((unsigned char)*e)) {
            while (isdigit((unsigned char)*e)) e++;
            q = e;
        }
    }
    /* 吃掉 f / F 后缀 —— 必须算作字面量的一部分 */
    if (*q == 'f' || *q == 'F') q++;
    return (size_t)(q - p);
}

/*
 * 判断 p 处是否为纯整数字面量，返回其长度（0 表示不是）。
 *
 * 允许：123、3u、7U
 * 排除：后跟 '.' 或 'e'（那是浮点）、后跟 f/F（那是浮点）、
 *       后跟其它字母（如 2x 这种非法或宏）
 *
 * 【原实现的两个错误】
 *   1. `if (isalpha(*q)) return 0;` 会把 `3u` 判定为「不是整数」。
 *      uint 字面量是 GLSL ES 明确支持的，必须识别。
 *   2. 未区分 f/F 与 u/U —— 前者是浮点后缀（必须排除），
 *      后者是整数后缀（必须接受）。
 */
static size_t int_literal_len(const char *p) {
    const char *q = p;
    if (!isdigit((unsigned char)*q)) return 0;
    while (isdigit((unsigned char)*q)) q++;
    if (*q == '.' || *q == '_') return 0;
    if (*q == 'u' || *q == 'U') {
        q++;                    /* uint 字面量：连同后缀一起算 */
        if (isalnum((unsigned char)*q) || *q == '.') return 0;
        return (size_t)(q - p);
    }
    if (*q == 'f' || *q == 'F') return 0;   /* 浮点后缀，不是整数 */
    if (*q == 'e' || *q == 'E') return 0;   /* 指数，是浮点 */
    if (isalpha((unsigned char)*q)) return 0;
    return (size_t)(q - p);
}

/* ------------------------------------------------------------------ */
/* 整数标识符表（用于避免把「整数表达式的字面量」浮点化）              */
/* ------------------------------------------------------------------ */

/* 前向声明：定义在后面，但扫描/处理阶段都要用它跳过注释与字符串。 */
static const char *skip_comment_or_string(const char *p, const char *end);

/*
 * 前向声明：定义在文件后半部分。
 * 规则 P（fix_mixed_int_uint）与 normalize_int_literals 都要用它按行隔离
 * 预处理器指令 —— 改写其中的字面量或运算符必然产出非法指令。
 */
static int line_starts_with_directive(const char *p, const char *e);

/*
 * 【真实故障 —— 这是本项目最隐蔽的一个转换器缺陷】
 *
 * MC 的 rendertype_end_portal.fsh 里有：
 *     end_portal_layer(float(i + 1))      // i 声明为 int
 *
 * 原始做法把 `1` 浮点化，产出：
 *     end_portal_layer(float(i + 1.0))
 * 而 `i + 1.0` 是 **int + float** —— GLSL ES 禁止（已用 glslang 实测确认），
 * 于是 end_portal 着色器编译失败。
 *
 * 讽刺的是 `float(i + 1)` 本来是**合法**的（float() 是转换构造函数，
 * 已由 glslang 实测确认）；正是转换器把它改坏了。
 *
 * 【根因】
 *   判定「整数是否要浮点化」只看了「字面量左右紧邻的字符」，
 *   没有看**另一个操作数的类型**。`i + 1` 里 `i` 是 int，
 *   因此 `1` 应保持整数。只有两个操作数都非整数时，
 *   `int字面量 + float字面量` 才需要浮点化 ——
 *   这种写法在 GLSL ES 中同样非法（glslang 实测：S4 禁止），
 *   而桌面 GLSL 允许（这也是 normalize_int_literals 存在的理由）。
 *
 * 【修复】
 *   收集源码里声明为 int/uint 的标识符，构成一张表。
 *   若某条语句中出现该表中的标识符，则其整数【字面量】不得浮点化
 *   （表达式为整数域）；但真正的浮点构造实参 vecN/matN(...) 例外，
 *   那里的字面量仍可安全浮点化（glslang 实测 V6/V7：允许）。
 *
 *   这比此前「只把它记为已知局限」的做法更正确，也不会误伤
 *   本来就正确的着色器（浮点语句里通常不会出现 int 变量）。
 */
#define INT_IDENT_MAX 512

typedef struct {
    char name[64];
} int_ident;

typedef struct {
    int_ident items[INT_IDENT_MAX];
    int       count;
    int       saturated;   /* 超限置 1；饱和后 stmt_has_int_ident 不再可靠 */
} int_ident_table;

static int int_ident_find(const int_ident_table *t,
                          const char *s, size_t len) {
    if (t == NULL) return 0;
    for (int i = 0; i < t->count; i++) {
        if (strlen(t->items[i].name) == len &&
            strncmp(t->items[i].name, s, len) == 0) {
            return 1;
        }
    }
    return 0;
}

static void int_ident_add(int_ident_table *t, const char *s, size_t len) {
    if (t == NULL || len == 0 || len >= sizeof(t->items[0].name)) return;
    if (int_ident_find(t, s, len)) return;
    if (t->count >= INT_IDENT_MAX) { t->saturated = 1; return; }
    memcpy(t->items[t->count].name, s, len);
    t->items[t->count].name[len] = '\0';
    t->count++;
}

/*
 * 扫描全部源码，收集声明为 int / uint / ivecN / uvecN / bvecN 的变量名。
 *
 * 【为什么必须包含整数**向量** —— 一次严重回归的教训】
 *   最初只收 int / uint 标量。于是 MC 的这条语句：
 *       in ivec2 UV2;
 *       ... texelFetch(Sampler2, UV2 / 16, 0)
 *   里 `UV2` 不在整数变量表中，stmt_has_int_ident 判为假，
 *   新加的「规则 B'」就把 `16` 浮点化成 `16.0`，
 *   又配合情形 B 的 `vec2(UV2)` 包装，最终产出：
 *       texelFetch(Sampler2, vec2(UV2) / 16.0, 0)
 *   texelFetch 的第二参数必须是 **ivec2**，于是整个 MC 着色器集合里
 *   19 个顶点着色器编译失败：
 *       'texelFetch' : no matching overloaded function found
 *   回归门禁（audit-mc-shaders.ps1）当场抓住。
 *
 *   结论：只要语句里出现**任何整数类型**的变量，其整数字面量就必须
 *   保持整数 —— 标量与向量一视同仁。
 *
 * 覆盖常见声明形态：
 *     int i;          int i = 0;       int i, j;
 *     uniform int n;  in int k;        flat in int m;
 *     for (int i = 0; ...)
 *     in ivec2 UV2;   uniform uvec3 u; bvec4 b;
 *
 * 检测方式：遇到词法边界上的类型关键字后，取紧随其后的标识符。
 * `into` 之类不会被误判（词法边界检查）。
 *
 * ============ 规则 P 为什么必须用「只要标量」的那一份 ============
 *   本函数可以生成两张表，取舍由 include_vectors 决定：
 *
 *   include_vectors = 1（Rule B' 用）
 *     整数**字面量**是否浮点化，只看语句里有没有整数变量。
 *     `in ivec2 UV2; ... UV2 / 16` 里 16 必须是整数，
 *     所以向量必须算数 —— 少收向量会直接毁掉 MC 的 19 个顶点着色器
 *     （见上面那段 texelFetch 的教训）。
 *
 *   include_vectors = 0（规则 P 用）—— **本次真机材质错乱的根因**
 *     Sodium 0.8.13 的区块顶点着色器（渲染静态方块）里有：
 *         in uvec2 a_TexCoord;
 *         return vec2(a_TexCoord & TEXTURE_MAX_VALUE) / float(TEXTURE_MAX_COORD);
 *     `a_TexCoord` 被向量表收进去后，规则 P 把左操作数判成
 *     「确定是标量 int」，而右操作数 `TEXTURE_MAX_VALUE` 是真 uint，
 *     于是判定「混合 int/uint」，包成了：
 *         return vec2(uint(a_TexCoord) & TEXTURE_MAX_VALUE) / ...
 *
 *     GLSL ES 里 `uint(uvec2)` 是**合法**的（glslang 不报错），
 *     但它等价于「取第 0 分量」——实测 SPIR-V：
 *         原始 : OpIAnd %v2uint  ->  OpConvertUToF %v2float          = (u, v)
 *         改写 : OpCompositeExtract %uint %14 0                      = u
 *                OpCompositeConstruct %v2float %u %u                = (u, u)
 *     贴图坐标从 (u,v) 塌成 (u,u) —— 纹理沿对角线采样，
 *     于是**静态方块材质全错，且完全静默**（不报错、不崩溃、日志干净）。
 *     这正是 2026-10-05 15:2x 那轮真机测试的现象。
 *
 *     修法：规则 P 只接受**标量**整数名。向量一律不参与，
 *     因为它只会包 uint(...)（标量），而操作数是向量时本就无需改写。
 *     另加一道保险：带分量选择（`.z` 等）的操作数必然来自向量，直接放弃。
 */
/*
 * 判断一个 token 是否是「纯整数字面量」：十进制 或 十六进制，可带 u/U 后缀。
 *
 * 用于 `#define NAME <integer literal>` 的识别（见 scan_int_idents_core）。
 * 刻意不接受浮点字面量（`1.0`、`1e3f`）、也不接受字符/字符串 ——
 * 那些宏的名字参与运算时类型不是 int，登记进去会导致误判。
 */
static int int_literal_token_is_decimal_or_hex(const char *b, size_t n) {
    if (n == 0) return 0;
    size_t i = 0;
    if (n >= 3 && b[0] == '0' && (b[1] == 'x' || b[1] == 'X')) {
        i = 2;
        if (i >= n) return 0;
        size_t digits = 0;
        for (; i < n; i++) {
            if (isxdigit((unsigned char)b[i])) { digits++; continue; }
            break;
        }
        if (digits == 0) return 0;
        if (i == n) return 1;
        if (i == n - 1 && (b[i] == 'u' || b[i] == 'U')) return 1;
        return 0;
    }
    size_t digits = 0;
    for (; i < n; i++) {
        if (isdigit((unsigned char)b[i])) { digits++; continue; }
        break;
    }
    if (digits == 0) return 0;
    if (i == n) return 1;
    if (i == n - 1 && (b[i] == 'u' || b[i] == 'U')) return 1;
    return 0;
}

static void scan_int_idents_core(const char *src, int_ident_table *t,
                                 int include_vectors) {
    /*
     * 先把 GLSL 的**内建整数变量**登记进去。
     *
     * 【为什么必须显式登记 —— MC 的 rendertype_lines.vsh 是最后一处回归】
     *     ... gl_VertexId % 2 ...
     *   `gl_VertexId` 是内建 int，源码里没有声明语句，扫描不到，
     *   于是 stmt_has_int_ident 为假，`2` 被浮点化成 `2.0`：
     *       '%' : no operation '%' exists that takes a left-hand operand of
     *             type 'gl_VertexId highp int' and a right operand of type
     *             'const float'
     *   这是唯一一个在 MC 125 个着色器回归里剩下的失败点。
     */
    static const char *const kIntBuiltins[] = {
        /*
         * 【大小写要写对】GLSL 规范里是 `gl_VertexID`、`gl_InstanceID`
         * （结尾两个大写字母 ID）。MC 的 rendertype_lines.vsh 用的正是
         * `gl_VertexID`；先前误写成 `gl_VertexId`，登记了不存在的名字，
         * 于是 `gl_VertexID % 2` 里的 2 仍被浮点化。
         * 两种拼写都登记，避免不同来源的着色器写法差异。
         */
        "gl_VertexID", "gl_VertexId", "gl_InstanceID", "gl_InstanceId",
        "gl_InstanceIndex", "gl_PrimitiveID", "gl_DrawID",
        "gl_BaseVertex", "gl_BaseInstance",
        "gl_Layer", "gl_ViewportIndex", "gl_SampleID", "gl_NumSamples",
        NULL
    };
    for (int i = 0; i < (int)LIST_LEN(kIntBuiltins) && kIntBuiltins[i] != NULL; i++) {
        int_ident_add(t, kIntBuiltins[i], strlen(kIntBuiltins[i]));
    }

    const char *end = src + strlen(src);

    /*
     * 【历史注记】这里曾把「宏体是整数字面量」的宏名登记为整数标识符，
     * 以修 Flywheel 的
     *     #define TRANSPARENCY_WAVELET_COEFFICIENT_COUNT 16
     * 但实测证明该做法是错的：同一个宏在源码里会同时出现在
     *     int   上下文：clamp(int(floor(d)), 0, COUNT - 1)
     *     float 上下文：depth * COUNT
     * GLSL ES 没有隐式转换，两侧需要**相反**的改写，因此不能把宏名
     * 一刀切定为 int。现在的做法是 expand_int_const_macros 在流水线
     * 早期把宏按定义文本展开成字面量，再由既有的字面量规则按位置判断。
     */

    const char *p = src;

    while (p < end) {
        const char *skipped = skip_comment_or_string(p, end);
        if (skipped != NULL) { p = skipped; continue; }
        if (!is_ident_char(*p)) { p++; continue; }

        const char *q = p;
        while (q < end && is_ident_char(*q)) q++;
        size_t len = (size_t)(q - p);

        int is_int_kw = (len == 3 && strncmp(p, "int", 3) == 0) ||
                        (len == 4 && strncmp(p, "uint", 4) == 0) ||
                        (len == 5 && strncmp(p, "ivec2", 5) == 0) ||
                        (len == 5 && strncmp(p, "ivec3", 5) == 0) ||
                        (len == 5 && strncmp(p, "ivec4", 5) == 0) ||
                        (len == 5 && strncmp(p, "uvec2", 5) == 0) ||
                        (len == 5 && strncmp(p, "uvec3", 5) == 0) ||
                        (len == 5 && strncmp(p, "uvec4", 5) == 0) ||
                        (len == 5 && strncmp(p, "bvec2", 5) == 0) ||
                        (len == 5 && strncmp(p, "bvec3", 5) == 0) ||
                        (len == 5 && strncmp(p, "bvec4", 5) == 0);

        /*
         * 规则 P 的表只收标量：向量声明在这里就跳过。
         * 标量判别只看关键字长度 —— int/uint 是 3/4 个字符，
         * ivecN/uvecN/bvecN 都是 5 个字符。
         */
        if (!include_vectors && len != 3 && len != 4) {
            p = q;
            continue;
        }
        if (is_int_kw) {
            const char *r = q;
            while (r < end && (*r == ' ' || *r == '\t' ||
                               *r == '\n' || *r == '\r')) {
                r++;
            }
            if (r < end && is_ident_char(*r) && !isdigit((unsigned char)*r)) {
                const char *s2 = r;
                while (s2 < end && is_ident_char(*s2)) s2++;
                size_t nl = (size_t)(s2 - r);
                int_ident_add(t, r, nl);
                p = s2;
                continue;
            }
        }
        p = q;
    }
}

/*
 * 规则 B' 专用：整数标量 + 整数向量都要收。
 * （`UV2 / 16` 这类语句里的 16 必须保持整数，见上面 texelFetch 的教训。）
 */
static void scan_int_idents(const char *src, int_ident_table *t) {
    scan_int_idents_core(src, t, 1);
}

/*
 * 规则 P（fix_mixed_int_uint）专用：**只收标量**。
 * 向量参与混合运算时无需改写（只有标量才会被包 uint(...)），
 * 而把向量误判成标量会产出 `uint(uvec2)` —— 合法但语义错误的
 * 「取第 0 分量」，静默毁掉贴图坐标。详见 scan_int_idents_core 的说明。
 */
static void scan_scalar_int_idents(const char *src, int_ident_table *t) {
    scan_int_idents_core(src, t, 0);
}

/*
 * 判断语句 [s, e) 是否处于浮点上下文。
 *
 * 判定依据（满足任一即可）：
 *   1. 出现浮点字面量（1.0 / .5 / 1e3）
 *   2. 声明了 float 类型
 *   3. 出现浮点向量/矩阵类型名（vecN / matN，需词法边界）
 *
 * 第 3 条是必要的：`vec2 scaled = uv * 3;` 这类语句既没有浮点字面量，
 * 也没有 "float" 关键字，但整个表达式的类型显然是浮点，
 * 其中的整数必须浮点化。注意必须检查词法边界 —— 否则 ivec2 里的
 * "vec2" 子串会被误判成浮点类型（ivec2 是整数向量）。
 */
static int stmt_is_float_context(const char *s, const char *e) {
    /* 条件 1：出现浮点字面量（要求处于词法边界） */
    for (const char *p = s; p < e; p++) {
        if (p > s && is_ident_char(p[-1])) continue;
        if (float_literal_len(p) > 0) return 1;
    }

    /* 条件 2：声明了 float 类型 */
    for (const char *p = s; p + 5 <= e; p++) {
        if (strncmp(p, "float", 5) != 0) continue;
        int left_ok  = (p == s) || !is_ident_char(p[-1]);
        int right_ok = (p + 5 >= e) || !is_ident_char(p[5]);
        if (left_ok && right_ok) return 1;
    }

    /* 条件 3：出现浮点向量/矩阵类型名（vecN / matN） */
    for (const char *p = s; p < e; p++) {
        if (p > s && is_ident_char(p[-1])) continue;   /* 排除 ivec2 等 */
        const char *q = p;
        while (q < e && is_ident_char(*q)) q++;
        size_t n = (size_t)(q - p);
        if (n > 0 && name_in_list(p, n, FLOAT_CTORS, LIST_LEN(FLOAT_CTORS))) {
            return 1;
        }
    }
    return 0;
}

/*
 * 判断位置 p 处的整数是否是一个【算术运算符】的操作数。
 *
 * 只看紧邻的左右两侧（跳过空白），是否存在二元算术运算符
 * （* / + -）或其一元形式（-x）。
 *
 * 【为什么必须限定为算术运算符】
 *   这排除了比值、比较与位运算，从而避免把
 *       rendertype == 0     (int 与 int 比较)
 *   误改成
 *       rendertype == 0.0   (int 与 float 比较，在 GLSL ES 中非法)
 *   FML 的片元着色器正是这种写法，且该语句里同时含 vec4，
 *   仅靠「语句级浮点上下文」判定会误伤。
 *
 * 【为什么必须识别注释 —— 这是一个真实故障】
 *   真机上出现过：
 *       #line 0 1        ->  #line 0 1.0       （非法指令，编译失败）
 *   原因：`1` 的右侧跨过换行后是下一行的块注释开头，
 *   而这里的判定把那个斜杠当成了【除法运算符】，于是把 1 改写成 1.0。
 *   因此判断运算符时必须排除「斜杠后面紧跟星号或斜杠」的情形
 *   （那是注释起始），也要排除「星号前面是斜杠」的情形（那是注释结束）。
 */
static int is_comment_open(const char *q, const char *e) {
    return (q + 1 < e) && q[0] == '/' && (q[1] == '*' || q[1] == '/');
}

static int is_comment_close(const char *q, const char *s) {
    /* q 指向 '*'，且其前一个字符是 '/' -> 这是块注释的结尾 */
    return (q > s + 1) && q[0] == '*' && q[-1] == '/';
}

/*
 * 判断位置 p 左侧紧邻的是不是一个**浮点内建函数调用的右括号**，
 * 允许中间隔一个算术运算符（那样 p 是右操作数）。
 *
 * 例：`min(a, b) + 1` 里的 `1` -> 跳过 '+' 后左侧是 min 的 ')' -> 命中
 *     `foo(int) + 1`    里的 `1` -> foo 不是已知浮点函数      -> 不命中
 *
 * 【为什么必须把「越过运算符」也算进来】
 *   BSL 的写法是 `int(min(a, b) + 1)`，`1` 的紧邻左字符是 '+' 而不是 ')'，
 *   若只看紧邻字符就永远不命中。真正决定 `1` 类型的是 `+` 的**另一侧**。
 *
 * 【为什么必须单独成函数】两处都要用它：
 *   1. int_is_arith_operand：决定整数字面量是否要浮点化
 *   2. process_stmt 的规则 C/E：语句里出现 int 变量时本应拒绝一切浮点化，
 *      但 `int samples = int(min(a,b) + 1);` 这种写法必须例外。
 */
static int left_is_float_builtin_call(const char *p, const char *s) {
    const char *l = p;
    while (l > s && (l[-1] == ' ' || l[-1] == '\t' ||
                     l[-1] == '\n' || l[-1] == '\r')) l--;
    if (l <= s) return 0;

    /* 允许中间隔一个算术运算符 */
    if (l[-1] == '+' || l[-1] == '-' || l[-1] == '*' || l[-1] == '/') {
        l--;
        while (l > s && (l[-1] == ' ' || l[-1] == '\t' ||
                         l[-1] == '\n' || l[-1] == '\r')) l--;
        if (l <= s) return 0;
    }
    if (l[-1] != ')') return 0;

    /* 从 ')' 反向配对到它的 '(' */
    const char *t = l - 1;
    int d2 = 0;
    while (t > s) {
        if (t[-1] == ')') { d2++; t--; continue; }
        if (t[-1] == '(') {
            if (d2 == 0) break;
            d2--; t--; continue;
        }
        t--;
    }
    if (t <= s || t[-1] != '(') return 0;

    const char *u = t - 1;
    while (u > s && (*u == ' ' || *u == '\t')) u--;
    const char *we = u;
    while (u > s && is_ident_char(u[-1])) u--;
    size_t fl2 = (size_t)(we - u);
    if (fl2 == 0) return 0;
    return name_in_list(u, fl2, FLOAT_BUILTIN_FNS,
                        LIST_LEN_TERM(FLOAT_BUILTIN_FNS));
}

/*
 * 下面两个 helper 定义在文件后半部分，
 * 而 int_ctor_lhs_is_float_operand 在本行之后就要用到它们，
 * 因此把前向声明放在这里（而非文件更下方 —— 那样会变成
 * "static declaration follows non-static declaration"）。
 */
static int ends_with_float_literal(const char *m, const char *s);
static const char *lhs_base_ident(const char *m, const char *s, size_t *len);

/*
 * 判断「整数字面量 p 是否位于 int(...) 内部、且其左侧的算术运算
 * 左操作数属于浮点域」。
 *
 * 【真机证据 —— BSL 018_terrain_solid.vsh 的第 141 行，整条管线因此失败】
 *     int blockID = int(mc_Entity.x / 100);
 *   真机报：
 *     '/' : no operation '/' exists that takes a left-hand operand of type
 *           'float' and a right operand of type 'const int'
 *   `int(...)` 在 INT_CTORS 表里 → int_ctor_depth > 0，
 *   而旧守卫只豁免「左侧是浮点内建函数调用」，`int` 不是浮点内建，
 *   于是 `100` 完全不进入任何规则。
 *
 * 【判据】
 *   1. 本字面量的紧邻左侧（跳过空白）是一个算术运算符；
 *   2. 该运算符的**左操作数**属于浮点域 —— 按两种形式识别：
 *        a) 以浮点字面量结尾（`1.0 / 100`）
 *        b) 是一个标识符（可带 .xy 这类 swizzle），且**不在整数变量表里**
 *           —— `mc_Entity.x / 100` 的 `mc_Entity` 是 vec2 变换输入，
 *           自然不在 int_idents 中，因此判为浮点。
 *
 * 【为什么用「不在整数表里」而不是「在浮点表里」】
 *   process_stmt 拿得到的是 int_idents，没有浮点变量表。
 *   反向判据在此更稳：GLSL 里绝大多数变量是浮点，
 *   而**确定是整数**的才会进 int_idents。误判方向也是安全的 ——
 *   即便某变量类型未知而被当作浮点，改写只是给整数字面量加 `.0`，
 *   在 `int(...)` 内部这个实参本来就是浮点域，不会产生新错误。
 *
 * 【为什么只在 int_ctor_depth > 0 时才需要】见调用点的说明：
 * 非构造函数上下文里 int_is_arith_operand 已经覆盖同一种形态。
 */
static int int_ctor_lhs_is_float_operand(const char *p, const char *s,
                                         const char *e,
                                         const int_ident_table *int_idents) {
    (void)e;
    /* 紧邻左侧必须是算术运算符 */
    const char *l = p;
    while (l > s && (l[-1] == ' ' || l[-1] == '\t' ||
                     l[-1] == '\n' || l[-1] == '\r')) l--;
    if (l <= s) return 0;
    {
        char c = l[-1];
        if (c != '+' && c != '-' && c != '*' && c != '/' && c != '%') return 0;
        /* 排除注释：'/' 后紧跟 '*' 或 '/' 不是除法 */
        if (c == '/' && l < e && (*l == '*' || *l == '/')) return 0;
    }

    /*
     * 取运算符的**左操作数末端**。
     *
     * 【这里曾经写错，值得记下来】
     *   第一版写的是
     *       const char *m = l - 1;
     *       while (m > s && (*m == ' ' || ...)) m--;
     *   条件用的是 *m 而**不是 m[-1]**。
     *   `l - 1` 已经落在运算符上（例如 `/`），再用 *m 判空白必然为假，
     *   于是 m 停在运算符本身，后面「取标识符基名」自然取不到东西，
     *   整个判据恒为假 —— `int(mc_Entity.x / 100)` 里的 `100` 因此从未被改写。
     *   正确做法与 left_is_float_builtin_call 保持一致：以**位置**看待 m，
     *   判据必须是 m[-1]。
     */
    const char *m = l - 1;
    while (m > s && (m[-1] == ' ' || m[-1] == '\t' ||
                     m[-1] == '\n' || m[-1] == '\r')) m--;
    if (m <= s) return 0;

    /* (a) 以浮点字面量结尾 */
    if (ends_with_float_literal(m, s)) return 1;

    /* (b) 标识符（跨过 .xy 这类 swizzle 取基名），且不是已知整型变量 */
    {
        size_t ul = 0;
        const char *u = lhs_base_ident(m, s, &ul);
        if (ul > 0) {
            if (int_idents == NULL || !int_ident_find(int_idents, u, ul)) {
                return 1;
            }
        }
    }
    return 0;
}

/* 定义在文件后半部分；规则 K 的声明处守卫要用它判定「整数族类型名」。 */
static int type_family_of_token(const char *s, size_t n);

/* 定义在文件后半部分；规则 J 要用它做「是否声明为浮点」的正向确认。 */
static int ident_declared_as_float_anywhere(const char *src, const char *end,
                                            const char *name, size_t nlen);

/* 定义在文件后半部分；规则 L 要用它判断三元某一侧是否为浮点。 */
static int starts_with_float_literal(const char *p, const char *end);

/* 规则 K 的判据，定义在 float_lhs_eq_int 附近。 */
static int is_assign_int_to_float(const char *p, const char *s,
                                  const char *e);

/*
 * 当前正在处理的着色器源码区间。
 *
 * 【为什么用文件作用域变量而不是加参数】
 *   规则 J（== / !=）与规则 K（floatVar = int）都需要「整源区间」来调用
 *   ident_declared_as_float_anywhere 做**正向**类型确认
 *   ——「查不到负面证据」不能当正面结论，这一点已经踩过一次
 *   （gl_VertexID 不在整数表里，被判成浮点，反而改坏 MC 着色器）。
 *   调用链（process_stmt -> 各规则）已经很长，逐层加参数要改十几处签名，
 *   风险大于收益；这里用只读的全程区间，在 normalize_int_literals 入口
 *   设置一次，整个转换过程中不再写入。
 */
static const char *g_src_begin = NULL;
static const char *g_src_end = NULL;

/*
 * 判断「整数字面量 p 的左侧是 `==` 或 `!=`，而该运算符的**左操作数确实
 * 被声明为浮点**」。
 *
 * 【真机证据 —— BSL 020_terrain_translucent.fsh 的第 3092 行】
 *     float cloudBlendOpacity = step(viewLength, cloudViewLength);
 *     if (cloudBlendOpacity == 0) discard;
 *   驱动报：
 *     '==' : no operation '==' exists that takes a left-hand operand of type
 *            'float' and a right operand of type 'const int'
 *
 * 【为什么必须单独成规则】
 *   情形 C 处理「整数在右、浮点在左」，但它的运算符集合只列出
 *   `+ - * / < >`，**刻意排除了 `=` 与 `!`**（当时的考虑是避开
 *   复合赋值和 ==/!= 的歧义）。于是 `==`、`!=` 这两个最常见的比较
 *   从未进入任何规则 —— 从第一轮排查遗留到现在。
 *
 * 【★判据必须是「正向确认浮点」，不能用「不在整数表里」★】
 *   第一版用「标识符不在 int_idents 表里 => 视为浮点」，结果把
 *   MC 自带着色器改坏了：
 *       gl_VertexID % 2 == 0.0
 *   `gl_VertexID` 是**内建变量**，扫描器只收集「声明出来的」整数名，
 *   它自然不在表里，于是 `0` 被当成浮点右侧改写成 `0.0`，
 *   反而造出 `int == float`：
 *       '==' : no operation '==' exists that takes a left-hand operand of
 *              type 'int' and a right operand of type 'const float'
 *   （MC 审计 125 个里当场失败 1 个，属于本轮引入的回归。
 *    教训：**「查不到负面证据」不能当作正面结论**。）
 *
 *   现在改为正向确认：用 ident_declared_as_float_anywhere 检查
 *   「该名字在源码里是否真的被声明为 float / vecN / matN」。
 *   因此：
 *       floatVar == 0        -> floatVar 确有浮点声明 -> 改写
 *       gl_VertexID == 0     -> 查不到浮点声明        -> 不改（正确）
 *       intVar == 0          -> 只有 int 声明         -> 不改（正确）
 */
static int float_lhs_eq_int(const char *p, const char *s,
                            const char *src_start, const char *src_end) {
    /* 紧邻左侧必须是 '=' 或 '!'，且其左边还有一个 '='（即 == / != ） */
    const char *l = p;
    while (l > s && (l[-1] == ' ' || l[-1] == '\t' ||
                     l[-1] == '\n' || l[-1] == '\r')) l--;
    if (l - 1 <= s) return 0;
    if (!(l[-1] == '=' && l[-2] == '=') && !(l[-1] == '=' && l[-2] == '!')) {
        return 0;
    }
    /* 排除 === 之类的三连符号（保守起见） */
    if (l - 2 > s && l[-3] == '=') return 0;

    /* 取左操作数 */
    const char *m = l - 2;
    while (m > s && (m[-1] == ' ' || m[-1] == '\t' ||
                     m[-1] == '\n' || m[-1] == '\r')) m--;
    if (m <= s) return 0;

    /* 左操作数是浮点字面量：`0.5 == 0`（少见，但一并支持） */
    if (ends_with_float_literal(m, s)) return 1;

    /* 左操作数是标识符（跨过 .xy 这类 swizzle 取基名）——必须正向确认浮点 */
    {
        size_t ul = 0;
        const char *u = lhs_base_ident(m, s, &ul);
        if (ul == 0) return 0;
        return ident_declared_as_float_anywhere(src_start, src_end, u, ul);
    }
}

/*
 * 规则 K 的判据：`floatVar = <int literal>;`（普通赋值，右值就是该字面量）。
 *
 * 见调用点处的详细说明。要点是必须**正向确认**左侧是浮点变量，
 * 否则 `intVar = 0;` 会被误改成 `0.0`，反而制造新的类型错误。
 */
static int is_assign_int_to_float(const char *p, const char *s,
                                  const char *e) {
    /* 1. 字面量右侧（跳空白与数字本身之后的空白）必须是语句结束 */
    const char *r = p;
    while (r < e && isdigit((unsigned char)*r)) r++;
    while (r < e && (*r == ' ' || *r == '\t' ||
                     *r == '\n' || *r == '\r')) r++;
    if (r < e) return 0;          /* 右边还有别的东西，不是「整体右值」 */

    /* 2. 左侧必须是普通的 '='（排除 == != <= >= += -= *= /= %=） */
    const char *l = p;
    while (l > s && (l[-1] == ' ' || l[-1] == '\t' ||
                     l[-1] == '\n' || l[-1] == '\r')) l--;
    if (l <= s || l[-1] != '=') return 0;
    if (l - 1 > s && (l[-2] == '=' || l[-2] == '!' ||
                      l[-2] == '<' || l[-2] == '>' ||
                      l[-2] == '+' || l[-2] == '-' ||
                      l[-2] == '*' || l[-2] == '/' || l[-2] == '%')) {
        return 0;
    }

    /* 3. 赋值号左侧的标识符必须被正向确认为浮点声明 */
    const char *m = l - 1;
    while (m > s && (m[-1] == ' ' || m[-1] == '\t' ||
                     m[-1] == '\n' || m[-1] == '\r')) m--;
    if (m <= s) return 0;

    size_t ul = 0;
    const char *u = lhs_base_ident(m, s, &ul);
    if (ul == 0) return 0;

    /*
     * 【声明处守卫 —— 必须放在按名字查表之前】
     *
     * 若左值本身就是**行内声明**（`int i = 0` / `uint n = 1` / `ivec2 v = ...`），
     * 那它的类型就写在眼前，是**确定事实**，绝不能改写 ——
     * 否则会产出 `int i = 0.0;`（ES 报 '=' : cannot convert
     * from 'const float' to 'temp highp int'）。
     *
     * 【真机证据 —— Veil 的 pinwheel 着色器】
     *   veil:voxel_shadow 里：
     *       for (int i = 0; i < VOXELSHADOW_MAX_STEPS; i++) { ... }
     *   同一文件另有浮点的 `i`（`vec3 i = ...` 之类），于是按【名字】的
     *   ident_declared_as_float_anywhere 判定为「浮点声明」，把 `0`
     *   改成 `0.0`，产出 `for (int i = 0.0; ...)`。
     *   这是**与本次同名污染同族**的缺陷：类型按名字全局推断，
     *   而声明处的类型其实是确定的。
     */
    {
        const char *ty = u;
        while (ty > s && (ty[-1] == ' ' || ty[-1] == '\t' ||
                          ty[-1] == '\n' || ty[-1] == '\r')) ty--;
        const char *tye = ty;
        while (ty > s && is_ident_char(ty[-1])) ty--;
        size_t tl = (size_t)(tye - ty);
        if (tl > 0 && type_family_of_token(ty, tl) == 1) return 0;  /* 整数族 */
    }

    return ident_declared_as_float_anywhere(g_src_begin, g_src_end, u, ul);
}

/*
 * ============ 三元运算符两侧类型统一（规则 L） ============
 *
 * 【真机证据 —— 本轮唯一错误，BSL hand_cutout.vsh】
 *     isMainHand = float(gl_Position.x * (isRightHanded ? 1 : -1.0) > 0.0);
 *   驱动报：
 *     ':' : no operation ':' exists that takes a left-hand operand of
 *           type 'const int' and a right operand of type 'const float'
 *
 * 【为什么现有全部规则都漏掉】
 *   GLSL ES 规范 5.9 要求条件表达式的两个分支**类型一致**（或可隐式互转）。
 *   这里真分支是 `1`(int)、假分支是 `-1.0`(float)。桌面 GLSL 会把 int
 *   提升为 float，ES 不做任何转换，于是一条语句毁掉整个着色器。
 *   本转换器的所有规则都是「就地处理单个字面量」——
 *   它们会把 `1` 改成 `1.0`，但**不会回头去统一另一侧**；
 *   而且 `-1.0` 本来就是浮点、无需改写，所以另一侧永远不会被触及。
 *
 * 【做法：先扫一遍，看到 `? :` 就判断两侧类型，把整数一侧补成浮点】
 *   这里做的是**文本层面的最小侵入**：
 *     `? 1 : -1.0`   -> `? 1.0 : -1.0`
 *   只处理「一侧是整数、另一侧是浮点」的情形；两侧都是整数或都是浮点时
 *   原样保留（都是整数是合法的，两侧都浮点也合法）。
 *
 * 【安全性】
 *   - 只在确认存在 `?` 与配对的 `:`（且处于同一括号深度）时才动手；
 *   - 只在「另一侧确实是浮点」时才把整数侧补成浮点；
 *   - 嵌套三元由内向外逐层处理（每次只改最内层尚未平衡的那个）。
 */
static const char *find_matching_colon(const char *qmark, const char *end) {
    int paren = 0, bracket = 0;
    int nested_q = 0;
    for (const char *p = qmark + 1; p < end; p++) {
        char c = *p;
        if (c == '(') { paren++; continue; }
        if (c == ')') { if (paren == 0) return NULL; paren--; continue; }
        if (c == '[') { bracket++; continue; }
        if (c == ']') { if (bracket > 0) bracket--; continue; }
        if (c == ';' || c == '{' || c == '}') return NULL;
        if (paren > 0 || bracket > 0) continue;
        if (c == '?') { nested_q++; continue; }
        if (c == ':') {
            if (nested_q > 0) { nested_q--; continue; }
            return p;
        }
    }
    return NULL;
}

/*
 * 判断 [zone_start, zone_end) 这一段（三元的一侧）的「值类型」：
 *   1 = 浮点（以浮点字面量结尾、或含浮点字面量、或含已声明浮点变量）
 *   0 = 整数（以纯整数字面量结尾/开头）
 *  -1 = 无法判定
 *
 * 这里只做保守判断：只要整段里出现浮点字面量或已知浮点变量就当作浮点。
 */
static int ternary_side_is_float(const char *zone_start, const char *zone_end,
                                 const char *src_start, const char *src_end) {
    for (const char *p = zone_start; p < zone_end; p++) {
        if (p > zone_start && is_ident_char(p[-1])) continue;
        if (starts_with_float_literal(p, zone_end)) return 1;
        if (is_ident_char(*p)) {
            const char *b = p;
            while (p < zone_end && is_ident_char(*p)) p++;
            size_t nl = (size_t)(p - b);
            if (ident_declared_as_float_anywhere(src_start, src_end,
                                                 b, nl)) {
                return 1;
            }
            p--;
        }
    }
    return 0;
}

/* 判断 [zone_start, zone_end) 里是否出现纯整数字面量（作为候补改写对象） */
static const char *ternary_find_int_literal(const char *zone_start,
                                            const char *zone_end) {
    for (const char *p = zone_start; p < zone_end; p++) {
        if (p > zone_start && is_ident_char(p[-1])) continue;
        if (float_literal_len(p) > 0) {   /* 跳过浮点字面量整体 */
            p += float_literal_len(p) - 1;
            continue;
        }
        if (isdigit((unsigned char)*p) &&
            int_literal_len(p) > 0) {
            return p;
        }
    }
    return NULL;
}

/*
 * 扫描整段源码，把 `? int : float` / `? float : int` 里的整数侧补成浮点。
 * 由内向外反复处理，直到没有可改的位置（嵌套三元需要多轮）。
 * 结果写入 out（在 [*cursor, qmark) 之间原样复制）。
 */
static void balance_ternary_branches(sbuf *out, const char *src) {
    const char *end = src + strlen(src);
    const char *p = src;

    while (p < end) {
        if (*p != '?') { sbuf_put(out, p, 1); p++; continue; }

        /* 找到配对的 ':'；找不到就按普通字符输出 */
        const char *colon = find_matching_colon(p, end);
        if (colon == NULL) { sbuf_put(out, p, 1); p++; continue; }

        /* 真分支 = (p, colon)，假分支 = (colon, 本表达式结束) */
        const char *true_b = p + 1;
        const char *true_e = colon;
        const char *false_b = colon + 1;

        /*
         * 假分支的结束位置：向后扫到同一层的 `;` `)` `,` `}` 或另一个
         * 未配对的 `:`。
         */
        const char *false_e = false_b;
        {
            int paren = 0, bracket = 0;
            while (false_e < end) {
                char c = *false_e;
                if (c == '(') { paren++; false_e++; continue; }
                if (c == ')') { if (paren == 0) break; paren--; false_e++; continue; }
                if (c == '[') { bracket++; false_e++; continue; }
                if (c == ']') { if (bracket > 0) bracket--; false_e++; continue; }
                if (paren == 0 && bracket == 0 &&
                    (c == ';' || c == ',' || c == '}' || c == ':')) break;
                false_e++;
            }
        }

        int t_float = ternary_side_is_float(true_b, true_e, src, end);
        int f_float = ternary_side_is_float(false_b, false_e, src, end);

        if (t_float != f_float) {
            /* 整数的那一侧找第一个整数字面量补 '.0' */
            const char *z_b = t_float ? false_b : true_b;
            const char *z_e = t_float ? false_e : true_e;
            const char *lit = ternary_find_int_literal(z_b, z_e);
            if (lit != NULL) {
                /* 输出到该字面量为止，然后补上 .0，其余照常继续 */
                sbuf_put(out, p, (size_t)(lit - p));
                size_t n = int_literal_len(lit);
                sbuf_put(out, lit, n);
                sbuf_puts(out, ".0");
                p = lit + n;
                continue;
            }
        }
        /* 无需平衡：原样输出 '?' 继续扫描（后续字面量由既有规则处理） */
        sbuf_put(out, p, 1);
        p++;
    }
}

/*
 * 判断位置 p 处的整数字面量是否是**整数型采样函数的最后实参**。
 *
 * 【真机/整包验证证据】
 *     textureLod(colortex1, texCoord + offsets[4] * blurFactor / view, 0)
 *     textureGrad(tex, coord, dPdx, dPdy)      // 后两个是浮点，不适用
 * 末位 `0`（LOD 层）在 GLSL ES 里**必须是浮点**：
 *     'textureLod' : no matching overloaded function found
 * 桌面 GLSL 接受整数 LOD，光影包因此大量这么写。
 *
 * 只处理「确实带 LOD 参数」的纹理函数：
 *   textureLod / textureOffset 的 float LOD
 *   texelFetch 的 int LOD 是**合法**的（texelFetch 签名就是 int lod），
 *   故不列入 —— 把它改成浮点反而制造错误。
 *
 * 实现：向左找到最近的 ',' 或 '('，若为 '(' 且前面的名字属于
 * kFloatLodFns，则判定为真。
 */
static int in_float_lod_arg(const char *p, const char *s) {
    static const char *const kFloatLodFns[] = {
        "textureLod", "textureProjLod", "texture2DLod", "texture3DLod",
        "textureCubeLod", "texture", NULL
    };
    const char *t = p;
    while (t > s) {
        char c = t[-1];
        if (c == '(' || c == ',') break;
        if (c == ')' || c == ';' || c == '{' || c == '}' || c == '=') return 0;
        t--;
    }
    /*
     * 【注意】在 ',' 上 break 时 t[-1] 就是那个逗号本身。
     * 上一版写成 `t[-1] != '('` 就 return 0，于是「字面量不是第一个实参」
     * 的情况（LOD 通常正是第 3 个实参）全部被拒，规则 G 从未生效。
     * 这里改成「停下的位置是 '(' 或 ','」都算落在实参列表内。
     */
    if (t <= s || (t[-1] != '(' && t[-1] != ',')) return 0;

    const char *list_open = NULL;
    if (t[-1] == '(') {
        list_open = t - 1;
    } else {
        /* 从逗号反向配对到本实参列表的 '(' */
        const char *u2 = t - 1;
        int dd = 0;
        while (u2 > s) {
            if (u2[-1] == ')') { dd++; u2--; continue; }
            if (u2[-1] == '(') {
                if (dd == 0) { list_open = u2 - 1; break; }
                dd--; u2--; continue;
            }
            u2--;
        }
        if (list_open == NULL) return 0;
    }

    const char *u = list_open;
    while (u > s && (*u == ' ' || *u == '\t')) u--;
    const char *we = u;
    while (u > s && is_ident_char(u[-1])) u--;
    size_t fl = (size_t)(we - u);
    if (fl == 0) return 0;

    /* texture() 也可能是 texture(tex, uv) 或 texture(tex, uv, bias) */
    for (int i = 0; i < (int)LIST_LEN(kFloatLodFns) && kFloatLodFns[i] != NULL; i++) {
        if (strlen(kFloatLodFns[i]) == fl &&
            strncmp(kFloatLodFns[i], u, fl) == 0) {
            /*
             * 对 texture() 要确认本字面量确实是**第 3 个**实参，
             * 否则 texture(tex, vec2(0, 1)) 里的 1 会被误改。
             */
            if (fl == 7 && strncmp(u, "texture", 7) == 0) {
                int commas = 0;
                for (const char *q = t; q < p; q++) {
                    if (*q == ',') commas++;
                }
                if (commas != 2) return 0;
            }
            return 1;
        }
    }
    return 0;
}

/* ================= 宏实参保护（规则 M） ================= */

/*
 * 源码里 `#define` 出来的宏名字表。
 *
 * 【为什么需要 —— 真机 Flywheel 的第三处误改】
 *   light_lut.glsl 用宏展开 31 个采样点：
 *       #define _FLW_FETCH_LIGHT(_x, _y, _z, i) { \
 *           uvec2 light = _flw_lightAt(sectionOffset, uvec3(blockInSectionPos + ivec3(_x, _y, _z))); \
 *           ...
 *       }
 *       _FLW_FETCH_LIGHT(-1, -1, -1, 0)
 *   我们把调用处的 `-1` 改成了 `-1.0`：
 *       _FLW_FETCH_LIGHT(-1.0, -1.0, -1.0, 0)
 *
 * 【为什么这是错的】
 *   实参是**文本替换**，它会被代进宏体里的每一个 `_x`。
 *   宏体里既有 `ivec3(_x, _y, _z)`（接受浮点，实测 glslc 判定合法），
 *   也可能有**纯整数上下文**。实测（native/tools 下的 bug3-danger.ps1）：
 *       G1  只喂 ivecN(...) 构造          -> LEGAL
 *       G2  构造 + `int k = _x;`          -> ILLEGAL  cannot convert from 'const float' to 'int'
 *       G3  构造 + `arr[_x]`              -> ILLEGAL  scalar integer expression required
 *       G4  构造 + `255 & _x`             -> ILLEGAL  no operation '&' exists
 *   也就是说：把实参浮点化是**给用户代码埋雷**，是否爆炸取决于宏体怎么用。
 *   Flywheel 恰好只用了 G1 的形态，所以这次没炸 —— 但那是运气。
 *
 * 【为什么"一律不改宏实参"是安全的】
 *   宏实参的语义由宏体决定，转换器看不到展开后的形态；
 *   保持原样（整数）是**不改写用户意图**的中性选择。
 *   实测：本仓库全部语料（BSL 166 + MC 核心 125 + fixtures 14）
 *   **一个宏调用都没有**（measure-macro-args.ps1 结果：invocations = 0），
 *   因此本规则对既有产品是完全的空操作。
 */
#define MACRO_NAME_MAX 256
static char g_macro_names[MACRO_NAME_MAX][48];
static int  g_macro_count = 0;

static void collect_macro_names(const char *src) {
    const char *end = src + strlen(src);
    g_macro_count = 0;
    for (const char *p = src; p < end; p++) {
        if (p > src && is_ident_char(p[-1])) continue;
        if (*p != '#') continue;
        const char *q = p + 1;
        while (q < end && (*q == ' ' || *q == '\t')) q++;
        if (q + 6 > end || strncmp(q, "define", 6) != 0) continue;
        q += 6;
        if (q < end && is_ident_char(*q)) continue;   /* 排除 #defineX */
        while (q < end && (*q == ' ' || *q == '\t')) q++;
        if (q >= end || !is_ident_char(*q)) continue;
        const char *b = q;
        while (q < end && is_ident_char(*q)) q++;
        size_t nl = (size_t)(q - b);
        if (nl == 0 || nl >= sizeof(g_macro_names[0])) continue;
        if (g_macro_count >= MACRO_NAME_MAX) break;
        memcpy(g_macro_names[g_macro_count], b, nl);
        g_macro_names[g_macro_count][nl] = '\0';
        g_macro_count++;
    }
}

static int is_macro_name(const char *b, size_t len) {
    for (int i = 0; i < g_macro_count; i++) {
        if (strlen(g_macro_names[i]) == len &&
            strncmp(g_macro_names[i], b, len) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * 判断位置 p 处的整数字面量是否位于某个**宏调用**的实参列表内。
 *
 * 判据：向左跳过空白与一个前导一元符号（+/-），
 *       若紧邻的是 '(' 或 ','，且该实参列表的 '(' 之前是已 #define 的宏名，
 *       则判定为真。
 *
 * 【为什么要跳过一元符号】
 *   `M(-1, ...)` 里 `1` 的左侧紧邻是 '-'，再往左才是 '('。
 *   不跳符号就永远匹配不到第一个实参 —— 而那正是我们出问题的那一个。
 *   `a - 1` 不会误判：跳过 '-' 后左边是标识符 `a`，不是 '(' 或 ','。
 */
static int in_macro_arg(const char *p, const char *s) {
    const char *t = p;
    while (t > s && (t[-1] == ' ' || t[-1] == '\t' ||
                     t[-1] == '\n' || t[-1] == '\r')) t--;
    if (t > s && (t[-1] == '-' || t[-1] == '+')) {
        t--;
        while (t > s && (t[-1] == ' ' || t[-1] == '\t' ||
                         t[-1] == '\n' || t[-1] == '\r')) t--;
    }
    if (t <= s || (t[-1] != '(' && t[-1] != ',')) return 0;

    /* 找本实参列表的 '(' */
    const char *list_open = NULL;
    if (t[-1] == '(') {
        list_open = t - 1;
    } else {
        const char *u2 = t - 1;
        int dd = 0;
        while (u2 > s) {
            if (u2[-1] == ')') { dd++; u2--; continue; }
            if (u2[-1] == '(') {
                if (dd == 0) { list_open = u2 - 1; break; }
                dd--; u2--; continue;
            }
            u2--;
        }
        if (list_open == NULL) return 0;
    }

    const char *u = list_open;
    while (u > s && (*u == ' ' || *u == '\t')) u--;
    const char *we = u;
    while (u > s && is_ident_char(u[-1])) u--;
    size_t fl = (size_t)(we - u);
    if (fl == 0) return 0;
    return is_macro_name(u, fl);
}

/* ================= 规则 N：uint 标量与整数字面量的运算（补 u 后缀） ================= */

/*
 * 「标量 uint 名字表」。
 *
 * 【为什么需要 —— 真机 Flywheel 飞轮 instancing 编译失败的第 3 组错误】
 *   Flywheel 的 packed_material.glsl 写的是：
 *       void _flw_unpackMaterialProperties(uint p, out FlwMaterial m) {
 *           ...
 *           m.ambientOcclusion = (p & _FLW_AMBIENT_OCCLUSION_MASK) != 0;
 *       }
 *   其中 `p` 与 `_FLW_AMBIENT_OCCLUSION_MASK` 都是 uint，而字面量 `0` 是 int。
 *   驱动报：
 *       error: wrong operand types  no operation '!=' exists that takes a
 *              left-hand operand of type 'uint' and a right operand of type
 *              'const int' (or there is no acceptable conversion)
 *
 * 【为什么桌面 GLSL 不报错】
 *   桌面 GLSL 允许 uint 与 int 隐式互转；GLSL ES 3.x **明确禁止**隐式整型
 *   转换（只能显式写成 uint(...)）。因此桌面能跑的着色器在 ES 下必然失败。
 *
 * 【修法：给字面量补 `u` 后缀 —— 语义完全等价，零歧义】
 *   用 glslc（ES 3.20）逐组合实测（native/tools/es-uint-op-probe.ps1 与
 *   es-uint-probe.ps1）：
 *       uint OP 0     -> 非法    （OP = != == > < >= <= & | ^ + - * / %）
 *       uint OP 0u    -> 合法
 *       uint >> 0     -> 合法    （移位右操作数允许 int，**不能动**）
 *       uint << 0     -> 合法
 *       uint  x = 0;  -> 非法    （声明初始化与普通赋值同理）
 *   `0` 与 `0u` 的**数值**完全相同，不存在任何解释歧义 ——
 *   这是本项目里少见的「可以放心全自动改写」的情形。
 *
 * 【为什么必须精确判定左操作数是 uint，不能凭「字面量紧邻」就加 u】
 *   同一份 Flywheel 源码里还有：
 *       int flw_fogShape;  ...  if (fogShape == 0) { ... }
 *       for (int i = 0; i < 0 || i >= int(size); ...)
 *   这些都是 **int** 与 int 比较，本来就合法；若补上 `u` 就变成
 *   `int == uint` —— 反而制造出新的非法代码。
 *   因此本规则要求**正向确认**左操作数确实是 uint 标量。
 *   （教训：本项目已两次栽在「查不到负面证据就当正面结论」上。）
 *
 * 【同名歧义必须排除】
 *   实测 Flywheel 源码里有 132 个 uint 名与 16 个 int 名，其中 `base`、`index`
 *   **同时**被声明为 uint 和 int（不同作用域）。对这类名字无从判断该按哪种
 *   处理，一律排除（宁可漏改，不可误改 —— 误改会级联放大）。
 */
static int_ident_table g_uint_idents;
static int g_uint_idents_ready = 0;

/*
 * 「标量 int 名字表」（含 gl_VertexID 等 int 内建）。
 *
 * 规则 P 需要它来判断混合 int/uint 运算里的 **int 侧**。
 * 表内容直接复用 normalize_int_literals 里已经扫好的 int_idents
 * （那边已经在扫全源码），避免重复扫描。
 */
static int_ident_table g_src_int_idents;
static int g_src_int_ready = 0;

/*
 * ============ 用户自定义函数的形参类型表（规则 R） ============
 *
 * 【真机/离线证据 —— 缺陷 2】
 *   转换器只知道**内建**函数的类型（FLOAT_CTORS / INT_CTORS /
 *   FLOAT_BUILTIN_FNS），对**用户自定义**函数一无所知。于是
 *       float c(int n) { return float(n) / 16.0; }
 *       o = vec4(c(1), 1.0);
 *   里的 `c(1)` 走「规则 A：float 构造函数实参一律浮点化」，
 *   被改成 `c(1.0)`，GLSL ES 报：
 *       'c' : no matching overloaded function found
 *   桌面 GLSL 允许 int->float 隐式转换，所以桌面不报错。
 *   实测（fixtures/probe_userfn_int_args.frag）确认这是**整类**问题：
 *       c1(1)      -> c1(1.0)        （int 形参）
 *       u1(2)      -> u1(2.0)        （uint 形参）
 *       m1(4,0.5)  -> m1(4.0, 0.5)   （第 0 个是 int）
 *       m2(5,6,.5) -> m2(5.0,6.0,.5) （前两个是 int）
 *       b1(8)      -> b1(8.0)        （int 形参）
 *   注意 `v1(ivec2(3))` **正确保持不动** —— 因为 `ivec2(` 是整数构造函数，
 *   int_ctor_depth>0 会屏蔽改写。缺的只是「用户函数形参」这一路信息。
 *
 * 【为什么必须记录「第几个形参」而不是「有没有 int 形参」】
 *   `m1(int a, float b)` 里 `m1(4, 0.5)` 的 `4` 要保持整数、
 *   `0.5` 本来就是浮点。若只看「该函数含 int 形参」，就会把
 *   浮点形参位置的字面量也一并保留，制造新的隐式转换错误。
 */
#define FN_SIG_MAX   128
#define FN_PARAM_MAX 8

/* 形参类别：0 = 浮点族，1 = 有符号整数族，2 = 无符号整数族 */
typedef struct {
    char name[64];
    int  nparams;
    int  param_cls[FN_PARAM_MAX];
    int  conflicting;               /* 同名不同签名 -> 整条作废（见下） */
} fn_sig;

typedef struct {
    fn_sig items[FN_SIG_MAX];
    int    count;
} fn_sig_table;

static fn_sig_table g_fn_sigs;
static int g_fn_sigs_ready = 0;

/*
 * 前置声明 —— 必须放在 collect_user_fn_sigs 之前。
 *
 * 【教训】这几个函数定义在文件后半部分，若声明落在使用点之后，
 * gcc 报 -Wimplicit-function-declaration 而**脚本仍会用旧二进制**，
 * 症状是「补丁看起来实现了却零效果」。第一次修此文件时正是栽在这里，
 * 因此这次显式前移并保留这条注记。
 */
static int is_decl_qualifier_token(const char *s, size_t n);
static int is_control_keyword(const char *s, size_t n);
static int int_family_of_token(const char *s, size_t n);

/*
 * 收集用户自定义函数的形参类型。
 *
 * 识别形如 `返回类型 函数名(参数列表) {` 的定义 —— 只认带函数体的**定义**，
 * 不认声明（声明可能在别处、甚至不存在于本转换器的输入里）。
 *
 * 参数列表按逗号切分，每位取「类型族」：
 *     整数族（int/uint/ivecN/uvecN/bvecN） -> 1
 *     浮点族（float/vecN/matN）           -> 0
 *     不确定（自定义类型 / 结构体 / 数组） -> 标为 -1，调用方视为未知并放弃
 *
 * 【为什么要判 -1 并整条放弃】
 *   形如 `void f(MyStruct s, int n)` 时，第 0 位未知不代表第 1 位不能用 ——
 *   但 `f(...)` 的实参**位置**只有在参数列表完整可解析时才可靠对齐
 *   （宏、逗号运算符都可能打乱切分）。保守起见：只要有一位不确定就放弃整条。
 */ 
static void collect_user_fn_sigs(const char *src, fn_sig_table *t) {
    const char *end = src + strlen(src);
    t->count = 0;

    const char *p = src;
    while (p < end && t->count < FN_SIG_MAX) {
        const char *skipped = skip_comment_or_string(p, end);
        if (skipped != NULL) { p = skipped; continue; }
        if (!is_ident_char(*p)) { p++; continue; }

        /* 取一个 token 作为「返回类型」候选 */
        const char *rte = p;
        while (rte < end && is_ident_char(*rte)) rte++;
        size_t rtl = (size_t)(rte - p);
        if (rtl == 0) { p++; continue; }

        /* 跳过限定符（highp / const / ...）继续找真正的返回类型 */
        const char *ty = p, *tye = rte;
        size_t tyl = rtl;
        int guard = 0;
        while (tyl > 0 && is_decl_qualifier_token(ty, tyl) && guard++ < 8) {
            const char *q = tye;
            while (q < end && (*q == ' ' || *q == '\t' ||
                               *q == '\n' || *q == '\r')) q++;
            const char *qe = q;
            while (qe < end && is_ident_char(*qe)) qe++;
            if (qe == q) { tyl = 0; break; }
            ty = q; tye = qe; tyl = (size_t)(qe - q);
        }
        int ret_family = type_family_of_token(ty, tyl);
        if (ret_family == 0) { p = rte; continue; }

        /* 返回类型之后：函数名 + '(' */
        const char *q = tye;
        while (q < end && (*q == ' ' || *q == '\t' ||
                           *q == '\n' || *q == '\r')) q++;
        const char *qne = q;
        while (qne < end && is_ident_char(*qne)) qne++;
        size_t qnl = (size_t)(qne - q);
        if (qnl == 0 || qnl >= sizeof(t->items[0].name)) { p = rte; continue; }
        if (is_control_keyword(q, qnl)) { p = rte; continue; }

        const char *lp = qne;
        while (lp < end && (*lp == ' ' || *lp == '\t' ||
                            *lp == '\n' || *lp == '\r')) lp++;
        if (lp >= end || *lp != '(') { p = rte; continue; }

        /* 配对到 ')' */
        const char *rp = lp + 1;
        int d = 1;
        while (rp < end && d > 0) {
            if (*rp == '(') d++;
            else if (*rp == ')') d--;
            if (d == 0) break;
            rp++;
        }
        if (rp >= end) break;

        /* 参数列表之后必须是 '{'（是定义而非声明） */
        const char *body = rp + 1;
        while (body < end && (*body == ' ' || *body == '\t' ||
                              *body == '\n' || *body == '\r')) body++;
        if (body >= end || *body != '{') { p = rte; continue; }

        /* 切分参数并取各位的类型族 */
        {
            fn_sig sig;
            memset(&sig, 0, sizeof(sig));
            memcpy(sig.name, q, qnl);
            sig.name[qnl] = '\0';
            sig.nparams = 0;

            int unknown = 0;
            const char *a = lp + 1;
            if (a == rp) {
                /* 空参数列表 */
                sig.nparams = 0;
            } else {
                while (a < rp && sig.nparams < FN_PARAM_MAX) {
                    /* 取本位的类型 token */
                    while (a < rp && (*a == ' ' || *a == '\t' ||
                                      *a == '\n' || *a == '\r' ||
                                      *a == ',')) a++;
                    if (a >= rp) break;
                    const char *be = a;
                    while (be < rp && is_ident_char(*be)) be++;
                    size_t bl = (size_t)(be - a);
                    if (bl == 0) { unknown = 1; break; }

                    int fam = type_family_of_token(a, bl);
                    /* 跳过限定符再判一次（`const int n` / `in float x`） */
                    if (fam == 0 && is_decl_qualifier_token(a, bl)) {
                        const char *b2 = be;
                        while (b2 < rp && (*b2 == ' ' || *b2 == '\t' ||
                                           *b2 == '\n' || *b2 == '\r')) b2++;
                        const char *b2e = b2;
                        while (b2e < rp && is_ident_char(*b2e)) b2e++;
                        if (b2e > b2) {
                            fam = type_family_of_token(b2, (size_t)(b2e - b2));
                            be = b2e;
                        }
                    }
                    if (fam == 0) { unknown = 1; break; }   /* 自定义类型等 */

                    if (fam == 2) {
                        sig.param_cls[sig.nparams++] = 0;          /* 浮点族 */
                    } else {
                        int ic = int_family_of_token(a, bl);
                        if (ic == 0) {
                            /*
                             * 已确认是整数族但细分不出符号性？不应发生。
                             * 为免在不确定信息上改写，整条作废。
                             */
                            unknown = 1;
                            break;
                        }
                        sig.param_cls[sig.nparams++] = ic;          /* 1 或 2 */
                    }

                    /* 跳到下一个顶层逗号 */
                    int dd = 0;
                    while (a < rp) {
                        if (*a == '(' || *a == '[') dd++;
                        else if (*a == ')' || *a == ']') { if (dd > 0) dd--; }
                        else if (*a == ',' && dd == 0) { break; }
                        a++;
                    }
                }
            }

            if (!unknown && sig.nparams > 0) {
                /* 同名已存在？比较签名，不一致则整条作废 */
                int found = 0;
                for (int i = 0; i < t->count; i++) {
                    if (strcmp(t->items[i].name, sig.name) == 0) {
                        found = 1;
                        if (t->items[i].nparams != sig.nparams ||
                            memcmp(t->items[i].param_cls, sig.param_cls,
                                   (size_t)sig.nparams * sizeof(int)) != 0) {
                            t->items[i].conflicting = 1;
                        }
                        break;
                    }
                }
                if (!found) t->items[t->count++] = sig;
            }
        }
        p = body;
    }
}

/*
 * 判断「位置 p 处的字面量是用户自定义函数第 k 个实参，且该位形参的类型类别」。
 *
 * 返回：-1 = **不适用或无法确定**（不是用户函数调用 / 签名表无此名 / 名冲突 / 位置对不上）
 *        0 = 该位形参确为浮点族 -> 整数字面量必须补 `.0`
 *        1 = 有符号整数族（int / ivecN）     -> 字面量必须保持整数
 *        2 = 无符号整数族（uint / uvecN）    -> 字面量必须补 `u` 后缀
 *
 * 【为什么必须区分「已知浮点」与「不适用」】
 *   两者都对应「不是整数形参」，但对字面量的要求相反：
 *     已知浮点   -> `f1(3)` 必须变 `f1(3.0)`（ES 无 int->float 隐式转换）
 *     不适用     -> 交给原有规则处理，**绝不能动**
 *   旧版把两者都返回 0，于是 float 形参的镜像改写永远不生效。
 */
static int user_fn_param_class(const char *p, const char *s,
                               const fn_sig_table *t) {
    if (t == NULL || t->count == 0 || p <= s) return -1;

    /*
     * 1. 向左跳过空白；p 前面应当是 ',' 或 '('（即它正好是某个实参的开头）。
     *
     * 2'. **并且向右必须是 ',' 或 ')'** —— 即「整个实参就是这个裸字面量」。
     *
     * 【为什么必须加这道右界 —— A/B 实测抓到的真实回归】
     *   只判左界时，复合实参会被误认为「实参就是该字面量」。Flywheel 的
     *       return clamp(int(floor(depth * 16)), 0, 16 - 1);
     *   里 `16 - 1` 的 `16` 左侧确实是 ','，于是被当成某函数的实参：
     *   反向配对越过已经平衡的 `clamp(...)` 前缀后，命中的 '(' 顶着
     *   一个**用户函数**名，第 k 位恰为浮点 -> 把 `16` 改成 `16.0`，
     *   进而把整句变成 float 上下文，连 `1` 也被连带改成 `1.0`，最终产出
     *       clamp(int(floor(depth * 16.0)), 0, 16.0 - 1.0)
     *   破坏了 clamp 的 int 重载（ES 报 no matching overloaded function）。
     *
     *   用「右侧必须是 ',' 或 ')'」把适用范围收回到**真正的裸字面量实参**，
     *   既保住了 `c1(1)` / `u1(2)` / `f1(3)` / `m1(6, 0.5)`，也彻底排除
     *   `16 - 1` 这类复合表达式。这与规则 R 的立意一致：
     *   只在「形参类型**确定**决定了该字面量的类型」时才改写。
     */
    {
        const char *rgt = p;
        while (rgt < (s + strlen(s)) && isdigit((unsigned char)*rgt)) rgt++;
        while (rgt < (s + strlen(s)) && (*rgt == ' ' || *rgt == '\t')) rgt++;
        if (rgt < (s + strlen(s)) && *rgt != ',' && *rgt != ')') return -1;
    }

    const char *l = p;
    while (l > s && (l[-1] == ' ' || l[-1] == '\t')) l--;
    if (l <= s) return -1;
    if (l[-1] != ',' && l[-1] != '(') return -1;

    /* 2. 从该 '(' / ',' 反向配对，取本次调用的名称与第 k 个实参序号 */
    int k = 0;                     /* 已越过的顶层逗号数 = 参数序号 */
    /*
     * 反向配对：q 指向「当前待检查的字符」（不是 q[-1]）。
     *
     * 【指针语义必须写对 —— 这里返工过一次】
     *   第一版写成 `while (q > s) { if (q[-1] == ')') ... }`，
     *   即拿 q[-1] 当当前字符，但起手用 `q = l - 1`（已指向 '(' 或 ','），
     *   于是每次都在检查**前一个**字符。结果第一个 `c1(1)` 处
     *   q 指向 '('，q[-1] 是 '1'，三个分支全不命中，直接 q-- 越过 '('，
     *   一路穿到文件开头才返回 0 —— 规则 R 形同不存在。
     *   症状再次是「代码看起来对、却零效果」，与括号配对那次同因。
     */
    const char *q = l - 1;         /* 指向本次调用内、该实参之前的 '(' 或 ',' */
    int d2 = 0;
    for (;;) {
        if (q <= s) return -1;
        char c = *q;
        if (c == ')') { d2++; q--; continue; }
        if (c == '(') {
            if (d2 == 0) break;    /* 命中本次调用的 '(' */
            d2--;
            q--;
            continue;
        }
        if (c == ',' && d2 == 0) { k++; q--; continue; }
        q--;
    }
    /* 此刻 q 指向本次调用的 '(' */
    const char *fn_start = q;
    while (fn_start > s && is_ident_char(fn_start[-1])) fn_start--;
    size_t fnl = (size_t)(q - fn_start);
    if (fnl == 0 || fnl >= sizeof(t->items[0].name)) return -1;
    if (q == fn_start) return -1;   /* '(' 前面没有名字（分组括号） */

    /* 3. 查签名表 */
    for (int i = 0; i < t->count; i++) {
        if (t->items[i].conflicting) continue;
        if (strlen(t->items[i].name) != fnl ||
            strncmp(t->items[i].name, fn_start, fnl) != 0) continue;
        if (k < 0 || k >= t->items[i].nparams) return -1;
        return t->items[i].param_cls[k];
    }
    return -1;
}

/*
 * 同名重载的处理：GLSL 允许重载，但**形参类型不同**。
 * 遇到同名不同签名时，我们无法判断调用点选中的是哪一个
 * （桌面 GLSL 会隐式转换，调用点本身不足以区分），
 * 因此把该名字整体标记为 conflicting 并**放弃使用签名信息** ——
 * 宁可退回改动前的行为（可能漏改），也不要在不确定的信息上做改写。
 * 依据：「查不到确定的正面证据就不能当负面结论」这条教训已踩过两次。
 */

/*
 * 整数族细分：0 = 非整数，1 = 有符号（int / ivecN），2 = 无符号（uint / uvecN）。
 *
 * 【为什么必须区分】
 *   `void f(int n)` 与 `void f(uint n)` 对实参的要求相反：
 *       f(3)   -> int 形参：保持裸字面量
 *       f(3)   -> uint 形参：必须补 `u` 后缀（ES 不做 int->uint 隐式转换，
 *                 而桌面允许；同为「桌面能过、ES 不能过」的隐式转换差异）
 *   若把两者混为一谈，就会漏掉 uint 情形，或把 int 情形错改成 `3u`。
 */
static int int_family_of_token(const char *s, size_t n) {
    if (n == 3 && strncmp(s, "int", 3) == 0)  return 1;
    if (n == 4 && strncmp(s, "uint", 4) == 0) return 2;
    if (n == 5) {
        if (strncmp(s, "ivec", 4) == 0) return 1;
        if (strncmp(s, "uvec", 4) == 0) return 2;
    }
    if (n == 5 && strncmp(s, "bvec", 4) == 0) return 1;  /* bvec 不是 uint */
    return 0;
}

/* 定义在文件后半部分；collect_uint_vars 要用它排除同名歧义。 */
static int ident_declared_as_other_int_anywhere(const char *src, const char *end,
                                                const char *name, size_t nlen);

static int uint_ident_find(const char *b, size_t len) {
    if (!g_uint_idents_ready) return 0;
    return int_ident_find(&g_uint_idents, b, len);
}

/*
 * 收集「标量 uint」名字。
 *
 * 收集范围：源码中**任何**位置出现的 `uint <标识符>` —— 全局 uniform、
 * 函数形参、局部变量一视同仁（本规则只看名字，不看作用域；
 * 这是安全的，因为下面两条排除条件已把有歧义的名字剔除）。
 *
 * 排除条件（任一成立即排除）：
 *   1. 该名字在别处被声明为浮点（float / vecN / matN）
 *   2. 该名字在别处被声明为其它整数类型（int / ivecN / uvecN / bvecN）
 *      —— 注意 `uint` 本身**不在**这个列表里，否则每个名字都会自我否决。
 */
static void collect_uint_vars(const char *src, int_ident_table *out) {
    const char *end = src + strlen(src);
    out->count = 0;
    out->saturated = 0;

    for (const char *p = src; p + 4 <= end; p++) {
        const char *skipped = skip_comment_or_string(p, end);
        if (skipped != NULL) { p = skipped - 1; continue; }
        if (strncmp(p, "uint", 4) != 0) continue;
        if (p > src && is_ident_char(p[-1])) continue;
        if (p + 4 < end && is_ident_char(p[4])) continue;

        const char *r = p + 4;
        while (r < end && (*r == ' ' || *r == '\t' ||
                           *r == '\n' || *r == '\r')) r++;
        if (r >= end || *r == '(' || !is_ident_char(*r) ||
            isdigit((unsigned char)*r)) {
            continue;
        }
        const char *s2 = r;
        while (s2 < end && is_ident_char(*s2)) s2++;
        size_t nl = (size_t)(s2 - r);

        if (nl > 0 && nl < sizeof(out->items[0].name) &&
            !ident_declared_as_float_anywhere(src, end, r, nl) &&
            !ident_declared_as_other_int_anywhere(src, end, r, nl)) {
            int_ident_add(out, r, nl);
        }
        p = s2 - 1;         /* for 循环自身还会 ++ */
    }
}

/* 从 m（运算符起点）向左跳过空白后取标识符基名。 */
static const char *uid_lhs_before(const char *m, const char *s, size_t *len) {
    const char *t = m;
    while (t > s && (t[-1] == ' ' || t[-1] == '\t' ||
                     t[-1] == '\n' || t[-1] == '\r')) t--;
    if (t <= s) { *len = 0; return t; }
    return lhs_base_ident(t, s, len);
}

/*
 * 判断「以 m 为末尾（不含）」的表达式是否为 **uint 标量**。
 *
 * 只识别三类形态，其余一律返回 0（保守 -> 宁可漏改，不可误改）：
 *   1. 标识符（含 .x/.y 这类 swizzle 的基名）
 *   2. 显式转换 `uint(...)`
 *   3. 分组括号 `( ... )` —— 递归看括号内最后一个元素
 *
 * 明确否定（这些结果必不是 uint）：浮点构造 vecN/matN、浮点内建函数
 * （min/max/floor…）、其它整数构造 int()/ivecN()/uvecN()/bvecN()。
 * 未知函数不猜。
 */
static int expr_is_uint_scalar(const char *m, const char *s) {
    if (m <= s) return 0;
    const char *t = m;
    while (t > s && (t[-1] == ' ' || t[-1] == '\t' ||
                     t[-1] == '\n' || t[-1] == '\r')) t--;
    if (t <= s) return 0;

    if (t[-1] == ')') {
        /* 反向配对到对应的 '(' */
        const char *rp = t - 1;         /* 指向 ')' */
        const char *q = rp;
        int d = 0;
        while (q > s) {
            if (q[-1] == ')') { d++; q--; continue; }
            if (q[-1] == '(') { if (d == 0) break; d--; q--; continue; }
            q--;
        }
        if (q <= s || q[-1] != '(') return 0;

        /* '(' 前面若紧跟标识符，则这是一个函数调用 / 构造函数 */
        const char *u = q - 1;
        while (u > s && (*u == ' ' || *u == '\t' ||
                         *u == '\n' || *u == '\r')) u--;
        const char *we = u;
        while (u > s && is_ident_char(u[-1])) u--;
        size_t ul = (size_t)(we - u);

        if (ul > 0) {
            /* 显式转换 uint(...) —— 唯一正面确认 */
            if (ul == 4 && strncmp(u, "uint", 4) == 0) return 1;
            if (name_in_list(u, ul, FLOAT_CTORS, LIST_LEN(FLOAT_CTORS))) return 0;
            if (name_in_list(u, ul, INT_CTORS, LIST_LEN(INT_CTORS))) return 0;
            if (name_in_list(u, ul, FLOAT_BUILTIN_FNS,
                             LIST_LEN_TERM(FLOAT_BUILTIN_FNS))) return 0;
            return 0;                   /* 未知函数：不猜 */
        }
        /* 纯分组括号：看括号内最后一个元素 */
        return expr_is_uint_scalar(rp, s);
    }

    /* 标识符 / swizzle / 带 u 后缀的字面量 */
    {
        size_t ul = 0;
        const char *u = lhs_base_ident(t, s, &ul);
        if (ul == 0) return 0;

        /*
         * 【带 u/U 后缀的整数字面量本身就是 uint】
         *   形如 `uintVar & 0xFFu`、`uintVar | 3u`。
         *   lhs_base_ident 会把 `0xFFu` 整段当作一个标识符（数字与字母
         *   都是 is_ident_char），于是名字表里查不到。但它的**类型是确定的**
         *   （带 u 后缀 == uint），可以无条件认定。
         *   【判据】首字符是数字且末字符是 u/U。
         */
        if (isdigit((unsigned char)u[0]) && (u[ul - 1] == 'u' || u[ul - 1] == 'U')) {
            return 1;
        }
        return uint_ident_find(u, ul);
    }
}

/*
 * 规则 N 判据：位置 p 的整数字面量是否应当补 `u` 后缀。
 *
 * 覆盖三类形态（均由 glslc 实测确认为 ES 非法）：
 *   (a) 复合赋值   `uintVar += 0`        -> `+= 0u`
 *   (b) 普通赋值   `uintVar = 0`         -> `= 0u`（含声明初始化 `uint v = 0;`）
 *   (c) 二元运算   `uintExpr OP 0`       -> `OP 0u`
 *       OP ∈ { != == > < >= <= && || & | ^ + - * / % }
 *       **不含移位 << >>**（实测合法，右操作数为 int 是允许的）。
 *
 * 三类都要求左操作数被**正向确认**为 uint 标量。
 */
static int uint_int_literal_needs_u_suffix(const char *p, const char *s) {
    size_t n = int_literal_len(p);
    if (n == 0) return 0;
    if (p[n - 1] == 'u' || p[n - 1] == 'U') return 0;   /* 已有后缀 */

    const char *l = p;
    while (l > s && (l[-1] == ' ' || l[-1] == '\t' ||
                     l[-1] == '\n' || l[-1] == '\r')) l--;
    if (l <= s) return 0;

    char c1 = l[-1];
    char c0 = (l - 2 >= s) ? l[-2] : '\0';

    /* (a) 复合赋值 `+= -= *= /= %= &= |= ^=`：左值必须是 uint 变量 */
    if (c1 == '=' && (c0 == '+' || c0 == '-' || c0 == '*' || c0 == '/' ||
                      c0 == '%' || c0 == '&' || c0 == '|' || c0 == '^')) {
        size_t ul = 0;
        const char *u = uid_lhs_before(l - 2, s, &ul);
        return ul > 0 && uint_ident_find(u, ul);
    }

    /*
     * (b) 普通赋值 / 声明初始化 `x = 0`。
     *     排除 == != <= >=（那三种走 (c)）。
     *     左值必须是「声明为 uint 的标识符」。
     */
    if (c1 == '=' && c0 != '=' && c0 != '!' && c0 != '<' && c0 != '>') {
        size_t ul = 0;
        const char *u = uid_lhs_before(l - 1, s, &ul);
        return ul > 0 && uint_ident_find(u, ul);
    }

    /* (c) `uint 表达式 OP 字面量` */
    {
        int sensitive = 0;
        int oplen = 1;
        if (c1 == '=') {
            if (c0 == '=' || c0 == '!') { sensitive = 1; oplen = 2; }      /* == != */
            else if (c0 == '<' || c0 == '>') { sensitive = 1; oplen = 2; } /* <= >= */
        }
        /*
         * 【刻意不含 && 与 ||】
         *   实测：`uintVar && 0` 与 `uintVar && 0u` **都是非法**的 ——
         *   && / || 要求两侧为 bool，而 uint 本身就不是 bool，
         *   补 `u` 后缀根本修不好，只会做一次无意义的改动。
         *   真正需要的是把 `p` 改写成 `p != 0u`，那是表达式重写，
         *   超出「给字面量补后缀」的范围，故显式跳过。
         */
        else if ((c1 == '&' && c0 == '&') || (c1 == '|' && c0 == '|')) {
            return 0;
        }
        else if ((c1 == '<' || c1 == '>') && c0 != '<' && c0 != '>') {
            sensitive = 1;              /* < > 但排除 << >> */
        } else if (c1 == '+' || c1 == '-' || c1 == '*' ||
                   c1 == '/' || c1 == '%') { sensitive = 1; }
        else if (c1 == '&' || c1 == '|' || c1 == '^') {
            sensitive = 1;              /* 单个 & | ^（&& || 已在上方排除） */
        }
        if (!sensitive) return 0;

        return expr_is_uint_scalar(l - oplen, s);
    }
}

/* ================= 规则 P：混合 int/uint 二元运算（补 uint() 转换） ================= */

/*
 * 【为什么需要 —— Flywheel instancing 的最后一处阻塞】
 *   真机（2026-10-05 14:24，shader=220）报：
 *       ERROR: 15:79: '-' : no operation '-' exists that takes a left-hand
 *              operand of type 'gl_VertexID int' and a right operand of
 *              type 'in uint'
 *       ERROR: 15:79: 'assign' : cannot convert from 'gl_VertexID int' to 'uint'
 *     --> flywheel:internal/common.vert
 *     79 |     flw_vertexId = gl_VertexID - baseVertex;
 *   其中 `gl_VertexID` 是 int（内建），`baseVertex` 是 `in uint`。
 *   桌面 GLSL 允许 int 与 uint 混算，GLSL ES **完全禁止**。
 *
 * 【语义问题 —— 已用 SPIR-V 逐指令证明无歧义】
 *   本规则之前一直没做，是担心「int -> uint 还是 uint -> int」有歧义。
 *   实测（native/tools/probe_mixed_int_uint.ps1，glslang 前端 + spirv-dis）：
 *       桌面 #version 460:  uint r = i0 + u0;
 *         %13 = OpBitcast %uint %12      <- int 按位重解释为 uint
 *         %17 = OpIAdd   %uint %13 %16
 *       ES   #version 320 es: uint r = uint(i0) + u0;
 *         %13 = OpBitcast %uint %12      <- 完全相同
 *         %17 = OpIAdd   %uint %13 %16
 *   **两条序列逐指令一致**。规范也确实如此：混合大小相同的
 *   有符号/无符号整型时，有符号操作数隐式转换为无符号。
 *   因此补 `uint(...)` 是**语义等价**的改写，不是近似。
 *   （OpBitcast 等价 C 的 `(unsigned)i`，负数变为 2^32-|i|。）
 *
 * 【为什么只给 int 侧补，而不是两侧都转 int】
 *   理论上 `uint(int(a) OP int(b))` 也合法，但对无符号量按有符号比较
 *   会改变结果（`0xFFFFFFFFu` 会变成 -1），与桌面不一致。
 *   补 `uint()` 才是逐位等价的。
 *
 * 【运算符范围（glslc 实测，ES 3.20）】
 *   需转换： + - * / % & | ^  与比较 < > <= >= == !=
 *   不需转换：<< >>（右操作数为 int 是**合法**的，不得动）
 *   && || 不处理：uint 本身不是 bool，补 uint() 修不好（同规则 N）。
 *   复合赋值同规则： += -= *= /= %= &= |= ^= 需转换； <<= >>= 合法。
 *
 * 【安全设计：只重解释「确定是 int」的表达式】
 *   与既有规则一致，本规则只做**正向确认**，绝不靠「查不到当另一种」。
 *   判据分两步：
 *     1. 一侧确定为 uint（uint 名字表 / `uint(...)` / 带 u 后缀字面量）
 *     2. 另一侧确定为 int（int 名字表 / `int(...)` / 整数字面量 /
 *        int 内建如 gl_VertexID）
 *   两步都成立才改写。因此 `intVar OP intVar2` 与 `uintVar OP uintVar2`
 *   都完全不受影响。
 */

/* ================= 规则 P：混合 int/uint 二元运算（补 uint() 转换） ================= */

/*
 * 【为什么需要 —— Flywheel instancing 的最后一处阻塞】
 *   真机（2026-10-05 14:24，shader=220）报：
 *       ERROR: 15:79: '-' : no operation '-' exists that takes a left-hand
 *              operand of type 'gl_VertexID int' and a right operand of
 *              type 'in uint'
 *       ERROR: 15:79: 'assign' : cannot convert from 'gl_VertexID int' to 'uint'
 *     --> flywheel:internal/common.vert
 *     79 |     flw_vertexId = gl_VertexID - baseVertex;
 *   其中 `gl_VertexID` 是 int（内建），`baseVertex` 是 `in uint`。
 *   桌面 GLSL 允许 int 与 uint 混算，GLSL ES **完全禁止**。
 *
 * 【语义无歧义 —— 已用 SPIR-V 逐指令证明】
 *   本规则一直没做，是担心「转 int 还是转 uint」有歧义。
 *   实测（native/tools/probe_mixed_int_uint.ps1 = glslang 前端 + spirv-dis）：
 *       桌面 #version 460:  uint r = i0 + u0;
 *         %13 = OpBitcast %uint %12      <- int 按位重解释为 uint
 *         %17 = OpIAdd   %uint %13 %16
 *       ES   #version 320 es: uint r = uint(i0) + u0;
 *         %13 = OpBitcast %uint %12      <- **完全相同**
 *         %17 = OpIAdd   %uint %13 %16
 *   两条序列逐指令一致。规范亦如此：混合大小相同的两种整型时，
 *   有符号操作数隐式转换为无符号。
 *   因此补 `uint(...)` 是**语义等价**的改写，不是近似。
 *   （OpBitcast 等价 C 的 `(unsigned)i`；负数变为 2^32-|i|。）
 *
 * 【为什么只给 int 侧补，而不是两侧都转 int】
 *   虽然 `uint(int(a) OP int(b))` 也合法，但对无符号量按有符号比较
 *   会改变结果（`0xFFFFFFFFu` 会变成 -1），与桌面不一致。
 *   补 `uint()` 才是逐位等价的。
 *
 * 【运算符范围（glslc 实测，ES 3.20）】
 *   需转换：+ - * / % & | ^ 与比较 < > <= >= == !=
 *   不需转换：<< >>（右操作数为 int **合法**，不得动）
 *   复合赋值同理： += -= *= /= %= &= |= ^= 需转换； <<= >>= 合法。
 *   && || 不处理：uint 本身不是 bool，补 uint() 修不好（同规则 N）。
 *
 * 【安全设计：只处理「可单独识别的单 token 操作数」】
 *   本规则**不**解析任意复杂表达式 —— 那需要完整类型系统，是刻意回避的。
 *   它只识别两侧各为一个「完整 token」的形态：
 *       <token> OP <token>
 *   token 定义为：标识符（查表） / uint(..) / int(..) / 整数字面量。
 *   两侧类型已知且**一为 uint、一为 int** 时才改写。
 *   其余一律原样输出（宁可漏改，不可误改）。
 *   这样 `gl_VertexID - baseVertex` 被覆盖，而 `intVar < intVar2`、
 *   `uvecA * f`、任意含算术的复合表达式都绝不会被误碰。
 */

/* 操作数类型 */
#define OPERAND_NONE 0
#define OPERAND_INT  1
#define OPERAND_UINT 2

/*
 * 从 p 开始向前识别一个「单 token 操作数」。
 * 成功时把操作数末尾（不含）写入 *pe 并返回其类型；否则返回 OPERAND_NONE。
 */
static int operand_kind_forward(const char *p, const char *end,
                                const char **pe) {
    const char *q = p;
    while (q < end && (*q == ' ' || *q == '\t' ||
                       *q == '\n' || *q == '\r')) q++;
    if (q >= end) return OPERAND_NONE;

    /* 整数字面量（含 0xFF 这种十六进制） */
    if (isdigit((unsigned char)*q)) {
        const char *t = q;
        while (t < end && (isalnum((unsigned char)*t))) t++;
        if (t == q) return OPERAND_NONE;
        if (t[-1] == 'u' || t[-1] == 'U') { *pe = t; return OPERAND_UINT; }
        for (const char *z = q; z < t; z++) {
            if (*z == '.' || *z == 'e' || *z == 'E') return OPERAND_NONE;
        }
        *pe = t;
        return OPERAND_INT;
    }

    if (!is_ident_char(*q)) return OPERAND_NONE;

    const char *b = q;
    while (q < end && is_ident_char(*q)) q++;
    size_t nl = (size_t)(q - b);

    /* 后面紧跟 '(' -> 构造函数 / 函数调用 */
    const char *r = q;
    while (r < end && (*r == ' ' || *r == '\t' ||
                       *r == '\n' || *r == '\r')) r++;
    if (r < end && *r == '(') {
        int kind = OPERAND_NONE;
        if (nl == 4 && strncmp(b, "uint", 4) == 0) kind = OPERAND_UINT;
        else if (nl == 3 && strncmp(b, "int", 3) == 0) kind = OPERAND_INT;
        if (kind == OPERAND_NONE) return OPERAND_NONE;   /* 其它调用：不猜 */
        int d = 0;
        const char *t = r;
        while (t < end) {
            if (*t == '(') d++;
            else if (*t == ')') { d--; if (d == 0) { t++; break; } }
            t++;
        }
        if (d != 0) return OPERAND_NONE;
        *pe = t;
        return kind;
    }

    /* 纯标识符：查两张名字表 */
    if (uint_ident_find(b, nl)) { *pe = q; return OPERAND_UINT; }
    if (g_src_int_ready && int_ident_find(&g_src_int_idents, b, nl)) {
        *pe = q; return OPERAND_INT;
    }
    return OPERAND_NONE;
}

/*
 * 向后识别一个「单 token 操作数」（以 m 为末尾、不含）。
 * 成功时把操作数起点写入 *ps 并返回类型；否则返回 OPERAND_NONE。
 */
static int operand_kind_backward(const char *m, const char *s, const char **ps) {
    const char *t = m;
    while (t > s && (t[-1] == ' ' || t[-1] == '\t' ||
                     t[-1] == '\n' || t[-1] == '\r')) t--;
    if (t <= s) return OPERAND_NONE;

    /* 以 ')' 结尾：可能是 uint(..) / int(..)，也可能是别的调用 */
    if (t[-1] == ')') {
        const char *q = t;
        int d = 0;
        while (q > s) {
            if (q[-1] == ')') { d++; q--; continue; }
            if (q[-1] == '(') { if (d == 0) break; d--; q--; continue; }
            q--;
        }
        if (q <= s || q[-1] != '(') return OPERAND_NONE;
        const char *u = q - 1;
        while (u > s && (*u == ' ' || *u == '\t')) u--;
        const char *we = u;
        while (u > s && is_ident_char(u[-1])) u--;
        size_t ul = (size_t)(we - u);
        if (ul == 4 && strncmp(u, "uint", 4) == 0) { *ps = u; return OPERAND_UINT; }
        if (ul == 3 && strncmp(u, "int", 3) == 0)  { *ps = u; return OPERAND_INT;  }
        return OPERAND_NONE;        /* 其它调用：不猜 */
    }

    if (is_ident_char(t[-1]) && !isdigit((unsigned char)t[-1])) {
        /*
         * 标识符，可能带 `.x`/`.y` 这类分量选择。
         *
         * 【带分量选择时必须放弃 —— 这是第二道保险】
         *   分量选择只可能出现在**向量**上，而规则 P 只会给标量包 uint()。
         *   一旦这里带 `.`，说明我们对操作数的类型判断至少有一层是靠
         *   基名猜的（`a_LightAndData.z` 这种写法在 ivec/uvec 上很常见），
         *   与其猜，不如不动。
         *
         *   实证：Sodium 的 `(a_LightAndData.z & 1u) != 0u` 被改写成
         *   `(uint(a_LightAndData.z) & 1u) != 0u`。此处恰好是标量分量，
         *   改写无害；但同一段代码风格换个上下文就会踩到向量分量，
         *   那时 `uint(vec2分量)` 同样会静默取第 0 分量。
         *   放弃改写只是少修一处「本来就合法」的代码（桌面的 lenient 行为），
         *   代价远小于误改。
         */
        size_t bl = 0;
        const char *base = lhs_base_ident(t, s, &bl);
        if (bl == 0) return OPERAND_NONE;
        if (isdigit((unsigned char)base[0])) return OPERAND_NONE;
        if ((size_t)(t - base) > bl) return OPERAND_NONE;   /* 带 .xyz 分量 */
        if (uint_ident_find(base, bl)) { *ps = base; return OPERAND_UINT; }
        if (g_src_int_ready && int_ident_find(&g_src_int_idents, base, bl)) {
            *ps = base; return OPERAND_INT;
        }
        return OPERAND_NONE;
    }

    /* 数字/十六进制字面量 */
    if (isdigit((unsigned char)t[-1]) ||
        ((t[-1] >= 'a' && t[-1] <= 'f') || (t[-1] >= 'A' && t[-1] <= 'F'))) {
        const char *u = t;
        while (u > s && (isalnum((unsigned char)u[-1]))) u--;
        size_t ul = (size_t)(t - u);
        if (ul == 0) return OPERAND_NONE;
        if (u[ul - 1] == 'u' || u[ul - 1] == 'U') { *ps = u; return OPERAND_UINT; }
        for (size_t z = 0; z < ul; z++) {
            if (u[z] == '.' || u[z] == 'e' || u[z] == 'E') return OPERAND_NONE;
        }
        *ps = u;
        return OPERAND_INT;
    }
    return OPERAND_NONE;
}

/*
 * 规则 P 主体：按字节扫描，在每个候选运算符处向前/向后各识别一个单 token
 * 操作数；若一为 uint、一为 int，则给 int 侧包上 `uint(...)`。
 *
 * 处理顺序：先把（可能被包裹的）左右操作数写回，再跳过已消费区间。
 * 因为左右都只取「单 token」，跳过范围精确，不会漏掉后续运算符。
 *
 * 返回值：实际改写处数。调用方据此上报降级事件 ——
 * 本规则以前是完全静默的，真机测试时无法判断它有没有生效；
 * 有了计数就能在设备日志里直接确认。
 */
static int fix_mixed_int_uint(sbuf *out, const char *src) {
    const char *end = src + strlen(src);
    const char *p = src;
    int rewrites = 0;

    while (p < end) {
        /* 预处理器行整行原样复制（#define/#line 等不得改写） */
        {
            const char *nl = memchr(p, '\n', (size_t)(end - p));
            const char *line_end = nl ? nl : end;
            if (line_starts_with_directive(p, line_end)) {
                size_t n = (size_t)(line_end - p) + (nl ? 1 : 0);
                sbuf_put(out, p, n);
                p += n;
                continue;
            }
        }

        const char *skipped = skip_comment_or_string(p, end);
        if (skipped != NULL) {
            sbuf_put(out, p, (size_t)(skipped - p));
            p = skipped;
            continue;
        }

        char c = *p;
        int oplen = 0;
        int is_shift = 0;

        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
            c == '&' || c == '|' || c == '^') {
            if (c == '+' || c == '-') {
                if ((p + 1 < end && p[1] == c) || (p > src && p[-1] == c)) {
                    oplen = 0;                      /* ++ / -- 与一元 +/- */
                } else if (p + 1 < end && p[1] == '=') {
                    oplen = 2;                      /* += -= */
                } else {
                    oplen = 1;
                }
            } else if (c == '&' && p + 1 < end && p[1] == '&') {
                oplen = 0;                          /* && 要求 bool，不处理 */
            } else if (c == '|' && p + 1 < end && p[1] == '|') {
                oplen = 0;                          /* || 同上 */
            } else if (p + 1 < end && p[1] == '=') {
                oplen = 2;                          /* *= /= %= &= |= ^= */
            } else {
                oplen = 1;
            }
        } else if (c == '<' || c == '>') {
            if (p + 1 < end && p[1] == c) {
                is_shift = 1; oplen = 2;            /* << >> 合法，不处理 */
            } else if (p + 1 < end && p[1] == '=') {
                oplen = 2;                          /* <= >= 与 <<= >>= */
                if (p > src && p[-1] == c) oplen = 0;
            } else {
                oplen = 1;                          /* < > */
                if (p > src && p[-1] == c) oplen = 0;
            }
        } else if (c == '=') {
            if (p + 1 < end && p[1] == '=') oplen = 2;      /* == */
            else oplen = 1;                                 /* = */
        } else if (c == '!') {
            if (p + 1 < end && p[1] == '=') oplen = 2;      /* != */
        }

        if (oplen == 0 || is_shift) {
            sbuf_put(out, p, 1);
            p++;
            continue;
        }

        const char *ls = NULL;
        int lk = operand_kind_backward(p, src, &ls);
        if (lk == OPERAND_NONE) {
            sbuf_put(out, p, 1);
            p++;
            continue;
        }

        const char *re = NULL;
        int rk = operand_kind_forward(p + oplen, end, &re);
        if (rk == OPERAND_NONE) {
            sbuf_put(out, p, 1);
            p++;
            continue;
        }

        /* 只有「一为 uint、一为 int」才有改写必要 */
        if (!((lk == OPERAND_UINT && rk == OPERAND_INT) ||
              (lk == OPERAND_INT  && rk == OPERAND_UINT))) {
            sbuf_put(out, p, 1);
            p++;
            continue;
        }

        /*
         * 普通赋值 `=` 单独把关：它表达的是「把 int 赋给 uint 变量」，
         * 与混合运算同源，但左值必须是**声明的 uint 变量名本身**
         * （不能是 uint(...) 之类的表达式，那本来就是 uint）。
         */
        if (oplen == 1 && c == '=') {
            int plain = 1;
            for (const char *z = ls; z < p; z++) {
                if (!is_ident_char(*z)) { plain = 0; break; }
            }
            if (!(plain && uint_ident_find(ls, (size_t)(p - ls)))) {
                sbuf_put(out, p, 1);
                p++;
                continue;
            }
        }

        /*
         * 写回前必须先把已经写出的「左操作数原文」从输出中撤回。
         *
         * 【为什么 —— 这是端到端验证抓到的一个真实 bug】
         *   本函数是**逐字节**扫描并写出的：扫到运算符时，左操作数
         *   （例如 `gl_VertexID`）早已在之前若干次迭代里逐个字符写进了 out。
         *   若此处直接再写一遍左操作数，就会产出：
         *       gl_VertexID uint(gl_VertexID) - baseVertex      // 多了一份
         *   由于 [ls, p) 区间内的字节在此前都是**原样 1:1** 写出的
         *   （本函数只在改写处批量写出，其余都走单字符路径），
         *   因此 out 末尾的 (p - ls) 个字节恰好就是 [ls, p)。
         *   直接把 len 回退即可撤回，无需重新扫描。
         */
        if (p > ls && out->len >= (size_t)(p - ls)) {
            out->len -= (size_t)(p - ls);
            rewrites++;
        }

        /*
         * 写回。两种形态：
         *   int 侧在左： uint(<左>) OP <右>
         *   int 侧在右： <左> OP uint(<右>)
         */
        if (lk == OPERAND_INT) {
            /* 左操作数原文 = [ls, p)，其中 ls..(左操作数末尾) 是 token，
               其后是空白。分别取出 token 与空白，以便包住 token 而保留空白。 */
            const char *tokEnd = p;
            while (tokEnd > ls && (tokEnd[-1] == ' ' || tokEnd[-1] == '\t' ||
                                   tokEnd[-1] == '\n' || tokEnd[-1] == '\r')) {
                tokEnd--;
            }
            sbuf_puts(out, "uint(");
            sbuf_put(out, ls, (size_t)(tokEnd - ls));
            sbuf_puts(out, ")");
            sbuf_put(out, tokEnd, (size_t)(p - tokEnd));     /* 保留空白 */
            sbuf_put(out, p, (size_t)oplen);
            sbuf_put(out, p + oplen, (size_t)(re - (p + oplen)));
        } else {
            /*
             * 左操作数原样；右操作数包 uint()。
             *
             * 【空白归属必须精确 —— 这是测试抓到的一个真实缺陷】
             *   [p+oplen, re) 里除了右操作数 token，**前面还有一段空白**：
             *       baseVertex - gl_VertexID
             *                  ^ 运算符后、操作数前的空格
             *  若直接把整段塞进 uint(...)，会产出
             *       baseVertex -uint( gl_VertexID)
             *  虽然仍能编译，但语义上把空白挪进了括号，且可读性差、
             *  还可能让后续规则（如三元配平）的文本匹配失效。
             *  正确做法：先写出这段前导空白，再写 uint(token)。
             */
            const char *tok = p + oplen;
            while (tok < re && (*tok == ' ' || *tok == '\t' ||
                                *tok == '\n' || *tok == '\r')) {
                tok++;
            }
            sbuf_put(out, ls, (size_t)(p - ls));            /* 左操作数 + 空白 */
            sbuf_put(out, p, (size_t)oplen);                /* 运算符 */
            sbuf_put(out, p + oplen, (size_t)(tok - (p + oplen)));  /* 前导空白 */
            sbuf_puts(out, "uint(");
            sbuf_put(out, tok, (size_t)(re - tok));         /* 裸操作数 */
            sbuf_puts(out, ")");
        }

        p = re;
    }

    return rewrites;
}


/* ================= 阴影采样器精度（ES 下必须显式声明） ================= */

/*
 * 阴影采样器变量的名字表。
 *
 * 【用途】ES 的 `texture(sampler2DShadow, vec3)` 返回 **float**，
 * 而桌面 GLSL 的 `shadow2D`/`texture2DShadow` 返回 **vec4**。
 * 光影包因此在结果上写 `.x`/`.r`，在 ES 下就变成「对 float 取分量」：
 *     'scalar swizzle' : not supported
 * 这是整包里最大的一组错误（36 条、跨 13 个文件）。
 *
 * 修法是删除那个多余的 `.x`（float 上取 .x 语义上就是自身）。
 * 但删除必须有依据 —— 只有**确定该表达式是采样器调用结果**时才删，
 * 因此这里收集源码里声明为 sampler*Shadow 的变量名。
 */
#define SHADOW_SAMPLER_MAX 64
static char g_shadow_samplers[SHADOW_SAMPLER_MAX][48];
static int  g_shadow_sampler_count = 0;

static void collect_shadow_sampler_names(const char *src) {
    static const char *const kShadowTypes[] = {
        "sampler2DShadow", "samplerCubeShadow",
        "sampler2DArrayShadow", "sampler1DShadow", NULL
    };
    const char *end = src + strlen(src);
    g_shadow_sampler_count = 0;

    for (const char *p = src; p < end; p++) {
        if (p > src && is_ident_char(p[-1])) continue;
        for (int i = 0; i < (int)LIST_LEN(kShadowTypes) && kShadowTypes[i] != NULL; i++) {
            size_t tl = strlen(kShadowTypes[i]);
            if ((size_t)(end - p) < tl) continue;
            if (strncmp(p, kShadowTypes[i], tl) != 0) continue;
            if (p + tl < end && is_ident_char(p[tl])) continue;

            /* 类型名之后是声明符名（可能逗号分隔多个） */
            const char *q = p + tl;
            while (q < end) {
                while (q < end && (*q == ' ' || *q == '\t' ||
                                   *q == '\n' || *q == '\r')) q++;
                if (q >= end || !is_ident_char(*q)) break;
                const char *b = q;
                while (q < end && is_ident_char(*q)) q++;
                size_t nl = (size_t)(q - b);
                if (nl > 0 && nl < sizeof(g_shadow_samplers[0]) &&
                    g_shadow_sampler_count < SHADOW_SAMPLER_MAX) {
                    memcpy(g_shadow_samplers[g_shadow_sampler_count], b, nl);
                    g_shadow_samplers[g_shadow_sampler_count][nl] = '\0';
                    g_shadow_sampler_count++;
                }
                /* 跳过数组下标与初始化器，遇到 ',' 继续收下一个名字 */
                while (q < end && (*q == ' ' || *q == '\t')) q++;
                if (q < end && *q == '[') {
                    int br = 0;
                    while (q < end) {
                        if (*q == '[') br++;
                        else if (*q == ']') { br--; q++; if (br == 0) break; continue; }
                        q++;
                    }
                }
                while (q < end && (*q == ' ' || *q == '\t')) q++;
                if (q < end && *q == ',') { q++; continue; }
                break;   /* ';' 或 '=' 结束本声明 */
            }
            break;
        }
    }
}

static int is_shadow_sampler_name(const char *b, size_t len) {
    for (int i = 0; i < g_shadow_sampler_count; i++) {
        if (strlen(g_shadow_samplers[i]) == len &&
            strncmp(g_shadow_samplers[i], b, len) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * 从位置 q（指向 '.'）向后匹配 `.<单个分量>` 并返回该分量的结束位置；
 * 不是「点 + 单分量」则返回 NULL。
 * 单分量形式（`.x`）在 ES 的 float 上是非法的，需要删除。
 */
static const char *match_single_component(const char *q, const char *end) {
    if (q >= end || *q != '.') return NULL;
    const char *r = q + 1;
    if (r >= end) return NULL;
    char c = *r;
    if (c != 'x' && c != 'y' && c != 'z' && c != 'w' &&
        c != 'r' && c != 'g' && c != 'b' && c != 'a') {
        return NULL;
    }
    r++;
    /* 后面不能再接标识符字符（否则是 .xy 之类多分量，或别的名字） */
    if (r < end && is_ident_char(*r)) return NULL;
    return r;
}

/*
 * 判断位置 q（指向 '.'）是否为「阴影采样器调用结果上的单分量 swizzle」。
 *
 * 【判据】`.<c>` 的左侧是一个以 ')' 结尾的表达式，而该 ')' 配对到的 '('
 *   前面的函数名是 texture / textureLod / textureProj / shadow2D 之类，
 *   且调用里出现过**阴影采样器变量**。
 *   只有三条同时成立才删除，避免误伤 `vec4Var.x` 这类合法写法。
 *
 * 返回 1 表示应当删除这个 `.<c>`。
 */
static int shadow_swizzle_removable(const char *q, const char *s,
                                    const char *end) {
    if (q <= s) return 0;
    const char *t = q;
    while (t > s && (t[-1] == ' ' || t[-1] == '\t' ||
                     t[-1] == '\n' || t[-1] == '\r')) t--;
    if (t <= s || t[-1] != ')') return 0;

    /* 反向配对到 '(' */
    const char *u = t - 1;      /* 指向 ')' */
    int d = 0;
    while (u > s) {
        if (u[-1] == ')') { d++; u--; continue; }
        if (u[-1] == '(') {
            if (d == 0) break;
            d--; u--; continue;
        }
        u--;
    }
    if (u <= s || u[-1] != '(') return 0;

    /* '(' 前面的函数名 */
    const char *fn = u - 1;
    while (fn > s && (*fn == ' ' || *fn == '\t')) fn--;
    const char *fe = fn;
    while (fn > s && is_ident_char(fn[-1])) fn--;
    size_t fl = (size_t)(fe - fn);
    if (fl == 0) return 0;

    static const char *const kSampleFns[] = {
        "texture", "textureLod", "textureProj", "textureGrad",
        "texture2D", "texture2DShadow", "texture2DLod", "texture2DProj",
        "shadow2D", "shadow2DProj", "texelFetch", NULL
    };
    int is_sample = 0;
    for (int i = 0; i < (int)LIST_LEN(kSampleFns) && kSampleFns[i] != NULL; i++) {
        if (strlen(kSampleFns[i]) == fl &&
            strncmp(kSampleFns[i], fn, fl) == 0) { is_sample = 1; break; }
    }
    if (!is_sample) return 0;

    /* 实参列表里必须出现阴影采样器变量 */
    for (const char *z = u; z < t; z++) {
        if (z > u && is_ident_char(z[-1])) continue;
        if (!is_ident_char(*z)) continue;
        const char *b = z;
        while (z < t && is_ident_char(*z)) z++;
        if (is_shadow_sampler_name(b, (size_t)(z - b))) return 1;
    }
    return 0;
}

static int int_is_arith_operand(const char *p, const char *s, const char *e) {
    /* 左侧：跳过空白，看前一个字符 */
    const char *l = p;
    while (l > s && (l[-1] == ' ' || l[-1] == '\t' || l[-1] == '\n' ||
                     l[-1] == '\r')) {
        l--;
    }
    /* 右侧：跳过数字本身，再跳过空白 */
    const char *r = p;
    while (r < e && isdigit((unsigned char)*r)) r++;
    while (r < e && (*r == ' ' || *r == '\t' || *r == '\n' || *r == '\r')) r++;

    int left_arith  = 0;
    int right_arith = 0;

    if (l > s) {
        char c = l[-1];
        /* '*', '/', '+', '-', '%' 均为算术；'-' 也可能是一元负号，同样需浮点化 */
        if (c == '*' || c == '/' || c == '+' || c == '-' || c == '%') {
            /* 排除注释：'/' 后紧跟 '*' 是块注释起始；'*' 前是 '/' 是块注释结束 */
            if (c == '/' && is_comment_open(l - 1, e)) {
                /* 注释起始，不是除法 */
            } else if (c == '*' && is_comment_close(l - 1, s)) {
                /* 注释结束，不是乘法 */
            } else {
                left_arith = 1;
            }
        } else if (c == '<' || c == '>') {
            /*
             * 比较运算符在**左**侧（本字面量是右操作数）。
             * 例：`furthestPlane < 0`、`... > 1`。
             * `float < int` 在 GLSL ES 非法。
             * （排除 <= >= << >>）
             */
            if (!(l - 1 > s && (l[-2] == '<' || l[-2] == '>'))) {
                if (!(l < e && *l == '=')) left_arith = 1;
            }
        } else if (c == '=') {
            /*
             * 【本轮补充 —— 复合比较运算符 <= / >=】
             *   真机 Flywheel 的 wavelet.glsl：
             *       float t = coefficient_depth >= TRANSPARENCY_WAVELET_COEFFICIENT_COUNT ? ...
             *   宏展开后为 `coefficient_depth >= 16`。此处 `l[-1] == '='`，
             *   既不在 `* / + - %` 里，也不是裸 `<`/`>`，
             *   于是这条规则完全不匹配，`16` 保持 int —— 真机报：
             *       '>=' : no operation '>=' exists that takes a left-hand
             *              operand of type 'temp highp float' and a right
             *              operand of type 'const int'
             *   （`==`/`!=` 已在别处由规则 J 覆盖，这里只补 <= / >=，
             *     并且要排除 `=` 赋值与 `+=` 这类复合赋值。）
             */
            if (l - 1 > s && (l[-2] == '<' || l[-2] == '>')) {
                left_arith = 1;
            }
        } else if (c == ')') {
            /*
             * 左侧是函数调用/括号表达式的结尾。
             *
             * 【为什么需要 —— 真机 BSL 最后一个错误】
             *     int samples = int(min(planeDifference / sampleLength,
             *                           maxSamples) + 1);
             *   这里 `1` 左侧是 `)`，右侧是 `;`，既不是算术运算符的操作数，
             *   也不满足规则 D，于是长期漏改。
             *   判据必须严格：只有那个括号属于**浮点内建函数**时才改写。
             */
            if (left_is_float_builtin_call(p, s)) left_arith = 1;
        }
    }
    if (r < e) {
        char c = *r;
        if (c == '*' || c == '/' || c == '+' || c == '-' || c == '%') {
            if (c == '/' && is_comment_open(r, e)) {
                /* 下一行以注释开头，不是除法 */
            } else if (c == '*' && is_comment_close(r, s)) {
                /* 注释结束，不是乘法 */
            } else {
                right_arith = 1;
            }
        } else if (c == '<' || c == '>') {
            /*
             * 比较运算符。
             *
             * 【为什么必须包含 —— 真机 BSL 证据】
             *     float furthestPlane = max(lowerPlane, upperPlane);
             *     if (furthestPlane < 0) return vec4(0.0);
             *     ... (moonNormalX * moonNormalX + ...) > 1 ...
             *   `float < int` 在 GLSL ES 非法，报：
             *     '<' : no operation '<' exists that takes a left-hand
             *           operand of type 'float' and a right operand of
             *           type 'const int'
             *   上面只列了 '* / + - %'，把比较符漏在外面，于是这类全部未改。
             *
             * 安全性：调用方仅在【语句处于浮点上下文】且语句中
             * **没有整数变量**（stmt_has_int_ident == 0）时才会走到这里，
             * 因此 `rendertype == 0`（int 比较）不会被误改。
             */
            right_arith = 1;
        }
    }
    return left_arith || right_arith;
}

/*
 * 判断一个 token 是否是「裸**整数**字面量」。
 *
 * 必须与浮点字面量区分开：
 *     `16 - 1`    另一侧 16 是整数  -> 纯整数常量表达式，无需改写
 *     `0.5 * 2`   另一侧 0.5 是浮点 -> 必须把 2 改成 2.0（否则 float*int 非法）
 */
static int token_is_bare_int_literal(const char *b, size_t n) {
    if (n == 0 || !isdigit((unsigned char)b[0])) return 0;
    size_t digits = 0;
    int hex = (n > 2 && b[0] == '0' && (b[1] == 'x' || b[1] == 'X'));
    for (size_t i = 0; i < n; i++) {
        char c = b[i];
        if (isdigit((unsigned char)c)) { digits++; continue; }
        if (c == 'u' || c == 'U') continue;
        if (hex && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) continue;
        if (hex && (c == 'x' || c == 'X')) continue;
        return 0;       /* '.', 'e', 'f' 等 -> 浮点字面量 */
    }
    return digits > 0;
}

/*
 * 【用途 —— 修规则 B' 的一个真实误改】
 *   规则 B' 的放宽前提是：「纯整数运算一定会用到某个 int 变量
 *   （否则就是常量折叠，改不改都不影响结果）」。
 *   这个前提在宏展开之后**不再成立**：
 *       #define TRANSPARENCY_WAVELET_COEFFICIENT_COUNT 16
 *       add_to_index(coefficients, TRANSPARENCY_WAVELET_COEFFICIENT_COUNT - 1, addend);
 *   展开为
 *       add_to_index(coefficients, 16 - 1, addend);
 *   这是**纯常量表达式**：没有 int 变量，也没有任何浮点证据。
 *   但规则 B' 只看「紧邻运算符」就把它改成了 `16.0 - 1.0`，
 *   而目标形参是 `int index` —— 实参类型不匹配：
 *       'add_to_index' : no matching overloaded function found
 *
 *   常量折叠无需任何改写（`16 - 1` 在 ES 下完全合法），
 *   因此：字面量 p 的**另一侧操作数**若也是裸数字字面量，
 *   就说明这是纯常量表达式，放弃改写。
 *
 *   判据：从两侧找出运算符，跳过它，再看另一侧的操作数类型。
 *   注意另一侧必须是**整数**字面量才放弃；若是浮点字面量
 *   （`0.5 * 2`）则仍需把 `2` 改成 `2.0`。
 */
static int arith_other_side_is_bare_literal(const char *p, size_t n,
                                            const char *s, const char *e) {
    /* 左侧：跳过空白，若遇运算符则跳过它再取操作数 */
    {
        const char *l = p;
        while (l > s && (l[-1] == ' ' || l[-1] == '\t' ||
                         l[-1] == '\n' || l[-1] == '\r')) l--;
        if (l > s && (l[-1] == '+' || l[-1] == '-' || l[-1] == '*' ||
                      l[-1] == '/' || l[-1] == '%')) {
            const char *m = l - 1;
            while (m > s && (m[-1] == ' ' || m[-1] == '\t' ||
                             m[-1] == '\n' || m[-1] == '\r')) m--;
            const char *t = m;
            while (t > s && (is_ident_char(t[-1]) || t[-1] == '.')) t--;
            if (t < m && token_is_bare_int_literal(t, (size_t)(m - t))) return 1;
        }
    }
    /* 右侧：同理 */
    {
        const char *r = p + n;
        while (r < e && (*r == ' ' || *r == '\t' ||
                         *r == '\n' || *r == '\r')) r++;
        if (r < e && (*r == '+' || *r == '-' || *r == '*' ||
                      *r == '/' || *r == '%')) {
            const char *m = r + 1;
            while (m < e && (*m == ' ' || *m == '\t' ||
                             *m == '\n' || *m == '\r')) m++;
            const char *t = m;
            while (t < e && (is_ident_char(*t) || *t == '.')) t++;
            if (t > m && token_is_bare_int_literal(m, (size_t)(t - m))) return 1;
        }
    }
    return 0;
}

static int is_float_vec_type(const char *s, size_t len);

/*
 * 处理单条语句，把浮点上下文中的整数字面量改写为浮点字面量。
 *
 * 【改写决策规则 —— 必须足够保守】
 *
 *   规则 A（高置信）：位于浮点构造函数（vecN/matN）实参内。
 *       例：vec4(1, 1, 1, 1)  ->  vec4(1.0, 1.0, 1.0, 1.0)
 *       即使嵌套在更深层的括号里也算（如 vec4((a/b) * 2 - 1, ...) 中的 2 与 1）。
 *       依据：构造函数实参的转换是 GLSL ES 明确允许的，因此这些整数
 *       一定是浮点语义。
 *
 *   规则 B（中置信）：语句整体处于浮点上下文，且该整数是【算术运算】
 *       （* / + -）的操作数。
 *       例：vec2 scaled = uv * 3;  ->  uv * 3.0
 *
 * 【为什么规则 B 必须限定在算术运算符旁 —— 这是一个真实的陷阱】
 *   FML 的片元着色器有这样一行：
 *       if (rendertype == 0) fragColor = vec4(1,1,1,texture(tex, fTex).r) * fColour;
 *   语句里有 vec4，因此整条语句被判定为「浮点上下文」。
 *   但其中的 `0` 是【整数比较】的操作数（rendertype 是 int）。
 *   若不加限制地把语句内所有整数都浮点化，会得到 `rendertype == 0.0`，
 *   即 int 与 float 比较 —— 在 GLSL ES 中同样是非法运算。
 *   那样只是把「编译失败的原因」换了一个，问题并未解决。
 *   限定为「算术运算符的操作数」即可排除比值/比较/位运算，安全得多。
 *
 * 【已知残余局限】
 *   形如 vec2 v = vec2(i + 1, 0.0)（i 为 int）的写法，`1` 会被改为 1.0，
 *   使 `i + 1.0` 成为 int+float 错误。此类「浮点构造实参里嵌整数算术」
 *   的写法极少见，且需要完整的类型推导才能正确处理。
 *   权衡后选择不改写错误方向：宁可漏改（保持现状）也不误改（破坏
 *   本来正确的着色器）。
 */

/* 规则 H 的实现位于 starts_with_float_literal 之后，这里前置声明。 */
static int in_float_builtin_arglist(const char *p, const char *s,
                                    const char *e,
                                    const int_ident_table *int_idents);

/*
 * int(...) 守卫的豁免判据，实现位于 left_is_float_builtin_call 附近，
 * 这里前置声明（它用到后面才定义的 ends_with_float_literal 与
 * lhs_base_ident）。
 */
static int int_ctor_lhs_is_float_operand(const char *p, const char *s,
                                         const char *e,
                                         const int_ident_table *int_idents);

/*
 * 规则 J 的判据，实现位于 int_ctor_lhs_is_float_operand 附近。
 * 它需要**整源区间**（src_start/src_end）来做「该名字是否真的被声明为浮点」
 * 的正向确认，因此不能只传语句区间。
 */
static int float_lhs_eq_int(const char *p, const char *s,
                            const char *src_start, const char *src_end);

static void process_stmt(sbuf *out, const char *s, const char *e,
                         const int_ident_table *int_idents) {
    int stmt_float = stmt_is_float_context(s, e);

    /*
     * 语句里是否出现整数变量。
     *
     * 【为什么需要这个】见 int_ident_table 的说明：
     *   `float(i + 1)` 里的 `1` 曾被我方浮点化成 `i + 1.0`，
     *   把本来合法的代码改成了非法的 int+float。
     *   只要语句中出现 int 变量，该语句的整数【字面量】就必须保持整数。
     *
     * 浮点构造实参 vecN/matN(...) 例外：那里的字面量可安全浮点化
     *   （glslang 实测：vec2(0,0) / vec4(1,1,1,1) 均合法）。
     */
    int stmt_has_int_ident = 0;
    if (int_idents != NULL && int_idents->count > 0) {
        const char *p = s;
        while (p < e) {
            const char *skipped = skip_comment_or_string(p, e);
            if (skipped != NULL) { p = skipped; continue; }
            if (!is_ident_char(*p)) { p++; continue; }
            const char *q = p;
            while (q < e && is_ident_char(*q)) q++;
            if (int_ident_find(int_idents, p, (size_t)(q - p))) {
                stmt_has_int_ident = 1;
                break;
            }
            p = q;
        }
    }

    /*
     * 括号栈：记录每层括号的性质
     *    1 = 浮点构造函数实参列表
     *   -1 = 整数构造函数实参列表
     *    0 = 其它（函数调用、分组等）
     * 同时维护两个计数器，因为内层声明会遮蔽外层：
     * 只要 int_ctor_depth > 0 就一律不改写；
     * 只要 float_ctor_depth > 0 就按规则 A 改写。
     */
    int stack[32];
    int depth = 0;
    int int_ctor_depth = 0;
    int float_ctor_depth = 0;
    int bracket = 0;        /* 数组下标深度 */

    const char *p = s;
    while (p < e) {
        /* 行注释：原样复制 */
        if (p + 1 < e && p[0] == '/' && p[1] == '/') {
            const char *nl = memchr(p, '\n', (size_t)(e - p));
            size_t n = nl ? (size_t)(nl - p + 1) : (size_t)(e - p);
            sbuf_put(out, p, n);
            p += n;
            continue;
        }
        /* 块注释：原样复制 */
        if (p + 1 < e && p[0] == '/' && p[1] == '*') {
            const char *close = NULL;
            for (const char *q = p + 2; q + 1 < e; q++) {
                if (q[0] == '*' && q[1] == '/') { close = q + 2; break; }
            }
            size_t n = close ? (size_t)(close - p) : (size_t)(e - p);
            sbuf_put(out, p, n);
            p += n;
            continue;
        }

        /* 括号与下标：维护栈 */
        if (*p == '(') {
            /*
             * 判断这个 '(' 是否属于某个构造函数：向前跳过空白，
             * 取紧邻的标识符与名字表比对。
             */
            const char *q = p;
            while (q > s && (q[-1] == ' ' || q[-1] == '\t')) q--;
            const char *end = q;
            while (q > s && is_ident_char(q[-1])) q--;
            size_t nlen = (size_t)(end - q);

            int kind = 0;
            if (nlen > 0) {
                if (name_in_list(q, nlen, FLOAT_CTORS, LIST_LEN(FLOAT_CTORS))) {
                    kind = 1;
                    float_ctor_depth++;
                } else if (name_in_list(q, nlen, INT_CTORS, LIST_LEN(INT_CTORS))) {
                    kind = -1;
                    int_ctor_depth++;
                }
            }
            if (depth < 32) stack[depth] = kind;
            depth++;
            sbuf_put(out, p, 1);
            p++;
            continue;
        }
        if (*p == ')') {
            if (depth > 0) {
                depth--;
                if (depth < 32) {
                    if (stack[depth] == -1) int_ctor_depth--;
                    else if (stack[depth] == 1) float_ctor_depth--;
                }
            }
            sbuf_put(out, p, 1);
            p++;
            continue;
        }
        if (*p == '[') { bracket++; sbuf_put(out, p, 1); p++; continue; }
        if (*p == ']') { if (bracket > 0) bracket--; sbuf_put(out, p, 1); p++; continue; }

        /*
         * ===== 删除阴影采样结果上的多余单分量 swizzle =====
         *
         * 【真机/整包最大的一组错误：36 条、跨 13 个文件】
         *   桌面 GLSL 的 shadow2D / texture2DShadow 返回 **vec4**，
         *   光影包因此在结果上写 `.x` 或 `.r`：
         *       return texture(shadowtex, shadowPos).x;
         *   而 GLSL ES 的 texture(sampler2DShadow, vec3) 返回 **float**
         *   （阴影采样只给一个深度比较结果），对 float 取 `.x` 非法：
         *       'scalar swizzle' : not supported
         *
         * 【为什么直接删除是正确的】float 上取 `.x` 语义上就是它自身，
         *   删除后表达式类型与语义都不变。
         *
         * 【为什么必须严格判定才删】`vec4Var.x` 是合法且常见的写法，
         *   无条件删除会大面积破坏代码。这里要求
         *     1. 后面是「点 + 单个分量」（`.xy` 这种多分量不删）；
         *     2. 左侧以 ')' 结尾，且配对 '(' 前面是纹理采样函数名；
         *     3. 该调用的实参里出现过采样器源码中声明为 sampler*Shadow 的变量。
         *   三条同时成立才删（见 shadow_swizzle_removable）。
         */
        if (*p == '.' && bracket == 0) {
            const char *comp_end = match_single_component(p, e);
            if (comp_end != NULL && shadow_swizzle_removable(p, s, e)) {
                p = comp_end;      /* 跳过整个 `.<c>`，不写出任何字节 */
                continue;
            }
        }

        /*
         * 数字 token 的统一解析。
         *
         * 【必须先尝试浮点，且必须整体跳过】
         *   这一步曾出现严重 bug：改写 `vec4(..., 1.0)` 时，
         *   扫描器在 `1` 处未识别为浮点（因为把 `1.0` 拆开看了），
         *   于是把尾部的 `0` 当成独立整数改写，产出 `1.0.0`。
         *   正确做法：在数字起点先尝试 float_literal_len；
         *   命中则整块原样复制（浮点字面量无需改写），
         *   并且绝不能从它的中间位置再次进入数字处理。
         */
        if (bracket == 0 &&
            /*
             * 【int(...) 的实参是浮点域，必须例外】
             *
             *   真机 BSL 证据（错误 2/3 的根因）：
             *       int samples = int(min(planeDifference / sampleLength,
             *                             maxSamples) + 1);
             *   `int(...)` 的实参表达式类型是 float（`min(float,float)+1`），
             *   最后才被 int() 收敛成 int。因此实参内部的 `1` **必须**是
             *   浮点，否则报：
             *       '+' : no operation '+' exists that takes a left-hand
             *             operand of type 'const float' and a right operand
             *             of type 'const int'
             *   旧代码无条件要求 int_ctor_depth == 0，于是 int(...) 内部的
             *   数字**完全不进入**任何规则（规则 D/E/G 全部失效），
             *   这一类长期漏改。
             *
             *   放宽条件：只要该字面量的左侧是**浮点内建函数调用**
             *   （left_is_float_builtin_call 会自己跳过一层算术运算符，
             *    所以 `min(...) + 1` 里的 `1` 也能命中），
             *   就允许在 int(...) 内部继续按常规规则处理。
             *
             *   注意：`float(maxSamples)` 那种「整数实参」原本就不需要
             *   浮点化（min 的两个实参由情形 F 单独包 float()），
             *   这里放宽不会误伤 —— 因为进入下方规则后仍要过
             *   int_is_arith_operand / 规则 D 的严格判据。
             *
             *   【补充：仅左侧为浮点内建还不够 —— 真机 terrain_solid.vsh】
             *       int blockID = int(mc_Entity.x / 100);
             *   真机只报这一条，整条渲染管线因此失败：
             *       '/' : no operation '/' exists that takes a left-hand
             *             operand of type 'float' and a right operand of type
             *             'const int'
             *   `int(...)` 的实参是 `mc_Entity.x / 100`，其中 `mc_Entity.x`
             *   是 float。但 `int` 不是浮点内建函数，旧的豁免条件不成立，
             *   于是 `100` 被整段跳过。
             *
             *   正确认识：**整数构造函数的实参表达式本身就是浮点域**
             *   （构造函数的职责就是把这个浮点值收敛成整数）。
             *   因此这里把豁免扩展到两类：
             *     (a) 左侧是浮点内建函数调用（旧行为，覆盖 int(min(..)+1)）
             *     (b) 左侧是**算术运算符且其左操作数属于浮点域**
             *         （覆盖 mc_Entity.x / 100 这类「float op int」）
             *   两者都只放宽「是否允许进入下方规则」，
             *   真正的改写仍由 int_is_arith_operand 等严格判据决定，
             *   所以 `int(a * 2)`（a 为 int）不会被误改 ——
             *   因为 a 既不在浮点表里，其左侧也不是浮点内建调用。
             *
             *   【★补充 == / !=：真机 terrain_translucent.fsh 的第二个错误★】
             *       float cloudBlendOpacity = ...;
             *       if (cloudBlendOpacity == 0) discard;
             *   驱动报：
             *       '==' : no operation '==' exists that takes a left-hand
             *              operand of type 'float' and a right operand of type
             *              'const int'
             *   情形 C 的运算符集合当初**刻意排除**了 '=' 与 '!'
             *   （为避免与复合赋值、==/!= 混淆），结果 `==`、`!=` 里
             *   「浮点在左、整数在右」这一方向**任何规则都没覆盖**，
             *   从第一轮一直遗漏到现在。
             *   这里补上：仅当左侧是已知浮点、且右侧确实是整数时才改写，
             *   因此 `intVar == intVar2`、枚举比较等合法写法不受影响。
             */
            (int_ctor_depth == 0 ||
             left_is_float_builtin_call(p, s) ||
             int_ctor_lhs_is_float_operand(p, s, e, int_idents) ||
             float_lhs_eq_int(p, s, g_src_begin, g_src_end)) &&
            !(p > s && is_ident_char(p[-1])) &&
            (isdigit((unsigned char)*p) || *p == '.')) {
            size_t fl = float_literal_len(p);
            if (fl > 0) {
                /* 已是浮点字面量，原样保留（绝不二次改写） */
                sbuf_put(out, p, fl);
                p += fl;
                continue;
            }
            size_t n = int_literal_len(p);
            if (n > 0) {
                int convert = 0;
                if (in_macro_arg(p, s)) {
                    /* 规则 M：宏实参保持原样（见 in_macro_arg 的说明） */
                    sbuf_put(out, p, n);
                    p += n;
                    continue;
                }
                /*
                 * ---- 规则 R：用户自定义函数的实参类型 ----
                 *
                 * 【为什么必须放在规则 A 之前】
                 *   规则 A 是「在 float 构造函数的实参列表里 -> 一律浮点化」，
                 *   它只知道自己身处 `vec4(` 之类里面，**不知道具体函数**。
                 *   于是
                 *       float c(int n) { ... }
                 *       o = vec4(c(1), 1.0);
                 *   的 `c(1)` 被改成 `c(1.0)`，ES 报
                 *       'c' : no matching overloaded function found
                 *   桌面 GLSL 允许 int->float 隐式转换，所以桌面不报错。
                 *   实测确认这是**整类**问题（见 probe_userfn_int_args.frag）：
                 *   int/uint 形参都会中招；而经由 `ivec2(3)` 传入的反而正确。
                 *
                 * 【与规则 N 的关系】
                 *   规则 N 处理的是「uint 变量旁边的字面量」，同样要补 `u`；
                 *   两条规则针对**不同来源**的同一需求
                 *   （一个是变量，一个是函数形参），因此分别放置、互不冲突。
                 */
                if (g_fn_sigs_ready) {
#ifndef GLESMOD_NO_RULE_R
                    int cls = user_fn_param_class(p, s, &g_fn_sigs);
                    if (cls == 1) {
                        /* int / ivecN 形参：保持裸整数字面量 */
                        sbuf_put(out, p, n);
                        p += n;
                        continue;
                    }
                    if (cls == 2) {
                        /*
                         * uint / uvecN 形参：必须补 u 后缀 ——
                         * 但**仅当字面量还没有后缀**。
                         *
                         * 【实测踩到的 bug】int_literal_len 会把已有的
                         * `u`/`U` 后缀算进返回长度（`3u` -> n=2），
                         * 若再无脑追加一个 `u`，就得到 `3uu`（非法记号）。
                         * 探针里 `inc(3u)` 当场变成 `inc(3uu)`。
                         */
                        sbuf_put(out, p, n);
                        if (!(n > 0 && (p[n - 1] == 'u' || p[n - 1] == 'U'))) {
                            sbuf_puts(out, "u");
                        }
                        p += n;
                        continue;
                    }
                    if (cls == 0) {
                        /*
                         * 浮点形参：ES 无 int->float 隐式转换，必须补 `.0`。
                         *
                         * 【必须在这里直接输出并 continue，不能只置 convert=1】
                         *   convert 之后还会被「规则 A / B / B' / D」重新赋值：
                         *       if (float_ctor_depth > 0) convert = 1;
                         *       else if (stmt_float) convert = int_is_arith_operand(...);
                         *   对 `ratio(4)` 而言 float_ctor_depth==0、
                         *   stmt_float==1（语句里有 float 关键字），
                         *   而 `4` 后面是 ')' 不是算术运算符，
                         *   于是 int_is_arith_operand 返回 0，**把 convert 冲掉**，
                         *   结果 `ratio(4)` 依旧没补 `.0`（实测确认）。
                         *   签名表既然已经**确定**了形参类型，就应像 cls==1/2
                         *   那样当即收尾，不再让后续启发式覆盖这个确定结论。
                         */
                        sbuf_put(out, p, n);
                        sbuf_puts(out, ".0");
                        p += n;
                        continue;
                    }
#else
                    (void)user_fn_param_class;   /* 对照构建：禁用规则 R */
#endif
                }
                /*
                 * ---- 规则 N：uint 标量旁的整数字面量必须补 `u` 后缀 ----
                 *
                 * 【为什么放在这里、且必须 `continue`】
                 *   它决定的是**字面量自身的类型**（int -> uint），
                 *   而下方全部规则都在决定「要不要变成浮点」。
                 *   两者互斥：一个 uint 操作数旁边的字面量只能是 int 或 uint，
                 *   绝不该变成 float。因此这里先判、命中即收尾。
                 *
                 * 【为什么不会与浮点规则冲突】
                 *   本规则要求左操作数被正向确认为 uint 标量，
                 *   而 `uint OP int字面量` 在 GLSL ES 下**本来就是非法的**
                 *   （见 uint_int_literal_needs_u_suffix 的实测记录）。
                 *   也就是说：凡是本规则会命中的地方，输入着色器都已经编译不过，
                 *   不存在「把一个本来正确的浮点上下文改坏」的可能。
                 */
                if (uint_int_literal_needs_u_suffix(p, s)) {
                    sbuf_put(out, p, n);
                    sbuf_puts(out, "u");
                    p += n;
                    continue;
                }
                /*
                 * ---- 规则 S：内建函数的整型实参位置（缺陷 A）----
                 *
                 * 【真机证据】native.log 14:20:04，shader=101
                 *   RAW : vec2 screen_size = vec2(textureSize(DiffuseSampler0, 0));
                 *   我们原样把 lod 改成了 0.0，驱动报：
                 *       ERROR: 0:21: 'textureSize' : no matching overloaded function found
                 *       ERROR: 0:47: 'textureSize' : no matching overloaded function found
                 *   因为 `textureSize(sampler2D, int lod)` 的 lod **必须是 int**，
                 *   而外层 `vec2(...)` 让 float_ctor_depth > 0，
                 *   紧接着的规则 A 就把 lod 浮点化了。
                 *
                 * 【为什么不能只靠 FLOAT_CTORS / INT_CTORS】
                 *   那两张表描述的是「**构造函数**的实参」，靠括号栈的
                 *   int_ctor_depth / float_ctor_depth 计数实现，表达不了
                 *   「**某个函数的第 N 个实参**」这种位置信息。
                 *   因此单独用 INT_ARG_BUILTINS 做位置判定。
                 *
                 * 【同一位置上的相反要求 —— 必须按函数区分】
                 *   textureSize(s, int lod)      -> lod 是 int，绝不能补 .0
                 *   textureLod(s, uv, float lod) -> lod 是 float，**必须**补 .0
                 *   （textureLod 不在表里；实测确认我们补对了，见探针
                 *     textureLod_literal 的归因为「我们修对了」。）
                 *   两者写法一模一样，只有函数名不同 —— 一刀切必然错一个。
                 *
                 * 【安全性】命中即原样输出并收尾：本规则只**保留**整数，
                 *   不做任何其他改写，因此不存在把浮点上下文改坏的可能。
                 */
#ifndef GLESMOD_NO_RULE_S
                if (in_builtin_int_arg(p, s)) {
                    sbuf_put(out, p, n);
                    p += n;
                    continue;
                }
#else
                (void)in_builtin_int_arg;   /* 对照构建：禁用规则 S */
#endif
                if (float_ctor_depth > 0) {
                    convert = 1;            /* 规则 A */
                } else if (stmt_float) {
                    convert = int_is_arith_operand(p, s, e);   /* 规则 B */
                    /*
                     * 规则 B 与规则 B' 共用同一条安全约束：纯常量表达式
                     * （运算符另一侧也是整数常量）无需改写。
                     * 真机 Flywheel 的
                     *     float scale_coefficient = get_coefficients(coefficients,
                     *                                              16 - 1);
                     * 就落在这一支：语句含 `float` 关键字 -> stmt_float 为真
                     * -> 规则 B 把 `16 - 1` 改成 `16.0 - 1.0`，
                     * 而形参是 `int index` -> 实参类型不匹配。
                     */
                    if (convert &&
                        arith_other_side_is_bare_literal(p, n, s, e)) {
                        convert = 0;
                    }
                }
                /*
                 * 规则 B'：语句里没有整数变量时，只按「相邻运算符」判定，
                 * **不再要求 stmt_float**。
                 *
                 * 【为什么必须放宽 —— 整包验证暴露的真实缺陷】
                 *   stmt_is_float_context 只看三类证据：浮点字面量、
                 *   `float` 关键字、`vecN/matN` 类型名。而 BSL 里大量存在
                 *   这样的语句：
                 *       vec2 testCoord = ...;
                 *       if (testCoord.x > -2 && testCoord.x < 3) { ... }
                 *   整句只有整数常量与**浮点变量名**，三类证据一个都没有，
                 *   于是 stmt_float 为 0，规则 B 根本不执行，
                 *   `-2`、`3` 原样留下 -> `float > int` 编译失败。
                 *   这类语句在 composite5.fsh 里一次报了 4 条错。
                 *
                 * 【为什么放宽是安全的】
                 *   int_is_arith_operand 要求字面量**紧邻** * / + - % < >，
                 *   且此处已确认语句中没有任何 int/uint 变量
                 *   （stmt_has_int_ident 为 0）。纯整数运算一定会用到
                 *   某个 int 变量（否则就是常量折叠，改不改都不影响结果），
                 *   因此不会把合法的整数算术改坏。
                 *   赋值号、逗号、括号都不在运算符集合内，所以
                 *   `f(2)`、`int x = 2` 之类不会被误改。
                 */
                if (!convert && !stmt_has_int_ident) {
                    if (int_is_arith_operand(p, s, e) &&
                        !arith_other_side_is_bare_literal(p, n, s, e)) {
                        convert = 1;
                    }
                }
                /*
                 * 规则 D：整数字面量就是整个右值，而左侧是浮点声明。
                 *
                 * 【真机 + glslang 双重证据】
                 *     float cloudThickness = 5;
                 *   真机 Adreno 报：
                 *     ERROR: 0:1591: '=' : cannot convert from 'const int' to 'float'
                 *   glslang 报同样的错：
                 *     '=' : cannot convert from ' const int' to ' temp highp float'
                 *   桌面 GLSL 允许这种隐式转换，光影包因此大量这么写。
                 *
                 * 【纠正一个长期存在的错误认识】
                 *   本项目主机测试里曾写着「GLSL ES 允许初始化处的隐式 int->float」，
                 *   并断言 `float a = 2;` 必须保持整数。**该断言是错的。**
                 *   用 glslang 实测 `float a = 2;` 直接报错（见
                 *   native/tools/.cache/probe_int_float_init.vert）。
                 *   那个错误断言之所以长期没被发现，是因为：
                 *     - 审计只跑 MC 自带着色器（没有这种写法）
                 *     - 夹具里也恰好没有「纯整数字面量赋给 float」
                 *   教训：不要凭「看起来更保守」就接受一个假设，
                 *   凡是类型规则都用 glslang 实测。
                 *
                 * 【必须与构造函数区分】`vec4(1,1,1,1)` 是**合法**的
                 *   （vecN 构造函数的实参允许隐式转换，已实测）。
                 *   所以本规则只在「字面量是整个右值」时生效，
                 *   构造函数内的字面量由规则 A 与本规则的交集处理，
                 *   而规则 C（语句含整数变量）依然优先。
                 *
                 * 判据：
                 *   - 语句处于浮点上下文
                 *   - 字面量左边（跳空白）是 '=' 且不是 '==' / '!=' / '<=' / '>='
                 *   - 字面量右边（跳空白）就是语句结束（语句区间是 [s,e)，e 指向分隔符）
                 *   - 赋值号左侧确实含 float/vecN/matN 类型名（避免改坏 `int x = 2;`）
                 */
                if (!convert && stmt_float && !stmt_has_int_ident) {
                    const char *l = p;
                    while (l > s && (l[-1] == ' ' || l[-1] == '\t' ||
                                     l[-1] == '\n' || l[-1] == '\r')) {
                        l--;
                    }
                    if (l > s) l--;
                    const char *r2 = p + n;
                    while (r2 < e && (*r2 == ' ' || *r2 == '\t' ||
                                      *r2 == '\n' || *r2 == '\r')) {
                        r2++;
                    }
                    if (r2 >= e && l > s && *l == '=' &&
                        !(l[-1] == '=') &&
                        !(l > s + 1 && (l[-1] == '!' || l[-1] == '<' ||
                                        l[-1] == '>'))) {
                        int lhs_decl = 0;
                        for (const char *q2 = s; q2 < l; q2++) {
                            if (q2 + 5 <= l && strncmp(q2, "float", 5) == 0 &&
                                (q2 == s || !is_ident_char(q2[-1])) &&
                                (q2 + 5 >= l || !is_ident_char(q2[5]))) {
                                lhs_decl = 1;
                                break;
                            }
                        }
                        if (!lhs_decl) {
                            for (const char *q2 = s; q2 < l; q2++) {
                                if (q2 > s && is_ident_char(q2[-1])) continue;
                                const char *q3 = q2;
                                while (q3 < l && is_ident_char(*q3)) q3++;
                                if (q3 > q2 &&
                                    is_float_vec_type(q2, (size_t)(q3 - q2))) {
                                    lhs_decl = 1;
                                    break;
                                }
                            }
                        }
                        if (lhs_decl) convert = 1;
                    }
                }
                /*
                 * 规则 C：语句中出现整数变量 -> 整数域，字面量保持整数。
                 *
                 * `float(i + 1)`（i 为 int）必须原样保留：
                 * 若把 1 改成 1.0，得到 int + float，GLSL ES 直接拒绝。
                 * 这条规则正是为修复该缺陷而加（曾把 end_portal 着色器改坏）。
                 *
                 * 【例外只有一个】整数字面量紧跟在**浮点内建函数调用**之后
                 * （或位于其混合类型实参列表内）—— 那里的浮点是确定的。
                 * 真机 BSL 证据：
                 *     int samples = int(min(planeDifference / sampleLength,
                 *                           maxSamples) + 1);
                 * 语句里出现了 int 变量 samples，规则 C 本会拒绝一切浮点化，
                 * 于是 `+ 1` 保持为 `float + int`，报：
                 *     '+' : no operation '+' exists that takes a left-hand
                 *           operand of type 'const float' and a right operand
                 *           of type 'const int'
                 *
                 * 【特别注意：不能把 float_ctor_depth > 0 也当作例外】
                 *   构造函数的实参里出现整数是**合法**的（`vec4(1,1,1,1)`
                 *   在 ES 下编译通过），主机端测试明确要求这种写法保持整数：
                 *       [FAIL] 含 int 变量的语句中 vec4(1,1,1,...) 保持整数
                 *   曾把 `float_ctor_depth > 0` 一并列为例外，结果被测试当场
                 *   抓住 —— 规则 C 必须无条件压过规则 A。
                 */
                if (stmt_has_int_ident) {
                    if (!left_is_float_builtin_call(p, s)) {
                        convert = 0;
                    }
                }
                /*
                 * 规则 E：整数字面量紧跟在**浮点内建函数调用**之后。
                 *
                 * 【为什么必须独立于规则 B】规则 B 的前提是 stmt_float
                 * 为真（语句里有 float 关键字 / 浮点字面量 / vecN）。
                 * 而真机 BSL 的这条语句：
                 *     int samples = int(min(planeDifference / sampleLength,
                 *                           maxSamples) + 1);
                 * 三者都不具备，stmt_float 为 0，规则 B 根本不执行，
                 * 于是 `+ 1` 始终没被浮点化。
                 * 这里独立判定：只要左侧是浮点内建调用的右括号即改写。
                 */
                if (!convert && left_is_float_builtin_call(p, s)) convert = 1;
                /* 规则 G：整数型采样函数的 LOD 实参必须是浮点 */
                if (!convert && in_float_lod_arg(p, s)) convert = 1;
                /*
                 * 规则 H：字面量落在**浮点内建函数的实参列表**内。
                 *
                 * 放在规则 C 之后、与 E/G 并列，是为了让「语句含整数变量」
                 * 的保守判定仍然优先；H 自己会检查实参列表内确有浮点证据，
                 * 因此不会把 `max(intA, 2)` 改坏。
                 */
                if (!convert && in_float_builtin_arglist(p, s, e,
                                                        int_idents)) convert = 1;
                /*
                 * 规则 I：字面量位于 int(...) 内部，且左侧算术运算的
                 * 左操作数属于浮点域。
                 *
                 * 【真机 BSL 018_terrain_solid.vsh 第 141 行，阻塞整条管线】
                 *     int blockID = int(mc_Entity.x / 100);
                 *   真机只报这一条错：
                 *     '/' : no operation '/' exists that takes a left-hand
                 *           operand of type 'float' and a right operand of
                 *           type 'const int'
                 *
                 * 【为什么必须在这里再加一条】
                 *   同样的判据已经加在数字处理的**进入条件**上
                 *   （见 process_stmt 顶部的 int_ctor_depth 豁免），
                 *   但那一层只决定「是否进入处理」。
                 *   真正决定改不改的是下面这条：
                 *     规则 C：语句中出现整数变量时 convert 归零。
                 *   本语句里 `blockID` 正是整数变量（int blockID = ...），
                 *   于是规则 C 把 convert 又清成了 0 ——
                 *   进入条件放宽了，却在这里被规则 C 拦回去。
                 *
                 *   而 `int(...)` 的实参**必然是浮点域**（构造函数就是
                 *   把浮点收敛成整数），所以这点必须压过规则 C。
                 *
                 * 【安全性】判据要求「左操作数不是已知整型变量」，
                 *   因此 `int(intVar / 100)` 这类纯整数运算不会被改写。
                 */
                if (!convert && int_ctor_depth > 0 &&
                    int_ctor_lhs_is_float_operand(p, s, e, int_idents)) {
                    convert = 1;
                }
                /*
                 * ---- 规则 J：整数字面量是 `floatVar == / != <lit>` 的右操作数 ----
                 *
                 * 【为什么必须放在这里，而不是只写进外层守卫】
                 *   外层守卫只是「允许进入数字处理」，真正的改写由本决策链
                 *   决定。把条件只加在守卫上会漏改 —— 这正是本轮踩到的坑
                 *   （规则 I 当初也犯过同样的错）。
                 *
                 * 【真机证据】BSL 020_terrain_translucent.fsh 第 3092 行：
                 *     float cloudBlendOpacity = step(viewLength, cloudViewLength);
                 *     if (cloudBlendOpacity == 0) discard;
                 *   `float == const int` 在 ES 下非法：
                 *     '==' : no operation '==' exists that takes a left-hand
                 *            operand of type 'float' and a right operand of type
                 *            'const int'
                 *
                 * 【为什么此前完全没有覆盖】情形 C 的运算符集合刻意不含
                 *   `=` 与 `!`（为避开复合赋值与 ==/!= 的歧义），
                 *   于是 `==`、`!=` 这两个最常见的比较从第一轮遗留至今。
                 */
                if (!convert && float_lhs_eq_int(p, s, g_src_begin, g_src_end)) {
                    convert = 1;
                }
                /*
                 * ---- 规则 K：`floatVar = <int literal>;` 普通赋值 ----
                 *
                 * 【真机证据 —— BSL shadow.glsl:238】
                 *     varying float mat;
                 *     ...
                 *     mat = 0;                                  // <- 报错行
                 *     if (blockID == 201 || blockID == 202) mat = 1;
                 *   驱动报（整包共 9 条、跨 3 个文件）：
                 *     'assign' : cannot convert from 'const int' to 'float'
                 *   桌面 GLSL 允许 `float = int` 隐式转换，ES 不允许。
                 *
                 * 【为什么既有的规则 D 抓不到】
                 *   规则 D 只处理**声明时的初始化器**：
                 *       float f = 5;      -> 5.0
                 *   它要求「字面量是整个右值」且「左侧含 float 类型名」。
                 *   而这里是**先声明、后赋值**，赋值号左边只有一个变量名，
                 *   没有任何类型关键字，因此 D 的判据不成立。
                 *
                 * 【判据】
                 *   1. 字面量右侧（跳空白）就是语句结束（本 token 是整个右值）；
                 *   2. 赋值号是**普通** `=`（排除 == != <= >= += 等）；
                 *   3. 赋值号左侧的标识符**确实被声明为浮点**
                 *      （正向确认，避免把 `intVar = 0;` 改坏）。
                 */
                if (!convert && is_assign_int_to_float(p, s, e)) convert = 1;
                sbuf_put(out, p, n);
                if (convert) sbuf_puts(out, ".0");
                p += n;
                continue;
            }
        }

        sbuf_put(out, p, 1);
        p++;
    }
}

/*
 * 判断 [p, e) 这一段是否「以预处理器指令开头」。
 *
 * 【为什么必须按行判断，而不是看当前字符是不是 '#'】
 *   真机上出现过这样的产物：
 *       #line 0 1   ->   #line 0 1.0      （非法指令，编译失败）
 *   原因是原实现只在「扫描指针恰好停在 '#' 上」时才保护预处理器行。
 *   而源码中 `#line` 前面往往还有换行（例如 #version 被移除后留下的空行），
 *   此时指针停在 '\n' 上，`*p == '#'` 不成立，保护被跳过，
 *   于是 `#line 0 1` 被当成普通语句拿去改写字面量。
 *
 *   正确做法：逐行观察，只要某行去掉前导空白后以 '#' 开头，
 *   整行就按原样复制，绝不进入字面量改写逻辑。
 */
static int line_starts_with_directive(const char *p, const char *e) {
    const char *q = p;
    while (q < e && (*q == ' ' || *q == '\t')) q++;
    return (q < e) && *q == '#';
}

/* ------------------------------------------------------------------ */
/* 整数向量与浮点的混合运算修复                                        */
/* ------------------------------------------------------------------ */

/*
 * 【真实故障】
 *   驱动报：
 *       ERROR: 1:14: '/' : wrong operand types  no operation '/' exists that
 *       takes a left-hand operand of type 'in 2-component vector of int'
 *       and a right operand of type 'const float'
 *
 *   源头是 Minecraft 自己的 include（light.glsl）：
 *       vec4 minecraft_sample_lightmap(sampler2D lightMap, ivec2 uv) {
 *           return texture(lightMap, clamp(uv / 256.0, ...));
 *       }
 *   `uv` 是 ivec2，`uv / 256.0` 是「整数向量 ÷ 浮点」。
 *   桌面 GLSL 允许这种隐式转换，**GLSL ES 不允许** —— 必须显式写成 vec2(uv)。
 *
 *   后果不是局部错误：MC 的核心着色器（rendertype_solid 等）都通过
 *   #moj_import 引入 light.glsl，因此它们会**全部编译失败**，
 *   reloadShaders 抛异常，渲染管线失效。表现为：逻辑照常运行
 *   （主菜单背景音乐正常播放、按钮可点），但**画面停在最后一帧不动**。
 *
 * 【为什么不能靠内嵌 include 解决】
 *   本库确实内嵌了一份修正过的 light.glsl，但实测发现 MC 会自己展开
 *   #moj_import —— 我们收到的源码里已经没有该指令，
 *   内嵌内容永远不会被使用。因此必须在【表达式层面】修复。
 *
 * 【修复策略】
 *   1. 扫描源码，收集被声明为 ivec2/3/4 或 uvec2/3/4 的变量名及其维度
 *   2. 若这些变量作为 `/` 或 `*` 的左操作数，且右操作数是【浮点字面量】，
 *      则把该变量改写为 vecN(变量)
 *
 *   只处理「右操作数是浮点字面量」这一确定无歧义的情形，
 *   因此不会误伤合法的整数运算（例如 `ivec2 / 2` 应保持整数除法）。
 */
/*
 * 整数变量表容量。
 *
 * 真机 BSL：deferred1.fsh 预处理后 693 行，涉及的 ivec/int 变量远超 64 个。
 * 容量不足会**静默**丢失登记，症状是「一部分表达式改了、一部分没改」——
 * 最难排查的一类故障。因此这里取足够大的值，并由调用方检测饱和。
 */
#define IVEC_VAR_MAX 256

typedef struct {
    char name[64];
    int dim;        /* 2 / 3 / 4 */
} ivec_var;

/* 跳过注释与字符串，返回新的位置。用于两个扫描阶段。 */
static const char *skip_comment_or_string(const char *p, const char *end) {
    if (p + 1 < end && p[0] == '/' && p[1] == '/') {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        return nl ? nl + 1 : end;
    }
    if (p + 1 < end && p[0] == '/' && p[1] == '*') {
        for (const char *q = p + 2; q + 1 < end; q++) {
            if (q[0] == '*' && q[1] == '/') return q + 2;
        }
        return end;
    }
    if (*p == '"') {
        const char *q = p + 1;
        while (q < end && *q != '"') {
            if (*q == '\\' && q + 1 < end) q++;
            q++;
        }
        return (q < end) ? q + 1 : end;
    }
    return NULL;    /* 不是注释/字符串 */
}

/* 整数向量类型关键字 -> 维度 */
static int ivec_dim_of(const char *s, size_t len) {
    static const struct { const char *kw; int dim; } KWS[] = {
        { "ivec2", 2 }, { "ivec3", 3 }, { "ivec4", 4 },
        { "uvec2", 2 }, { "uvec3", 3 }, { "uvec4", 4 },
    };
    for (size_t i = 0; i < sizeof(KWS) / sizeof(KWS[0]); i++) {
        if (strlen(KWS[i].kw) == len && strncmp(s, KWS[i].kw, len) == 0) {
            return KWS[i].dim;
        }
    }
    return 0;
}

/*
 * 尝试把 `name.y` / `name.xy` / `name.length()` 解析为「整数域表达式」。
 *
 * 【为什么必须处理 —— 真机 BSL 上占错误的四成】
 *   errors 里反复出现：
 *       '/' : no operation '/' exists that takes a left-hand operand of type
 *             'int' and a right operand of type 'const float'
 *   源头是
 *       uniform ivec2 eyeBrightnessSmooth;
 *       float eBS = eyeBrightnessSmooth.y / 240.0;
 *   `.y` 是 **int**（向量分量的类型就是其分量类型），
 *   而 `int / float` 在 GLSL ES 非法、在桌面合法。
 *   旧实现只知道「声明为 int 的变量」，swizzle 出来的 int 不在表里，
 *   因此这一整类都漏掉了。
 *
 *   同一个函数也顺带覆盖 `someInt.length()`（返回 int）。
 *
 * 返回解析出的分量个数（0 = 不适用）：1 表示标量分量，N 表示 N 分量。
 * *expr_end 被推进到 swizzle / 调用之后。
 */
static int int_component_expr(const char *p, const char *end,
                              const ivec_var *vars, int var_count,
                              const char **expr_end) {
    const char *q = p;
    while (q < end && is_ident_char(*q)) q++;
    size_t len = (size_t)(q - p);
    if (len == 0) return 0;

    int dim = 0;
    for (int i = 0; i < var_count; i++) {
        if (strlen(vars[i].name) == len &&
            strncmp(vars[i].name, p, len) == 0) {
            dim = vars[i].dim;
            break;
        }
    }
    /* 只有向量才有分量可取；标量走普通标识符路径 */
    if (dim < 2) return 0;

    const char *r = q;
    while (r < end && (*r == ' ' || *r == '\t' ||
                       *r == '\n' || *r == '\r')) r++;

    if (r >= end || *r != '.') return 0;

    const char *t = r + 1;
    if (t >= end || !is_ident_char(*t)) return 0;

    /* .length() 返回 int（标量） */
    if ((size_t)(end - t) >= 6 && strncmp(t, "length", 6) == 0) {
        const char *u = t + 6;
        while (u < end && (*u == ' ' || *u == '\t')) u++;
        if (u < end && *u == '(') {
            int d = 0;
            while (u < end) {
                if (*u == '(') d++;
                else if (*u == ')') { d--; if (d == 0) { u++; break; } }
                u++;
            }
            *expr_end = u;
            return 1;
        }
        return 0;
    }

    /* swizzle：分量个数就是结果维度 */
    const char *u = t;
    int n_comp = 0;
    while (u < end && is_ident_char(*u)) {
        char c = *u;
        if (c != 'x' && c != 'y' && c != 'z' && c != 'w' &&
            c != 'r' && c != 'g' && c != 'b' && c != 'a') {
            return 0;
        }
        n_comp++;
        u++;
    }
    if (n_comp == 0 || n_comp > 4) return 0;
    *expr_end = u;
    return n_comp;
}

/*
 * 判断某个标识符在源码**任何位置**是否被声明为浮点类型
 * （float / vec2 / vec3 / vec4 / matN）。
 *
 * 【用途】给 collect_ivec_vars 做冲突检测。
 *   同一个名字在不同作用域可能是不同类型：
 *       float minOf(vec3  x) { ... }     // x 是 vec3
 *       int   minOf(ivec3 x) { ... }     // x 是 ivec3
 *   只要该名字出现过浮点声明，就保守地不把它当整数，
 *   避免把浮点用法误包成 `vec3(x)`（那种误改会级联出几十个错误）。
 *
 * 检测方式：找到等于 name 的标识符后，看它**左侧**最近的类型关键字。
 * 只回退有限距离并遇 `; { } )` 停止，避免跨语句误判。
 */
static int ident_declared_as_float_anywhere(const char *src, const char *end,
                                           const char *name, size_t nlen) {
    static const char *const kFloatTypes[] = {
        "float", "vec2", "vec3", "vec4",
        "mat2", "mat3", "mat4",
        "mat2x2", "mat2x3", "mat2x4", "mat3x2", "mat3x3", "mat3x4",
        "mat4x2", "mat4x3", "mat4x4",
        NULL
    };

    for (const char *p = src; p + nlen <= end; p++) {
        if (strncmp(p, name, nlen) != 0) continue;
        if (p > src && is_ident_char(p[-1])) continue;
        if (p + nlen < end && is_ident_char(p[nlen])) continue;

        /* 右侧紧跟 '(' 说明是函数调用，不是声明 */
        const char *r = p + nlen;
        while (r < end && (*r == ' ' || *r == '\t')) r++;
        if (r < end && *r == '(') continue;

        /* 向左回退找类型关键字 */
        const char *u = p;
        int guard = 0;
        while (u > src && guard++ < 64) {
            if (is_ident_char(u[-1])) {
                const char *we = u;
                while (u > src && is_ident_char(u[-1])) u--;
                size_t tl = (size_t)(we - u);
                for (int i = 0; i < (int)LIST_LEN(kFloatTypes) && kFloatTypes[i] != NULL; i++) {
                    if (strlen(kFloatTypes[i]) == tl &&
                        strncmp(kFloatTypes[i], u, tl) == 0) {
                        return 1;
                    }
                }
                continue;   /* 是限定符或别的标识符，继续往前 */
            }
            if (u[-1] == ';' || u[-1] == '{' || u[-1] == '}' ||
                u[-1] == ')' || u[-1] == '(' || u[-1] == '=' ||
                u[-1] == ',' || u[-1] == '+' || u[-1] == '-' ||
                u[-1] == '*' || u[-1] == '/') {
                break;      /* 跨到表达式/语句边界，停止 */
            }
            u--;
        }
    }
    return 0;
}

/*
 * 与 ident_declared_as_float_anywhere 同构，但**只统计真正的全局作用域**
 * （花括号深度 0 且圆括号深度 0）的浮点声明。
 *
 * ============ 为什么需要它 —— Veil 兼容暴露的真实缺陷 ============
 *
 * 【真机/离线证据：Veil 的 pinwheel 着色器】
 *   program/light/point.fsh 的 include 链里，**同一个名字 uv 在不同函数
 *   的形参列表里换类型**：
 *       veil:space_helper : vec4 screenToWorldSpace(vec2 uv, float depth) {...}
 *       veil:light        : vec2 minecraft_sample_lightmap_coords(ivec2 uv) {
 *                               return clamp(uv / 256.0, vec2(...), vec2(...));
 *                           }
 *   GLSL 把形参限制在各自函数作用域内，所以这是完全合法的桌面写法。
 *
 *   但 collect_ivec_vars 的「约束 2」用的是**名字级、全文件**的排除：
 *   只要 `uv` 在文件任何位置出现过浮点声明，就把 `uv` 从整数表里剔除。
 *   于是 `ivec2 uv` 被 `vec2 uv` 一票否决，`uv / 256.0` 永远不被包裹，
 *   GLSL ES 报：
 *       '/' : no operation '/' exists that takes a left-hand operand of
 *             type 'in highp 2-component vector of int' and a right operand
 *             of type 'const float'
 *   **桌面 GLSL 330 不报这个错**，所以它属于「桌面能过、ES 不能过」
 *   的隐式转换差异 —— 和 `pow(f, 3)` / `vec2 * 2` 同类，是本转换器的职责。
 *
 * 【为什么「收紧到全局」正好是正确且最小的改动】
 *   collect_ivec_vars / collect_int_scalar_vars 本身**只在花括号深度 0
 *   收集**（约束 1）。既然表的输入只来自全局作用域，那么用于否决它的
 *   浮点声明也就只应看全局作用域 —— 两边的判别范围必须一致。
 *   函数形参写在 '{' 之前，所以必须在花括号深度之外**再排除圆括号内**，
 *   否则 `f(vec2 uv, ...)` 的参数仍会被当成「全局浮点声明」。
 *
 * 【约束 1 的原始回归（minOf(vec3 x) / minOf(ivec3 x)）怎么办】
 *   那两个 `x` 都是**函数形参**（圆括号内），故本函数不再互相否决。
 *   此时 `x` 会进入整数向量表，但其使用点的误改由既有的两道守卫拦下：
 *     - Rule O：matched_dim==1 且其后紧跟 '.' -> 归零
 *     - Rule Q：matched_dim>0 且其前紧邻 '.' -> 归零
 *   且 情形 A/E 都要求「另一侧确实处于浮点域」才改写。
 *   结论必须由回归门禁（MC 125/125、BSL 166/166、Flywheel、Sodium 语料）
 *   来确认，不能靠推理断言 —— 这也是本次改动必须跑全套门禁的原因。
 */
static int ident_declared_as_float_at_global_scope(const char *src,
                                                  const char *end,
                                                  const char *name,
                                                  size_t nlen) {
    static const char *const kFloatTypes[] = {
        "float", "vec2", "vec3", "vec4",
        "mat2", "mat3", "mat4",
        "mat2x2", "mat2x3", "mat2x4", "mat3x2", "mat3x3", "mat3x4",
        "mat4x2", "mat4x3", "mat4x4",
        NULL
    };

    int depth = 0;      /* 花括号深度：>0 表示在函数体内 */
    int paren = 0;      /* 圆括号深度：>0 表示在实参/形参列表内 */
    const char *p = src;

    while (p + nlen <= end) {
        /*
         * 必须跳过注释与字符串：否则注释里的 '{' / '}' 会让深度计数失真，
         * 进而把函数体内的声明误判为全局（或反之）。
         */
        const char *skipped = skip_comment_or_string(p, end);
        if (skipped != NULL) { p = skipped; continue; }

        if (*p == '{') { depth++; p++; continue; }
        if (*p == '}') { if (depth > 0) depth--; p++; continue; }
        if (*p == '(') { paren++; p++; continue; }
        if (*p == ')') { if (paren > 0) paren--; p++; continue; }

        if (*p != name[0]) { p++; continue; }
        if (strncmp(p, name, nlen) != 0) { p++; continue; }
        if (p > src && is_ident_char(p[-1])) { p++; continue; }
        if (p + nlen < end && is_ident_char(p[nlen])) { p++; continue; }

        /* 非全局作用域（函数形参 / 局部变量）的声明不参与否决 */
        if (depth != 0 || paren != 0) { p += nlen; continue; }

        /* 右侧紧跟 '(' 说明是函数调用，不是声明 */
        const char *r = p + nlen;
        while (r < end && (*r == ' ' || *r == '\t')) r++;
        if (r < end && *r == '(') { p += nlen; continue; }

        /* 向左回退找类型关键字（与 anywhere 版同构） */
        const char *u = p;
        int guard = 0;
        while (u > src && guard++ < 64) {
            if (is_ident_char(u[-1])) {
                const char *we = u;
                while (u > src && is_ident_char(u[-1])) u--;
                size_t tl = (size_t)(we - u);
                for (int i = 0;
                     i < (int)LIST_LEN(kFloatTypes) && kFloatTypes[i] != NULL;
                     i++) {
                    if (strlen(kFloatTypes[i]) == tl &&
                        strncmp(kFloatTypes[i], u, tl) == 0) {
                        return 1;
                    }
                }
                continue;   /* 是限定符或别的标识符，继续往前 */
            }
            if (u[-1] == ';' || u[-1] == '{' || u[-1] == '}' ||
                u[-1] == ')' || u[-1] == '(' || u[-1] == '=' ||
                u[-1] == ',' || u[-1] == '+' || u[-1] == '-' ||
                u[-1] == '*' || u[-1] == '/') {
                break;      /* 跨到表达式/语句边界，停止 */
            }
            u--;
        }
        p += nlen;
    }
    return 0;
}

/*
 * 与 ident_declared_as_float_anywhere 同构，但检测「其它整数类型」。
 *
 * 【为什么类型表里**不含** `uint`】
 *   本函数用于为「标量 uint 名字表」(collect_uint_vars) 做同名歧义排除。
 *   若把 `uint` 也列进来，那么每个被检查的名字都会因为
 *   「它自己被声明为 uint」而遭到否决 —— 整张表恒为空，规则 N 永不生效。
 *   这正是实测中发现 `base`、`index` 同时被声明为 uint 与 int 的原因：
 *   它们如实反映了「同名跨作用域换类型」，必须排除。
 */
static int ident_declared_as_other_int_anywhere(const char *src, const char *end,
                                               const char *name, size_t nlen) {
    static const char *const kOtherIntTypes[] = {
        "int",
        "ivec2", "ivec3", "ivec4",
        "uvec2", "uvec3", "uvec4",
        "bvec2", "bvec3", "bvec4",
        NULL
    };

    for (const char *p = src; p + nlen <= end; p++) {
        if (strncmp(p, name, nlen) != 0) continue;
        if (p > src && is_ident_char(p[-1])) continue;
        if (p + nlen < end && is_ident_char(p[nlen])) continue;

        /* 右侧紧跟 '(' 说明是函数调用，不是声明 */
        const char *r = p + nlen;
        while (r < end && (*r == ' ' || *r == '\t')) r++;
        if (r < end && *r == '(') continue;

        /* 向左回退找类型关键字 */
        const char *u = p;
        int guard = 0;
        while (u > src && guard++ < 64) {
            if (is_ident_char(u[-1])) {
                const char *we = u;
                while (u > src && is_ident_char(u[-1])) u--;
                size_t tl = (size_t)(we - u);
                for (int i = 0; i < (int)LIST_LEN(kOtherIntTypes) && kOtherIntTypes[i] != NULL; i++) {
                    if (strlen(kOtherIntTypes[i]) == tl &&
                        strncmp(kOtherIntTypes[i], u, tl) == 0) {
                        return 1;
                    }
                }
                continue;   /* 是限定符或别的标识符，继续往前 */
            }
            if (u[-1] == ';' || u[-1] == '{' || u[-1] == '}' ||
                u[-1] == ')' || u[-1] == '(' || u[-1] == '=' ||
                u[-1] == ',' || u[-1] == '+' || u[-1] == '-' ||
                u[-1] == '*' || u[-1] == '/') {
                break;      /* 跨到表达式/语句边界，停止 */
            }
            u--;
        }
    }
    return 0;
}

/*
 * 收集被声明为整数向量的变量名。
 *
 * 匹配形如 `ivec2 uv` / `uniform ivec3 x` / `in ivec2 UV2` 的声明：
 * 关键字后面紧跟一个标识符即为变量名。
 *
 * ================= 必须加的两条约束（真机踩坑记录） =================
 *
 * 【约束 1：只收全局作用域】
 *   本函数原先全文扫描，把**函数形参与局部变量**也收了进来。
 *   ComplementaryReimagined 的 deferred1.fsh 里有这样一对重载：
 *       float minOf(vec3  x) { return min(x.x, min(x.y, x.z)); }
 *       int   minOf(ivec3 x) { return min(x.x, min(x.y, x.z)); }
 *   **两个重载的形参都叫 x**。全文扫描读到 `ivec3 x` 就把 x 记为
 *   「整数向量」，于是另一个重载（形参是 vec3）里的 x 也被当成整数，
 *   被改写器包成了：
 *       return min(vec3(x).x, min(vec3(x).y, vec3(x).z));   // 语义全错
 *   一次误改产生几十个连锁错误（return 类型不匹配、min 无重载、
 *   构造实参不足…），输出从 1317 行膨胀到 3478 行，**等于毁掉整个着色器**。
 *
 *   修法：记录花括号深度，只在深度 0（全局）时收集。
 *   函数形参与局部变量因此天然被排除。
 *
 * 【约束 2：同名标识符若在别处被声明为浮点，则不算整数】
 *   光有约束 1 还不够：Iris 注入的文本里可能在不同作用域重复使用同一名字。
 *   只要该名字在**任何**位置出现过浮点声明，就保守地不把它当整数 ——
 *   宁可漏改也不能误改，因为误改会级联放大（见上）。
 */
static int collect_ivec_vars(const char *src, ivec_var *out, int max) {
    const char *end = src + strlen(src);
    const char *p = src;
    int n = 0;
    int depth = 0;      /* 花括号深度：只在 0（全局作用域）收集 */
    int paren = 0;    /* 圆括号深度：函数形参写在 '{' 之前，必须靠它排除 */

    while (p < end && n < max) {
        const char *skipped = skip_comment_or_string(p, end);
        if (skipped != NULL) { p = skipped; continue; }

        if (*p == '(') { paren++; p++; continue; }
        if (*p == ')') { if (paren > 0) paren--; p++; continue; }
        if (*p == '{') { depth++; p++; continue; }
        if (*p == '}') { if (depth > 0) depth--; p++; continue; }

        if (!is_ident_char(*p)) { p++; continue; }

        const char *q = p;
        while (q < end && is_ident_char(*q)) q++;
        size_t len = (size_t)(q - p);

        int dim = ivec_dim_of(p, len);
        if (dim > 0) {
            /* 跳过空白，取下一个标识符作为变量名 */
            const char *r = q;
            while (r < end && (*r == ' ' || *r == '\t' ||
                               *r == '\n' || *r == '\r')) {
                r++;
            }
            if (r < end && is_ident_char(*r) && !isdigit((unsigned char)*r)) {
                const char *s2 = r;
                while (s2 < end && is_ident_char(*s2)) s2++;
                size_t nl = (size_t)(s2 - r);
                /*
                 * 约束 2：同名标识符若在**全局作用域**被声明为浮点，
                 * 就不把它当整数。
                 *
                 * 【为什么不再是「任何位置」—— Veil 兼容暴露的真实缺陷】
                 *   Veil 的 include 链里，同一名字 uv 在不同函数的形参列表
                 *   里换类型：
                 *       veil:space_helper : vec4 screenToWorldSpace(vec2 uv, ...)
                 *       veil:light        : vec2 minecraft_sample_lightmap_coords(ivec2 uv)
                 *                           { return clamp(uv / 256.0, ...); }
                 *   GLSL 把形参限制在各自函数作用域内，这是合法写法。
                 *   但「任何位置」的排除让 `vec2 uv` 一票否决了 `ivec2 uv`，
                 *   于是 `uv / 256.0` 永不包裹，ES 报
                 *       '/' : no operation '/' exists ... 'vector of int' ... 'const float'
                 *   而**桌面 330 不报** —— 属于桌面/ES 隐式转换差异，
                 *   与 `pow(f, 3)`、`vec2 * 2` 同类，是本转换器的职责。
                 *
                 *   本函数本来就**只在花括号深度 0 收集**（约束 1），
                 *   因此用于否决的浮点声明也只应看全局作用域 —— 两边范围必须一致。
                 *   形参写在 '{' 之前，故新函数还会排除圆括号内的声明。
                 *
                 *   约束 1 的原始回归（minOf(vec3 x) / minOf(ivec3 x)）由
                 *   使用点的守卫（Rule O/Q、情形 A/E 均需「另一侧确为浮点」）
                 *   承担，必须由回归门禁确认，不能靠推理断言。
                 */
                if (nl < sizeof(out[0].name) &&
                    !ident_declared_as_float_at_global_scope(src, end, r, nl)) {
                    memcpy(out[n].name, r, nl);
                    out[n].name[nl] = '\0';
                    out[n].dim = dim;
                    n++;
                    p = s2;
                    continue;
                }
            }
        }
        p = q;
    }
    return n;
}

/* 判断位置 p 起的文本是否为浮点字面量（形如 256.0 / .5 / 1e3 / 0.25f） */
static int starts_with_float_literal(const char *p, const char *end) {
    if (p >= end) return 0;
    if (float_literal_len(p) > 0) {
        /*
         * float_literal_len 现在会吃 f/F 后缀，所以这里直接看
         * 它扫过的区间里是否含小数点、指数或后缀即可。
         */
        const char *q = p;
        int has_dot = 0, has_exp = 0, has_suffix = 0;
        while (q < end && (isdigit((unsigned char)*q) || *q == '.' ||
                           *q == 'e' || *q == 'E' || *q == '+' || *q == '-' ||
                           *q == 'f' || *q == 'F')) {
            if (*q == '.') has_dot = 1;
            if (*q == 'e' || *q == 'E') has_exp = 1;
            if (*q == 'f' || *q == 'F') has_suffix = 1;
            q++;
        }
        return has_dot || has_exp || has_suffix;
    }
    return 0;
}

/*
 * 判断标识符是否是**控制流关键字**（不是函数名）。
 *
 * 【为什么必须有它】
 *   找「本 return 属于哪个函数」时要反向看 `{` 前面的 `)`，
 *   而 `if (x) {` / `for (...) {` / `while (...) {` 形状完全一样。
 *   若不排除它们，嵌套在 `if` 里的 `return 0;` 会被认为属于
 *   「名为 if、返回类型未知」的东西，于是永远匹配不到所属函数。
 *   （这是实测踩到的：Flywheel 的 `return 0;` 恰好就在 `if` 块里。）
 */
static int is_control_keyword(const char *s, size_t n) {
    static const char *const kCtrl[] = {
        "if", "for", "while", "switch", "else", "do", NULL
    };
    for (int i = 0; kCtrl[i] != NULL; i++) {
        if (strlen(kCtrl[i]) == n && strncmp(kCtrl[i], s, n) == 0) return 1;
    }
    return 0;
}

/* 该 token 是否是声明限定符（const / highp / in / ...），不是类型名 */
static int is_decl_qualifier_token(const char *s, size_t n) {
    static const char *const kQuals[] = {
        "uniform", "const", "in", "out", "inout", "attribute", "varying",
        "flat", "smooth", "noperspective", "centroid", "invariant", "precise",
        "lowp", "mediump", "highp", "patch", "sample", "readonly",
        "writeonly", "coherent", "volatile", "restrict", "shared", "buffer",
        NULL
    };
    for (int i = 0; kQuals[i] != NULL; i++) {
        if (strlen(kQuals[i]) == n && strncmp(kQuals[i], s, n) == 0) return 1;
    }
    return 0;
}

/* 该 token 是否是**浮点**类型名（float / vecN / matN） */
static int is_float_type_token(const char *s, size_t n) {
    if (n == 5 && strncmp(s, "float", 5) == 0) return 1;
    if (is_float_vec_type(s, n)) return 1;          /* vec2 / vec3 / vec4 */
    static const char *const kMats[] = {
        "mat2", "mat3", "mat4",
        "mat2x2", "mat2x3", "mat2x4", "mat3x2", "mat3x3", "mat3x4",
        "mat4x2", "mat4x3", "mat4x4", NULL
    };
    for (int i = 0; kMats[i] != NULL; i++) {
        if (strlen(kMats[i]) == n && strncmp(kMats[i], s, n) == 0) return 1;
    }
    return 0;
}

/*
 * 修复「浮点返回类型的函数体内 `return <整数字面量>;`」。
 *
 * ================== 为什么这是**我们的**职责 ==================
 *
 * 真机 Flywheel 的 flywheel:internal/wavelet.glsl：
 *     float total_absorbance(in sampler2DArray coefficients) {
 *         float scale_coefficient = get_coefficients(coefficients, COUNT - 1);
 *         if (scale_coefficient == 0) {
 *             return 0;            // <-- int 字面量，函数返回 float
 *         }
 *         ...
 *     }
 * Flywheel 在 `float` 返回类型的函数里写 `return 0;`。
 *
 * 那么这算"Flywheel 的 bug"、还是我们要适配的桌面专属写法？
 * **实测给了确定答案**（native/tools/probe_return_int_float.ps1）：
 *     #version 460 桌面 : `float f() { return 0; }`   -> ACCEPTED
 *     #version 320 es   : 同一段代码                  -> REJECTED
 *         'return' : type does not match, or is not convertible to
 *                    the function's return type
 * 桌面**接受**、ES **拒绝** —— 与 `float f = 0;`、`vec2 * 2`、`float < int`、
 * `pow(f, 3)` 完全同类。因此这是本转换器要适配的**语言差异**，
 * 说成"上游 bug"是不对的（同类我们已经处理过很多次）。
 *
 * 【实现方式：一趟前向扫描 + 大括号深度栈】
 *   维护 fn_float[depth]：当前这层大括号「所属函数是否返回浮点」。
 *   遇到 `{` 时判断它是否开启一个函数体（`{` 前是 `)`，而该 `)` 配对的 `(`
 *   之前的标识符不是 if/for/while/switch —— 见 is_control_keyword），
 *   并**向父层继承**：`fn_float[d] = 本层是浮点函数 || fn_float[d-1]`。
 *
 *   继承是必须的：Flywheel 的 `return 0;` 位于函数体内的 `if` 块里，
 *   若只看"本层大括号是否函数体"，会得到"不是"，从而漏改。
 *   GLSL 不允许嵌套函数定义，所以函数体内的任何 `{` 都是普通语句块，
 *   继承父层的函数返回类型是正确且唯一的解释。
 *
 * 【判据（保守：只处理整个返回值就是一个整数字面量的情形）】
 *   1. 字面量紧跟 `return`（跳过空白），其后（跳空白）是 `;`
 *   2. 该字面量**仍是纯整数**（无小数点、无 e/E、无 f/F 后缀）
 *      —— 若前面的规则已把它改成 `0.0`，此处自然跳过
 *   3. 所在函数返回浮点（float / vecN / matN）
 *   不分析分支、不猜测其它表达式形态。
 */
static int fix_return_int_in_float_fn(sbuf *out, const char *src) {
    const char *end = src + strlen(src);
    const char *p = src;
    int rewrites = 0;

    /* fn_float[d] = 第 d 层大括号「所属函数是否返回浮点」 */
    int fn_float[80];
    int depth = 0;

    while (p < end) {
        const char *sk = skip_comment_or_string(p, end);
        if (sk != NULL) {
            sbuf_put(out, p, (size_t)(sk - p));
            p = sk;
            continue;
        }

        /* ---- 进入一层大括号 ---- */
        if (*p == '{') {
            int this_is_float_fn = 0;
            /*
             * 【指针语义 —— 必须从 '{' 的**前一个**字符开始】
             *   本函数的 p 指向 '{'。若从 q = p 起步再"跳过空白"，
             *   因为 *q 正是 '{'（不是空白），循环一次都不执行，
             *   随后检查 q[-1] 拿到的是 '{' 前面那个空格 —— 于是
             *   `q[-1] == ')'` 永远为假，整条规则从不生效。
             *   （这正是本次实测的第一个缺陷：规则写完了却毫无作用。）
             *   正确做法：从 p - 1 起步。
             */
            const char *q = p;
            while (q > src && (q[-1] == ' ' || q[-1] == '\t' ||
                               q[-1] == '\n' || q[-1] == '\r')) q--;
            if (q > src && q[-1] == ')') {
                const char *u = q - 1;
                int dd = 0;
                while (u > src) {
                    if (u[-1] == ')') { dd++; u--; continue; }
                    if (u[-1] == '(') { if (dd == 0) break; dd--; u--; continue; }
                    u--;
                }
                if (u > src && u[-1] == '(') {
                    const char *fn = u - 1;
                    while (fn > src && (*fn == ' ' || *fn == '\t')) fn--;
                    const char *fe = fn;
                    while (fn > src && is_ident_char(fn[-1])) fn--;
                    size_t fl = (size_t)(fe - fn);
                    if (fl > 0 && !is_control_keyword(fn, fl)) {
                        /* 函数名之前是返回类型（可能带限定符） */
                        /*
                         * 【同样是指针语义】ty 初值指向**函数名首字符**，
                         * 要跳过的是它**前面**的空白，所以必须看 ty[-1]。
                         * 写成 `*ty == ' '` 时循环一次都不跑，tl 恒为 0，
                         * 于是永远判不出返回类型 —— 规则静默失效。
                         */
                        const char *ty = fn;
                        while (ty > src && (ty[-1] == ' ' || ty[-1] == '\t' ||
                                            ty[-1] == '\n' || ty[-1] == '\r')) {
                            ty--;
                        }
                        const char *te = ty;
                        while (ty > src && is_ident_char(ty[-1])) ty--;
                        size_t tl = (size_t)(te - ty);
                        if (tl > 0 && is_decl_qualifier_token(ty, tl)) {
                            const char *t2 = ty;
                            while (t2 > src && (t2[-1] == ' ' || t2[-1] == '\t' ||
                                                t2[-1] == '\n' || t2[-1] == '\r')) {
                                t2--;
                            }
                            const char *e2 = t2;
                            while (t2 > src && is_ident_char(t2[-1])) t2--;
                            ty = t2;
                            tl = (size_t)(e2 - t2);
                        }
                        if (tl > 0 && is_float_type_token(ty, tl)) {
                            this_is_float_fn = 1;
                        }
                    }
                }
            }
            int inherited = (depth > 0) ? fn_float[depth - 1] : 0;
            if (depth < 80) fn_float[depth] = this_is_float_fn || inherited;
            depth++;
            sbuf_put(out, p, 1);
            p++;
            continue;
        }

        /* ---- 退出一层大括号 ---- */
        if (*p == '}') {
            if (depth > 0) depth--;
            sbuf_put(out, p, 1);
            p++;
            continue;
        }

        /* ---- `return <整数字面量>;` ---- */
        if (strncmp(p, "return", 6) == 0 &&
            (p == src || !is_ident_char(p[-1])) &&
            !is_ident_char(p[6])) {
            const char *lit = p + 6;
            while (*lit == ' ' || *lit == '\t' ||
                   *lit == '\n' || *lit == '\r') lit++;
            if (isdigit((unsigned char)*lit)) {
                const char *le = lit;
                while (le < end && (isalnum((unsigned char)*le) || *le == '.')) le++;
                const char *r = le;
                while (*r == ' ' || *r == '\t' ||
                       *r == '\n' || *r == '\r') r++;
                int pure_int = (le > lit);
                for (const char *z = lit; z < le; z++) {
                    if (!isdigit((unsigned char)*z)) { pure_int = 0; break; }
                }
                if (pure_int && *r == ';' && depth > 0 && fn_float[depth - 1]) {
                    sbuf_put(out, p, (size_t)(le - p));
                    sbuf_puts(out, ".0");
                    rewrites++;
                    p = le;
                    continue;
                }
            }
        }

        sbuf_put(out, p, 1);
        p++;
    }
    return rewrites;
}

/*
 * ================= 规则 H：字面量位于浮点内建的实参列表内 =================
 *
 * 【真机证据 —— pow 类错误占某光影包全部错误的绝大多数】
 *
 * 用 glslang（ES 3.20）逐条实测，下列写法**全部被拒**，而桌面 GLSL 接受：
 *     pow(f, 3)     sin(3)        cos(3)       radians(3)
 *     step(0.5, 3)  smoothstep(0.0, 1.0, 3)   mix(0.0, 1.0, 3)
 *     clamp(u, 0.0, 1)   min(u, 3)   max(u, 3)
 * 报错一律是：
 *     'pow' : no matching overloaded function found
 *     'min' : no matching overloaded function found
 * 原因：`3` 是 `const int`，而 ES 不做 int->float 隐式转换，
 * 于是没有任何重载能匹配。
 *
 * 【为什么现有规则全部漏掉它】
 *   规则 A 只覆盖 vecN/matN 构造函数的实参；
 *   规则 B / B' 要求字面量紧邻算术或比较运算符；
 *   规则 D 要求字面量是整个右值；
 *   规则 E 只看「左侧是浮点内建调用的右括号」——
 *   而 `pow(f, 3)` 里的 `3` 左侧是**逗号**：整整一串规则没有一条命中。
 *   结果 `pow` 的第 2 个实参永远不转换（某光影包 18 个着色器、66 条错误
 *   全部源自这一条）。
 *
 * 【做法：从字面量向左做一次配对扫描】
 *   沿途跳过空白、逗号、运算符与成对括号；遇到未配对的 '(' 时，
 *   取它前面的函数名，若属于 FLOAT_BUILTIN_FNS 即判定命中。
 *   扫描中若发现**另一个整数字面量**则放弃 —— 那说明整个实参列表是整数域
 *   （如 a 为 int 时的 `max(a, 2)`），改写会制造新错误。
 *
 * 【★浮点证据必须是「真的浮点」★ —— 这里曾经误改过合法代码】
 *   起初把任意标识符都当作浮点证据，于是
 *       const int count = 6;
 *       index = clamp(index, 0, count);      // 完全合法的 ES（有 int 重载）
 *   被改成了
 *       index = clamp(float(index), 0.0, count);
 *   既把 `0` 误改成 `0.0`，又没处理 count，把一个本来正确的着色器改坏。
 *   现在证据只认三类：
 *       (a) 浮点字面量；
 *       (b) 带 '.' 的 swizzle（`.xy` 之类，基类型必为浮点）；
 *       (c) **不在 int_idents 表里**的标识符。
 *   (c) 是判断 `index` / `count` 这类整型变量的关键。
 */
/*
 * 判断「以 pe 为末尾（不含）的那个 token」是否为「已知整数变量」，
 * 允许带 `.x` 这类分量选择（如 `someIvec.x`）。
 *
 * 用于 in_float_builtin_arglist 判断内建函数走的是 int 重载还是 float 重载：
 * 第一个实参是整数变量时，整个实参列表都处于整数域。
 */
static int grid_base_is_int_ident(const char *pe, const char *s,
                                  const int_ident_table *int_idents) {
    if (int_idents == NULL || pe <= s) return 0;
    const char *e = pe;
    /*
     * 跳过尾部 swizzle：GLSL 分量选择最多 4 个字符，且其前面必须是 '.'。
     * （与 lhs_base_ident 同一判据，避免把 `posx` / `view` 误当 swizzle。）
     */
    {
        const char *t = e;
        int n = 0;
        while (t > s && n < 4 &&
               (t[-1] == 'x' || t[-1] == 'y' || t[-1] == 'z' || t[-1] == 'w' ||
                t[-1] == 'r' || t[-1] == 'g' || t[-1] == 'b' || t[-1] == 'a')) {
            t--;
            n++;
        }
        if (n > 0 && t > s && t[-1] == '.' &&
            t - 1 > s && is_ident_char(t[-2])) {
            e = t - 1;
        }
    }
    const char *b = e;
    while (b > s && is_ident_char(b[-1])) b--;
    if (b == e) return 0;
    return int_ident_find(int_idents, b, (size_t)(e - b));
}

static int in_float_builtin_arglist(const char *p, const char *s,
                                    const char *e,
                                    const int_ident_table *int_idents) {
    int depth = 0;
    int seen_float_evidence = 0;
    int first_token = 1;
    const char *q = p;

    /* 右侧若紧跟标识符字符（`3x`），说明是宏残留，保守放过 */
    {
        const char *r = p;
        while (r < e && isdigit((unsigned char)*r)) r++;
        if (r < e && is_ident_char(*r)) return 0;
    }

    while (q > s) {
        char c = q[-1];

        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { q--; continue; }

        if (c == ')') { depth++; q--; first_token = 0; continue; }

        if (c == '(') {
            if (depth > 0) { depth--; q--; first_token = 0; continue; }
            {
                const char *u = q - 1;
                while (u > s && (*u == ' ' || *u == '\t')) u--;
                const char *we = u;
                while (u > s && is_ident_char(u[-1])) u--;
                size_t ul = (size_t)(we - u);
                if (ul == 0) return 0;
                if (!name_in_list(u, ul, FLOAT_BUILTIN_FNS,
                                  LIST_LEN_TERM(FLOAT_BUILTIN_FNS))) {
                    return 0;
                }
                /*
                 * 【重载必须看**第一个**实参 —— 真机 Flywheel 的根因之一】
                 *
                 * clamp / min / max / abs / sign 这些内建函数在 GLSL ES 里
                 * **同时有 int 与 float 两套重载**，重载由第一个实参决定：
                 *     clamp(floatValue, 0.0, 1.0)   // 字面量必须是浮点
                 *     clamp(intValue,   0,   16)    // 字面量必须是整数
                 *
                 * 本函数原先只问「实参列表里有没有浮点证据」。Flywheel 的
                 *     int index = clamp(int(floor(coefficient_depth)), 0,
                 *                       TRANSPARENCY_WAVELET_COEFFICIENT_COUNT - 1);
                 * 第一个实参是 `int(...)`（整数！），但列表里还有
                 * `coefficient_depth`（float 形参）提供浮点证据，
                 * 于是 `0` -> `0.0`、`COUNT - 1` -> `COUNT - 1.0`，
                 * 产出 `clamp(int, 0.0, 15.0)` —— 既选不到 int 重载
                 * （实参类型不匹配），也选不到 float 重载（第一个实参是 int）。
                 * Adreno 报 'clamp' : no matching overloaded function found。
                 *
                 * 因此：先看第一个实参。
                 *   以 `int(` / `uint(` 开头 或 是一个已知整数变量
                 *   -> 走整数重载，本实参列表内的字面量一律不得浮点化。
                 */
                {
                    const char *f = q;      /* 指向第一个实参的起点 */
                    while (f < e && (*f == ' ' || *f == '\t' ||
                                     *f == '\n' || *f == '\r')) f++;
                    if (f < e) {
                        /* `int(...)` / `uint(...)` -> 整数域 */
                        const char *fe = f;
                        while (fe < e && is_ident_char(*fe)) fe++;
                        size_t fl = (size_t)(fe - f);
                        if ((fl == 3 && strncmp(f, "int", 3) == 0) ||
                            (fl == 4 && strncmp(f, "uint", 4) == 0) ||
                            (fl == 4 && strncmp(f, "ivec", 4) == 0) ||
                            (fl == 4 && strncmp(f, "uvec", 4) == 0)) {
                            return 0;
                        }
                        /* 已知整数变量（含其分量，如 `someIvec.x`） */
                        if (grid_base_is_int_ident(fe, f, int_idents)) return 0;
                    }
                }
                return seen_float_evidence;
            }
        }

        if (c == ',' || c == '+' || c == '-' || c == '*' || c == '/' ||
            c == '%' || c == '<' || c == '>') {
            q--; first_token = 0; continue;
        }

        if (c == '.') { seen_float_evidence = 1; q--; first_token = 0; continue; }

        if (isdigit((unsigned char)c) || is_ident_char(c)) {
            const char *t = q;
            int had_dot = 0;
            while (t > s && (is_ident_char(t[-1]) || t[-1] == '.')) {
                if (t[-1] == '.') had_dot = 1;
                t--;
            }
            size_t tl = (size_t)(q - t);

            /* 若左侧第一个 token 就是标识符/数字，说明本字面量不是独立
             * 实参（`a[3]`、`x2`、`foo3`），保守放弃 */
            if (first_token) return 0;
            if (tl == 0) return 0;

            if (float_literal_len(t) == tl) {
                seen_float_evidence = 1;
                q = t;
                continue;
            }

            /*
             * 纯数字 token（整数域）：**跳过，但不计作浮点证据**。
             *
             * 【为什么不能直接 return 0 —— 真实漏改】
             *     clamp(vertexPos.z, min(cos(Angle), 0), 1)
             *   处理最外层最后一个 `1` 时，向左会先遇到内层的整数 `0`。
             *   那个 `0` 同样位于浮点实参列表内，稍后也会被改写为 `0.0`，
             *   但本次扫描看到的是**改写前的原始文本**，于是旧写法在此
             *   直接放弃，`1` 永远改不掉（真机 shader 33 即此形）。
             *   正确做法是继续向左扫：浮点证据由 `Angle`、`cos` 提供，
             *   最终在 `clamp(` 处命中。
             *   若整个实参列表确实都是整数（如 `max(intA, 2)`），
             *   扫到左括号时证据仍为 0，照样安全返回。
             */
            {
                int all_digit = 1;
                for (const char *z = t; z < q; z++) {
                    if (!isdigit((unsigned char)*z)) { all_digit = 0; break; }
                }
                if (all_digit) {
                    q = t;
                    first_token = 0;
                    continue;
                }
            }

            /*
             * 标识符。带 '.' 的 swizzle 必为浮点；否则查整数变量表 ——
             * 是已知整型变量的**不能**作为浮点证据（否则会把
             * `clamp(index, 0, count)` 这种合法的 int 重载调用改坏）。
             */
            if (had_dot) {
                seen_float_evidence = 1;
            } else if (int_idents == NULL ||
                       !int_ident_find(int_idents, t, tl)) {
                seen_float_evidence = 1;
            }
            q = t;
            first_token = 0;
            continue;
        }

        return 0;   /* 其它字符无法判定 */
    }
    return 0;
}

/*
 * 判断某个标识符是否为浮点向量（vec2/3/4）变量。
 *
 * 【用途】识别赋值语句的左值类型。
 *   形如 `texCoord2 = UV2;`（out vec2 = in ivec2）需要改写为
 *   `texCoord2 = vec2(UV2);`，但前提是左值确实是浮点向量 ——
 *   若左值是 ivec2（`ivec2 iv = UV2;`），加 vec2() 反而会出错。
 *   因此必须知道左值的类型，不能只看赋值号。
 */
static int is_float_vec_type(const char *s, size_t len) {
    static const char *const KWS[] = { "vec2", "vec3", "vec4" };
    for (size_t i = 0; i < sizeof(KWS) / sizeof(KWS[0]); i++) {
        if (strlen(KWS[i]) == len && strncmp(s, KWS[i], len) == 0) return 1;
    }
    return 0;
}

/*
 * 类型族判定：1 = 整数族（int/uint/ivecN/uvecN），2 = 浮点族（float/vecN/matN），
 * 0 = 都不是（限定符、标识符、自定义类型名…）。
 *
 * 供「使用点按形参类型判定」使用，见 enclosing_fn_param_class。
 */
static int type_family_of_token(const char *s, size_t n) {
    if (n == 3 && strncmp(s, "int", 3) == 0)   return 1;
    if (n == 4 && strncmp(s, "uint", 4) == 0)  return 1;
    if (ivec_dim_of(s, n) > 0)                 return 1;   /* ivecN / uvecN */
    if (n == 5 && strncmp(s, "float", 5) == 0) return 2;
    if (is_float_vec_type(s, n))               return 2;   /* vec2/3/4 */
    static const char *const kMats[] = {
        "mat2", "mat3", "mat4",
        "mat2x2", "mat2x3", "mat2x4", "mat3x2", "mat3x3", "mat3x4",
        "mat4x2", "mat4x3", "mat4x4", NULL
    };
    for (int i = 0; kMats[i] != NULL; i++) {
        if (strlen(kMats[i]) == n && strncmp(kMats[i], s, n) == 0) return 2;
    }
    return 0;
}

/*
 * 按【使用点所在函数】的形参列表判定 name 的类型族。
 *
 * 返回：1 = 该名字是本函数形参且为整数族
 *       2 = 该名字是本函数形参且为浮点族
 *       0 = 不是本函数的形参，或无法判定（调用方应退回既有启发式）
 *
 * ============ 为什么必须有它 —— Veil 兼容暴露的真实缺陷 ============
 *
 * 【事实】`collect_float_vec_vars` / `collect_ivec_vars` **并没有**做到
 * 注释里写的「只在全局作用域收集」：函数**返回类型**（`vec2 a(`，此时
 * 尚在 `{` 之前的深度 0）与**形参**（写在 `(` 之内）都会被按名字收录。
 *   实测（-DGLESMOD_DIAG 打印）：
 *       vec2 a(vec2 uv){...}  vec2 b(ivec2 uv){...}
 *     -> fvec = { fragColor, a, b, c, d, e, uv }
 *     -> ivec = { uv, coord, ... }
 *   即 `uv` 同时在两张表里，而两张表都是「名字级」的。
 *
 * 【后果】Veil 的 `ivec2 uv` 形参跟着 `vec2 uv` 一起被判成浮点，
 * 于是 情形 C 把 `uv + ivec2(1)` 里右侧的 `ivec2(1)` 当成
 * 「整数表达式赋给浮点」，包成 `vec2(ivec2(1))`，输出
 *     `uv + vec2(ivec2(1))`   -> ivec2 + vec2，ES 报 '+': wrong operand types
 *   GLSL 把形参限制在各自函数作用域内，所以**只有**「按使用点所在函数
 *   的形参类型判定」才是对的；按名字全文件判定必然出错。
 *
 * 【做法】正向扫描到 p，用块栈记录 p 之前最后一个未闭合的 '{'；
 * 该 '{' 之前若是 ')'，就配对出形参列表，在其中查找 name 的
 * 「类型关键字 + 标识符」对。任何一步不成立即返回 0（未知），
 * 调用方维持原有行为 —— 宁可漏改，不可误改。
 */
static int enclosing_fn_param_class(const char *src, const char *p,
                                    const char *name, size_t nlen) {
    if (p <= src || nlen == 0) return 0;

    /* 1. 块栈：找出 p 之前最后一个未闭合的 '{' */
    const char *stack[64];
    int sp = 0;
    for (const char *z = src; z < p; ) {
        const char *sk = skip_comment_or_string(z, p);
        if (sk != NULL) { z = sk; continue; }
        if (*z == '{') {
            if (sp < 64) stack[sp] = z;
            sp++;
            z++;
            continue;
        }
        if (*z == '}') { if (sp > 0) sp--; z++; continue; }
        z++;
    }
    if (sp == 0) return 0;                 /* 不在任何块内（全局） */
    const char *open = stack[sp - 1];

    /* 2. open 之前应是形参列表的 ')'；否则这不是函数体（if/for 块） */
    const char *r = open;
    while (r > src && (r[-1] == ' ' || r[-1] == '\t' ||
                       r[-1] == '\n' || r[-1] == '\r')) r--;
    if (r <= src || r[-1] != ')') return 0;

    /* 3. 配对到 '(' —— 注意用「右侧是 ')'」的写法与既有惯例一致 */
    const char *lparen = NULL;
    {
        const char *t = r;                 /* r[-1] 是形参列表的 ')' */
        int d2 = 0;
        while (t > src) {
            if (t[-1] == ')') { d2++; t--; continue; }
            if (t[-1] == '(') {
                /*
                 * 【顺序至关重要 —— 实测踩到】必须先 `d2--` 再判零。
                 * 先前写成「先判 d2==0 再 d2--」，于是形参列表自身的
                 * 那个 ')' 被当成嵌套括号（d2 从 0 变 1），一路向左找不到
                 * 匹配的 '('，函数**永远**返回 0（= 无法判定）。
                 * 症状极具迷惑性：补的对策「看起来已实现」，却毫无效果
                 * —— 因为判定根本没生效，代码又回退到按名字的旧表。
                 */
                d2--;
                if (d2 == 0) { lparen = t - 1; break; }
                t--;
                continue;
            }
            t--;
        }
    }
    if (lparen == NULL) return 0;

    /* 4. 在 [lparen+1, r-1) 内查找 `类型族名字 name` */
    const char *z = lparen + 1;
    const char *pend = r - 1;
    while (z < pend) {
        while (z < pend && (*z == ' ' || *z == '\t' || *z == '\n' ||
                            *z == '\r' || *z == ',')) z++;
        if (z >= pend) break;

        const char *te = z;
        while (te < pend && is_ident_char(*te)) te++;
        size_t tl = (size_t)(te - z);
        if (tl == 0) { z++; continue; }

        int cls = type_family_of_token(z, tl);
        if (cls != 0) {
            const char *v = te;
            while (v < pend && (*v == ' ' || *v == '\t' ||
                                *v == '\n' || *v == '\r')) v++;
            if (v < pend && is_ident_char(*v) && !isdigit((unsigned char)*v)) {
                const char *ve = v;
                while (ve < pend && is_ident_char(*ve)) ve++;
                size_t vl = (size_t)(ve - v);
                if (vl == nlen && strncmp(v, name, nlen) == 0) return cls;
                z = ve;
                continue;
            }
        }
        z = te;
    }
    return 0;
}

/*
 * 判定「标识符 name 在它所属函数内的**声明**类型族」。
 *
 * 返回：1 = 整数族（int/uint/ivecN/uvecN/bvecN）
 *       2 = 浮点族（float/vecN/matN）
 *       0 = 未找到确定声明
 *
 * 只搜索 use 之前的文本（声明必在首次使用之前），并限定在其所属函数体
 * 区间内 —— 这是同一个「按名字全文件推断」缺陷家族的**通用**解法。
 *
 * ============ 为什么需要它 —— 真机 Flywheel 第三次同类故障 ============
 *
 *     Failed to compile pipeline/instancing/create_instance_rotating/
 *                      flywheel_material_default_default.vert
 *     error: no matching overloaded function found
 *      --> flywheel:util/quaternion.glsl
 *     21 |     vec3 i = q.xyz;
 *     22 |     return v + 2.0 * cross(i, cross(i, v) + q.w * v);
 *        |                      ^^^^^
 *   对**我们自己的输出**做 diff 还原出：
 *       cross(float(i), cross(float(i), v) + q.w * v)
 *   `cross` 在 kStrictFloatFns 里（ES 无整数重载），情形 F 因此不要求
 *   任何浮点证据就包 float()。但此处的 `i` 是 `vec3 i = q.xyz;`
 *   —— 它是浮点向量！被当成整数只因为同一程序**别处**有一个 `int i`
 *   （Flywheel 的 indexOf 之类），整数表按名字收录。
 *
 * 【为什么不能只看紧邻 token】
 *   先试过「看 expr_start 前面是不是类型名」，但 `cross(i, ...)` 里
 *   `i` 前面是 '('，不是声明，所以完全不生效（实测确认）。
 *   声明可能在函数体外几行，必须**按函数作用域搜索**。
 *
 * 【宁可漏改，不可误改】
 *   找到 float 族声明 -> 明确不包（这是确定的正确结论）。
 *   找到 int 族声明   -> 维持原有改写行为。
 *   都没找到          -> 返回 0，调用方维持原行为，不引入变化。
 */
static int local_decl_class_in_fn(const char *src, const char *use,
                                  const char *name, size_t nlen) {
    if (src == NULL || use == NULL || nlen == 0 || nlen >= 64) return 0;

    /* 1. 用块栈找出 use 之前最后一个未闭合的 '{' = 所属函数体起点 */
    const char *stack[64];
    int sp = 0;
    for (const char *z = src; z < use; ) {
        const char *sk = skip_comment_or_string(z, use);
        if (sk != NULL) { z = sk; continue; }
        if (*z == '{') {
            if (sp < 64) stack[sp] = z;
            sp++;
            z++;
            continue;
        }
        if (*z == '}') { if (sp > 0) sp--; z++; continue; }
        z++;
    }
    const char *body = (sp > 0) ? stack[sp - 1] : src;

    /* 2. 在 [body, use) 内找 `TYPE name`，只看最后一个匹配（最近的声明） */
    int found = 0;
    const char *p = body;
    while (p < use) {
        const char *sk = skip_comment_or_string(p, use);
        if (sk != NULL) { p = sk; continue; }
        if (*p == '(') {
            int d = 0;
            while (p < use) {
                if (*p == '(') d++;
                else if (*p == ')') { d--; if (d == 0) { p++; break; } }
                p++;
            }
            continue;
        }
        if (!is_ident_char(*p)) { p++; continue; }

        const char *te = p;
        while (te < use && is_ident_char(*te)) te++;
        size_t tl = (size_t)(te - p);

        int fam = type_family_of_token(p, tl);
        if (fam == 0) { p = te; continue; }

        const char *v = te;
        while (v < use && (*v == ' ' || *v == '\t' ||
                           *v == '\n' || *v == '\r')) v++;
        if (v < use && is_ident_char(*v) && !isdigit((unsigned char)*v)) {
            const char *ve = v;
            while (ve < use && is_ident_char(*ve)) ve++;
            if ((size_t)(ve - v) == nlen && strncmp(v, name, nlen) == 0) {
                found = fam;   /* 记录最近一次声明 */
            }
            p = ve;
            continue;
        }
        p = te;
    }
    return found;
}

/*
 * 【为什么不能只查名字表 —— Veil 兼容暴露的真实缺陷，实测在案】
 *
 * Veil 的 include 链里同一个名字在不同函数的形参列表里换类型：
 *     veil:space_helper : vec4 screenToWorldSpace(vec2  uv, float depth)
 *     veil:light        : vec2 minecraft_sample_lightmap_coords(ivec2 uv)
 * GLSL 把形参限制在各自函数作用域内，这是合法写法；`uv` 在后者里
 * **确定是 ivec2**。但：
 *   - 名字表按名字收录，`vec2 uv` 会把 `uv` 放进 fvecs
 *     （实测该表还会收录函数形参与返回类型，见 enclosing_fn_param_class）；
 *   - 情形 C 只看「fvecs 里有没有这个名字」就判定左操作数是浮点。
 * 于是 `uv + ivec2(1)` 被改写成 `uv + vec2(ivec2(1))`，ES 报：
 *     '+' : no operation '+' exists that takes a left-hand operand of type
 *           'in highp 2-component vector of int' and a right operand of type
 *           'const 2-component vector of float'
 * 桌面 GLSL 330 接受该隐式转换 —— 属本转换器职责。
 *
 * 【为什么「形参优先」是纠正而非取舍】
 *   形参类型由所在函数唯一确定，不存在歧义。只有名字不是本函数形参
 *   （全局变量 / 结构体成员 / 无法判定）时，才回退到按名字的全局表，
 *   从而与改动前的行为保持一致 —— 宁可漏改，不可误改。
 */
static int ident_is_float_operand(const char *src, const char *at,
                                  const char *name, size_t nlen,
                                  const ivec_var *fscalars, int fscalar_count,
                                  const ivec_var *fvecs, int fvec_count) {
    if (nlen == 0) return 0;

    /* 本函数形参的类型是确定事实，优先于任何按名字的全局表 */
    {
        int cls = enclosing_fn_param_class(src, at, name, nlen);
        if (cls == 1) return 0;     /* 整数族形参 -> 绝不是浮点操作数 */
        if (cls == 2) return 1;     /* 浮点族形参 -> 确为浮点操作数 */
    }

    for (int i = 0; i < fvec_count; i++) {
        if (strlen(fvecs[i].name) == nlen &&
            strncmp(name, fvecs[i].name, nlen) == 0) return 1;
    }
    for (int i = 0; i < fscalar_count; i++) {
        if (strlen(fscalars[i].name) == nlen &&
            strncmp(name, fscalars[i].name, nlen) == 0) return 1;
    }
    return 0;
}

/*
 * 收集被声明为浮点向量的变量名（vec2/3/4）。
 * 匹配 `out vec2 texCoord2` / `vec3 pos` 这类声明。
 */
/*
 * 判断「以 pe 为末尾（不含）的那个 token」是否为一个**已知的浮点域操作数**：
 * 命中即说明该 token 整体是「浮点标量」或「浮点向量」。
 *
 * 【用途 —— 真机 Flywheel 的 `int * float`】
 *   flywheel:internal/wavelet.glsl：
 *       float wavelet_phase = ((index + 1) & 1) * exp2(-power);
 *   `((index + 1) & 1)` 整体是 **int**（按位与只能作用于整数），
 *   而右侧 `exp2(...)` 是浮点。`int * float` 在 GLSL ES 非法：
 *       '*' : no operation '*' exists that takes a left-hand operand of
 *             type 'temp highp int' and a right operand of type 'temp highp float'
 *   情形 A/E 只覆盖「**单个标识符**（或它的分量）作为左操作数」，
 *   而这里的左操作数是一个**括号表达式**，因此一直漏改。
 *
 *   本函数把「括号表达式的结尾 / 标识符的结尾」解析为一个浮点操作数，
 *   供调用方在识别出「左操作数是整数域」后包上 float(...) 使用。
 *
 * 支持两类结尾：
 *   1. `xxx)`  —— 实参列表或分组括号的结尾
 *   2. `ident`（可带 .x/.xy 分量）—— 变量或分量
 *
 * 刻意**不**匹配类型名/构造函数（`float`/`vec3`/`int`/`uvec3`…）以及
 * 一定返回整数的内建（`floatBitsToInt` 等）：这些不是浮点操作数。
 */
static int float_operand_ends_at(const char *pe, const char *s,
                                 const ivec_var *fscalars, int fscalar_count,
                                 const ivec_var *fvecs, int fvec_count) {
    if (pe <= s) return 0;

    if (pe[-1] == ')') {
        /* 反向配对到对应的 '(' */
        const char *q = pe;
        int d = 0;
        while (q > s) {
            if (q[-1] == ')') { d++; q--; continue; }
            if (q[-1] == '(') { if (d == 0) break; d--; q--; continue; }
            q--;
        }
        if (q <= s || q[-1] != '(') return 0;
        const char *u = q - 1;
        while (u > s && (*u == ' ' || *u == '\t')) u--;
        const char *we = u;
        while (u > s && is_ident_char(u[-1])) u--;
        size_t fl = (size_t)(we - u);
        if (fl == 0) return 1;      /* 纯分组括号 `(...)`：交给别的规则 */
        /*
         * 构造函数 `T(...)` 的结果类型就是 T。
         *   float/vecN/matN  -> 浮点操作数（是）
         *   int/uint/ivecN/uvecN/bvecN/bool -> 不是
         */
        if (is_float_vec_type(u, fl)) return 1;
        {
            static const char *const kFloatCtors[] = {
                "float", "mat2", "mat3", "mat4",
                "mat2x2", "mat2x3", "mat2x4",
                "mat3x2", "mat3x3", "mat3x4",
                "mat4x2", "mat4x3", "mat4x4",
                "double", NULL
            };
            for (int i = 0; i < (int)LIST_LEN(kFloatCtors) && kFloatCtors[i] != NULL; i++) {
                if (strlen(kFloatCtors[i]) == fl &&
                    strncmp(kFloatCtors[i], u, fl) == 0) return 1;
            }
        }
        /* 浮点内建（length/dot/exp2/...）的返回值是浮点 */
        return name_in_list(u, fl, FLOAT_BUILTIN_FNS,
                            LIST_LEN_TERM(FLOAT_BUILTIN_FNS));
    }

    if (is_ident_char(pe[-1])) {
        const char *e = pe;
        /* 跳过分量选择（最多 4 个字符，且前面必须是 '.'） */
        {
            const char *t = e;
            int n = 0;
            while (t > s && n < 4 &&
                   (t[-1] == 'x' || t[-1] == 'y' || t[-1] == 'z' || t[-1] == 'w' ||
                    t[-1] == 'r' || t[-1] == 'g' || t[-1] == 'b' || t[-1] == 'a')) {
                t--; n++;
            }
            if (n > 0 && t > s && t[-1] == '.' &&
                t - 1 > s && is_ident_char(t[-2])) {
                e = t - 1;
            }
        }
        const char *b = e;
        while (b > s && is_ident_char(b[-1])) b--;
        if (b == e) return 0;
        size_t nl = (size_t)(e - b);
        for (int i = 0; i < fscalar_count; i++) {
            if (strlen(fscalars[i].name) == nl &&
                strncmp(fscalars[i].name, b, nl) == 0) return 1;
        }
        for (int i = 0; i < fvec_count; i++) {
            if (strlen(fvecs[i].name) == nl &&
                strncmp(fvecs[i].name, b, nl) == 0) return 1;
        }
        return 0;
    }
    return 0;
}

static int collect_float_scalar_vars(const char *src, ivec_var *out, int max);

static int collect_float_vec_vars(const char *src, ivec_var *out, int max) {
    const char *end = src + strlen(src);
    const char *p = src;
    int n = 0;
    int depth = 0;   /* 只在全局作用域收集，避免函数形参被当全局（见 collect_ivec_vars） */
    int paren = 0;    /* 圆括号深度：函数形参写在 '{' 之前，必须靠它排除 */

    while (p < end && n < max) {
        const char *skipped = skip_comment_or_string(p, end);
        if (skipped != NULL) { p = skipped; continue; }
        if (*p == '(') { paren++; p++; continue; }
        if (*p == ')') { if (paren > 0) paren--; p++; continue; }
        if (*p == '{') { depth++; p++; continue; }
        if (*p == '}') { if (depth > 0) depth--; p++; continue; }
        if (!is_ident_char(*p)) { p++; continue; }

        const char *q = p;
        while (q < end && is_ident_char(*q)) q++;
        size_t len = (size_t)(q - p);

        if (is_float_vec_type(p, len)) {
            const char *r = q;
            while (r < end && (*r == ' ' || *r == '\t' ||
                               *r == '\n' || *r == '\r')) {
                r++;
            }
            if (r < end && is_ident_char(*r) && !isdigit((unsigned char)*r)) {
                const char *s2 = r;
                while (s2 < end && is_ident_char(*s2)) s2++;
                size_t nl = (size_t)(s2 - r);
                if (nl < sizeof(out[0].name)) {
                    memcpy(out[n].name, r, nl);
                    out[n].name[nl] = '\0';
                    /* 维度直接取关键字末位数字：vec2->2, vec3->3, vec4->4 */
                    out[n].dim = p[len - 1] - '0';
                    n++;
                    p = s2;
                    continue;
                }
            }
        }
        p = q;
    }
    return n;
}

/*
 * 判断该标识符在本语句内是否被声明为 **整数标量**（int / uint）。
 *
 * 【为什么必须补上 —— 真机 Flywheel 的第二处误包装】
 *   rotateByQuaternion 里有一个局部 `vec3 i = q.xyz;`，于是浮点向量表收录了 `i`。
 *   另一处函数里写的是：
 *       int i = coord - start;      // coord、start 均为 int
 *   is_assign_rhs_to_float_vec 只看左值名字 `i` 命中浮点表，
 *   就把右值当成「要赋给浮点向量的整数表达式」，包成：
 *       int i = float(coord) - start;      // float - int，ES 非法
 *   实测（native.log 差分 + 最小复现 C1/C2）确认：
 *   把那个 vec3 改名后就不再触发，说明污染源就是同名。
 *
 * 【为什么"只看本语句"就够】
 *   局部声明与使用点最近，优先级高于任何更外层声明。
 *   只承认「紧邻标识符之前的类型关键字」，不会跨语句误判。
 */
static int ident_declared_as_int_scalar_near(const char *id_start, const char *src) {
    const char *b = id_start;
    while (b > src && (b[-1] == ' ' || b[-1] == '\t' ||
                       b[-1] == '\n' || b[-1] == '\r')) {
        b--;
    }
    const char *e2 = b;
    while (b > src && is_ident_char(b[-1])) b--;
    size_t tl = (size_t)(e2 - b);
    if (tl == 0) return 0;
    if (tl == 3 && strncmp(b, "int", 3) == 0)  return 1;
    if (tl == 4 && strncmp(b, "uint", 4) == 0) return 1;
    return 0;
}

/*
 * 判断该标识符在任何位置是否被声明为 **整数向量**（ivecN / uvecN）。
 *
 * 【为什么需要 —— 真机 Flywheel 飞轮 instancing 编译失败】
 *   Flywheel 的 light_lut.glsl 里同时存在两个 `light`：
 *       struct FlwLightAo { ... vec2 light; ... };   // 结构体成员，类型 vec2
 *       ...
 *       uvec2 light = _flw_lightAt(sectionOffset, uvec3(...));   // 局部，类型 uvec2
 *   浮点向量表按 **名字** 收录，于是 `light` 命中；
 *   is_assign_rhs_to_float_vec 只看左值名字，就把它当成浮点向量，
 *   给右值套上 vec2(...)，得到 `uvec2 light = vec2(...)` —— 类型不匹配。
 *   该行位于宏体内、展开 31 次，一次制造 31 条错误（实测 27 条）。
 *
 * 【为什么不能"全文件范围内只要出现整数声明就否决浮点"】
 *   那会走向另一个极端：与 collect_ivec_vars 的约束 2 相互否决，
 *   使两张表同时把该名字排除，于是既不按整数处理、也不按浮点处理，
 *   真正的 `vec2 v = ivec2var;`（需要包 vec2）反而漏改。
 *   实测到的缺陷是**局部变量被结构体成员的名字污染**，
 *   因此这里只否决「在本语句内被声明为整数向量」的情形 —— 精确、无副作用。
 */
static int ident_declared_as_int_vec_near(const char *id_start, const char *src) {
    /* 向前跳过空白 */
    const char *b = id_start;
    while (b > src && (b[-1] == ' ' || b[-1] == '\t' ||
                       b[-1] == '\n' || b[-1] == '\r')) {
        b--;
    }
    /* 再向前取一个标识符，它应当是本标识符的类型名 */
    const char *e2 = b;
    while (b > src && is_ident_char(b[-1])) b--;
    size_t tl = (size_t)(e2 - b);
    if (tl == 0) return 0;
    return ivec_dim_of(b, tl) > 0;
}

/*
 * 判断位置 var_start 处的 ivec 变量是否为「赋值给浮点向量」的右值。
 *
 * 从 var_start 向前扫描：跳过空白应遇到【单个】'='，
 * 再向前取标识符作为左值，并确认它是浮点向量变量。
 *
 * 必须排除复合/比较运算符（== != <= >= += 等），否则
 * `if (a == UV2)` 这类比较会被误判为赋值。
 */
static int is_assign_rhs_to_float_vec(const char *var_start, const char *src,
                                      const ivec_var *fvecs, int fvec_count) {
    /* 1. 向前跳过空白，应停在 '=' 之后 */
    const char *l = var_start;
    while (l > src && (l[-1] == ' ' || l[-1] == '\t' ||
                       l[-1] == '\n' || l[-1] == '\r')) {
        l--;
    }
    if (l <= src || l[-1] != '=') return 0;

    /* 2. 排除复合/比较运算符 */
    if (l - 2 >= src) {
        switch (l[-2]) {
            case '=': case '!': case '<': case '>':
            case '+': case '-': case '*': case '/': case '%':
            case '&': case '|': case '^':
                return 0;
            default:
                break;
        }
    }

    /* 3. 越过 '='，跳过空白 */
    const char *r = l - 1;          /* 指向 '=' */
    while (r > src && (r[-1] == ' ' || r[-1] == '\t' ||
                       r[-1] == '\n' || r[-1] == '\r')) {
        r--;
    }

    /* 4. 从 r 向前取标识符（r 指向标识符之后的第一个字符位置） */
    const char *id_end = r;
    const char *id_start = r;
    while (id_start > src && is_ident_char(id_start[-1])) id_start--;
    size_t nlen = (size_t)(id_end - id_start);
    if (nlen == 0) return 0;

    /*
     * 4b. 若该左值就在本语句内被声明为**整数向量**，则它绝不是浮点向量。
     *
     * 【真机 Flywheel 证据】结构体成员 `vec2 light;` 与局部 `uvec2 light = ...`
     * 同名，浮点表按名字收录了 light，于是把 `uvec2 light = f(...)`
     * 误判为「赋值给浮点向量」，给右值套 vec2(...) 造成类型不匹配。
     * 局部声明与使用点最近，优先级最高 —— 见到整数向量类型名即否决。
     */
    if (ident_declared_as_int_vec_near(id_start, src)) return 0;

    /*
     * 4c. 同理：本语句内被声明为**整数标量**（int/uint）也否决。
     *
     * 【真机 Flywheel 第二例】`rotateByQuaternion` 里的 `vec3 i = q.xyz;`
     * 让浮点表收录了 `i`；另一处的 `int i = coord - start;` 因此被误改
     * （实测会把 -2 → -2.0、coord → float(coord)，产出 ES 非法的 float-int）。
     * 实测对照：把 `vec3 i` 改名为 `ii` 后不再触发。
     */
    if (ident_declared_as_int_scalar_near(id_start, src)) return 0;

    for (int i = 0; i < fvec_count; i++) {
        if (strlen(fvecs[i].name) == nlen &&
            strncmp(id_start, fvecs[i].name, nlen) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * 收集返回整数向量的函数名及其维度。
 *
 * 【为什么必须单独处理函数返回值】
 *   Sodium 0.8.13 的 chunk_vertex.glsl 里有：
 *       vec3 _get_draw_translation(uint pos) {
 *           return _get_relative_chunk_coord(pos) * vec3(16.0);
 *       }
 *   而 `_get_relative_chunk_coord` 的声明是 `uvec3 _get_relative_chunk_coord(...)`。
 *   于是 `uvec3返回值 * vec3` —— **GLSL ES 非法**（uint 向量与 float 向量
 *   之间没有隐式转换），桌面 GLSL 合法。
 *   实测（native/tools/sodium_pattern_probe.py，glslang 裁决）：
 *       '*' : no operation '*' exists that takes a left-hand operand of type
 *             '3-component vector of uint' and a right operand of type
 *             'const 3-component vector of float'
 *   这会让 Sodium 的区块着色器**永远编译失败** -> 进世界即崩溃。
 *
 *   注意：函数返回值既不是变量也不是构造函数，先前的实现（只按变量名匹配）
 *   完全覆盖不到，因此必须显式收集「返回类型 + 函数名」。
 */
static int collect_ivec_returning_funcs(const char *src, ivec_var *out, int max) {
    const char *end = src + strlen(src);
    const char *p = src;
    int n = 0;

    while (p < end && n < max) {
        const char *skipped = skip_comment_or_string(p, end);
        if (skipped != NULL) { p = skipped; continue; }

        if (!is_ident_char(*p)) { p++; continue; }

        const char *q = p;
        while (q < end && is_ident_char(*q)) q++;
        size_t len = (size_t)(q - p);

        /* p..q 是类型名；须为整数向量类型，且它「看起来像声明」而非调用 */
        int dim = ivec_dim_of(p, len);
        if (dim > 0) {
            /*
             * 排除构造函数调用 `uvec3( ... )`：类型名后紧跟 '(' 的是构造，
             * 不是「返回类型 函数名(」的声明。
             */
            const char *r = q;
            while (r < end && (*r == ' ' || *r == '\t' ||
                               *r == '\n' || *r == '\r')) {
                r++;
            }
            if (r < end && *r != '(' && is_ident_char(*r)) {
                const char *fn = r;
                while (r < end && is_ident_char(*r)) r++;
                size_t fnl = (size_t)(r - fn);
                /* 函数名后（允许空白）必须紧跟 '(' */
                const char *s2 = r;
                while (s2 < end && (*s2 == ' ' || *s2 == '\t' ||
                                    *s2 == '\n' || *s2 == '\r')) {
                    s2++;
                }
                if (s2 < end && *s2 == '(' && fnl < sizeof(out[0].name)) {
                    memcpy(out[n].name, fn, fnl);
                    out[n].name[fnl] = '\0';
                    out[n].dim = dim;
                    n++;
                }
            }
        }
        p = q;
    }
    return n;
}

/*
 * ============ 内建函数的「整型返回」与「整型实参」表（缺陷 A / B） ============
 *
 * 【真机证据 —— Simulated，native.log 14:20:03 / 14:20:04】
 *
 *   shader=57（simulated:redstone_accumulator/diode 的片元）
 *       RAW : vec4 colB = texture(TextureSheet,
 *                     (texCoord + (vec2(16.0, 0.0) / textureSize(TextureSheet, 0))));
 *       我们原样送出 -> 驱动报：
 *           '/' : no operation '/' exists that takes a left-hand operand of
 *                 type 'const 2-component vector of float' and a right operand
 *                 of type '2-component vector of int'
 *       └─ 缺陷 B：`textureSize` 返回 **ivec2**，赋值/参与浮点运算时必须
 *                  显式转成 vec2。我们只登记了**用户函数**的整型返回值，
 *                  不知道内建函数也会返回整型向量。
 *
 *   shader=101（simulated:contraption_diagram/outline_diagram 的片元）
 *       RAW : vec2 screen_size = vec2(textureSize(DiffuseSampler0, 0));
 *       我们改为 0.0 -> 驱动报：
 *           'textureSize' : no matching overloaded function found
 *       └─ 缺陷 A：`textureSize(sampler, int lod)` 的 lod **必须是 int**。
 *                  规则 A（浮点构造函数实参一律浮点化）只知道自己身处
 *                  `vec2(` 之内，不认识「第几个实参」这个概念，于是把
 *                  lod 改成了 `0.0`。
 *
 * 【为什么两张表必须成对存在】
 *   它们是同一件事的两面：`textureSize` 是「实参要 int、返回 ivec」的内建函数。
 *   只修 A 会留下 B，只修 B 会留下 A —— 真机上两个着色器同时中招，
 *   正是两个方向各掉一半。
 *
 * 【为什么不能「把内建也塞进 FLOAT_CTORS/INT_CTORS」】
 *   那两张表表达的是「**构造函数**的实参类型」，其判定发生在
 *   normalize_int_literals 的括号栈里（int_ctor_depth / float_ctor_depth）。
 *   而这里需要的是「**某函数的第 N 个实参**」，是位置级信息，
 *   括号栈的深度计数表达不了。故另立一张位置表。
 */

/* 返回整型向量的内建函数（GLSL ES 3.2 规范第 8 章）。 */
typedef struct {
    const char *name;
    int dim;            /* 返回的整数向量维度：2 / 3 / 4 */
} int_returning_builtin;

static const int_returning_builtin INT_RETURNING_BUILTINS[] = {
    { "textureSize", 2 },   /* ivec2 textureSize(gsampler2D s, int lod) */
    { "imageSize",   2 },   /* ivec2 imageSize(gimage2D image) */
};

/*
 * 必须是 int 的实参位置（0 基）。
 *
 * 【为什么只收这几个 —— 宁可漏改，不能误改】
 *   把某个位置误判为「要 int」而实际是 float，会造成**漏改**
 *   （ES 缺隐式转换 -> 编译失败）；把 float 位置误判为要 int 没有风险。
 *   反过来，把一个本当保持 int 的位置浮点化就是缺陷 A 本身。
 *   因此本表只收「按 GLSL ES 规范确定是 int」的位置，且逐个可验证。
 *
 * 【刻意不收的（避免误判）】
 *   textureLod(sampler, P, float lod)  —— lod 是 **float**，不在表内；
 *   texture(sampler, P, float bias)   —— bias 是 float；
 *   textureGrad / textureProj           —— 无 int 实参。
 */
typedef struct {
    const char *name;
    int int_args[4];        /* int 实参位置，-1 结尾 */
} int_arg_builtin;

static const int_arg_builtin INT_ARG_BUILTINS[] = {
    { "textureSize",    { 1, -1 } },          /* textureSize(s, int lod) */
    { "texelFetch",     { 2, -1 } },          /* texelFetch(s, ivecN P, int lod) */
    { "imageLoad",      { 1, -1 } },          /* imageLoad(image, ivecN P) */
    { "imageStore",     { 1, -1 } },          /* imageStore(image, ivecN P, data) */
    { "imageAtomicAdd", { 1, -1 } },
    { "imageAtomicMin", { 1, -1 } },
    { "imageAtomicMax", { 1, -1 } },
    { "imageAtomicAnd", { 1, -1 } },
    { "imageAtomicOr",  { 1, -1 } },
    { "imageAtomicXor", { 1, -1 } },
    { "imageAtomicExchange", { 1, -1 } },
    { "imageAtomicCompSwap", { 1, -1 } },
    { "atomicAdd",      { 1, -1 } },          /* atomicAdd(mem, ivecN P, data) */
    { "atomicMin",      { 1, -1 } },
    { "atomicMax",      { 1, -1 } },
    { "atomicAnd",      { 1, -1 } },
    { "atomicOr",       { 1, -1 } },
    { "atomicXor",      { 1, -1 } },
    { "atomicExchange", { 1, -1 } },
    { "atomicCompSwap", { 1, -1 } },
    { "bitfieldExtract", { 1, 2, -1 } },      /* (value, int offset, int bits) */
    { "bitfieldInsert",  { 2, 3, -1 } },      /* (base, insert, int offset, int bits) */
};

/*
 * 名字是否为「实参要 int」的内建函数；是则填充该表项，否则返回 NULL。
 */
static const int_arg_builtin *builtin_int_arg_entry(const char *s, size_t len) {
    for (size_t i = 0; i < LIST_LEN(INT_ARG_BUILTINS); i++) {
        if (strlen(INT_ARG_BUILTINS[i].name) == len &&
            strncmp(INT_ARG_BUILTINS[i].name, s, len) == 0) {
            return &INT_ARG_BUILTINS[i];
        }
    }
    return NULL;
}

/**
 * 判断位置 p（一个整数字面量的起点）是否处于「内建函数的整型实参」上。
 *
 * 例：
 *   vec2(textureSize(tex, 0))     <- 这个 0 是 int，绝不能变成 0.0
 *   texelFetch(tex, P, 0)         <- 同上（第 2 个实参）
 *
 * 实现：从 p 向左扫描，找到最近的**未配对** '('，取其左边的标识符
 *       与「本实参的序号」，查 INT_ARG_BUILTINS 判定。
 *
 * 括号配对细节（容易写错的地方）：
 *   - `）` 使 paren++ ；遇到 `(` 时若 paren>0 则 paren--；
 *     只有 paren==0 的那个 `(` 才是「包含本实参的调用」。反过来写会永远
 *     找到最深层的那个括号，判定必然出错。
 *   - `[ ]` 内的逗号不是实参分隔符，单独用一个 brack 计数器排除。
 *   - 只数「本层」的逗号（paren==0 && brack==0）。
 */
static int in_builtin_int_arg(const char *p, const char *src) {
    int paren = 0, brack = 0, arg_index = 0;
    const char *q = p;
    while (q > src) {
        char c = q[-1];
        if (c == ')') {
            paren++;
        } else if (c == '(') {
            if (paren > 0) {
                paren--;
            } else {
                /* 找到包含本实参的调用了 */
                const char *e2 = q - 1;
                const char *s2 = e2;
                while (s2 > src && is_ident_char(s2[-1])) s2--;
                size_t nl = (size_t)(e2 - s2);
                if (nl == 0) return 0;
                const int_arg_builtin *fn = builtin_int_arg_entry(s2, nl);
                if (fn == NULL) return 0;
                for (int i = 0; i < 4 && fn->int_args[i] >= 0; i++) {
                    if (fn->int_args[i] == arg_index) return 1;
                }
                return 0;
            }
        } else if (c == ']') {
            brack++;
        } else if (c == '[') {
            if (brack > 0) brack--;
        } else if (c == ',' && paren == 0 && brack == 0) {
            arg_index++;
        }
        q--;
    }
    return 0;
}

/**
 * 判断位置 p 是否处在某个**浮点构造函数**的实参列表里。
 *
 * 【为什么需要】
 *   对 `vec2(textureSize(tex, 0))` 而言，外层 `vec2(...)` 已经完成了
 *   ivec2 -> vec2 的显式转换，无需再包一层；而若再包一层会得到
 *   `vec2(vec2(textureSize(tex, 0)))`，语义虽同、但与缺陷 A 的修复相互干扰。
 *   更重要的是：**该形态下外层浮点上下文会渗透进 textureSize 的 lod 实参**，
 *   正是缺陷 A 的现场。因此情形 B 在此形态下应主动放弃改写。
 *
 * 实现与 in_builtin_int_arg 同构：向左找最近未配对的 '('，
 * 看它左边的标识符是不是浮点构造函数名。
 */
static int inside_float_ctor_args(const char *p, const char *src) {
    int paren = 0;
    const char *q = p;
    while (q > src) {
        char c = q[-1];
        if (c == ')') {
            paren++;
        } else if (c == '(') {
            if (paren > 0) {
                paren--;
            } else {
                const char *e2 = q - 1;
                const char *s2 = e2;
                while (s2 > src && is_ident_char(s2[-1])) s2--;
                size_t nl = (size_t)(e2 - s2);
                if (nl == 0) return 0;
                return name_in_list(s2, nl, FLOAT_CTORS, LIST_LEN(FLOAT_CTORS));
            }
        }
        q--;
    }
    return 0;
}

/*
 * 把「返回整型向量的内建函数」登记进 ivec 表。
 *
 * 【为什么把内建塞进同一张表就够】
 *   主循环里 (b)/(c) 两路都是「按 ivec 表匹配名字」：
 *   命中后 matched_dim 取表里的维度、并把 expr_end 扩到实参结束。
 *   用户函数与内建**除了名字来源不同，后续处理完全相同**，
 *   所以直接复用既有的 8 处查表点，不需要为内建新开一套情形 ——
 *   改动面最小、语义最一致。
 *
 * 【为什么必须「源码里真的出现过」才登记 —— 一个必须避开的副作用】
 *   调用方 fix_int_vector_float_ops 开头有快速路径：
 *       if (ivec_count == 0) { sbuf_puts(out, src); return; }
 *   若无条件登记 textureSize/imageSize，ivec_count 永远 >= 2，
 *   快速路径**永久失效**，于是**每一个**着色器都要跑完整主循环。
 *   那是一个与本次缺陷无关的行为变化，会让以前从未进入主循环的着色器
 *   暴露在主循环的既有启发式之下 —— 属于典型的「修 A 引入 B」。
 *   因此这里先确认名字在源码里**作为标识符**出现过，才登记；
 *   没用到的着色器行为与修复前完全一致。
 */
static int collect_builtin_int_returning_funcs(const char *src, ivec_var *out,
                                               int max) {
    int n = 0;
    for (size_t i = 0; i < LIST_LEN(INT_RETURNING_BUILTINS) && n < max; i++) {
        const char *nm = INT_RETURNING_BUILTINS[i].name;
        size_t l = strlen(nm);
        if (l >= sizeof(out[0].name)) continue;

        int present = 0;
        for (const char *p = src; *p != '\0'; p++) {
            if (strncmp(p, nm, l) != 0) continue;
            if (p > src && is_ident_char(p[-1])) continue;      /* 名字的一部分 */
            if (is_ident_char(p[l])) continue;                  /* 更长标识符的前缀 */
            present = 1;
            break;
        }
        if (!present) continue;

        memcpy(out[n].name, nm, l + 1);
        out[n].dim = INT_RETURNING_BUILTINS[i].dim;
        n++;
    }
    if (n > 0) {
        /*
         * 一次性诊断。这条字符串同时充当**构建标记**：
         * native/tools/required_strings.txt 里登记了 GLESMOD_RULE_S_BUILTIN_INT_ARGS，
         * 构建校验会确认它存在于产物中，从而区分「新代码已编入」与「用了旧 .so」。
         */
        glesmod_log("规则S 已识别内建函数的整型实参与整型返回值"
                    "（缺陷A/B：textureSize 的 lod 保持 int、其返回值补 vecN 包装；"
                    "GLESMOD_RULE_S_BUILTIN_INT_ARGS）");
    }
    return n;
}

/*
 * 收集「浮点标量」声明（float），用于识别 `整数向量 * float变量` 的非法混算。
 *
 *   Sodium chunk_vertex.glsl:
 *       const float VERTEX_SCALE = 32.0 / float(POSITION_MAX_COORD);
 *       const float VERTEX_OFFSET = -8.0;
 *       _vert_position = (_deinterleave_u20x3(a_Position) * VERTEX_SCALE)
 *                        + VERTEX_OFFSET;
 *   uvec3 * float 在 GLSL ES 非法（桌面合法），必须写成
 *   vec3(...) * VERTEX_SCALE。
 */
static int collect_float_scalar_vars(const char *src, ivec_var *out, int max) {
    const char *end = src + strlen(src);
    const char *p = src;
    int n = 0;
    int depth = 0;   /* 只在全局作用域收集，避免函数形参被当全局 */
    int paren = 0;    /* 圆括号深度：函数形参写在 '{' 之前，必须靠它排除 */

    while (p < end && n < max) {
        const char *skipped = skip_comment_or_string(p, end);
        if (skipped != NULL) { p = skipped; continue; }
        if (*p == '(') { paren++; p++; continue; }
        if (*p == ')') { if (paren > 0) paren--; p++; continue; }
        if (*p == '{') { depth++; p++; continue; }
        if (*p == '}') { if (depth > 0) depth--; p++; continue; }
        if (!is_ident_char(*p)) { p++; continue; }

        const char *q = p;
        while (q < end && is_ident_char(*q)) q++;
        size_t len = (size_t)(q - p);

        if (len == 5 && strncmp(p, "float", 5) == 0) {
            const char *r = q;
            while (r < end && (*r == ' ' || *r == '\t' ||
                               *r == '\n' || *r == '\r')) r++;
            /* `float x` / `const float y`：类型名后是标识符（且不是 float(） */
            if (r < end && *r != '(' && is_ident_char(*r) &&
                !isdigit((unsigned char)*r)) {
                const char *s2 = r;
                while (s2 < end && is_ident_char(*s2)) s2++;
                size_t nl = (size_t)(s2 - r);
                if (nl < sizeof(out[0].name)) {
                    memcpy(out[n].name, r, nl);
                    out[n].name[nl] = '\0';
                    out[n].dim = 1;     /* 标量 */
                    n++;
                    p = s2;
                    continue;
                }
            }
        }
        p = q;
    }
    return n;
}

/*
 * 收集「整数标量」声明（uint / int），用于识别整数标量与浮点向量的非法混算。
 *
 *   Sodium: `vec3 translation = u_RegionID * vec3(16.0);`
 *           u_RegionID 是 `uniform uint` -> uint * vec3，ES 非法。
 */
static int collect_int_scalar_vars(const char *src, ivec_var *out, int max) {
    const char *end = src + strlen(src);
    const char *p = src;
    int n = 0;
    int depth = 0;   /* 只在全局作用域收集，避免函数形参被当全局 */
    int paren = 0;    /* 圆括号深度：函数形参写在 '{' 之前，必须靠它排除 */

    while (p < end && n < max) {
        const char *skipped = skip_comment_or_string(p, end);
        if (skipped != NULL) { p = skipped; continue; }
        if (*p == '(') { paren++; p++; continue; }
        if (*p == ')') { if (paren > 0) paren--; p++; continue; }
        if (*p == '{') { depth++; p++; continue; }
        if (*p == '}') { if (depth > 0) depth--; p++; continue; }
        if (!is_ident_char(*p)) { p++; continue; }

        const char *q = p;
        while (q < end && is_ident_char(*q)) q++;
        size_t len = (size_t)(q - p);

        int is_int_type = (len == 4 && strncmp(p, "uint", 4) == 0) ||
                          (len == 3 && strncmp(p, "int", 3) == 0);
        if (is_int_type) {
            const char *r = q;
            while (r < end && (*r == ' ' || *r == '\t' ||
                               *r == '\n' || *r == '\r')) {
                r++;
            }
            /* 形如 `int i` / `uniform uint u_RegionID`：类型名后是标识符 */
            if (r < end && *r != '(' && is_ident_char(*r) &&
                !isdigit((unsigned char)*r)) {
                const char *s2 = r;
                while (s2 < end && is_ident_char(*s2)) s2++;
                size_t nl = (size_t)(s2 - r);
                /*
                 * 同名冲突排除：若该名字在**全局作用域**被声明为浮点，
                 * 就不把它当整数。理由见 collect_ivec_vars 的约束 2 ——
                 * 光影包里同一名字常在不同作用域换类型
                 * （`float pow2(float x)` 与 `int pow2(int x)`），
                 * 误当整数会把浮点用法包成 `float(x)` 从而产生语法错误。
                 *
                 * 【为什么从「任何位置」收紧为「全局作用域」】
                 *   同 collect_ivec_vars 约束 2：本收集器也只在深度 0 收集，
                 *   否决范围必须与之对齐，否则函数形参的同名换类型会让
                 *   两张表同时为空，真正的 `ivec2 / float` 反而漏改。
                 */
                if (nl < sizeof(out[0].name) &&
                    !ident_declared_as_float_at_global_scope(src, end, r, nl)) {
                    memcpy(out[n].name, r, nl);
                    out[n].name[nl] = '\0';
                    out[n].dim = 1;     /* 1 = 标量 */
                    n++;
                    p = s2;
                    continue;
                }
            }
        }
        p = q;
    }
    return n;
}

/*
 * 主修复过程：处理「整数向量与浮点混用」的两类情形。
 *
 *   情形 A（算术运算）：`ivecN变量 / 浮点字面量`
 *       改写为 `vecN(变量) / 浮点字面量`
 *
 *   情形 B（赋值）：`浮点向量变量 = ivecN变量`
 *       改写为 `浮点向量变量 = vecN(变量)`
 *
 * 【为什么两类都要做，而且必须区分左右值】
 *   驱动报过两种错：
 *     ERROR: 1:14: '/' : wrong operand types ... '2-component vector of int'
 *            and 'const float'                        -> 情形 A
 *     ERROR: 0:34: 'assign' : cannot convert from
 *            'attribute 2-component vector of int' to
 *            'varying 2-component vector of float'     -> 情形 B
 *   桌面 GLSL 都允许隐式转换，GLSL ES 都不允许。
 *
 *   情形 B 必须确认左值是浮点向量：若左值本身是 ivec2
 *   （`ivec2 iv = UV2;`），加 vec2() 反而制造错误。
 *
 *   情形 A 只处理右操作数为【浮点字面量】的情形，
 *   因此 `texelFetch(Sampler2, UV2 / 16, 0)` 这类必需的整数除法保持不变。
 *
 * 【本次扩充：Sodium 区块着色器暴露的三种漏网情形】
 *   原先只匹配「被声明为 ivecN/uvecN 的**变量**」作为左操作数，实测覆盖不住：
 *     1. **构造函数**：`uvec3(pos) * vec3(16.0)`      （类型名 + 实参）
 *     2. **函数返回值**：`_get_relative_chunk_coord(pos) * vec3(16.0)`
 *        （函数返回 uvec3）
 *     3. **整数标量**：`u_RegionID * vec3(16.0)`       （uniform uint）
 *   三者都是「整数域操作数 × 浮点向量」，ES 一律拒绝。
 *   Sodium 的 chunk_vertex.glsl 同时命中 2 和 3，导致区块着色器必然失败。
 *
 *   统一策略：把左操作数记为「(起始, 结束, 维度)」的**表达式片段**，
 *   而不是变量名，然后按需加 `vecN( ... )` 包裹。
 */
/*
 * 【下面三个辅助函数为什么放在这里而不是文件更前面】
 *   lhs_base_ident / in_float_lod_arg / paren_expr_is_float 都用到
 *   ivec_var 类型与 starts_with_float_literal()。这两者在本文件中定义
 *   较晚，先前把它们写在调用点之前会导致「unknown type name 'ivec_var'」
 *   与「implicit declaration of starts_with_float_literal」编译错误。
 *   放在 fix_int_vector_float_ops 之前即可，此处所有依赖均已就绪。
 */
/*
 * 从位置 m（某个标识符的结束处）反向取出**基标识符**，跳过尾部的 swizzle。
 *
 * 【为什么需要 —— 整包验证暴露的真实缺陷】
 *     vec2 frac = fract(shadowPos.xy * shadowMapResolution);
 *     shadowPos.xy = (floor(shadowPos.xy * shadowMapResolution) + 0.5)
 *                    / shadowMapResolution;
 *   `shadowMapResolution` 是 `const int`，`vec2 * const int` 在 ES 非法。
 *   情形 C 判定「左侧是否浮点」时反向取标识符，遇到 `.xy` 只会取到 `y`，
 *   而 `y` 不在浮点表里，于是 lhs_float 判为假，整类都不改写。
 *
 *   正确做法是跨过 swizzle 取到基名 `shadowPos`，它的类型才是真正的左值类型。
 *
 * 返回基标识符起点，*len 为其长度。
 */
/*
 * 判断「结束于位置 m 的那个 token」是否是一个**浮点字面量**。
 *
 * 【为什么必须单独成函数 —— 真机 BSL 证据（错误 3/3 的根因）】
 *     color += 0.25f * lightCol * visibility * (1.0f + 0.25f * isEyeInWater);
 *     `isEyeInWater` 是 `uniform int`。桌面 GLSL 允许 `float * int` 的隐式
 *     转换，GLSL ES 不允许，真机报：
 *         '*' : no operation '*' exists that takes a left-hand operand of
 *               type 'const float' and a right operand of type 'uniform int'
 *
 *     转换器本来有「情形 C」专门处理这一类（整数在右、浮点在左），
 *     它判定左操作数是否为浮点时写的是：
 *         if (isdigit(m[-1]) || m[-1] == '.') lhs_float = 1;
 *     BSL **把每一个浮点字面量都带 f 后缀**（`0.25f`、`1.0E-4f`），
 *     于是 `m[-1]` 是 'f' —— 既不是数字也不是小数点 —— 判定失败；
 *     随后回退到 `lhs_base_ident` 又会把单个 'f' 当成标识符，同样不在
 *     浮点变量表里。整类写法因此**从未被改写**，直到真机把行号指出来。
 *
 * 判据：从 m 反向吃掉 [标识符字符 | '.'] 得到 token 起点，
 *       再用 float_literal_len 校验「整个 token 恰好是一个浮点字面量」。
 *       这样 `0.25f`、`1.0f`、`1e3f`、`1.0E-4f` 都命中，
 *       而 `uv.x`（token 为 `uv.x`）、`25`（纯整数）都不会误判。
 */
static int ends_with_float_literal(const char *m, const char *s) {
    if (m <= s) return 0;
    const char *t = m;
    /*
     * 反向吃掉 [标识符字符 | '.' | '+' | '-']：后两者是为指数形式的
     * 负号准备的（`1.0E-4f`）。多吃的字符不会造成误判，因为下面用
     * float_literal_len 做**整段精确比对**：`a-b`、`1-2` 这类 token
     * 的 float_literal_len 都不等于 token 长度，直接落空。
     */
    while (t > s && (is_ident_char(t[-1]) || t[-1] == '.' ||
                     t[-1] == '+' || t[-1] == '-')) t--;
    if (t == m) return 0;
    size_t n = (size_t)(m - t);
    return float_literal_len(t) == n;
}

static const char *lhs_base_ident(const char *m, const char *s, size_t *len) {
    const char *e = m;
    /*
     * 跳过尾部 swizzle。
     *
     * 【必须加长度与格式约束】直接看「末尾是 x/y/z/w」会把普通标识符
     * （如 `posx`、`view`、`blur`）误当成 swizzle，进而跨过并不存在的点。
     * GLSL swizzle 字符数最多 4，且**其前面必须是 '.'**。
     * 这里先要求「整段都是 swizzle 字符且长度 <= 4」，再检查点号。
     */
    {
        const char *t = e;
        int n = 0;
        while (t > s && n < 4 &&
               (t[-1] == 'x' || t[-1] == 'y' || t[-1] == 'z' || t[-1] == 'w' ||
                t[-1] == 'r' || t[-1] == 'g' || t[-1] == 'b' || t[-1] == 'a')) {
            t--;
            n++;
        }
        /* t 停在 swizzle 起点；若更早还有 swizzle 字符说明长度超 4，放弃 */
        if (n > 0 && t > s && t[-1] == '.') {
            if (!(t - 1 > s && is_ident_char(t[-2]))) {
                /* '.' 前不是标识符（可能是 `1.xy` 这样的字面量），不处理 */
            } else {
                e = t - 1;
            }
        }
    }
    const char *b = e;
    while (b > s && is_ident_char(b[-1])) b--;
    *len = (size_t)(e - b);
    return b;
}

/*
 * 判断位置 m 左侧那个「以 ')' 结尾」的左操作数是否属于浮点域。
 *
 * 【为什么需要 —— 整包验证暴露的真实缺陷（影响 20+ 个着色器）】
 *     vec2 frac = fract(shadowPos.xy * shadowMapResolution);
 *     shadowPos.xy = (floor(shadowPos.xy * shadowMapResolution) + 0.5)
 *                    / shadowMapResolution;
 *     getShadow(shadowtex, shadowPos.st + vec2(0.0, 0.0) / shadowMapResolution, ...)
 *   这三处的右操作数都是 `shadowMapResolution`（const int），而左操作数
 *   分别是 `floor(...)`、`(...)`、`vec2(...)`，**都以 ')' 结尾**。
 *   情形 C 反查左操作数标识符时遇到 ')' 只能得到空串，于是判定不出浮点，
 *   这类 `float / int` 全部漏改。
 *
 * 【做法】从 ')' 反向配对到它的 '('，看那个 '(' 前面的名字：
 *   是浮点构造函数（vecN/matN）、浮点内建函数（floor 等）或已知浮点变量
 *   => 左操作数属于浮点域。
 */
static int paren_expr_is_float(const char *m, const char *s,
                               const ivec_var *fvecs, int fvec_count,
                               const ivec_var *fscalars, int fscalar_count) {
    if (m <= s || m[-1] != ')') return 0;
    const char *t = m - 1;      /* 指向 ')' */
    int d = 0;
    while (t > s) {
        if (t[-1] == ')') { d++; t--; continue; }
        if (t[-1] == '(') {
            if (d == 0) break;
            d--; t--; continue;
        }
        t--;
    }
    if (t <= s || t[-1] != '(') return 0;

    /* 括号内若含浮点字面量，本身就是浮点上下文 */
    for (const char *q = t; q < m; q++) {
        if (starts_with_float_literal(q, m)) return 1;
    }

    /* 看 '(' 前面的名字 */
    const char *u = t - 1;
    while (u > s && (*u == ' ' || *u == '\t')) u--;
    const char *we = u;
    while (u > s && is_ident_char(u[-1])) u--;
    size_t ul = (size_t)(we - u);
    if (ul == 0) return 0;

    if (name_in_list(u, ul, FLOAT_BUILTIN_FNS,
                     LIST_LEN_TERM(FLOAT_BUILTIN_FNS))) {
        return 1;
    }
    if (name_in_list(u, ul, FLOAT_CTORS, LIST_LEN(FLOAT_CTORS))) return 1;
    for (int i = 0; i < fvec_count; i++) {
        if (strlen(fvecs[i].name) == ul &&
            strncmp(u, fvecs[i].name, ul) == 0) return 1;
    }
    for (int i = 0; i < fscalar_count; i++) {
        if (strlen(fscalars[i].name) == ul &&
            strncmp(u, fscalars[i].name, ul) == 0) return 1;
    }
    return 0;
}

static void fix_int_vector_float_ops(sbuf *out, const char *src) {
    ivec_var ivecs[IVEC_VAR_MAX];
    int ivec_count = collect_ivec_vars(src, ivecs, IVEC_VAR_MAX);

    /*
     * 追加「返回整数向量的函数名」与「整数标量变量名」。
     * 表格共用，后续按名字长度+内容匹配即可（两者都不会与变量名冲突，
     * 因为函数名必然后跟 '('）。
     */
    {
        ivec_var funcs[IVEC_VAR_MAX];
        int nf = collect_ivec_returning_funcs(src, funcs, IVEC_VAR_MAX);
        for (int i = 0; i < nf && ivec_count < IVEC_VAR_MAX; i++) {
            ivecs[ivec_count++] = funcs[i];
        }
        /*
         * 【缺陷 B 的修复 —— 内建函数的整型返回值】
         *
         * collect_ivec_returning_funcs 只扫源码里的「类型名 函数名(」声明，
         * 因此**只认用户函数**。而 GLSL 里有一批内建函数也返回 ivecN，
         * 其中 textureSize 在真实工程里极常见：
         *     vec2 size = textureSize(tex, 0);          // 直接赋给 vec2
         *     vec2(16.0,0.0) / textureSize(tex, 0)      // 参与浮点运算
         * 两式在桌面 GLSL 下合法（有隐式转换），在 ES 下非法。
         *
         * 【真机证据】native.log 14:20:03，shader=57：
         *     RAW : vec4 colB = texture(TextureSheet, (texCoord +
         *                   (vec2(16.0, 0.0) / textureSize(TextureSheet, 0))));
         *     驱动: '/' : no operation '/' exists that takes a left-hand
         *           operand of type 'const 2-component vector of float' and a
         *           right operand of type '2-component vector of int'
         * 登记后，主循环的 (b) 路会把 `textureSize(...)` 整个当作「维度=2 的
         * 整数操作数」，交给情形 A / B 正常处理，与用户函数走同一条路。
         */
#ifndef GLESMOD_NO_RULE_S
        nf = collect_builtin_int_returning_funcs(src, funcs, IVEC_VAR_MAX);
        for (int i = 0; i < nf && ivec_count < IVEC_VAR_MAX; i++) {
            ivecs[ivec_count++] = funcs[i];
        }
#else
        (void)collect_builtin_int_returning_funcs;  /* 对照构建：禁用规则 S */
#endif
        ivec_var scalars[IVEC_VAR_MAX];
        int ns = collect_int_scalar_vars(src, scalars, IVEC_VAR_MAX);
        for (int i = 0; i < ns && ivec_count < IVEC_VAR_MAX; i++) {
            ivecs[ivec_count++] = scalars[i];
        }
        /* 再把 uvecN 类型的构造函数视为「维度已知的表达式起点」， */
    }

    if (ivec_count == 0) {
        sbuf_puts(out, src);
        return;
    }

    ivec_var fvecs[IVEC_VAR_MAX];
    int fvec_count = collect_float_vec_vars(src, fvecs, IVEC_VAR_MAX);

    /*
     * 浮点【标量】变量表（float）。
     *
     * 【为什么必须单独收集 —— 这是 Sodium 区块顶点着色器幸存的最后一处】
     *   chunk_vertex.glsl 里有：
     *       const float VERTEX_SCALE = 32.0 / float(POSITION_MAX_COORD);
     *       _vert_position = (_deinterleave_u20x3(a_Position) * VERTEX_SCALE)
     *                        + VERTEX_OFFSET;
     *   `_deinterleave_u20x3` 返回 uvec3，VERTEX_SCALE 是 float。
     *   于是得到 `uvec3 * float` —— **GLSL ES 非法**，桌面合法。
     *   真机 glslang 原话：
     *       '*' : no operation '*' exists that takes a left-hand operand of type
     *             '3-component vector of uint' and a right operand of type
     *             'const float'
     *   先前的右操作数判定只覆盖「浮点字面量」「vecN(...)」「浮点向量变量」，
     *   漏掉了最简单的「浮点标量变量」，因此这一处始终没被改写。
     */
    ivec_var fscalars[IVEC_VAR_MAX];
    int fscalar_count = collect_float_scalar_vars(src, fscalars, IVEC_VAR_MAX);


#ifdef GLESMOD_DIAG
    fprintf(stderr, "[diag] ivec_count=%d fvec_count=%d fscalar_count=%d\n",
            ivec_count, fvec_count, fscalar_count);
    for (int i = 0; i < fvec_count; i++)
        fprintf(stderr, "[diag]   fvec[%d] = %s\n", i, fvecs[i].name);
#endif

    /*
     * 诊断：统计「同名不同型」的标识符 —— 即同时出现在浮点表与整数表里的名字。
     *
     * 【为什么统计这个】
     *   这是唯一能定量回答「收紧类型判定会不会误伤既有产品」的办法：
     *   歧义名越多，收紧的覆盖面越大、回归风险越高。
     *   为 0 则说明收紧是空操作。
     */
#ifdef GLESMOD_DIAG
    {
        int ambiguous = 0;
        for (int i = 0; i < fvec_count; i++) {
            for (int j = 0; j < ivec_count; j++) {
                if (strcmp(fvecs[i].name, ivecs[j].name) == 0) {
                    fprintf(stderr, "[diag-amb] fvec '%s' ALSO in ivec (dim=%d)\n",
                            fvecs[i].name, ivecs[j].dim);
                    ambiguous++;
                    break;
                }
            }
        }
        for (int i = 0; i < fscalar_count; i++) {
            for (int j = 0; j < ivec_count; j++) {
                if (strcmp(fscalars[i].name, ivecs[j].name) == 0) {
                    fprintf(stderr, "[diag-amb] fscalar '%s' ALSO in ivec (dim=%d)\n",
                            fscalars[i].name, ivecs[j].dim);
                    ambiguous++;
                    break;
                }
            }
        }
        fprintf(stderr, "[diag-amb] total ambiguous names = %d\n", ambiguous);
    }
#endif

    /*
     * ============ 缺陷 3 的修法：剔除歧义名 ============
     *
     * 上面那段诊断一直在数「同时出现在浮点表与整数表里的名字」，
     * 但此前只是**统计**，没有据此采取行动。正是这些名字导致缺陷 3：
     *
     * 【实测证据】native/tools/fixtures/probe_scope_alias_arith.frag：
     *     int   add(int a, int b) { return a + b; }        // b 是 int
     *     float blend(int a, float b) { return b * 2.0; }  // b 是 float
     *   GLSL 把形参限制在各自函数内，合法。但浮点表按**名字**收录了 `b`，
     *   情形 H 判「另一侧是不是浮点」时查到 `b` 命中，就在 `add()` 里
     *   把左侧的 `a` 包成 float，产出
     *       int add(int a, int b) { return float(a) + b; }   // float + int，ES 非法
     *   桌面 GLSL 允许隐式转换，桌面不报错 —— 属本转换器职责。
     *
     * 【为什么「剔除」是这里唯一安全的动作】
     *   名字同时出现在两张表里，说明**按名字**判定它属于哪个类型族是不可靠的：
     *   它在不同作用域确实换过类型。此时任何基于名字的结论都可能落到错的
     *   作用域上。剔除后：该名字不再被当成「已知浮点」，
     *   于是不会去包裹别的操作数；而它真正作为浮点使用的地方，
     *   源码里必定另有浮点证据（浮点字面量、vecN(...)、swizzle），
     *   由规则 A/B'/C 独立覆盖，不会漏改。
     *
     * 【为什么只在这里做，而不是改 8 处查表点】
     *   fscalars / fvecs 的查询散落在情形 A / B / C / E / H 与
     *   `paren_expr_is_float` / `float_operand_ends_at` 等处（共 8 组）。
     *   在表**建成之后、主循环之前**剔除一次，所有查询点自动受益，
     *   改动面最小、语义最清晰。
     */
    {
        int dropped = 0;
#ifndef GLESMOD_NO_AMBIG_DROP
        for (int i = 0; i < fvec_count; i++) {
            int ambiguous = 0;
            for (int j = 0; j < ivec_count; j++) {
                if (strcmp(fvecs[i].name, ivecs[j].name) == 0) {
                    ambiguous = 1;
                    break;
                }
            }
            if (ambiguous) {
                /* 用末元素覆盖，缩容一个 */
                fvecs[i] = fvecs[fvec_count - 1];
                fvec_count--;
                i--;
                dropped++;
            }
        }
        for (int i = 0; i < fscalar_count; i++) {
            int ambiguous = 0;
            for (int j = 0; j < ivec_count; j++) {
                if (strcmp(fscalars[i].name, ivecs[j].name) == 0) {
                    ambiguous = 1;
                    break;
                }
            }
            if (ambiguous) {
                fscalars[i] = fscalars[fscalar_count - 1];
                fscalar_count--;
                i--;
                dropped++;
            }
        }
        if (dropped > 0) {
            glesmod_degrade(GLESMOD_DEGRADE_ENUM_MAPPED,
                            "着色器存在跨作用域同名的整数/浮点标识符 —— "
                            "已从浮点表中剔除这些歧义名，避免把整数操作数"
                            "误包成 float()（缺陷 3，GLESMOD_AMBIG_DROP）");
        }
#else
        (void)dropped;   /* 对照构建：禁用歧义名剔除 */
#endif
    }

    const char *end = src + strlen(src);
    const char *p = src;

    while (p < end) {
        /* 预处理器行整行原样复制（#line 等不得改写） */
        {
            const char *nl = memchr(p, '\n', (size_t)(end - p));
            const char *line_end = nl ? nl : end;
            if (line_starts_with_directive(p, line_end)) {
                size_t n = (size_t)(line_end - p) + (nl ? 1 : 0);
                sbuf_put(out, p, n);
                p += n;
                continue;
            }
        }

        const char *skipped = skip_comment_or_string(p, end);
        if (skipped != NULL) {
            sbuf_put(out, p, (size_t)(skipped - p));
            p = skipped;
            continue;
        }

        if (!is_ident_char(*p)) {
            /*
             * ================= 情形 I：括号表达式整体是整数域 =================
             *
             * 【真机 Flywheel 的 int * float】
             *   flywheel:internal/wavelet.glsl：
             *       float wavelet_phase = ((index + 1) & 1) * exp2(-power);
             *   `((index + 1) & 1)` 整体是 **int**（按位与只作用于整数），
             *   右侧 `exp2(...)` 是浮点，`int * float` 在 ES 非法：
             *       '*' : no operation '*' exists that takes a left-hand
             *             operand of type 'temp highp int' and a right
             *             operand of type 'temp highp float'
             *
             *   情形 A/E 只覆盖「**标识符**（或其分量）作左操作数」，
             *   而这里是**括号表达式**，所以此前所有规则都不命中。
             *
             * 【判据 —— 必须双侧都确认，宁可漏改不可误改】
             *   只有当括号表达式内部**确实存在按位运算**（`&`/`|`/`^`/`<<`/`>>`）
             *   时才认为它是整数域。按位只能作用于整数，这是可靠的：
             *   不存在任何浮点数参与 `&` 的合法写法。
             *   再加上右操作数确实是浮点，才包 float()。
             *
             *   反例（必须不触发）：`(a + b) * 2.0` —— 括号里是纯算术，
             *   若 a/b 是 int，这里会由情形 C/H 之类的规则按其自身语义处理，
             *   本规则不介入。
             */
            if (*p == '(') {
                /*
                 * 【必须先排除「调用/构造函数的括号」】
                 *   若 '(' 前面（跳空白）是标识符字符，那它是 `f(...)` 或
                 *   `float(...)` 这类**调用括号**，不是分组括号。
                 *   否则会对 `float((x & 0xFFu)) / 255.0` 再包一层，
                 *   产出已输出内容 + `float(` + 原文 = `floatfloat(...)`
                 *   （这是实测踩到的真实缺陷）。
                 *   只有分组括号才代表「一个整数域的子表达式」。
                 */
                const char *pb = p;
                while (pb > src && (pb[-1] == ' ' || pb[-1] == '\t' ||
                                    pb[-1] == '\n' || pb[-1] == '\r')) pb--;
                if (!(pb > src && is_ident_char(pb[-1]))) {
                const char *close = p;
                int d = 0;
                while (close < end) {
                    if (*close == '(') { d++; close++; continue; }
                    if (*close == ')') { d--; close++; if (d == 0) break; continue; }
                    close++;
                }
                if (d == 0 && close > p + 1) {
                    int has_bitwise = 0;
                    for (const char *z = p + 1; z < close - 1; z++) {
                        if (*z == '&' || *z == '|' || *z == '^') {
                            has_bitwise = 1;
                            break;
                        }
                        if ((*z == '<' && z + 1 < close && z[1] == '<') ||
                            (*z == '>' && z + 1 < close && z[1] == '>')) {
                            has_bitwise = 1;
                            break;
                        }
                    }
                    if (has_bitwise) {
                        const char *r = close;
                        while (r < end && (*r == ' ' || *r == '\t' ||
                                           *r == '\n' || *r == '\r')) r++;
                        if (r < end && (*r == '*' || *r == '/' ||
                                        *r == '+' || *r == '-')) {
                            const char *t = r + 1;
                            while (t < end && (*t == ' ' || *t == '\t' ||
                                               *t == '\n' || *t == '\r')) t++;
                            int rhs_float = 0;
                            if (starts_with_float_literal(t, end)) {
                                rhs_float = 1;
                            } else if (t < end && is_ident_char(*t)) {
                                const char *te = t;
                                while (te < end && is_ident_char(*te)) te++;
                                size_t tl = (size_t)(te - t);
                                /* 直接是浮点函数名？ */
                                for (int i = 0;
                                     i < (int)LIST_LEN(FLOAT_BUILTIN_FNS) &&
                                     FLOAT_BUILTIN_FNS[i] != NULL; i++) {
                                    if (strlen(FLOAT_BUILTIN_FNS[i]) == tl &&
                                        strncmp(FLOAT_BUILTIN_FNS[i], t, tl) == 0) {
                                        rhs_float = 1;
                                        break;
                                    }
                                }
                                if (!rhs_float) {
                                    rhs_float = float_operand_ends_at(te, src,
                                                    fscalars, fscalar_count,
                                                    fvecs, fvec_count);
                                }
                            }
                            if (rhs_float) {
                                sbuf_puts(out, "float(");
                                sbuf_put(out, p, (size_t)(close - p));
                                sbuf_puts(out, ")");
                                p = close;
                                continue;
                            }
                        }
                    }
                }
                }       /* end: 仅分组括号 */
            }
            sbuf_put(out, p, 1);
            p++;
            continue;
        }

        const char *q = p;
        while (q < end && is_ident_char(*q)) q++;
        size_t len = (size_t)(q - p);

        /*
         * 计算「整数域左操作数」的表达式范围 [expr_start, expr_end) 与维度。
         *
         * 三种来源：
         *   (a) 整数向量构造函数  uvec3( ... )      维度取自类型名
         *   (b) 返回整数向量的函数调用  f( ... )     维度取自返回类型
         *   (c) 整数向量 / 整数标量变量              维度取自声明
         * 前两者含实参列表，(c) 只有变量名本身。
         */
        const char *expr_start = p;
        const char *expr_end = q;      /* 默认只有标识符本身 */
        int matched_dim = 0;

        int ctor_dim = ivec_dim_of(p, len);
        if (ctor_dim > 0) {
            const char *r = q;
            while (r < end && (*r == ' ' || *r == '\t' ||
                               *r == '\n' || *r == '\r')) r++;
            if (r < end && *r == '(') {
                /* (a) 构造函数：把整段 uvecN(...) 作为操作数 */
                int depth = 0;
                const char *t = r;
                while (t < end) {
                    if (*t == '(') depth++;
                    else if (*t == ')') {
                        depth--;
                        if (depth == 0) { t++; break; }
                    }
                    t++;
                }
                if (depth == 0) { expr_end = t; }
                matched_dim = ctor_dim;
            }
        }

        if (matched_dim == 0) {
            /* (b)/(c)：按已有表匹配 */
            for (int i = 0; i < ivec_count; i++) {
                if (strlen(ivecs[i].name) == len &&
                    strncmp(p, ivecs[i].name, len) == 0) {
                    matched_dim = ivecs[i].dim;
                    /*
                     * 函数调用形态（名字后紧跟 '(' ）要连实参一起作为操作数，
                     * 否则会得到 `vec3(f)（pos)` 这样的错误展开。
                     */
                    const char *r = q;
                    while (r < end && (*r == ' ' || *r == '\t' ||
                                       *r == '\n' || *r == '\r')) r++;
                    if (r < end && *r == '(') {
                        int depth = 0;
                        const char *t = r;
                        while (t < end) {
                            if (*t == '(') depth++;
                            else if (*t == ')') {
                                depth--;
                                if (depth == 0) { t++; break; }
                            }
                            t++;
                        }
                        if (depth == 0) expr_end = t;
                    }
                    break;
                }
            }
        }

        /*
         * 【真机 Flywheel 第四例 —— 标量绝不可能带成员或分量访问】
         *
         * 真机报（shader=220，pipeline/instancing/…/embedded.vert）：
         *     ERROR: 10:2: 'constructor' : can't convert
         *     ERROR: 10:2: 'pose' : field selection requires structure,
         *                   vector, or matrix on left hand side
         *
         * 根因是转换器**自己**把源码改坏了。对照 native.log 的 RAW / 转换后：
         *     RAW : flw_vertexPos = i.pose * flw_vertexPos;
         *     转换: flw_vertexPos = float(i).pose * flw_vertexPos;
         *
         * 来龙去脉（与 bug ①②③ 同族，都是「同名跨作用域」污染）：
         *   - `struct FlwInstance { ... mat4x4 pose; ... }`，形参写作
         *       void flw_instanceVertex(in FlwInstance i)
         *     这里 `i` 是**结构体**，`i.pose` 完全合法。
         *   - 但同一文件另一处有一行 `int i = coord - start;`，
         *     collect_int_scalar_vars 按【名字】把 `i` 收进整数表（dim=1）。
         *   - 于是 `i.pose` 里的 `i` 命中整数标量表，matched_dim==1，
         *     再经情形 B 判定「整数标量赋给浮点向量」 -> 包上 float()。
         *   `float(i)` 是标量，对标量取 `.pose` 于 GLSL ES 非法。
         *
         * 【为什么这条判据是可靠的、且不会有副作用】
         *   GLSL 中**标量不可能有成员或分量访问**
         *   （`'scalar swizzle' : not supported with this profile: es`，
         *     桌面 GLSL 同样禁止对标量取 .xyz）。
         *   因此「整数标量表命中(dim==1) + 紧邻 '.'」必然意味着
         *   这个名字其实不是标量 —— 是同名冲突。
         *   放弃改写即可；真正的整数标量永远不会因为这个条件被漏改。
         *
         * 【为什么必须放在这里，而不是写进情形 B】
         *   matched_dim 是后续情形 A/B/C/E 共同的前置条件。
         *   在这里归零，四条情形一起失效，语义最清晰；
         *   只改情形 B 会让情形 A/E 仍可能误判。
         */
        if (matched_dim == 1) {
            const char *d = expr_end;
            while (d < end && (*d == ' ' || *d == '\t' ||
                               *d == '\n' || *d == '\r')) d++;
            if (d < end && *d == '.') matched_dim = 0;
        }

        /*
         * 【真机 Flywheel 第五例 —— 成员名与整数变量同名（上一条的镜像）】
         *
         * 真机报（片元 pipeline/instancing/frag/flywheel_material_default/
         *         flywheel_light_smooth_when_embedded_embedded.frag）：
         *     ERROR: 3:5: 'x' undeclared identifier
         *     ERROR: 3:5: '' : methods are not supported
         *     ERROR: 3:5: no matching overloaded function found
         *     ERROR: 3:5: no operation '*' exists ...
         *
         * 对照 native.log 的 RAW / 转换后：
         *     RAW : return min(n2.x + n2.y * (3. + normal.y) + n2.z, 1.);
         *     转换: return min(n2.float(x) + n2.y * (3. + normal.y) + n2.z, 1.);
         *
         * 转换器把**成员名** `x` 当成了变量，包成 `float(x)` ——
         * 而且包错了地方：变成 `n2.float(x)`，在语法层面就是非法的。
         * （`float(x)` 单独看也是错的：x 并不是变量。）
         *
         * 来龙去脉：同一个 Flywheel 装配着色器里有
         *     uint _flw_hash(in uint x) { ... }   // 形参 x
         *     uint x = _flw_hash(id);             // 局部变量 x
         * 收集器按【名字】把 `x` 收进整数标量表。于是 `n2.x` 里的 `x`
         * 命中该表，matched_dim==1，再经情形 B 判定「整数标量参与浮点运算」
         * 就包上了 float()。
         *
         * 【为什么这条判据可靠】
         *   GLSL 里 `.` 之后的标识符**必然是成员名或分量选择**
         *   （`DOT FIELD_SELECTION`），它永远不是一个变量引用。
         *   因此「整数标量表命中 + 紧邻其左是 '.'」必然意味着
         *   这个名字其实不是整数变量 —— 是同名污染。放弃改写即可。
         *   真正的整数变量不可能出现在点号右边，所以不会漏改。
         *
         *   GLSL 允许点号两侧有空白（`n2 . x` 合法），所以要跨过空白判断。
         */
        if (matched_dim > 0) {
            const char *b = p;
            while (b > src && (b[-1] == ' ' || b[-1] == '\t' ||
                               b[-1] == '\n' || b[-1] == '\r')) b--;
            if (b > src && b[-1] == '.') matched_dim = 0;
        }

        /*
         * 【使用点守卫（三）—— 按「本函数形参」的类型判定，而非按名字】
         *
         * 这是 Veil 兼容暴露的真实缺陷，也是前两道守卫（Rule O / Rule Q）
         * 无法覆盖的那一类：**同名形参在不同函数里换类型**。
         *
         * 实测证据（native/tools/fixtures/probe_int_name_scope_shadow.frag）：
         *     vec2 a(vec2  uv) { return uv / 256.0; }   // 浮点形参
         *     vec2 b(ivec2 uv) { return uv / 256.0; }   // 整数形参
         * 名字表把 `uv` 同时收进 fvec 与 ivec（实测 -DGLESMOD_DIAG：
         * fvec 里甚至有 `a`..`e` 这些函数名）。于是情形 C 拿 fvecs
         * 判定「左操作数是浮点」，把 b() 里 `uv + ivec2(1)` 改成
         *     `uv + vec2(ivec2(1))`   -> ivec2 + vec2
         * ES 报：
         *     '+' : no operation '+' exists that takes a left-hand operand of
         *           type 'in highp 2-component vector of int' and a right
         *           operand of type 'const 2-component vector of float'
         * 而桌面 GLSL 330 接受隐式转换 —— 属本转换器职责。
         *
         * 【为什么这是「纠正错误判定」而不是「收紧的取舍」】
         *   GLSL 里形参类型由所在函数唯一确定，不存在歧义。
         *   返回 2（本函数形参且为浮点族）时，该名字在此处**确实是浮点**，
         *   对它包 vecN()/float() 一律错误，必须放弃改写。
         *   返回 1 说明是整数族形参，维持改写（正确）。
         *   返回 0 说明不是本函数形参（全局变量 / 结构体成员 / 无法判定），
         *   交由既有启发式处理，行为与改动前一致。
         */
        if (matched_dim > 0) {
            int pcls = enclosing_fn_param_class(src, p, p, len);
            if (pcls == 2) matched_dim = 0;
        }

        /*
         * 【使用点守卫（四）—— 点号后不是合法分量名 => 该名字是结构体实例】
         *
         * 这是「歧义名剔除」解开的遮蔽所暴露出来的回归。真机代码：
         *     struct FlwLightAo { vec2 light; float ao; };
         *     FlwLightAo light;                       // 结构体实例
         *     flw_fragLight = max(flw_fragLight, light.light);
         *
         * `light` 同时出现在整数表（`uvec2 light = _flw_lightAt(...)`）与
         * 浮点表（结构体成员 `vec2 light;`）里，于是被歧义名剔除从浮点表移除；
         * 但整数表里**仍然保留**，情形 F 随即将它当作「浮点内建函数的整数实参」
         * 包成 `vec2(light)`，产出
         *     flw_fragLight = max(flw_fragLight, vec2(light).light);
         * 驱动报（真机 + glslang 一致）：
         *     'scalar swizzle' : not supported with this profile: es
         *     'light' : unknown swizzle selection
         *
         * 【判据的可靠性】GLSL 的分量选择只认 x/y/z/w 与 r/g/b/a。
         *   因此「紧跟 '.', 而其后的 token 含非分量字母」时，点号右侧
         *   **不可能**是 swizzle，只能是结构体成员名 ——
         *   那么这个基名一定不是向量（标量/向量都不可能有该形态的成员）。
         *   真正的整数向量只会写 `.xyzw` / `.rgba`，因此不会漏改。
         *   `.length()` 单独放行（它是向量的合法成员函数）。
         */
        if (matched_dim > 0) {
            const char *d = expr_end;
            while (d < end && (*d == ' ' || *d == '\t' ||
                               *d == '\n' || *d == '\r')) d++;
            if (d < end && *d == '.') {
                const char *m = d + 1;
                while (m < end && (*m == ' ' || *m == '\t')) m++;
                const char *me = m;
                while (me < end && is_ident_char(*me)) me++;
                size_t ml = (size_t)(me - m);
                int is_length = (ml == 6 && strncmp(m, "length", 6) == 0);
                if (ml > 0 && !is_length) {
                    int all_comp = 1;
                    for (const char *z = m; z < me; z++) {
                        char c = *z;
                        if (c != 'x' && c != 'y' && c != 'z' && c != 'w' &&
                            c != 'r' && c != 'g' && c != 'b' && c != 'a') {
                            all_comp = 0;
                            break;
                        }
                    }
                    if (!all_comp) matched_dim = 0;   /* 结构体成员，放弃改写 */
                }
            }
        }

        if (matched_dim > 0) {
            /* ---- 情形 A：expr 与浮点向量/字面量做 * 或 / ---- */
            const char *r = expr_end;
            while (r < end && (*r == ' ' || *r == '\t' ||
                               *r == '\n' || *r == '\r')) {
                r++;
            }
            if (r < end && (*r == '/' || *r == '*')) {
                const char *t = r + 1;
                while (t < end && (*t == ' ' || *t == '\t' ||
                                   *t == '\n' || *t == '\r')) {
                    t++;
                }
                /*
                 * 右操作数可以是浮点字面量（16.0）或浮点向量变量
                 * （Sodium: `* vec3(16.0)` 是构造函数，需再往前看一个标识符）。
                 * 只要确定右操作数处于浮点域，左操作数就要显式转 float。
                 */
                int rhs_float = starts_with_float_literal(t, end);
                if (!rhs_float) {
                    /* 识别 vecN(...) / 已声明的浮点向量变量 / 浮点标量变量 */
                    const char *u = t;
                    while (u < end && is_ident_char(*u)) u++;
                    size_t ul = (size_t)(u - t);
                    if (ul == 0) {
                        ;   /* 例如左括号，保守不动 */
                    } else if (is_float_vec_type(t, ul)) {
                        rhs_float = 1;
                    } else {
                        for (int i = 0; i < fvec_count; i++) {
                            if (strlen(fvecs[i].name) == ul &&
                                strncmp(t, fvecs[i].name, ul) == 0) {
                                rhs_float = 1;
                                break;
                            }
                        }
                        /*
                         * 浮点标量变量（如 VERTEX_SCALE）。
                         * 整数向量 * 浮点标量同样非法，必须显式转 float。
                         */
                        if (!rhs_float) {
                            for (int i = 0; i < fscalar_count; i++) {
                                if (strlen(fscalars[i].name) == ul &&
                                    strncmp(t, fscalars[i].name, ul) == 0) {
                                    rhs_float = 1;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (rhs_float) {
                    size_t elen = (size_t)(expr_end - expr_start);
                    char wrap[16];
                    if (matched_dim == 1) {
                        /* 标量：float(x) */
                        snprintf(wrap, sizeof(wrap), "float(");
                    } else {
                        snprintf(wrap, sizeof(wrap), "vec%d(", matched_dim);
                    }
                    sbuf_puts(out, wrap);
                    sbuf_put(out, expr_start, elen);
                    sbuf_puts(out, ")");
                    p = expr_end;
                    continue;
                }
            }

            /*
             * ---- 情形 E：整数向量的**分量**参与浮点运算 ----
             *
             * 【真机 BSL 上占错误的四成，是这一轮的主因】
             *   uniform ivec2 eyeBrightnessSmooth;
             *   float eBS = eyeBrightnessSmooth.y / 240.0;
             *   `.y` 的类型是 int，而 `int / const float` 在 GLSL ES 非法：
             *       '/' : no operation '/' exists that takes a left-hand
             *             operand of type 'int' and a right operand of type
             *             'const float'
             *   旧实现只登记「声明为 int 的变量名」，swizzle 出来的 int
             *   完全不在视野里，因此这一整类从未被改写。
             *
             *   改写为 float(...)，因为该分量在表达式里总是当浮点用。
             */
            {
                const char *ce = NULL;
                int cdim = int_component_expr(expr_start, end, ivecs,
                                              ivec_count, &ce);
                if (cdim > 0 && ce != NULL) {
                    int needs = 0;

                    /* (1) 后随算术运算符，且另一侧属于浮点域 */
                    const char *r = ce;
                    while (r < end && (*r == ' ' || *r == '\t' ||
                                       *r == '\n' || *r == '\r')) r++;
                    if (r < end && (*r == '/' || *r == '*' ||
                                    *r == '+' || *r == '-')) {
                        const char *t = r + 1;
                        while (t < end && (*t == ' ' || *t == '\t' ||
                                           *t == '\n' || *t == '\r')) t++;
                        if (starts_with_float_literal(t, end)) {
                            needs = 1;
                        } else {
                            const char *u = t;
                            while (u < end && is_ident_char(*u)) u++;
                            size_t ul = (size_t)(u - t);
                            if (ul > 0 && is_float_vec_type(t, ul)) needs = 1;
                            for (int i = 0; i < fscalar_count && !needs; i++) {
                                if (strlen(fscalars[i].name) == ul &&
                                    strncmp(t, fscalars[i].name, ul) == 0) needs = 1;
                            }
                            for (int i = 0; i < fvec_count && !needs; i++) {
                                if (strlen(fvecs[i].name) == ul &&
                                    strncmp(t, fvecs[i].name, ul) == 0) needs = 1;
                            }
                        }
                        /* 左侧是浮点字面量/浮点变量 */
                        if (!needs) {
                            const char *l = expr_start;
                            while (l > src && (l[-1] == ' ' || l[-1] == '\t' ||
                                               l[-1] == '\n' || l[-1] == '\r')) l--;
                            if (l > src && (isdigit((unsigned char)l[-1]) ||
                                            l[-1] == '.')) {
                                needs = 1;
                            } else if (l > src) {
                                const char *u = l;
                                while (u > src && is_ident_char(u[-1])) u--;
                                size_t ul = (size_t)(l - u);
                                for (int i = 0; i < fscalar_count && !needs; i++) {
                                    if (strlen(fscalars[i].name) == ul &&
                                        strncmp(u, fscalars[i].name, ul) == 0) needs = 1;
                                }
                                for (int i = 0; i < fvec_count && !needs; i++) {
                                    if (strlen(fvecs[i].name) == ul &&
                                        strncmp(u, fvecs[i].name, ul) == 0) needs = 1;
                                }
                            }
                        }
                    }

                    if (needs) {
                        sbuf_puts(out, "float(");
                        sbuf_put(out, expr_start, (size_t)(ce - expr_start));
                        sbuf_puts(out, ")");
                        p = ce;
                        continue;
                    }
                }
            }

            /* ---- 情形 B：赋值给浮点向量的右值 ---- */
            if (is_assign_rhs_to_float_vec(expr_start, src, fvecs, fvec_count)) {
                /*
                 * 【缺陷 A 的第二个修复点 —— 不要给已是浮点构造器实参的表达式再套一层】
                 *
                 * 形态：`vec2 screen_size = vec2(textureSize(tex, 0));`
                 *   - 外层 `vec2(...)` 已完成了 ivec2 -> vec2 的显式转换；
                 *   - 而且它是**浮点构造器实参**，而缺陷 A 的修复要求
                 *     这个位置的 `0` 保持 int。若这里再包一层，得到
                 *     `vec2(vec2(textureSize(tex, 0)))` —— 语义虽仍是 vec2，
                 *     但证明我们没理解「外层已经是显式转换」。
                 *   实测（native.log shader=101 行 16/42）确实产出过
                 *   `vec2 screen_size = vec2(textureSize(X, 0.0));`，
                 *   驱动报 'textureSize' : no matching overloaded function found。
                 *
                 * 放弃改写是安全的：外层构造器已经保证了类型正确。
                 */
                if (inside_float_ctor_args(expr_start, src)) {
                    sbuf_put(out, expr_start, (size_t)(expr_end - expr_start));
                    p = expr_end;
                    continue;
                }
                size_t elen = (size_t)(expr_end - expr_start);
                char wrap[16];
                if (matched_dim == 1) {
                    snprintf(wrap, sizeof(wrap), "float(");
                } else {
                    snprintf(wrap, sizeof(wrap), "vec%d(", matched_dim);
                }
                sbuf_puts(out, wrap);
                sbuf_put(out, expr_start, elen);
                sbuf_puts(out, ")");
                p = expr_end;
                continue;
            }

            /*
             * ---- 情形 C：整数操作数在【右侧】，左侧是浮点 ----
             *
             * 【为什么必须处理 —— BSL deferred1.fsh 上占错误的绝大多数】
             *   真机报了大量：
             *     ERROR: 0:188: '-' : no operation '-' exists that takes a
             *            left-hand operand of type 'float' and a right operand
             *            of type 'uniform int'
             *   源头是光影包的
             *     float nightMult = 1.0 - moonPhase;      // moonPhase 是 uniform int
             *     float b = 3.0 - worldTime;
             *   `float - int` 在桌面合法、ES 非法。
             *
             * 同时也覆盖比较运算符：`float < int`（`timeAngle < SHADOW_RES`）
             * 同样是 ES 非法而桌面合法。
             *
             * 【为什么限定左操作数必须是「已知的浮点变量/字面量」】
             *   若左操作数是整数（如 `uvec >> uvec` 的位运算），
             *   加 float() 反而制造错误。只有确认左侧属于浮点域才改写。
             */
            {
                const char *l = expr_start;
                while (l > src && (l[-1] == ' ' || l[-1] == '\t' ||
                                   l[-1] == '\n' || l[-1] == '\r')) {
                    l--;
                }
                int op_is_arith_or_cmp = 0;
                if (l > src) {
                    char c = l[-1];
                    if (c == '+' || c == '-' || c == '*' || c == '/' ||
                        c == '<' || c == '>') {
                        op_is_arith_or_cmp = 1;
                    }
                }
                /* 排除复合运算符（==, !=, <=, >=, +=, ...）与注释 */
                if (op_is_arith_or_cmp && l - 1 > src) {
                    char prev = (l - 1 > src) ? l[-2] : '\0';
                    if ((l[-1] == '<' && prev == '<') ||
                        (l[-1] == '>' && prev == '>') ||
                        (l[-1] == '<' && l[0] == '=') ||
                        (l[-1] == '>' && l[0] == '=') ||
                        (l[-1] == '<' && prev == '-') ||
                        (l[-1] == '<' && prev == '/')) {
                        op_is_arith_or_cmp = 0;
                    }
                }

                if (op_is_arith_or_cmp) {
                    const char *m = l - 1;
                    while (m > src && (m[-1] == ' ' || m[-1] == '\t' ||
                                       m[-1] == '\n' || m[-1] == '\r')) {
                        m--;
                    }
                    int lhs_float = 0;
                    if (m > src) {
                        /*
                         * 左操作数以浮点字面量结尾 -> 浮点。
                         *
                         * 【必须用 ends_with_float_literal，不能只看 m[-1]】
                         *   BSL 的浮点字面量全部带 f 后缀（`0.25f`），
                         *   旧写法 `isdigit(m[-1]) || m[-1] == '.'` 在 'f'
                         *   上失败，导致 `0.25f * isEyeInWater` 整类漏改。
                         *   详见 ends_with_float_literal 的说明。
                         */
                        if (ends_with_float_literal(m, src)) {
                            lhs_float = 1;
                        }
                    }
                    if (!lhs_float && m > src && m[-1] == ')') {
                        /*
                         * 左操作数以 ')' 结尾（函数调用/括号表达式）。
                         * 例：`(... + 0.5) / const int`、
                         *     `vec2(0.0, 1.0) / const int`
                         */
                        lhs_float = paren_expr_is_float(m, src, fvecs,
                                                        fvec_count,
                                                        fscalars,
                                                        fscalar_count);
                    }
                    if (!lhs_float && m > src) {
                        /*
                         * 左侧标识符，判定是否浮点。
                         * 必须跨过 `.xy` 这类 swizzle 取基名 —— 见
                         * lhs_base_ident 的说明（`shadowPos.xy * int`）。
                         *
                         * 【必须用 ident_is_float_operand 而不是直接查表】
                         *   名字表按【名字】收录，同一名字在不同函数的形参
                         *   里换类型时会互相污染。Veil 的 `vec2 uv` 与
                         *   `ivec2 uv` 就是实例：直接查表会把后者误判为浮点，
                         *   进而把 `uv + ivec2(1)` 改坏成
                         *   `uv + vec2(ivec2(1))`（ES：'+' 无匹配重载）。
                         *   助手优先按「本函数形参」判定，形参类型是确定事实。
                         */
                        size_t ul = 0;
                        const char *u = lhs_base_ident(m, src, &ul);
                        if (ul > 0) {
                            lhs_float = ident_is_float_operand(src, m, u, ul,
                                                               fscalars,
                                                               fscalar_count,
                                                               fvecs,
                                                               fvec_count);
                        }
                    }
                    if (lhs_float) {
                        size_t elen = (size_t)(expr_end - expr_start);
                        char wrap[16];
                        if (matched_dim == 1) {
                            snprintf(wrap, sizeof(wrap), "float(");
                        } else {
                            snprintf(wrap, sizeof(wrap), "vec%d(", matched_dim);
                        }
                        sbuf_puts(out, wrap);
                        sbuf_put(out, expr_start, elen);
                        sbuf_puts(out, ")");
                        p = expr_end;
                        continue;
                    }
                }
            }

            /*
             * ---- 情形 F：整数变量作为**浮点内建函数**的实参 ----
             *
             * 【真机 BSL 上最后一个真实错误】
             *     int samples = int(min(planeDifference / sampleLength, maxSamples) + 1);
             *     `maxSamples` 是宏展开出的 `const int 32`，而 `planeDifference`
             *     / `sampleLength` 是浮点。于是 `min(float, int)` 无匹配重载：
             *         'min' : no matching overloaded function found
             *         '+'   : no operation '+' exists that takes a left-hand
             *                 operand of type 'const float' and a right operand
             *                 of type 'const int'
             *     又把 1 改成 1.0。三处相互独立，必须分开处理。
             *
             * 判据：本 token 左侧（跳过空白与 '(' 与逗号）是某个
             * **只接受浮点的内建函数名**，且同一实参列表里已有浮点分量。
             * 满足则给该整数变量包上 float(...)。
             *
             * 这里刻意只处理一小批确定「参数类型一致」的内建函数，
             * 不做通用推断：`max(int, int)` 是合法的，误加 float() 会把
             * 合法代码改坏。宁可漏改也不能误改。
             */
            {
                static const char *const kFloatOnlyFns[] = {
                    "min", "max", "clamp", "mix", "step", "smoothstep",
                    "pow", "mod", "radians", "degrees", "distance", "length",
                    "dot", "normalize", "reflect", "refract", "faceforward",
                    "exp", "exp2", "log", "log2", "sqrt", "inversesqrt",
                    "abs", "sign", "floor", "ceil", "fract", "mod",
                    "sin", "cos", "tan", "asin", "acos", "atan",
                    "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
                    NULL
                };
                /*
                 * 【严格浮点专用 —— ES 里**完全没有**整数重载的函数】
                 *
                 * 上面那张表里的 min/max/clamp/abs/sign/floor/... 在 GLSL ES
                 * 里**同时有 int 与 float 两套重载**，所以只有在「实参列表中
                 * 已出现浮点」时才敢包 float()（否则会把合法的 int 调用改坏）。
                 *
                 * 但下面这批函数 ES 只定义了浮点版本，传 int 一定报
                 *     'exp2' : no matching overloaded function found
                 * 因此对它们**不需要**任何浮点证据，只要实参是整数变量就必须
                 * 显式转换。
                 *
                 * 【真机 Flywheel 证据】wavelet.glsl 里：
                 *     int power = TRANSPARENCY_WAVELET_RANK - i;
                 *     float wavelet_phase = ((index + 1) & 1) * exp2(-power);
                 *     float addend = fma(fma(-exp2(-power), k, depth),
                 *                        wavelet_sign, wavelet_phase) * exp2(float(power) * 0.5) * signal;
                 * `-power` 里的 power 是 int，`exp2` 无 int 重载，
                 * 真机报 exp2 / fma 均 no matching overloaded function found。
                 * （同一行里 `exp2(float(power) * 0.5)` 已被正确改写，
                 *   说明原机制能工作，只是漏了「实参前带一元负号」这一形态。）
                 */
                static const char *const kStrictFloatFns[] = {
                    "radians", "degrees",
                    "sin", "cos", "tan", "asin", "acos", "atan",
                    "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
                    "pow", "exp", "exp2", "log", "log2", "sqrt", "inversesqrt",
                    "normalize", "faceforward", "reflect", "refract",
                    "length", "distance", "dot", "cross",
                    "fma",
                    "determinant", "transpose", "inverse",
                    NULL
                };
                const char *l = expr_start;
                while (l > src && (l[-1] == ' ' || l[-1] == '\t' ||
                                   l[-1] == '\n' || l[-1] == '\r')) l--;
                /*
                 * 【一元负号 —— 本轮 Flywheel 暴露的缺口】
                 *   `exp2(-power)` 里 token 的紧邻左字符是 '-'，而不是
                 *   ',' 或 '('，于是本情形完全不进入。
                 *   `-` 若前面是 '(' 或 ','，则它是**一元**负号，
                 *   此时应把它一并跳过，继续按「实参」处理。
                 *   （若 `-` 前面是标识符/数字，那是二元减法，不处理。）
                 *   注意：只跳过多出来的负号本身，不改写它 ——
                 *   expr_start..expr_end 仍然只覆盖标识符本身，
                 *   于是产出 `-float(power)`，语义与括号无关、1:1 等价。
                 */
                if (l > src && l[-1] == '-') {
                    const char *b = l - 1;
                    while (b > src && (b[-1] == ' ' || b[-1] == '\t' ||
                                       b[-1] == '\n' || b[-1] == '\r')) b--;
                    if (b > src && (b[-1] == '(' || b[-1] == ',')) l = b;
                }
                /* 真正的相邻字符是 l[-1]（见下面那段「指针语义」注释） */
                if (l > src && (l[-1] == ',' || l[-1] == '(')) {
                    const char *fn_open = NULL;
                    if (l[-1] == '(') {
                        fn_open = l - 1;
                    } else {
                        /*
                         * 逗号：向前找到本次实参列表的 '('。
                         * 需要跳过嵌套的括号，并注意本实参自身可能含逗号
                         * （如 `f(a, min(b, c))` 里的内层逗号）。
                         */
                        const char *t = l - 1;
                        int d2 = 0;
                        while (t > src) {
                            if (t[-1] == ')') d2++;
                            else if (t[-1] == '(') {
                                if (d2 == 0) break;
                                d2--;
                            } else if (t[-1] == ',' && d2 == 0) {
                                break;      /* 上一个实参，说明不是第一个 */
                            }
                            t--;
                        }
                        if (t > src && t[-1] == '(') fn_open = t - 1;
                    }
                    if (fn_open != NULL) {
                        const char *u = fn_open;
                        while (u > src && (*u == ' ' || *u == '\t')) u--;
                        const char *we = u;
                        while (u > src && is_ident_char(u[-1])) u--;
                        size_t fl2 = (size_t)(we - u);
                        int is_float_fn = 0;
                        for (int i = 0;
                             i < (int)LIST_LEN(kFloatOnlyFns) &&
                             kFloatOnlyFns[i] != NULL; i++) {
                            if (strlen(kFloatOnlyFns[i]) == fl2 &&
                                strncmp(kFloatOnlyFns[i], u, fl2) == 0) {
                                is_float_fn = 1;
                                break;
                            }
                        }
                        int is_strict = 0;
                        for (int i = 0;
                             i < (int)LIST_LEN(kStrictFloatFns) &&
                             kStrictFloatFns[i] != NULL; i++) {
                            if (strlen(kStrictFloatFns[i]) == fl2 &&
                                strncmp(kStrictFloatFns[i], u, fl2) == 0) {
                                is_strict = 1;
                                break;
                            }
                        }
                        /*
                         * 严格浮点专用的函数**不需要**浮点证据：
                         * ES 里根本没有整数重载，传 int 必错。
                         * 其余（min/max/clamp/...）仍要求列表中已出现浮点，
                         * 否则 `max(a, b)`（a/b 都是 int）会被误改。
                         */
                        int do_wrap = is_strict;

                        if (is_float_fn || is_strict) {
                            /*
                             * 【使用点守卫（五）—— 本语句的声明才是事实】
                             *
                             * 真机 Flywheel 第三次同类故障：
                             *     Failed to compile
                             *       pipeline/instancing/create_instance_rotating/
                             *       flywheel_material_default_default.vert
                             *     error: no matching overloaded function found
                             *      --> flywheel:util/quaternion.glsl
                             *     21 |     vec3 i = q.xyz;
                             *     22 |     return v + 2.0 * cross(i, cross(i, v) + q.w * v);
                             *        |                      ^^^^^
                             * 对**我们自己的输出**做 diff，还原出的正是：
                             *     cross(float(i), cross(float(i), v) + q.w * v)
                             * `cross` 在 kStrictFloatFns 里（ES 无整数重载），
                             * 情形 F 因此**不需要浮点证据**就包了 float()。
                             * 但此处 `i` 是 `vec3 i = q.xyz;` —— 浮点向量！
                             * 它之所以被当成整数，是因为同一程序别处有一个
                             * `int i`，整数表按**名字**收录（同名污染）。
                             *
                             * 【为什么这条守卫是决定性的】
                             *   「本语句内紧邻声明的类型」是确定事实，优先级
                             *   高于任何按名字的全文件推断。真正的整数变量
                             *   不会写成 `vec3 i = ...`，所以不会漏改。
                             *   守卫放在**最终决策点**，is_strict 与
                             *   is_float_fn 两条路径一并覆盖。
                             */
                            if (do_wrap) {
                                /*
                                 * 走通用助手：按**所属函数内的声明**判定，
                                 * 而不是看紧邻 token（`cross(i, ...)` 里
                                 * `i` 前面是 '('，紧邻查法必然失败）。
                                 * 找到浮点族声明即明确不包；
                                 * 找到整数族或查不到则维持原行为。
                                 */
                                int cls = local_decl_class_in_fn(src, expr_start,
                                                                 expr_start, len);
                                if (cls == 2) {
                                    do_wrap = 0;   /* 本函数内声明为浮点族 */
                                }
                            }

                            /*
                             * is_float_fn 类（min/max/clamp/... 在 ES 里同时有
                             * int 与 float 重载）仍需「实参列表中已有浮点」作证据，
                             * 否则 `max(a, b)`（a/b 皆为 int）会被误改。
                             * is_strict 类（cross/exp2/...）ES 无整数重载，
                             * 前面已直接置 do_wrap = 1。
                             */
                            if (is_float_fn && is_strict == 0 && do_wrap == 0) {
                                const char *close = u;
                                int d2 = 0;
                                while (close < end) {
                                    if (*close == '(') { d2++; close++; continue; }
                                    if (*close == ')') { if (d2 > 0) d2--; close++; if (d2 == 0) break; continue; }
                                    close++;
                                }
                                int list_has_float = 0;
                                for (const char *q2 = u; q2 < close; q2++) {
                                    if (starts_with_float_literal(q2, close)) {
                                        list_has_float = 1;
                                        break;
                                    }
                                }
                                if (!list_has_float) {
                                    for (const char *q2 = u; q2 < close; q2++) {
                                        if (!is_ident_char(*q2)) continue;
                                        const char *q3 = q2;
                                        while (q3 < close && is_ident_char(*q3)) q3++;
                                        size_t nl3 = (size_t)(q3 - q2);
                                        for (int i = 0; i < fscalar_count; i++) {
                                            if (strlen(fscalars[i].name) == nl3 &&
                                                strncmp(q2, fscalars[i].name, nl3) == 0) {
                                                list_has_float = 1;
                                                break;
                                            }
                                        }
                                        for (int i = 0; i < fvec_count && !list_has_float; i++) {
                                            if (strlen(fvecs[i].name) == nl3 &&
                                                strncmp(q2, fvecs[i].name, nl3) == 0) {
                                                list_has_float = 1;
                                                break;
                                            }
                                        }
                                        if (list_has_float) break;
                                        q2 = q3 - 1;
                                    }
                                }
                                do_wrap = list_has_float;
                            }

                            if ((is_float_fn || is_strict) && do_wrap) {
                                size_t elen = (size_t)(expr_end - expr_start);
                                if (matched_dim == 1) {
                                    sbuf_puts(out, "float(");
                                } else {
                                    char wrap[16];
                                    snprintf(wrap, sizeof(wrap), "vec%d(",
                                             matched_dim);
                                    sbuf_puts(out, wrap);
                                }
                                sbuf_put(out, expr_start, elen);
                                sbuf_puts(out, ")");
                                p = expr_end;
                                continue;
                            }
                        }
                    }
                }
            }

            /*
             * ---- 情形 H：整数变量与**浮点**做比较或算术 ----
             *
             * 【整包验证暴露的真实缺口】
             *   BSL 自己的源码里就写着：
             *       uniform int isEyeInWater;
             *       if (isEyeInWater == 1.0) UnderwaterDistort(newTexCoord);
             *   也见 `intVar < 0.5`、`intVar >= 1.0` 等。
             *   桌面 GLSL 允许 int 与 float 混合运算（隐式转换），GLSL ES 不允许：
             *       '==' : no operation '==' exists that takes a left-hand
             *              operand of type 'int' and a right operand of type
             *              'const float'
             *   情形 C 只处理「整数在右侧、浮点在左侧」，方向相反的这一类
             *   一直没被覆盖。
             *
             * 【真机 BSL 007_basic.fsh 补充证据 —— 算术运算同样需要】
             *       for (int i = 0; i < 4; i++) {
             *           float dist = (i + gradNoise) / 12.0f;   // gradNoise 是 float
             *   驱动报：
             *       '+' : no operation '+' exists that takes a left-hand
             *             operand of type 'int' and a right operand of type 'float'
             *       '/' : ... int and const float
             *       '=' : cannot convert from 'int' to 'float'
             *   整条语句因第一个 int 未被浮点化而全崩。
             *
             * 只有当另一侧**确定是浮点**时才改写，避免把
             * `intVar == intVar2`、`intA + intB` 这类合法运算改坏。
             */
            {
                const char *r = expr_end;
                while (r < end && (*r == ' ' || *r == '\t' ||
                                   *r == '\n' || *r == '\r')) r++;
                if (r < end && (*r == '=' || *r == '!' ||
                                *r == '<' || *r == '>' ||
                                *r == '+' || *r == '-' ||
                                *r == '*' || *r == '/')) {
                    int is_cmp = 0, op_len = 1;
                    if (*r == '+' || *r == '-' || *r == '*' || *r == '/') {
                        /*
                         * 排除复合赋值 += -= *= /= 以及 ++ --
                         * （那两种写法里左侧是左值，不能包 float()）。
                         */
                        int compound = (r + 1 < end && r[1] == '=');
                        int incdec   = (r + 1 < end && r[1] == *r);
                        /*
                         * 排除注释起始（行注释与块注释），
                         * 否则下一行以注释开头的代码会被当成除法。
                         */
                        int comment  = (r + 1 < end &&
                                        (*r == '/' &&
                                         (r[1] == '/' || r[1] == '*')));
                        if (!compound && !incdec && !comment) {
                            is_cmp = 1;
                            op_len = 1;
                        }
                    } else if ((*r == '=' || *r == '!') &&
                               r + 1 < end && r[1] == '=') {
                        is_cmp = 1; op_len = 2;      /* == != */
                    } else if (*r == '<' || *r == '>') {
                        /* 排除移位 << >> 与赋值 <<= >>= */
                        if (!(r + 1 < end && ((r[1] == *r) || r[1] == '='))) {
                            is_cmp = 1; op_len = 1;  /* < > */
                        } else if (r + 1 < end && r[1] == '=') {
                            is_cmp = 1; op_len = 2;  /* <= >= */
                        }
                    }
                    if (is_cmp) {
                        const char *t = r + op_len;
                        while (t < end && (*t == ' ' || *t == '\t' ||
                                           *t == '\n' || *t == '\r')) t++;
                        int rhs_float = starts_with_float_literal(t, end);
                        if (!rhs_float && t < end && is_ident_char(*t)) {
                            const char *u = t;
                            while (u < end && is_ident_char(*u)) u++;
                            size_t ul = (size_t)(u - t);
                            for (int i = 0; i < fscalar_count && !rhs_float; i++) {
                                if (strlen(fscalars[i].name) == ul &&
                                    strncmp(t, fscalars[i].name, ul) == 0) {
                                    rhs_float = 1;
                                }
                            }
                            for (int i = 0; i < fvec_count && !rhs_float; i++) {
                                if (strlen(fvecs[i].name) == ul &&
                                    strncmp(t, fvecs[i].name, ul) == 0) {
                                    rhs_float = 1;
                                }
                            }
                        }
                        if (rhs_float) {
                            size_t elen = (size_t)(expr_end - expr_start);
                            char wrap[16];
                            if (matched_dim == 1) {
                                snprintf(wrap, sizeof(wrap), "float(");
                            } else {
                                snprintf(wrap, sizeof(wrap), "vec%d(",
                                         matched_dim);
                            }
                            sbuf_puts(out, wrap);
                            sbuf_put(out, expr_start, elen);
                            sbuf_puts(out, ")");
                            p = expr_end;
                            continue;
                        }
                    }
                }
            }

            /*
             * ---- 情形 D：整个右值就是单个整数常量，而左值是浮点声明 ----
             *
             *   float f2 = SHADOW_RES;   // SHADOW_RES 为 const int
             *   真机：ERROR: 0:444: '=' : cannot convert from 'const int' to 'float'
             *
             * 判据：本 token 之后（跳过空白）直到语句结束都没有其他内容，
             * 且语句等号左侧含 float/vecN 类型名。
             */
            {
                const char *r = expr_end;
                while (r < end && (*r == ' ' || *r == '\t' ||
                                   *r == '\n' || *r == '\r')) {
                    r++;
                }
                if (r < end && *r == ';') {
                    /* 找到本语句起点：向前回到上一个 ';' 或 '{' 或 '}' */
                    const char *st = expr_start;
                    while (st > src && st[-1] != ';' && st[-1] != '{' &&
                           st[-1] != '}' && st[-1] != '\n') {
                        st--;
                    }

                    /*
                     * 【必须先确认这是一个「赋值语句」 —— 真机回归的根因】
                     *
                     * 本情形处理的是 `vecN fv = ivecVar;` 这种「右值就是那个整数
                     * 变量、左值是浮点声明」。但它此前只检查「标识符后面是 ';'」，
                     * 于是把**声明语句**也当成了赋值：
                     *
                     *   struct FlwLightAo {
                     *       vec2 light;      <-- 结构体成员声明，没有 '='
                     *   };
                     *   ...
                     *   uvec2 light = _flw_lightAt(...);
                     *
                     * 语句起点到 `light` 之间确实有浮点向量类型名 `vec2`，
                     * 于是判定「左值是浮点向量」，把成员名包裹成
                     *       vec2 vec2(light);
                     * 驱动随即报（真机 latest.log 原文）：
                     *       error: Syntax error:  syntax error
                     *       --> flywheel:internal/api_impl.glsl
                     *       1 | struct FlwLightAo {
                     *       2 |     vec2 light;
                     *         |     ^^^^
                     * Flywheel 因此从 'flywheel:instancing' 掉到 'flywheel:off'。
                     *
                     * 修法：区间内必须存在一个**真正的赋值号**。
                     * 结构体成员/变量声明没有 '='，由此被正确排除。
                     * 同时排除复合与比较运算符，避免把 `a == b`、`a += b`
                     * 误当作赋值。
                     */
                    {
                        int has_assign = 0, d = 0;
                        for (const char *z = st; z < expr_start; z++) {
                            if (*z == '(' || *z == '[') { d++; continue; }
                            if (*z == ')' || *z == ']') { if (d > 0) d--; continue; }
                            if (*z != '=' || d != 0) continue;
                            if (z + 1 < expr_start && z[1] == '=') continue; /* == */
                            if (z > st && (z[-1] == '=' || z[-1] == '!' ||
                                           z[-1] == '<' || z[-1] == '>' ||
                                           z[-1] == '+' || z[-1] == '-' ||
                                           z[-1] == '*' || z[-1] == '/' ||
                                           z[-1] == '%')) {
                                continue;   /* == != <= >= += -= *= /= %= */
                            }
                            has_assign = 1;
                            break;
                        }
                        if (!has_assign) {
                            sbuf_put(out, p, len);
                            p = q;
                            continue;
                        }
                    }

                    int lhs_is_float = 0;
                    for (const char *r2 = st; r2 < expr_start; r2++) {
                        if (r2 + 5 <= expr_start &&
                            strncmp(r2, "float", 5) == 0 &&
                            (r2 == st || !is_ident_char(r2[-1])) &&
                            (r2 + 5 >= expr_start || !is_ident_char(r2[5]))) {
                            lhs_is_float = 1;
                            break;
                        }
                    }
                    if (!lhs_is_float) {
                        for (const char *r2 = st; r2 < expr_start; r2++) {
                            if (r2 > st && is_ident_char(r2[-1])) continue;
                            const char *r3 = r2;
                            while (r3 < expr_start && is_ident_char(*r3)) r3++;
                            if (r3 > r2 &&
                                is_float_vec_type(r2, (size_t)(r3 - r2))) {
                                lhs_is_float = 1;
                                break;
                            }
                        }
                    }
                    if (lhs_is_float) {
                        size_t elen = (size_t)(expr_end - expr_start);
                        char wrap[16];
                        if (matched_dim == 1) {
                            snprintf(wrap, sizeof(wrap), "float(");
                        } else {
                            snprintf(wrap, sizeof(wrap), "vec%d(", matched_dim);
                        }
                        sbuf_puts(out, wrap);
                        sbuf_put(out, expr_start, elen);
                        sbuf_puts(out, ")");
                        p = expr_end;
                        continue;
                    }
                }
            }
        }

        sbuf_put(out, p, len);
        p = q;
    }
}

/*
 * 遍历整个源码，按语句边界（; { }）分段处理。
 *
 * 预处理器行（含 #line / #define / #version 等）整行原样复制，
 * 绝不改写其中的字面量 —— 那些是编译器指令而非表达式，
 * 改写它们必然产出非法指令。
 */
static void normalize_int_literals(sbuf *out, const char *src) {
    const char *p = src;

    /*
     * 先收集全源码里声明为 int/uint 的标识符。
     *
     * 【为什么必须先扫一遍全源码，而不是逐语句判断】
     *   `float(i + 1)` 里的 `i` 声明在别处（循环头）。只有拿到
     *   全文件的整数变量表，才能判断 `i + 1` 是整数表达式，
     *   从而不去浮点化其中的 `1`。见 int_ident_table 的说明。
     */
    int_ident_table int_idents;
    int_idents.count = 0;
    int_idents.saturated = 0;
    scan_int_idents(src, &int_idents);

    /*
     * 规则 P（fix_mixed_int_uint）需要同一份源码的「整数标量表」。
     * 它是独立的一趟扫描，因此在这里把结果公开出去（只读）。
     *
     * 【必须用 scan_scalar_int_idents 而不是上面那张表】
     *   规则 P 只能给**标量**包 uint()。若表里混入整数向量
     *   （`in uvec2 a_TexCoord;`），`a_TexCoord & TEXTURE_MAX_VALUE`
     *   会被判成「int 标量 & uint」而改写成 `uint(a_TexCoord) & ...`。
     *   那在 ES 下合法但等价于取第 0 分量，贴图坐标 (u,v) 塌成 (u,u)，
     *   静态方块材质全错且没有任何报错 —— 2026-10-05 真机回归的根因。
     *   规则 B' 仍然使用含向量的 int_idents（它判的是「语句里有没有
     *   整数变量」，向量必须算数），两者诉求不同，故各用一张表。
     */
    scan_scalar_int_idents(src, &g_src_int_idents);
    g_src_int_ready  = 1;

    /* 规则 J 需要整源区间做正向类型确认（见 g_src_begin 的说明） */
    g_src_begin = src;
    g_src_end = src + strlen(src);

    /*
     * 阴影采样器名字表（用于删除 shadow2D 结果上的多余 `.x`）。
     * ES 的 texture(sampler2DShadow, vec3) 返回 float，
     * 对 float 取 .x 非法；桌面返回 vec4 因此包内写法带 .x/.r。
     */
    collect_shadow_sampler_names(src);

    /*
     * 规则 M 需要知道哪些名字是 #define 出来的宏，
     * 这样才能把宏调用处的整数字面量保护起来（见 in_macro_arg）。
     */
    collect_macro_names(src);

    /*
     * 规则 N 需要「哪些名字是标量 uint」这张表。
     * 必须在这里（逐语句处理之前）建好，且整趟转换过程中只读。
     */
    collect_uint_vars(src, &g_uint_idents);
    g_uint_idents_ready = 1;

    /*
     * 规则 R 需要「用户自定义函数的形参类型」这张表。
     * 同样在处理之前建好，整趟转换只读。
     */
    collect_user_fn_sigs(src, &g_fn_sigs);
    g_fn_sigs_ready = 1;
    if (g_fn_sigs.count > 0) {
        /*
         * 一次性诊断（每个着色器最多一行，且只在表非空时输出）。
         *
         * 双重用途：
         *   1. 真机排查时能确认规则 R 确实读取到了用户函数的签名 ——
         *      若某着色器整型实参仍被浮点化，看这一行在不在即可判断
         *      「是签名没识别到」还是「签名识别到了但判定有误」。
         *   2. 充当**产物内容标记**：字符串只存在于规则 R 的代码路径里，
         *      required_strings.txt 因此能证明构建产物包含本次修复
         *      （见该文件开头关于「陈旧产物」的说明）。
         */
        glesmod_log("规则R 已建立用户函数签名表（缺陷2：用户函数整型实参"
                    "不再被浮点化；GLESMOD_RULE_R_SIGNATURES）");
    }

    /*
     * ===== 规则 L（三元配平）在函数末尾作为「后置一趟」执行 =====
     *
     * 【真机证据】BSL hand_cutout.vsh（原文件）写的是：
     *       isMainHand = float(gl_Position.x * (isRightHanded ? 1 : -1) > 0.0f);
     *   两侧**本来都是整数**，是**合法的**。但转换器逐字面量处理时，
     *   右操作数 `-1` 左侧是一元负号 `-`，命中规则 B（算术操作数）
     *   被浮点化成 `-1.0`；而 `1` 两侧是 `?` 和 `:`，既非算术也不是
     *   vecN 实参，**没有任何规则会碰它**，于是输出变成：
     *       isRightHanded ? 1 : -1.0
     *   —— 转换器【自己制造】了三元两侧类型不一致，驱动随即报：
     *       ':' : no operation ':' exists that takes a left-hand operand of
     *             type 'const int' and a right operand of type 'const float'
     *
     * 【为什么必须「后置」而不是「前置」】
     *   前置（在其它规则之前看原文）时两侧都是 int，判定「无需配平」，
     *   然后后续规则才把一侧浮点化 —— 配平被绕过。
     *   只有等所有逐字面量规则跑完、看到**最终文本**再做一次配平，
     *   才能真正消除这种「被我方改出来」的类型不一致。
     */
    p = src;

    while (*p != '\0') {
        /*
         * 预处理器行：从行首（含前导空白）到最后的本行内容，原样复制。
         * 用行首判断而非当前字符判断，见 line_starts_with_directive 的说明。
         */
        {
            const char *eol = strchr(p, '\n');
            const char *line_end = eol ? eol : p + strlen(p);
            if (line_starts_with_directive(p, line_end)) {
                size_t n = (size_t)(line_end - p) + (eol ? 1 : 0);
                sbuf_put(out, p, n);
                p += n;
                continue;
            }
        }

        /* 找到本条语句的结束位置（; { }） */
        const char *end = p;
        while (*end != '\0' && *end != ';' && *end != '{' && *end != '}') {
            end++;
        }

        process_stmt(out, p, end, &int_idents);

        /* 原样输出分隔符 */
        if (*end != '\0') {
            sbuf_put(out, end, 1);
            p = end + 1;
        } else {
            p = end;
        }
    }

    /*
     * ===== 规则 L 后置执行：三元运算符两侧类型配平 =====
     *
     * 必须在所有逐字面量规则**之后**做，原因见函数开头关于规则 L 的说明：
     * 本转换器自己会把 `-1` 浮点化成 `-1.0`，从而把合法的
     * `cond ? 1 : -1` 改坏成 `cond ? 1 : -1.0`。
     * 只有看到最终文本才能发现并修正这种「自伤」。
     */
    if (!out->failed) {
        sbuf balanced;
        sbuf_init(&balanced, INITIAL_CAP);
        if (!balanced.failed) {
            balance_ternary_branches(&balanced, out->buf);
            if (!balanced.failed) {
                free(out->buf);
                out->buf = balanced.buf;
                out->len = balanced.len;
                out->cap = balanced.cap;
            } else {
                free(balanced.buf);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* #moj_import 展开                                                    */
/* ------------------------------------------------------------------ */

/*
 * 【这是真机 "could not preload shader position" 的最终根因】
 *
 * Minecraft 的核心着色器大量使用它自己的预处理指令：
 *     #moj_import <fog.glsl>
 * 而 **GLSL 从未支持 #moj_import**。若把它原样送给驱动，ES 编译器直接报错：
 *     ERROR: 0:4: '' : GLSL compile error: malformed preprocessor directive
 * 这正是真机上 position 着色器无法编译、进而
 *     IllegalStateException: could not preload shader position
 * 的原因。
 *
 * 曾长期误以为「MC 会在调用 glShaderSource 之前展开它」。实测否定了这个假设：
 * 驱动报的错误行号（0:4）与 position.vsh 中 #moj_import 所在的输出行号
 * 完全吻合，说明它是【原样】送到驱动的。
 *
 * 因此本库必须自己完成展开。
 *
 * 【为什么不从磁盘读取 include 文件】
 *   游戏资源打包在 jar / 资源包里，native 层没有可靠的路径去定位它们，
 *   而且用户在设置里切换资源包后路径就会变。
 *   这些 include 的内容是 Minecraft 自己的着色器工具函数（很短的数学函数），
 *   内容稳定、体量极小，因此直接内嵌一份等价实现最可靠。
 *
 * 【内嵌版本必须自身就是合法 GLSL ES】
 *   MC 的原始写法面向桌面 GLSL，个别地方在 ES 下不合法，已就地修正并注明，
 *   否则内联后仍会编译失败。例如 light.glsl 里 `uv / 256.0`（uv 为 ivec2）
 *   在 ES 中不允许隐式 int->float 转换，改为 `vec2(uv) / 256.0`。
 */

/*
 * fog.glsl —— MC 的雾计算工具函数。
 * 与 MC 原版逐字一致（去掉其开头的 #version 150），本身已是合法 ES 语法。
 */
static const char *const MOJ_FOG_GLSL =
"vec4 linear_fog(vec4 inColor, float vertexDistance, float fogStart, float fogEnd, vec4 fogColor) {\n"
"    if (vertexDistance <= fogStart) {\n"
"        return inColor;\n"
"    }\n"
"\n"
"    float fogValue = vertexDistance < fogEnd ? smoothstep(fogStart, fogEnd, vertexDistance) : 1.0;\n"
"    return vec4(mix(inColor.rgb, fogColor.rgb, fogValue * fogColor.a), inColor.a);\n"
"}\n"
"\n"
"float linear_fog_fade(float vertexDistance, float fogStart, float fogEnd) {\n"
"    if (vertexDistance <= fogStart) {\n"
"        return 1.0;\n"
"    } else if (vertexDistance >= fogEnd) {\n"
"        return 0.0;\n"
"    }\n"
"\n"
"    return smoothstep(fogEnd, fogStart, vertexDistance);\n"
"}\n"
"\n"
"float fog_distance(vec3 pos, int shape) {\n"
"    if (shape == 0) {\n"
"        return length(pos);\n"
"    } else {\n"
"        float distXZ = length(pos.xz);\n"
"        float distY = abs(pos.y);\n"
"        return max(distXZ, distY);\n"
"    }\n"
"}\n";

/*
 * light.glsl —— MC 的光照工具函数。
 *
 * 【与 MC 原版的唯一差异】
 *   MC 原文（`minecraft_sample_lightmap` 内）：
 *       clamp(uv / 256.0, ...)      // uv 是 ivec2
 *   桌面 GLSL 允许 ivec2 与 float 混算，GLSL ES 不允许。
 *   改为 vec2(uv) / 256.0，语义完全等价。
 */
static const char *const MOJ_LIGHT_GLSL =
"#define MINECRAFT_LIGHT_POWER   (0.6)\n"
"#define MINECRAFT_AMBIENT_LIGHT (0.4)\n"
"\n"
"vec4 minecraft_mix_light(vec3 lightDir0, vec3 lightDir1, vec3 normal, vec4 color) {\n"
"    float light0 = max(0.0, dot(lightDir0, normal));\n"
"    float light1 = max(0.0, dot(lightDir1, normal));\n"
"    float lightAccum = min(1.0, (light0 + light1) * MINECRAFT_LIGHT_POWER + MINECRAFT_AMBIENT_LIGHT);\n"
"    return vec4(color.rgb * lightAccum, color.a);\n"
"}\n"
"\n"
"vec4 minecraft_sample_lightmap(sampler2D lightMap, ivec2 uv) {\n"
"    return texture(lightMap, clamp(vec2(uv) / 256.0, vec2(0.5 / 16.0), vec2(15.5 / 16.0)));\n"
"}\n";

/* matrix.glsl —— MC 的 2D 旋转矩阵工具函数（与 MC 原版一致） */
static const char *const MOJ_MATRIX_GLSL =
"mat2 mat2_rotate_z(float radians) {\n"
"    return mat2(\n"
"        cos(radians), -sin(radians),\n"
"        sin(radians), cos(radians)\n"
"    );\n"
"}\n";

/* projection.glsl —— MC 的投影坐标变换工具函数（与 MC 原版一致） */
static const char *const MOJ_PROJECTION_GLSL =
"vec4 projection_from_position(vec4 position) {\n"
"    vec4 projection = position * 0.5;\n"
"    projection.xy = vec2(projection.x + projection.w, projection.y + projection.w);\n"
"    projection.zw = position.zw;\n"
"    return projection;\n"
"}\n";

typedef struct {
    const char *name;   /* include 名（不含尖括号/引号） */
    const char *body;   /* 等价实现（不含 #version） */
} moj_include;

static const moj_include MOJ_INCLUDES[] = {
    { "fog.glsl",        MOJ_FOG_GLSL },
    { "light.glsl",      MOJ_LIGHT_GLSL },
    { "matrix.glsl",     MOJ_MATRIX_GLSL },
    { "projection.glsl", MOJ_PROJECTION_GLSL },
};

static const char *lookup_moj_include(const char *name) {
    for (size_t i = 0; i < sizeof(MOJ_INCLUDES) / sizeof(MOJ_INCLUDES[0]); i++) {
        if (strcmp(MOJ_INCLUDES[i].name, name) == 0) {
            return MOJ_INCLUDES[i].body;
        }
    }
    return NULL;
}

/*
 * 展开源码中的所有 #moj_import 指令。
 *
 * 支持两种写法（MC 两种都用）：
 *     #moj_import <fog.glsl>
 *     #moj_import "fog.glsl"
 *
 * 无法解析的 include 会被替换为一条注释，并记录降级事件 ——
 * 宁可让后续因「未定义函数」而报出清晰错误，也不要留下
 * #moj_import 这种会让驱动吐 "malformed preprocessor directive" 的指令。
 */
static void expand_moj_imports(sbuf *out, const char *src) {
    static const char KEYWORD[] = "#moj_import";
    const size_t KW_LEN = sizeof(KEYWORD) - 1;

    const char *p = src;
    while (*p != '\0') {
        const char *eol = strchr(p, '\n');
        size_t line_len = eol ? (size_t)(eol - p + 1) : strlen(p);

        /* 跳过前导空白后判断是否为 #moj_import 行 */
        const char *q = p;
        const char *line_end = p + line_len;
        while (q < line_end && (*q == ' ' || *q == '\t')) q++;

        if ((size_t)(line_end - q) > KW_LEN &&
            strncmp(q, KEYWORD, KW_LEN) == 0) {
            const char *r = q + KW_LEN;
            while (r < line_end && (*r == ' ' || *r == '\t')) r++;

            /* 取 <> 或 "" 内的名字 */
            char name[128];
            size_t n = 0;
            char closer = '\0';
            if (r < line_end && *r == '<') { closer = '>'; r++; }
            else if (r < line_end && *r == '"') { closer = '"'; r++; }

            if (closer != '\0') {
                while (r < line_end && *r != closer && n + 1 < sizeof(name)) {
                    name[n++] = *r++;
                }
                name[n] = '\0';

                if (r < line_end && *r == closer) {
                    const char *body = lookup_moj_include(name);
                    if (body != NULL) {
                        sbuf_puts(out, body);
                        sbuf_puts(out, "\n");
                    } else {
                        /*
                         * 未知 include：留一条注释占位。
                         *
                         * 注释在预处理前就被替换为空白，因此不会触发任何
                         * 编译器诊断，比留下 #moj_import 好得多；若因此出现
                         * 未定义函数，驱动会给出明确的位置，而不是含混的
                         * "malformed preprocessor directive"。
                         *
                         * 注释文本刻意不写出 #moj_import 这个字面量：
                         * 避免任何驱动在注释处理上不严谨时又被误判为指令，
                         * 也让「输出中不含该指令」成为可断言的不变量。
                         */
                        sbuf_puts(out, "/* glesmod: 未解析的着色器 include: ");
                        sbuf_puts(out, name);
                        sbuf_puts(out, "（不在内嵌清单中） */\n");

                        char detail[192];
                        snprintf(detail, sizeof(detail),
                                 "无法解析着色器 include <%s>，相关功能可能缺失",
                                 name);
                        glesmod_degrade(GLESMOD_DEGRADE_SHADER_UNSUPPORTED_FEATURE,
                                        detail);
                    }
                    p += line_len;
                    continue;
                }
            }
        }

        sbuf_put(out, p, line_len);
        p += line_len;
    }
}

/* ------------------------------------------------------------------ */
/* 版本声明处理                                                        */
/* ------------------------------------------------------------------ */

/*
 * 提取并移除源码中【所有】#version 行的内容。
 * 返回第一处出现的版本号（如 150），无声明时返回 0。
 *
 * 【必须移除全部，而不是只移除第一处 —— 这是真机启动失败的根因】
 *
 *   GLSL 规范要求 #version 只能出现在程序中「第一个非注释、非空白」的位置。
 *   出现第二处 #version 是**编译错误**：
 *       #version directive must occur before anything else in the program
 *
 *   而 Minecraft 编译前的 #moj_import 展开会把 include 文件的内容
 *   原样内联进来，其中包括它们自己的 #version 行。实测的 fog.glsl 开头就是
 *   `#version 150`，于是 MC 交给 glShaderSource 的源码是：
 *
 *       #version 150
 *
 *       #version 150        <- 从 fog.glsl 内联进来的，位于文件中部！
 *
 *       vec4 linear_fog(...) { ... }
 *       ...
 *
 *   只移除第一处会导致输出里残留文件中部的那一条：
 *
 *       #version 320 es     <- 我们添加的
 *       precision highp float;
 *       ...
 *       #version 150        <- 残留，非法位置 -> 编译失败
 *
 *   真机表现即为此：Minecraft 的 position 着色器无法编译，
 *   报 "could not preload shader position"。而 MC 的核心着色器几乎都
 *   通过 #moj_import 引入 fog.glsl，因此这类失败是大面积的。
 *
 *   移除多余 #version 是安全且正确的：我们已经在文件最前面放了
 *   一份等价的 #version 320 es，语义由它决定。
 */
static int extract_version(const char *src, sbuf *body) {
    const char *p = src;
    int version = 0;

    while (*p != '\0') {
        const char *eol = strchr(p, '\n');
        size_t line_len = eol ? (size_t)(eol - p + 1) : strlen(p);

        /* 判断本行是否为 #version 行（允许前导空白） */
        const char *q = p;
        while (*q == ' ' || *q == '\t') q++;
        if (strncmp(q, "#version", 8) == 0) {
            /* 记录第一个出现的版本号，供诊断使用 */
            if (version == 0) {
                version = (int)strtol(q + 8, NULL, 10);
            }
            /* 跳过该行（不写入 body） */
            p += line_len;
            continue;
        }

        sbuf_put(body, p, line_len);
        p += line_len;
    }
    return version;
}

/* ------------------------------------------------------------------ */
/* 非恒定全局初始化器（non-constant global initializer）               */
/* ------------------------------------------------------------------ */

/*
 * 【为什么必须重写它 —— 真机证据，且决定了光影能否加载】
 *
 * 2026-10-01 真机（Iris 1.8.14-beta.1 + BSL，Adreno 750 / ES 3.2）：
 *     ShaderCompileException: deferred1.vsh:
 *       ERROR: 0:17: 'iris_FogColor' : Only consts can be used in a global initializer
 *       ERROR: 0:17: 'iris_FogDensity' : Only consts can be used in a global initializer
 *       ERROR: 0:17: 'iris_FogStart' : ...
 *       ERROR: 0:17: 'iris_FogEnd' : ...
 *
 * 出问题的那一行是 **Iris 自己注入的**（不是光影包的，也不是我们的），
 * 来自其 CommonTransformer / VanillaTransformer / VanillaCoreTransformer：
 *     iris_FogParameters irisInt_Fog = iris_FogParameters(
 *         iris_FogColor, iris_FogDensity, iris_FogStart, iris_FogEnd,
 *         1.0 / (iris_FogEnd - iris_FogStart));
 * 右侧引用 uniform，属于「非常量初始化器」。桌面 GLSL 允许，
 * **GLSL ES 不允许**。
 *
 * 【为什么不能靠「加一个扩展声明」解决】
 *   glslang 给出的诊断是：
 *       needs GL_EXT_shader_non_constant_global_initializers
 *   即存在一个专门扩展。但**我们不会替驱动声明它**，理由：
 *     1. 真机回传的全部日志里，该扩展名**零命中** —— 没有证据表明
 *        Adreno 实现并报告了它。
 *     2. 报出该错误的正是 Adreno 自己的编译前端，说明它**正在执行**
 *        这条规则；由我们谎称支持，只会把「明确报错」变成
 *        「行为未定义」。本项目已确立原则：能力声明必须如实。
 *   => 采用**版本与驱动都无关**的改写，不依赖任何扩展。
 *
 * 【改写方式（语义等价）】
 *   全局声明去掉初始化器，再把赋值语句插到**每个函数体的开头**。
 *   插到每个函数而不是只插 main()，是因为该变量可能在任意函数里被读取；
 *   每个函数进入时都重新赋同样的值，因此无论从哪个入口调用都正确，
 *   且函数间互相调用时也不会读到未赋值的状态。
 */
typedef struct {
    char name[128];   /* 变量名 */
    char init[512];   /* 初始化表达式（不含分号） */
    size_t decl_offset; /* 该声明在源码中的偏移，用于保证只注入其后出现的函数 */
} global_init_mover;

/*
 * 【必须与 NAME_SET_MAX 同量级 —— 这是本项目第四次「容量写小」缺陷】
 *
 * 原值为 32。BSL 的 020_terrain_translucent.fsh 经 Iris 预处理后有 1099 行、
 * 139 个全局名，其中需要搬移的**非常量全局初始化器远超 32 个**。
 * 超出部分被 `if (eligible && movers->count < 32)` 静默丢弃 ——
 * 声明仍被剥掉了初始化器（那一步在计数之前，无条件执行），
 * 但赋值语句没被注入任何函数体，于是：
 *     float fogDensity = 1.0f * mix(..., weatherWeight);
 *   变成裸声明 `float fogDensity;`，驱动报
 *     'fogDensity' : Only consts can be used in a global initializer
 *   而同一文件里排在前面、落在 32 个名额内的变量却能正常工作，
 *   因此表现为「同一个文件里部分变量报错、部分不报」，极难定位。
 *
 * 扩容到 256（与 name_set 一致），并新增 saturated 标志
 * 让超限**出声**（见 name_set 的同类说明）。
 */
typedef struct {
    global_init_mover item[256];
    int count;
    int saturated;   /* 超限时置 1；调用方据此上报降级事件 */
} global_init_movers;

/*
 * 名字表容量。
 *
 * 【为什么从 64 提到 256 —— 真机 BSL 证据】
 *   BSL 的 deferred1.fsh 经 Iris 预处理后有 **693 行**，全局变量上百个
 *   （lightColSqrt / minLightColSqrt / skyColSqrt / fogColSqrt ...）。
 *   旧容量 64 会在读到一半时悄悄停止登记（name_set_add 直接 return），
 *   于是后半段所有变量的初始化器都判定不出「非常量」，驱动报：
 *       'non-constant global initializer'  ×20+
 *   这类「容量静默截断」缺陷没有任何症状，只能靠真机日志发现。
 *   MC 自带着色器规模小，所以 125 个着色器的回归门禁完全没有暴露它。
 *
 * 【教训】容量类缺陷必须让它在超限时**出声**，否则等于把排查成本
 *   推给下一次真机测试。下文 saturated 标志即为此设。
 */
#define NAME_SET_MAX 256

typedef struct {
    char name[NAME_SET_MAX][56];
    int count;
    int saturated;   /* 超限时置 1，调用方据此上报降级事件 */
} name_set;

static int name_set_has(const name_set *s, const char *name, size_t len) {
    for (int i = 0; i < s->count; i++) {
        if (strlen(s->name[i]) == len &&
            strncmp(s->name[i], name, len) == 0) {
            return 1;
        }
    }
    return 0;
}

static void name_set_add(name_set *s, const char *p, size_t len) {
    if (len == 0 || len >= sizeof(s->name[0])) return;
    if (s->count >= NAME_SET_MAX) {
        s->saturated = 1;
        return;
    }
    /* 已在集合里则不重复添加 */
    for (int i = 0; i < s->count; i++) {
        if (strlen(s->name[i]) == len && strncmp(s->name[i], p, len) == 0) return;
    }
    memcpy(s->name[s->count], p, len);
    s->name[s->count][len] = '\0';
    s->count++;
}

/*
 * 收集一条声明里**所有**声明符名字。
 *
 * 【为什么不能只取最后一个名字】真机 BSL 证据（deferred1.fsh，0:53 起）：
 *     varying vec3 sunVec, upVec, eastVec;
 *     uniform float timeAngle, timeBrightness;
 *     uniform mat4  gbufferProjection, gbufferPreviousProjection, ...;
 * 旧实现只取分号前最后一个标识符，于是 sunVec / upVec / timeAngle 全部漏掉。
 * 引用它们的全局初始化器因此判定不出「非常量」，驱动报：
 *     'sunVec' : Only consts can be used in a global initializer
 *
 * 【为什么不能把下标数字当名字】旧实现还漏判了数组长度：
 *     vec2 aoSampleOffsets[4] = vec2[4]( ... );
 * 收集到的是 "4"（数字也是标识符字符），随后初始化器里的 `vec2[4]`
 * 又撞上这个名字，整句被误判、改写成 `4 = vec2[4](...);`，
 * 数组名与类型一起丢掉，属于数据损坏级缺陷。
 *
 * 本函数按声明语法取名字：
 *     限定符*  类型名  声明符 (',' 声明符)* ';'
 *     声明符 := 标识符 ( '[' 常量表达式 ']' )* ( '=' 初始化器 )?
 * 括号、方括号、花括号内的内容一律跳过，因此数组长度、初始化器、
 * struct 成员列表都不会被当成声明符。
 */
static const char *const k_decl_qualifiers[] = {
    "uniform", "const", "in", "out", "inout", "attribute", "varying",
    "flat", "smooth", "noperspective", "centroid", "invariant", "precise",
    "lowp", "mediump", "highp", "patch", "sample", "struct", "readonly",
    "writeonly", "coherent", "volatile", "restrict", "shared", "buffer",
    "layout", NULL
};

/* 跳过平衡的括号组，返回 ')' 之后的位置；p 必须指向 '(' */
static const char *skip_paren_group(const char *p, const char *end) {
    int d = 0;
    while (p < end) {
        if (*p == '(') d++;
        else if (*p == ')') { d--; if (d == 0) return p + 1; }
        p++;
    }
    return end;
}

/* 跳过平衡的花括号组，返回 '}' 之后的位置；p 必须指向 '{' */
static const char *skip_brace_group(const char *p, const char *end) {
    int d = 0;
    while (p < end) {
        if (*p == '{') d++;
        else if (*p == '}') { d--; if (d == 0) return p + 1; }
        p++;
    }
    return end;
}

static void collect_declarator_names(const char *begin, const char *end,
                                     name_set *out) {
    const char *p = begin;
    int have_type = 0;

    /* 跳过前导空白与注释 */
    for (;;) {
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
        if (p + 1 < end && p[0] == '/' && p[1] == '/') {
            const char *e = strchr(p, '\n');
            p = (e != NULL && e < end) ? e : end;
            continue;
        }
        if (p + 1 < end && p[0] == '/' && p[1] == '*') {
            const char *e = strstr(p + 2, "*/");
            p = (e != NULL && e + 2 <= end) ? e + 2 : end;
            continue;
        }
        break;
    }

    /*
     * 首标识符若紧跟 '=' / '(' / '[' / '.'，说明这是一条**表达式语句**
     * （如 `eBS = eyeBrightnessSmooth.y / 240.0;`），不是声明。
     * 此时若继续解析，会把类型名位置错认成 `eBS`、把右值里的
     * 标识符误收成「全局变量名」。
     */
    if (p < end && is_ident_char(*p)) {
        const char *b = p;
        while (p < end && is_ident_char(*p)) p++;
        (void)b;
        const char *t = p;
        while (t < end && (*t == ' ' || *t == '\t')) t++;
        if (t < end && (*t == '=' || *t == '(' || *t == '[' || *t == '.')) {
            return;
        }
        p = b;
    }

    while (p < end) {
        if (is_ident_char(*p)) {
            const char *b = p;
            while (p < end && is_ident_char(*p)) p++;
            size_t len = (size_t)(p - b);

            if (!have_type) {
                int is_q = 0;
                for (int i = 0; i < (int)LIST_LEN(k_decl_qualifiers) && k_decl_qualifiers[i] != NULL; i++) {
                    if (strlen(k_decl_qualifiers[i]) == len &&
                        strncmp(k_decl_qualifiers[i], b, len) == 0) {
                        is_q = 1;
                        /* layout(std140) 这类带参数的限定符，整组跳过 */
                        if (len == 6 && strncmp(b, "layout", 6) == 0) {
                            const char *t = p;
                            while (t < end && (*t == ' ' || *t == '\t')) t++;
                            if (t < end && *t == '(') p = skip_paren_group(t, end);
                        }
                        break;
                    }
                }
                if (!is_q) have_type = 1;   /* 这就是类型名 */
                continue;
            }

            /* 声明符名字 */
            name_set_add(out, b, len);

            /* 跳过该声明符的数组维度、初始化器、struct 成员块 */
            int depth = 0;
            while (p < end) {
                if (*p == '[' || *p == '(') { depth++; p++; continue; }
                if (*p == ']' || *p == ')') { if (depth > 0) depth--; p++; continue; }
                if (depth == 0) {
                    if (*p == '=') {
                        /* 跳到本声明符后的顶层 ',' 或语句结束 */
                        p++;
                        int d2 = 0;
                        while (p < end) {
                            if (*p == '(' || *p == '[' || *p == '{') { d2++; p++; continue; }
                            if (*p == ')' || *p == ']' || *p == '}') { if (d2 > 0) d2--; p++; continue; }
                            if (d2 == 0 && *p == ',') break;
                            if (d2 == 0 && *p == ';') break;
                            p++;
                        }
                        break;
                    }
                    if (*p == ',') { p++; break; }        /* 下一个声明符 */
                    if (*p == ';') return;
                    if (*p == '{') { p = skip_brace_group(p, end); continue; }
                }
                p++;
            }
            continue;
        }
        if (*p == '{') { p = skip_brace_group(p, end); continue; }
        p++;
    }
}

/*
 * 收集 `} instanceName;` 形式（接口块尾部）里的名字。
 * 这段文本没有类型前缀，所以不能用 collect_declarator_names：
 * 它会把唯一的名字当成类型名而一个都不收。
 */
static void collect_block_tail_names(const char *begin, const char *end,
                                     name_set *out) {
    const char *p = begin;
    int depth = 0;
    while (p < end) {
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
        if (p + 1 < end && p[0] == '/' && p[1] == '/') {
            const char *e = strchr(p, '\n');
            p = (e != NULL && e < end) ? e : end;
            continue;
        }
        if (p + 1 < end && p[0] == '/' && p[1] == '*') {
            const char *e = strstr(p + 2, "*/");
            p = (e != NULL && e + 2 <= end) ? e + 2 : end;
            continue;
        }
        if (*p == '[') { depth++; p++; continue; }
        if (*p == ']') { if (depth > 0) depth--; p++; continue; }
        if (*p == '(') { p = skip_paren_group(p, end); continue; }
        if (is_ident_char(*p) && depth == 0) {
            const char *b = p;
            while (p < end && is_ident_char(*p)) p++;
            /* 纯数字（数组长度）不是名字 */
            int all_digit = 1;
            for (const char *r = b; r < p; r++) {
                if (!isdigit((unsigned char)*r)) { all_digit = 0; break; }
            }
            if (!all_digit) name_set_add(out, b, (size_t)(p - b));
            continue;
        }
        p++;
    }
}

/*
 * 扫描源码，收集所有 `uniform ...` / `in ...` / `attribute ...` /
 * `varying ...` 形式的变量名（含逗号分隔的多个声明符）。
 *
 * 只处理**行首**出现的这些限定符：着色器里的 uniform 与顶点输入
 * 一定写在全局作用域的独立行上，据此可以避免把函数体里的
 * `in`/`out` 之类误收进来。
 */
static void collect_uniform_names(const char *src, name_set *out) {
    const char *p = src;
    while (*p != '\0') {
        const char *eol = strchr(p, '\n');
        size_t line_len = eol ? (size_t)(eol - p + 1) : strlen(p);
        const char *line_end = p + line_len;

        const char *q = p;
        while (q < line_end && (*q == ' ' || *q == '\t')) q++;

        size_t rest = (size_t)(line_end - q);
        int hit = 0;
        if (rest > 7 && strncmp(q, "uniform", 7) == 0) hit = 1;
        else if (rest > 7 && strncmp(q, "varying", 7) == 0) hit = 1;
        else if (rest > 9 && strncmp(q, "attribute", 9) == 0) hit = 1;
        else if (rest > 2 && strncmp(q, "in", 2) == 0 &&
                 (q[2] == ' ' || q[2] == '\t')) hit = 1;

        if (hit) collect_declarator_names(q, line_end, out);

        p += line_len;
    }
}

/*
 * 判断一段初始化表达式是否「非常量」。
 *
 * 两类来源都算：
 *   1. 引用 uniform / in 变量 —— Iris 注入的 irisInt_Fog 属于此类
 *   2. 引用**普通全局变量** —— 光影包自己常这么写，真机 BSL 的
 *      `float lightMorning = 1.0 - timeAngle;` 属于此类，报
 *      Only consts can be used in a global initializer
 *
 * globals 为已收集的全局变量名表；is_global 用单独的标记数组区分
 * 「该名字确实是全局声明」而不是同名的局部标识符。
 */
typedef struct {
    name_set names;
    /*
     * 【必须是 NAME_SET_MAX，不能是 64 —— 这是真机 020_terrain_translucent.fsh
     *  的失败根因，也是本项目第三次「容量写小」缺陷】
     *
     *   names.count 由 collect_declarator_names 累加，最大值是 NAME_SET_MAX(256)；
     *   而本数组原先只有 64 项。global_names_has() 按
     *       for (i = 0; i < g->names.count; i++)
     *           if (g->is_global[i] && ...)
     *   逐项读取，于是当全局声明数超过 64 时**越界读取相邻内存**，
     *   返回 0 或 1 完全取决于当时的栈/结构体布局。
     *
     *   实测三个真实着色器的全局声明数：
     *       020_terrain_translucent.fsh  116   <- 失败
     *       007_basic.fsh                 89   <- 恰好没踩中
     *       001_deferred1.fsh             94   <- 恰好没踩中
     *   因此表现为「同样的写法，有的文件能过、有的不过」，极难定位。
     *
     *   具体后果：terrain_translucent 的
     *       float fogDensity = 1.0f * mix(..., weatherWeight);
     *   依赖全局 weatherWeight，而 global_names_has("weatherWeight")
     *   因越界读到 0 -> 被判定为「常量初始化器」-> 不剥离 -> 驱动报
     *       'weatherWeight' : Only consts can be used in a global initializer
     *
     *   教训（与 name_set 的 saturated 同类）：容量类缺陷必须让数组大小
     *   跟着实际上限走，绝不能用「看起来够用」的小值。
     */
    unsigned char is_global[NAME_SET_MAX];
} global_names;

static int global_names_has(const global_names *g, const char *b, size_t len) {
    for (int i = 0; i < g->names.count; i++) {
        if (g->is_global[i] &&
            strlen(g->names.name[i]) == len &&
            strncmp(g->names.name[i], b, len) == 0) {
            return 1;
        }
    }
    return 0;
}

static int init_is_non_constant(const char *begin, const char *end,
                                const name_set *names,
                                const global_names *globals) {
    const char *p = begin;
    while (p < end) {
        if (is_ident_char(*p)) {
            const char *b = p;
            while (p < end && is_ident_char(*p)) p++;
            size_t len = (size_t)(p - b);
            if (name_set_has(names, b, len)) return 1;
            if (globals != NULL && global_names_has(globals, b, len)) return 1;
        } else {
            p++;
        }
    }
    return 0;
}

/*
 * 在一条全局语句里定位「初始化赋值号」。
 *
 * 【为什么不能简单找第一个 '='】三次真机失败的教训：
 *
 *   1. `layout (location = 0) out vec4 outColor0;`
 *      Iris 的 CommonTransformer 会把光影包的 `gl_FragData[N]`
 *      改写成这种带 location 的 out 声明（模板就是
 *      "layout (location = __index) out vec4 __name;"）。
 *      这里的 '=' 属于 layout 限定符，**永远不可能是初始化器**。
 *      上一版按「第一个 '='」处理，把该行改写成 `layout (location;`，
 *      驱动遂报：
 *          ERROR: 0:<n>: 'location' : not a legal layout qualifier id
 *          ERROR: 0:<n>: ';' : Syntax error:  syntax error
 *      正是真机日志中 deferred1.fsh 的报错。
 *
 *   2. 注释里的 '=' / ';' 会劫持语句切分。夹具注释里写了一行
 *      `layout (location = 0) out vec4 outColor0;` 作为说明，
 *      结果整段注释被当成语句改写，输出错乱。注释文本不是语法。
 *
 *   3. `float f = (a == b) ? 1.0 : 2.0;` 这类初始化器含比较符。
 *
 * 因此本函数：跳过注释与字符串；只认**圆括号深度为 0** 的 '='；
 * 排除 == != <= >= 以及复合赋值。找不到返回 NULL，调用方原样写出。
 */
static const char *find_init_eq(const char *stmt, const char *stmt_end) {
    int pdepth = 0;
    const char *q = stmt;

    while (q < stmt_end) {
        /* 行注释：整段跳过（其中的 '=' ';' 都不是语法） */
        if (q[0] == '/' && q + 1 < stmt_end && q[1] == '/') {
            const char *e = strchr(q, '\n');
            q = (e != NULL && e < stmt_end) ? e : stmt_end;
            continue;
        }
        /* 块注释：整段跳过 */
        if (q[0] == '/' && q + 1 < stmt_end && q[1] == '*') {
            const char *e = strstr(q + 2, "*/");
            q = (e != NULL && e + 2 <= stmt_end) ? e + 2 : stmt_end;
            continue;
        }
        /* 字符串字面量 */
        if (q[0] == '"') {
            q++;
            while (q < stmt_end && *q != '"') {
                if (*q == '\\' && q + 1 < stmt_end) q++;
                q++;
            }
            if (q < stmt_end) q++;
            continue;
        }

        if (*q == '(') { pdepth++; q++; continue; }
        if (*q == ')') { if (pdepth > 0) pdepth--; q++; continue; }

        /* 括号内的 '=' 一律不是初始化器（layout 限定符、宏参数等） */
        if (*q != '=' || pdepth > 0) { q++; continue; }

        /* 排除 == */
        if (q + 1 < stmt_end && q[1] == '=') { q += 2; continue; }

        /* 排除 <= >= != 与复合赋值 += -= *= /= %= &= |= ^= */
        if (q > stmt) {
            char c = q[-1];
            if (c == '=' || c == '!' || c == '<' || c == '>' ||
                c == '+' || c == '-' || c == '*' || c == '/' ||
                c == '%' || c == '&' || c == '|' || c == '^') {
                q++;
                continue;
            }
        }
        return q;
    }
    return NULL;
}

/*
 * 找出全局作用域里带「非常量初始化器」的声明，并写出「去掉初始化器」的源码。
 *
 * 【实现要点：默认原样输出】
 *   本函数必须把输入的**每一个字节**都写到 out（除了被改写的初始化器部分），
 *   否则会静默丢代码。上一版只在「命中特例」时才输出，结果转换结果里
 *   只剩 #version 行 —— 被主机端测试当场抓住。
 *   现在的结构是：逐语句处理，**先无条件输出，再考虑是否改写**，
 *   这样「不改写」与「改写」走同一条写出路径，不可能漏。
 *
 * 【语句边界 —— 两个曾经致命的地方】
 *   本函数只处理**全局作用域**的语句，且候选语句**绝不能跨越花括号**。
 *   第一版有两个缺陷，导致把整个函数体当作一条「全局语句」，
 *   删掉了从 '=' 到 ';' 之间的大量源码，花括号因此失衡，报出：
 *       error: 'uniform' : not allowed in nested scope
 *       error: 'fogValue' : undeclared identifier
 *   两个缺陷分别是：
 *     1. 遇到 '{' / '}' 时不重置语句起点，于是累加跨度跨过了整个函数；
 *     2. 搜索 '=' 时未考虑花括号深度，于是函数体内的赋值被当成
 *        「全局声明的初始化器」。
 *   修法：**遇到任何花括号都把语句起点重置到其后**。
 *   这样候选语句天然不可能包含 '{' 或 '}'，两个缺陷同时消失。
 */
static void rewrite_nonconst_globals(sbuf *out, const char *src,
                                     global_init_movers *movers) {
    name_set uniforms;
    uniforms.count = 0;
    collect_uniform_names(src, &uniforms);

    /*
     * 全局变量名集合。
     *
     * 【为什么必须收集它们】光影包会写
     *     vec3 upVec = normalize(upPosition);
     *     float lightMorning = 1.0 - timeAngle;   // timeAngle 也是全局
     * 第二行的初始化器引用的是**普通全局变量**而非 uniform，
     * 若不把它认定为「非常量」，这一行会原样留在全局作用域，
     * 驱动仍然报 Only consts can be used in a global initializer。
     *
     * 判定条件：在花括号深度 0 处、以类型名开头、不含 '(' 的声明语句
     * （即不是函数原型/定义）。
     */
    global_names globals;
    globals.names.count = 0;
    memset(globals.is_global, 0, sizeof(globals.is_global));
    {
        int d2 = 0;
        const char *q = src;
        const char *s2 = src;
        /*
         * 【为什么不再需要 last_brace】接口块实例名改用「从 ';' 反向
         * 精确识别 `} name;` 形态」的局部判据，不再依赖任何跨语句的状态，
         * 因此这个变量已无用途（编译器会给出 set-but-not-used 警告）。
         */

        while (*q != '\0') {
            if (q[0] == '/' && q[1] == '/') {
                const char *e = strchr(q, '\n');
                q = e ? e : q + strlen(q);
                continue;
            }
            if (q[0] == '/' && q[1] == '*') {
                const char *e = strstr(q + 2, "*/");
                q = e ? e + 2 : q + strlen(q);
                continue;
            }
            if (*q == '{') {
                d2++;
                q++;
                s2 = q;
                continue;
            }
            if (*q == '}') {
                if (d2 > 0) d2--;
                q++;
                s2 = q;
                continue;
            }
            if (*q == ';' && d2 == 0) {
                const char *se = q;

                /*
                 * 接口块实例名：`layout(...) uniform B { ... } iris_fogP;`
                 *
                 * 【判定方式：从 ';' 反向精确识别，不依赖任何状态跟踪】
                 *   早先试过「记住最近的 '}' 位置」的做法，但那个位置会在
                 *   下一条普通语句处过期，于是要么漏收（条件写反），
                 *   要么把两条语句之间的大段文本误当名字，两种都是回归。
                 *
                 *   可靠的判据是局部语法形态：从 ';' 往前跳过空白，
                 *   若读到一串标识符（可带逗号分隔）并且**紧接其前的正是
                 *   '}'**，那这段标识符就只能是接口块的实例名。
                 *   普通声明（`float eBS;`）前面不是 '}', 因此天然不会误收。
                 */
                {
                    const char *e2 = se;
                    while (e2 > src && (e2[-1] == ' ' || e2[-1] == '\t' ||
                                        e2[-1] == '\n' || e2[-1] == '\r')) e2--;
                    if (e2 > src && is_ident_char(e2[-1])) {
                        /*
                         * 反向跳过「标识符(,标识符)*」，允许方括号（数组实例名）。
                         */
                        const char *t = e2;
                        int ok = 1;
                        int guard = 0;
                        while (t > src && guard++ < 512) {
                            char c = t[-1];
                            if (is_ident_char(c) || c == ']' || c == '[' ||
                                c == ',' || c == ' ') {
                                t--;
                                continue;
                            }
                            break;
                        }
                        if (t > src && t[-1] == '}') ok = 1; else ok = 0;
                        if (ok) {
                            collect_block_tail_names(t, e2, &globals.names);
                            for (int i = 0; i < globals.names.count; i++) {
                                globals.is_global[i] = 1;
                            }
                        }
                    }
                }

                /*
                 * 全局作用域的声明。collect_declarator_names 自己会：
                 *   - 跳过 uniform / const / layout 等限定符
                 *   - 跳过注释
                 *   - 排除表达式语句（首标识符后紧跟 '=' '(' '[' '.'）
                 *   - 收集**全部**逗号分隔的声明符名
                 *   - 忽略数组长度与初始化器内容
                 */
                collect_declarator_names(s2, se, &globals.names);
                for (int i = 0; i < globals.names.count; i++) {
                    globals.is_global[i] = 1;
                }

                s2 = q + 1;
                q++;
                continue;
            }
            q++;
        }
    }

    int depth = 0;
    const char *stmt = src;      /* 当前全局语句起点 */
    const char *p = src;

    while (*p != '\0') {
        /* 跳过注释与字符串（其中的 ';' '=' 不算语法） */
        if (p[0] == '/' && p[1] == '/') {
            const char *e = strchr(p, '\n');
            p = e ? e : p + strlen(p);
            continue;
        }
        if (p[0] == '/' && p[1] == '*') {
            const char *e = strstr(p + 2, "*/");
            p = e ? e + 2 : p + strlen(p);
            continue;
        }
        if (p[0] == '"') {
            p++;
            while (*p != '\0' && *p != '"') {
                if (*p == '\\' && p[1] != '\0') p++;
                p++;
            }
            if (*p == '"') p++;
            continue;
        }

        if (*p == '{') {
            depth++;
            /* 关键：重置语句起点，使候选语句不可能跨越花括号 */
            sbuf_put(out, stmt, (size_t)(p + 1 - stmt));
            stmt = p + 1;
            p++;
            continue;
        }
        if (*p == '}') {
            if (depth > 0) depth--;
            sbuf_put(out, stmt, (size_t)(p + 1 - stmt));
            stmt = p + 1;
            p++;
            continue;
        }

        if (*p == ';' && depth == 0) {
            const char *stmt_end = p;          /* 指向 ';' */
            size_t stmt_len = (size_t)(stmt_end - stmt) + 1;

            /*
             * ===== 特例 U：剥离 uniform 的初始化器 =====
             *
             * 【真机证据 —— 这是压倒性的一类错误】
             *   Iris 导出的 patched_shaders/001_deferred1.vsh 里就有：
             *       uniform bool  heavyFog = false;
             *       uniform float cloudHeight = 192.0f;
             *   真机 Adreno 对每个这种行都报：
             *       'uniform' : cannot initialize this type of qualifier
             *   ComplementaryReimagined 的 deferred1.vsh 一次报了 3 条；
             *   之后 70 个错误里的绝大多数（return 类型不匹配、
             *   min/clamp 无匹配重载、构造实参不足、vec 维度不匹配）
             *   都是**这一条的连锁后果**：
             *   uniform 初始化失败 -> 该 uniform 的类型/值不可用 ->
             *   依赖它的每个函数返回值都推不出来。
             *   也就是说：**修好这一条能一次性消掉几十个错误。**
             *
             * 【为什么桌面没事】
             *   桌面 GLSL 规范同样不允许 uniform 初始化，但多数桌面驱动
             *   宽容地忽略它。GLSL ES 规范与 Adreno 都严格拒绝。
             *
             * 【改写方式】直接删掉 `= ...` 部分，保留 `uniform <type> <name>`。
             *   uniform 的值由 CPU 侧通过 glUniform* 设置，
             *   代码里那个初始值本来就不生效，删掉**语义完全不变**。
             *
             * 【为什么不能复用后面的“搬移”机制】
             *   搬移机制会把赋值插进每个函数体 —— 那对 uniform 是**错的**，
             *   因为 uniform 是只读的，不能被赋值。
             *   所以必须在这里单独处理，只删不搬。
             */
            {
                const char *sc = stmt;
                while (sc < stmt_end && (*sc == ' ' || *sc == '\t' ||
                                         *sc == '\n' || *sc == '\r')) sc++;
                if (strncmp(sc, "uniform", 7) == 0) {
                    const char *u = find_init_eq(stmt, stmt_end);
                    if (u != NULL) {
                        /* 输出到赋值号前，去掉尾部空白，补分号 */
                        const char *de = u;
                        while (de > stmt && (de[-1] == ' ' || de[-1] == '\t' ||
                                             de[-1] == '\n' || de[-1] == '\r')) {
                            de--;
                        }
                        sbuf_put(out, stmt, (size_t)(de - stmt));
                        sbuf_puts(out, ";");

                        char note[224];
                        snprintf(note, sizeof(note),
                                 "已剥离 uniform 的初始化器（GLSL ES 禁止 "
                                 "`uniform T x = v;`，桌面驱动多容忍；"
                                 "该初始值本就不生效，删除后语义不变）");
                        glesmod_degrade(GLESMOD_DEGRADE_ENUM_MAPPED, note);

                        stmt = stmt_end + 1;
                        p++;
                        continue;
                    }
                }
            }

            /*
             * 在本语句内找**初始化**赋值号。
             * 不能按「第一个 '='」处理：layout 限定符里的 '='、注释里的 '='、
             * 字符串里的 '=' 都不是初始化器。详见 find_init_eq 的说明。
             */
            const char *eq = find_init_eq(stmt, stmt_end);

            /*
             * 候选条件：有赋值号、初始化器引用 uniform/全局变量、
             * 且不是 uniform/const/预处理器/函数定义。
             */
            int eligible = (eq != NULL);
            if (eligible) {
                /*
                 * 语句开头若是 layout 限定符，则这是接口限定声明
                 * （Iris 注入的 `layout (location = N) out vec4 outColorN;`），
                 * 不存在初始化器，必须原样保留。
                 */
                const char *sc0 = stmt;
                while (sc0 < stmt_end && (*sc0 == ' ' || *sc0 == '\t' ||
                                          *sc0 == '\n' || *sc0 == '\r' ||
                                          *sc0 == '/')) {
                    if (sc0[0] == '/' && sc0 + 1 < stmt_end && sc0[1] == '*') {
                        const char *e2 = strstr(sc0 + 2, "*/");
                        sc0 = (e2 != NULL && e2 + 2 <= stmt_end) ? e2 + 2 : stmt_end;
                        continue;
                    }
                    if (sc0[0] == '/' && sc0 + 1 < stmt_end && sc0[1] == '/') {
                        const char *e2 = strchr(sc0, '\n');
                        sc0 = (e2 != NULL && e2 < stmt_end) ? e2 : stmt_end;
                        continue;
                    }
                    sc0++;
                }
                if (strncmp(sc0, "layout", 6) == 0) eligible = 0;
            }
            if (eligible) {
                /*
                 * 语句里出现 preprocessor 行，说明与 #ifdef 等混在一起，保守跳过。
                 * 判定必须跳过注释——注释里出现 '#' 是常事。
                 */
                const char *hash = NULL;
                for (const char *q = stmt; q < stmt_end; ) {
                    if (q[0] == '/' && q + 1 < stmt_end && q[1] == '/') {
                        const char *e2 = strchr(q, '\n');
                        q = (e2 != NULL && e2 < stmt_end) ? e2 : stmt_end;
                        continue;
                    }
                    if (q[0] == '/' && q + 1 < stmt_end && q[1] == '*') {
                        const char *e2 = strstr(q + 2, "*/");
                        q = (e2 != NULL && e2 + 2 <= stmt_end) ? e2 + 2 : stmt_end;
                        continue;
                    }
                    if (*q == '#') { hash = q; break; }
                    q++;
                }
                if (hash != NULL) eligible = 0;
            }
            if (eligible) {
                /* 语句开头是否就是 uniform / const 等限定符 */
                const char *sc = stmt;
                while (sc < stmt_end && (*sc == ' ' || *sc == '\t' ||
                                         *sc == '\n' || *sc == '\r')) sc++;
                /* 跳过注释后重取首词 */
                while (sc < stmt_end && sc[0] == '/' && sc + 1 < stmt_end) {
                    if (sc[1] == '*') {
                        const char *e2 = strstr(sc + 2, "*/");
                        sc = (e2 != NULL && e2 + 2 <= stmt_end) ? e2 + 2 : stmt_end;
                    } else if (sc[1] == '/') {
                        const char *e2 = strchr(sc, '\n');
                        sc = (e2 != NULL && e2 < stmt_end) ? e2 : stmt_end;
                    } else {
                        break;
                    }
                    while (sc < stmt_end && (*sc == ' ' || *sc == '\t' ||
                                             *sc == '\n' || *sc == '\r')) sc++;
                }
                if (strncmp(sc, "uniform", 7) == 0) eligible = 0;
                else if (strncmp(sc, "const", 5) == 0) eligible = 0;
                else if (strncmp(sc, "in ", 3) == 0) eligible = 0;
                else if (strncmp(sc, "out ", 4) == 0) eligible = 0;
                else if (strncmp(sc, "layout", 6) == 0) eligible = 0;
            }
            if (eligible) {
                /*
                 * 赋值号左侧若含 '('，说明这是函数调用/构造而非声明
                 * （如 `vec3 a = b, c(1.0);` 不存在，但保守排除）。
                 */
                for (const char *q = stmt; q < eq; q++) {
                    if (*q == '(') { eligible = 0; break; }
                }
            }
            if (eligible) {
                eligible = init_is_non_constant(eq + 1, stmt_end, &uniforms,
                                               &globals);
            }

            if (eligible && movers->count < (int)(sizeof(movers->item) /
                                                 sizeof(movers->item[0]))) {
                /* 变量名 = 赋值号左侧最后一个标识符 */
                const char *last = NULL;
                size_t last_len = 0;
                for (const char *q = stmt; q < eq; ) {
                    if (is_ident_char(*q)) {
                        const char *b = q;
                        while (q < eq && is_ident_char(*q)) q++;
                        last = b;
                        last_len = (size_t)(q - b);
                    } else {
                        q++;
                    }
                }

                if (last != NULL && last_len > 0 && last_len < sizeof(movers->item[0].name)) {
                    global_init_mover *m = &movers->item[movers->count];
                    memcpy(m->name, last, last_len);
                    m->name[last_len] = '\0';

                    const char *ib = eq + 1;
                    while (ib < stmt_end && (*ib == ' ' || *ib == '\t' ||
                                             *ib == '\n' || *ib == '\r')) ib++;
                    size_t in_len = (size_t)(stmt_end - ib);
                    if (in_len >= sizeof(m->init)) in_len = sizeof(m->init) - 1;
                    memcpy(m->init, ib, in_len);
                    while (in_len > 0 && (m->init[in_len - 1] == ' ' ||
                                          m->init[in_len - 1] == '\t' ||
                                          m->init[in_len - 1] == '\n' ||
                                          m->init[in_len - 1] == '\r')) {
                        in_len--;
                    }
                    m->init[in_len] = '\0';

                    /*
                     * decl_offset 必须记录在**输出缓冲区**（即 stripped）中的位置，
                     * 因为 inject_global_assignments 里的 brace_off 也是在
                     * stripped 上测量的。
                     *
                     * 【曾经写错的地方】上一版写成 (stmt - src)，那是输入缓冲区
                     * （replaced）里的偏移。而本函数会删掉前面若干个初始化器，
                     * 使 stripped 比 replaced 短，于是同一条声明在 stripped 中的
                     * 真实位置要小得多。结果 decl_offset 偏大，注入时被判为
                     * 「声明在函数之后」而跳过 —— 变量被剥离了初始化器却
                     * 从未赋值。真机夹具证据（bsl_like_deferred.frag）：
                     *     float f1;   // 只在 main 里补回，helper 里漏掉
                     *     float f3;   // 哪里都没补回 ← 永久未初始化
                     * 两个不同数值的坐标系混用，是这类缺陷的典型来源。
                     */
                    m->decl_offset = out->len;
                    movers->count++;

                    /*
                     * 写出声明部分：从语句起点到赋值号，再去掉尾部空白，
                     * 补一个分号。类型与变量名因此完整保留，只是没有初始化器。
                     * （数组并非本问题的关注点：等号前含 '[' 时不参与。）
                     */
                    const char *decl_end = eq;
                    while (decl_end > stmt && (decl_end[-1] == ' ' ||
                                               decl_end[-1] == '\t')) {
                        decl_end--;
                    }
                    sbuf_put(out, stmt, (size_t)(decl_end - stmt));
                    sbuf_puts(out, ";");

                    stmt = stmt_end + 1;
                    p++;
                    continue;
                }
            }

            /* 默认路径：原样输出整条语句 */
            sbuf_put(out, stmt, stmt_len);
            (void)stmt_len;
            stmt = stmt_end + 1;
            p++;
            continue;
        }

        p++;
    }

    /* 收尾：剩余内容（最后一个 ';' 之后的部分，通常是函数体） */
    if (*stmt != '\0') {
        sbuf_puts(out, stmt);
    }
}

/*
 * 在每个**函数体**的开头插入复原赋值。
 *
 * 【两个必须守住的约束 —— 都由主机端测试当场抓到了错误版本】
 *
 * 1. **只注入函数体，不注入其它花括号**
 *    第一版对每个 '{' 都插入，于是 if / for / 结构体定义也会被插入，
 *    语句被从中间切断，出现：
 *        error: 'fogValue' : undeclared identifier
 *        error: 'leng' : undeclared identifier
 *    判别方法：函数体的 '{' 前面（跳过空白）必定是 ')'；
 *    结构体定义的 '{' 前面是标识符。只对前者注入。
 *
 * 2. **只注入声明位置在它之前的变量**
 *    若某全局变量声明在函数之后，却在更早的函数开头被赋值，
 *    会报 'undeclared identifier'。用 decl_offset 与 '{' 的偏移比较即可排除。
 *
 * 【实现要点：默认原样输出】除插入点外，输入的每个字节都原样写出。
 */
/*
 * 判断源码里是否把某个名字（如 `texture`）**声明成了变量**。
 *
 * 【为什么不能简单搜 "texture" 子串】
 *   绝大多数着色器都会调用 `texture(...)`，子串搜索必然命中，
 *   于是会把不需要改名的着色器也整体重命名，反而制造问题。
 *
 * 【判据】扫描全文，找到标识符等于 name 的位置，然后看它**左侧**：
 *   跳过空白后，前一个字符若不是 '(' `.` 之类的调用/成员访问符号，
 *   且再往左能找到一个**类型名**（sampler2D/sampler3D/vecN/float/int/...），
 *   则说明这是声明位置。
 *
 *   形如 `uniform sampler2D texture;`  -> 命中
 *        `texture(tex, uv)`           -> 不命中（左侧是 '(' 或行首）
 *        `float x = texture.foo`      -> 不命中（左侧是 '.'）
 */
static int declares_ident_as_var(const char *src, const char *name) {
    static const char *const kTypes[] = {
        "sampler2D", "sampler3D", "samplerCube", "sampler2DShadow",
        "sampler2DArray", "samplerCubeShadow", "sampler2DArrayShadow",
        "isampler2D", "usampler2D", "isampler3D", "usampler3D",
        "float", "int", "uint", "bool", "double",
        "vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4",
        "uvec2", "uvec3", "uvec4", "bvec2", "bvec3", "bvec4",
        "mat2", "mat3", "mat4", "mat2x2", "mat2x3", "mat2x4",
        "mat3x2", "mat3x3", "mat3x4", "mat4x2", "mat4x3", "mat4x4",
        NULL
    };
    const char *end = src + strlen(src);
    size_t nlen = strlen(name);

    for (const char *p = src; p + nlen <= end; p++) {
        /*
         * 【必须跳过注释】—— 真实存在的缺陷，由宿主端夹具当场抓到
         *
         *   夹具 bsl_user_texture2DShadow.frag 的**注释**里写了：
         *       //   float texture(sampler2DShadow s, vec3 p) { ... }
         *   这是对错误产物的说明，不是代码。但本函数原先直接逐字符扫描，
         *   看到 `float texture(` 就认定「声明了名为 texture 的变量/函数」，
         *   于是把整个着色器里所有 `texture(...)` 调用改名为
         *   glesmod_texture —— 连内建调用一起改坏。
         *
         *   真实光影包的注释里同样频繁出现 `texture2D(...)` 之类的示例写法，
         *   因此这不是夹具特有问题，而是必须堵住的漏洞。
         */
        if (p[0] == '/' && p + 1 < end && p[1] == '/') {
            const char *nl = memchr(p, '\n', (size_t)(end - p));
            p = nl ? nl : end - 1;
            continue;
        }
        if (p[0] == '/' && p + 1 < end && p[1] == '*') {
            const char *cl = strstr(p + 2, "*/");
            p = (cl != NULL && cl + 1 < end) ? cl + 1 : end - 1;
            continue;
        }

        if (strncmp(p, name, nlen) != 0) continue;
        /* 词法边界 */
        if (p > src && is_ident_char(p[-1])) continue;
        if (p + nlen < end && is_ident_char(p[nlen])) continue;

        /* 左侧必须不是调用/成员访问 */
        const char *l = p;
        while (l > src && (l[-1] == ' ' || l[-1] == '\t' ||
                           l[-1] == '\n' || l[-1] == '\r')) l--;
        if (l > src && (l[-1] == '(' || l[-1] == '.' || l[-1] == '#' ||
                        l[-1] == ',')) {
            continue;
        }
        if (l <= src) continue;     /* 行首：可能是 `texture = ...` 赋值，不算声明 */

        /*
         * 再往左找最近的标识符；若它是类型名，则确认为声明。
         *
         * 【回退必须在一处「必然表示这是表达式」的符号前停住】
         *   这里曾造成一次严重回归：MC 的
         *       vec4 color = texture(Sampler0, texCoord);
         *   `texture` 左侧跳过空白是 '='（不触发上面的跳过条件），
         *   回退找标识符越过 '=' 找到 `color`（不是类型名），
         *   继续往左找到 `vec4` —— **是类型名**，于是误判为「声明了 texture」，
         *   把整个着色器里的 `texture(...)` 调用全改名成 glesmod_texture，
         *   MC 的 125 个着色器里 65 个当场编译失败：
         *       'glesmod_texture' : no matching overloaded function found
         *
         *   修法：回退过程中一旦遇到 '=' '(' ')' ',' ';' '{' '}' 或任何
         *   运算符，立即停止 —— 这些都是「右侧是表达式」的标志，
         *   真正的声明位置不会出现它们。
         */
        const char *u = l;
        int guard = 0;
        while (u > src && guard < 96) {
            guard++;
            if (is_ident_char(u[-1])) {
                const char *we = u;
                while (u > src && is_ident_char(u[-1])) u--;
                size_t tl = (size_t)(we - u);
                for (int i = 0; i < (int)LIST_LEN(kTypes) && kTypes[i] != NULL; i++) {
                    if (strlen(kTypes[i]) == tl &&
                        strncmp(kTypes[i], u, tl) == 0) {
                        return 1;
                    }
                }
                /* 找到别的标识符（可能是限定符），继续往前看 */
                continue;
            }
            /* 表达式标志：立即停止，不再往前找 */
            if (u[-1] == '=' || u[-1] == '(' || u[-1] == ')' ||
                u[-1] == ',' || u[-1] == ';' || u[-1] == '{' ||
                u[-1] == '}' || u[-1] == '+' || u[-1] == '-' ||
                u[-1] == '*' || u[-1] == '/' || u[-1] == '%' ||
                u[-1] == '<' || u[-1] == '>' || u[-1] == '?' ||
                u[-1] == ':' || u[-1] == '[' || u[-1] == ']') {
                break;
            }
            u--;
        }
    }
    return 0;
}

/*
 * 判断名字 [name, name+nlen) 是否在函数形参表区间 [lp, rp) 里出现过（整词）。
 *
 * 【为什么必须有这个检查 —— 真机 BSL 证据（错误 1/3 的根因）】
 *     vec4 weatherCol = mix(weatherRain, ...);          // 全局
 *     vec3 CalcLightColor(vec3 sun, vec3 night, vec3 weatherCol) { ... }
 *   全局变量与函数形参**同名**。桌面 GLSL 里全局初始化器在全局作用域执行，
 *   函数体内的 `weatherCol` 是形参（vec3），两者互不干扰，合法。
 *
 *   但转换器会把全局初始化器「搬进」每个使用它的函数体内：
 *     vec3 CalcLightColor(vec3 sun, vec3 night, vec3 weatherCol) {
 *         weatherCol = mix(weatherRain, ...);   // vec4 赋给 vec3 形参！
 *   真机报：
 *     'assign' : cannot convert from '4-component vector of float' to
 *                'in 3-component vector of float'
 *
 *   凡是「形参名与全局名撞车」的函数，都**不能**注入该全局的复原赋值：
 *   形参会遮蔽全局名，注入进去就是给形参赋值，类型必然不符。
 *   本函数所在的这段语句因此改为「跳过该注入」，全局名在该函数内不可见，
 *   与桌面语义完全一致。
 */
static int param_list_has_name(const char *lp, const char *rp,
                               const char *name, size_t nlen) {
    if (nlen == 0) return 0;
    for (const char *q = lp; q < rp; ) {
        if (!is_ident_char(*q)) { q++; continue; }
        const char *b = q;
        while (q < rp && is_ident_char(*q)) q++;
        if ((size_t)(q - b) == nlen && strncmp(b, name, nlen) == 0) return 1;
    }
    return 0;
}

/*
 * 从函数体 '{' 的位置 p 反向找出形参表区间 [*lp, *rp)。
 * 返回 1 表示成功（p 前面确实是 `... )`，且能找到配对的 '('）。
 */
static int find_function_param_list(const char *p, const char *src,
                                    const char **lp, const char **rp) {
    const char *q = p;
    while (q > src && (q[-1] == ' ' || q[-1] == '\t' ||
                       q[-1] == '\n' || q[-1] == '\r')) q--;
    if (q <= src || q[-1] != ')') return 0;
    const char *t = q - 1;      /* 指向 ')' */
    *rp = t + 1;
    int d = 0;
    while (t > src) {
        if (t[-1] == ')') { d++; t--; continue; }
        if (t[-1] == '(') {
            if (d == 0) { *lp = t; return 1; }
            d--; t--; continue;
        }
        t--;
    }
    return 0;
}

static void inject_global_assignments(sbuf *out, const char *src,
                                      const global_init_movers *movers) {
    if (movers->count == 0) {
        sbuf_puts(out, src);
        return;
    }

    int depth = 0;
    const char *p = src;

    while (*p != '\0') {
        /* 跳过注释与字符串，避免其中的花括号被误计数 */
        if (p[0] == '/' && p[1] == '/') {
            const char *e = strchr(p, '\n');
            const char *stop = e ? e : p + strlen(p);
            sbuf_put(out, p, (size_t)(stop - p));
            p = stop;
            continue;
        }
        if (p[0] == '/' && p[1] == '*') {
            const char *e = strstr(p + 2, "*/");
            const char *stop = e ? e + 2 : p + strlen(p);
            sbuf_put(out, p, (size_t)(stop - p));
            p = stop;
            continue;
        }
        if (p[0] == '"') {
            const char *start = p;
            p++;
            while (*p != '\0' && *p != '"') {
                if (*p == '\\' && p[1] != '\0') p++;
                p++;
            }
            if (*p == '"') p++;
            sbuf_put(out, start, (size_t)(p - start));
            continue;
        }

        if (*p == '{') {
            int is_function_body = 0;
            const char *plp = NULL;
            const char *prp = NULL;
            if (depth == 0) {
                /*
                 * 向前跳过空白，')' 则说明是函数体。
                 * 同时取出形参表区间，供后面做「形参遮蔽」判断。
                 */
                if (find_function_param_list(p, src, &plp, &prp)) {
                    is_function_body = 1;
                }
            }

            sbuf_puts(out, "{");
            if (is_function_body) {
                size_t brace_off = (size_t)(p - src);
                sbuf_puts(out, "\n");
                for (int i = 0; i < movers->count; i++) {
                    /* 只注入声明在这个函数之前的变量 */
                    if (movers->item[i].decl_offset >= brace_off) continue;
                    /*
                     * 【形参遮蔽检查】形参名与全局名相同时不能注入。
                     * 见 param_list_has_name 的说明：注入 = 给形参赋全局值，
                     * 维度/类型必然不符（真机 'weatherCol' 就是这一例）。
                     * 语义上：形参遮蔽了全局名，函数内本就看不到全局变量，
                     * 跳过注入与桌面 GLSL 行为完全一致。
                     */
                    if (plp != NULL &&
                        param_list_has_name(plp, prp, movers->item[i].name,
                                            strlen(movers->item[i].name))) {
                        continue;
                    }
                    sbuf_puts(out, "    ");
                    sbuf_puts(out, movers->item[i].name);
                    sbuf_puts(out, " = ");
                    sbuf_puts(out, movers->item[i].init);
                    sbuf_puts(out, ";\n");
                }
            }
            depth++;
            p++;
            continue;
        }

        if (*p == '}') {
            if (depth > 0) depth--;
            sbuf_put(out, p, 1);
            p++;
            continue;
        }

        sbuf_put(out, p, 1);
        p++;
    }
}

/*
 * ================= 残留风险自检 =================
 *
 * 【要解决的问题】
 *   本项目反复出现同一类失败：转换器把某个写法漏改了，但**本地看不见**，
 *   只能等真机编译失败、用户贴日志、我再反推行号。每轮代价很高。
 *
 *   漏看的原因有两个：
 *     1. 光影包按宏条件（profile / 模组支持）选分支，我的复现未必与
 *        真机走同一条分支，漏改的恰好在没编到的分支里；
 *     2. 驱动只在**编译失败**时打日志，那时已经拿不到我们的输出文本。
 *
 *   所以把「自检」搬进转换器本身：转换完成后检查自己的输出，
 *   把仍然存在的非法构造写进降级事件。下次日志直接给出
 *   行号 + 内容，不必再靠猜。
 *
 * 【设计原则：只报确定非法的，绝不报「可能有问题」】
 *   误报会把排查引向无关方向，比漏报更有害。
 *   下面只收两类 GLSL ES 明确禁止、桌面 GLSL 明确允许的构造：
 *
 *     (1) 浮点类型声明后跟整数字面量初始化
 *             const float x = 5;      -> '=' cannot convert from 'const int'
 *         正常已被规则 D 处理；此处作为兜底，若仍存在说明有漏洞。
 *
 *     (2) 整数字面量直接与「已知的整数变量」做算术/比较
 *             某些位置的类型推导超出本转换器的启发式能力
 *
 *   每个着色器最多上报若干条，避免刷屏。
 */
static void report_risk(int *reported, const char *what,
                        const char *src, const char *at) {
    if (*reported >= 6) return;
    /* 计算行号 */
    int line = 1;
    for (const char *q = src; q < at && *q != '\0'; q++) {
        if (*q == '\n') line++;
    }
    /* 取该行内容（截断保护） */
    const char *ls = at;
    while (ls > src && ls[-1] != '\n') ls--;
    const char *le = at;
    while (*le != '\0' && *le != '\n' && (le - ls) < 100) le++;

    char detail[320];
    snprintf(detail, sizeof(detail), "残留风险[%s] 行 %d: %.*s",
             what, line, (int)(le - ls), ls);
    glesmod_degrade(GLESMOD_DEGRADE_SHADER_UNSUPPORTED_FEATURE, detail);
    (*reported)++;
}

static void scan_residual_risks(const char *src, shader_stage stage) {
    int reported = 0;
    const char *end = src + strlen(src);
    const char *p = src;

    while (p < end) {
        const char *skipped = skip_comment_or_string(p, end);
        if (skipped != NULL) { p = skipped; continue; }
        if (p[0] == '/' && p[1] == '*') {     /* 兜底：块注释 */
            const char *e = strstr(p + 2, "*/");
            p = (e != NULL && e + 2 <= end) ? e + 2 : end;
            continue;
        }
        if (p[0] == '/' && p[1] == '/') {
            const char *e = strchr(p, '\n');
            p = (e != NULL && e < end) ? e : end;
            continue;
        }

        /*
         * 检查 (1)：`float <name> = <纯整数字面量>;`
         * 只看「声明 + 初始化」这种最简形态，避免误报。
         */
        if ((p == src || !is_ident_char(p[-1])) &&
            (strncmp(p, "float", 5) == 0 || strncmp(p, "lowp ", 5) == 0 ||
             strncmp(p, "highp", 5) == 0 || strncmp(p, "mediump", 7) == 0)) {
            /* 取类型名长度 */
            size_t tl = 0;
            if (strncmp(p, "float", 5) == 0) tl = 5;
            else if (strncmp(p, "highp", 5) == 0) tl = 5;
            else if (strncmp(p, "mediump", 7) == 0) tl = 7;
            else tl = 5;
            if ((p + tl >= end) || is_ident_char(p[tl])) { p++; continue; }
            const char *q = p + tl;
            while (q < end && (*q == ' ' || *q == '\t')) q++;
            /* 取变量名 */
            if (q < end && is_ident_char(*q)) {
                const char *ne = q;
                while (ne < end && is_ident_char(*ne)) ne++;
                const char *r = ne;
                while (r < end && (*r == ' ' || *r == '\t')) r++;
                if (r < end && *r == '=') {
                    r++;
                    while (r < end && (*r == ' ' || *r == '\t')) r++;
                    /* 纯整数字面量，且其后紧跟 ';' */
                    if (r < end && isdigit((unsigned char)*r)) {
                        const char *ds = r;
                        while (r < end && isdigit((unsigned char)*r)) r++;
                        if (r < end && (*r == ';' || *r == ' ' ||
                                        *r == '\t' || *r == '\n')) {
                            report_risk(&reported,
                                        "float 初始化含整数字面量", src, ds);
                        }
                    }
                }
            }
        }
        p++;
    }
    (void)stage;
}

char *glesmod_convert_shader_source(const char *src, int *out_len, int stage_gl) {
    if (src == NULL) return NULL;
    if (out_len != NULL) *out_len = 0;

    /*
     * 已经是 ES 着色器则原样返回副本，不做任何改动。
     *
     * 判定依据是 #version 行里是否带 "es"。不能只看源码里是否出现过
     * "es" 子串——很多标识符（如 screenSize、texCoord）都含该字母。
     */
    {
        const char *v = strstr(src, "#version");
        if (v != NULL) {
            const char *eol = strchr(v, '\n');
            size_t n = eol ? (size_t)(eol - v) : strlen(v);
            /* 在该行范围内查找独立的 "es" 标记 */
            for (size_t i = 0; i + 1 < n; i++) {
                if (v[i] == 'e' && v[i + 1] == 's' &&
                    (i == 0 || !is_ident_char(v[i - 1])) &&
                    (i + 2 >= n || !is_ident_char(v[i + 2]))) {
                    size_t len = strlen(src);
                    char *copy = (char *)malloc(len + 1);
                    if (copy == NULL) return NULL;
                    memcpy(copy, src, len + 1);
                    if (out_len != NULL) *out_len = (int)len;
                    return copy;
                }
            }
        }
    }

    /*
     * 步骤 0：展开 #moj_import。
     *
     * 【必须最先做，这是真机启动失败的根因】
     *   #moj_import 是 Minecraft 自己的预处理指令，GLSL 不支持它。
     *   若原样送给驱动，ES 编译器报：
     *       ERROR: 0:4: '' : GLSL compile error: malformed preprocessor directive
     *   实测证据：position.vsh 的 #moj_import 在转换后输出中正好位于第 4 行，
     *   与驱动报告的行号完全吻合 —— 说明它确实被原样送到了驱动，
     *   而非如先前假设的「MC 已提前展开」。
     *
     *   放在最前面还有两个好处：
     *     1. include 内容里可能含须转换的写法（如 ivec2 与 float 混算），
     *        展开后再走后续各步即可一并处理
     *     2. include 若带 #version，也会被后续的「移除全部 #version」清掉
     */
    sbuf expanded;
    sbuf_init(&expanded, INITIAL_CAP);
    if (expanded.failed) return NULL;
    expand_moj_imports(&expanded, src);
    if (expanded.failed) {
        free(expanded.buf);
        return NULL;
    }

    /*
     * 阶段判定：优先采用调用方从 GL 查得的真实阶段，仅在未知时退回猜测。
     * 真实阶段决定了是否注入 precision 以及如何转换限定符，用错会直接
     * 导致编译失败（见 stage_from_gl 的说明）。
     */
    shader_stage stage = stage_from_gl(stage_gl);
    if (stage == STAGE_UNKNOWN) {
        stage = detect_stage(expanded.buf);
    }

    /* 步骤 1：剥离所有原 #version 声明（展开后的源码里可能不止一处） */
    sbuf body;
    sbuf_init(&body, INITIAL_CAP);
    if (body.failed) {
        free(expanded.buf);
        return NULL;
    }
    int version = extract_version(expanded.buf, &body);
    free(expanded.buf);
    (void)version;  /* 原版本号不参与决策，统一转为 320 es */

    /*
     * 步骤 1b：规范化 GLSL ES 不支持的预处理器指令。
     *
     *  - 桌面专属 #extension：
     *        #extension GL_ARB_separate_shader_objects : require
     *    ARB 扩展在 ES 里不存在，而 `require` 的语义是「不支持则报错」，
     *    直接导致编译失败。ES 3.x 已原生支持独立着色器对象与 location
     *    限定符，因此删掉该行是正确的。
     *
     *  - #error 的字符串消息：
     *        #error "Vertex compression must be enabled"
     *    GLSL ES 没有字符串字面量。Mali 即使在非活动分支里也会在引号上
     *    报 L0001，因此把消息规范化为合法 token。
     */
    sbuf noext;
    sbuf_init(&noext, INITIAL_CAP);
    if (noext.failed) {
        free(body.buf);
        return NULL;
    }
    normalize_preprocessor_directives(&noext, body.buf);
    free(body.buf);
    if (noext.failed) { free(noext.buf); return NULL; }
    body = noext;

    /*
     * 步骤 1c：展开「对象式整型常量宏」。
     *
     * 【为什么必须在字面量处理之前】
     *   见 expand_int_const_macros 的长注释：同一个宏可能同时出现在
     *   整数与浮点上下文里，而 GLSL ES 不允许隐式转换，两侧需要相反的
     *   改写。按定义文本展开后，各使用点由既有的字面量规则自然处理，
     *   无需任何新的类型推断。
     */
    {
        sbuf mac;
        sbuf_init(&mac, INITIAL_CAP);
        if (mac.failed) { free(body.buf); return NULL; }
        expand_int_const_macros(&mac, body.buf);
        free(body.buf);
        if (mac.failed) { free(mac.buf); return NULL; }
        body = mac;
    }

    /*
     * 步骤 2：替换废弃内置与旧式限定符（在 body 上进行）。
     *
     * 采用「链式替换」：每一步基于上一步的输出。这样每次只处理一条规则，
     * 逻辑线性可读，避免写成复杂的联合扫描。
     *
     * 【替换顺序的约束】
     *   长的名字必须先于短的替换，否则会产生错误的二次替换：
     *   若先替换 texture2D，texture2DProj 会变成 textureProj 之外的东西
     *   （texture2D -> texture，于是 texture2DProj 变成 textureProj 恰好正确，
     *   但 texture2DLod 会变成 textureLod 之外的形式而不正确）。
     *   因此固定按「长 -> 短」排列：texture2DProj / texture2DLod 先于 texture2D。
     */
    sbuf replaced;
    sbuf_init(&replaced, INITIAL_CAP);
    if (replaced.failed) {
        free(body.buf);
        return NULL;
    }
    {
        /*
         * 步骤 2a-0（必须在 texture2D -> texture 之前）：
         * 把名为 `texture` 的用户变量改名。
         *
         * 【整包验证暴露的真实缺陷 —— BSL 里影响面最大的单点问题】
         *   BSL 的 gbuffers_* 系列大量出现：
         *       uniform sampler2D texture;
         *   桌面 GLSL 1.20 里 `texture` 只是普通标识符，合法。
         *   但 GLSL ES 3.x 里 `texture` 是**内建函数名**，用户不能声明同名变量，
         *   驱动报：
         *       'texture' : redefinition
         *   更糟的是后续所有 `texture(tex, uv)` 调用都被解析成那个
         *   sampler2D 变量，级联报出：
         *       'texture' : can't use function syntax on variable
         *       'scalar swizzle' : not supported ...
         *   一个声明就能让整个着色器废掉。
         *
         * 【顺序至关重要】必须在本步骤稍后的 `texture2D` -> `texture` 替换
         *   **之前**完成。否则我们刚把调用改写成 `texture(...)`，
         *   紧接着就被这条重命名规则一并改掉，产出乱码。
         *
         * 【易错点】此处必须对 body.buf 操作。`replaced` 只是刚 sbuf_init
         *   出来的空缓冲（sbuf_init 仅分配，不拷贝），此前的写法对空缓冲
         *   做判定，导致这条规则从未生效。
         */
        if (declares_ident_as_var(body.buf, "texture")) {
            sbuf ren;
            sbuf_init(&ren, INITIAL_CAP);
            if (ren.failed) { free(body.buf); return NULL; }
            replace_word(&ren, body.buf, "texture", "glesmod_texture");
            if (ren.failed) { free(body.buf); free(ren.buf); return NULL; }
            free(body.buf);
            body = ren;
        }
    }

    /*
     * ============ 光影包【自定义】了某个会被我们改名的函数 ============
     *
     * 【真机证据 —— BSL 007_basic.fsh，阻塞整条光影管线的最后一个错误】
     *
     *   原文第 151 行：
     *       float texture2DShadow(sampler2DShadow shadowtex, vec3 shadowPos) {
     *           return vec4(texture(shadowtex, shadowPos)).x;
     *       }
     *   光影包**自己定义**了一个叫 texture2DShadow 的辅助函数。
     *   而我们的替换表把 texture2DShadow 无条件改写为 texture，于是产出：
     *       float texture(sampler2DShadow shadowtex, vec3 shadowPos) {
     *           return vec4(glesmod_texture(shadowtex, shadowPos)).x;
     *       }
     *   两个后果同时发生：
     *     a) 函数名撞上 GLSL ES 内建 texture()，驱动报
     *        'texture' :  can't redefine/overload built-in functions!
     *     b) 函数体内的调用被改名后解析为**自身**，驱动报
     *        INTERNAL ERROR: Has recursive call chain!
     *
     *   旧注释里断言「texture2DShadow2x2 这类包内自定义函数因词法边界
     *   不匹配而天然不会被误替换」——**该断言是错的**：
     *   `texture2DShadow` 在替换表里，而 BSL 确实定义了它。
     *
     * 【修法】把「定义位置的名字」与「函数体内的调用」用同一个名字
     *   统一改成 glesmod_ 前缀版本，一次完成。
     *   ES 的 texture(sampler2DShadow, vec3) 直接返回 float，
     *   所以包内那个只做 `.x` 提取的 shim 在 ES 下是多余的；
     *   但保留它（改名后）比删除更安全 —— 不改变调用点数量与语义。
     *
     * 【为什么必须与上面的 texture 变量重命名同级、且在替换表之前】
     *   替换表是**无条件**改名（不看是否有自定义定义），所以必须先跑；
     *   一旦改名，后面的 replace_word 就再也找不到原名，替换表自然放过。
     */
    {
        static const char *const kUserShadowFns[] = {
            "texture2DShadow", "texture2DShadow2x2", "texture2DGradARB",
            "texture2DLod", "texture2DProj", "texture2DGrad", "texture3D",
            "texture2D", "textureCube", "texture2DRect", "shadow2D",
            NULL
        };
        for (int fi = 0; fi < (int)LIST_LEN(kUserShadowFns) && kUserShadowFns[fi] != NULL; fi++) {
            const char *fn = kUserShadowFns[fi];
            /*
             * 判定「本文件是否定义了该名字的函数」——
             * 复用 declares_ident_as_var：它认「类型名 + 名字」这一形态，
             * 函数定义与变量声明都能识别，而调用/成员访问会跳过。
             */
            if (!declares_ident_as_var(body.buf, fn)) continue;

            {
                char newname[64];
                sbuf ren2;
                snprintf(newname, sizeof(newname), "glesmod_%s", fn);

                sbuf_init(&ren2, INITIAL_CAP);
                if (ren2.failed) { free(body.buf); return NULL; }
                replace_word(&ren2, body.buf, fn, newname);
                if (ren2.failed) { free(body.buf); free(ren2.buf); return NULL; }
                free(body.buf);
                body = ren2;
            }

            {
                char note[256];
                snprintf(note, sizeof(note),
                         "光影包自定义了函数 %s，已重命名为 glesmod_%s"
                         "（原名是桌面 GL 旧式内建；直接替换会重定义内建函数"
                         "并导致自递归）", fn, fn);
                glesmod_degrade(GLESMOD_DEGRADE_ENUM_MAPPED, note);
            }
        }
    }

    {
        /*
         * 替换规则：{from, to}
         *
         * 【顺序是硬约束：长的名字必须排在短的之前】
         *   替换是逐条链式进行的，后一条作用于前一条的输出。
         *   若把 `texture2D` 排在 `texture2DGradARB` 前面，
         *   `texture2DGradARB` 会先被裁成 `textureGradARB`，
         *   再匹配不到正确规则，产出驱动不认识的函数名。
         *   同理 `texture2DShadow` 必须排在 `texture2D` 之前。
         *
         * 【补齐的依据】整包扫描（tools/list_gl_funcs.py）统计出 BSL 实际用到：
         *     texture2D          178
         *     texture2DLod        47
         *     texture2DGradARB    16   <- 此前漏掉，导致 gbuffers_water 编译失败
         *     texture2DShadow      8   <- 此前漏掉
         *     texture3D            4   <- 此前漏掉
         *   漏掉一个就会让使用它的整个着色器失效。
         *
         * 【刻意不动的名字】textureCatmullRom / texture2DShadow2x2 是光影包
         *   自己定义的辅助函数（包内有定义），不是 GL 内建，替换反而会改坏。
         *   它们因为词法边界不匹配，天然不会被误替换。
         */
        const char *from[] = {
            "texture2DGradARB",   /* 必须最先（最长） */
            "texture2DGrad",
            "texture2DShadow",
            "texture2DProj",      /* 先于 texture2D */
            "texture2DLod",       /* 先于 texture2D */
            "texture3D",          /* 先于 texture（避免被后续规则二次处理） */
            "texture2D",
            "textureCube",
            "texture2DRect",
            "shadow2D",           /* 桌面旧式阴影采样 */
            "attribute",          /* 旧式顶点输入限定符（ES 3.x 用 in） */
            /*
             * 【必须去掉 noperspective】
             *   真机证据（Iris 导出的 patched_shaders/001_deferred1.fsh）：
             *       noperspective in vec2 texCoord;
             *   真机 Adreno 报：
             *       'noperspective' : Reserved word.
             *       'noperspective' : not supported for this version
             *   它出现在 GLSL 1.50/4.x（需要 GL 3.2+），但 **GLSL ES 3.20 的
             *   保留字表里没有实现插值限定符**，必须删掉。
             *   删掉后语义变为默认的 smooth 插值 —— 对光影包完全可接受
             *   （noperspective 只影响透视校正的细节，不会导致错误结果）。
             *   这里替换成空串（规则表里用 "" 表示删除）。
             */
            "noperspective",
        };
        const char *to[] = {
            "textureGrad",
            "textureGrad",
            "texture",
            "textureProj",
            "textureLod",
            "texture",
            "texture",
            "texture",
            "texture",
            "texture",
            "in",
            "",
        };
        const size_t rule_count = sizeof(from) / sizeof(from[0]);

        sbuf cur;
        sbuf_init(&cur, INITIAL_CAP);
        if (cur.failed) { free(body.buf); free(replaced.buf); return NULL; }
        sbuf_puts(&cur, body.buf);
        free(body.buf);

        for (size_t i = 0; i < rule_count && !cur.failed; i++) {
            sbuf next;
            sbuf_init(&next, INITIAL_CAP);
            if (next.failed) break;
            replace_word(&next, cur.buf, from[i], to[i]);
            free(cur.buf);
            cur = next;
        }

        if (cur.failed) { free(cur.buf); free(replaced.buf); return NULL; }

        /*
         * varying 的处理依赖阶段，不能放进上面的固定规则表：
         *   - 顶点着色器：varying 是输出 -> out
         *   - 片元着色器：varying 是输入 -> in
         *   - 阶段未知：保守地不动，让驱动报错（好过改错方向）
         */
        if (stage == STAGE_VERTEX || stage == STAGE_FRAGMENT) {
            const char *rep = (stage == STAGE_VERTEX) ? "out" : "in";
            sbuf next;
            sbuf_init(&next, INITIAL_CAP);
            if (next.failed) { free(cur.buf); free(replaced.buf); return NULL; }
            replace_word(&next, cur.buf, "varying", rep);
            free(cur.buf);
            cur = next;
        }

        if (cur.failed) { free(cur.buf); free(replaced.buf); return NULL; }
        free(replaced.buf);
        replaced = cur;
    }

    /*
     * 步骤 2b：把浮点上下文中的整数字面量改写为浮点字面量。
     *
     * 【为什么放在这里（在限定符转换之后、组装之前）】
     *   上一步已把 attribute/varying 换成 in/out，语句形态稳定，
     *   此步不再关心阶段差异，只做纯粹的表达式级改写。
     *
     * 【为什么必须做】
     *   桌面 GLSL 允许 vec2 * 2 这类隐式 int->float 转换，GLSL ES 不允许。
     *   这是 FML 早期显示窗口真机启动失败的直接原因：
     *       gl_Position = vec4((position/screenSize) * 2 - 1, 0.0, 1.0);
     *   提升 #version 无法解决 —— 这是语言规则差异，必须改写字面量。
     */
    {
        sbuf norm;
        sbuf_init(&norm, INITIAL_CAP);
        if (norm.failed) {
            free(replaced.buf);
            return NULL;
        }
        normalize_int_literals(&norm, replaced.buf);
        free(replaced.buf);
        if (norm.failed) { free(norm.buf); return NULL; }
        replaced = norm;
    }

    /*
     * 步骤 2d：修复「整数向量与浮点混合运算」。
     *
     * 【为什么必须做 —— 这曾导致渲染管线整体失效】
     *   驱动报：
     *       ERROR: 1:14: '/' : wrong operand types ... '2-component vector of
     *       int' and 'const float'
     *   源头是 MC 的 light.glsl：`clamp(uv / 256.0, ...)`（uv 为 ivec2）。
     *   桌面 GLSL 允许 ivec2 / float，GLSL ES 不允许。
     *
     *   由于 MC 的核心着色器几乎都 #moj_import light.glsl，
     *   它们会【全部】编译失败 -> reloadShaders 抛异常 -> 渲染管线失效。
     *   表现：逻辑正常（音乐播放、按钮可点），但画面静止不动。
     *
     *   放在字面量浮点化【之后】：此刻形如 `uv / 256.0` 的右操作数
     *   已确定是浮点字面量，判定无歧义。
     */
    {
        sbuf fixed;
        sbuf_init(&fixed, INITIAL_CAP);
        if (fixed.failed) {
            free(replaced.buf);
            return NULL;
        }
        fix_int_vector_float_ops(&fixed, replaced.buf);
        free(replaced.buf);
        if (fixed.failed) { free(fixed.buf); return NULL; }
        replaced = fixed;
    }

    /*
     * 步骤 2d2：混合 int/uint 二元运算（补 uint() 转换）。
     *
     * 【为什么必须做 —— Flywheel instancing 的最后一处阻塞】
     *   Flywheel 的 common.vert 写的是
     *       flw_vertexId = gl_VertexID - baseVertex;
     *   `gl_VertexID` 是 int（内建），`baseVertex` 是 `in uint`。
     *   桌面 GLSL 允许两者混算，GLSL ES **完全禁止**，驱动报：
     *       no operation '-' exists that takes a left-hand operand of type
     *       'gl_VertexID int' and a right operand of type 'in uint'
     *   语义上是把 int 操作数**按位重解释**为 uint（已用 SPIR-V 逐指令证明，
     *   见 fix_mixed_int_uint 的说明），因此补 uint() 是等价改写。
     *
     * 【为什么放在这里】
     *   上一步 fix_int_vector_float_ops 处理的是「整数向量/标量与浮点」的
     *   混算，两者互不重叠（本规则的判据要求一侧确定为 uint，
     *   而浮点混算两侧必有浮点）。放在其后可避免两趟互相干扰：
     *   此处语句形态已稳定（限定符、字面量、向量混算均已处理完）。
     */
    {
        sbuf miu;
        sbuf_init(&miu, INITIAL_CAP);
        if (miu.failed) { free(replaced.buf); return NULL; }
        int miu_rewrites = fix_mixed_int_uint(&miu, replaced.buf);
        free(replaced.buf);
        if (miu.failed) { free(miu.buf); return NULL; }

        /*
         * 上报改写次数。
         * 【为什么必须上报】本规则此前完全静默 —— 真机上「是否生效」
         * 无法与「本就没遇到混合 int/uint 代码」区分开。
         * 有了这条降级事件，设备日志里能直接看到计数。
         */
        if (miu_rewrites > 0) {
            char note[192];
            snprintf(note, sizeof(note),
                     "混合 int/uint 运算：已补 %d 处 uint() 转换"
                     "（桌面 GLSL 允许 int 与 uint 混算，GLSL ES 完全禁止）",
                     miu_rewrites);
            glesmod_degrade(GLESMOD_DEGRADE_ENUM_MAPPED, note);
        }

        replaced = miu;
    }

    /*
     * 步骤 2d3：`return <整数字面量>;` 位于**浮点返回类型**的函数内。
     *
     * 【为什么必须做 —— Flywheel 装配着色器最后 3 个错误】
     *   flywheel:internal/wavelet.glsl 里
     *       float total_absorbance(in sampler2DArray coefficients) {
     *           if (scale_coefficient == 0) {
     *               return 0;          // <-- int 字面量，函数返回 float
     *           }
     *   Flywheel 在 float 函数里写 `return 0;`。
     *
     *   这**不是**上游的 bug —— 实测已证明（probe_return_int_float.ps1）：
     *       #version 460 桌面 : ACCEPTED
     *       #version 320 es   : REJECTED
     *   与 `float f = 0;`、`vec2 * 2`、`float < int`、`pow(f, 3)` 完全同类，
     *   属于我们要适配的桌面专属写法。
     *
     * 【为什么放在字面量浮点化（2c）之后】
     *   2c 依赖语句上下文（stmt_is_float_context 等），而本步只看
     *   「所属函数的返回类型」这一条确定信息，两者判据不重叠。
     *   放在之后可保证：若 2c 已把 `return 0;` 改成 `return 0.0;`，
     *   本步会因为「已不是纯整数」而自然跳过，不会重复处理。
     */
    {
        sbuf ret;
        sbuf_init(&ret, INITIAL_CAP);
        if (ret.failed) { free(replaced.buf); return NULL; }
        int ret_rewrites = fix_return_int_in_float_fn(&ret, replaced.buf);
        free(replaced.buf);
        if (ret.failed) { free(ret.buf); return NULL; }

        if (ret_rewrites > 0) {
            char note[192];
            snprintf(note, sizeof(note),
                     "浮点返回类型函数内的 `return <整数>;`：已补 %d 处 `.0`"
                     "（桌面 GLSL 允许 int 隐式转 float，GLSL ES 禁止）",
                     ret_rewrites);
            glesmod_degrade(GLESMOD_DEGRADE_ENUM_MAPPED, note);
        }
        replaced = ret;
    }

    /*
     * 步骤 2e：把「非恒定全局初始化器」搬进函数体。
     *
     * 【为什么必须做 —— 光影加载失败的当前阻塞点】
     *   Iris 注入的 `irisInt_Fog` 用 uniform 初始化全局变量，
     *   GLSL ES 不允许（桌面允许）。真机报：
     *       ERROR: 0:17: 'iris_FogColor' : Only consts can be used in a
     *                     global initializer
     *   而正确修法**不是**加扩展声明（见文件上方注释中的理由），
     *   而是：全局声明去掉初始化器，赋值插到每个函数体开头。
     *
     * 放在最后：此时所有语法改写（限定符、字面量、整数向量运算）
     * 均已完成，语句形态稳定，识别语句边界最可靠。
     */
    {
        global_init_movers movers;
        movers.count = 0;

        sbuf stripped;
        sbuf_init(&stripped, INITIAL_CAP);
        if (stripped.failed) { free(replaced.buf); return NULL; }
        rewrite_nonconst_globals(&stripped, replaced.buf, &movers);
        free(replaced.buf);
        if (stripped.failed) { free(stripped.buf); return NULL; }

        if (movers.count > 0) {
            sbuf injected;
            sbuf_init(&injected, INITIAL_CAP);
            if (injected.failed) { free(stripped.buf); return NULL; }
            inject_global_assignments(&injected, stripped.buf, &movers);
            free(stripped.buf);
            if (injected.failed) { free(injected.buf); return NULL; }

            {
                char note[192];
                snprintf(note, sizeof(note),
                         "着色器含 %d 个非恒定全局初始化器，已改写为"
                         "「全局声明 + 函数内赋值」（GLSL ES 不允许前者；"
                         "此项不依赖任何扩展）",
                         movers.count);
                glesmod_degrade(GLESMOD_DEGRADE_ENUM_MAPPED, note);
            }
            replaced = injected;
        } else {
            replaced = stripped;
        }
    }

    /* 步骤 3：组装输出 */
    sbuf out;
    sbuf_init(&out, INITIAL_CAP);
    if (out.failed) {
        free(replaced.buf);
        return NULL;
    }

    sbuf_puts(&out, "#version 320 es\n");

    /*
     * 精度声明。
     *
     * 【两个阶段都必须声明 float 的默认精度 —— 这是整包验证发现的真实缺陷】
     *   GLSL ES 与桌面 GLSL 的关键差异：**float 在任何一个阶段都没有默认精度**
     *   （GLSL ES 3.20 规范 4.5.4：顶点语言没有默认浮点精度，
     *     片元语言也没有）。只有 int 在顶点阶段有默认精度 mediump。
     *
     *   此前只在片元阶段输出 precision，于是所有**顶点**着色器一出现
     *   `const float` 或显式 float 声明就报：
     *       'float' : type requires declaration of default precision qualifier
     *   真机 BSL 的 shadow.vsh / gbuffers_water.vsh 等一批顶点着色器
     *   正是因此编译失败（整包验证里 176 个着色器中有 55 个失败，
     *   顶点着色器占多数）。
     *
     * 【两个阶段】除 sampler2D / samplerCube 之外的**所有采样器类型**
     * 都没有默认精度，必须逐个声明。这是 Sodium 区块着色器编译失败的直接
     * 原因（它用了 `uniform isamplerBuffer u_SectionTimeInfo;`）。
     * 实测列表见 native/tools/es_sampler_precision_probe.py。
     *
     * 【为什么用 highp 而不是 mediump】
     *   mediump 的 float 只有约 10 位有效位、上限 65504。Minecraft 的着色器
     *   大量使用世界坐标、方块位置与时间累积值，这些数值远超 mediump 范围，
     *   用 mediump 会导致明显的渲染错位（远处顶点抖动、纹理坐标跳变）。
     *   highp 语义上是严格更安全的选择：不会让原本正确的着色器出错。
     *   ES 3.x 强制支持 highp，故可放心使用。
     */
    if (stage == STAGE_FRAGMENT || stage == STAGE_VERTEX) {
        sbuf_puts(&out, "precision highp float;\n");
        sbuf_puts(&out, "precision highp int;\n");
    }

    /* 采样器精度：两个阶段都需要，按源码实际用到的类型精简输出 */
    emit_sampler_precision(&out, replaced.buf);

    /*
     * gl_FragColor 处理。
     *
     * 桌面 GLSL 的 gl_FragColor 在 ES 3.x 中不存在，需改为自定义 out 变量。
     * 这是有风险的替换：若着色器同时使用 gl_FragData 或已有 out 声明，
     * 简单替换可能冲突。此处采取保守策略：
     *   - 仅当源码含 gl_FragColor 且不含 "out vec4" 声明时才注入
     */
    int need_frag_out = (stage == STAGE_FRAGMENT) &&
                        (strstr(replaced.buf, "gl_FragColor") != NULL);
    if (need_frag_out) {
        /* 检查是否已有 out vec4 声明，避免重复 */
        if (strstr(replaced.buf, "out vec4") == NULL &&
            strstr(replaced.buf, "out mediump vec4") == NULL) {
            sbuf_puts(&out, "out vec4 glesmod_FragColor;\n");
        }
    }

    /*
     * 【重要自检：Iris 注入的 out 声明与我们的 gl_FragColor 变量可能冲突】
     *
     * 背景：BSL 源码头文件里**没有** `#version`（Iris 会自己加），
     * 而“是否要处理 gl_FragColor”取决于我们收到的文本里有没有它。
     * Iris 的 CommonTransformer 会把包里的 `gl_FragData[N]` 改写成
     *     layout (location = N) out vec4 outColorN;
     * 于是我们的输出里会出现 **两套 frag 输出**：
     *     我们自己注入的   out vec4 glesmod_FragColor;      （无 location）
     *     Iris 注入的      layout (location = 0) out vec4 outColor0;
     * ES 里没有 location 的 frag 输出由链接器自动分配，
     * 可能撞上 location = 0 —— 这类冲突只在链接期报错，很难定位。
     *
     * 这里把它作为**残留风险**上报，让我们在日志里能看到
     * 「本着色器同时存在无 location 的 out 与 Iris 的 layout out」。
     */
    if (stage == STAGE_FRAGMENT &&
        strstr(out.buf, "out vec4 glesmod_FragColor") != NULL &&
        strstr(out.buf, "layout (location") != NULL) {
        glesmod_degrade(GLESMOD_DEGRADE_ENUM_MAPPED,
                        "着色器同时存在无 location 的 out vec4 glesmod_FragColor "
                        "与 Iris 注入的 layout(location=...) out —— "
                        "ES 下可能因 location 分配冲突而链接失败（见 patched_shaders）");
    }

    sbuf_puts(&out, "\n");

    /* 写入转换后的主体，并把 gl_FragColor 替换为我们的 out 变量 */
    if (need_frag_out) {
        sbuf frag;
        sbuf_init(&frag, INITIAL_CAP);
        if (frag.failed) {
            free(replaced.buf);
            free(out.buf);
            return NULL;
        }
        replace_word(&frag, replaced.buf, "gl_FragColor", "glesmod_FragColor");
        if (frag.failed) {
            free(replaced.buf);
            free(out.buf);
            free(frag.buf);
            return NULL;
        }
        sbuf_puts(&out, frag.buf);
        free(frag.buf);
    } else {
        sbuf_puts(&out, replaced.buf);
    }

    free(replaced.buf);

    if (out.failed) {
        free(out.buf);
        return NULL;
    }

    if (out_len != NULL) *out_len = (int)out.len;

    /*
     * 记录一次降级事件：表示「发生了转换」。
     * 这不是错误，但让用户知道着色器被改写过（便于排查渲染差异）。
     */
    glesmod_degrade(GLESMOD_DEGRADE_SHADER_CONVERSION_FAILED,
                    "着色器已由桌面 GLSL 转换为 GLSL ES（非错误，仅记录）");

    /*
     * ================= 残留风险自检 =================
     *
     * 【为什么要做这件事】
     *   本项目最难的问题一直是「转换器把某些写法漏改了，但本地看不见」。
     *   原因有两个：
     *     1. 光影包用宏条件（profile、模组支持）选分支，我们的复现
     *        未必和真机走同一条分支 —— 漏改的恰好在没编到的分支里。
     *     2. 真机只在**着色器编译失败时**才把驱动原话打进日志，
     *        而那时已经拿不到我们输出的文本了。
     *   于是每一轮都要靠「用户贴日志 → 我猜 → 改」来回，代价很高。
     *
     *   这里改成**让转换器自己报告**：转换完成后扫一遍自己的输出，
     *   把仍然存在的、确定非法的构造找出来，写进降级事件。
     *   这样下次日志会直接给出「哪个文件、哪一行、什么内容」，
     *   不必再依赖行号反推。
     *
     * 【只报确定非法的构造，不报「可能有问题」】
     *   宁可漏报也不误报 —— 误报会把排查引向无关方向。
     *   目前只检查两类，都是 GLSL ES 明确禁止、而桌面 GLSL 允许的：
     *     A. `const float` / `uniform float` 类型名后跟整数字面量初始化
     *        已由规则 D 处理，这里只做兜底校验
     *     B. 整数值与浮点数直接做 * / + / - / 比较（类型不匹配）
     *        这类需要完整类型推导，本转换器靠启发式，必然有漏网
     *   检查方式见 scan_residual_risks()。
     */
    scan_residual_risks(out.buf, stage);

    return out.buf;
}

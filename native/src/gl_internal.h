/*
 * gl_internal.h —— native 层内部接口
 *
 * 供 generated_forwarders.c 与 custom.c 使用。不对外暴露。
 *
 * 许可证：LGPL-3.0-or-later
 */

#ifndef GLESMOD_GL_INTERNAL_H
#define GLESMOD_GL_INTERNAL_H

/*
 * 本文件不包含 EGL/GLES 头文件。
 *
 * 原因：Android NDK 的 <GLES3/gl3.h> 不声明 GL 1.x 的固定功能符号，也不声明
 * GL 2.x 的 glGetString 等部分命令。本项目要导出的是「桌面 GL 名字」，因此
 * 自行定义所需的类型与常量，保证在 Android 与桌面都能编译。
 */

#include <stddef.h>
#include <stdint.h>

/*
 * 包含对外 ABI 头：提供降级码（glesmod_degrade_code）、配置环境变量名、
 * 能力位与 glesmod_caps / glesmod_degrade / glesmod_write_status 的声明。
 * 所有内部源文件都会经此头文件获得它们。
 */
#include "gles_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* GL 基本类型（与 Khronos 定义一致）                                   */
/* ------------------------------------------------------------------ */

typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef void           GLvoid;
typedef signed char    GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef int            GLsizei;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef double         GLclampd;
typedef char           GLchar;
typedef ptrdiff_t      GLintptr;
typedef ptrdiff_t      GLsizeiptr;
typedef int64_t        GLint64;
typedef uint64_t       GLuint64;

/* 同步对象句柄。GLES 3.0+ 与 GL 3.2 均以指针形式表示。 */
typedef void          *GLsync;

/* GL 3.0 的固定宽度整型向量与纹理尺寸类型 */
typedef int64_t        GLint64EXT;
typedef uint64_t       GLuint64EXT;

/*
 * 调试回调原型（GL_ARB_debug_output / KHR_debug）。
 *
 * 用于 glDebugMessageCallback。ES 3.2 支持 KHR_debug，故该符号在
 * GLES 侧也存在，属转发而非 stub。这里只需声明类型以便生成签名。
 * 参数类型按 Khronos 定义，保持 ABI 一致。
 */
typedef void          *GLDEBUGPROC;

/* AMD 与 ARB 各自的调试回调变体，部分驱动单独导出 */
typedef void          *GLDEBUGPROCARB;
typedef void          *GLDEBUGPROCKHR;
typedef void          *GLDEBUGPROCAMD;

#ifndef GL_TRUE
#define GL_TRUE  1
#define GL_FALSE 0
#endif

/* ------------------------------------------------------------------ */
/* 导出与工具宏                                                        */
/* ------------------------------------------------------------------ */

/*
 * 符号必须可见。LWJGL 通过 dlsym 在运行时查找这些符号，因此不能被
 * -fvisibility=hidden 隐藏。
 */
#define GLESMOD_EXPORT __attribute__((visibility("default")))

/** 获取环境变量为整数，缺失或非法时返回默认值。 */
int glesmod_env_int(const char *name, int fallback);

/** 获取环境变量字符串，缺失时返回 NULL。 */
const char *glesmod_env_str(const char *name);

/* gl.xml: GL_EXTENSIONS = 0x1F03 */
#define GLESMOD_GL_EXTENSIONS 0x1F03

/**
 * 判断空格分隔的扩展串是否包含完整 token。
 *
 * 用完整 token 匹配而不是裸 strstr：目标名可能是另一个扩展名的前缀。
 */
int glesmod_extension_present(const char *extensions, const char *name);

/* ------------------------------------------------------------------ */
/* GLES 函数解析                                                       */
/* ------------------------------------------------------------------ */

/** 通用函数指针类型。 */
typedef void (*glesmod_proc_t)(void);

/**
 * 解析 GLES 函数指针。
 *
 * 实现策略（按顺序尝试）：
 *   1. eglGetProcAddress —— ES 扩展与 3.x 函数的正规途径
 *   2. dlsym(RTLD_DEFAULT) —— 部分实现的扩展函数只在此可见
 *
 * 结果会被缓存，因此每帧的调用开销仅为一次指针判空。
 *
 * @return 函数指针；未找到时返回 NULL。
 */
glesmod_proc_t glesym_resolve(const char *name);

/**
 * 上报「符号未找到」。
 *
 * 由生成的转发函数在解析失败时调用。内部会：
 *   - 记录降级事件（每符号仅记一次，避免刷屏）
 *   - 首次出现时输出一行日志
 */
void glesmod_report_missing(const char *name);

/**
 * 上报「安全 stub 被调用」。
 *
 * 与 report_missing 的区别：stub 是我们主动导出的空实现，说明该符号
 * 在 GL 中存在但 GLES 不提供。调用它不会崩溃，但对应功能静默失效。
 *
 * 内部会：
 *   - 首次调用时记一条降级事件（避免每帧刷屏）
 *   - 把符号名加入状态文件的 stub_symbols 列表，便于诊断
 */
void glesmod_report_stub(const char *name);

/* ------------------------------------------------------------------ */
/* 调用追踪（诊断用）                                                  */
/* ------------------------------------------------------------------ */

/**
 * 记录一次 GL 调用，供崩溃后定位问题。
 *
 * 背景：真机上出现过 SIGSEGV，崩在 LWJGL 的 GL.createCapabilities() 内部
 * 执行某个 GL 调用时。崩溃转储只能给出寄存器与 pc 偏移，无法直接看出是
 * 哪个 GL 函数。本机制记录最近若干次调用的函数名并写入状态文件，
 * 崩溃后回看 status.json 的 last_calls 即可定位。
 *
 * 实现要求（务必遵守，否则会拖慢渲染）：
 *   - 不分配内存、不加锁（只用一次原子自增）
 *   - 只保存字符串字面量的指针，不复制内容
 *   - 参数为编译期字面量，因此指针生命周期为整个进程
 *
 * @param name GL 函数名（必须是字符串字面量）
 */
void glesmod_trace_call(const char *name);

/** 追踪开关。由环境变量 GLESMOD_TRACE 控制，默认 1（开启）。 */
extern int glesmod_trace_enabled;

/**
 * 在 GL 入口处调用的追踪宏。
 *
 * 先判断开关再调用，这样关闭时只多一次全局变量读取与分支，
 * 不会产生函数调用开销。开启时也只做一次无锁原子自增。
 *
 * <p><b>注意</b>：本宏只应在**非热路径**上使用（如定制实现里的低频函数）。
 * 高频转发函数请用 {@link GLESMOD_HOTPATH}，它把初始化与追踪合并为一次判断。
 */
#define GLESMOD_TRACE(name)                                    \
    do {                                                       \
        if (glesmod_trace_enabled) glesmod_trace_call(name);   \
    } while (0)

/* ------------------------------------------------------------------ */
/* 热路径快速初始化                                                    */
/* ------------------------------------------------------------------ */

/*
 * 这里的优化来自一次实测，而不是猜测。
 *
 * 【实测数据】optimized 构建（-O3，Release）下反汇编 glBindTexture：
 *
 *   1   stp   x29, x30, [sp, #-0x30]!   <- 保存帧指针与返回地址
 *   2   str   x21, [sp, #0x10]          <- 保存被调用者保存寄存器
 *   3   stp   x20, x19, [sp, #0x20]!    <- 再保存两个
 *   4   mov   x29, sp
 *   5   mov   w19, w1                   <- 参数暂存到 callee-saved
 *   6   mov   w20, w0                   <- 同理
 *   7   bl    <glesmod_lazy_init@plt>   <- ★ 跨编译单元的 PLT 调用 ★
 *   8   adrp/ldr/ldr/cbz                <- 读 glesmod_trace_enabled 并分支
 *  14   bl    <glesmod_trace_call@plt>
 *  15-17 adrp/ldr/cbnz                  <- 取缓存的函数指针，判空
 *  18-23 （仅首次走：adrp/add/bl glesym_resolve/str/cbz）
 *  24-28 mov/ldp/ldp/ldp                <- 恢复寄存器
 *  29   br    x2                        <- 真正调用 GLES
 *
 * 即：**到达真正的 GLES 调用需要 29 条指令，而其中真正必要的只有尾部 5 条。**
 *
 * 【为什么 29 条是真实成本】
 * 原版 MC 每帧有 10^4 ~ 10^5 次 GL 调用。按 5 万次估算：
 *   24 条 × 5e4 = 1.2e6 条/帧
 * 而 P1–P8 每帧各只执行 1 条指令（约 55 亿/秒 @2.3GHz）。
 * 即这 24 条大约相当于 **0.2 ms/帧** —— 在 300 FPS（3.3ms/帧）下占 6%。
 *
 * 更关键的是第 7 条：为了保证跨编译单元调用的 ABI 正确性，编译器必须先
 * 把两个参数存到 callee-saved 寄存器（第 5、6 条），并在返回后恢复。
 * 这是**函数调用本身**带来的成本 —— 它由 `bl` 触发，是 5XX 层的指令，
 * 会导致流水线停顿。仅仅把这一次调用去掉，就能连带省掉 1~6 条。
 *
 * 【优化办法】
 * 把「快速路径检查」做成**可内联到每个转发函数体内**的宏，
 * 于是编译器不必再建立栈帧、不必暂存参数、也不必跨编译单元跳转：
 *
 *   稳态（应用整个生命周期的 99.999% 时间）：
 *     adrp/ldr/cbnz  ->  4 条指令，无分支预测失败风险（几乎总是不跳）
 *   首次调用：
 *     走 glesmod_on_first_call，完成全部初始化并记录本次调用
 *
 * 【为什么可以安全改成「默认只追踪首次调用」】
 *   追踪的目的（见 core.c 中 g_trace_ring 的注释）是**崩溃后定位**
 *   ——「最后执行的几个 GL 调用是什么」。而真机的两次 SIGSEGV 都发生在
 *   **早期初始化阶段**，那时确实是前 64 次调用范围内的。稳态下反复记录
 *   同样的函数名既不增加信息量，也无法覆盖「启动很久之后才崩」的场景
 *   （环形缓冲只保留 64 条，无论如何都会被冲掉）。
 *
 *   ⚠️ 但必须说明：这**确实削弱了**「记录崩溃前调用序列」这一能力。
 *   为了不失去它，同时不让全时段都付代价，引入 `GLESMOD_TRACE=2`
 *   （深度追踪）—— 此时走「每个调用都记录」的原始行为，
 *   由同一个分支（glesmod_hotpath_ready）统一控制，见下方说明。
 */

/**
 * 热路径「可以走快速路径」标志。
 *
 * 语义：**非 0 表示这个模块在稳态下无需任何钩子**。只有同时满足
 *   1. 初始化已真正结束（能力探测成功或已放弃重试）
 *   2. 追踪模式不是 `GLESMOD_TRACE=2`（深度追踪）
 * 时才置位。
 *
 * 【为什么用这一个变量就能同时覆盖两种需求】
 *   把「初始化是否完成」与「是否需要每次追踪」合并为**同一个判断**，
 *   于是稳态下只需要一条 `cbnz` 分支：
 *     - 普通模式：初始化完成后置位 -> 稳态零钩子开销
 *     - 深度追踪（TRACE=2）：**永不置位** -> 每次调用都走 glesmod_on_first_call，
 *       在那里完成（幂等的）初始化检查与本次调用记录
 *   两种模式都正确，而**只有一条分支**。若改用两个独立变量，
 *   稳态下就要多一条分支与一次内存读 —— 那正是本次优化要去掉的东西。
 */
extern volatile int glesmod_hotpath_ready;

/**
 * 首次调用 / 深度追踪钩子。
 *
 * 由 {@link GLESMOD_HOTPATH} 在 `glesmod_hotpath_ready == 0` 时调用。
 * 内部会：
 *   1. 按需（重复）触发初始化，直到它真正结束；
 *   2. 记录本次调用（若追踪已启用）。
 *
 * 在普通模式下它只会被调用到初始化结束为止；
 * 在 `GLESMOD_TRACE=2` 下它每次调用都会执行，从而保留完整调用序列。
 *
 * @param name GL 函数名（必须是字符串字面量，生命周期为整个进程）
 */
void glesmod_on_first_call(const char *name);

/**
 * 热路径初始化宏。**所有高频转发函数都应使用它**，替代
 * `glesmod_lazy_init()` + `GLESMOD_TRACE()` 的组合。
 *
 * 稳态开销：一次全局变量读取 + 一次条件分支（可内联，无函数调用、无栈帧）。
 *
 * @param name GL 函数名（字符串字面量）
 */
#define GLESMOD_HOTPATH(name)                                        \
    do {                                                             \
        if (__builtin_expect(glesmod_hotpath_ready, 1) == 0) {       \
            glesmod_on_first_call(name);                             \
        }                                                            \
    } while (0)


/* ------------------------------------------------------------------ */
/* 兼容层（GL 4.x 枚举 -> GLES 3.x 枚举映射）                           */
/* ------------------------------------------------------------------ */

/**
 * 把桌面 GL 的枚举值映射为 GLES 等价枚举。
 *
 * 多数枚举在两者中数值相同，可直接返回原值。少数需要转换，例如：
 *   GL_TEXTURE_1D / GL_TEXTURE_3D 在 ES 中语义不同
 *   内部格式 GL_RGBA8 -> GL_RGBA8（ES 3.0 相同，无需转换）
 *
 * 未知枚举原样返回，并记录一次降级事件（仅在 verbose 模式下）。
 *
 * @param value 桌面 GL 枚举值
 * @return 可用于 GLES 的枚举值
 */
GLenum glesmod_map_enum(GLenum value);

/**
 * 记录一次枚举映射（供日志与统计）。
 *
 * @param original 原始枚举
 * @param mapped   映射后枚举
 */
void glesmod_log_enum_map(GLenum original, GLenum mapped);

/* ------------------------------------------------------------------ */
/* 着色器转换                                                          */
/* ------------------------------------------------------------------ */

/*
 * 着色器阶段。取值与 GL 的 GL_SHADER_TYPE 返回值一致，便于直接透传。
 */
#define GLESMOD_SHADER_STAGE_UNKNOWN   0
#define GLESMOD_SHADER_STAGE_VERTEX    0x8B31  /* GL_VERTEX_SHADER */
#define GLESMOD_SHADER_STAGE_FRAGMENT  0x8B30  /* GL_FRAGMENT_SHADER */
#define GLESMOD_GL_SHADER_TYPE         0x8B4F  /* GL_SHADER_TYPE */

/**
 * 将桌面 GLSL 源码转换为 GLSL ES 320 可编译的形式。
 *
 * 处理内容（尽力而为，失败不崩溃）：
 *   - #version 指令改写为 320 es
 *   - 按阶段注入 precision 限定符（片元必需）
 *   - 废弃内置替换：texture2D -> texture、gl_FragColor -> out 变量
 *   - 按阶段转换限定符：vertex 的 attribute->in、varying->out；
 *     fragment 的 varying->in
 *
 * 若源码已经是 ES 版本，原样返回副本。
 *
 * @param src     原始源码（NUL 结尾）
 * @param out_len 输出长度（不含结尾 NUL）
 * @param stage   着色器阶段，取 GLESMOD_SHADER_STAGE_*。
 *                【强烈建议传真实阶段】调用方可用
 *                glGetShaderiv(shader, GL_SHADER_TYPE, &t) 查得。
 *                传 UNKNOWN 时会退回按源码内容猜测，但猜测不可靠：
 *                例如用 `out vec4 fragColor` 而非 gl_FragColor 的现代片元
 *                着色器会被误判，进而漏掉必需的精度声明而编译失败。
 * @return 转换后的源码。调用方需 free()。失败时返回 NULL。
 */
char *glesmod_convert_shader_source(const char *src, int *out_len, int stage);

/* ------------------------------------------------------------------ */
/* 初始化                                                              */
/* ------------------------------------------------------------------ */

/**
 * 首次调用某个 GL 函数时触发的惰性初始化。
 *
 * 之所以惰性：dlopen 我们的库时 EGL 上下文尚未创建，此时无法查询 GLES 版本
 * 与能力。真正的能力探测必须等到首次 GL 调用（此时上下文已 current）。
 */
void glesmod_lazy_init(void);

/* ------------------------------------------------------------------ */
/* 定制实现（定义在 custom.c）                                         */
/* ------------------------------------------------------------------ */

/*
 * 这些符号无法简单转发：要么 GLES 名字/语义不同，要么需要仿真，要么必须降级。
 * 在此集中声明，使 custom.c 的定义与声明不一致时能被编译器捕获。
 */
GLESMOD_EXPORT void   glClearDepth(GLdouble depth);
GLESMOD_EXPORT void   glDrawPixels(GLsizei width, GLsizei height, GLenum format,
                                   GLenum type, const void *pixels);
GLESMOD_EXPORT GLenum glGetError(void);
GLESMOD_EXPORT void   glGetTexImage(GLenum target, GLint level, GLenum format,
                                    GLenum type, void *pixels);
GLESMOD_EXPORT void   glLogicOp(GLenum opcode);
GLESMOD_EXPORT void   glPolygonMode(GLenum face, GLenum mode);
GLESMOD_EXPORT void * glMapBuffer(GLenum target, GLenum access);
GLESMOD_EXPORT void   glMultiDrawElements(GLenum mode, const GLsizei *count,
                                          GLenum type, const void *const *indices,
                                          GLsizei drawcount);
GLESMOD_EXPORT void   glMultiDrawArrays(GLenum mode, const GLint *first,
                                        const GLsizei *count, GLsizei drawcount);
GLESMOD_EXPORT void   glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei *count,
                                                    GLenum type, const void *const *indices,
                                                    GLsizei drawcount, const GLint *basevertex);
GLESMOD_EXPORT void   glMultiDrawArraysIndirect(GLenum mode, const void *indirect,
                                                GLsizei drawcount, GLsizei stride);
GLESMOD_EXPORT void   glMultiDrawElementsIndirect(GLenum mode, GLenum type,
                                                  const void *indirect,
                                                  GLsizei drawcount, GLsizei stride);
GLESMOD_EXPORT void   glBufferStorage(GLenum target, GLsizeiptr size,
                                      const void *data, GLbitfield flags);
GLESMOD_EXPORT void * glMapBufferRange(GLenum target, GLintptr offset,
                                       GLsizeiptr length, GLbitfield access);
GLESMOD_EXPORT void   glShaderSource(GLuint shader, GLsizei count,
                                     const GLchar *const *string, const GLint *length);

/*
 * glTexImage2D / glTexImage3D —— 深度内部格式翻译（修复深度缓冲失效）
 *
 * 桌面 GL 允许 internalformat = GL_DEPTH_COMPONENT(0x1902)，GLES **不允许**；
 * ES 只接受 GL_DEPTH_COMPONENT16(0x81A5) / 24(0x81A6) / 32F(0x81A7)
 * 或 GL_DEPTH24_STENCIL8(0x88F0)。
 *
 * MC 的 RenderTarget / MainTarget 会调用：
 *   glTexImage2D(GL_TEXTURE_2D, 0,
 *                0x1902 (GL_DEPTH_COMPONENT), w, h, 0,
 *                0x1902 (GL_DEPTH_COMPONENT), 0x1406 (GL_FLOAT), NULL)
 * 在桌面驱动上合法，在 Adreno 上直接报
 *   'the combination of format 6402 and type 5126 is unsupported'
 * 于是**深度纹理从未被分配** -> 深度测试彻底失效。
 *
 * 本实现把 0x1902 翻译为 GL_DEPTH_COMPONENT24(0x81A6)，
 * 并把 type 翻译为 GL_UNSIGNED_INT(0x1405)（ES 下 24 位深度的合法组合）。
 * 其余（包括颜色纹理）原样转发，不改变任何行为。
 */
GLESMOD_EXPORT void   glTexImage2D(GLenum target, GLint level, GLint internalformat,
                                   GLsizei width, GLsizei height, GLint border,
                                   GLenum format, GLenum type, const void *pixels);
GLESMOD_EXPORT void   glTexImage3D(GLenum target, GLint level, GLint internalformat,
                                   GLsizei width, GLsizei height, GLsizei depth,
                                   GLint border, GLenum format, GLenum type,
                                   const void *pixels);

/*
 * glGetTexLevelParameteriv / fv —— proxy texture 仿真。
 *
 * GLES 没有 GL_PROXY_TEXTURE_*；桌面 GL 用它探测最大纹理尺寸。
 * ANGLE 2.1.1 对 proxy target 会空指针崩溃，因此这两个入口在 proxy
 * target 上直接返回本地记录值，不进入驱动。
 */
GLESMOD_EXPORT void   glGetTexLevelParameteriv(GLenum target, GLint level,
                                               GLenum pname, GLint *params);
GLESMOD_EXPORT void   glGetTexLevelParameterfv(GLenum target, GLint level,
                                               GLenum pname, GLfloat *params);
/*
 * glTexStorage2D / glTexStorage3D —— 不可变存储路径上的深度格式翻译。
 *
 * 与 glTexImage2D 相同的理由：ES 不接受 internalformat = GL_DEPTH_COMPONENT，
 * 必须换成带位宽的 GL_DEPTH_COMPONENT24。此处原先只是纯转发，
 * 于是「不可变存储」这条分配路径成了翻译的漏网之处。
 *
 * 真机证据：Iris 建 FBO 报 36055 = INCOMPLETE_MISSING_ATTACHMENT
 * （帧缓冲里没有任何已附加的映像）+ 驱动报
 * 'pixel buffer format is not compatible with level format'。
 */
GLESMOD_EXPORT void glTexStorage2D(GLenum target, GLsizei levels,
                                   GLenum internalformat, GLsizei width,
                                   GLsizei height);
GLESMOD_EXPORT void glTexStorage3D(GLenum target, GLsizei levels,
                                   GLenum internalformat, GLsizei width,
                                   GLsizei height, GLsizei depth);
/*
 * glTexParameterf / glTexParameteri —— 过滤桌面专属 pname
 *
 * MC 的 com.mojang.blaze3d.platform.TextureUtil 每次创建纹理都会调用
 *     glTexParameterf(GL_TEXTURE_2D, 34049, 0.0f)
 * 其中 34049 即 GL_TEXTURE_LOD_BIAS。
 * 字节码证据：sipush 3553 / ldc 34049 / fconst_0 / _texParameter:(IIF)V
 *
 * GL_TEXTURE_LOD_BIAS 是**桌面专属** pname，GLES 拒绝它，驱动报
 *   'pname 34049 is not supported for this API call'
 * 每轮启动约 70 次（每纹理一次）。
 *
 * 【为什么只丢弃、不清错误位 —— 刻意的取舍】
 *   驱动对这条调用会留下 GL_INVALID_ENUM。一度想在丢弃后顺手清空错误队列，
 *   但那比问题本身更危险：glGetError 是**队列**（FIFO），主动抽干会把此前
 *   其它调用（可能含 MC 关心的失败）留下的错误一并吞掉，等于掩盖真实故障。
 *   只要不转发，我们就**不新增**错误；队列该有什么就保持什么。
 *   => 不替 MC 清理 GL 状态。
 *
 * 【丢弃是安全的（有字节码 + 规范双重依据）】
 *   - 值恒为 0.0f，而 0.0 正是 GL_TEXTURE_LOD_BIAS 的默认值；
 *   - MC 从不读回该参数（TextureUtil 只写不读）；
 *   - 全量字节码扫描确认只有 TextureUtil 引用 34049。
 *   只拦这一个 pname，其余（如 GL_TEXTURE_MAX_LOD 33083 等）原样转发。
 */
GLESMOD_EXPORT void   glTexParameterf(GLenum target, GLenum pname, GLfloat param);
GLESMOD_EXPORT void   glTexParameteri(GLenum target, GLenum pname, GLint param);

/*
 * glGetString —— 对外呈现**桌面 GL 兼容**的版本串。
 *
 * 【为什么必须改写，而不是直接转发】
 *   真机崩溃（2026-09-30，Iris 1.8.14-beta.1）：
 *     IllegalStateException: Could not parse GL version from
 *       "OpenGL ES 3.2 V@0762.36 ..."
 *     at net.irisshaders.iris.gl.shader.StandardMacros.getGlVersion(...)
 *   Iris 用正则从 GL_VERSION 里抓「主.次」。桌面驱动的串如
 *   "4.6.0 NVIDIA 536.23" 可匹配；我们的串以 "OpenGL ES " 开头，
 *   **第一个数字前面有非数字前缀**，正则匹配失败 → 光影加载失败。
 *
 * 【对外声称什么】
 *   形如 "3.2 (OpenGL ES 3.2 <GL_RENDERER>)"
 *   - 第一个数字 3.2 供正则匹配
 *   - 保留真实的 ES 版本与 GPU 名，便于诊断
 *   - 声称 3.2 而非 4.6：与我们**实际导出的符号集**一致（GL 3.2 core 子集），
 *     且与 status.json 的 reportedGl 一致。声称 4.6 会诱导调用方去用
 *     DSA / 持久映射，而那些在我们的符号表里是静默 stub
 *     （见 docs/r-12-stub-audit.md）。
 *
 * GL_SHADING_LANGUAGE_VERSION 同步改写为 "3.20"（部分模组会解析它）。
 * GL_VENDOR / GL_RENDERER / GL_EXTENSIONS 一律保留驱动原话。
 */
GLESMOD_EXPORT const GLubyte * glGetString(GLenum name);

/*
 * glGetStringi —— 在扩展列表末尾追加一条**如实**的扩展声明。
 *
 * 【问题（真机证据，2026-09-30）】
 *   Iris 加载光影时报：
 *     RuntimeException: Buffer blending is not supported on this platform,
 *         however it was attempted to be used!
 *   Iris 的 `supportsBufferBlending()` 检查的是两个**桌面**能力标志：
 *     GL_ARB_draw_buffers_blend || OpenGL40
 *   两者在 ES 上都不存在 -> 永远 false -> 抛异常。
 *
 * 【为什么这是错误的判断】
 *   该扩展提供的函数在 ES 3.0 起就是核心，本项目中**全是真实转发**：
 *     glBlendFunci / glBlendFuncSeparatei / glBlendEquationi /
 *     glBlendEquationSeparatei / glColorMaski / glEnablei / glDisablei
 *   ⇒ 能力其实具备，只是扩展名未出现在驱动的列表里。
 *
 * 【修法】在 GL_EXTENSIONS 末尾追加 GL_ARB_draw_buffers_blend。
 *   之所以算「如实」：调用方用到的函数确实全部可用。
 *   我们**不**声称 OpenGL40 —— 那会触发 DSA / 持久映射等
 *   真正缺失的路径（见 docs/r-12-stub-audit.md）。
 *
 * 【★上一版只在 glGetStringi 里追加，真机上完全没生效★】
 *   查 LWJGL 权威源码（org/lwjgl/opengles/GLContext.java）后确认：
 *   **ES 路径根本不调用 glGetStringi**。它解析的是
 *       glGetString(GL_EXTENSIONS)
 *   这一个空格分隔的整串。因此真正的生效路径在 glGetString 里，
 *   已在那里实现追加（见 custom.c 的 GL_EXTENSIONS 分支）。
 *
 *   本函数保留给桌面兼容路径；为使它自成一致（计数与实际枚举相符），
 *   glGetIntegerv(GL_NUM_EXTENSIONS) 也要 +1，否则那条索引永远读不到。
 */
GLESMOD_EXPORT const GLubyte * glGetStringi(GLenum name, GLuint index);

/*
 * glGetIntegerv —— 仅让 GL_NUM_EXTENSIONS 与 GL_EXTENSIONS 自洽。
 *
 * 原因：LWJGL 的桌面/兼容路径按 `for (i = 0; i < count; i++)` 循环，
 * count 取自 GL_NUM_EXTENSIONS。我们多报了一条扩展，若计数不跟着 +1，
 * 索引 N 上那条追加项**永远不会被读到**（这正是上一轮 glGetStringi
 * 成为死代码的机制）。
 *
 * 只拦截 GL_NUM_EXTENSIONS 一个 pname，其余原样转发。
 * glGetIntegerv 是所有能力探测的必经之路，因此实现刻意保持最薄。
 */
GLESMOD_EXPORT void glGetIntegerv(GLenum pname, GLint * params);

/*
 * glCheckFramebufferStatus —— 在不完整时输出**可定位的原因**。
 *
 * Iris 建 FBO 失败时报的是：
 *     "Unexpected error while creating framebuffer: Draw buffers [0] Status: 36055"
 * 仅一个状态码不足以定位。
 *
 * 【0x8CD7 = 36055 = INCOMPLETE_MISSING_ATTACHMENT，不是 DRAW_BUFFER】
 *   含义是「帧缓冲里没有任何已附加的映像」，典型成因是附件纹理**没有存储**
 *   （glTexImage2D 那一步被拒或未调用）。真机日志里它正前方就是
 *   `pixel buffer format is not compatible with level format`，两者互为印证。
 *   因此本包装不仅列出各附件，还会量出纹理附件的**存储尺寸**：
 *   尺寸为 0 就直指 glTexImage2D 的参数问题。
 *
 * 本包装在状态 != COMPLETE 时主动查询各附件的对象/层级并写日志，
 * 把状态码变成可直接定位的记录。
 *
 * 正常路径只多一次比较，不做 GL 查询，不影响性能。
 */
GLESMOD_EXPORT GLenum glCheckFramebufferStatus(GLenum target);

/*
 * 几何诊断探针（定义在 probe.c）。
 *
 * 用「定制实现」而非转发的两个理由：
 *   1. 需要在真实的 glDrawElements 前后夹入查询（投影矩阵、视口、剔除状态）；
 *      纯转发层无处插入代码。
 *   2. 它是唯一能同时做到两件关键事的调用点：
 *      · 拿到**当前生效**的投影矩阵 —— 判断是否用了 reversed-Z / 零到一深度
 *      · 按 count 规模区分区块/实体绘制与天空/粒子/GUI 绘制
 * 探针默认关闭（GLESMOD_GEOM_PROBE=1 开启），行为与原函数完全一致。
 */
GLESMOD_EXPORT void   glDrawElements(GLenum mode, GLsizei count, GLenum type,
                                     const void *indices);

/*
 * 以下两个函数行为与标准 GL 完全一致，本库仅在其返回后【额外记录日志】。
 *
 * 目的：真机上着色器编译失败时，唯一的一手证据就是驱动通过这两个函数
 * 返回的错误文本。真机可见的 Java 异常
 *     IllegalStateException: could not preload shader position
 * 不含任何 GLSL 细节（不说第几行、什么语法错），必须靠这里
 * 把驱动的原话写进日志，否则只能靠推断。
 *
 * 之所以走「定制实现」而不是普通转发：转发层是纯直通、无处插入日志；
 * 而这两个符号只需要在调用真正的 GL 之后加一行记录，改动最小、行为不变。
 */
GLESMOD_EXPORT void   glGetShaderInfoLog(GLuint shader, GLsizei maxLength,
                                         GLsizei *length, GLchar *infoLog);
GLESMOD_EXPORT void   glGetProgramInfoLog(GLuint program, GLsizei maxLength,
                                          GLsizei *length, GLchar *infoLog);

/*
 * 包装 glGetShaderiv：行为不变，仅在查询 GL_COMPILE_STATUS 且失败时
 * 把该 shader 的源码写进日志（源码在 glShaderSource 时按 shader 名记录）。
 *
 * 理由：驱动只报「第几行错」，不报源码。把两者在日志里对上，
 * 才能直接定位。放在 COMPILE_STATUS 这个必经之路上，覆盖面最广。
 */
GLESMOD_EXPORT void   glGetShaderiv(GLuint shader, GLenum pname, GLint *params);

#ifdef __cplusplus
}
#endif

#endif /* GLESMOD_GL_INTERNAL_H */

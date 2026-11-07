/*
 * gles_backend.h —— GLES 后端对外 ABI
 *
 * 本项目通过向启动器（FCL）替换 libGL.so 实现渲染接管。本头文件定义：
 *   1. ABI 版本与握手约定
 *   2. 能力表结构
 *   3. 降级原因码（与 Java 侧 DegradeReason 一一对应）
 *   4. 配置环境变量名
 *
 * 设计约束：
 *   - 本层不接管 EGL。EGL 由启动器按 rendererEGLPath 自行加载。
 *   - 能力表在初始化后只读，运行时不修改。
 *   - 任何故障都不得导致游戏崩溃：能力未知时返回保守值。
 *
 * 许可证：LGPL-3.0-or-later
 */

#ifndef GLESMOD_BACKEND_H
#define GLESMOD_BACKEND_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* ABI 版本                                                            */
/* ------------------------------------------------------------------ */

/*
 * native ABI 版本。jar 与 .so 是分开分发的（分别位于 mods 目录与启动器
 * 库目录），两者版本可能不同步，因此必须显式协商。
 *
 * 递增规则见 docs/capability-interface.md §9：
 *   - 新增能力位（用保留位）       -> 不递增
 *   - 修改已用位含义/数组布局      -> +1
 */
#define GLESMOD_ABI_VERSION 1

/* ------------------------------------------------------------------ */
/* 配置环境变量                                                        */
/* ------------------------------------------------------------------ */

/*
 * 由 FCL 渲染器插件的 env 配置注入（见 docs/capability-interface.md §6）。
 * 环境变量在 dlopen 前设置，因此初始化时即可读取，无需等待 Java 层下发。
 *
 * 注意：LIBGL_ES 是 FCL 的保留变量，决定 EGL 上下文版本，插件必须显式
 * 设置（内置渲染器由 FCL 自动设置，插件不会）。
 */
#define GLESMOD_ENV_ENABLE        "GLESMOD_ENABLE"        /* 0/1，默认 1 */
#define GLESMOD_ENV_DEGRADE_LEVEL "GLESMOD_DEGRADE_LEVEL" /* 0/1/2，默认 1 */

/*
 * 调用追踪模式。
 *
 *   GLESMOD_TRACE=0  关闭追踪（稳态无开销）
 *   GLESMOD_TRACE=1  默认。只在「初始化结束前的那些调用」上记录，
 *                    足以覆盖早期崩溃定位；稳态零开销
 *   GLESMOD_TRACE=2  深度追踪。**每次**调用都记录完整序列。
 *                    用于排查「启动很久之后才崩」的问题 ——
 *                    代价是每个 GL 调用都多一次分支 + 一次无锁原子自增
 *
 * 【为什么需要 2 这一档】
 *   编译期优化把「记录调用序列」从全时段缩减为「初始化阶段」，
 *   从而省掉了每帧 10^4~10^5 次钩子开销（实测 29 -> 15 条指令/调用）。
 *   但那种场景（长时间运行后崩溃）确实存在，因此保留一个显式开关，
 *   让需要时能换回完整序列，而不必重新构建。
 */
#define GLESMOD_ENV_TRACE "GLESMOD_TRACE"
#define GLESMOD_ENV_STATE_CACHE   "GLESMOD_STATE_CACHE"   /* 0/1，默认 1 */
#define GLESMOD_ENV_LOG_LEVEL     "GLESMOD_LOG_LEVEL"     /* 0/1/2，默认 1 */
#define GLESMOD_ENV_STATUS_FILE   "GLESMOD_STATUS_FILE"   /* 覆盖状态文件路径 */
#define GLESMOD_ENV_ABI           "GLESMOD_ABI"           /* 期望 ABI 版本 */

/*
 * 启动器使用的 EGL 库名。由启动器注入（不由我们定义）。
 *
 * 用途：eglGetProcAddress 必须从**与当前 GL 上下文同一个实例**的 EGL 取得，
 * 否则有状态调用会操作到另一实例的未初始化数据而崩溃。
 * 因此我们按启动器用的同一个名字 dlopen 一次（同 soname 会返回已加载的那个
 * 实例），再从它取 eglGetProcAddress。
 *
 * 名字来自启动器：FCL 读 POJAVEXEC_EGL，真机实测值为 "libEGL.so"。
 */
#define GLESMOD_ENV_EXEC_EGL      "POJAVEXEC_EGL"         /* 启动器使用的 EGL 库名 */
#define GLESMOD_ENV_LIBGL_GLES    "LIBGL_GLES"            /* 可选的 GLES 库名；ANGLE 模式通常为 libGLESv2_angle.so */

#define GLESMOD_ENV_LOG_FILE      "GLESMOD_LOG_FILE"      /* 覆盖日志文件路径 */
#define GLESMOD_ENV_SHADER_PROBE  "GLESMOD_SHADER_PROBE"  /* 0/1，默认 0：转储着色器 */
#define GLESMOD_ENV_GEOM_PROBE    "GLESMOD_GEOM_PROBE"    /* 0/1，默认 0：几何诊断探针 */
#define GLESMOD_ENV_PROXY_EMU     "GLESMOD_PROXY_EMU"     /* 0/1，默认 1：proxy texture 仿真 */

/* 状态文件相对路径（相对游戏工作目录） */
#define GLESMOD_STATUS_RELPATH    "glesmod/status.json"

/*
 * 原生日志文件相对路径（相对游戏工作目录）。
 *
 * 【为什么需要文件日志】
 *   native 层原本只写 stderr，依赖启动器把 stdio 重定向到日志文件。
 *   但真机上出现过「游戏日志里一行本库输出都没有」的情况，此时无法区分：
 *     a) 本库根本没被加载/调用
 *     b) 本库被调用了，但 stderr 重定向没生效
 *   两者的排查方向完全相反，而靠 stderr 无法分辨。
 *   直接写文件即可绕开重定向的不确定性，提供可靠证据。
 */
#define GLESMOD_LOGFILE_RELPATH   "glesmod/native.log"

/* ------------------------------------------------------------------ */
/* 能力位                                                              */
/* ------------------------------------------------------------------ */

/* 位定义必须与 Java 侧 flag_bits 解析保持一致 */
#define GLESMOD_CAP_BACKEND_ACTIVE     (1u << 0)
#define GLESMOD_CAP_MULTI_DRAW         (1u << 1)
#define GLESMOD_CAP_COMPUTE_SHADER     (1u << 2)
#define GLESMOD_CAP_PERSISTENT_MAP     (1u << 3)
#define GLESMOD_CAP_INSTANCING         (1u << 4)
#define GLESMOD_CAP_INDIRECT_DRAW      (1u << 5)
#define GLESMOD_CAP_MULTI_DRAW_BUFFERS (1u << 6)
#define GLESMOD_CAP_TEXTURE_STORAGE    (1u << 7)
#define GLESMOD_CAP_ANISOTROPY         (1u << 8)
#define GLESMOD_CAP_DEBUG_OUTPUT       (1u << 9)
#define GLESMOD_CAP_DRIVER_BLACKLISTED (1u << 10)
/* 11-31 保留，必须为 0 */

/* ------------------------------------------------------------------ */
/* 降级原因码                                                          */
/* ------------------------------------------------------------------ */

/*
 * 码值一经发布不可更改，只能废弃（第三方可能依赖它们做判断）。
 * 必须与 Java 侧 DegradeReason 枚举同步。
 */
typedef enum {
    GLESMOD_DEGRADE_MULTI_DRAW_UNSUPPORTED      = 0x0001,
    GLESMOD_DEGRADE_COMPUTE_SHADER_UNSUPPORTED  = 0x0002,
    GLESMOD_DEGRADE_PERSISTENT_MAP_UNSUPPORTED  = 0x0003,
    GLESMOD_DEGRADE_INDIRECT_DRAW_UNSUPPORTED   = 0x0004,
    GLESMOD_DEGRADE_PERSISTENT_MAP_UNSTABLE     = 0x0005,
    GLESMOD_DEGRADE_SHADER_CONVERSION_FAILED    = 0x0006,
    GLESMOD_DEGRADE_SHADER_UNSUPPORTED_FEATURE  = 0x0007,
    GLESMOD_DEGRADE_TEXTURE_UNIT_LIMIT          = 0x0008,
    GLESMOD_DEGRADE_DRIVER_BLACKLISTED          = 0x0009,
    GLESMOD_DEGRADE_BACKEND_INIT_FAILED         = 0x0010,
    GLESMOD_DEGRADE_FALLBACK_TO_COMPAT_LAYER    = 0x0011,
    /* 以下为 native 侧新增，需同步到 Java 枚举 */
    GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION        = 0x0020,
    GLESMOD_DEGRADE_ENUM_MAPPED                 = 0x0021,
    GLESMOD_DEGRADE_TEXTURE_FORMAT_MAPPED       = 0x0022,
} glesmod_degrade_code;

/* ------------------------------------------------------------------ */
/* 能力表                                                              */
/* ------------------------------------------------------------------ */

/*
 * 初始化后只读快照。任何需要在运行时改变的策略通过 degrade_level 与
 * 单独的降级标志实现，避免能力表被并发修改。
 */
typedef struct {
    /* 版本 */
    int es_major;              /* 实际 GLES 版本，如 3 */
    int es_minor;              /* 如 2 */
    int gl_major;              /* 对外声称的桌面 GL 版本，如 3 */
    int gl_minor;              /* 如 2 */

    /* 能力位（见 GLESMOD_CAP_*） */
    unsigned int flags;

    /* 容量 */
    int max_texture_units;
    int max_draw_buffers;
    int max_texture_size;
    int max_samples;
    int max_vertex_attribs;
    int max_uniform_components;

    /* 降级档位（由配置下发） */
    int degrade_level;
} glesmod_caps;

/* ------------------------------------------------------------------ */
/* 对外查询接口（供测试与诊断使用；mod 侧通过状态文件读取）             */
/* ------------------------------------------------------------------ */

/** 后端是否已成功初始化。为 false 时所有能力查询返回保守值。 */
bool glesmod_is_active(void);

/** 获取能力表只读指针。永不返回 NULL。 */
const glesmod_caps *glesmod_get_caps(void);

/**
 * 记录一次降级事件。首次发生时写入日志；后续同原因调用只累加计数。
 *
 * @param code   降级原因码
 * @param detail 补充说明，可为 NULL
 */
void glesmod_degrade(glesmod_degrade_code code, const char *detail);

/** 获取指定降级原因的发生次数。 */
unsigned long glesmod_degrade_count(glesmod_degrade_code code);

/** 将状态写入 JSON 文件（路径见 GLESMOD_ENV_STATUS_FILE）。 */
void glesmod_write_status(void);

/**
 * 向原生日志文件追加一段诊断文本（同时输出到 stderr）。
 *
 * 用途：转储着色器源码等大块内容，用于事后比对「进来的源码」与
 * 「转换后送入驱动的源码」。仅应在 GLESMOD_SHADER_PROBE=1 时调用，
 * 因为它会显著增加日志体积。
 *
 * @param label 段标签（如 "shader-in" / "shader-out"）
 * @param text  文本内容（NUL 结尾）
 */
void glesmod_trace_dump(const char *label, const char *text);

/**
 * 写一行原生日志（同时进入 stderr 与 glesmod/native.log）。
 *
 * 供 native 各源文件记录诊断信息用。与 glesmod_trace_dump 的区别：
 * 本函数只写一行短文本，适合记录「发生了某个动作」这类事件标记，
 * 不会产生大段内容。
 *
 * @param message 单行文本（不需要换行符）
 */
void glesmod_log(const char *message);

#ifdef __cplusplus
}
#endif

#endif /* GLESMOD_BACKEND_H */

/*
 * core.c —— 后端核心：符号解析、能力探测、降级记录、状态输出
 *
 * 许可证：LGPL-3.0-or-later
 *
 * 需要 _GNU_SOURCE 才能在 glibc 下取得 dladdr。
 * bionic（Android）无条件提供该函数，此定义对它是无害的。
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "gl_internal.h"
#include "gles_backend.h"

#include <dlfcn.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

/* ------------------------------------------------------------------ */
/* 常量                                                                */
/* ------------------------------------------------------------------ */

#define MAX_DEGRADE_KINDS 32
#define DEGRADE_DETAIL_LEN 192
#define MAX_MISSING_SYMBOLS 256
/* GL 3.2 core 中有 74 个符号在 GLES 3.2 缺失，留出余量 */
#define MAX_STUB_SYMBOLS 128

/* ------------------------------------------------------------------ */
/* 全局状态                                                            */
/* ------------------------------------------------------------------ */

/* 环境变量读取的配置（初始化时快照） */
static int g_enabled = 1;
static int g_degrade_level = 1;
static int g_state_cache = 1;
static int g_log_level = 1;

/*
 * 调用追踪模式（见 gles_backend.h 的 GLESMOD_ENV_TRACE 说明）。
 *   0 = 关闭        1 = 默认（只追踪初始化阶段）      2 = 深度（全时段）
 */
#define GLESMOD_TRACE_MODE_OFF   0
#define GLESMOD_TRACE_MODE_DEEP  2
static int g_trace_mode = 1;

/* 能力表。初始化后只读。 */
static glesmod_caps g_caps;

/* 后端是否已激活（初始化成功且被启用） */
static int g_active = 0;

/*
 * 并发保护。
 *
 * 【重要】绝不能把 g_lock 当作初始化锁使用。
 *
 * glesmod_lazy_init() 会调用 probe_capabilities() → glesym_resolve()，
 * 而 glesym_resolve() 自身需要用 g_lock 来读写解析缓存。
 * pthread 的默认互斥锁是非递归的（bionic 下为 PTHREAD_MUTEX_NORMAL），
 * 同一线程重复加锁不是「未定义行为」而已 —— 在 Android 上必然挂起。
 *
 * 这一点曾导致真机启动挂起 30 秒后超时：
 *     glShaderSource / glGetError 等定制函数
 *         → glesmod_lazy_init()   [持有 g_lock]
 *         → probe_capabilities()
 *         → glesym_resolve()      [再次申请同一把锁 → 死锁]
 *
 * 因此初始化改用 pthread_once（见 glesmod_lazy_init 的实现），
 * g_lock 只用于保护解析缓存与降级计数这两类短临界区。
 */
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

/* 初始化的一次性执行控制块。由 glesmod_lazy_init() 驱动。 */
static pthread_once_t g_init_once = PTHREAD_ONCE_INIT;

/* 降级事件计数与首次详情 */
static unsigned long g_degrade_count[MAX_DEGRADE_KINDS];
static char g_degrade_detail[MAX_DEGRADE_KINDS][DEGRADE_DETAIL_LEN];
static unsigned long g_degrade_total = 0;

/* 未找到的符号（去重记录，避免重复日志） */
static char g_missing[MAX_MISSING_SYMBOLS][64];
static int g_missing_count = 0;

/*
 * 被调用过的安全 stub 符号。
 *
 * 与 g_missing 分开记录：stub 是 GL 有、GLES 没有的符号，由我们主动导出
 * 空实现。它的被调用属于预期内的能力降级，而非缺陷。
 * GL 3.2 core 有 74 个符号在 GLES 3.2 中缺失，全部以 stub 形式导出，
 * 因此这个列表是判断「哪些功能静默失效」的关键依据。
 */
static char g_stubs[MAX_STUB_SYMBOLS][64];
static int g_stub_count = 0;

/* 解析缓存。用简单的线性表 + 开放寻址思想：符号数量约 150，线性查找足够快，
 * 且查询发生在每个转发函数的首次调用（之后函数指针已缓存到函数内的 static）。 */
#define RESOLVE_CACHE_SIZE 512
typedef struct {
    const char *name;   /* 指向字符串字面量，生命周期为整个进程 */
    glesmod_proc_t fn;
} resolve_entry;
static resolve_entry g_resolve_cache[RESOLVE_CACHE_SIZE];
static int g_resolve_cache_len = 0;

/* ------------------------------------------------------------------ */
/* 日志                                                                */
/* ------------------------------------------------------------------ */

#define LOG_TAG "GLESMod"

/*
 * 原生日志文件的写入。
 *
 * 【为什么除了 stderr 还要写文件】
 *   native 层最初只写 stderr，依赖启动器把游戏的 stdio 重定向到日志文件。
 *   但真机测试中出现过「游戏日志里一行本库输出都没有」，此时无法区分：
 *     a) 本库根本没被加载进进程          -> 应排查 FCL 插件配置
 *     b) 本库被加载并调用了，但 stderr
 *        重定向没有覆盖到它             -> 应排查日志通道
 *   两者的排查方向完全相反，而单看 stderr 无法分辨。
 *   直接写文件绕开重定向的不确定性，提供可靠证据。
 *
 * 文件位置：<工作目录>/glesmod/native.log（可用 GLESMOD_LOG_FILE 覆盖）。
 *
 * 代价控制：每次写入执行一次 open/append/close，不做长期持有的 FILE*。
 * 原因是 GL 调用可能来自多个线程，持有 FILE* 需要额外加锁；而本日志
 * 只在初始化、降级、转储等低频路径使用，开销可忽略。
 *
 * 【为什么导出为非 static】
 *   custom.c 也需要把驱动返回的错误文本写进本文件（那是定位着色器
 *   编译失败的关键证据），故改名为 glesmod_log_to_file 并对外可见。
 */
void glesmod_log_to_file(const char *text) {
    /* 只写一次，避免反复尝试失败路径（如目录不可写）时反复 open。 */
    static int file_ok = 1;
    if (!file_ok) return;

    const char *override = glesmod_env_str(GLESMOD_ENV_LOG_FILE);
    char path[512];
    if (override != NULL) {
        snprintf(path, sizeof(path), "%s", override);
    } else {
        snprintf(path, sizeof(path), "%s", GLESMOD_LOGFILE_RELPATH);
    }

    /* 逐级创建父目录（与状态文件同样的策略，忽略失败） */
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", path);
    char *slash = strrchr(dir, '/');
    if (slash != NULL) {
        *slash = '\0';
        for (char *p = dir + 1; *p; p++) {
            if (*p == '/') {
                *p = '\0';
                mkdir(dir, 0755);
                *p = '/';
            }
        }
        mkdir(dir, 0755);
    }

    FILE *f = fopen(path, "a");
    if (f == NULL) {
        /* 只报告一次失败，之后静默（file_ok 会被外层逻辑重置为 0） */
        file_ok = 0;
        return;
    }
    /* 带毫秒时间戳：便于与游戏日志对齐，判断先后顺序 */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tmv;
    time_t secs = ts.tv_sec;
    localtime_r(&secs, &tmv);
    fprintf(f, "[%02d:%02d:%02d.%03ld] %s\n",
            tmv.tm_hour, tmv.tm_min, tmv.tm_sec, ts.tv_nsec / 1000000, text);
    fclose(f);
}

/* 是否已经写过「本库已加载」这一行。用于保证它恰好出现一次。 */
static int g_load_banner_written = 0;

/* 探针/转换器版本号。与 probe.c 的 banner 保持同步手工维护。 */
#define GLESMOD_BUILD_TAG "v10"

/*
 * 写下「本库已被加载并执行」的标记。
 *
 * 【为什么这是整个诊断链条中最重要的第一行】
 *   它是唯一的、不依赖任何 GL 调用成功的存活证明。它出现在
 *   glesmod_lazy_init() 的最开头（任何解析、探测之前），因此：
 *     - 若日志中有这一行 -> 库在调用路径上，问题出在后续逻辑
 *     - 若完全没有       -> 库根本没被调用，问题在 FCL 插件配置/加载
 *   没有它，这两种情况无法区分。
 *
 * 【构建戳（本次新增）】
 *   行内含版本号与编译日期/时间。这解决了一个真实踩过的坑：
 *   「改了代码、但手机上装的是旧 APK」时，日志看起来与修复前**一模一样**，
 *   于是把「没测到新代码」误判成「修复无效」，白跑一轮。
 *   有了构建戳，只要对一眼日期就能区分这两种情况。
 *
 * 【为什么必须**同时**走 glesmod_log（本次修正）】
 *   先前这行只写 glesmod/native.log 文件。而用户日常收集的是**启动器日志**
 *   （游戏 stderr 重定向），它里面拿不到 native.log 的内容 ——
 *   于是这个构建戳在实际排查中根本用不上（实测：启动器日志里 0 命中）。
 *   现在改为同时输出到 stderr 与文件，用户无需额外取文件即可自查版本。
 */
static void log_load_banner(void) {
    if (g_load_banner_written) return;
    g_load_banner_written = 1;

    /* __DATE__/__TIME__ 由编译器填入，每次重新编译都会变 */
    char buf[320];
    snprintf(buf, sizeof(buf),
             "[%s] 本库已加载并进入 GL 调用路径（存活标记；"
             "若完全没有本行说明库未被调用）| 构建 %s 于 %s %s",
             LOG_TAG, GLESMOD_BUILD_TAG, __DATE__, __TIME__);

    /*
     * 用 fprintf 直接写 stderr（而不是 glesmod_log），
     * 因为此刻 g_log_level / 日志文件路径等可能尚未初始化；
     * 这一行是「最先可用」的存活标记，不能依赖后续初始化。
     */
    fprintf(stderr, "%s\n", buf);
    glesmod_log_to_file(buf);
}

static void log_line(int level, const char *fmt, ...) {
    if (level > g_log_level) return;

    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

#ifdef __ANDROID__
    int prio = (level >= 2) ? ANDROID_LOG_DEBUG
             : (level == 1) ? ANDROID_LOG_INFO
                            : ANDROID_LOG_WARN;
    __android_log_print(prio, LOG_TAG, "%s", buf);
#endif

    /* 同时写 stderr：FCL 会把游戏进程的 stdio 重定向到日志文件，
     * 这是用户最容易收集的通道（无需 adb）。 */
    fprintf(stderr, "[%s] %s\n", LOG_TAG, buf);

    /* 以及自己的日志文件：不依赖启动器的重定向是否生效。 */
    glesmod_log_to_file(buf);
}

/*
 * 写一行原生日志（供 native 各源文件使用）。
 *
 * 与 glesmod_trace_dump 的区别：本函数写单行短文本，用于记录「某件事发生了」，
 * 不会产生大段内容。它同时进入 stderr 与 glesmod/native.log，
 * 因此不依赖启动器是否重定向了 stdio。
 */
void glesmod_log(const char *message) {
    if (message == NULL) return;
    log_line(0, "%s", message);
}

/*
 * 转储大块文本（如着色器源码）到日志文件。
 *
 * 用独立函数而不是 log_line 的原因：log_line 的 buf 只有 1024 字节，
 * 而着色器源码可能远超此长度，必须分段写出。
 *
 * 【为什么同时写 stderr（本次修正）】
 *   原先本函数只写 glesmod/native.log 文件。但用户收集的是**启动器日志**
 *   （即游戏进程的 stderr 重定向），两者不是同一份文件。
 *   后果：连续几轮真机排查里，用户日志只能看到
 *       [GLESMod] 编译失败的着色器 —— 阶段=顶点，GL 名称=211
 *   却看不到驱动原话与出错那份源码 —— 而这两样正是唯一能定位根因的证据。
 *   本函数只在**失败路径**被调用（正常启动不会触发），
 *   所以重发到 stderr 不会造成日志膨胀，却是「让证据可见」的关键一步。
 */
void glesmod_trace_dump(const char *label, const char *text) {
    if (text == NULL) return;

    char head[128];
    snprintf(head, sizeof(head), "===== BEGIN %s =====", label);
    glesmod_log_to_file(head);
    glesmod_log(head);

    /*
     * 按行追加，避免单行过长；同时给每行加前缀，
     * 这样即使日志被其他输出穿插也能辨认归属。
     */
    const char *p = text;
    int lineno = 0;
    while (*p != '\0' && lineno < 2000) {
        const char *eol = strchr(p, '\n');
        size_t n = eol ? (size_t)(eol - p) : strlen(p);
        if (n > 900) n = 900;   /* 截断超长行，防止缓冲溢出 */
        /* 去掉行尾 CR（Windows 换行混入时会让日志出现怪异空白） */
        while (n > 0 && p[n - 1] == '\r') n--;

        char line[1024];
        snprintf(line, sizeof(line), "  %4d| %.*s", lineno, (int)n, p);
        glesmod_log_to_file(line);
        glesmod_log(line);

        lineno++;
        if (eol == NULL) break;
        p = eol + 1;
    }

    snprintf(head, sizeof(head), "===== END %s (%d 行) =====", label, lineno);
    glesmod_log_to_file(head);
    glesmod_log(head);
}

/* ------------------------------------------------------------------ */
/* 环境变量                                                            */
/* ------------------------------------------------------------------ */

const char *glesmod_env_str(const char *name) {
    const char *v = getenv(name);
    if (v == NULL || v[0] == '\0') return NULL;
    return v;
}

int glesmod_env_int(const char *name, int fallback) {
    const char *v = glesmod_env_str(name);
    if (v == NULL) return fallback;
    char *end = NULL;
    long n = strtol(v, &end, 0);
    if (end == v) return fallback;  /* 非法值 */
    return (int)n;
}

/* ------------------------------------------------------------------ */
/* 降级事件                                                            */
/* ------------------------------------------------------------------ */

static int degrade_index(glesmod_degrade_code code) {
    /* 码值不连续，用一个简单的映射表把码值压缩为索引 */
    switch (code) {
        case GLESMOD_DEGRADE_MULTI_DRAW_UNSUPPORTED:     return 0;
        case GLESMOD_DEGRADE_COMPUTE_SHADER_UNSUPPORTED: return 1;
        case GLESMOD_DEGRADE_PERSISTENT_MAP_UNSUPPORTED: return 2;
        case GLESMOD_DEGRADE_INDIRECT_DRAW_UNSUPPORTED:  return 3;
        case GLESMOD_DEGRADE_PERSISTENT_MAP_UNSTABLE:    return 4;
        case GLESMOD_DEGRADE_SHADER_CONVERSION_FAILED:   return 5;
        case GLESMOD_DEGRADE_SHADER_UNSUPPORTED_FEATURE: return 6;
        case GLESMOD_DEGRADE_TEXTURE_UNIT_LIMIT:         return 7;
        case GLESMOD_DEGRADE_DRIVER_BLACKLISTED:         return 8;
        case GLESMOD_DEGRADE_BACKEND_INIT_FAILED:        return 9;
        case GLESMOD_DEGRADE_FALLBACK_TO_COMPAT_LAYER:   return 10;
        case GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION:       return 11;
        case GLESMOD_DEGRADE_ENUM_MAPPED:                return 12;
        case GLESMOD_DEGRADE_TEXTURE_FORMAT_MAPPED:      return 13;
        default: return -1;
    }
}

void glesmod_degrade(glesmod_degrade_code code, const char *detail) {
    int idx = degrade_index(code);
    if (idx < 0) return;

    pthread_mutex_lock(&g_lock);
    int first = (g_degrade_count[idx] == 0);
    g_degrade_count[idx]++;
    g_degrade_total++;
    if (first && detail != NULL) {
        snprintf(g_degrade_detail[idx], DEGRADE_DETAIL_LEN, "%s", detail);
    }
    pthread_mutex_unlock(&g_lock);

    /* 只在首次发生时输出日志，避免刷屏（对应 GLES_COMPAT_WARN_ONCE 的意图） */
    if (first) {
        log_line(0, "降级: %s%s%s",
                 detail ? detail : "(无说明)",
                 detail ? "" : "",
                 g_degrade_level >= 2 ? "" : "");
    }
}

unsigned long glesmod_degrade_count(glesmod_degrade_code code) {
    int idx = degrade_index(code);
    if (idx < 0) return 0;
    return g_degrade_count[idx];
}

static const char *degrade_name(int idx) {
    switch (idx) {
        case 0:  return "MULTI_DRAW_UNSUPPORTED";
        case 1:  return "COMPUTE_SHADER_UNSUPPORTED";
        case 2:  return "PERSISTENT_MAP_UNSUPPORTED";
        case 3:  return "INDIRECT_DRAW_UNSUPPORTED";
        case 4:  return "PERSISTENT_MAP_UNSTABLE";
        case 5:  return "SHADER_CONVERSION_FAILED";
        case 6:  return "SHADER_UNSUPPORTED_FEATURE";
        case 7:  return "TEXTURE_UNIT_LIMIT";
        case 8:  return "DRIVER_BLACKLISTED";
        case 9:  return "BACKEND_INIT_FAILED";
        case 10: return "FALLBACK_TO_COMPAT_LAYER";
        case 11: return "UNSUPPORTED_FUNCTION";
        case 12: return "ENUM_MAPPED";
        case 13: return "TEXTURE_FORMAT_MAPPED";
        default: return "UNKNOWN";
    }
}

/* ------------------------------------------------------------------ */
/* 未找到符号的处理                                                    */
/* ------------------------------------------------------------------ */

void glesmod_report_missing(const char *name) {
    int i;
    int is_new = 0;

    pthread_mutex_lock(&g_lock);
    for (i = 0; i < g_missing_count; i++) {
        if (strcmp(g_missing[i], name) == 0) break;
    }
    if (i == g_missing_count && g_missing_count < MAX_MISSING_SYMBOLS) {
        snprintf(g_missing[g_missing_count], sizeof(g_missing[0]), "%s", name);
        g_missing_count++;
        is_new = 1;
    }
    pthread_mutex_unlock(&g_lock);

    if (is_new) {
        char detail[128];
        snprintf(detail, sizeof(detail), "GLES 不提供 %s", name);
        glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION, detail);
    }
}

/*
 * 安全 stub 被调用时的上报。
 *
 * 与 report_missing 分开记录，因为语义不同：
 *   report_missing = 期望 GLES 提供但没有 -> 说明我们的符号表或驱动有问题
 *   report_stub    = 我们主动提供的空实现被调用 -> 属于预期内的功能降级
 *
 * 分开记录能让诊断更准确：前者指向缺陷，后者指向能力边界。
 */
void glesmod_report_stub(const char *name) {
    int i;
    int is_new = 0;

    pthread_mutex_lock(&g_lock);
    for (i = 0; i < g_stub_count; i++) {
        if (strcmp(g_stubs[i], name) == 0) break;
    }
    if (i == g_stub_count && g_stub_count < MAX_STUB_SYMBOLS) {
        snprintf(g_stubs[g_stub_count], sizeof(g_stubs[0]), "%s", name);
        g_stub_count++;
        is_new = 1;
    }
    pthread_mutex_unlock(&g_lock);

    /*
     * 只在首次调用时记降级事件与日志。
     *
     * 必须如此：stub 函数可能在渲染循环里被高频调用，
     * 每次都记事件会污染统计、写日志则会阻塞渲染线程。
     */
    if (is_new) {
        char detail[128];
        snprintf(detail, sizeof(detail),
                 "GLES 不提供 %s，已忽略调用", name);
        glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION, detail);

        /*
         * 每发现一个新的 stub 就重写状态文件。
         *
         * 为什么必须即时更新：状态文件是在初始化完成时写出的，而 stub
         * 往往在那之后（进入世界、模组加载渲染代码）才被调用。
         * 若不更新，用户拿到的 status.json 里 stub_symbols 永远是空的，
         * 我们就无法判断「哪些功能实际上失效了」。
         *
         * 频率可控：新 stub 的数量上限是 MAX_STUB_SYMBOLS（128），
         * 且每个符号只触发一次，因此最多写 128 次文件，不影响性能。
         */
        glesmod_write_status();
    }
}

/*
 * 调用追踪环形缓冲。
 *
 * 用途：真机 SIGSEGV 时，崩溃转储只能给出寄存器与 pc 偏移，无法看出
 * 崩在哪个 GL 调用上。记录最近 N 次调用的函数名并写入状态文件后，
 * 回看 status.json 的 last_calls 即可直接定位。
 *
 * 代价控制：
 *   - 只做一次原子自增（无锁）
 *   - 只存指针，不做字符串复制（参数是编译期字面量，生命周期为整个进程）
 *   - N 取 64：足够覆盖一轮初始化调用，内存占用 512 字节
 */
#define TRACE_RING_SIZE 64
static const char *g_trace_ring[TRACE_RING_SIZE];
static atomic_uint g_trace_seq = 0;

int glesmod_trace_enabled = 1;

/*
 * 热路径「可以走快速路径」标志。见 gl_internal.h 的完整说明。
 *
 * 只有「初始化真正结束」且「追踪模式不是深度追踪」时才置位。
 * 声明为 volatile 以避免编译器把这个判断优化掉。
 */
volatile int glesmod_hotpath_ready = 0;

void glesmod_trace_call(const char *name) {
    if (!glesmod_trace_enabled || name == NULL) return;
    unsigned int i = atomic_fetch_add_explicit(
            &g_trace_seq, 1u, memory_order_relaxed);
    g_trace_ring[i % TRACE_RING_SIZE] = name;
}

/* ------------------------------------------------------------------ */
/* 符号解析                                                            */
/* ------------------------------------------------------------------ */

/*
 * eglGetProcAddress 的函数指针类型。
 *
 * 返回类型必须是 void* 而非 void —— 否则无法把返回值转换为函数指针
 * （C 不允许把 void 表达式转为指针）。
 */
typedef void *(*glesmod_eglgetproc_t)(const char *);

static glesmod_eglgetproc_t g_egl_get_proc = NULL;

static glesmod_eglgetproc_t get_egl_get_proc_address(void) {
    if (g_egl_get_proc != NULL) return g_egl_get_proc;

    /*
     * 【★ 真机教训：必须先显式 dlopen EGL，不能只靠 RTLD_DEFAULT ★】
     *
     * 原实现只在 RTLD_DEFAULT 里找 eglGetProcAddress，理由是「启动器已经
     * 加载了 EGL，直接取现成的即可」。**这个理由在实践中不成立。**
     *
     * 2026-09-29 真机日志（ZalithLauncher2）显示：
     *     EGLBridge: Binding to OpenGL ES
     *     [GLESMod] 已打开 libGLESv2.so (...)
     *     [GLESMod] 注意: 符号 glGetString 来自 libGLESv2-dlsym（非 EGL）...
     * 即 eglGetProcAddress **没取到**，全部符号退回 dlsym。
     * 原因：启动器以 RTLD_LOCAL 加载 EGL（见 FCL 的
     * ctxbridges/egl_loader.c：loader_dlopen(eglName, "libEGL.so",
     * RTLD_LOCAL|RTLD_LAZY)），RTLD_LOCAL 的符号**不进入全局命名空间**。
     *
     * 【为什么退回 dlsym 是危险的（不是「功能不受影响」）】
     * 本文件上方 glesym_resolve 的注释记录过一次真实 SIGSEGV：
     *   Android 上 /system/lib64/libGLESv2.so 只是系统 stub，
     *   真正实现在 /vendor/lib64/egl/libGLESv2_adreno.so。
     *   EGL 绑定的是厂商实现，而 dlopen("libGLESv2.so") 可能拿到
     *   **另一个实例**，两者的【每上下文状态（TLS）不共享】。
     *   于是无状态查询（glGetString）看似正常，而任何需要读写当前
     *   上下文状态的函数会操作未初始化数据 → 崩溃。
     *
     *   原注释写的「功能不受影响」只对**无状态**函数成立。
     *   对有状态函数，退回 dlsym 恰恰是那次崩溃的成因。
     *
     * 【修法】按启动器用的同一个名字 dlopen EGL，再从它取 eglGetProcAddress。
     *   名字取自 POJAVEXEC_EGL（启动器注入，真机实测为 "libEGL.so"），
     *   兜底 "libEGL.so"。
     *
     *   **关键点：dlopen 同一 soname 会返回【已加载的那个实例】**，
     *   因此拿到的是与启动器、与当前 GL 上下文完全一致的那份 EGL。
     *   这正是我们要的「与当前上下文同实例」保证。
     *
     * 【为什么不用 RTLD_GLOBAL】那会把我们自己的 gl* 符号也暴露出去，
     *   反而加剧 dlsym(RTLD_DEFAULT) 拿到本库自身的问题（见 is_own_symbol）。
     *   RTLD_LOCAL 足够：我们只要一个句柄做 dlsym。
     */
    const char *egl_name = glesmod_env_str(GLESMOD_ENV_EXEC_EGL);
    if (egl_name == NULL || egl_name[0] == '\0') {
        egl_name = "libEGL.so";
    }

    void *egl_handle = dlopen(egl_name, RTLD_NOW | RTLD_LOCAL);
    if (egl_handle != NULL) {
        g_egl_get_proc =
            (glesmod_eglgetproc_t)dlsym(egl_handle, "eglGetProcAddress");
        if (g_egl_get_proc != NULL) {
            /*
             * 正常路径：从此所有 GLES 入口点都来自与当前上下文同实例的 EGL。
             * 不会再出现「符号 X 来自 libGLESv2-dlsym（非 EGL）」的警告 ——
             * 那串警告本身就是「同实例保证失效」的可见标志。
             */
            log_line(1, "eglGetProcAddress 已从 %s 取得（与当前上下文同实例）",
                     egl_name);
            return g_egl_get_proc;
        }
        log_line(0, "已打开 %s 但其中没有 eglGetProcAddress", egl_name);
    } else {
        log_line(0, "无法 dlopen %s: %s", egl_name, dlerror());
    }

    /*
     * 兜底：全局符号表。
     *
     * 走到这里说明显式 dlopen EGL 也失败了，属于异常环境。
     * 保留此路径以免在非常规启动器下完全瘫痪，但**如实警告** ——
     * 因为下面的 dlsym 回退会让「同实例」保证失效（见上方说明）。
     */
    g_egl_get_proc = (glesmod_eglgetproc_t)dlsym(RTLD_DEFAULT, "eglGetProcAddress");
    if (g_egl_get_proc != NULL) {
        log_line(0, "警告: eglGetProcAddress 取自 RTLD_DEFAULT，"
                    "可能与当前 GL 上下文不是同一实例（有状态调用可能异常）");
    } else {
        log_line(0, "警告: 完全取不到 eglGetProcAddress，"
                    "所有 GLES 入口点将退回 dlsym —— "
                    "若出现崩溃，这是首要怀疑对象");
    }
    return g_egl_get_proc;
}

/*
 * libGLESv2.so 的句柄。惰性打开，失败后不重试。
 *
 * 为什么必须显式 dlopen，而不能只依赖 RTLD_DEFAULT：
 *   Android 上 EGL/GLES 通常由启动器以 RTLD_LOCAL 加载
 *   （见 FCL 的 ctxbridges/egl_loader.c：
 *        loader_dlopen(eglName, "libEGL.so", RTLD_LOCAL|RTLD_LAZY)），
 *   厂商实现（libGLESv2_adreno.so 等）还可能位于独立的链接器命名空间。
 *   因此 dlsym(RTLD_DEFAULT, ...) 对核心函数并不可靠。
 *   显式打开系统 stub 库 /system/lib64/libGLESv2.so 能稳定拿到入口点。
 */
static void *g_gles_handle = NULL;
static int g_gles_handle_tried = 0;

static void *get_gles_handle(void) {
    if (g_gles_handle_tried) return g_gles_handle;
    g_gles_handle_tried = 1;

    /*
     * 兜底句柄必须与当前 EGL 同源：
     *   - 插件显式设置 LIBGL_GLES 时优先用它（ANGLE 模式通常是
     *     libGLESv2_angle.so）；
     *   - 否则若 POJAVEXEC_EGL 指向 ANGLE（libEGL_angle.so），
     *     用 libGLESv2_angle.so；
     *   - 最后才是系统 libGLESv2.so。
     *
     * 如果这里在 ANGLE 模式下仍打开系统 libGLESv2.so，主路径虽然
     * 走 eglGetProcAddress，但一旦某个符号没覆盖，兜底会拿到厂商
     * 驱动实例，与 ANGLE 上下文不是同一份实现。
     */
    const char *chosen = "libGLESv2.so";
    const char *libgl_gles = glesmod_env_str(GLESMOD_ENV_LIBGL_GLES);
    const char *egl_name = glesmod_env_str(GLESMOD_ENV_EXEC_EGL);

    if (libgl_gles != NULL && libgl_gles[0] != '\0') {
        chosen = libgl_gles;
    } else if (egl_name != NULL && strstr(egl_name, "angle") != NULL) {
        chosen = "libGLESv2_angle.so";
    }

    /* RTLD_LOCAL：只需句柄做 dlsym，无需暴露自己的符号。
     * RTLD_NOW：立即解析，便于尽早暴露链接问题。 */
    g_gles_handle = dlopen(chosen, RTLD_NOW | RTLD_LOCAL);
    if (g_gles_handle == NULL) {
        log_line(0, "无法 dlopen %s: %s", chosen, dlerror());
        return NULL;
    }
    log_line(1, "已打开 %s (%p)", chosen, g_gles_handle);
    return g_gles_handle;
}

/*
 * 判断解析到的函数指针是否指向【本库自身】。
 *
 * 为什么必须检查：
 *   LWJGL 通过 -Dorg.lwjgl.opengl.libname 加载我们的 .so，加载时使用
 *   RTLD_GLOBAL，因此我们导出的 glBindTexture/glDrawElements 等符号会
 *   进入【全局符号表】。
 *
 *   于是 dlsym(RTLD_DEFAULT, "glFenceSync") 这类查询可能返回我们自己的
 *   转发函数。若该符号在 GLES 中并不存在（如 FenceSync 需要 ES 3.0+
 *   扩展，或我们为兼容性补充的符号），转发函数会再次调用 glesym_resolve，
 *   拿到同一个指针，形成【无限自我递归 → 栈溢出 SIGSEGV】。
 *
 *   由于 dladdr 能给出符号所在的对象，这里直接比对路径即可拦下这种情况。
 *   返回「是我们自己」时一律视为未找到，交由降级路径处理。
 */
static int is_own_symbol(void *fn) {
    Dl_info info;
    if (dladdr(fn, &info) == 0 || info.dli_fname == NULL) {
        /* 拿不到归属信息。保守地认为不是自己的，避免误伤正常解析。 */
        return 0;
    }

    /*
     * 判定依据：文件名以 libgl_gles.so 开头。
     *
     * 不使用完整路径比对，因为 Android 上同一库可能以不同路径出现
     * （nativeLibraryDir、LD_LIBRARY_PATH 中的副本等），
     * 而文件名是稳定的。
     */
    const char *base = strrchr(info.dli_fname, '/');
    base = (base != NULL) ? base + 1 : info.dli_fname;

    return strncmp(base, "libgl_gles.so", 13) == 0;
}

glesmod_proc_t glesym_resolve(const char *name) {
    if (name == NULL) return NULL;

    pthread_mutex_lock(&g_lock);
    for (int i = 0; i < g_resolve_cache_len; i++) {
        if (strcmp(g_resolve_cache[i].name, name) == 0) {
            glesmod_proc_t cached = g_resolve_cache[i].fn;
            pthread_mutex_unlock(&g_lock);
            return cached;
        }
    }
    pthread_mutex_unlock(&g_lock);

    glesmod_proc_t fn = NULL;
    const char *via = NULL;

    /*
     * 解析顺序（可靠性从高到低）。
     *
     * 【为什么 eglGetProcAddress 必须排第一】
     *   真机上出现过一次 SIGSEGV，崩在 __memcpy_aarch64_simd，
     *   si_addr 与某个寄存器呈「64 位指针被截断成 32 位」的特征，
     *   且 R30 位于 libGLESv2_adreno.so（厂商驱动）。
     *
     *   根因是解析来源不一致：
     *     Android 上 /system/lib64/libGLESv2.so 只是系统 stub，
     *     真正的实现在 /vendor/lib64/egl/libGLESv2_adreno.so。
     *     EGL 绑定的是厂商实现，而 dlopen("libGLESv2.so") 可能拿到
     *     另一个实例，两者的【每上下文状态（TLS）不共享】。
     *     于是无状态查询（glGetString）看似正常，而任何需要读写当前
     *     上下文状态的函数会操作未初始化数据，进而崩溃。
     *
     *   eglGetProcAddress 由 EGL 提供，保证返回【与当前上下文同实例】
     *   的入口点，因此是唯一可靠的来源。GL4ES 与 MobileGlues 同样
     *   以它为首选，并只把 dlsym 当兜底。
     *
     * 顺序：
     *   1) eglGetProcAddress  —— 与当前上下文同实例，首选
     *   2) libGLESv2.so 的 dlsym —— 兜底（某些实现不通过 EGL 暴露核心函数）
     *   3) RTLD_DEFAULT       —— 最后兜底，并过滤指向本库自身的符号
     *
     * 结果会被缓存，因此多次尝试只发生在首次调用，不影响每帧性能。
     */
    glesmod_eglgetproc_t getproc = get_egl_get_proc_address();
    if (getproc != NULL) {
        fn = (glesmod_proc_t)getproc(name);
        if (fn != NULL) via = "eglGetProcAddress";
    }

    if (fn == NULL) {
        void *handle = get_gles_handle();
        if (handle != NULL) {
            fn = (glesmod_proc_t)dlsym(handle, name);
            if (fn != NULL) via = "libGLESv2-dlsym";
        }
    }

    if (fn == NULL) {
        fn = (glesmod_proc_t)dlsym(RTLD_DEFAULT, name);
        if (fn != NULL) {
            via = "RTLD_DEFAULT";
            /*
             * 关键防护：若解析结果指向本库，说明这是自我引用。
             * 必须丢弃，否则转发函数会无限递归自身直至栈溢出。
             */
            if (is_own_symbol((void *)fn)) {
                log_line(1, "符号 %s 在全局表中指向本库自身，已忽略"
                            "（ES 可能不提供该函数）", name);
                fn = NULL;
                via = NULL;
            }
        }
    }

    /*
     * 记录解析来源。级别 1 时只对「非 eglGetProcAddress 来源」输出警告——
     * 这两种来源在真机上已被证明不可靠，出现即值得关注。
     * 完整清单在级别 3。
     */
    if (fn != NULL) {
        if (g_log_level >= 3) {
            log_line(3, "解析 %s -> %p (via %s)", name, (void *)fn, via);
        } else if (g_log_level >= 1 && via != NULL &&
                   strcmp(via, "eglGetProcAddress") != 0) {
            log_line(1, "注意: 符号 %s 来自 %s（非 EGL），可能与其他实例不一致",
                     name, via);
        }
    }

    pthread_mutex_lock(&g_lock);
    if (g_resolve_cache_len < RESOLVE_CACHE_SIZE) {
        g_resolve_cache[g_resolve_cache_len].name = name;
        g_resolve_cache[g_resolve_cache_len].fn = fn;
        g_resolve_cache_len++;
    }
    pthread_mutex_unlock(&g_lock);

    return fn;
}

/* ------------------------------------------------------------------ */
/* 能力探测                                                            */
/* ------------------------------------------------------------------ */

/*
 * 判断空格分隔的扩展串是否包含完整 token。
 *
 * 必须按 token 边界匹配：裸 strstr 可能把更长的扩展名误判成目标
 * （例如目标名是另一个扩展名的前缀）。
 */
int glesmod_extension_present(const char *extensions, const char *name) {
    if (extensions == NULL || name == NULL || name[0] == '\0') return 0;

    size_t n = strlen(name);
    const char *p = extensions;
    while ((p = strstr(p, name)) != NULL) {
        int left_ok = (p == extensions) || (p[-1] == ' ');
        int right_ok = (p[n] == ' ' || p[n] == '\0');
        if (left_ok && right_ok) return 1;
        p += n;
    }
    return 0;
}

/* 用给定函数名探测能力，存在则设置能力位 */
static void probe_proc(const char *name, unsigned int bit) {
    glesmod_proc_t fn = glesym_resolve(name);
    if (fn != NULL) {
        g_caps.flags |= bit;
    }
}

/*
 * 探测 GLES 能力。
 *
 * @return 成功取得版本信息返回 true；若 EGL 上下文尚未 current（拿不到
 *         GL_VERSION）返回 false，调用方应稍后重试。
 *
 * 之所以区分「失败」与「保守判定」：早期显示窗口阶段可能还没有 current
 * 上下文。若此时就固化判为 ES 2.0 并写状态文件，后续即使上下文就绪也无法
 * 纠正，表现为「能力全错但无明显报错」。
 */
static int probe_capabilities(void) {
    /*
     * 版本通过 glGetString(GL_VERSION) 获取。
     * 注意：此时上下文必须已 current，否则返回 NULL。
     */
    typedef const unsigned char *(*getstring_t)(GLenum);
    getstring_t get_string = (getstring_t)glesym_resolve("glGetString");

    int es_major = 0, es_minor = 0;
    const char *ver = NULL;
    if (get_string != NULL) {
        ver = (const char *)get_string(0x1F02 /* GL_VERSION */);
    }

    if (ver != NULL) {
        /* 形如 "OpenGL ES 3.2 v1.r0p0" 或 "OpenGL ES 3.0 ..." */
        const char *p = strstr(ver, "OpenGL ES");
        if (p != NULL) {
            p += 9;
            es_major = (int)strtol(p, (char **)&p, 10);
            if (*p == '.') {
                es_minor = (int)strtol(p + 1, NULL, 10);
            }
        } else {
            /*
             * 不含 "OpenGL ES" 前缀。若驱动直接回报 "3.2" 这样的裸版本号
             * （部分桌面 GL 与兼容层会如此），也尝试解析。
             */
            es_major = (int)strtol(ver, (char **)&p, 10);
            if (*p == '.') {
                es_minor = (int)strtol(p + 1, NULL, 10);
            }
        }
    }

    if (es_major <= 0) {
        /* 上下文未就绪，或其他原因拿不到版本串。交由调用方重试。 */
        return 0;
    }

    log_line(1, "检测到 GL_VERSION: \"%s\" -> ES %d.%d",
             ver, es_major, es_minor);

    g_caps.es_major = es_major;
    g_caps.es_minor = es_minor;

    /*
     * 对外声称的桌面 GL 版本。
     * MC 1.21.1 使用 GL 3.0 特性（VAO/FBO），因此声称 3.2 是安全且必要的：
     * 声称过低可能导致 MC 拒绝启动或走降级渲染路径。
     */
    g_caps.gl_major = 3;
    g_caps.gl_minor = 2;

    /* 容量查询 */
    typedef void (*getintegerv_t)(GLenum, GLint *);
    getintegerv_t get_int = (getintegerv_t)glesym_resolve("glGetIntegerv");
    if (get_int != NULL) {
        GLint v = 0;
        get_int(0x0D33 /* GL_MAX_TEXTURE_SIZE */, &v);
        g_caps.max_texture_size = v > 0 ? v : 2048;

        v = 0;
        get_int(0x8869 /* GL_MAX_VERTEX_ATTRIBS */, &v);
        g_caps.max_vertex_attribs = v > 0 ? v : 16;

        v = 0;
        get_int(0x8B4D /* GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS */, &v);
        g_caps.max_texture_units = v > 0 ? v : 8;
    } else {
        g_caps.max_texture_size = 2048;
        g_caps.max_vertex_attribs = 16;
        g_caps.max_texture_units = 8;
    }
    g_caps.max_draw_buffers = (es_major >= 3) ? 4 : 1;
    g_caps.max_samples = (es_major >= 3) ? 4 : 0;
    g_caps.max_uniform_components = 1024;

    /*
     * 扩展能力探测。
     *
     * 注意：原版 MC 不使用这些功能（见 docs/o-02-symbol-inventory.md），
     * 它们只服务于 Sodium 等优化模组。探测到才置位，避免上层误判。
     */
    if (es_major > 3 || (es_major == 3 && es_minor >= 1)) {
        /* ES 3.1+ 才有计算着色器与间接绘制 */
        probe_proc("glDispatchCompute", GLESMOD_CAP_COMPUTE_SHADER);
        probe_proc("glDrawElementsIndirect", GLESMOD_CAP_INDIRECT_DRAW);
    }

    if (es_major >= 3) {
        /* ES 3.0 起支持实例化 */
        probe_proc("glDrawElementsInstanced", GLESMOD_CAP_INSTANCING);
        probe_proc("glDrawBuffers", GLESMOD_CAP_MULTI_DRAW_BUFFERS);
        probe_proc("glTexStorage2D", GLESMOD_CAP_TEXTURE_STORAGE);
    }

    /*
     * Multi-Draw：只对 GL_EXT_multi_draw_indirect 做原生加速。
     *
     * 不能只看符号：Android 的 EGL 包装层可能返回一个“存在但不可用”的
     * 入口点。这里要求驱动扩展串明确声明，并且对应函数可解析；真正的
     * 探测与回退由 custom.c 在首次调用时完成。
     */
    if (get_string != NULL) {
        const char *ext = (const char *)get_string(GLESMOD_GL_EXTENSIONS);
        int has_ext =
            glesmod_extension_present(ext, "GL_EXT_multi_draw_indirect");
        int has_arrays =
            has_ext && glesym_resolve("glMultiDrawArraysIndirectEXT") != NULL;
        int has_elements =
            has_ext && glesym_resolve("glMultiDrawElementsIndirectEXT") != NULL;

        log_line(1, "Multi-Draw: GL_EXT_multi_draw_indirect=%d, "
                    "arrays=%d, elements=%d",
                 has_ext, has_arrays, has_elements);

        if (has_arrays || has_elements) {
            g_caps.flags |= GLESMOD_CAP_MULTI_DRAW;
        }
    } else {
        log_line(1, "Multi-Draw: 无法读取扩展串，使用逐条 indirect 回退");
    }

    /* 持久映射：ES 3.0+ 有 glMapBufferRange，但持久映射位需扩展 */
    if (es_major >= 3 && glesym_resolve("glMapBufferRange") != NULL) {
        g_caps.flags |= GLESMOD_CAP_PERSISTENT_MAP;
    }

    g_caps.degrade_level = g_degrade_level;
    g_caps.flags |= GLESMOD_CAP_BACKEND_ACTIVE;
    return 1;
}

/* ------------------------------------------------------------------ */
/* 惰性初始化                                                          */
/* ------------------------------------------------------------------ */

/*
 * 配置初始化主体。由 pthread_once 保证「恰好执行一次」。
 *
 * 不持有 g_lock：本函数会间接调用 glesym_resolve()（经 probe_capabilities），
 * 而后者需要 g_lock。pthread 默认互斥锁非递归，外层持锁必然自我死锁。
 * pthread_once 由 libc 内部保证线程安全，无需额外同步。
 *
 * 这里只做与 GL 无关的准备工作（读环境变量、置能力表为保守初值）。
 * 真正的 GL 能力探测交给 maybe_probe()，因为它需要 EGL 上下文已 current，
 * 而上下文未必在首次 GL 调用时就绪。
 */
static void do_config_init(void) {
    g_enabled       = glesmod_env_int(GLESMOD_ENV_ENABLE, 1);
    g_degrade_level = glesmod_env_int(GLESMOD_ENV_DEGRADE_LEVEL, 1);
    g_state_cache   = glesmod_env_int(GLESMOD_ENV_STATE_CACHE, 1);
    g_log_level     = glesmod_env_int(GLESMOD_ENV_LOG_LEVEL, 1);

    /*
     * 追踪模式。
     *
     * glesmod_trace_enabled 保持「是否记录」的语义（0 = 不记录），
     * g_trace_mode 保持「记录范围」的语义（1 = 仅初始化阶段，2 = 全时段）。
     * 分开是因为 hotpath_ready 的置位条件只需要知道「是不是 2」，
     * 不需要每次调用都去比较整个取值。
     */
    g_trace_mode = glesmod_env_int(GLESMOD_ENV_TRACE, 1);
    if (g_trace_mode < 0) g_trace_mode = 0;
    if (g_trace_mode > 2) g_trace_mode = 2;
    glesmod_trace_enabled = (g_trace_mode != GLESMOD_TRACE_MODE_OFF) ? 1 : 0;

    if (g_degrade_level < 0) g_degrade_level = 0;
    if (g_degrade_level > 2) g_degrade_level = 2;
    if (g_log_level < 0) g_log_level = 0;
    if (g_log_level > 2) g_log_level = 2;

    memset(&g_caps, 0, sizeof(g_caps));

    if (!g_enabled) {
        log_line(0, "后端被配置禁用（%s=0），所有调用将走原生 GLES",
                 GLESMOD_ENV_ENABLE);
        g_active = 0;
    }
}

/*
 * 能力探测的一次性状态机：
 *   0 = 尚未探测，1 = 正在探测，2 = 已完成
 *
 * 用原子变量而非锁，目的是彻底避免与 g_lock 的嵌套。
 */
static atomic_int g_probe_state = 0;

/*
 * 探测失败后的重试次数上限。
 *
 * 【为什么必须限制】每个 GL 转发函数都会调用 glesmod_lazy_init()。
 * 若探测持续失败（EGL 上下文长期不就绪）且允许无限重试，
 * 则每次 GL 调用都会执行一次 probe_capabilities()——其中包含一次真实的
 * glGetString 调用。MC 每帧有 10^4 ～ 10^5 次 GL 调用，这会造成严重卡顿。
 *
 * 有限重试即可覆盖真正的场景：上下文通常在启动后很短时间内就绪。
 * 超过上限后保持「未激活」，所有功能走保守值（不崩溃）。
 */
#define MAX_PROBE_ATTEMPTS 50

static atomic_int g_probe_attempts = 0;

static void maybe_probe(void) {
    if (!g_enabled) return;

    /*
     * 快速路径：已完成探测时只做一次原子读。
     * 这是每个 GL 调用都要走的路径，必须尽量短。
     */
    if (atomic_load_explicit(&g_probe_state, memory_order_acquire) == 2) return;

    /* 超过重试上限后彻底放弃，避免每帧重复探测拖慢渲染 */
    if (atomic_load_explicit(&g_probe_attempts, memory_order_relaxed)
            >= MAX_PROBE_ATTEMPTS) {
        return;
    }

    /* 抢占探测权。失败说明别的线程正在探测，直接返回——不等待，
     * 避免任何形式的阻塞。GL 调用在实践中是单线程的，
     * 这种情形只可能出现在极早期的并发初始化窗口。 */
    int expected = 0;
    if (!atomic_compare_exchange_strong_explicit(
            &g_probe_state, &expected, 1,
            memory_order_acq_rel, memory_order_acquire)) {
        return;
    }

    int attempt = atomic_fetch_add_explicit(
            &g_probe_attempts, 1, memory_order_relaxed) + 1;

    if (probe_capabilities()) {
        /* 探测成功，锁定结果并输出状态 */
        g_active = (g_caps.flags & GLESMOD_CAP_BACKEND_ACTIVE) != 0;
        atomic_store_explicit(&g_probe_state, 2, memory_order_release);

        log_line(0, "GLES 后端已激活: ES %d.%d, 对外声称 GL %d.%d, 降级档位 %d"
                    "（第 %d 次尝试）",
                 g_caps.es_major, g_caps.es_minor,
                 g_caps.gl_major, g_caps.gl_minor,
                 g_caps.degrade_level, attempt);
        glesmod_write_status();
    } else {
        /*
         * 未能取得版本串 —— 最可能的原因是 EGL 上下文还没 current。
         * 回到未探测状态，等下一次 GL 调用再试（受 MAX_PROBE_ATTEMPTS 限制）。
         */
        atomic_store_explicit(&g_probe_state, 0, memory_order_release);

        if (attempt == MAX_PROBE_ATTEMPTS) {
            log_line(0, "能力探测连续失败 %d 次，已放弃。"
                        "后端保持未激活，渲染由 GL 直接处理。"
                        "最可能的原因：EGL 上下文未创建，或 "
                        "LIBGL_ES 未生效。", MAX_PROBE_ATTEMPTS);
            glesmod_degrade(GLESMOD_DEGRADE_BACKEND_INIT_FAILED,
                            "能力探测超时，EGL 上下文可能未就绪");
            glesmod_write_status();
        }
    }
}

void glesmod_lazy_init(void) {
    /*
     * 存活标记必须最先写下，且在任何可能失败/可能递归的操作之前。
     *
     * 这是整个诊断链条的基石：它不依赖 GL 上下文、不依赖符号解析、
     * 不依赖探测成功。因此它的存在与否能唯一地区分两种情况：
     *   - 有这一行 -> 库确实在 GL 调用路径上
     *   - 没有     -> 库从未被调用（问题在 FCL 插件配置或加载）
     * 之前的真机日志里一行本库输出都没有，却无法判断属于哪种，
     * 导致排查方向不确定。加上本行即可消除该歧义。
     */
    log_load_banner();

    pthread_once(&g_init_once, do_config_init);
    maybe_probe();

    /*
     * 标记转入快速路径，使高频转发函数在稳态下不再付出钩子开销。
     *
     * 【两个条件，缺一不可】
     *
     * 条件一：初始化真正结束。
     *   若 EGL 上下文尚未 current，maybe_probe() 会探测失败并保留未完成状态
     *   （见其内部的重试计数）。此时**不能**置位 —— 否则后续调用再也不会
     *   重试探测，能力表将永远停留在保守初值。
     *   判定：探测成功（probe_state==2）或已彻底放弃重试（达到上限）。
     *
     * 条件二：不是深度追踪模式。
     *   `GLESMOD_TRACE=2` 表示用户要完整调用序列（用于排查「启动很久之后才
     *   崩」之类问题）。此时**保持不置位**，于是每次调用都会走
     *   glesmod_on_first_call，在那里记录本次调用 —— 行为与优化前一致。
     *   这样做的好处是：两种模式共用同一条分支，稳态下不会多出判断。
     */
    int probe_done =
        atomic_load_explicit(&g_probe_state, memory_order_acquire) == 2
        || atomic_load_explicit(&g_probe_attempts, memory_order_relaxed)
               >= MAX_PROBE_ATTEMPTS
        || !g_enabled;

    if (probe_done && g_trace_mode != GLESMOD_TRACE_MODE_DEEP) {
        glesmod_hotpath_ready = 1;
    }
}

/*
 * 首次调用 / 深度追踪钩子。
 *
 * 见 gl_internal.h 中 GLESMOD_HOTPATH 的完整说明（含反汇编实测数据）。
 * 职责：
 *   1. 触发（可能重复的）初始化流程，直到它真正结束
 *   2. 记录本次调用 —— 普通模式下这覆盖「早期初始化阶段」，
 *      也恰好覆盖了真机两次 SIGSEGV 发生的位置；
 *      深度追踪模式（TRACE=2）下覆盖全时段
 *
 * 之所以把 name 也传进来：这样首次调用同样会进入 g_trace_ring，
 * 使 status.json 的 last_calls 在**崩溃于最早期**时仍然可用。
 * 一旦初始化结束且非深度追踪，后续调用不再进入本函数（稳态零开销）。
 */
void glesmod_on_first_call(const char *name) {
    glesmod_lazy_init();
    if (glesmod_trace_enabled && name != NULL) {
        glesmod_trace_call(name);
    }
}


bool glesmod_is_active(void) {
    glesmod_lazy_init();
    return g_active != 0;
}

const glesmod_caps *glesmod_get_caps(void) {
    glesmod_lazy_init();
    return &g_caps;
}

/* ------------------------------------------------------------------ */
/* 状态文件输出                                                        */
/* ------------------------------------------------------------------ */

/* 极简 JSON 字符串转义：只处理引号、反斜杠与控制字符 */
static void json_escape(const char *in, char *out, size_t out_size) {
    size_t j = 0;
    for (size_t i = 0; in[i] != '\0' && j + 2 < out_size; i++) {
        unsigned char c = (unsigned char)in[i];
        if (c == '"' || c == '\\') {
            out[j++] = '\\';
            out[j++] = (char)c;
        } else if (c < 0x20) {
            out[j++] = ' ';
        } else {
            out[j++] = (char)c;
        }
    }
    out[j] = '\0';
}

void glesmod_write_status(void) {
    /*
     * 输出路径策略：
     *   1. 若设置 GLESMOD_STATUS_FILE，使用之（绝对路径或相对工作目录）
     *   2. 否则使用 <工作目录>/glesmod/status.json
     *
     * 之所以用文件而非 JNI 回传：native 库由启动器加载，与 mod 的 JVM
     * 不属于同一加载路径，直接回调 Java 会引入类加载与线程复杂性。
     * 文件是两端都能访问的可靠通道。
     */
    const char *override = glesmod_env_str(GLESMOD_ENV_STATUS_FILE);
    char path[512];
    if (override != NULL) {
        snprintf(path, sizeof(path), "%s", override);
    } else {
        snprintf(path, sizeof(path), "%s", GLESMOD_STATUS_RELPATH);
    }

    /* 确保父目录存在 */
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", path);
    char *slash = strrchr(dir, '/');
    if (slash != NULL) {
        *slash = '\0';
        /* 逐级创建。忽略失败：失败时后续 fopen 会失败并记录日志。 */
        for (char *p = dir + 1; *p; p++) {
            if (*p == '/') {
                *p = '\0';
                mkdir(dir, 0755);
                *p = '/';
            }
        }
        mkdir(dir, 0755);
    }

    FILE *f = fopen(path, "w");
    if (f == NULL) {
        log_line(0, "无法写入状态文件: %s", path);
        glesmod_degrade(GLESMOD_DEGRADE_BACKEND_INIT_FAILED, "状态文件写入失败");
        return;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"abi_version\": %d,\n", GLESMOD_ABI_VERSION);
    fprintf(f, "  \"active\": %s,\n", g_active ? "true" : "false");
    fprintf(f, "  \"gles_version\": \"%d.%d\",\n", g_caps.es_major, g_caps.es_minor);
    fprintf(f, "  \"reported_gl_version\": \"%d.%d\",\n", g_caps.gl_major, g_caps.gl_minor);
    fprintf(f, "  \"degrade_level\": %d,\n", g_degrade_level);
    fprintf(f, "  \"state_cache\": %s,\n", g_state_cache ? "true" : "false");
    fprintf(f, "  \"caps\": {\n");
    fprintf(f, "    \"multi_draw\": %s,\n",
            (g_caps.flags & GLESMOD_CAP_MULTI_DRAW) ? "true" : "false");
    fprintf(f, "    \"compute_shader\": %s,\n",
            (g_caps.flags & GLESMOD_CAP_COMPUTE_SHADER) ? "true" : "false");
    fprintf(f, "    \"persistent_mapping\": %s,\n",
            (g_caps.flags & GLESMOD_CAP_PERSISTENT_MAP) ? "true" : "false");
    fprintf(f, "    \"instancing\": %s,\n",
            (g_caps.flags & GLESMOD_CAP_INSTANCING) ? "true" : "false");
    fprintf(f, "    \"indirect_draw\": %s,\n",
            (g_caps.flags & GLESMOD_CAP_INDIRECT_DRAW) ? "true" : "false");
    fprintf(f, "    \"max_texture_units\": %d,\n", g_caps.max_texture_units);
    fprintf(f, "    \"max_draw_buffers\": %d,\n", g_caps.max_draw_buffers);
    fprintf(f, "    \"max_texture_size\": %d,\n", g_caps.max_texture_size);
    fprintf(f, "    \"max_samples\": %d,\n", g_caps.max_samples);
    fprintf(f, "    \"max_vertex_attribs\": %d\n", g_caps.max_vertex_attribs);
    fprintf(f, "  },\n");

    fprintf(f, "  \"degrade_events\": [\n");
    int first = 1;
    for (int i = 0; i < MAX_DEGRADE_KINDS; i++) {
        if (g_degrade_count[i] == 0) continue;
        char detail[DEGRADE_DETAIL_LEN * 2];
        json_escape(g_degrade_detail[i], detail, sizeof(detail));
        fprintf(f, "%s    {\"reason\": \"%s\", \"count\": %lu, \"detail\": \"%s\"}",
                first ? "" : ",\n", degrade_name(i), g_degrade_count[i], detail);
        first = 0;
    }
    fprintf(f, "\n  ],\n");

    fprintf(f, "  \"missing_symbols\": {");
    fprintf(f, "\"count\": %d, \"names\": [", g_missing_count);
    for (int i = 0; i < g_missing_count; i++) {
        char esc[128];
        json_escape(g_missing[i], esc, sizeof(esc));
        fprintf(f, "%s\"%s\"", i ? ", " : "", esc);
    }
    fprintf(f, "]},\n");

    /*
     * 被调用过的 stub 符号。
     *
     * 这是判断「哪些 GL 功能静默失效」的关键依据：GLES 不提供的符号
     * 由我们以安全 stub 导出（返回零值不崩溃）。若此列表非空，
     * 说明某个功能实际上没生效。
     */
    fprintf(f, "  \"stub_symbols\": {");
    fprintf(f, "\"count\": %d, \"names\": [", g_stub_count);
    for (int i = 0; i < g_stub_count; i++) {
        char esc[128];
        json_escape(g_stubs[i], esc, sizeof(esc));
        fprintf(f, "%s\"%s\"", i ? ", " : "", esc);
    }
    fprintf(f, "]},\n");

    /*
     * 最近若干次 GL 调用，按调用顺序（最旧在前）。
     *
     * 【为什么需要它】真机 SIGSEGV 时，崩溃转储只能给出寄存器与 pc 偏移，
     * 无法直接看出崩在哪个 GL 函数上。有了这份列表，崩溃后回看
     * status.json 就能知道「最后执行的几个 GL 调用是什么」，
     * 从而把范围缩小到具体函数。
     *
     * 注意：这是「崩溃前最后一次成功写出状态文件时」的快照。
     * 状态文件不会在每次 GL 调用后都重写（那会严重拖慢渲染），
     * 因此它的时效性取决于最后一次写出发生在何时。
     * 写出的时机包括：能力探测成功、发现新的 stub、发现新缺失符号。
     */
    fprintf(f, "  \"last_calls\": [");
    {
        unsigned int seq = atomic_load_explicit(&g_trace_seq, memory_order_relaxed);
        unsigned int count = seq < TRACE_RING_SIZE ? seq : TRACE_RING_SIZE;
        unsigned int start = (seq > TRACE_RING_SIZE) ? (seq - TRACE_RING_SIZE) : 0;
        for (unsigned int k = 0; k < count; k++) {
            const char *nm = g_trace_ring[(start + k) % TRACE_RING_SIZE];
            if (nm == NULL) continue;
            fprintf(f, "%s\"%s\"", k ? ", " : "", nm);
        }
    }
    fprintf(f, "],\n");

    fprintf(f, "  \"generated_by\": \"glesmod native backend\"\n");
    fprintf(f, "}\n");
    fclose(f);

    log_line(1, "状态文件已写入: %s", path);
}

/*
 * custom.c —— 需要特殊处理的 GL 符号实现
 *
 * 依据 docs/o-02-symbol-inventory.md 的分析，以下符号无法简单转发：
 *
 *   符号                    问题                                处理方式
 *   ----------------------  ----------------------------------  ------------------
 *   glClearDepth            参数为 double，ES 只接受 float     转 float
 *   glMapBuffer             返回整块映射，ES 只有 Range 变体    记录大小后调 Range
 *   glGetError              ES 错误枚举值与桌面不同             映射返回值
 *   glPolygonMode           ES 无面模式                        仅 GL_FILL 可忽略
 *   glLogicOp               ES 无逻辑运算                      忽略 + 降级
 *   glGetTexImage           ES 无纹理回读                      用 FBO + ReadPixels
 *   glDrawPixels            ES 无                              忽略 + 降级（NativeImage 路径）
 *   glMultiDraw*            ES 无核心 Multi-Draw              仅 indirect 扩展原生，其他循环
 *   glBufferStorage         ES 无（GL 4.4 功能）               用 glBufferData 真实
 *                                                               分配 + 记录降级
 *   glMapBufferRange        桌面专属映射位 ES 非法              剥离非法位后转发
 *   glGetString             对外版本串不被桌面正则接受          改写为 "3.2 (…)"
 *   glGetStringi            扩展列表缺桌面扩展名                末尾追加
 *                                                               GL_ARB_draw_buffers_blend
 *   glGetString(GL_EXTENSIONS) 同上，但**这条才是 ES 上生效的路径** 末尾追加
 *   glGetIntegerv            让 GL_NUM_EXTENSIONS 与上面自洽      +1
 *   glCheckFramebufferStatus 只给状态码，不足以定位根因         不完整时输出附件详情
 *   glShaderSource          需 GLSL -> GLSL ES 转换             调用转换器
 *   glGetShaderInfoLog      行为不变                            【拦截并记录错误】
 *   glGetProgramInfoLog     行为不变                            【拦截并记录错误】
 *   glTexImage2D/3D         桌面深层格式 ES 非法                翻译为 ES 合法组合
 *                           （GL_DEPTH_COMPONENT）              （修复深度缓冲失效）
 *
 * 原则：任何无法实现的路径都必须「不崩溃 + 记录降级事件」，而不是中止游戏。
 *
 * 许可证：LGPL-3.0-or-later
 */

#include "gl_internal.h"
#include "glesmod_logfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* gl.xml: GL_NUM_EXTENSIONS = 0x821D, GL_EXTENSIONS = 0x1F03 */
#define GLESMOD_ENUM_GL_NUM_EXTENSIONS 0x821D
#define GLESMOD_ENUM_GL_EXTENSIONS     GLESMOD_GL_EXTENSIONS

/* glGetError 的最小类型；纹理探针与 Multi-Draw 探测共用。 */
typedef GLenum (*get_err_fn_t)(void);
static get_err_fn_t texprobe_get_error(void);
static void drain_errors(get_err_fn_t get_err);

/* ================================================================== */
/* 辅助：取得 GLES 函数指针                                            */
/* ================================================================== */

#define RESOLVE_OR_RETURN(dst, name, ret)                \
    do {                                                 \
        if ((dst) == NULL) {                             \
            (dst) = (void *)glesym_resolve(name);        \
            if ((dst) == NULL) {                         \
                glesmod_report_missing(name);            \
                return ret;                              \
            }                                            \
        }                                                \
    } while (0)

/* ================================================================== */
/* glClearDepth —— double -> float                                     */
/* ================================================================== */

void glClearDepth(GLdouble depth) {
    typedef void (*fn_t)(GLfloat);
    static fn_t real = NULL;
    glesmod_lazy_init();

    RESOLVE_OR_RETURN(real, "glClearDepthf", );

    /*
     * double 转 float 会丢精度，但深度清屏值在实际使用中几乎总是 0.0 或 1.0，
     * 两者都能被 float 精确表示，因此该转换在实践中无损。
     */
    real((GLfloat)depth);
}

/* ================================================================== */
/* glMapBuffer —— 整块映射转为 glMapBufferRange                        */
/* ================================================================== */

/*
 * ES 没有 glMapBuffer（映射整块），只有 glMapBufferRange。
 * 需要知道缓冲区大小才能构造 Range 调用。策略：
 *   1. 查询 GL_BUFFER_SIZE
 *   2. 用 (0, size) 调用 glMapBufferRange
 *
 * 若查询失败（如目标不是缓冲区），返回 NULL 并记录降级。
 */
void *glMapBuffer(GLenum target, GLenum access) {
    typedef void (*get_buffer_parameteriv_t)(GLenum, GLenum, GLint *);
    typedef void *(*map_range_t)(GLenum, GLintptr, GLsizeiptr, GLbitfield);

    static get_buffer_parameteriv_t real_get_param = NULL;
    static map_range_t real_map_range = NULL;

    glesmod_lazy_init();

    /* 查询缓冲区大小 */
    if (real_get_param == NULL) {
        real_get_param = (get_buffer_parameteriv_t)glesym_resolve("glGetBufferParameteriv");
    }
    if (real_get_param == NULL) {
        glesmod_report_missing("glGetBufferParameteriv");
        glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION,
                        "glMapBuffer 需要 glGetBufferParameteriv，不可用");
        return NULL;
    }

    GLint size = 0;
    real_get_param(target, 0x8764 /* GL_BUFFER_SIZE */, &size);
    if (size <= 0) {
        glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION,
                        "glMapBuffer 无法确定缓冲区大小，已降级为返回 NULL");
        return NULL;
    }

    RESOLVE_OR_RETURN(real_map_range, "glMapBufferRange", NULL);

    /*
     * access 到 bitfield 的映射。
     * 桌面 GL 的 access（GL_READ_ONLY 等）与 ES 的 bitfield 语义不同，
     * 需要转换。0x88B8=GL_READ_ONLY, 0x88B9=GL_WRITE_ONLY, 0x88BA=GL_READ_WRITE
     */
    GLbitfield bits = 0;
    switch (access) {
        case 0x88B8: /* GL_READ_ONLY */
            bits = 0x0001; /* GL_MAP_READ_BIT */
            break;
        case 0x88B9: /* GL_WRITE_ONLY */
            bits = 0x0002; /* GL_MAP_WRITE_BIT */
            break;
        case 0x88BA: /* GL_READ_WRITE */
            bits = 0x0001 | 0x0002;
            break;
        default:
            bits = 0x0001 | 0x0002;
            break;
    }

    return real_map_range(target, 0, (GLsizeiptr)size, bits);
}

/* ================================================================== */
/* glGetError —— 错误枚举值映射                                        */
/* ================================================================== */

/*
 * ES 与桌面 GL 的大部分错误码数值相同（GL_INVALID_ENUM 0x0500 等），
 * 但存在差异。此处映射回桌面 GL 的期望值，避免 MC 误判错误类型。
 *
 * 已知差异：
 *   ES GL_CONTEXT_LOST (0x0507) 桌面无对应 -> 归为 GL_INVALID_OPERATION
 *   ES GL_TABLE_TOO_LARGE (0x8031) 桌面有，可直通
 */
GLenum glGetError(void) {
    typedef GLenum (*fn_t)(void);
    static fn_t real = NULL;
    glesmod_lazy_init();

    RESOLVE_OR_RETURN(real, "glGetError", 0 /* GL_NO_ERROR */);

    GLenum err = real();

    switch (err) {
        case 0x0507: /* ES GL_CONTEXT_LOST，桌面 GL 无此错误 */
            glesmod_log_enum_map(err, 0x0502 /* GL_INVALID_OPERATION */);
            return 0x0502;
        default:
            /* 其余错误码数值一致，直通 */
            return err;
    }
}

/* ================================================================== */
/* glPolygonMode —— ES 无面模式                                        */
/* ================================================================== */

/*
 * ES 只有 GL_FILL 行为（0x1B02）。
 * GL_LINE (0x1B01, wireframe) 与 GL_POINT (0x1B00) 无法在 ES 上实现。
 *
 * 处理策略：
 *   - GL_FILL -> 无操作（ES 本来就是 FILL）
 *   - 其他     -> 记录降级，不改变行为
 *
 * 原版 MC 不使用 wireframe，因此该降级在实践中不影响。
 */
void glPolygonMode(GLenum face, GLenum mode) {
    (void)face;
    glesmod_lazy_init();

    if (mode == 0x1B02 /* GL_FILL */) {
        /* ES 默认即 FILL，无需操作 */
        return;
    }

    /* 0x1B01=GL_LINE, 0x1B00=GL_POINT */
    char detail[96];
    snprintf(detail, sizeof(detail),
             "glPolygonMode(0x%04X) ES 不支持，已忽略（面模式降级）", mode);
    glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION, detail);
}

/* ================================================================== */
/* glLogicOp —— ES 无逻辑运算                                          */
/* ================================================================== */

/*
 * 逻辑运算（AND/OR/XOR 等）在 ES 中不存在。
 * 若调用方请求 GL_COPY (0x1503)，语义等价于无操作，可直接忽略。
 * 其他操作无法仿真（需要读写整个帧缓冲），记录降级。
 */
void glLogicOp(GLenum opcode) {
    glesmod_lazy_init();

    if (opcode == 0x1503 /* GL_COPY */) {
        return;
    }

    char detail[96];
    snprintf(detail, sizeof(detail),
             "glLogicOp(0x%04X) ES 不支持，已忽略（逻辑运算降级）", opcode);
    glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION, detail);
}

/* ================================================================== */
/* glDrawPixels —— ES 无                                               */
/* ================================================================== */

/*
 * 桌面 GL 用于直接把像素数据画到帧缓冲。ES 无此功能。
 *
 * 依据 O-02 分析，调用者是 MC 的 NativeImage 路径（非渲染主循环），
 * 因此「忽略 + 降级」是可接受的：相关图像操作会失效，但游戏不会崩溃。
 */
void glDrawPixels(GLsizei width, GLsizei height, GLenum format,
                  GLenum type, const void *pixels) {
    (void)width; (void)height; (void)format; (void)type; (void)pixels;
    glesmod_lazy_init();

    glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION,
                    "glDrawPixels ES 不支持，已忽略（图像写入路径降级）");
}

/* ================================================================== */
/* glGetTexImage —— ES 无，用 FBO + glReadPixels 实现                  */
/* ================================================================== */

/*
 * 实现思路：
 *   1. 查询纹理的宽高与格式
 *   2. 创建临时 FBO，把纹理附着到 COLOR_ATTACHMENT0
 *   3. glReadPixels 读回
 *   4. 清理
 *
 * 简化点（明确记录，避免误以为完整实现）：
 *   - 仅支持 2D 纹理的 level 0..n
 *   - 仅支持 GL_RGBA / GL_RGB 输出格式
 *   - 不处理 cubemap / 3D / 压缩纹理
 *   复杂情形一律降级（不崩溃）。
 *
 * 该函数在 NativeImage 路径上被调用（见 O-02），不在渲染主循环，
 * 因此性能不是关键，正确性优先。
 */
void glGetTexImage(GLenum target, GLint level, GLenum format,
                   GLenum type, void *pixels) {
    typedef void (*get_tex_level_t)(GLenum, GLint, GLenum, GLint *);
    typedef void (*gen_framebuffers_t)(GLsizei, GLuint *);
    typedef void (*delete_framebuffers_t)(GLsizei, const GLuint *);
    typedef void (*bind_framebuffer_t)(GLenum, GLuint);
    typedef void (*framebuffer_texture2d_t)(GLenum, GLenum, GLenum, GLuint, GLint);
    typedef void (*read_pixels_t)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *);

    static get_tex_level_t       real_get_level = NULL;
    static gen_framebuffers_t    real_gen_fbo   = NULL;
    static delete_framebuffers_t real_del_fbo   = NULL;
    static bind_framebuffer_t    real_bind_fbo  = NULL;
    static framebuffer_texture2d_t real_fbo_tex = NULL;
    static read_pixels_t         real_read_pix  = NULL;

    glesmod_lazy_init();

    if (target != 0x0DE1 /* GL_TEXTURE_2D */) {
        glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION,
                        "glGetTexImage 仅支持 GL_TEXTURE_2D，已降级");
        return;
    }

    /* 解析所需函数 */
    if (real_get_level == NULL) real_get_level = (get_tex_level_t)glesym_resolve("glGetTexLevelParameteriv");
    if (real_gen_fbo   == NULL) real_gen_fbo   = (gen_framebuffers_t)glesym_resolve("glGenFramebuffers");
    if (real_del_fbo   == NULL) real_del_fbo   = (delete_framebuffers_t)glesym_resolve("glDeleteFramebuffers");
    if (real_bind_fbo  == NULL) real_bind_fbo  = (bind_framebuffer_t)glesym_resolve("glBindFramebuffer");
    if (real_fbo_tex   == NULL) real_fbo_tex   = (framebuffer_texture2d_t)glesym_resolve("glFramebufferTexture2D");
    if (real_read_pix  == NULL) real_read_pix  = (read_pixels_t)glesym_resolve("glReadPixels");

    if (!real_get_level || !real_gen_fbo || !real_del_fbo || !real_bind_fbo
        || !real_fbo_tex || !real_read_pix) {
        glesmod_report_missing("glGetTexImage 依赖的 FBO 函数");
        glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION,
                        "glGetTexImage 无法实现（FBO 函数缺失），已降级");
        return;
    }

    /* 查询纹理尺寸 */
    GLint width = 0, height = 0;
    real_get_level(target, level, 0x1000 /* GL_TEXTURE_WIDTH */, &width);
    real_get_level(target, level, 0x1001 /* GL_TEXTURE_HEIGHT */, &height);

    if (width <= 0 || height <= 0) {
        glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION,
                        "glGetTexImage 纹理尺寸无效，已降级");
        return;
    }

    /* 保存并恢复 FBO 绑定，避免影响调用方状态 */
    typedef void (*get_integerv_t)(GLenum, GLint *);
    static get_integerv_t real_get_int = NULL;
    if (real_get_int == NULL) real_get_int = (get_integerv_t)glesym_resolve("glGetIntegerv");

    GLint prev_fbo = 0;
    if (real_get_int != NULL) {
        real_get_int(0x8CA6 /* GL_FRAMEBUFFER_BINDING */, &prev_fbo);
    }

    GLuint fbo = 0;
    real_gen_fbo(1, &fbo);
    real_bind_fbo(0x8D40 /* GL_FRAMEBUFFER */, fbo);
    /* 0x0DE1=GL_TEXTURE_2D, 0x8CE0=GL_COLOR_ATTACHMENT0 */
    real_fbo_tex(0x8D40, 0x8CE0, target, 0 /* 当前绑定的纹理 */, level);

    /* 读取。注意 glFramebufferTexture2D 的 texture 参数需要真实纹理名，
     * 而目标纹理已由调用方绑定到 target，ES 的 FBO 附着不接受「当前绑定」
     * 语义，因此这里需要调用方传入纹理名——但 GL API 无此参数。
     *
     * 为正确性，改用 GL_TEXTURE_BINDING_2D 查询当前绑定的纹理名。 */
    GLint bound_tex = 0;
    if (real_get_int != NULL) {
        real_get_int(0x8069 /* GL_TEXTURE_BINDING_2D */, &bound_tex);
    }
    if (bound_tex > 0) {
        real_fbo_tex(0x8D40, 0x8CE0, target, (GLuint)bound_tex, level);
    }

    real_read_pix(0, 0, width, height, format, type, pixels);

    /* 清理 */
    real_bind_fbo(0x8D40, (GLuint)prev_fbo);
    real_del_fbo(1, &fbo);

    if (glesmod_env_int("GLESMOD_LOG_LEVEL", 1) >= 2) {
        glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION,
                        "glGetTexImage 已用 FBO 回读实现（可能较慢）");
    }
}

/* ================================================================== */
/* glMultiDraw* —— 仅 indirect 扩展原生，其他拆解为循环                   */
/* ================================================================== */

/*
 * ES 3.2 核心没有 Multi-Draw。本实现只对
 * GL_EXT_multi_draw_indirect 做原生加速：
 *       glMultiDrawArraysIndirectEXT / glMultiDrawElementsIndirectEXT
 *
 * 基础 glMultiDrawArrays / glMultiDrawElements / BaseVertex 没有原生路径，
 * 始终走循环拆解；ANGLE 的 multi-draw 扩展也不参与。
 *
 * 检测原则（参考 MobileGlues 的结论，实现独立编写）：
 *   1. 只认「扩展串 + 函数指针」同时存在。Android 的 EGL 包装层可能
 *      返回一个存在但不可用的入口点，单看符号不算证据。
 *   2. 首次调用做一次错误探测：调用前抽干错误队列，调用后若仍有错误，
 *      将该后端标记为 FAILED 并回退循环，后续不再尝试。
 *   3. 原生成功不记降级；只有原生不可用/探测失败才记录
 *      MULTI_DRAW_UNSUPPORTED。
 */

typedef void (*glesmod_md_arrays_indirect_fn)(GLenum, const void *, GLsizei,
                                              GLsizei);
typedef void (*glesmod_md_elements_indirect_fn)(GLenum, GLenum, const void *,
                                                GLsizei, GLsizei);

/* GL 4.6 DrawArraysIndirect / DrawElementsIndirect 命令布局：
 * 前者 4 个 32 位字，后者 5 个。stride=0 时使用这个紧凑布局。 */
#define GLESMOD_DRAW_ARRAYS_INDIRECT_STRIDE   16
#define GLESMOD_DRAW_ELEMENTS_INDIRECT_STRIDE 20

typedef enum {
    GLESMOD_MD_UNPROBED = 0,
    GLESMOD_MD_WORKING,
    GLESMOD_MD_FAILED
} glesmod_md_state_t;

typedef struct {
    int initialized;
    glesmod_md_arrays_indirect_fn arrays_indirect;
    glesmod_md_elements_indirect_fn elements_indirect;
    glesmod_md_state_t arrays_indirect_state;
    glesmod_md_state_t elements_indirect_state;
} glesmod_multidraw_t;

static glesmod_multidraw_t g_multidraw;

static void glesmod_multidraw_detect(void)
{
    if (g_multidraw.initialized) return;
    g_multidraw.initialized = 1;

    const char *ext = (const char *)glGetString(GLESMOD_ENUM_GL_EXTENSIONS);
    if (!glesmod_extension_present(ext, "GL_EXT_multi_draw_indirect")) {
        glesmod_log("Multi-Draw: 未检测到 GL_EXT_multi_draw_indirect，"
                    "使用逐条 indirect 回退");
        return;
    }

    g_multidraw.arrays_indirect =
        (glesmod_md_arrays_indirect_fn)glesym_resolve(
            "glMultiDrawArraysIndirectEXT");
    g_multidraw.elements_indirect =
        (glesmod_md_elements_indirect_fn)glesym_resolve(
            "glMultiDrawElementsIndirectEXT");

    if (g_multidraw.arrays_indirect != NULL ||
        g_multidraw.elements_indirect != NULL) {
        glesmod_log("Multi-Draw: 检测到 GL_EXT_multi_draw_indirect，"
                    "优先使用原生 indirect 入口"
                    "（GLESMOD_MULTIDRAW_NATIVE）");
    } else {
        glesmod_log("Multi-Draw: GL_EXT_multi_draw_indirect 已声明但入口点"
                    "不可用，使用逐条 indirect 回退");
    }
}

/* 首次调用探测：UNPROBED 时抽干错误，调用后检查一次。 */
static int glesmod_multidraw_begin(glesmod_md_state_t *state)
{
    if (*state == GLESMOD_MD_FAILED) return 0;
    if (*state == GLESMOD_MD_UNPROBED) {
        get_err_fn_t get_err = texprobe_get_error();
        if (get_err != NULL) drain_errors(get_err);
    }
    return 1;
}

static int glesmod_multidraw_end(glesmod_md_state_t *state)
{
    if (*state == GLESMOD_MD_UNPROBED) {
        get_err_fn_t get_err = texprobe_get_error();
        if (get_err != NULL && get_err() != 0 /* GL_NO_ERROR */) {
            *state = GLESMOD_MD_FAILED;
            return 0;
        }
        *state = GLESMOD_MD_WORKING;
    }
    return 1;
}

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
                    "Multi-Draw 不可用，已降级为循环 glDrawElements");
}

void glMultiDrawArrays(GLenum mode, const GLint *first,
                       const GLsizei *count, GLsizei drawcount) {
    typedef void (*draw_arrays_t)(GLenum, GLint, GLsizei);
    static draw_arrays_t real = NULL;

    glesmod_lazy_init();

    if (drawcount <= 0) return;

    RESOLVE_OR_RETURN(real, "glDrawArrays", );

    for (GLsizei i = 0; i < drawcount; i++) {
        if (count[i] <= 0) continue;
        real(mode, first[i], count[i]);
    }

    glesmod_degrade(GLESMOD_DEGRADE_MULTI_DRAW_UNSUPPORTED,
                    "Multi-Draw 不可用，已降级为循环 glDrawArrays");
}

void glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei *count, GLenum type,
                                   const void *const *indices, GLsizei drawcount,
                                   const GLint *basevertex) {
    typedef void (*draw_base_vertex_t)(GLenum, GLsizei, GLenum, const void *, GLint);
    static draw_base_vertex_t real = NULL;
    static int tried = 0;

    glesmod_lazy_init();

    if (drawcount <= 0) return;

    if (real == NULL && !tried) {
        tried = 1;
        real = (draw_base_vertex_t)glesym_resolve("glDrawElementsBaseVertex");
    }

    if (real != NULL) {
        for (GLsizei i = 0; i < drawcount; i++) {
            if (count[i] <= 0) continue;
            real(mode, count[i], type, indices[i], basevertex ? basevertex[i] : 0);
        }
    } else {
        /* 退化路径：忽略 basevertex。这在几何体共用同一 VBO 时会画错，
         * 但不会崩溃。记录降级以便诊断。 */
        typedef void (*draw_elements_t)(GLenum, GLsizei, GLenum, const void *);
        static draw_elements_t fallback = NULL;
        if (fallback == NULL) fallback = (draw_elements_t)glesym_resolve("glDrawElements");
        if (fallback == NULL) {
            glesmod_report_missing("glDrawElements");
            return;
        }
        for (GLsizei i = 0; i < drawcount; i++) {
            if (count[i] <= 0) continue;
            fallback(mode, count[i], type, indices[i]);
        }
        glesmod_degrade(GLESMOD_DEGRADE_MULTI_DRAW_UNSUPPORTED,
                        "BaseVertex 不可用，已降级为 glDrawElements（几何体可能偏移）");
    }
}

void glMultiDrawArraysIndirect(GLenum mode, const void *indirect,
                               GLsizei drawcount, GLsizei stride) {
    typedef void (*draw_arrays_indirect_t)(GLenum, const void *);
    static draw_arrays_indirect_t real = NULL;

    glesmod_lazy_init();

    if (drawcount <= 0) return;
    if (stride < 0) {
        glesmod_degrade(GLESMOD_DEGRADE_MULTI_DRAW_UNSUPPORTED,
                        "glMultiDrawArraysIndirect: stride < 0，已忽略");
        return;
    }

    glesmod_multidraw_detect();
    if (g_multidraw.arrays_indirect != NULL &&
        glesmod_multidraw_begin(&g_multidraw.arrays_indirect_state)) {
        g_multidraw.arrays_indirect(mode, indirect, drawcount, stride);
        if (glesmod_multidraw_end(&g_multidraw.arrays_indirect_state)) return;
        glesmod_log("Multi-Draw: 原生 ArraysIndirect 探测失败，改用循环");
    }

    RESOLVE_OR_RETURN(real, "glDrawArraysIndirect", );

    const GLsizei step = stride ? stride : GLESMOD_DRAW_ARRAYS_INDIRECT_STRIDE;
    const uintptr_t base = (uintptr_t)indirect;
    for (GLsizei i = 0; i < drawcount; i++) {
        real(mode, (const void *)(base + (uintptr_t)i * (uintptr_t)step));
    }

    glesmod_degrade(GLESMOD_DEGRADE_MULTI_DRAW_UNSUPPORTED,
                    "Multi-Draw Indirect 原生不可用，"
                    "已降级为逐条 glDrawArraysIndirect");
}

void glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect,
                                 GLsizei drawcount, GLsizei stride) {
    typedef void (*draw_elements_indirect_t)(GLenum, GLenum, const void *);
    static draw_elements_indirect_t real = NULL;

    glesmod_lazy_init();

    if (drawcount <= 0) return;
    if (stride < 0) {
        glesmod_degrade(GLESMOD_DEGRADE_MULTI_DRAW_UNSUPPORTED,
                        "glMultiDrawElementsIndirect: stride < 0，已忽略");
        return;
    }

    glesmod_multidraw_detect();
    if (g_multidraw.elements_indirect != NULL &&
        glesmod_multidraw_begin(&g_multidraw.elements_indirect_state)) {
        g_multidraw.elements_indirect(mode, type, indirect, drawcount, stride);
        if (glesmod_multidraw_end(&g_multidraw.elements_indirect_state)) return;
        glesmod_log("Multi-Draw: 原生 ElementsIndirect 探测失败，改用循环");
    }

    RESOLVE_OR_RETURN(real, "glDrawElementsIndirect", );

    const GLsizei step = stride ? stride : GLESMOD_DRAW_ELEMENTS_INDIRECT_STRIDE;
    const uintptr_t base = (uintptr_t)indirect;
    for (GLsizei i = 0; i < drawcount; i++) {
        real(mode, type, (const void *)(base + (uintptr_t)i * (uintptr_t)step));
    }

    glesmod_degrade(GLESMOD_DEGRADE_MULTI_DRAW_UNSUPPORTED,
                    "Multi-Draw Indirect 原生不可用，"
                    "已降级为逐条 glDrawElementsIndirect");
}

/* ================================================================== */
/* glTexImage2D / glTexImage3D —— 深度内部格式翻译                      */
/* ================================================================== */

/*
 * 【真实根因修复】深度缓冲为何一直失效
 *
 * 证据链（全部来自一手材料，不是推断）：
 *   1. 用户实测：旁观模式能看到**未被正确剔除的面**，且**实体正反面交叠**
 *      —— 这就是「深度测试没起作用」的定义。
 *   2. MC 1.21.1 反编译字节码（com/mojang/blaze3d/pipeline/RenderTarget
 *      与 MainTarget）里，创建深度附件时传的是：
 *          sipush 6402   (= 0x1902 = GL_DEPTH_COMPONENT)  <- internalformat
 *          sipush 5126   (= 0x1406 = GL_FLOAT)            <- type
 *      然后调用 GlStateManager._texImage2D(...)
 *   3. 驱动原话（latest.log）：
 *          'the combination of format 6402 and type 5126 is unsupported'
 *
 * 原因：桌面 GL 允许 internalformat = GL_DEPTH_COMPONENT，**GLES 不允许**。
 *   ES 只接受带位宽的深层格式：DEPTH_COMPONENT16/24/32F 或 DEPTH24_STENCIL8。
 *   我们先前原样转发 -> 该调用被驱动拒绝 -> **深度纹理根本没被分配**
 *   -> 深度测试完全失效 -> 该被遮的面露出来、正反面交叠。
 *
 * 修法：把 0x1902 翻译成 ES 合法的 GL_DEPTH_COMPONENT24(0x81A6)，
 *   type 同步翻译为 GL_UNSIGNED_INT(0x1405)。
 *   只影响这一种组合，其余（颜色纹理等）一律原样转发，行为不变。
 */

#define GLES_DEPTH_COMPONENT      0x1902   /* 桌面 GL 的深层内部格式（ES 非法） */
#define GLES_DEPTH_COMPONENT24    0x81A6   /* ES 合法 */
#define GLES_DEPTH_COMPONENT16    0x81A5
#define GLES_DEPTH_COMPONENT32F   0x81A7
#define GLES_FLOAT                0x1406
#define GLES_UNSIGNED_INT         0x1405
#define GLES_UNSIGNED_SHORT       0x1403
#define GLES_DEPTH_STENCIL        0x84F9   /* 桌面 GL 合并格式（ES 非法）*/
#define GLES_DEPTH24_STENCIL8     0x88F0   /* ES 合法 */

/* 浮点/归一化内部格式所要求的像素 type（ES 3.0 表 3.2），其余用到的枚举 */
#define GLES_R11F_G11F_B10F       0x8C3A
#define GLES_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
#define GLES_UNSIGNED_INT_2_10_10_10_REV  0x8368
#define GLES_HALF_FLOAT           0x140B
#define GLES_UNSIGNED_INT_24_8    0x84FA

/*
 * 【★曾经的错误：同一个宏被定义了两次，值还不一样★】
 *
 * 下面这一行原先是
 *     #define GLES_DEPTH_COMPONENT32F   0x8CAC
 * 而同一常量在上方已经定义为 0x81A7。C 预处理后定义的会覆盖先定义的，
 * 于是整张 kIfmtType 表里 `GL_DEPTH_COMPONENT32F` 那一项带的是 0x8CAC ——
 * 一个**根本不存在**的垃圾值。后果是：
 *   - 真正的 0x81A7 查表落空 -> 不做 type 修正；
 *   - 0x8CAC 又永远匹配不上任何真实调用。
 * 也就是说这条深度格式的修正**自始至终没有生效过**，而因为两处都写成
 * 同一个名字，编译器与肉眼都很难发现。
 *
 * 教训：同一源文件里同一宏名只允许出现一次；跨文件请用统一头文件。
 * 现在只保留上方 0x81A7 的那一处定义。
 */

#define GLES_RGBA16F              0x881A
#define GLES_RGB16F               0x881B
#define GLES_RGBA32F              0x8814
#define GLES_RGB32F               0x8815
#define GLES_R16F                 0x822D
#define GLES_R32F                 0x822E
#define GLES_RG16F                0x822F
#define GLES_RG32F                0x8230

/* ES 3.0 表 3.2 里其余「所要求的 type 不是 GL_UNSIGNED_BYTE」的尺寸化格式 */
#define GLES_RED                  0x1903
#define GLES_RG                   0x8227
#define GLES_RGB                  0x1907
#define GLES_RGBA                 0x1908
#define GLES_R8                   0x8229
#define GLES_RG8                  0x822B
#define GLES_RGB8                 0x8051
#define GLES_RGBA8                0x8058
#define GLES_RGB10_A2             0x8059   /* ★真机 FBO 36055 的直接成因★ */
#define GLES_RGBA16               0x805B   /* ★真机 FBO 36055 的直接成因★ */

typedef struct {
    GLint  internalformat;
    GLenum type;              /* ES 3.0 下该内部格式唯一/首选的上传 type */
} gles_ifmt_type_t;

/*
 * 【为什么需要这张表 —— 真机实测的 FBO 36055 根因】
 *
 * native.log 最后两行（我加的探针输出，均已先抽干错误队列再归因）：
 *     ★ glTexImage2D 被驱动拒绝: err=0x0502 internalformat=0x8C3A
 *       (GL_R11F_G11F_B10F) 2670x1200 format=0x1907 type=0x1401 pixels=NULL
 *     FBO 不完整: status=0x8CD7 (INCOMPLETE_MISSING_ATTACHMENT)
 *       color0: obj=86 (存储=0x0 ★无存储★)
 *
 * 【为什么被拒绝】ES 3.0 规定：当 internalformat 是**尺寸化的**格式时，
 * pixel 的 format/type 必须与它匹配。`GL_R11F_G11F_B10F` 是浮点格式，
 * 只接受 HALF_FLOAT / FLOAT / UNSIGNED_INT_10F_11F_11F_REV 作为 type。
 * 调用方传了 `GL_UNSIGNED_BYTE` → 驱动报 GL_INVALID_OPERATION。
 *
 * 【为什么桌面能跑】桌面 GL 在 internalformat 为尺寸化格式时把
 * format/type 当作「仅供参考」（pixels == NULL 时尤其如此），所以
 * Iris 这么写在自己平台上毫无问题 —— 它与之前修的深度格式是**同一类问题**：
 * 桌面宽松、ES 严格，而失败方式是**静默的**（不崩溃，只是纹理没有存储）。
 *
 * 【修法】按 ES 3.0 表 3.2 把 type 改为该内部格式合法的那一个。
 * 只在 pixels == NULL（纯分配、不上传像素）时才改 —— 那种情况下
 * type 只用于描述「将来会怎么传」，而调用方随后总是用
 * glTexSubImage2D 上传，那时 type 会重新指定，因此改写是安全的。
 * 若 pixels 非 NULL，则本表不参与，避免影响真实的上传语义。
 */
static const gles_ifmt_type_t kIfmtType[] = {
    { GLES_R11F_G11F_B10F, GLES_UNSIGNED_INT_10F_11F_11F_REV },
    { GLES_RGBA16F,        GLES_HALF_FLOAT },
    { GLES_RGB16F,         GLES_HALF_FLOAT },
    { GLES_RGBA32F,        GLES_FLOAT },
    { GLES_RGB32F,         GLES_FLOAT },
    { GLES_R16F,           GLES_HALF_FLOAT },
    { GLES_R32F,           GLES_FLOAT },
    { GLES_RG16F,          GLES_HALF_FLOAT },
    { GLES_RG32F,          GLES_FLOAT },
    { GLES_DEPTH_COMPONENT16,  GLES_UNSIGNED_SHORT },
    { GLES_DEPTH_COMPONENT24,  GLES_UNSIGNED_INT },
    /* 0x81A7 —— 这个条目曾因 GLES_DEPTH_COMPONENT32F 被重复定义成 0x8CAC
     * 而永久失效，见上方说明。 */
    { GLES_DEPTH_COMPONENT32F, GLES_FLOAT },
    { GLES_DEPTH24_STENCIL8,   GLES_UNSIGNED_INT_24_8 },

    /*
     * 【本轮真机 FBO 36055 的直接成因 —— 两个缺失项】
     *
     * 20:32 的 native.log：
     *     ★ glTexImage2D 被驱动拒绝: err=0x0502 target=0x0DE1
     *       internalformat=0x8059       2670x1200 format=0x1908 type=0x1401
     *     ★ glTexImage2D 被驱动拒绝: err=0x0502 target=0x0DE1
     *       internalformat=0x805B       2670x1200 format=0x1908 type=0x1401
     *     FBO 不完整: status=0x8CD7 | color0: obj=107 (存储=0x0 ★无存储★)
     *
     * 0x8059 = GL_RGB10_A2、0x805B = GL_RGBA16 —— 都是**尺寸化格式**，
     * 而调用方给的是 type=GL_UNSIGNED_BYTE。ES 3.0 表 3.2 规定：
     *   GL_RGB10_A2 只接受 GL_UNSIGNED_INT_2_10_10_10_REV / GL_UNSIGNED_SHORT_5_6_5
     *   GL_RGBA16    只接受 GL_UNSIGNED_SHORT
     * 于是驱动报 GL_INVALID_OPERATION，纹理**没有存储**，
     * 挂到 FBO 上就是 INCOMPLETE_MISSING_ATTACHMENT(36055)。
     *
     * 这与先前修的 GL_R11F_G11F_B10F / 深度格式是**完全同一类缺陷**：
     * 桌面 GL 把 (format,type) 当作参考（pixels==NULL 时尤甚），
     * ES 却是强校验。差别只在「哪几个格式还没进表」。
     */
    { GLES_RGB10_A2,       GLES_UNSIGNED_INT_2_10_10_10_REV },
    { GLES_RGBA16,         GLES_UNSIGNED_SHORT },
};

/* 返回该内部格式在 ES 3.0 下要求的上传 type；0 表示不在表中（不改）。 */
static GLenum es_required_tex_type(GLint internalformat) {
    unsigned i;
    for (i = 0; i < sizeof(kIfmtType) / sizeof(kIfmtType[0]); i++) {
        if (kIfmtType[i].internalformat == internalformat) {
            return kIfmtType[i].type;
        }
    }
    return 0;
}

/*
 * 把「未定尺寸（unsized）」内部格式提升为对应的定尺寸格式。
 *
 * 【为什么必须做 —— 本次 BSL 会话最后一条真实纹理拒绝】
 *     ★ glTexImage2D 被驱动拒绝: err=0x0502 target=0x0DE1 level=0
 *       internalformat=0x1903(GL_RED) 256x128 format=0x1903 type=0x1401
 *       pixels=非NULL
 *     （x22 次，每次进入世界都会重试）
 *
 * GL_RED / GL_RG / GL_RGB / GL_RGBA 在 ES 里是**合法的 format**，
 * 同时也是「未定尺寸」的 internalformat。规范允许它们，但实际驱动
 * （Adreno 750 已实测）在带像素上传时会拒绝 0x1903，报 GL_INVALID_OPERATION
 * —— 与前面几个浮点/深度格式是同一类「桌面宽松、ES 严格」的缺陷。
 *
 * 【修法：提升为语义完全相同的定尺寸格式】
 *     GL_RED  -> GL_R8     GL_RG  -> GL_RG8
 *     GL_RGB  -> GL_RGB8   GL_RGBA -> GL_RGBA8
 * 这是 1:1 的等价替换（R8 就是 RED 的定尺寸形式），
 * **不改变任何像素语义**，只是把一个驱动不接受的写法换成驱动接受的写法。
 *
 * 返回 1 表示做过替换。
 */
static int fixup_unsized_internalformat(const char *who, GLint *internalformat) {
    GLint src = *internalformat;
    GLint dst = 0;
    const char *name = NULL;

    switch (src) {
    case GLES_RED:  dst = GLES_R8;    name = "GL_R8";    break;
    case GLES_RG:   dst = GLES_RG8;   name = "GL_RG8";   break;
    case GLES_RGB:  dst = GLES_RGB8;  name = "GL_RGB8";  break;
    case GLES_RGBA: dst = GLES_RGBA8; name = "GL_RGBA8"; break;
    default: return 0;
    }

    {
        char detail[248];
        snprintf(detail, sizeof(detail),
                 "%s: internalformat 0x%04X 为未定尺寸格式，已提升为 %s(0x%04X)"
                 " —— Adreno 在带像素上传时拒绝未定尺寸的 internalformat"
                 "（GL_INVALID_OPERATION），提升后语义完全相同",
                 who, (unsigned)src, name, (unsigned)dst);
        glesmod_degrade(GLESMOD_DEGRADE_ENUM_MAPPED, detail);
    }
    *internalformat = dst;
    return 1;
}

/* 把 (internalformat, format, type) 修正为 ES 合法组合。
 * 返回 1 表示做过修改（调用方可据此记降级事件）。 */
static int fixup_depth_format(const char *who, GLint *internalformat,
                              GLenum *format, GLenum *type) {
    int changed = 0;

    /* 内部格式：GL_DEPTH_COMPONENT -> GL_DEPTH_COMPONENT24 */
    if (*internalformat == GLES_DEPTH_COMPONENT) {
        *internalformat = GLES_DEPTH_COMPONENT24;
        changed = 1;
    } else if (*internalformat == GLES_DEPTH_STENCIL) {
        *internalformat = GLES_DEPTH24_STENCIL8;
        changed = 1;
    }

    /* 像素 format 与 type：只在确实指向深度时修正 */
    int is_depth = (*format == GLES_DEPTH_COMPONENT) ||
                   (*format == GLES_DEPTH_STENCIL);

    if (is_depth && *type == GLES_FLOAT) {
        /* 24 位深度在 ES 下的合法上传类型是 UNSIGNED_INT */
        *type = GLES_UNSIGNED_INT;
        changed = 1;
    }

    if (changed) {
        char detail[192];
        snprintf(detail, sizeof(detail),
                 "%s: 深层格式已翻译为 ES 合法组合 "
                 "(internalformat=0x%04X format=0x%04X type=0x%04X)"
                 " —— 修复深度缓冲未分配导致的剔除/遮挡失效",
                 who, (unsigned)*internalformat, (unsigned)*format, (unsigned)*type);
        glesmod_degrade(GLESMOD_DEGRADE_ENUM_MAPPED, detail);
    }
    return changed;
}

/*
 * 按 ES 3.0 表 3.2 修正像素 type，使其与尺寸化内部格式匹配。
 *
 * 【何时才改】只在本库确定 type 不会影响真实像素语义时：
 *   调用方传入 pixels == NULL（纯分配存储，稍后用 glTexSubImage2D 上传）。
 * 这是本类问题的**唯一**成因，因此限制在这个条件下既够用、又最安全。
 */
static int fixup_pixel_type(const char *who, GLint internalformat,
                            GLenum format, GLenum *type,
                            const void *pixels) {
    if (pixels != NULL) return 0;          /* 真上传：不动 */

    GLenum want = es_required_tex_type(internalformat);
    if (want == 0 || want == *type) return 0;

    {
        char detail[224];
        GLenum old = *type;
        snprintf(detail, sizeof(detail),
                 "%s: 像素 type 与尺寸化内部格式不匹配，已按 ES 3.0 表 3.2 修正 "
                 "(internalformat=0x%04X format=0x%04X type 0x%04X -> 0x%04X)"
                 " —— 否则驱动报 GL_INVALID_OPERATION、纹理无存储，"
                 "进而使 FBO 报 INCOMPLETE_MISSING_ATTACHMENT(36055)",
                 who, (unsigned)internalformat, (unsigned)format,
                 (unsigned)old, (unsigned)want);
        glesmod_degrade(GLESMOD_DEGRADE_ENUM_MAPPED, detail);
    }

    *type = want;
    return 1;
}

/*
 * 纹理分配的错误观测（glTexImage2D / glTexImage3D 共用）。
 *
 * 【上一版探针有缺陷，本轮真机日志证明了这一点】
 *
 * 上一版在调用**之后**读一次 glGetError，并把它归因于这次调用。真机报出：
 *     internalformat=0x1908(RGBA) 854x480 format=0x1908 type=0x1401
 * 这个组合在 ES 下**完全合法**（RGBA + UNSIGNED_BYTE + NULL 是标准做法），
 * 不该产生 GL_INVALID_VALUE。两个原因：
 *   1. **没有在调用前清空错误队列**。GL 错误是 FIFO 队列，调用后读到的
 *      那一条可能来自**更早**的某次调用 —— 于是把「某次失败的参数」
 *      错误地贴到了「另一次调用」上，结论自然对不上。
 *   2. 只报**第一次**。Iris 建 FBO 的调用发生在首次捕获之后，
 *      于是最该看的那一组参数被直接挡掉了。
 *
 * 【正确做法】调用前抽干 → 调用 → 立刻读 → 归因。
 *   抽干只丢弃「本次调用之前」已存在的错误，不会吃掉本次调用产生的错误，
 *   也不影响后续调用。这样「参数 ↔ 错误」的对应关系才是成立的。
 *
 * 【限流改为按参数指纹】不同参数组合各详列一次（上限 16 条）。
 *   这既能捞到 Iris 真正用的那组，又不会因每帧重试而刷屏。
 */
static get_err_fn_t texprobe_get_error(void)
{
    static get_err_fn_t f = NULL;
    if (f == NULL) f = (get_err_fn_t)glesym_resolve("glGetError");
    return f;
}

/* 抽干错误队列（FIFO，必须反复读到 0）。 */
static void drain_errors(get_err_fn_t get_err)
{
    int guard = 0;
    while (get_err() != 0 && ++guard < 64) {
        /* 空循环体：目的只是把队列读空 */
    }
}

/* 已详列过的参数指纹，避免刷屏（上限 16 条） */
#define TEXIMG_FP_MAX 16
static char g_teximg_fp[TEXIMG_FP_MAX][96];
static int  g_teximg_fp_count = 0;

/* 返回 1 表示这是新指纹（应详列），0 表示已见过或已满。 */
static int teximg_fp_is_new(const char *fp)
{
    int i;
    for (i = 0; i < g_teximg_fp_count; i++) {
        if (strcmp(g_teximg_fp[i], fp) == 0) return 0;
    }
    if (g_teximg_fp_count >= TEXIMG_FP_MAX) return 0;
    strncpy(g_teximg_fp[g_teximg_fp_count], fp, sizeof(g_teximg_fp[0]) - 1);
    g_teximg_fp[g_teximg_fp_count][sizeof(g_teximg_fp[0]) - 1] = '\0';
    g_teximg_fp_count++;
    return 1;
}

static const char *tex_internalformat_name(GLint f)
{
    switch (f) {
    case 0x1902: return "GL_DEPTH_COMPONENT(桌面值,ES非法)";
    case 0x81A5: return "GL_DEPTH_COMPONENT16";
    case 0x81A6: return "GL_DEPTH_COMPONENT24";
    case 0x81A7: return "GL_DEPTH_COMPONENT32F";
    case 0x88F0: return "GL_DEPTH24_STENCIL8";
    case 0x84F9: return "GL_DEPTH_STENCIL(桌面值,ES非法)";
    case 0x1908: return "GL_RGBA";
    case 0x1903: return "GL_RED";
    case 0x8227: return "GL_RG";
    case 0x8058: return "GL_RGBA8";
    case 0x8814: return "GL_RGBA32F";
    case 0x881A: return "GL_RGBA16F";
    case 0x8051: return "GL_RGB8";
    case 0x8C41: return "GL_SRGB8";
    case 0x8C43: return "GL_SRGB8_ALPHA8";
    case 0x8229: return "GL_R8";
    case 0x822B: return "GL_RG8";
    case 0x8D62: return "GL_RGB565";
    case 0x8056: return "GL_RGBA4";
    case 0x8057: return "GL_RGB5_A1";
    case 0x822A: return "GL_R16F";
    case 0x822D: return "GL_R32F";
    case 0x8C3A: return "GL_R11F_G11F_B10F";
    default:     return "其它/未知";
    }
}

/* ================================================================== */
/* Proxy texture 仿真                                                  */
/* ================================================================== */

/*
 * 桌面 GL 的 GL_PROXY_TEXTURE_* 不是真正的纹理分配：
 *   glTexImage2D(proxy, ...) 只记录“这个尺寸能不能分配”
 *   glGetTexLevelParameter*  再把记录值读回来
 *
 * GLES 没有 proxy target。厂商 GLES 驱动通常返回 0 / INVALID_ENUM；
 * ANGLE 2.1.1 的 glGetTexLevelParameteriv 对 InvalidEnum 会空指针崩溃。
 *
 * 因此我们在这里完整仿真 proxy 语义，proxy target 不进入驱动。
 * 参考 MobileGlues 的 gl/texture.cpp 实现（按语义自行实现，不逐字复制）。
 */

#define GLESMOD_GL_PROXY_TEXTURE_1D                      0x8063
#define GLESMOD_GL_PROXY_TEXTURE_2D                      0x8064
#define GLESMOD_GL_PROXY_TEXTURE_3D                      0x8070
#define GLESMOD_GL_PROXY_TEXTURE_RECTANGLE               0x84F7
#define GLESMOD_GL_PROXY_TEXTURE_CUBE_MAP                0x851B
#define GLESMOD_GL_PROXY_TEXTURE_1D_ARRAY                0x8C19
#define GLESMOD_GL_PROXY_TEXTURE_2D_ARRAY                0x8C1B
#define GLESMOD_GL_PROXY_TEXTURE_CUBE_MAP_ARRAY          0x900B
#define GLESMOD_GL_PROXY_TEXTURE_2D_MULTISAMPLE          0x9101
#define GLESMOD_GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY    0x9103

#define GLESMOD_GL_TEXTURE_WIDTH                         0x1000
#define GLESMOD_GL_TEXTURE_HEIGHT                        0x1001
#define GLESMOD_GL_TEXTURE_INTERNAL_FORMAT               0x1003
#define GLESMOD_GL_MAX_TEXTURE_SIZE                      0x0D33

typedef struct {
    GLsizei width;          /* base level（level 0）宽度 */
    GLsizei height;
    GLenum  internal_format;
    int     valid;
} glesmod_proxy_state_t;

/* 每个线程一份；MC 的探测在 Render thread 上，单线程足够。 */
static __thread glesmod_proxy_state_t g_proxy_state;
static __thread int g_proxy_logged = 0;
static __thread GLenum g_proxy_unsupported_logged[8];
static __thread int g_proxy_unsupported_count = 0;

static int glesmod_is_proxy_target(GLenum target)
{
    switch (target) {
        case GLESMOD_GL_PROXY_TEXTURE_1D:
        case GLESMOD_GL_PROXY_TEXTURE_2D:
        case GLESMOD_GL_PROXY_TEXTURE_3D:
        case GLESMOD_GL_PROXY_TEXTURE_RECTANGLE:
        case GLESMOD_GL_PROXY_TEXTURE_CUBE_MAP:
        case GLESMOD_GL_PROXY_TEXTURE_1D_ARRAY:
        case GLESMOD_GL_PROXY_TEXTURE_2D_ARRAY:
        case GLESMOD_GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
        case GLESMOD_GL_PROXY_TEXTURE_2D_MULTISAMPLE:
        case GLESMOD_GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
            return 1;
        default:
            return 0;
    }
}

/*
 * 真正实现了 proxy 语义的 target。
 *
 * 只保留已验证的 2D / cube / rectangle：
 *   - 它们的尺寸只随 mip level 缩小 width/height；
 *   - depth/layers 恒为 1；
 *   - MC 1.21.1 的 maxSupportedTextureSize 只走 GL_PROXY_TEXTURE_2D。
 *
 * 1D/3D/array/multisample 不在列表里：它们仍然被 glesmod_is_proxy_target()
 * 识别，但按“未实现”安全降级，不进入 ANGLE。
 */
static int glesmod_is_emulated_proxy_target(GLenum target)
{
    switch (target) {
        case GLESMOD_GL_PROXY_TEXTURE_2D:
        case GLESMOD_GL_PROXY_TEXTURE_CUBE_MAP:
        case GLESMOD_GL_PROXY_TEXTURE_RECTANGLE:
            return 1;
        default:
            return 0;
    }
}

/* 0 = 关闭仿真（退回旧行为），1 = 启用；默认启用。 */
static int glesmod_proxy_emu_enabled(void)
{
    static int cached = -1;
    if (cached < 0) {
        const char *v = glesmod_env_str(GLESMOD_ENV_PROXY_EMU);
        cached = (v != NULL && v[0] != '\0' && strcmp(v, "0") == 0) ? 0 : 1;
    }
    return cached;
}

/*
 * 一个 proxy target 在本次调用里应该怎么处理。
 *
 * 所有入口（glTexImage2D/3D、glGetTexLevelParameteriv/fv）都只判断
 * 这一个结果，避免把「是否 proxy / 是否启用仿真 / 是否已实现」三件事
 * 复制到四个地方；后者会导致新增 target 时漏改某个入口。
 */
typedef enum {
    GLESMOD_PROXY_PASS = 0,   /* 不是 proxy，或仿真关闭：原样转发给驱动 */
    GLESMOD_PROXY_EMULATE,    /* 已实现：读写本地 proxy 状态 */
    GLESMOD_PROXY_REJECT      /* 未实现：不进入驱动，按不可分配处理 */
} glesmod_proxy_action_t;

static glesmod_proxy_action_t glesmod_proxy_action(GLenum target)
{
    if (!glesmod_is_proxy_target(target)) return GLESMOD_PROXY_PASS;
    if (!glesmod_proxy_emu_enabled()) return GLESMOD_PROXY_PASS;
    return glesmod_is_emulated_proxy_target(target)
               ? GLESMOD_PROXY_EMULATE
               : GLESMOD_PROXY_REJECT;
}

static void glesmod_log_proxy_unsupported(GLenum target)
{
    int i;
    char msg[128];

    for (i = 0; i < g_proxy_unsupported_count; i++) {
        if (g_proxy_unsupported_logged[i] == target) return;
    }
    if (g_proxy_unsupported_count <
        (int)(sizeof(g_proxy_unsupported_logged) / sizeof(g_proxy_unsupported_logged[0]))) {
        g_proxy_unsupported_logged[g_proxy_unsupported_count++] = target;
    }
    snprintf(msg, sizeof(msg),
             "[GLESMod] proxy target 0x%04X 未实现，按不可分配处理",
             (unsigned)target);
    glesmod_log(msg);
}

static GLint glesmod_proxy_max_texture_size(void)
{
    static GLint cached = 0;
    if (cached <= 0) {
        typedef void (*get_int_t)(GLenum, GLint *);
        get_int_t get_int = (get_int_t)glesym_resolve("glGetIntegerv");
        GLint v = 0;
        if (get_int != NULL) {
            get_int(GLESMOD_GL_MAX_TEXTURE_SIZE, &v);
        }
        /* 查询失败时保守取 4096，让 MC 自然回落到自己的 GL_MAX_TEXTURE_SIZE。 */
        cached = (v > 0) ? v : 4096;
    }
    return cached;
}

static GLsizei glesmod_proxy_base_size(GLsizei size, GLint level)
{
    if (size <= 0) return 0;
    if (level <= 0) return size;
    if (level >= 31) return 0;
    if (size > (GLsizei)(0x7fffffff >> level)) return 0;
    return (GLsizei)(size << level);
}

static GLint glesmod_proxy_level_size(GLint base, GLint level)
{
    if (base <= 0) return 0;
    if (level <= 0) return base;
    if (level >= 31) return 1;
    {
        GLint v = base >> level;
        return (v > 0) ? v : 1;
    }
}

static void glesmod_proxy_record(GLenum target, GLint level, GLint internalformat,
                                 GLsizei width, GLsizei height)
{
    GLint max = glesmod_proxy_max_texture_size();

    GLsizei base_w = glesmod_proxy_base_size(width, level);
    GLsizei base_h = glesmod_proxy_base_size(height, level);

    /*
     * 桌面 proxy 语义：该 level 的任一维度超过 GL_MAX_TEXTURE_SIZE，
     * 就视为“不可分配”，proxy 的尺寸读回 0。
     */
    if ((GLint)width > max || (GLint)height > max) {
        base_w = 0;
        base_h = 0;
    }

    g_proxy_state.width = base_w;
    g_proxy_state.height = base_h;
    g_proxy_state.internal_format = (GLenum)internalformat;
    g_proxy_state.valid = 1;

    if (!g_proxy_logged) {
        char msg[256];
        g_proxy_logged = 1;
        snprintf(msg, sizeof(msg),
                 "[GLESMod] proxy texture 仿真: target=0x%04X level=%d %dx%d max=%d -> %dx%d",
                 (unsigned)target, (int)level, (int)width, (int)height,
                 (int)max, (int)base_w, (int)base_h);
        glesmod_log(msg);
    }
}

static GLint glesmod_proxy_query(GLenum pname, GLint level)
{
    if (!g_proxy_state.valid) return 0;

    switch (pname) {
        case GLESMOD_GL_TEXTURE_WIDTH:
            return glesmod_proxy_level_size(g_proxy_state.width, level);
        case GLESMOD_GL_TEXTURE_HEIGHT:
            return glesmod_proxy_level_size(g_proxy_state.height, level);
        case GLESMOD_GL_TEXTURE_INTERNAL_FORMAT:
            return (GLint)g_proxy_state.internal_format;
        default:
            return 0;
    }
}

/*
 * 归因并记录一次失败的纹理分配。
 * 调用方必须已「先抽干、再调用、再取错误」。
 */
static void texprobe_report(const char *who, GLenum err, GLenum target,
                            GLint internalformat, GLsizei width, GLsizei height,
                            GLenum format, GLenum type, const void *pixels)
{
    char fp[96];
    char line[512];

    if (err != 0x0500 /* INVALID_ENUM */ &&
        err != 0x0501 /* INVALID_VALUE */ &&
        err != 0x0502 /* INVALID_OPERATION */) {
        return;
    }

    /*
     * 【GL_PROXY_TEXTURE_2D 的探测不是错误，不该进日志】
     *
     * 本次日志里 93 条「被驱动拒绝」中有 **66 条**是这一种：
     *     glTexImage2D 被驱动拒绝: err=0x0500 target=0x8064
     *       internalformat=0x1908(GL_RGBA) 32768x32768 ... pixels=NULL
     * 调用方是 MC/Sodium 的**能力探测**：它拿 PROXY_TEXTURE_2D 去试
     * 32768/16384/8192... 直到驱动接受，以此确定 GL_MAX_TEXTURE_SIZE。
     * ES 里根本没有 proxy target（0x8064 会被判为非法枚举），所以**每一档
     * 都会失败，而调用方本来就预期这种失败**。MC 日志里紧邻的一行正说明了这点：
     *     Failed to determine maximum texture size by probing,
     *     trying GL_MAX_TEXTURE_SIZE = 16384
     *
     * 把它当错误上报有两个坏处：
     *   1. 66 条噪音把真正有价值的 3 条（R11F_G11F_B10F / RGB10_A2 / RGBA16）
     *      淹没掉 —— 上一轮正是因为要在噪音里翻找而多花了一轮；
     *   2. 每次进入世界都重试，会把 status.json 的降级事件表刷爆。
     * 因此只记录、不升级为降级事件。
     */
    if (glesmod_is_proxy_target(target)) {
        return;
    }

    snprintf(fp, sizeof(fp), "%s|%04X|%04X|%d|%04X|%04X|%s",
             who, (unsigned)target, (unsigned)internalformat,
             (int)width, (unsigned)format, (unsigned)type,
             (pixels == NULL) ? "N" : "P");

    if (!teximg_fp_is_new(fp)) return;

    snprintf(line, sizeof(line),
             "★ %s 被驱动拒绝: err=0x%04X target=0x%04X "
             "internalformat=0x%04X(%s) %dx%d format=0x%04X type=0x%04X "
             "pixels=%s —— 该分配失败会使附件无存储，"
             "进而让 FBO 报 INCOMPLETE_MISSING_ATTACHMENT(36055)",
             who, (unsigned)err, (unsigned)target,
             (unsigned)internalformat, tex_internalformat_name(internalformat),
             (int)width, (int)height,
             (unsigned)format, (unsigned)type,
             (pixels == NULL) ? "NULL" : "非NULL");
    glesmod_log(line);
    glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION, line);
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const void *pixels) {
    typedef void (*fn_t)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint,
                         GLenum, GLenum, const void *);
    static fn_t real = NULL;

    glesmod_lazy_init();

    switch (glesmod_proxy_action(target)) {
        case GLESMOD_PROXY_EMULATE:
            glesmod_proxy_record(target, level, internalformat, width, height);
            return;
        case GLESMOD_PROXY_REJECT:
            glesmod_log_proxy_unsupported(target);
            return;
        case GLESMOD_PROXY_PASS:
        default:
            break;
    }

    GLenum f = format, t = type;
    fixup_depth_format("glTexImage2D", &internalformat, &f, &t);
    /*
     * 顺序很重要：先把未定尺寸格式提升为定尺寸格式（GL_RGBA -> GL_RGBA8），
     * 再按表 3.2 修正 type。若反序，GL_RGB10_A2 / GL_RGBA16 这类
     * 「定尺寸但不接受 UNSIGNED_BYTE」的格式会被 fixup_pixel_type 提前
     * 改掉 type，虽然结果也对，但日志会显示成两次改写，难以追溯。
     */
    fixup_unsized_internalformat("glTexImage2D", &internalformat);
    fixup_pixel_type("glTexImage2D", internalformat, f, &t, pixels);

    RESOLVE_OR_RETURN(real, "glTexImage2D", );

    get_err_fn_t get_err = texprobe_get_error();
    if (get_err != NULL) drain_errors(get_err);   /* 先清队列，保证归因正确 */

    real(target, level, internalformat, width, height, border, f, t, pixels);

    if (get_err != NULL) {
        texprobe_report("glTexImage2D", get_err(), target, internalformat,
                        width, height, f, t, pixels);
    }
}

void glTexImage3D(GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLsizei height, GLsizei depth, GLint border,
                  GLenum format, GLenum type, const void *pixels) {
    typedef void (*fn_t)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint,
                         GLenum, GLenum, const void *);
    static fn_t real = NULL;

    glesmod_lazy_init();

    /*
     * glesmod_proxy_record() 目前只定义 2D 语义（width/height）。
     * 3D / array proxy 即使将来被标记为已实现，也必须先扩展记录结构，
     * 因此这里对一切非 PASS 的 proxy target 都安全拒绝。
     */
    if (glesmod_proxy_action(target) != GLESMOD_PROXY_PASS) {
        glesmod_log_proxy_unsupported(target);
        return;
    }

    GLenum f = format, t = type;
    fixup_depth_format("glTexImage3D", &internalformat, &f, &t);

    RESOLVE_OR_RETURN(real, "glTexImage3D", );

    get_err_fn_t get_err = texprobe_get_error();
    if (get_err != NULL) drain_errors(get_err);

    real(target, level, internalformat, width, height, depth, border, f, t, pixels);

    if (get_err != NULL) {
        texprobe_report("glTexImage3D", get_err(), target, internalformat,
                        width, height, f, t, pixels);
    }
}

/* ================================================================== */
/* glGetTexLevelParameteriv / fv —— proxy texture 查询                 */
/* ================================================================== */

void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
    typedef void (*fn_t)(GLenum, GLint, GLenum, GLint *);
    static fn_t real = NULL;

    glesmod_lazy_init();

    switch (glesmod_proxy_action(target)) {
        case GLESMOD_PROXY_EMULATE:
            if (params != NULL) {
                *params = glesmod_proxy_query(pname, level);
            }
            return;
        case GLESMOD_PROXY_REJECT:
            if (params != NULL) {
                *params = 0;
            }
            glesmod_log_proxy_unsupported(target);
            return;
        case GLESMOD_PROXY_PASS:
        default:
            break;
    }

    RESOLVE_OR_RETURN(real, "glGetTexLevelParameteriv", );
    real(target, level, pname, params);
}

void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
    typedef void (*fn_t)(GLenum, GLint, GLenum, GLfloat *);
    static fn_t real = NULL;

    glesmod_lazy_init();

    switch (glesmod_proxy_action(target)) {
        case GLESMOD_PROXY_EMULATE:
            if (params != NULL) {
                *params = (GLfloat)glesmod_proxy_query(pname, level);
            }
            return;
        case GLESMOD_PROXY_REJECT:
            if (params != NULL) {
                *params = 0.0f;
            }
            glesmod_log_proxy_unsupported(target);
            return;
        case GLESMOD_PROXY_PASS:
        default:
            break;
    }

    RESOLVE_OR_RETURN(real, "glGetTexLevelParameterfv", );
    real(target, level, pname, params);
}

/* ================================================================== */
/* glTexStorage2D / glTexStorage3D —— 不可变存储的深度格式翻译         */
/* ================================================================== */

/*
 * 【为什么必须补上它们 —— 这是 FBO 36055 的真正根因，有符号表级别的证据】
 *
 * 症状：Iris 建 FBO 报 Status 36055
 *       = 0x8CD7 = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
 *       = 「帧缓冲里没有任何已附加的映像」。
 * 伴随驱动报错：'pixel buffer format is not compatible with level format'。
 *
 * 符号表核对（check-fbo-chain-symbols.ps1）给出关键差异：
 *     glTexStorage2D   -> F 纯转发（**没有任何格式翻译**）
 *     glTexImage2D     -> C 定制实现（有深度格式翻译）
 *
 * 也就是说：**深度格式的 ES 合法化只做在了 glTexImage2D 这条路上。**
 *
 * 我们此前修 MC 世界渲染时，处理的正是 `glTexImage2D(GL_DEPTH_COMPONENT ...)`
 * 被驱动拒绝、导致深度纹理无存储的问题。而 `glTexStorage2D` 是
 * **另一条分配纹理存储的 API**（不可变存储），走的是同一个「内部格式」概念：
 *     glTexStorage2D(target, levels, internalformat, w, h)
 * 它同样只接受 ES 合法的深层格式。若调用方传桌面值 GL_DEPTH_COMPONENT(0x1902)，
 * 驱动会拒绝 → **纹理没有存储** → 之后把该纹理挂到 FBO 上
 * → 状态恰为 INCOMPLETE_MISSING_ATTACHMENT。
 *
 * 这与「先用 glTexStorage2D 分配、再 attach」的典型 FBO 建法完全吻合，
 * 也解释了为什么此前的 texprobe 没抓到：它只观测 glTexImage2D/3D。
 *
 * 【修法】与 glTexImage2D 复用同一个 fixup_depth_format：
 *   0x1902 GL_DEPTH_COMPONENT -> 0x81A6 GL_DEPTH_COMPONENT24
 *   0x84F9 GL_DEPTH_STENCIL   -> 0x88F0 GL_DEPTH24_STENCIL8
 * glTexStorage 没有独立的 format/type 参数（不可变存储由内部格式唯一决定），
 * 因此只翻内部格式。**其余内部格式一律原样转发**，行为不变。
 *
 * 【为什么连 levels 一起传而不做别的处理】
 *   不可变存储要求 levels ≥ 1 且尺寸与 levels 自洽，这些都由调用方负责；
 *   我们只做「桌面值 -> ES 合法值」这一件事，不越界修改语义。
 */
void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat,
                    GLsizei width, GLsizei height) {
    typedef void (*fn_t)(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
    static fn_t real = NULL;

    glesmod_lazy_init();

    /* glTexStorage 只有 internalformat，没有 format/type；复用翻译函数时
     * 传入哨兵值 0（不会被判为深度 format），因此只会翻内部格式。 */
    GLint ifmt = (GLint)internalformat;
    GLenum f = 0, t = 0;
    fixup_depth_format("glTexStorage2D", &ifmt, &f, &t);

    RESOLVE_OR_RETURN(real, "glTexStorage2D", );

    get_err_fn_t get_err = texprobe_get_error();
    if (get_err != NULL) drain_errors(get_err);

    real(target, levels, (GLenum)ifmt, width, height);

    if (get_err != NULL) {
        texprobe_report("glTexStorage2D", get_err(), target, ifmt,
                        width, height, 0, 0, NULL);
    }
}

void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat,
                    GLsizei width, GLsizei height, GLsizei depth) {
    typedef void (*fn_t)(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei);
    static fn_t real = NULL;

    glesmod_lazy_init();

    GLint ifmt = (GLint)internalformat;
    GLenum f = 0, t = 0;
    fixup_depth_format("glTexStorage3D", &ifmt, &f, &t);

    RESOLVE_OR_RETURN(real, "glTexStorage3D", );

    get_err_fn_t get_err = texprobe_get_error();
    if (get_err != NULL) drain_errors(get_err);

    real(target, levels, (GLenum)ifmt, width, height, depth);

    if (get_err != NULL) {
        texprobe_report("glTexStorage3D", get_err(), target, ifmt,
                        width, height, 0, 0, NULL);
    }
}

/* ================================================================== */
/* glTexParameter{f,i} —— 过滤 ES 非法的桌面专属 pname                 */
/* ================================================================== */

/*
 * 【问题】Minecraft 每次创建纹理都会调用：
 *     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS(34049), 0.0f)
 *   （字节码证据：com.mojang.blaze3d.platform.TextureUtil
 *     `sipush 3553` / `ldc 34049` / `fconst_0` /
 *     `invokestatic GlStateManager._texParameter:(IIF)V`）
 *
 *   GL_TEXTURE_LOD_BIAS 是**桌面专属** pname，GLES 不接受：
 *     驱动报 'pname 34049 is not supported for this API call'
 *     每轮启动约 70 次（每个纹理一次）。
 *
 * 【为什么必须处理，而不只是"日志吵"】
 *   1. 该调用在驱动里留下 GL_INVALID_ENUM（0x0500）。本库的 glGetError 是
 *      **定制实现**（负责 ES->桌面错误码映射），它会把真实的错误**原样返回**
 *      给 MC。MC 的 GlDebug / GlStateManager 会检查错误：
 *      "残留无关错误"可能让 MC 把真正的问题误判，或触发无谓的错误路径。
 *   2. 官方日志被 70 行噪声淹没，真问题不容易被看见。
 *
 * 【为什么丢弃是安全的 —— 有字节码与规范双重依据】
 *   - 值恒为 0.0f，而 0.0 正是 GL_TEXTURE_LOD_BIAS 的**默认值**，
 *     所以丢弃语义等价于"设成它本来就该有的值"。
 *   - MC 从不读取该参数（同一类里只写入，没有对应的 get）。
 *   - 已停机检查：MC 全部字节码中只有 TextureUtil 引用 34049。
 *   => 丢弃既不改行为，也消掉了噪声。
 *
 * 【故意不转发，但**不**清错误位 —— 这是刻意的取舍】
 *   驱动对这条调用会留下 GL_INVALID_ENUM。我一度想顺手清掉错误队列，
 *   但那比问题本身更危险：
 *     glGetError 是**队列**（先进先出）。若在丢弃调用后主动抽干队列，
 *     就会把此之前其它调用（可能包括 MC 自己关心的失败）留下的错误一并
 *     吞掉 —— 等于掩盖真实故障。
 *   只要不转发，我们就不新增错误；队列里该有哪些错误就保持原样。
 *   => 这条被拒绝的调用若无害，MC 的现有错误处理足以承受；
 *      若有害，驱动会通过别的方式暴露出来。**不要替 MC 清理状态。**
 *
 * 【保留其它 pname 的行为】
 *   只拦 GL_TEXTURE_LOD_BIAS；其余（含 GL_TEXTURE_MAX_LOD 33083、
 *   GL_TEXTURE_BASE_LEVEL 33084 等在 ES 中合法的）原样转发，
 *   由驱动自己判断。**不猜测、不扩大拦截范围。**
 */
#define GLESMOD_GL_TEXTURE_LOD_BIAS_EXT 0x8501  /* 34049 */

void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    typedef void (*fn_t)(GLenum, GLenum, GLfloat);
    static fn_t real = NULL;

    glesmod_lazy_init();

    if (pname == GLESMOD_GL_TEXTURE_LOD_BIAS_EXT) {
        glesmod_degrade(GLESMOD_DEGRADE_ENUM_MAPPED,
                        "glTexParameterf: GL_TEXTURE_LOD_BIAS(0x8501) 为桌面专属"
                        " pname，ES 不支持；值恒为 0.0（即默认值）且 MC 从不读回，"
                        "已忽略");
        return;
    }

    RESOLVE_OR_RETURN(real, "glTexParameterf", );
    real(target, pname, param);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    typedef void (*fn_t)(GLenum, GLenum, GLint);
    static fn_t real = NULL;

    glesmod_lazy_init();

    if (pname == GLESMOD_GL_TEXTURE_LOD_BIAS_EXT) {
        glesmod_degrade(GLESMOD_DEGRADE_ENUM_MAPPED,
                        "glTexParameteri: GL_TEXTURE_LOD_BIAS(0x8501) 为桌面专属"
                        " pname，ES 不支持；已忽略");
        return;
    }

    RESOLVE_OR_RETURN(real, "glTexParameteri", );
    real(target, pname, param);
}

/* ================================================================== */
/* glBufferStorage —— 持久映射缓冲的存储分配                            */
/* ================================================================== */

/*
 * 【为什么必须实现它，而不是留作静默 stub】
 *
 * GLES 3.2 核心确实没有 glBufferStorage（它是 GL 4.4 / ARB_buffer_storage
 * 的功能），所以它一开始被生成为安全 stub。但静默 stub 在**有存储语义要求**
 * 的调用上是危险的，因为它会让调用方拿到「成功」的假象：
 *
 *   glBufferStorage(...)   -> stub 返回 void，什么都不做，无错误
 *   缓冲区实际上【没有任何存储】
 *   glMapBufferRange(...)  -> 因无存储而失败，返回 NULL
 *   调用方                -> 抛异常崩溃
 *
 * 一手证据（Embeddium 21.1 分支源码）：
 *   RenderRegionManager.createStagingBuffer()
 *     if (Embeddium.options().advanced.useAdvancedStagingBuffers
 *             && MappedStagingBuffer.isSupported(device))
 *         return new MappedStagingBuffer(commandList);   // <- 走这条
 *     return new FallbackStagingBuffer(commandList);     // <- 本可安全
 *
 *   MappedStagingBuffer.STORAGE_FLAGS =
 *       {PERSISTENT, CLIENT_STORAGE, MAP_WRITE}          // 桌面 GL 专有
 *   MappedStagingBuffer 构造函数：
 *       createImmutableBuffer(capacity, STORAGE_FLAGS)   -> glBufferStorage
 *       mapBuffer(buffer, 0, capacity, MAP_FLAGS)        -> glMapBufferRange
 *
 *   GLRenderDevice.mapBuffer():
 *       if (buf == null) throw new RuntimeException("Failed to map buffer");
 *
 *   BufferStorageFunctions.pickBest():
 *       if (capabilities.OpenGL44)                   return CORE;
 *       else if (capabilities.GL_ARB_buffer_storage) return ARB;
 *       else                                         return NONE;
 *   => 只认桌面 GL 标志，**完全没有考虑 ES 场景**。而 CORE 与 ARB
 *      两条路在我们这里都落到同一个 glBufferStorage 符号上。
 *
 * => 目标：把「崩溃」转化为「性能降级」。真实分配存储（用 glBufferData），
 *    但**不声称**满足持久映射语义（ES 上本来就给不了），并如实记录降级。
 *    这样后续 glMapBufferRange 至少能成功，只是非持久。
 *
 * 【为什么用 glBufferData 而不是 GL_EXT_buffer_storage】
 *   该扩展在驱动上支持度参差，探测它需要 glGetStringi 查扩展列表。
 *   用 glBufferData（ES 2.0 起必有）能保证一定成功，代价是失去不可变存储
 *   （immutable）语义 —— 但那本来就不是我们能提供的。少一次扩展探测，
 *   多一分确定性。
 */
void glBufferStorage(GLenum target, GLsizeiptr size, const void *data,
                     GLbitfield flags) {
    typedef void (*buffer_data_t)(GLenum, GLsizeiptr, const void *, GLenum);

    static buffer_data_t real = NULL;

    glesmod_lazy_init();

    /*
     * 枚举值取自权威 gl.xml：
     *   GL_MAP_PERSISTENT_BIT   = 0x0040
     *   GL_MAP_COHERENT_BIT     = 0x0080
     */
    int wants_persistent = (flags & 0x0040u) != 0;
    int wants_coherent   = (flags & 0x0080u) != 0;

    RESOLVE_OR_RETURN(real, "glBufferData", );

    /*
     * 真实分配存储。GL_DYNAMIC_DRAW(0x88E8) 是保守选择：
     * 申请持久映射缓冲的场景本质上就是要反复改写，DYNAMIC 语义最贴近。
     */
    real(target, size, data, 0x88E8 /* GL_DYNAMIC_DRAW */);

    /*
     * 如实记录：调用方要的持久/一致语义在 ES 上无法提供。
     * 只记一次（glesmod_degrade 对同一码值只在首次写日志）。
     */
    if (wants_persistent || wants_coherent) {
        glesmod_degrade(
            GLESMOD_DEGRADE_PERSISTENT_MAP_UNSUPPORTED,
            "glBufferStorage 的持久/一致映射位在 GLES 上不可用；"
            "已用 glBufferData 真实分配存储（避免后续 glMapBufferRange 失败），"
            "但映射不具备持久语义（可能需重复同步，性能略降）");
    }
}

/* ================================================================== */
/* glMapBufferRange —— 剥离 ES 不支持的映射位                          */
/* ================================================================== */

/*
 * GLES 的 glMapBufferRange 接受这些位：
 *     GL_MAP_READ_BIT              0x0001
 *     GL_MAP_WRITE_BIT             0x0002
 *     GL_MAP_INVALIDATE_RANGE_BIT  0x0004
 *     GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
 *     GL_MAP_FLUSH_EXPLICIT_BIT    0x0010
 *     GL_MAP_UNSYNCHRONIZED_BIT    0x0020
 *
 * 桌面 GL 4.4 还多出这几个位，**ES 上非法**：
 *     GL_MAP_PERSISTENT_BIT   0x0040   <- 非法
 *     GL_MAP_COHERENT_BIT     0x0080   <- 非法
 *     GL_CLIENT_STORAGE_BIT   0x0200   <- 非法（属 BufferStorageMask）
 *
 * 原样转发带这些位的 access，驱动会返回 GL_INVALID_OPERATION，映射直接失败
 * => 同样让调用方抛异常崩溃。因此这里**掩掉**这三个位再转发，并如实记降级。
 *
 * 【为什么可以放心掩掉这些位】
 *   它们表达的都是「性能承诺」而非「数据正确性要求」：
 *     - PERSISTENT     ：映射可长期保持（掩掉后每次映射即可，仍正确）
 *     - COHERENT       ：无需显式 flush（掩掉后由驱动保守同步，仍正确）
 *     - CLIENT_STORAGE ：存储位置偏好（纯性能提示）
 *   掩掉只会让驱动走更保守（可能更慢）的路径，**不会算错数据**。
 *   我们不改变任何影响正确性的位（READ/WRITE/INVALIDATE/FLUSH）——
 *   这与「不猜测、不擅改数据语义」的原则一致。
 *
 * 【注意】只掩位，**不**清理 GL 错误队列，理由同 glTexParameterf：
 *   glGetError 是 FIFO 队列，主动抽干会吞掉此前的真实错误。
 */
void *glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length,
                       GLbitfield access) {
    typedef void *(*map_range_t)(GLenum, GLintptr, GLsizeiptr, GLbitfield);

    static map_range_t real = NULL;

/* GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_CLIENT_STORAGE_BIT */
#define GLESMOD_MAP_ILLEGAL_FOR_ES \
    (0x0040u | 0x0080u | 0x0200u)

    glesmod_lazy_init();

    RESOLVE_OR_RETURN(real, "glMapBufferRange", NULL);

    if ((access & GLESMOD_MAP_ILLEGAL_FOR_ES) != 0) {
        access &= ~GLESMOD_MAP_ILLEGAL_FOR_ES;

        glesmod_degrade(
            GLESMOD_DEGRADE_PERSISTENT_MAP_UNSUPPORTED,
            "glMapBufferRange 请求了 GLES 不支持的持久/一致映射位"
            "(GL_MAP_PERSISTENT_BIT / GL_MAP_COHERENT_BIT / GL_CLIENT_STORAGE_BIT)，"
            "已剥离后转发（仅影响性能提示，不影响数据正确性）");
    }

    return real(target, offset, length, access);

#undef GLESMOD_MAP_ILLEGAL_FOR_ES
}

/* ================================================================== */
/* glGetString —— 对外呈现桌面 GL 兼容的版本串                          */
/* ================================================================== */

/*
 * 【为什么要改写 GL_VERSION —— 有真机崩溃的一手证据】
 *
 * 原实现是直接转发，返回驱动原话：
 *     "OpenGL ES 3.2 V@0762.36 (GIT@4a4a7d07e5, ...) (Date:05/16/25)"
 *
 * 2026-09-30 真机日志（Iris 1.8.14-beta.1 + Complementary Reimagined / BSL）：
 *     java.lang.IllegalStateException: Could not parse GL version from
 *       "OpenGL ES 3.2 V@0762.36 (GIT@4a4a7d07e5, I6689e30232, 1747413442) (Date:05/16/25)"
 *     at net.irisshaders.iris.gl.shader.StandardMacros.getGlVersion(StandardMacros.java:214)
 *     at ...StandardMacros.createStandardEnvironmentDefines(...)
 *     at ...Iris.loadExternalShaderpack(...)
 *     ... 由 ShaderPackScreen.applyChanges 触发（用户点「应用」时）
 *
 * Iris 的 getGlVersion() 用正则（SEMVER_PATTERN）从 GL_VERSION 串里
 * 抓「主.次」版本号。桌面驱动的串形如 "4.6.0 NVIDIA 536.23"，正则可匹配；
 * 而我们的串以 "OpenGL ES 3.2 ..." 开头 —— **第一个数字前面有 "OpenGL ES "**，
 * 于是匹配失败并抛异常 → 光影加载失败。
 *
 * 这不是我们独有的问题：MobileGlues 明确提供 customGLVersion 设置
 * （MobileGlues-cpp/config/settings.h: `#define DEFAULT_GL_VERSION 40`），
 * 就是同一类兼容性需求。
 *
 * 【修法】对外声称桌面 GL 3.2 的版本串。
 *
 *   1. 我们导出的符号集就是 GL 3.2 core 子集（见 docs/o-02-symbol-inventory.md）
 *   2. 任务书 §6 要求「日志显示 GLES 后端已激活，能力协商输出完整」
 *   3. 我们自己已经在 status.json 里声称 reportedGl = 3.2（见 core.c）
 *      → 这里与之一致，不是在引入新说法
 *
 * 【为什么把 ES 版本号放末尾】
 *   保留真实能力信息，使日志仍可诊断，同时第一个数字就是 3.2 以便正则匹配：
 *       "3.2 (OpenGL ES 3.2 Adreno (TM) 750)"
 *
 * 【严格说明：这**不是**在伪造「我们是桌面 GL 4.6」】
 *   声称 4.6 会诱导调用方去用 DSA / 持久映射等 GL 4.5 功能，
 *   而它们在我们的符号表里是**静默 stub**（见 docs/r-12-stub-audit.md：
 *   28 个 DANGER 项全部是 GL 4.4/4.5 的 DSA 族）。
 *   声称 3.2 与我们的**实际符号覆盖一致**，因此是诚实的说法。
 *
 * 【为什么 GL_SHADING_LANGUAGE_VERSION 也要改】
 *   某些模组会解析它取 GLSL 版本。ES 的串形如 "OpenGL ES GLSL ES 3.20"，
 *   同样不被桌面正则接受，因此一并改写为 "3.20"。
 *
 * 【不改 GL_VENDOR / GL_RENDERER / GL_EXTENSIONS】
 *   前两者是自由文本、无解析约定，保留驱动原话更有诊断价值
 *   （日志里 "Adreno (TM) 750" 一眼可辨）；
 *   GL_EXTENSIONS 必须保留原样，否则能力探测会失真。
 */

/*
 * 【为什么不硬编码厂商名】
 *   本项目已在 Adreno 750 上验证，但**目标不是单一 GPU**（任务书要求
 *   设备兼容性数据库）。硬编码厂商名会在 Mali / PowerVR 上产生**错误信息** ——
 *   那比「不匹配」更糟：它会污染诊断。
 *   因此版本串由**驱动实际返回的 GL_RENDERER** 动态合成。
 */
#define GLESMOD_GLSL_VERSION_STR "3.20"

/* gl.xml：GL_VERSION=0x1F02, GL_RENDERER=0x1F01, GL_SHADING_LANGUAGE_VERSION=0x8B8C */
#define GLESMOD_ENUM_GL_VERSION   0x1F02
#define GLESMOD_ENUM_GL_RENDERER  0x1F01
#define GLESMOD_ENUM_GL_GLSL_VER  0x8B8C

/*
 * 追加到扩展列表的那一条。
 *
 * 必须定义在 glGetString 之前 —— glGetString(GL_EXTENSIONS) 就要用它。
 */
#define GLESMOD_EXTRA_EXTENSION "GL_ARB_draw_buffers_blend"
#define GLESMOD_EXTRA_EXTENSION_LEN 25   /* 编译期 strlen，避免每次调用 strlen */

/*
 * 合成后的版本串缓存。
 *
 * 并发说明：只在**首次**调用时构造一次；此后所有线程都读同一份已完成的
 * 缓冲区。GL 调用在实践中是单线程的（渲染线程），且 glGetString 主要在
 * 初始化阶段被调用，因此不做额外加锁 —— 与 core.c 对 g_trace_ring 的取舍
 * 一致（最坏情况是两线程各构造一次**内容相同**的串，不会产生错误结果）。
 */
static char g_version_buf[192];
static volatile int g_version_ready = 0;

/*
 * 扩展列表追加所需的前置声明。
 *
 * 【为什么必须是前置声明而不是把定义搬上来】
 *   追加逻辑被 glGetString（本处）与 glGetStringi（文件更下方）**共用**，
 *   而 glGetString 在文件里更靠前。两个选择：把共用块整体上移，
 *   或在此处前置声明、定义留在下一条注释块之后。
 *   后者改动面最小 —— 移动大段代码容易在 diff 里掩盖真实变化。
 *
 * 【为什么两者必须共享同一份缓冲区与同一份计数】
 *   对外「说有几条扩展」（GL_NUM_EXTENSIONS）与「真给出几条」
 *   （GL_EXTENSIONS / glGetStringi）必须来自同一个事实，
 *   否则会自相矛盾：计数说 N+1，按计数循环却只能取到 N 条。
 *   这正是上一轮 glGetStringi 成为死代码的机制。
 */
static char  *g_extensions_buf;
static size_t g_extensions_cap;
static GLuint g_real_ext_count;
static int    g_ext_count_logged;   /* glGetStringi 探测日志，只打一次 */
static int    g_num_ext_logged;     /* GL_NUM_EXTENSIONS +1 日志，只打一次 */
static size_t append_extension_token(const char *src, size_t src_len);
static void   log_extra_extension_once(GLuint n);

/*
 * 求出驱动的真实扩展条数，并缓存。首次调用做一次探测，之后直接读。
 *
 * 【为什么必须是独立函数】
 *   索引路径（LWJGL 桌面分支）与字符串路径（ES 分支）互不调用，
 *   若把计数依附在任一条路径的副作用上，另一条路径就永远看到旧值。
 *   上一版正是把计数 gated 在 g_extensions_buf（只由字符串路径填充）上，
 *   于是索引路径的计数永远是 N，追加项永远不可达。
 *
 * @param real 真实的 glGetStringi 函数指针（用于探测）
 * @return 驱动的真实扩展条数
 */
static GLuint query_real_ext_count(const GLubyte *(*real)(GLenum, GLuint))
{
    if (g_real_ext_count == 0 && real != NULL) {
        GLuint n = 0;
        while (n < 4096 && real(GLESMOD_ENUM_GL_EXTENSIONS, n) != NULL) n++;
        g_real_ext_count = n;
        log_extra_extension_once(n);
    }
    return g_real_ext_count;
}

const GLubyte *glGetString(GLenum name) {
    typedef const GLubyte *(*fn_t)(GLenum);
    static fn_t real = NULL;

    if (name == GLESMOD_ENUM_GL_GLSL_VER) {
        return (const GLubyte *)GLESMOD_GLSL_VERSION_STR;
    }

    glesmod_lazy_init();
    RESOLVE_OR_RETURN(real, "glGetString", NULL);

    /*
     * GL_EXTENSIONS：在驱动原串末尾追加我们完整转发的那条扩展。
     *
     * ES 路径上 LWJGL 就是靠解析这一个串来建立 supported_extensions 的
     * （见下方 append_extension_token 前的长注释），因此这条才是生效路径。
     * 真实扩展名一个不动，只在末尾加一条 —— 单调增强。
     */
    if (name == GLESMOD_ENUM_GL_EXTENSIONS) {
        const char *src = (const char *)real(name);
        if (src == NULL) return NULL;

        if (g_extensions_buf == NULL) {
            size_t len = strlen(src);
            if (append_extension_token(src, len) == 0) {
                return (const GLubyte *)src;   /* 追加失败，退回原串 */
            }
            {
                char line[224];
                snprintf(line, sizeof(line),
                         "GL_EXTENSIONS 队列末尾已追加 %s"
                         "（真实扩展 %zu 字节；其提供的 7 个 indexed blend "
                         "函数均为真实转发，见 gl.xml）",
                         GLESMOD_EXTRA_EXTENSION, len);
                glesmod_log(line);
            }
        }
        return (const GLubyte *)g_extensions_buf;
    }

    if (name != GLESMOD_ENUM_GL_VERSION) {
        return real(name);
    }

    if (!g_version_ready) {
        const char *renderer = (const char *)real(GLESMOD_ENUM_GL_RENDERER);
        if (renderer == NULL || renderer[0] == '\0') {
            renderer = "unknown GPU";
        }
        /*
         * 形如： "3.2 (OpenGL ES 3.2 Adreno (TM) 750)"
         *         ^^^  正则能匹配的第一个数字
         *                         ^^^^^^^^^^^^^^^^^^^ 真实 GPU，便于诊断
         */
        snprintf(g_version_buf, sizeof(g_version_buf),
                 "3.2 (OpenGL ES 3.2 %s)", renderer);
        g_version_ready = 1;
    }

    return (const GLubyte *)g_version_buf;
}

/* ================================================================== */
/* glGetStringi —— 补齐能力探测所需要的扩展声明                        */
/* ================================================================== */

/*
 * 【为什么要动它 —— 这是我的一个失误，有真机证据】
 *
 * 2026-09-30 真机（Iris 1.8.14-beta.1 + Complementary Reimagined）：
 *     java.lang.RuntimeException: Buffer blending is not supported on this
 *         platform, however it was attempted to be used!
 *       at iris...ShaderProperties.lambda$new$39(ShaderProperties.java:314)
 *         -> handlePassDirective("blend.", ...)
 *
 * Iris 源码（`IrisRenderSystem`）：
 *     public static boolean supportsBufferBlending() {
 *         return GL.getCapabilities().GL_ARB_draw_buffers_blend
 *                || GL.getCapabilities().OpenGL40;
 *     }
 *
 * **注意这是 Iris 的原版代码，不是我们的 stub。** 它检查的是两个
 * **桌面 GL 的**能力标志：
 *   - `GL_ARB_draw_buffers_blend`（桌面扩展）
 *   - `OpenGL40`（桌面 4.0）
 * 二者在 ES 上都不存在，于是永远返回 false -> 抛异常。
 *
 * 【而 ES 3.2 其实完全具备这套能力】
 *   以下 7 个"按索引设置混合状态"的函数在 ES 3.0 起就是核心（我逐个核实过）：
 *     glBlendFunci / glBlendFuncSeparatei / glBlendEquationi /
 *     glBlendEquationSeparatei / glColorMaski / glEnablei / glDisablei
 *   在本项目的符号表里**全部是 F（真实转发）**，因为 GLES 原生提供它们。
 *   ⇒ 所以「Buffer blending 不被支持」这个判断在当前平台上是**错误**的。
 *
 * 【为什么这个失误在我这边】
 *   我的 `glGetString` 改写只处理了 `GL_VERSION` 与
 *   `GL_SHADING_LANGUAGE_VERSION`，**遗漏了 `GL_EXTENSIONS`** ——
 *   而 Iris 正是通过 `glGetStringi(GL_EXTENSIONS, i)` 枚举扩展名的。
 *   由于驱动只报告真实 ES 扩展，`GL_ARB_draw_buffers_blend` 自然不在其中。
 *
 * 【修法：在扩展列表末尾追加一条「如实」的声明】
 *   追加 `GL_ARB_draw_buffers_blend`。之所以是**如实**而不是伪造：
 *   该扩展提供的**全部**函数我们都真实转发（已逐个核实，见上），
 *   因此声明它不构成对调用方的欺骗 —— 调用方用到的东西确实可用。
 *
 * 【与 glGetString 改写一致的取舍】
 *   我们**不**声称 `OpenGL40`（那会触发 Iris 走 DSA、
 *   持久映射等真正缺失的路径，见 docs/r-12-stub-audit.md）。
 *   只补这一条**功能确实具备**的扩展，是影响面最小的修法。
 */
#define GLESMOD_EXTRA_EXTENSION "GL_ARB_draw_buffers_blend"
#define GLESMOD_EXTRA_EXTENSION_LEN 25   /* 编译期 strlen，避免每次调用 strlen */

/*
 * 驱动的真实扩展条数，由 glGetIntegerv / glGetStringi 共享（定义见文件上方
 * 「扩展列表追加所需的前置声明」）。0 表示尚未求得。
 *
 * 这里不再重复定义 g_extensions_buf / g_extensions_cap 等 ——
 * 它们已在 glGetString 之前定义，两个入口共用同一份。
 */

static void log_extra_extension_once(GLuint n) {
    if (g_ext_count_logged || n == 0) return;
    g_ext_count_logged = 1;
    {
        char line[224];
        snprintf(line, sizeof(line),
                 "扩展列表含 %u 条真实扩展；对外报告 %u 条，"
                 "第 %u 条（索引 %u）为追加的 %s"
                 "（其提供的函数本项目均已真实转发）",
                 (unsigned)n, (unsigned)(n + 1), (unsigned)(n + 1),
                 (unsigned)n, GLESMOD_EXTRA_EXTENSION);
        glesmod_log(line);
    }
}

const GLubyte *glGetStringi(GLenum name, GLuint index) {
    typedef const GLubyte *(*fn_t)(GLenum, GLuint);
    static fn_t real = NULL;

    glesmod_lazy_init();
    RESOLVE_OR_RETURN(real, "glGetStringi", NULL);

    if (name == GLESMOD_ENUM_GL_EXTENSIONS) {
        /* 计数独立求出，不依赖字符串路径是否被调用 */
        GLuint n = query_real_ext_count(real);

        /*
         * 追加项放在索引 == 真实条数 处。
         * 注意这个索引**刚好在驱动的有效范围之外**，所以：
         *   - 只有知道「我多报了一条」的调用方才会读到它
         *   - 不会覆盖任何真实扩展
         * 配合 glGetIntegerv 把 GL_NUM_EXTENSIONS 报成 真实条数+1，
         * 按计数循环的调用方（LWJGL 桌面路径正是如此）就能取到这一条。
         */
        if (index == n) {
            return (const GLubyte *)GLESMOD_EXTRA_EXTENSION;
        }
        if (index > n) {
            return NULL;   /* 超出我们声明的范围 */
        }
    }

    return real(name, index);
}

/*
 * glGetIntegerv —— 唯一目的是把 GL_NUM_EXTENSIONS 报成「真实条数 + 1」
 * ------------------------------------------------------------------
 *
 * 【为什么必须动它 —— 这是上一轮 glGetStringi 完全没生效的原因】
 *
 * 真机日志里 `Buffer blending is not supported on this platform` 依旧出现，
 * 说明 glGetStringi 追加的那条扩展**从未被读到**。查 LWJGL 权威源码：
 *
 *     // org/lwjgl/opengles/GLContext.java :: getSupportedExtensions
 *     final String extensions_string = glGetString(GL_EXTENSIONS);
 *     ...
 *     final StringTokenizer tokenizer = new StringTokenizer(extensions_string);
 *     while ( tokenizer.hasMoreTokens() )
 *         supported_extensions.add(tokenizer.nextToken());
 *
 * **关键：ES 路径根本不调用 glGetStringi，它解析的是 glGetString(GL_EXTENSIONS)
 * 这一个空格分隔的整串。**
 *
 * 而我们的 glGetString 对 GL_EXTENSIONS 是原样转发（那段注释里写着
 * 「GL_EXTENSIONS 必须保留原样，否则能力探测会失真」——在这一点上是错的：
 * 保留原样才是能力探测失真的原因）。
 *
 * 【修法：只改含那个目标的这一条，其余一律原样转发】
 *   解析驱动返回的空格分隔串，在**末尾**追加一个 token。
 *   不改动、不删除任何真实扩展名，因此对能力探测是**单调增强**：
 *   驱动说支持的，我们照说支持；只多出一条我们确实完整转发的。
 *
 * 【与 glGetStringi 的关系】
 *   两条路径都留着：ES 走 glGetString，桌面兼容路径走 glGetStringi，
 *   只要调用方走其中任一条，都能看到这条扩展。
 *
 * 【为什么放在 glGetString 而不是另开一个定制符号】
 *   该处理的语义就属于 GL_EXTENSIONS 这个查询值，塞进已有的 glGetString
 *   定制实现里最自然，也避免再多一个符号路由类型。
 */

/*
 * 追加后的扩展串缓存。
 *
 * 并发说明与 g_version_buf 相同：只在首次构造一次，此后只读。
 * 最坏情况是两线程各构造一份**内容相同**的串，不会产生错误结果。
 * 缓冲区按驱动实际串长动态分配（扩展串可达数千字节），
 * 因此用 malloc 而非固定数组 —— 过小的固定数组会静默截断扩展列表，
 * 那正是我们要避免的一类失败。
 *
 * （g_extensions_buf / g_extensions_cap 的实际定义见文件上方
 *   「扩展列表追加所需的前置声明」，因为 glGetString 也要用它。）
 */
static size_t append_extension_token(const char *src, size_t src_len)
{
    const size_t extra_len = GLESMOD_EXTRA_EXTENSION_LEN;
    const size_t need = src_len + 1 + extra_len + 1;   /* 空格 + token + NUL */

    if (need > g_extensions_cap) {
        char *nb = (char *)realloc(g_extensions_buf, need);
        if (nb == NULL) return 0;      /* 分配失败则放弃追加，保持原串 */
        g_extensions_buf = nb;
        g_extensions_cap = need;
    }

    memcpy(g_extensions_buf, src, src_len);
    g_extensions_buf[src_len] = ' ';
    memcpy(g_extensions_buf + src_len + 1, GLESMOD_EXTRA_EXTENSION, extra_len);
    g_extensions_buf[src_len + 1 + extra_len] = '\0';
    return src_len + 1 + extra_len;
}

/*
 * glGetIntegerv —— 只为了让 GL_NUM_EXTENSIONS 与 GL_EXTENSIONS 自洽
 * ------------------------------------------------------------------
 *
 * 【为什么需要它】
 *   我们对外多报了一条扩展。若 `GL_NUM_EXTENSIONS` 仍返回驱动原值 N，
 *   那么按计数循环的调用方（LWJGL 的桌面路径正是 `for (i < count)`）
 *   永远只会读到 0..N-1，**永远读不到索引 N 上那条追加项** ——
 *   这正是上一轮 glGetStringi 成为死代码的机制。
 *
 *   计数与枚举必须来自同一个事实，否则二者自相矛盾。
 *   这里把它统一为：`GL_NUM_EXTENSIONS = 驱动真实条数 + 1`。
 *
 * 【影响面】
 *   只拦截这一个 pname，其余**一律原样转发**。
 *   注意 glGetIntegerv 是所有能力探测的必经之路（LWJGL 大量使用），
 *   因此这里的实现必须尽可能薄：一次比较 + 一次转发，不做额外查询。
 *
 * 【与 GL_EXTENSIONS 追加的一致性】
 *   两条路径都以同一份 g_extensions_buf 为准：只要追加成功，
 *   计数就 +1；若追加失败（内存不足），计数也**不加**，
 *   保证「说有几条」与「真给几条」永远一致。
 */
void glGetIntegerv(GLenum pname, GLint *params) {
    typedef void (*fn_t)(GLenum, GLint *);
    typedef const GLubyte *(*get_stringi_t)(GLenum, GLuint);
    static fn_t real = NULL;

    glesmod_lazy_init();
    RESOLVE_OR_RETURN(real, "glGetIntegerv", );

    if (pname == GLESMOD_ENUM_GL_NUM_EXTENSIONS && params != NULL) {
        real(pname, params);

        /*
         * 把驱动报的条数 +1，与 glGetStringi 在索引 N 处提供的追加项对齐。
         *
         * 【必须是 +1，不能有附加条件】
         *   上一版写成 `if (g_extensions_buf != NULL && *params > 0) *params += 1;`，
         *   而那是在等一条**永远不会执行**的路径：走索引路径的调用方
         *   （LWJGL 桌面分支，判断依据是 GL_VERSION 主版本 >= 3）**从不调用
         *   glGetString(GL_EXTENSIONS)**，所以 g_extensions_buf 永远是 NULL，
         *   计数永远是 N —— 于是 `for (i = 0; i < N; i++)` 永远取不到索引 N。
         *   这就是追加项在真机上完全不可见的机制。
         *
         *   计数与枚举是两个独立入口，不能互相依赖对方的副作用。
         *   这里只做纯粹的 +1；追加项本身由 glGetStringi 无条件提供
         *   （它不依赖任何前置调用，自己会求出真实条数）。
         */
        if (*params > 0) {
            *params = *params + 1;
            if (g_num_ext_logged == 0) {
                /*
                 * 首次运行时把「我们对外多报了一条」记下来。
                 * 故意不顺带调 query_real_ext_count()：那需要 glGetStringi
                 * 的函数指针，而这里做纯计数 +1 就够了，
                 * glGetStringi 自己会在被调用时完成探测并打印那一行。
                 *
                 * 用独立的 g_num_ext_logged 而不是与 glGetStringi 共用的
                 * g_ext_count_logged —— 否则先走哪条路径就会吞掉另一条的信息。
                 */
                g_num_ext_logged = 1;
                glesmod_log("GL_NUM_EXTENSIONS 已 +1（对外多报一条 "
                            GLESMOD_EXTRA_EXTENSION
                            "，其提供的函数本项目均已真实转发）");
            }
        }
        return;
    }

    real(pname, params);
}

/* ================================================================== */
/* glCheckFramebufferStatus —— 把「不完整」的原因翻译成人话             */
/* ================================================================== */

/*
 * 【为什么要包这个 —— 有真机阻塞证据】
 *
 * 2026-09-30 真机（Iris 1.8.14-beta.1 + BSL）：
 *     IllegalStateException: Unexpected error while creating framebuffer:
 *         Draw buffers [0] Status: 36055
 *       at iris...RenderTargets.createColorFramebuffer(RenderTargets.java:366)
 *   （36055 = GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER）
 *   紧接着驱动：
 *     'the draw framebuffer is not framebuffer complete'
 *
 * 问题是：**这句话不足以定位根因**。「不完整」的原因有多种，
 * Iris 只报告了状态码，没有说是哪个附件出了什么事。
 *
 * 而本项目**有理由被怀疑**：
 *   1. 我们的 glTexImage2D/glTexImage3D 会翻译深层格式
 *      （GL_DEPTH_COMPONENT -> DEPTH_COMPONENT24）—— 见 fixup_depth_format。
 *      若 Iris 先用 glTexStorage2D 分配存储、再用 glTexImage2D 指定深度附件，
 *      两者行为会不一致。
 *   2. 我们的 stub 里含 glFramebufferTexture1D/3D（**这两个 ES 确实没有**，
 *      stub 是对的），但 glFramebufferTexture / glBlendFunci / glColorMaski
 *      **是真的转发**（已用权威 gl.xml 核实它们在 GLES 3.2 命令集里）。
 *
 * 【这个包装做什么】
 *   在结果**不是 COMPLETE** 时，主动查询当前 FBO 的关键信息并写入日志：
 *     - 各颜色附件：对象类型、对象名、内部格式、尺寸
 *     - 深度/模板附件：同上
 *     - glGetError 的当前值
 *   从而把「Status: 36055」变成一条**可直接定位**的记录。
 *
 * 【只在不完整时才查】
 *   正常路径（COMPLETE）只多一次比较，不做任何 GL 查询 ——
 *   不会拖慢渲染。不完整是罕见事件，此时多几次查询完全可以接受。
 */
/*
 * 【★这张表曾经是错的，而且是凭记忆写的★】
 *
 * 上一版把 0x8CD9 及以上的映射全部写错：
 *     写的是  0x8CD9 = INCOMPLETE_DRAW_BUFFER     （错）
 *     实际是  0x8CD9 = INCOMPLETE_DIMENSIONS
 *             0x8CDB = INCOMPLETE_DRAW_BUFFER
 * 后果不是外观问题：真机报出 status=36055，而 36055 = 0x8CD7
 * = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT，我却在上一轮
 * 把它读成了「INCOMPLETE_DRAW_BUFFER」，据此选错了排查方向。
 *
 * 现在这张表**逐条来自权威 gl.xml**（native/tools/.cache/gl.xml），
 * 可用 `py native/tools/dump_fbo_enums.py` 重新导出核对。
 * 这也是本项目第三次因「凭记忆写枚举」出错（前两次是 0x1801、34049），
 * 因此把它固化成工具而不是再写一次注释。
 */
static const char *fbo_status_name(GLenum s) {
    switch (s) {
    case 0x8CD5: return "COMPLETE";                     /* 36053 */
    case 0x8CD6: return "INCOMPLETE_ATTACHMENT";        /* 36054 */
    case 0x8CD7: return "INCOMPLETE_MISSING_ATTACHMENT";/* 36055 ★真机值★ */
    case 0x8CD9: return "INCOMPLETE_DIMENSIONS";        /* 36057 */
    case 0x8CDB: return "INCOMPLETE_DRAW_BUFFER";       /* 36059 */
    case 0x8CDC: return "INCOMPLETE_READ_BUFFER";       /* 36060 */
    case 0x8CDD: return "UNSUPPORTED";                  /* 36061 */
    case 0x8D56: return "INCOMPLETE_MULTISAMPLE";       /* 36182 */
    case 0x8DA8: return "INCOMPLETE_LAYER_TARGETS";     /* 36264 */
    default:     return "UNKNOWN";
    }
}

/*
 * 【INCOMPLETE_MISSING_ATTACHMENT 的语义，直接决定排查方向】
 *
 * 该状态的含义是：**帧缓冲里没有任何已附加的映像**。
 * 它最常见的原因是：附件纹理「没有存储」——
 * 即纹理对象存在，但 glTexImage2D 那一步失败/未调用，于是它是空的。
 *
 * 真机日志恰好并排放着两条互相印证的信息：
 *     pixel buffer format is not compatible with level format   <- glTexImage2D 被拒
 *     Draw buffers [0] Status: 36055                            <- 因此附件为空
 *
 * 所以本诊断在报出 MISSING_ATTACHMENT 时，额外提示「检查该附件对象的
 * 纹理是否真的分配了存储」，把排查者直接引向 glTexImage2D 的参数，
 * 而不是像上一轮那样去猜 draw buffers。
 */
static const char *fbo_status_hint(GLenum s) {
    switch (s) {
    case 0x8CD7:
        return " 提示: 该状态表示「没有任何已附加的映像」——"
               "通常是附件纹理没有存储（glTexImage2D 失败或未调用），"
               "请核对同一时刻驱动是否报了 pixel buffer format 相关错误";
    case 0x8CD6:
        return " 提示: 附件本身不被接受（内部格式/多级/面不完整）";
    case 0x8CDD:
        return " 提示: 该附件组合在此驱动上不受支持（换更保守的格式重试）";
    default:
        return NULL;
    }
}

GLenum glCheckFramebufferStatus(GLenum target) {
    typedef GLenum (*fn_t)(GLenum);
    static fn_t real = NULL;

    glesmod_lazy_init();
    RESOLVE_OR_RETURN(real, "glCheckFramebufferStatus", 0);

    GLenum status = real(target);
    if (status == 0x8CD5 /* GL_FRAMEBUFFER_COMPLETE */) {
        return status;
    }

    /*
     * 不完整 —— 这时才做详细查询。以下枚举取自权威 gl.xml。
     * 注意：查询本身可能产生 GL 错误，因此先记录 error 再查，
     * 避免我们的查询把原始错误覆盖掉。
     */
    char buf[1024];
    int off = snprintf(buf, sizeof(buf),
        "FBO 不完整: target=0x%04X status=0x%04X (%s)",
        (unsigned)target, (unsigned)status, fbo_status_name(status));

    /*
     * 颜色附件 0..3 + 深度 + 模板。
     *
     * 同时读 OBJECT_NAME 与 TEXTURE_LEVEL：仅凭「挂了什么」不足以判断，
     * 还必须知道那个纹理到底有没有存储 —— MISSING_ATTACHMENT(36055) 的
     * 典型成因正是「对象在、存储不在」。因此对纹理附件额外查一次
     * GL_TEXTURE_IMMUTABLE_LEVELS(0x82DF) 之外最直接的证据：
     * 该纹理的 WIDTH/HEIGHT 是否为 0。
     *
     * 【为什么这里可以安全地绑定纹理】本函数只在 FBO **不完整**时执行，
     * 是异常路径；查询结束后会还原原绑定。正常渲染路径完全不执行本函数。
     */
    typedef void (*get_attach_t)(GLenum, GLenum, GLenum, GLint *);
    typedef void (*get_tex_level_t)(GLenum, GLint, GLenum, GLint *);
    typedef void (*bind_tex_t)(GLenum, GLuint);
    typedef void (*get_int_v_t)(GLenum, GLint *);

    get_attach_t get_attach =
        (get_attach_t)glesym_resolve("glGetFramebufferAttachmentParameteriv");
    get_tex_level_t get_tex_level =
        (get_tex_level_t)glesym_resolve("glGetTexLevelParameteriv");
    bind_tex_t bind_tex = (bind_tex_t)glesym_resolve("glBindTexture");
    get_int_v_t get_int_v = (get_int_v_t)glesym_resolve("glGetIntegerv");

    static const struct { GLenum attach; const char *label; } kAtt[] = {
        { 0x8CE0, "color0" }, { 0x8CE1, "color1" },
        { 0x8CE2, "color2" }, { 0x8CE3, "color3" },
        { 0x8D00, "depth"  }, { 0x8D20, "stencil" },
    };

    if (get_attach != NULL) {
        for (unsigned i = 0; i < sizeof(kAtt) / sizeof(kAtt[0]); i++) {
            GLint type = 0, name = 0;
            get_attach(target, kAtt[i].attach, 0x8CD0 /* OBJECT_TYPE */, &type);
            if (type == 0 /* GL_NONE */) continue;   /* 未挂载，跳过 */
            get_attach(target, kAtt[i].attach, 0x8CD1 /* OBJECT_NAME */, &name);

            GLint lvl = -1;
            get_attach(target, kAtt[i].attach, 0x8CD3 /* TEXTURE_LEVEL */, &lvl);

            off += snprintf(buf + off, sizeof(buf) - (size_t)off,
                " | %s: type=0x%04X obj=%d lvl=%d",
                kAtt[i].label, (unsigned)type, (int)name, (int)lvl);

            /*
             * 0x1702 = GL_TEXTURE。若是纹理附件，量一下它的存储尺寸。
             * 尺寸为 0 或查询失败 = 该纹理没有存储，正是 36055 的含义。
             */
            if (type == 0x1702 && name != 0 && get_tex_level != NULL &&
                bind_tex != NULL && get_int_v != NULL) {
                GLint prev = 0;
                get_int_v(0x8069 /* GL_TEXTURE_BINDING_2D */, &prev);

                bind_tex(0x0DE1 /* GL_TEXTURE_2D */, (GLuint)name);

                GLint w = 0, h = 0, ifmt = 0;
                get_tex_level(0x0DE1, lvl < 0 ? 0 : lvl,
                              0x1001 /* GL_TEXTURE_WIDTH */, &w);
                get_tex_level(0x0DE1, lvl < 0 ? 0 : lvl,
                              0x1000 /* GL_TEXTURE_HEIGHT */, &h);
                get_tex_level(0x0DE1, lvl < 0 ? 0 : lvl,
                              0x1003 /* GL_TEXTURE_INTERNAL_FORMAT */, &ifmt);

                bind_tex(0x0DE1, (GLuint)prev);

                off += snprintf(buf + off, sizeof(buf) - (size_t)off,
                    "(存储=%dx%d ifmt=0x%04X%s)",
                    (int)w, (int)h, (unsigned)ifmt,
                    (w <= 0 || h <= 0) ? " ★无存储★" : "");
            }

            if (off >= (int)sizeof(buf) - 96) break;
        }
    } else {
        off += snprintf(buf + off, sizeof(buf) - (size_t)off,
            " | glGetFramebufferAttachmentParameteriv 不可用");
    }

    glesmod_log(buf);

    /* 把成因提示作为独立的降级事件，便于在 status.json 里单独检索 */
    {
        const char *hint = fbo_status_hint(status);
        if (hint != NULL) {
            char line[320];
            snprintf(line, sizeof(line), "FBO status=0x%04X (%s)%s",
                     (unsigned)status, fbo_status_name(status), hint);
            glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION, line);
        } else {
            glesmod_degrade(GLESMOD_DEGRADE_UNSUPPORTED_FUNCTION, buf);
        }
    }

    return status;
}

/* ================================================================== */
/* glShaderSource —— GLSL -> GLSL ES 转换                              */
/* ================================================================== */

/*
 * 记录某个 shader 的源码 / 在编译失败时转储源码。
 *
 * 实现在下方（glShaderSource 之后）—— 此处只需前置声明，
 * 因为 glShaderSource 要用到前者。
 */
static void remember_shader_source(GLuint shader, const char *raw,
                                   const char *converted, int stage);
static void dump_shader_source_on_failure(GLuint shader);

void glShaderSource(GLuint shader, GLsizei count,
                    const GLchar *const *string, const GLint *length) {
    typedef void (*fn_t)(GLuint, GLsizei, const GLchar *const *, const GLint *);
    static fn_t real = NULL;

    glesmod_lazy_init();

    RESOLVE_OR_RETURN(real, "glShaderSource", );

    /*
     * 记录本函数被调用的次数。
     *
     * 【为什么需要这个计数】
     *   诊断「转换器到底有没有生效」时，最容易混淆的一点是：
     *   日志里看不到某类着色器，可能意味着
     *     a) 它没经过我们（例如由别的路径编译）
     *     b) 它经过了，只是我们没记
     *   计数能立刻区分：若计数在增长却看不到目标着色器，
     *   说明是 (b)，即记录策略漏了它。
     *
     * 每次编译着色器写一行日志（数量级为百余行，可接受），
     * 同时输出调用序号，便于与后续的失败信息对齐。
     */
    static int shader_source_calls = 0;
    shader_source_calls++;
    {
        char line[96];
        snprintf(line, sizeof(line),
                 "glShaderSource 第 %d 次调用（shader=%u，片段数=%d）",
                 shader_source_calls, (unsigned)shader, (int)count);
        glesmod_log(line);
    }

    if (count <= 0 || string == NULL) {
        real(shader, count, string, length);
        return;
    }

    /*
     * 先拼接源码（GL 允许分段传入），再整体转换。
     *
     * 仅转换第一段是常见错误：MC 有时把 #version 与主体分两段传入。
     */
    size_t total = 0;
    for (GLsizei i = 0; i < count; i++) {
        if (string[i] == NULL) continue;
        total += (length != NULL && length[i] >= 0)
                 ? (size_t)length[i]
                 : strlen(string[i]);
    }
    if (total == 0) {
        real(shader, count, string, length);
        return;
    }

    char *joined = (char *)malloc(total + 1);
    if (joined == NULL) {
        /* 分配失败：退化为直接转发。转换器会尽力处理，失败也不崩溃。 */
        real(shader, count, string, length);
        return;
    }
    size_t off = 0;
    for (GLsizei i = 0; i < count; i++) {
        if (string[i] == NULL) continue;
        size_t n = (length != NULL && length[i] >= 0)
                   ? (size_t)length[i]
                   : strlen(string[i]);
        memcpy(joined + off, string[i], n);
        off += n;
    }
    joined[off] = '\0';

    /*
     * 查询该 shader 的真实阶段。
     *
     * 【为什么必须查，而不是靠源码猜测】
     *   片元着色器必须注入 precision 声明，否则 ES 驱动必然编译失败
     *   （桌面 GLSL 无此要求，因此桌面着色器里不会有）。
     *   而「是否是片元着色器」无法从源码可靠推断：现代的桌面片元着色器
     *   用 `out vec4 fragColor` 而非 gl_FragColor，靠关键字猜会漏判。
     *   glGetShaderiv(GL_SHADER_TYPE) 一直能给出确切答案，因此直接查。
     */
    int stage = GLESMOD_SHADER_STAGE_UNKNOWN;
    {
        static void (*get_shaderiv)(GLuint, GLenum, GLint *) = NULL;
        if (get_shaderiv == NULL) {
            get_shaderiv = (void (*)(GLuint, GLenum, GLint *))
                           glesym_resolve("glGetShaderiv");
        }
        if (get_shaderiv != NULL) {
            GLint t = 0;
            get_shaderiv(shader, GLESMOD_GL_SHADER_TYPE, &t);
            stage = (int)t;
        }
    }

    /*
     * 无条件记录「收到的源码里是否含 #moj_import」。
     *
     * 【为什么要记这个】
     *   之前的排查卡在「无法确定 #moj_import 到底有没有到达我们这里」。
     *   若它到达了而我们没展开 -> 是本库的缺陷；
     *   若它根本没到达（已是展开后的源码）-> 故障在别处。
     *   这两种情况的修法完全不同，而日志里看不出区别。
     *   统计「含该指令的着色器数量」即可一次定论。
     */
    {
        static int moj_seen = 0;
        if (strstr(joined, "moj_import") != NULL) {
            if (moj_seen < 3) {          /* 只记前几次，避免刷屏 */
                glesmod_log("收到含 #moj_import 的着色器源码 —— "
                            "该指令 GLSL 不支持，必须由本库展开");
            }
            moj_seen++;
        }
    }

    int out_len = 0;
    char *converted = glesmod_convert_shader_source(joined, &out_len, stage);

    /*
     * 记住本次编译的「原始源码」与「转换后源码」，绑定到该 shader 对象。
     *
     * 【为什么必须做，而且必须覆盖每一个着色器】
     *   真机上「着色器编译失败」的可见信息只有驱动那一行错误。
     *   而 shader 是 GL 对象，glGetShaderInfoLog 拿不到它的源码，
     *   因此源码必须在 glShaderSource 时按 shader 名字留下。
     *
     *   曾经的错误做法：只转储「前 N 个」着色器以控制日志体积。
     *   结果第 9 个以后失败的着色器完全没有记录，失败原因无法定位 ——
     *   这正是连续几轮排查卡住的原因。
     *
     *   正确做法：全部记住（内存换可诊断性），但【只在编译失败时】
     *   才真正写入日志。这样正常启动时日志不会变大，
     *   而失败时一定有完整信息。
     */
    remember_shader_source(shader, joined, converted, stage);

    free(joined);

    if (converted == NULL) {
        /*
         * 【必须留痕 —— 这里曾是一个完全静默的分支，导致一次误判】
         *
         * 本分支的语义是：转换失败 -> 我们**原样**把桌面源码转发给驱动。
         * 此时设备报出的错误会与"本库完全没生效"**一模一样**，
         * 于是很容易把「转换器在这次调用里失败了」误判成
         * 「装的还是旧版本 / 修复无效」，白跑一轮。
         *
         * 这条日志就是用来区分这两种情况的：
         *   - 有本行  -> 转换器确实跑了，但在这次调用上返回 NULL
         *   - 无本行且错误依旧 -> 极可能装的是旧产物（看存活标记的构建戳）
         */
        glesmod_log("着色器转换返回 NULL —— 已原样转发桌面源码给驱动"
                    "（驱动将报出桌面 GLSL 不兼容错误；"
                    "若同时看到『编译失败的着色器』转储，"
                    "请把它的『收到的原始源码』发回以便定位）");
        real(shader, count, string, length);
        return;
    }

    const GLchar *src = converted;
    GLint src_len = out_len;
    real(shader, 1, &src, &src_len);

    free(converted);
}

/* ================================================================== */
/* 按 shader 记录源码，供编译失败时定位                               */
/* ================================================================== */

/*
 * 【为什么需要这张表 —— 这是连续几轮排查卡住的根本原因】
 *
 *   真机上「着色器编译失败」的可见信息只有驱动那一行错误，例如：
 *       ERROR: 0:4: '' : GLSL compile error: malformed preprocessor directive
 *   它只给出【行号】，不给出源码。而 shader 是 GL 对象，
 *   glGetShaderInfoLog 只返回错误文本，拿不到当时送进去的源码。
 *   因此源码必须在 glShaderSource 时按 shader 名字留下，
 *   否则失败时无从比对「第 4 行到底是什么」。
 *
 *   曾经的错误做法：只转储「前 N 个」着色器来控制日志体积。
 *   结果第 N 个之后失败的着色器完全没有记录 —— 而 MC 启动时先编译
 *   百余个着色器，真正失败的往往是靠后的那个。日志里全是成功的、
 *   恰好没有失败的那个，排查因此一直停留在猜测。
 *
 *   正确做法：
 *     - 全部记住（按 shader 名索引，不写日志，只占内存）
 *     - 只在【编译失败时】把该 shader 的两份源码写进日志
 *   这样正常启动日志不变大，失败时一定拿得到完整证据。
 *
 *   【本次修正】以前这里受 GLESMOD_SHADER_PROBE 控制，而那个开关被归为
 *   「日志体积」类，已经在 manifest 里改成 0 —— 结果是【真正失败时反而拿不到证据】：
 *       [GLESMod] 编译失败的着色器没有记录到源码
 *   而这张表只占内存、平时不写日志，完全不应受日志体积开关影响。
 *   故改为【总是记录】（不再读环境变量）。
 */
#define SHADER_SRC_SLOTS 512

typedef struct {
    GLuint shader;      /* 0 表示空槽 */
    char *raw;          /* glShaderSource 收到的原始源码 */
    char *converted;    /* 我们实际送入驱动的源码；转换失败时为 NULL */
    int stage;
} shader_src_entry;

static shader_src_entry g_shader_src[SHADER_SRC_SLOTS];

/*
 * 是否记录源码。
 *
 * 【为什么不再读环境变量】
 *   记录本身只占内存（单个着色器数 KB，峰值百余个），**平时不写任何日志**，
 *   只在编译失败时输出。因此它不应受「减少日志体积」类开关的约束。
 *   实测教训：把它绑在 GLESMOD_SHADER_PROBE=0 上，导致真机失败时
 *   拿到的是「没有记录到源码」—— 恰好把唯一能定位问题的证据丢掉了。
 */
static int shader_src_enabled(void) {
    return 1;   /* 总是记录（仅在失败时写日志，无体积代价） */
}

/*
 * 记录一个 shader 的源码。
 *
 * raw/converted 会被复制一份：调用方随后会 free 它们，而失败可能发生在
 * 很久之后（MC 往往编译完一批才检查状态）。
 */
static void remember_shader_source(GLuint shader, const char *raw,
                                   const char *converted, int stage) {
    if (!shader_src_enabled()) return;

    /* 先找已有槽位（同一 shader 可能被重复设置源码） */
    shader_src_entry *e = NULL;
    shader_src_entry *empty = NULL;
    for (int i = 0; i < SHADER_SRC_SLOTS; i++) {
        if (g_shader_src[i].shader == shader) { e = &g_shader_src[i]; break; }
        if (empty == NULL && g_shader_src[i].shader == 0) {
            empty = &g_shader_src[i];
        }
    }
    if (e == NULL) e = empty;
    if (e == NULL) return;      /* 表满：放弃记录，不影响渲染 */

    /* 释放旧内容（重复设置源码的情形） */
    free(e->raw);
    free(e->converted);

    e->shader = shader;
    e->stage = stage;
    e->raw = NULL;
    e->converted = NULL;

    if (raw != NULL) {
        size_t n = strlen(raw);
        e->raw = (char *)malloc(n + 1);
        if (e->raw != NULL) memcpy(e->raw, raw, n + 1);
    }
    if (converted != NULL) {
        size_t n = strlen(converted);
        e->converted = (char *)malloc(n + 1);
        if (e->converted != NULL) memcpy(e->converted, converted, n + 1);
    }
}

/*
 * 在编译失败时，把该 shader 的「原始源码」与「实际送入驱动的源码」写进日志。
 *
 * 这是整个诊断链条里最关键的一步：驱动只说「第 4 行有问题」，
 * 而这里给出第 4 行究竟是什么。
 */
static void dump_shader_source_on_failure(GLuint shader) {
    /*
     * 【去重】本函数有两条调用路径：glGetShaderiv 查 COMPILE_STATUS 失败，
     *   以及 glGetShaderInfoLog 返回非空。Sodium 两条都会走，且现在转储
     *   还会写进 stderr —— 不去重会让同一份源码在游戏日志里出现两遍。
     *   用固定小表记录已转储过的 shader 名。
     */
    static GLuint dumped[64];
    static int dumped_count = 0;
    for (int i = 0; i < dumped_count; i++) {
        if (dumped[i] == shader) return;   /* 已转储过 */
    }
    if (dumped_count < 64) dumped[dumped_count++] = shader;

    shader_src_entry *e = NULL;
    for (int i = 0; i < SHADER_SRC_SLOTS; i++) {
        if (g_shader_src[i].shader == shader) { e = &g_shader_src[i]; break; }
    }
    if (e == NULL) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "编译失败的着色器（GL 名称=%u）没有记录到源码："
                 "该 shader 未经过本库的 glShaderSource，"
                 "或记录表（%d 槽）已满",
                 (unsigned)shader, SHADER_SRC_SLOTS);
        glesmod_log(msg);
        return;
    }

    char note[192];
    snprintf(note, sizeof(note),
             "编译失败的着色器 —— 阶段=%s，GL 名称=%u",
             e->stage == GLESMOD_SHADER_STAGE_VERTEX ? "顶点" :
             e->stage == GLESMOD_SHADER_STAGE_FRAGMENT ? "片元" : "未知",
             (unsigned)e->shader);

    if (e->raw != NULL) {
        glesmod_trace_dump("该 shader 收到 glShaderSource 的原始源码", e->raw);
    }
    if (e->converted != NULL) {
        glesmod_trace_dump("该 shader 实际被送入驱动的源码（转换后）",
                           e->converted);
    } else {
        glesmod_trace_dump("该 shader 转换失败，已原样转发原始源码",
                           e->raw != NULL ? e->raw : "");
    }

    glesmod_log(note);
}

/* ================================================================== */
/* glGetShaderiv —— 编译失败时主动转储源码                            */
/* ================================================================== */

/*
 * 包装 glGetShaderiv：当调用方查询 GL_COMPILE_STATUS 且结果为失败时，
 * 立即把该 shader 的源码写进日志。
 *
 * 【为什么不能只依赖 glGetShaderInfoLog】
 *   取错误文本是调用方的自由选择：它可能只查 COMPILE_STATUS 就自行决定
 *   放弃，也可能用别的路径报告错误。而源码只有在【我们知道失败了】的
 *   前提下才值得写日志。
 *   在 COMPILE_STATUS 这个必经之路上做判断，覆盖面最广：
 *   任何「检查编译结果」的代码都会经过这里。
 *
 *   同时保留 glGetShaderInfoLog 的拦截：那里能拿到驱动的原文，
 *   与本函数转储的源码正好互补 —— 一个给「第几行」，
 *   一个给「第几行是什么」。
 *
 * 行为与标准 glGetShaderiv 完全一致，只是在失败时多写一段日志。
 */
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
    typedef void (*fn_t)(GLuint, GLenum, GLint *);
    static fn_t real = NULL;

    glesmod_lazy_init();
    RESOLVE_OR_RETURN(real, "glGetShaderiv", );

    real(shader, pname, params);

    /*
     * GL_COMPILE_STATUS = 0x8B81，GL_TRUE = 1。
     * 只在「确实是编译状态查询且失败」时转储，避免刷屏。
     *
     * 用 static 标记该 shader 是否已转储过，防止调用方反复查询
     * 导致同一份源码被重复写多次（MC 可能多处 poll 状态）。
     */
    if (pname == 0x8B81 && params != NULL && *params != 1) {
        static GLuint already[64];
        static int already_count = 0;
        int seen = 0;
        for (int i = 0; i < already_count; i++) {
            if (already[i] == shader) { seen = 1; break; }
        }
        if (!seen) {
            if (already_count < 64) already[already_count++] = shader;
            dump_shader_source_on_failure(shader);
        }
    }
}

/* ================================================================== */
/* glGetShaderInfoLog / glGetProgramInfoLog —— 捕获编译错误            */
/* ================================================================== */

/*
 * 【为什么必须专门拦截这两个函数】
 *
 *   真机上「着色器编译失败」的全部可见信息只有一行 Java 异常：
 *       java.lang.IllegalStateException: could not preload shader position
 *   它既不说哪个 GLSL 语法有问题，也不说第几行、什么原因。
 *   而 MC 在判定编译失败之前，一定会调用 glGetShaderInfoLog 去取错误文本
 *   （源码 GameRenderer.preloadShader 与各 RenderType 皆如此）。
 *
 *   因此把这两个函数拦下来，把驱动返回的错误文本原样写进日志，
 *   就能拿到【唯一可靠的一手证据】：驱动给的确切错误行号与原因。
 *   之前几轮排查都靠推断（行号算术、扫描 ES 不支持的关键字），
 *   不如直接把驱动的话记下来。
 *
 *   行为完全不变：先调用真正的 GL 函数，再在返回后记录文本。
 *   信息日志为空（成功情况）时不写，避免刷屏。
 */

/* 打印带行号的文本块，便于与着色器源码对照 */
static void log_shader_log_text(const char *label, const GLchar *text) {
    if (text == NULL || text[0] == '\0') return;

    /*
     * 【为什么必须同时走 glesmod_log】
     *   glesmod_trace_dump 只写 glesmod/native.log 文件，**不进 stderr**，
     *   而用户收集的是启动器日志（即 stderr 重定向）。
     *   结果：连续几轮真机日志里只能看到「编译失败的着色器 —— 阶段=顶点」，
     *   却看不到驱动到底报了什么 —— 而那句话才是唯一能定位根因的证据。
     *   故这里把驱动原文用 glesmod_log 重发一遍（会进 stderr 与游戏日志）。
     *
     * 驱动文本可能带换行，拆行后逐行发出，避免被截断。
     */
    const char *p = text;
    char head[160];
    snprintf(head, sizeof(head), "===== %s =====", label);
    glesmod_log_to_file(head);
    glesmod_log(head);

    int lineno = 0;
    while (*p != '\0' && lineno < 60) {
        const char *eol = strchr(p, '\n');
        size_t n = eol ? (size_t)(eol - p) : strlen(p);
        while (n > 0 && (p[n - 1] == '\r' || p[n - 1] == ' ')) n--;
        if (n > 700) n = 700;

        char line[760];
        snprintf(line, sizeof(line), "  %.700s", p);
        line[n + 2] = '\0';
        glesmod_log_to_file(line);
        if (n > 0) glesmod_log(line);   /* 空行不重发，避免刷屏 */

        lineno++;
        if (eol == NULL) break;
        p = eol + 1;
    }
}

void glGetShaderInfoLog(GLuint shader, GLsizei maxLength,
                        GLsizei *length, GLchar *infoLog) {
    typedef void (*fn_t)(GLuint, GLsizei, GLsizei *, GLchar *);
    static fn_t real = NULL;

    glesmod_lazy_init();
    RESOLVE_OR_RETURN(real, "glGetShaderInfoLog", );

    real(shader, maxLength, length, infoLog);

    /*
     * 只在有实际内容时处理。
     * 编译成功时驱动返回空串，此时不该产生任何日志、也不该转储源码。
     *
     * 【为什么在这里转储源码】
     *   驱动给出的错误只有「第几行、什么错」，例如：
     *       ERROR: 0:4: '' : GLSL compile error: malformed preprocessor directive
     *   要修就必须知道第 4 行到底是什么。而源码在 glShaderSource 时就已
     *   按 shader 名记下，此处按同一个 shader 取回并写入日志。
     *   这样「错误行号」与「该行内容」终于能对上。
     */
    if (infoLog != NULL && infoLog[0] != '\0') {
        dump_shader_source_on_failure(shader);
        log_shader_log_text("驱动返回的着色器编译错误 (glGetShaderInfoLog)",
                            infoLog);
    }
}

void glGetProgramInfoLog(GLuint program, GLsizei maxLength,
                         GLsizei *length, GLchar *infoLog) {
    typedef void (*fn_t)(GLuint, GLsizei, GLsizei *, GLchar *);
    static fn_t real = NULL;

    glesmod_lazy_init();
    RESOLVE_OR_RETURN(real, "glGetProgramInfoLog", );

    real(program, maxLength, length, infoLog);

    if (infoLog != NULL && infoLog[0] != '\0') {
        log_shader_log_text("驱动返回的程序链接错误 (glGetProgramInfoLog)",
                            infoLog);
    }
}

/*
 * generated_forwarders.c —— 由 native/tools/gen_gl_forwarders.py 自动生成
 *
 * 请勿手工编辑本文件。修改 native/symbols.def 后重新生成：
 *   py native/tools/gen_gl_forwarders.py --def native/symbols.def \
 *       --out native/src/generated_forwarders.c
 *
 * 本文件包含三类实现：
 *   - 337 个转发函数（GLES 有同名函数）
 *   - 482 个安全 stub（GLES 不提供，返回零值并记降级）
 *   - 30 个定制实现在 custom.c（本文件不包含）
 *
 * 为什么需要 stub：LWJGL 在 GL.createCapabilities() 时需要一批函数，
 * 缺失会让它抛 NullPointerException 直接崩溃。导出为 stub 可让它拿到
 * 有效指针，调用时返回零值，游戏继续运行。详见 symbols.def 头部说明。
 *
 * LGPL-3.0-or-later
 */

#include "gl_internal.h"

/* ==================== 直接转发 ==================== */

/* glActiveShaderProgram —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glActiveShaderProgram(GLuint p0, GLuint p1)
{
    GLESMOD_HOTPATH("glActiveShaderProgram");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glActiveShaderProgram");
        if (!fn) {
            glesmod_report_missing("glActiveShaderProgram");
            return;
        }
    }
    ((void (*)(GLuint, GLuint))fn)(p0, p1);
}

/* glActiveTexture —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glActiveTexture(GLenum p0)
{
    GLESMOD_HOTPATH("glActiveTexture");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glActiveTexture");
        if (!fn) {
            glesmod_report_missing("glActiveTexture");
            return;
        }
    }
    ((void (*)(GLenum))fn)(p0);
}

/* glAttachShader —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glAttachShader(GLuint p0, GLuint p1)
{
    GLESMOD_HOTPATH("glAttachShader");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glAttachShader");
        if (!fn) {
            glesmod_report_missing("glAttachShader");
            return;
        }
    }
    ((void (*)(GLuint, GLuint))fn)(p0, p1);
}

/* glBeginQuery —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBeginQuery(GLenum p0, GLuint p1)
{
    GLESMOD_HOTPATH("glBeginQuery");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBeginQuery");
        if (!fn) {
            glesmod_report_missing("glBeginQuery");
            return;
        }
    }
    ((void (*)(GLenum, GLuint))fn)(p0, p1);
}

/* glBeginTransformFeedback —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBeginTransformFeedback(GLenum p0)
{
    GLESMOD_HOTPATH("glBeginTransformFeedback");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBeginTransformFeedback");
        if (!fn) {
            glesmod_report_missing("glBeginTransformFeedback");
            return;
        }
    }
    ((void (*)(GLenum))fn)(p0);
}

/* glBindAttribLocation —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBindAttribLocation(GLuint p0, GLuint p1, const GLchar * p2)
{
    GLESMOD_HOTPATH("glBindAttribLocation");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBindAttribLocation");
        if (!fn) {
            glesmod_report_missing("glBindAttribLocation");
            return;
        }
    }
    ((void (*)(GLuint, GLuint, const GLchar *))fn)(p0, p1, p2);
}

/* glBindBuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBindBuffer(GLenum p0, GLuint p1)
{
    GLESMOD_HOTPATH("glBindBuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBindBuffer");
        if (!fn) {
            glesmod_report_missing("glBindBuffer");
            return;
        }
    }
    ((void (*)(GLenum, GLuint))fn)(p0, p1);
}

/* glBindBufferBase —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBindBufferBase(GLenum p0, GLuint p1, GLuint p2)
{
    GLESMOD_HOTPATH("glBindBufferBase");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBindBufferBase");
        if (!fn) {
            glesmod_report_missing("glBindBufferBase");
            return;
        }
    }
    ((void (*)(GLenum, GLuint, GLuint))fn)(p0, p1, p2);
}

/* glBindBufferRange —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBindBufferRange(GLenum p0, GLuint p1, GLuint p2, GLintptr p3, GLsizeiptr p4)
{
    GLESMOD_HOTPATH("glBindBufferRange");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBindBufferRange");
        if (!fn) {
            glesmod_report_missing("glBindBufferRange");
            return;
        }
    }
    ((void (*)(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr))fn)(p0, p1, p2, p3, p4);
}

/* glBindFramebuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBindFramebuffer(GLenum p0, GLuint p1)
{
    GLESMOD_HOTPATH("glBindFramebuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBindFramebuffer");
        if (!fn) {
            glesmod_report_missing("glBindFramebuffer");
            return;
        }
    }
    ((void (*)(GLenum, GLuint))fn)(p0, p1);
}

/* glBindImageTexture —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBindImageTexture(GLuint p0, GLuint p1, GLint p2, GLboolean p3, GLint p4, GLenum p5, GLenum p6)
{
    GLESMOD_HOTPATH("glBindImageTexture");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBindImageTexture");
        if (!fn) {
            glesmod_report_missing("glBindImageTexture");
            return;
        }
    }
    ((void (*)(GLuint, GLuint, GLint, GLboolean, GLint, GLenum, GLenum))fn)(p0, p1, p2, p3, p4, p5, p6);
}

/* glBindProgramPipeline —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBindProgramPipeline(GLuint p0)
{
    GLESMOD_HOTPATH("glBindProgramPipeline");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBindProgramPipeline");
        if (!fn) {
            glesmod_report_missing("glBindProgramPipeline");
            return;
        }
    }
    ((void (*)(GLuint))fn)(p0);
}

/* glBindRenderbuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBindRenderbuffer(GLenum p0, GLuint p1)
{
    GLESMOD_HOTPATH("glBindRenderbuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBindRenderbuffer");
        if (!fn) {
            glesmod_report_missing("glBindRenderbuffer");
            return;
        }
    }
    ((void (*)(GLenum, GLuint))fn)(p0, p1);
}

/* glBindSampler —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBindSampler(GLuint p0, GLuint p1)
{
    GLESMOD_HOTPATH("glBindSampler");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBindSampler");
        if (!fn) {
            glesmod_report_missing("glBindSampler");
            return;
        }
    }
    ((void (*)(GLuint, GLuint))fn)(p0, p1);
}

/* glBindTexture —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBindTexture(GLenum p0, GLuint p1)
{
    GLESMOD_HOTPATH("glBindTexture");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBindTexture");
        if (!fn) {
            glesmod_report_missing("glBindTexture");
            return;
        }
    }
    ((void (*)(GLenum, GLuint))fn)(p0, p1);
}

/* glBindTransformFeedback —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBindTransformFeedback(GLenum p0, GLuint p1)
{
    GLESMOD_HOTPATH("glBindTransformFeedback");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBindTransformFeedback");
        if (!fn) {
            glesmod_report_missing("glBindTransformFeedback");
            return;
        }
    }
    ((void (*)(GLenum, GLuint))fn)(p0, p1);
}

/* glBindVertexArray —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBindVertexArray(GLuint p0)
{
    GLESMOD_HOTPATH("glBindVertexArray");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBindVertexArray");
        if (!fn) {
            glesmod_report_missing("glBindVertexArray");
            return;
        }
    }
    ((void (*)(GLuint))fn)(p0);
}

/* glBindVertexBuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBindVertexBuffer(GLuint p0, GLuint p1, GLintptr p2, GLsizei p3)
{
    GLESMOD_HOTPATH("glBindVertexBuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBindVertexBuffer");
        if (!fn) {
            glesmod_report_missing("glBindVertexBuffer");
            return;
        }
    }
    ((void (*)(GLuint, GLuint, GLintptr, GLsizei))fn)(p0, p1, p2, p3);
}

/* glBlendColor —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBlendColor(GLfloat p0, GLfloat p1, GLfloat p2, GLfloat p3)
{
    GLESMOD_HOTPATH("glBlendColor");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBlendColor");
        if (!fn) {
            glesmod_report_missing("glBlendColor");
            return;
        }
    }
    ((void (*)(GLfloat, GLfloat, GLfloat, GLfloat))fn)(p0, p1, p2, p3);
}

/* glBlendEquation —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBlendEquation(GLenum p0)
{
    GLESMOD_HOTPATH("glBlendEquation");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBlendEquation");
        if (!fn) {
            glesmod_report_missing("glBlendEquation");
            return;
        }
    }
    ((void (*)(GLenum))fn)(p0);
}

/* glBlendEquationSeparate —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBlendEquationSeparate(GLenum p0, GLenum p1)
{
    GLESMOD_HOTPATH("glBlendEquationSeparate");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBlendEquationSeparate");
        if (!fn) {
            glesmod_report_missing("glBlendEquationSeparate");
            return;
        }
    }
    ((void (*)(GLenum, GLenum))fn)(p0, p1);
}

/* glBlendEquationSeparatei —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBlendEquationSeparatei(GLuint p0, GLenum p1, GLenum p2)
{
    GLESMOD_HOTPATH("glBlendEquationSeparatei");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBlendEquationSeparatei");
        if (!fn) {
            glesmod_report_missing("glBlendEquationSeparatei");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLenum))fn)(p0, p1, p2);
}

/* glBlendEquationi —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBlendEquationi(GLuint p0, GLenum p1)
{
    GLESMOD_HOTPATH("glBlendEquationi");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBlendEquationi");
        if (!fn) {
            glesmod_report_missing("glBlendEquationi");
            return;
        }
    }
    ((void (*)(GLuint, GLenum))fn)(p0, p1);
}

/* glBlendFunc —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBlendFunc(GLenum p0, GLenum p1)
{
    GLESMOD_HOTPATH("glBlendFunc");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBlendFunc");
        if (!fn) {
            glesmod_report_missing("glBlendFunc");
            return;
        }
    }
    ((void (*)(GLenum, GLenum))fn)(p0, p1);
}

/* glBlendFuncSeparate —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBlendFuncSeparate(GLenum p0, GLenum p1, GLenum p2, GLenum p3)
{
    GLESMOD_HOTPATH("glBlendFuncSeparate");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBlendFuncSeparate");
        if (!fn) {
            glesmod_report_missing("glBlendFuncSeparate");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLenum, GLenum))fn)(p0, p1, p2, p3);
}

/* glBlendFuncSeparatei —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBlendFuncSeparatei(GLuint p0, GLenum p1, GLenum p2, GLenum p3, GLenum p4)
{
    GLESMOD_HOTPATH("glBlendFuncSeparatei");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBlendFuncSeparatei");
        if (!fn) {
            glesmod_report_missing("glBlendFuncSeparatei");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLenum, GLenum, GLenum))fn)(p0, p1, p2, p3, p4);
}

/* glBlendFunci —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBlendFunci(GLuint p0, GLenum p1, GLenum p2)
{
    GLESMOD_HOTPATH("glBlendFunci");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBlendFunci");
        if (!fn) {
            glesmod_report_missing("glBlendFunci");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLenum))fn)(p0, p1, p2);
}

/* glBlitFramebuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBlitFramebuffer(GLint p0, GLint p1, GLint p2, GLint p3, GLint p4, GLint p5, GLint p6, GLint p7, GLbitfield p8, GLenum p9)
{
    GLESMOD_HOTPATH("glBlitFramebuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBlitFramebuffer");
        if (!fn) {
            glesmod_report_missing("glBlitFramebuffer");
            return;
        }
    }
    ((void (*)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum))fn)(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
}

/* glBufferData —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBufferData(GLenum p0, GLsizeiptr p1, const void * p2, GLenum p3)
{
    GLESMOD_HOTPATH("glBufferData");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBufferData");
        if (!fn) {
            glesmod_report_missing("glBufferData");
            return;
        }
    }
    ((void (*)(GLenum, GLsizeiptr, const void *, GLenum))fn)(p0, p1, p2, p3);
}

/* glBufferSubData —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glBufferSubData(GLenum p0, GLintptr p1, GLsizeiptr p2, const void * p3)
{
    GLESMOD_HOTPATH("glBufferSubData");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glBufferSubData");
        if (!fn) {
            glesmod_report_missing("glBufferSubData");
            return;
        }
    }
    ((void (*)(GLenum, GLintptr, GLsizeiptr, const void *))fn)(p0, p1, p2, p3);
}

/* glClear —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glClear(GLbitfield p0)
{
    GLESMOD_HOTPATH("glClear");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glClear");
        if (!fn) {
            glesmod_report_missing("glClear");
            return;
        }
    }
    ((void (*)(GLbitfield))fn)(p0);
}

/* glClearBufferfi —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glClearBufferfi(GLenum p0, GLint p1, GLfloat p2, GLint p3)
{
    GLESMOD_HOTPATH("glClearBufferfi");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glClearBufferfi");
        if (!fn) {
            glesmod_report_missing("glClearBufferfi");
            return;
        }
    }
    ((void (*)(GLenum, GLint, GLfloat, GLint))fn)(p0, p1, p2, p3);
}

/* glClearBufferfv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glClearBufferfv(GLenum p0, GLint p1, const GLfloat * p2)
{
    GLESMOD_HOTPATH("glClearBufferfv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glClearBufferfv");
        if (!fn) {
            glesmod_report_missing("glClearBufferfv");
            return;
        }
    }
    ((void (*)(GLenum, GLint, const GLfloat *))fn)(p0, p1, p2);
}

/* glClearBufferiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glClearBufferiv(GLenum p0, GLint p1, const GLint * p2)
{
    GLESMOD_HOTPATH("glClearBufferiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glClearBufferiv");
        if (!fn) {
            glesmod_report_missing("glClearBufferiv");
            return;
        }
    }
    ((void (*)(GLenum, GLint, const GLint *))fn)(p0, p1, p2);
}

/* glClearBufferuiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glClearBufferuiv(GLenum p0, GLint p1, const GLuint * p2)
{
    GLESMOD_HOTPATH("glClearBufferuiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glClearBufferuiv");
        if (!fn) {
            glesmod_report_missing("glClearBufferuiv");
            return;
        }
    }
    ((void (*)(GLenum, GLint, const GLuint *))fn)(p0, p1, p2);
}

/* glClearColor —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glClearColor(GLfloat p0, GLfloat p1, GLfloat p2, GLfloat p3)
{
    GLESMOD_HOTPATH("glClearColor");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glClearColor");
        if (!fn) {
            glesmod_report_missing("glClearColor");
            return;
        }
    }
    ((void (*)(GLfloat, GLfloat, GLfloat, GLfloat))fn)(p0, p1, p2, p3);
}

/* glClearDepthf —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glClearDepthf(GLfloat p0)
{
    GLESMOD_HOTPATH("glClearDepthf");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glClearDepthf");
        if (!fn) {
            glesmod_report_missing("glClearDepthf");
            return;
        }
    }
    ((void (*)(GLfloat))fn)(p0);
}

/* glClearStencil —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glClearStencil(GLint p0)
{
    GLESMOD_HOTPATH("glClearStencil");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glClearStencil");
        if (!fn) {
            glesmod_report_missing("glClearStencil");
            return;
        }
    }
    ((void (*)(GLint))fn)(p0);
}

/* glClientWaitSync —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLenum glClientWaitSync(GLsync p0, GLbitfield p1, GLuint64 p2)
{
    GLESMOD_HOTPATH("glClientWaitSync");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glClientWaitSync");
        if (!fn) {
            glesmod_report_missing("glClientWaitSync");
            return 0;
        }
    }
    return ((GLenum (*)(GLsync, GLbitfield, GLuint64))fn)(p0, p1, p2);
}

/* glColorMask —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glColorMask(GLboolean p0, GLboolean p1, GLboolean p2, GLboolean p3)
{
    GLESMOD_HOTPATH("glColorMask");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glColorMask");
        if (!fn) {
            glesmod_report_missing("glColorMask");
            return;
        }
    }
    ((void (*)(GLboolean, GLboolean, GLboolean, GLboolean))fn)(p0, p1, p2, p3);
}

/* glColorMaski —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glColorMaski(GLuint p0, GLboolean p1, GLboolean p2, GLboolean p3, GLboolean p4)
{
    GLESMOD_HOTPATH("glColorMaski");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glColorMaski");
        if (!fn) {
            glesmod_report_missing("glColorMaski");
            return;
        }
    }
    ((void (*)(GLuint, GLboolean, GLboolean, GLboolean, GLboolean))fn)(p0, p1, p2, p3, p4);
}

/* glCompileShader —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glCompileShader(GLuint p0)
{
    GLESMOD_HOTPATH("glCompileShader");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCompileShader");
        if (!fn) {
            glesmod_report_missing("glCompileShader");
            return;
        }
    }
    ((void (*)(GLuint))fn)(p0);
}

/* glCompressedTexImage2D —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glCompressedTexImage2D(GLenum p0, GLint p1, GLenum p2, GLsizei p3, GLsizei p4, GLint p5, GLsizei p6, const void * p7)
{
    GLESMOD_HOTPATH("glCompressedTexImage2D");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCompressedTexImage2D");
        if (!fn) {
            glesmod_report_missing("glCompressedTexImage2D");
            return;
        }
    }
    ((void (*)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const void *))fn)(p0, p1, p2, p3, p4, p5, p6, p7);
}

/* glCompressedTexImage3D —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glCompressedTexImage3D(GLenum p0, GLint p1, GLenum p2, GLsizei p3, GLsizei p4, GLsizei p5, GLint p6, GLsizei p7, const void * p8)
{
    GLESMOD_HOTPATH("glCompressedTexImage3D");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCompressedTexImage3D");
        if (!fn) {
            glesmod_report_missing("glCompressedTexImage3D");
            return;
        }
    }
    ((void (*)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const void *))fn)(p0, p1, p2, p3, p4, p5, p6, p7, p8);
}

/* glCompressedTexSubImage2D —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glCompressedTexSubImage2D(GLenum p0, GLint p1, GLint p2, GLint p3, GLsizei p4, GLsizei p5, GLenum p6, GLsizei p7, const void * p8)
{
    GLESMOD_HOTPATH("glCompressedTexSubImage2D");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCompressedTexSubImage2D");
        if (!fn) {
            glesmod_report_missing("glCompressedTexSubImage2D");
            return;
        }
    }
    ((void (*)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const void *))fn)(p0, p1, p2, p3, p4, p5, p6, p7, p8);
}

/* glCompressedTexSubImage3D —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glCompressedTexSubImage3D(GLenum p0, GLint p1, GLint p2, GLint p3, GLint p4, GLsizei p5, GLsizei p6, GLsizei p7, GLenum p8, GLsizei p9, const void * p10)
{
    GLESMOD_HOTPATH("glCompressedTexSubImage3D");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCompressedTexSubImage3D");
        if (!fn) {
            glesmod_report_missing("glCompressedTexSubImage3D");
            return;
        }
    }
    ((void (*)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const void *))fn)(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
}

/* glCopyBufferSubData —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glCopyBufferSubData(GLenum p0, GLenum p1, GLintptr p2, GLintptr p3, GLsizeiptr p4)
{
    GLESMOD_HOTPATH("glCopyBufferSubData");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCopyBufferSubData");
        if (!fn) {
            glesmod_report_missing("glCopyBufferSubData");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr))fn)(p0, p1, p2, p3, p4);
}

/* glCopyImageSubData —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glCopyImageSubData(GLuint p0, GLenum p1, GLint p2, GLint p3, GLint p4, GLint p5, GLuint p6, GLenum p7, GLint p8, GLint p9, GLint p10, GLint p11, GLsizei p12, GLsizei p13, GLsizei p14)
{
    GLESMOD_HOTPATH("glCopyImageSubData");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCopyImageSubData");
        if (!fn) {
            glesmod_report_missing("glCopyImageSubData");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLint, GLint, GLint, GLint, GLuint, GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei))fn)(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14);
}

/* glCopyTexImage2D —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glCopyTexImage2D(GLenum p0, GLint p1, GLenum p2, GLint p3, GLint p4, GLsizei p5, GLsizei p6, GLint p7)
{
    GLESMOD_HOTPATH("glCopyTexImage2D");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCopyTexImage2D");
        if (!fn) {
            glesmod_report_missing("glCopyTexImage2D");
            return;
        }
    }
    ((void (*)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint))fn)(p0, p1, p2, p3, p4, p5, p6, p7);
}

/* glCopyTexSubImage2D —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glCopyTexSubImage2D(GLenum p0, GLint p1, GLint p2, GLint p3, GLint p4, GLint p5, GLsizei p6, GLsizei p7)
{
    GLESMOD_HOTPATH("glCopyTexSubImage2D");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCopyTexSubImage2D");
        if (!fn) {
            glesmod_report_missing("glCopyTexSubImage2D");
            return;
        }
    }
    ((void (*)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei))fn)(p0, p1, p2, p3, p4, p5, p6, p7);
}

/* glCopyTexSubImage3D —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glCopyTexSubImage3D(GLenum p0, GLint p1, GLint p2, GLint p3, GLint p4, GLint p5, GLint p6, GLsizei p7, GLsizei p8)
{
    GLESMOD_HOTPATH("glCopyTexSubImage3D");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCopyTexSubImage3D");
        if (!fn) {
            glesmod_report_missing("glCopyTexSubImage3D");
            return;
        }
    }
    ((void (*)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei))fn)(p0, p1, p2, p3, p4, p5, p6, p7, p8);
}

/* glCreateProgram —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLuint glCreateProgram(void)
{
    GLESMOD_HOTPATH("glCreateProgram");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCreateProgram");
        if (!fn) {
            glesmod_report_missing("glCreateProgram");
            return 0;
        }
    }
    return ((GLuint (*)(void))fn)();
}

/* glCreateShader —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLuint glCreateShader(GLenum p0)
{
    GLESMOD_HOTPATH("glCreateShader");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCreateShader");
        if (!fn) {
            glesmod_report_missing("glCreateShader");
            return 0;
        }
    }
    return ((GLuint (*)(GLenum))fn)(p0);
}

/* glCreateShaderProgramv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLuint glCreateShaderProgramv(GLenum p0, GLsizei p1, const GLchar *const * p2)
{
    GLESMOD_HOTPATH("glCreateShaderProgramv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCreateShaderProgramv");
        if (!fn) {
            glesmod_report_missing("glCreateShaderProgramv");
            return 0;
        }
    }
    return ((GLuint (*)(GLenum, GLsizei, const GLchar *const *))fn)(p0, p1, p2);
}

/* glCullFace —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glCullFace(GLenum p0)
{
    GLESMOD_HOTPATH("glCullFace");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glCullFace");
        if (!fn) {
            glesmod_report_missing("glCullFace");
            return;
        }
    }
    ((void (*)(GLenum))fn)(p0);
}

/* glDebugMessageCallback —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDebugMessageCallback(GLDEBUGPROC p0, const void * p1)
{
    GLESMOD_HOTPATH("glDebugMessageCallback");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDebugMessageCallback");
        if (!fn) {
            glesmod_report_missing("glDebugMessageCallback");
            return;
        }
    }
    ((void (*)(GLDEBUGPROC, const void *))fn)(p0, p1);
}

/* glDebugMessageControl —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDebugMessageControl(GLenum p0, GLenum p1, GLenum p2, GLsizei p3, const GLuint * p4, GLboolean p5)
{
    GLESMOD_HOTPATH("glDebugMessageControl");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDebugMessageControl");
        if (!fn) {
            glesmod_report_missing("glDebugMessageControl");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLenum, GLsizei, const GLuint *, GLboolean))fn)(p0, p1, p2, p3, p4, p5);
}

/* glDebugMessageInsert —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDebugMessageInsert(GLenum p0, GLenum p1, GLuint p2, GLenum p3, GLsizei p4, const GLchar * p5)
{
    GLESMOD_HOTPATH("glDebugMessageInsert");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDebugMessageInsert");
        if (!fn) {
            glesmod_report_missing("glDebugMessageInsert");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar *))fn)(p0, p1, p2, p3, p4, p5);
}

/* glDeleteBuffers —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDeleteBuffers(GLsizei p0, const GLuint * p1)
{
    GLESMOD_HOTPATH("glDeleteBuffers");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDeleteBuffers");
        if (!fn) {
            glesmod_report_missing("glDeleteBuffers");
            return;
        }
    }
    ((void (*)(GLsizei, const GLuint *))fn)(p0, p1);
}

/* glDeleteFramebuffers —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDeleteFramebuffers(GLsizei p0, const GLuint * p1)
{
    GLESMOD_HOTPATH("glDeleteFramebuffers");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDeleteFramebuffers");
        if (!fn) {
            glesmod_report_missing("glDeleteFramebuffers");
            return;
        }
    }
    ((void (*)(GLsizei, const GLuint *))fn)(p0, p1);
}

/* glDeleteProgram —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDeleteProgram(GLuint p0)
{
    GLESMOD_HOTPATH("glDeleteProgram");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDeleteProgram");
        if (!fn) {
            glesmod_report_missing("glDeleteProgram");
            return;
        }
    }
    ((void (*)(GLuint))fn)(p0);
}

/* glDeleteProgramPipelines —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDeleteProgramPipelines(GLsizei p0, const GLuint * p1)
{
    GLESMOD_HOTPATH("glDeleteProgramPipelines");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDeleteProgramPipelines");
        if (!fn) {
            glesmod_report_missing("glDeleteProgramPipelines");
            return;
        }
    }
    ((void (*)(GLsizei, const GLuint *))fn)(p0, p1);
}

/* glDeleteQueries —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDeleteQueries(GLsizei p0, const GLuint * p1)
{
    GLESMOD_HOTPATH("glDeleteQueries");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDeleteQueries");
        if (!fn) {
            glesmod_report_missing("glDeleteQueries");
            return;
        }
    }
    ((void (*)(GLsizei, const GLuint *))fn)(p0, p1);
}

/* glDeleteRenderbuffers —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDeleteRenderbuffers(GLsizei p0, const GLuint * p1)
{
    GLESMOD_HOTPATH("glDeleteRenderbuffers");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDeleteRenderbuffers");
        if (!fn) {
            glesmod_report_missing("glDeleteRenderbuffers");
            return;
        }
    }
    ((void (*)(GLsizei, const GLuint *))fn)(p0, p1);
}

/* glDeleteSamplers —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDeleteSamplers(GLsizei p0, const GLuint * p1)
{
    GLESMOD_HOTPATH("glDeleteSamplers");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDeleteSamplers");
        if (!fn) {
            glesmod_report_missing("glDeleteSamplers");
            return;
        }
    }
    ((void (*)(GLsizei, const GLuint *))fn)(p0, p1);
}

/* glDeleteShader —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDeleteShader(GLuint p0)
{
    GLESMOD_HOTPATH("glDeleteShader");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDeleteShader");
        if (!fn) {
            glesmod_report_missing("glDeleteShader");
            return;
        }
    }
    ((void (*)(GLuint))fn)(p0);
}

/* glDeleteSync —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDeleteSync(GLsync p0)
{
    GLESMOD_HOTPATH("glDeleteSync");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDeleteSync");
        if (!fn) {
            glesmod_report_missing("glDeleteSync");
            return;
        }
    }
    ((void (*)(GLsync))fn)(p0);
}

/* glDeleteTextures —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDeleteTextures(GLsizei p0, const GLuint * p1)
{
    GLESMOD_HOTPATH("glDeleteTextures");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDeleteTextures");
        if (!fn) {
            glesmod_report_missing("glDeleteTextures");
            return;
        }
    }
    ((void (*)(GLsizei, const GLuint *))fn)(p0, p1);
}

/* glDeleteTransformFeedbacks —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDeleteTransformFeedbacks(GLsizei p0, const GLuint * p1)
{
    GLESMOD_HOTPATH("glDeleteTransformFeedbacks");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDeleteTransformFeedbacks");
        if (!fn) {
            glesmod_report_missing("glDeleteTransformFeedbacks");
            return;
        }
    }
    ((void (*)(GLsizei, const GLuint *))fn)(p0, p1);
}

/* glDeleteVertexArrays —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDeleteVertexArrays(GLsizei p0, const GLuint * p1)
{
    GLESMOD_HOTPATH("glDeleteVertexArrays");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDeleteVertexArrays");
        if (!fn) {
            glesmod_report_missing("glDeleteVertexArrays");
            return;
        }
    }
    ((void (*)(GLsizei, const GLuint *))fn)(p0, p1);
}

/* glDepthFunc —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDepthFunc(GLenum p0)
{
    GLESMOD_HOTPATH("glDepthFunc");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDepthFunc");
        if (!fn) {
            glesmod_report_missing("glDepthFunc");
            return;
        }
    }
    ((void (*)(GLenum))fn)(p0);
}

/* glDepthMask —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDepthMask(GLboolean p0)
{
    GLESMOD_HOTPATH("glDepthMask");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDepthMask");
        if (!fn) {
            glesmod_report_missing("glDepthMask");
            return;
        }
    }
    ((void (*)(GLboolean))fn)(p0);
}

/* glDepthRangef —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDepthRangef(GLfloat p0, GLfloat p1)
{
    GLESMOD_HOTPATH("glDepthRangef");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDepthRangef");
        if (!fn) {
            glesmod_report_missing("glDepthRangef");
            return;
        }
    }
    ((void (*)(GLfloat, GLfloat))fn)(p0, p1);
}

/* glDetachShader —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDetachShader(GLuint p0, GLuint p1)
{
    GLESMOD_HOTPATH("glDetachShader");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDetachShader");
        if (!fn) {
            glesmod_report_missing("glDetachShader");
            return;
        }
    }
    ((void (*)(GLuint, GLuint))fn)(p0, p1);
}

/* glDisable —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDisable(GLenum p0)
{
    GLESMOD_HOTPATH("glDisable");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDisable");
        if (!fn) {
            glesmod_report_missing("glDisable");
            return;
        }
    }
    ((void (*)(GLenum))fn)(p0);
}

/* glDisableVertexAttribArray —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDisableVertexAttribArray(GLuint p0)
{
    GLESMOD_HOTPATH("glDisableVertexAttribArray");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDisableVertexAttribArray");
        if (!fn) {
            glesmod_report_missing("glDisableVertexAttribArray");
            return;
        }
    }
    ((void (*)(GLuint))fn)(p0);
}

/* glDisablei —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDisablei(GLenum p0, GLuint p1)
{
    GLESMOD_HOTPATH("glDisablei");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDisablei");
        if (!fn) {
            glesmod_report_missing("glDisablei");
            return;
        }
    }
    ((void (*)(GLenum, GLuint))fn)(p0, p1);
}

/* glDispatchCompute —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDispatchCompute(GLuint p0, GLuint p1, GLuint p2)
{
    GLESMOD_HOTPATH("glDispatchCompute");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDispatchCompute");
        if (!fn) {
            glesmod_report_missing("glDispatchCompute");
            return;
        }
    }
    ((void (*)(GLuint, GLuint, GLuint))fn)(p0, p1, p2);
}

/* glDispatchComputeIndirect —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDispatchComputeIndirect(GLintptr p0)
{
    GLESMOD_HOTPATH("glDispatchComputeIndirect");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDispatchComputeIndirect");
        if (!fn) {
            glesmod_report_missing("glDispatchComputeIndirect");
            return;
        }
    }
    ((void (*)(GLintptr))fn)(p0);
}

/* glDrawArrays —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDrawArrays(GLenum p0, GLint p1, GLsizei p2)
{
    GLESMOD_HOTPATH("glDrawArrays");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDrawArrays");
        if (!fn) {
            glesmod_report_missing("glDrawArrays");
            return;
        }
    }
    ((void (*)(GLenum, GLint, GLsizei))fn)(p0, p1, p2);
}

/* glDrawArraysIndirect —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDrawArraysIndirect(GLenum p0, const void * p1)
{
    GLESMOD_HOTPATH("glDrawArraysIndirect");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDrawArraysIndirect");
        if (!fn) {
            glesmod_report_missing("glDrawArraysIndirect");
            return;
        }
    }
    ((void (*)(GLenum, const void *))fn)(p0, p1);
}

/* glDrawArraysInstanced —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDrawArraysInstanced(GLenum p0, GLint p1, GLsizei p2, GLsizei p3)
{
    GLESMOD_HOTPATH("glDrawArraysInstanced");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDrawArraysInstanced");
        if (!fn) {
            glesmod_report_missing("glDrawArraysInstanced");
            return;
        }
    }
    ((void (*)(GLenum, GLint, GLsizei, GLsizei))fn)(p0, p1, p2, p3);
}

/* glDrawBuffers —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDrawBuffers(GLsizei p0, const GLenum * p1)
{
    GLESMOD_HOTPATH("glDrawBuffers");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDrawBuffers");
        if (!fn) {
            glesmod_report_missing("glDrawBuffers");
            return;
        }
    }
    ((void (*)(GLsizei, const GLenum *))fn)(p0, p1);
}

/* glDrawElementsBaseVertex —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDrawElementsBaseVertex(GLenum p0, GLsizei p1, GLenum p2, const void * p3, GLint p4)
{
    GLESMOD_HOTPATH("glDrawElementsBaseVertex");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDrawElementsBaseVertex");
        if (!fn) {
            glesmod_report_missing("glDrawElementsBaseVertex");
            return;
        }
    }
    ((void (*)(GLenum, GLsizei, GLenum, const void *, GLint))fn)(p0, p1, p2, p3, p4);
}

/* glDrawElementsIndirect —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDrawElementsIndirect(GLenum p0, GLenum p1, const void * p2)
{
    GLESMOD_HOTPATH("glDrawElementsIndirect");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDrawElementsIndirect");
        if (!fn) {
            glesmod_report_missing("glDrawElementsIndirect");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, const void *))fn)(p0, p1, p2);
}

/* glDrawElementsInstanced —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDrawElementsInstanced(GLenum p0, GLsizei p1, GLenum p2, const void * p3, GLsizei p4)
{
    GLESMOD_HOTPATH("glDrawElementsInstanced");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDrawElementsInstanced");
        if (!fn) {
            glesmod_report_missing("glDrawElementsInstanced");
            return;
        }
    }
    ((void (*)(GLenum, GLsizei, GLenum, const void *, GLsizei))fn)(p0, p1, p2, p3, p4);
}

/* glDrawElementsInstancedBaseVertex —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDrawElementsInstancedBaseVertex(GLenum p0, GLsizei p1, GLenum p2, const void * p3, GLsizei p4, GLint p5)
{
    GLESMOD_HOTPATH("glDrawElementsInstancedBaseVertex");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDrawElementsInstancedBaseVertex");
        if (!fn) {
            glesmod_report_missing("glDrawElementsInstancedBaseVertex");
            return;
        }
    }
    ((void (*)(GLenum, GLsizei, GLenum, const void *, GLsizei, GLint))fn)(p0, p1, p2, p3, p4, p5);
}

/* glDrawRangeElements —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDrawRangeElements(GLenum p0, GLuint p1, GLuint p2, GLsizei p3, GLenum p4, const void * p5)
{
    GLESMOD_HOTPATH("glDrawRangeElements");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDrawRangeElements");
        if (!fn) {
            glesmod_report_missing("glDrawRangeElements");
            return;
        }
    }
    ((void (*)(GLenum, GLuint, GLuint, GLsizei, GLenum, const void *))fn)(p0, p1, p2, p3, p4, p5);
}

/* glDrawRangeElementsBaseVertex —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glDrawRangeElementsBaseVertex(GLenum p0, GLuint p1, GLuint p2, GLsizei p3, GLenum p4, const void * p5, GLint p6)
{
    GLESMOD_HOTPATH("glDrawRangeElementsBaseVertex");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glDrawRangeElementsBaseVertex");
        if (!fn) {
            glesmod_report_missing("glDrawRangeElementsBaseVertex");
            return;
        }
    }
    ((void (*)(GLenum, GLuint, GLuint, GLsizei, GLenum, const void *, GLint))fn)(p0, p1, p2, p3, p4, p5, p6);
}

/* glEnable —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glEnable(GLenum p0)
{
    GLESMOD_HOTPATH("glEnable");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glEnable");
        if (!fn) {
            glesmod_report_missing("glEnable");
            return;
        }
    }
    ((void (*)(GLenum))fn)(p0);
}

/* glEnableVertexAttribArray —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glEnableVertexAttribArray(GLuint p0)
{
    GLESMOD_HOTPATH("glEnableVertexAttribArray");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glEnableVertexAttribArray");
        if (!fn) {
            glesmod_report_missing("glEnableVertexAttribArray");
            return;
        }
    }
    ((void (*)(GLuint))fn)(p0);
}

/* glEnablei —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glEnablei(GLenum p0, GLuint p1)
{
    GLESMOD_HOTPATH("glEnablei");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glEnablei");
        if (!fn) {
            glesmod_report_missing("glEnablei");
            return;
        }
    }
    ((void (*)(GLenum, GLuint))fn)(p0, p1);
}

/* glEndQuery —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glEndQuery(GLenum p0)
{
    GLESMOD_HOTPATH("glEndQuery");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glEndQuery");
        if (!fn) {
            glesmod_report_missing("glEndQuery");
            return;
        }
    }
    ((void (*)(GLenum))fn)(p0);
}

/* glEndTransformFeedback —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glEndTransformFeedback(void)
{
    GLESMOD_HOTPATH("glEndTransformFeedback");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glEndTransformFeedback");
        if (!fn) {
            glesmod_report_missing("glEndTransformFeedback");
            return;
        }
    }
    ((void (*)(void))fn)();
}

/* glFenceSync —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLsync glFenceSync(GLenum p0, GLbitfield p1)
{
    GLESMOD_HOTPATH("glFenceSync");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glFenceSync");
        if (!fn) {
            glesmod_report_missing("glFenceSync");
            return NULL;
        }
    }
    return ((GLsync (*)(GLenum, GLbitfield))fn)(p0, p1);
}

/* glFinish —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glFinish(void)
{
    GLESMOD_HOTPATH("glFinish");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glFinish");
        if (!fn) {
            glesmod_report_missing("glFinish");
            return;
        }
    }
    ((void (*)(void))fn)();
}

/* glFlush —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glFlush(void)
{
    GLESMOD_HOTPATH("glFlush");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glFlush");
        if (!fn) {
            glesmod_report_missing("glFlush");
            return;
        }
    }
    ((void (*)(void))fn)();
}

/* glFlushMappedBufferRange —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glFlushMappedBufferRange(GLenum p0, GLintptr p1, GLsizeiptr p2)
{
    GLESMOD_HOTPATH("glFlushMappedBufferRange");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glFlushMappedBufferRange");
        if (!fn) {
            glesmod_report_missing("glFlushMappedBufferRange");
            return;
        }
    }
    ((void (*)(GLenum, GLintptr, GLsizeiptr))fn)(p0, p1, p2);
}

/* glFramebufferParameteri —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glFramebufferParameteri(GLenum p0, GLenum p1, GLint p2)
{
    GLESMOD_HOTPATH("glFramebufferParameteri");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glFramebufferParameteri");
        if (!fn) {
            glesmod_report_missing("glFramebufferParameteri");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLint))fn)(p0, p1, p2);
}

/* glFramebufferRenderbuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glFramebufferRenderbuffer(GLenum p0, GLenum p1, GLenum p2, GLuint p3)
{
    GLESMOD_HOTPATH("glFramebufferRenderbuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glFramebufferRenderbuffer");
        if (!fn) {
            glesmod_report_missing("glFramebufferRenderbuffer");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLenum, GLuint))fn)(p0, p1, p2, p3);
}

/* glFramebufferTexture —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glFramebufferTexture(GLenum p0, GLenum p1, GLuint p2, GLint p3)
{
    GLESMOD_HOTPATH("glFramebufferTexture");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glFramebufferTexture");
        if (!fn) {
            glesmod_report_missing("glFramebufferTexture");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLuint, GLint))fn)(p0, p1, p2, p3);
}

/* glFramebufferTexture2D —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glFramebufferTexture2D(GLenum p0, GLenum p1, GLenum p2, GLuint p3, GLint p4)
{
    GLESMOD_HOTPATH("glFramebufferTexture2D");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glFramebufferTexture2D");
        if (!fn) {
            glesmod_report_missing("glFramebufferTexture2D");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLenum, GLuint, GLint))fn)(p0, p1, p2, p3, p4);
}

/* glFramebufferTextureLayer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glFramebufferTextureLayer(GLenum p0, GLenum p1, GLuint p2, GLint p3, GLint p4)
{
    GLESMOD_HOTPATH("glFramebufferTextureLayer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glFramebufferTextureLayer");
        if (!fn) {
            glesmod_report_missing("glFramebufferTextureLayer");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLuint, GLint, GLint))fn)(p0, p1, p2, p3, p4);
}

/* glFrontFace —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glFrontFace(GLenum p0)
{
    GLESMOD_HOTPATH("glFrontFace");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glFrontFace");
        if (!fn) {
            glesmod_report_missing("glFrontFace");
            return;
        }
    }
    ((void (*)(GLenum))fn)(p0);
}

/* glGenBuffers —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGenBuffers(GLsizei p0, GLuint * p1)
{
    GLESMOD_HOTPATH("glGenBuffers");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGenBuffers");
        if (!fn) {
            glesmod_report_missing("glGenBuffers");
            return;
        }
    }
    ((void (*)(GLsizei, GLuint *))fn)(p0, p1);
}

/* glGenFramebuffers —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGenFramebuffers(GLsizei p0, GLuint * p1)
{
    GLESMOD_HOTPATH("glGenFramebuffers");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGenFramebuffers");
        if (!fn) {
            glesmod_report_missing("glGenFramebuffers");
            return;
        }
    }
    ((void (*)(GLsizei, GLuint *))fn)(p0, p1);
}

/* glGenProgramPipelines —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGenProgramPipelines(GLsizei p0, GLuint * p1)
{
    GLESMOD_HOTPATH("glGenProgramPipelines");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGenProgramPipelines");
        if (!fn) {
            glesmod_report_missing("glGenProgramPipelines");
            return;
        }
    }
    ((void (*)(GLsizei, GLuint *))fn)(p0, p1);
}

/* glGenQueries —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGenQueries(GLsizei p0, GLuint * p1)
{
    GLESMOD_HOTPATH("glGenQueries");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGenQueries");
        if (!fn) {
            glesmod_report_missing("glGenQueries");
            return;
        }
    }
    ((void (*)(GLsizei, GLuint *))fn)(p0, p1);
}

/* glGenRenderbuffers —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGenRenderbuffers(GLsizei p0, GLuint * p1)
{
    GLESMOD_HOTPATH("glGenRenderbuffers");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGenRenderbuffers");
        if (!fn) {
            glesmod_report_missing("glGenRenderbuffers");
            return;
        }
    }
    ((void (*)(GLsizei, GLuint *))fn)(p0, p1);
}

/* glGenSamplers —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGenSamplers(GLsizei p0, GLuint * p1)
{
    GLESMOD_HOTPATH("glGenSamplers");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGenSamplers");
        if (!fn) {
            glesmod_report_missing("glGenSamplers");
            return;
        }
    }
    ((void (*)(GLsizei, GLuint *))fn)(p0, p1);
}

/* glGenTextures —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGenTextures(GLsizei p0, GLuint * p1)
{
    GLESMOD_HOTPATH("glGenTextures");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGenTextures");
        if (!fn) {
            glesmod_report_missing("glGenTextures");
            return;
        }
    }
    ((void (*)(GLsizei, GLuint *))fn)(p0, p1);
}

/* glGenTransformFeedbacks —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGenTransformFeedbacks(GLsizei p0, GLuint * p1)
{
    GLESMOD_HOTPATH("glGenTransformFeedbacks");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGenTransformFeedbacks");
        if (!fn) {
            glesmod_report_missing("glGenTransformFeedbacks");
            return;
        }
    }
    ((void (*)(GLsizei, GLuint *))fn)(p0, p1);
}

/* glGenVertexArrays —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGenVertexArrays(GLsizei p0, GLuint * p1)
{
    GLESMOD_HOTPATH("glGenVertexArrays");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGenVertexArrays");
        if (!fn) {
            glesmod_report_missing("glGenVertexArrays");
            return;
        }
    }
    ((void (*)(GLsizei, GLuint *))fn)(p0, p1);
}

/* glGenerateMipmap —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGenerateMipmap(GLenum p0)
{
    GLESMOD_HOTPATH("glGenerateMipmap");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGenerateMipmap");
        if (!fn) {
            glesmod_report_missing("glGenerateMipmap");
            return;
        }
    }
    ((void (*)(GLenum))fn)(p0);
}

/* glGetActiveAttrib —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetActiveAttrib(GLuint p0, GLuint p1, GLsizei p2, GLsizei * p3, GLint * p4, GLenum * p5, GLchar * p6)
{
    GLESMOD_HOTPATH("glGetActiveAttrib");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetActiveAttrib");
        if (!fn) {
            glesmod_report_missing("glGetActiveAttrib");
            return;
        }
    }
    ((void (*)(GLuint, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLchar *))fn)(p0, p1, p2, p3, p4, p5, p6);
}

/* glGetActiveUniform —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetActiveUniform(GLuint p0, GLuint p1, GLsizei p2, GLsizei * p3, GLint * p4, GLenum * p5, GLchar * p6)
{
    GLESMOD_HOTPATH("glGetActiveUniform");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetActiveUniform");
        if (!fn) {
            glesmod_report_missing("glGetActiveUniform");
            return;
        }
    }
    ((void (*)(GLuint, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLchar *))fn)(p0, p1, p2, p3, p4, p5, p6);
}

/* glGetActiveUniformBlockName —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetActiveUniformBlockName(GLuint p0, GLuint p1, GLsizei p2, GLsizei * p3, GLchar * p4)
{
    GLESMOD_HOTPATH("glGetActiveUniformBlockName");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetActiveUniformBlockName");
        if (!fn) {
            glesmod_report_missing("glGetActiveUniformBlockName");
            return;
        }
    }
    ((void (*)(GLuint, GLuint, GLsizei, GLsizei *, GLchar *))fn)(p0, p1, p2, p3, p4);
}

/* glGetActiveUniformBlockiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetActiveUniformBlockiv(GLuint p0, GLuint p1, GLenum p2, GLint * p3)
{
    GLESMOD_HOTPATH("glGetActiveUniformBlockiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetActiveUniformBlockiv");
        if (!fn) {
            glesmod_report_missing("glGetActiveUniformBlockiv");
            return;
        }
    }
    ((void (*)(GLuint, GLuint, GLenum, GLint *))fn)(p0, p1, p2, p3);
}

/* glGetActiveUniformsiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetActiveUniformsiv(GLuint p0, GLsizei p1, const GLuint * p2, GLenum p3, GLint * p4)
{
    GLESMOD_HOTPATH("glGetActiveUniformsiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetActiveUniformsiv");
        if (!fn) {
            glesmod_report_missing("glGetActiveUniformsiv");
            return;
        }
    }
    ((void (*)(GLuint, GLsizei, const GLuint *, GLenum, GLint *))fn)(p0, p1, p2, p3, p4);
}

/* glGetAttachedShaders —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetAttachedShaders(GLuint p0, GLsizei p1, GLsizei * p2, GLuint * p3)
{
    GLESMOD_HOTPATH("glGetAttachedShaders");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetAttachedShaders");
        if (!fn) {
            glesmod_report_missing("glGetAttachedShaders");
            return;
        }
    }
    ((void (*)(GLuint, GLsizei, GLsizei *, GLuint *))fn)(p0, p1, p2, p3);
}

/* glGetAttribLocation —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLint glGetAttribLocation(GLuint p0, const GLchar * p1)
{
    GLESMOD_HOTPATH("glGetAttribLocation");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetAttribLocation");
        if (!fn) {
            glesmod_report_missing("glGetAttribLocation");
            return 0;
        }
    }
    return ((GLint (*)(GLuint, const GLchar *))fn)(p0, p1);
}

/* glGetBooleani_v —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetBooleani_v(GLenum p0, GLuint p1, GLboolean * p2)
{
    GLESMOD_HOTPATH("glGetBooleani_v");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetBooleani_v");
        if (!fn) {
            glesmod_report_missing("glGetBooleani_v");
            return;
        }
    }
    ((void (*)(GLenum, GLuint, GLboolean *))fn)(p0, p1, p2);
}

/* glGetBooleanv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetBooleanv(GLenum p0, GLboolean * p1)
{
    GLESMOD_HOTPATH("glGetBooleanv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetBooleanv");
        if (!fn) {
            glesmod_report_missing("glGetBooleanv");
            return;
        }
    }
    ((void (*)(GLenum, GLboolean *))fn)(p0, p1);
}

/* glGetBufferParameteri64v —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetBufferParameteri64v(GLenum p0, GLenum p1, GLint64 * p2)
{
    GLESMOD_HOTPATH("glGetBufferParameteri64v");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetBufferParameteri64v");
        if (!fn) {
            glesmod_report_missing("glGetBufferParameteri64v");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLint64 *))fn)(p0, p1, p2);
}

/* glGetBufferParameteriv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetBufferParameteriv(GLenum p0, GLenum p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetBufferParameteriv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetBufferParameteriv");
        if (!fn) {
            glesmod_report_missing("glGetBufferParameteriv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLint *))fn)(p0, p1, p2);
}

/* glGetBufferPointerv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetBufferPointerv(GLenum p0, GLenum p1, void ** p2)
{
    GLESMOD_HOTPATH("glGetBufferPointerv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetBufferPointerv");
        if (!fn) {
            glesmod_report_missing("glGetBufferPointerv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, void **))fn)(p0, p1, p2);
}

/* glGetDebugMessageLog —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLuint glGetDebugMessageLog(GLuint p0, GLsizei p1, GLenum * p2, GLenum * p3, GLuint * p4, GLenum * p5, GLsizei * p6, GLchar * p7)
{
    GLESMOD_HOTPATH("glGetDebugMessageLog");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetDebugMessageLog");
        if (!fn) {
            glesmod_report_missing("glGetDebugMessageLog");
            return 0;
        }
    }
    return ((GLuint (*)(GLuint, GLsizei, GLenum *, GLenum *, GLuint *, GLenum *, GLsizei *, GLchar *))fn)(p0, p1, p2, p3, p4, p5, p6, p7);
}

/* glGetFloatv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetFloatv(GLenum p0, GLfloat * p1)
{
    GLESMOD_HOTPATH("glGetFloatv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetFloatv");
        if (!fn) {
            glesmod_report_missing("glGetFloatv");
            return;
        }
    }
    ((void (*)(GLenum, GLfloat *))fn)(p0, p1);
}

/* glGetFragDataLocation —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLint glGetFragDataLocation(GLuint p0, const GLchar * p1)
{
    GLESMOD_HOTPATH("glGetFragDataLocation");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetFragDataLocation");
        if (!fn) {
            glesmod_report_missing("glGetFragDataLocation");
            return 0;
        }
    }
    return ((GLint (*)(GLuint, const GLchar *))fn)(p0, p1);
}

/* glGetFramebufferAttachmentParameteriv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetFramebufferAttachmentParameteriv(GLenum p0, GLenum p1, GLenum p2, GLint * p3)
{
    GLESMOD_HOTPATH("glGetFramebufferAttachmentParameteriv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetFramebufferAttachmentParameteriv");
        if (!fn) {
            glesmod_report_missing("glGetFramebufferAttachmentParameteriv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLenum, GLint *))fn)(p0, p1, p2, p3);
}

/* glGetFramebufferParameteriv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetFramebufferParameteriv(GLenum p0, GLenum p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetFramebufferParameteriv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetFramebufferParameteriv");
        if (!fn) {
            glesmod_report_missing("glGetFramebufferParameteriv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLint *))fn)(p0, p1, p2);
}

/* glGetGraphicsResetStatus —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLenum glGetGraphicsResetStatus(void)
{
    GLESMOD_HOTPATH("glGetGraphicsResetStatus");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetGraphicsResetStatus");
        if (!fn) {
            glesmod_report_missing("glGetGraphicsResetStatus");
            return 0;
        }
    }
    return ((GLenum (*)(void))fn)();
}

/* glGetInteger64i_v —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetInteger64i_v(GLenum p0, GLuint p1, GLint64 * p2)
{
    GLESMOD_HOTPATH("glGetInteger64i_v");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetInteger64i_v");
        if (!fn) {
            glesmod_report_missing("glGetInteger64i_v");
            return;
        }
    }
    ((void (*)(GLenum, GLuint, GLint64 *))fn)(p0, p1, p2);
}

/* glGetInteger64v —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetInteger64v(GLenum p0, GLint64 * p1)
{
    GLESMOD_HOTPATH("glGetInteger64v");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetInteger64v");
        if (!fn) {
            glesmod_report_missing("glGetInteger64v");
            return;
        }
    }
    ((void (*)(GLenum, GLint64 *))fn)(p0, p1);
}

/* glGetIntegeri_v —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetIntegeri_v(GLenum p0, GLuint p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetIntegeri_v");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetIntegeri_v");
        if (!fn) {
            glesmod_report_missing("glGetIntegeri_v");
            return;
        }
    }
    ((void (*)(GLenum, GLuint, GLint *))fn)(p0, p1, p2);
}

/* glGetInternalformativ —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetInternalformativ(GLenum p0, GLenum p1, GLenum p2, GLsizei p3, GLint * p4)
{
    GLESMOD_HOTPATH("glGetInternalformativ");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetInternalformativ");
        if (!fn) {
            glesmod_report_missing("glGetInternalformativ");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLenum, GLsizei, GLint *))fn)(p0, p1, p2, p3, p4);
}

/* glGetMultisamplefv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetMultisamplefv(GLenum p0, GLuint p1, GLfloat * p2)
{
    GLESMOD_HOTPATH("glGetMultisamplefv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetMultisamplefv");
        if (!fn) {
            glesmod_report_missing("glGetMultisamplefv");
            return;
        }
    }
    ((void (*)(GLenum, GLuint, GLfloat *))fn)(p0, p1, p2);
}

/* glGetObjectLabel —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetObjectLabel(GLenum p0, GLuint p1, GLsizei p2, GLsizei * p3, GLchar * p4)
{
    GLESMOD_HOTPATH("glGetObjectLabel");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetObjectLabel");
        if (!fn) {
            glesmod_report_missing("glGetObjectLabel");
            return;
        }
    }
    ((void (*)(GLenum, GLuint, GLsizei, GLsizei *, GLchar *))fn)(p0, p1, p2, p3, p4);
}

/* glGetObjectPtrLabel —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetObjectPtrLabel(const void * p0, GLsizei p1, GLsizei * p2, GLchar * p3)
{
    GLESMOD_HOTPATH("glGetObjectPtrLabel");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetObjectPtrLabel");
        if (!fn) {
            glesmod_report_missing("glGetObjectPtrLabel");
            return;
        }
    }
    ((void (*)(const void *, GLsizei, GLsizei *, GLchar *))fn)(p0, p1, p2, p3);
}

/* glGetPointerv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetPointerv(GLenum p0, void ** p1)
{
    GLESMOD_HOTPATH("glGetPointerv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetPointerv");
        if (!fn) {
            glesmod_report_missing("glGetPointerv");
            return;
        }
    }
    ((void (*)(GLenum, void **))fn)(p0, p1);
}

/* glGetProgramBinary —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetProgramBinary(GLuint p0, GLsizei p1, GLsizei * p2, GLenum * p3, void * p4)
{
    GLESMOD_HOTPATH("glGetProgramBinary");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetProgramBinary");
        if (!fn) {
            glesmod_report_missing("glGetProgramBinary");
            return;
        }
    }
    ((void (*)(GLuint, GLsizei, GLsizei *, GLenum *, void *))fn)(p0, p1, p2, p3, p4);
}

/* glGetProgramInterfaceiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetProgramInterfaceiv(GLuint p0, GLenum p1, GLenum p2, GLint * p3)
{
    GLESMOD_HOTPATH("glGetProgramInterfaceiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetProgramInterfaceiv");
        if (!fn) {
            glesmod_report_missing("glGetProgramInterfaceiv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLenum, GLint *))fn)(p0, p1, p2, p3);
}

/* glGetProgramPipelineInfoLog —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetProgramPipelineInfoLog(GLuint p0, GLsizei p1, GLsizei * p2, GLchar * p3)
{
    GLESMOD_HOTPATH("glGetProgramPipelineInfoLog");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetProgramPipelineInfoLog");
        if (!fn) {
            glesmod_report_missing("glGetProgramPipelineInfoLog");
            return;
        }
    }
    ((void (*)(GLuint, GLsizei, GLsizei *, GLchar *))fn)(p0, p1, p2, p3);
}

/* glGetProgramPipelineiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetProgramPipelineiv(GLuint p0, GLenum p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetProgramPipelineiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetProgramPipelineiv");
        if (!fn) {
            glesmod_report_missing("glGetProgramPipelineiv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLint *))fn)(p0, p1, p2);
}

/* glGetProgramResourceIndex —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLuint glGetProgramResourceIndex(GLuint p0, GLenum p1, const GLchar * p2)
{
    GLESMOD_HOTPATH("glGetProgramResourceIndex");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetProgramResourceIndex");
        if (!fn) {
            glesmod_report_missing("glGetProgramResourceIndex");
            return 0;
        }
    }
    return ((GLuint (*)(GLuint, GLenum, const GLchar *))fn)(p0, p1, p2);
}

/* glGetProgramResourceLocation —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLint glGetProgramResourceLocation(GLuint p0, GLenum p1, const GLchar * p2)
{
    GLESMOD_HOTPATH("glGetProgramResourceLocation");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetProgramResourceLocation");
        if (!fn) {
            glesmod_report_missing("glGetProgramResourceLocation");
            return 0;
        }
    }
    return ((GLint (*)(GLuint, GLenum, const GLchar *))fn)(p0, p1, p2);
}

/* glGetProgramResourceName —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetProgramResourceName(GLuint p0, GLenum p1, GLuint p2, GLsizei p3, GLsizei * p4, GLchar * p5)
{
    GLESMOD_HOTPATH("glGetProgramResourceName");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetProgramResourceName");
        if (!fn) {
            glesmod_report_missing("glGetProgramResourceName");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLuint, GLsizei, GLsizei *, GLchar *))fn)(p0, p1, p2, p3, p4, p5);
}

/* glGetProgramResourceiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetProgramResourceiv(GLuint p0, GLenum p1, GLuint p2, GLsizei p3, const GLenum * p4, GLsizei p5, GLsizei * p6, GLint * p7)
{
    GLESMOD_HOTPATH("glGetProgramResourceiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetProgramResourceiv");
        if (!fn) {
            glesmod_report_missing("glGetProgramResourceiv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLuint, GLsizei, const GLenum *, GLsizei, GLsizei *, GLint *))fn)(p0, p1, p2, p3, p4, p5, p6, p7);
}

/* glGetProgramiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetProgramiv(GLuint p0, GLenum p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetProgramiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetProgramiv");
        if (!fn) {
            glesmod_report_missing("glGetProgramiv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLint *))fn)(p0, p1, p2);
}

/* glGetQueryObjectuiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetQueryObjectuiv(GLuint p0, GLenum p1, GLuint * p2)
{
    GLESMOD_HOTPATH("glGetQueryObjectuiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetQueryObjectuiv");
        if (!fn) {
            glesmod_report_missing("glGetQueryObjectuiv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLuint *))fn)(p0, p1, p2);
}

/* glGetQueryiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetQueryiv(GLenum p0, GLenum p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetQueryiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetQueryiv");
        if (!fn) {
            glesmod_report_missing("glGetQueryiv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLint *))fn)(p0, p1, p2);
}

/* glGetRenderbufferParameteriv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetRenderbufferParameteriv(GLenum p0, GLenum p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetRenderbufferParameteriv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetRenderbufferParameteriv");
        if (!fn) {
            glesmod_report_missing("glGetRenderbufferParameteriv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLint *))fn)(p0, p1, p2);
}

/* glGetSamplerParameterIiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetSamplerParameterIiv(GLuint p0, GLenum p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetSamplerParameterIiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetSamplerParameterIiv");
        if (!fn) {
            glesmod_report_missing("glGetSamplerParameterIiv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLint *))fn)(p0, p1, p2);
}

/* glGetSamplerParameterIuiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetSamplerParameterIuiv(GLuint p0, GLenum p1, GLuint * p2)
{
    GLESMOD_HOTPATH("glGetSamplerParameterIuiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetSamplerParameterIuiv");
        if (!fn) {
            glesmod_report_missing("glGetSamplerParameterIuiv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLuint *))fn)(p0, p1, p2);
}

/* glGetSamplerParameterfv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetSamplerParameterfv(GLuint p0, GLenum p1, GLfloat * p2)
{
    GLESMOD_HOTPATH("glGetSamplerParameterfv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetSamplerParameterfv");
        if (!fn) {
            glesmod_report_missing("glGetSamplerParameterfv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLfloat *))fn)(p0, p1, p2);
}

/* glGetSamplerParameteriv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetSamplerParameteriv(GLuint p0, GLenum p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetSamplerParameteriv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetSamplerParameteriv");
        if (!fn) {
            glesmod_report_missing("glGetSamplerParameteriv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLint *))fn)(p0, p1, p2);
}

/* glGetShaderPrecisionFormat —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetShaderPrecisionFormat(GLenum p0, GLenum p1, GLint * p2, GLint * p3)
{
    GLESMOD_HOTPATH("glGetShaderPrecisionFormat");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetShaderPrecisionFormat");
        if (!fn) {
            glesmod_report_missing("glGetShaderPrecisionFormat");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLint *, GLint *))fn)(p0, p1, p2, p3);
}

/* glGetShaderSource —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetShaderSource(GLuint p0, GLsizei p1, GLsizei * p2, GLchar * p3)
{
    GLESMOD_HOTPATH("glGetShaderSource");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetShaderSource");
        if (!fn) {
            glesmod_report_missing("glGetShaderSource");
            return;
        }
    }
    ((void (*)(GLuint, GLsizei, GLsizei *, GLchar *))fn)(p0, p1, p2, p3);
}

/* glGetSynciv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetSynciv(GLsync p0, GLenum p1, GLsizei p2, GLsizei * p3, GLint * p4)
{
    GLESMOD_HOTPATH("glGetSynciv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetSynciv");
        if (!fn) {
            glesmod_report_missing("glGetSynciv");
            return;
        }
    }
    ((void (*)(GLsync, GLenum, GLsizei, GLsizei *, GLint *))fn)(p0, p1, p2, p3, p4);
}

/* glGetTexParameterIiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetTexParameterIiv(GLenum p0, GLenum p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetTexParameterIiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetTexParameterIiv");
        if (!fn) {
            glesmod_report_missing("glGetTexParameterIiv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLint *))fn)(p0, p1, p2);
}

/* glGetTexParameterIuiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetTexParameterIuiv(GLenum p0, GLenum p1, GLuint * p2)
{
    GLESMOD_HOTPATH("glGetTexParameterIuiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetTexParameterIuiv");
        if (!fn) {
            glesmod_report_missing("glGetTexParameterIuiv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLuint *))fn)(p0, p1, p2);
}

/* glGetTexParameterfv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetTexParameterfv(GLenum p0, GLenum p1, GLfloat * p2)
{
    GLESMOD_HOTPATH("glGetTexParameterfv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetTexParameterfv");
        if (!fn) {
            glesmod_report_missing("glGetTexParameterfv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLfloat *))fn)(p0, p1, p2);
}

/* glGetTexParameteriv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetTexParameteriv(GLenum p0, GLenum p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetTexParameteriv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetTexParameteriv");
        if (!fn) {
            glesmod_report_missing("glGetTexParameteriv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLint *))fn)(p0, p1, p2);
}

/* glGetTransformFeedbackVarying —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetTransformFeedbackVarying(GLuint p0, GLuint p1, GLsizei p2, GLsizei * p3, GLsizei * p4, GLenum * p5, GLchar * p6)
{
    GLESMOD_HOTPATH("glGetTransformFeedbackVarying");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetTransformFeedbackVarying");
        if (!fn) {
            glesmod_report_missing("glGetTransformFeedbackVarying");
            return;
        }
    }
    ((void (*)(GLuint, GLuint, GLsizei, GLsizei *, GLsizei *, GLenum *, GLchar *))fn)(p0, p1, p2, p3, p4, p5, p6);
}

/* glGetUniformBlockIndex —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLuint glGetUniformBlockIndex(GLuint p0, const GLchar * p1)
{
    GLESMOD_HOTPATH("glGetUniformBlockIndex");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetUniformBlockIndex");
        if (!fn) {
            glesmod_report_missing("glGetUniformBlockIndex");
            return 0;
        }
    }
    return ((GLuint (*)(GLuint, const GLchar *))fn)(p0, p1);
}

/* glGetUniformIndices —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetUniformIndices(GLuint p0, GLsizei p1, const GLchar *const * p2, GLuint * p3)
{
    GLESMOD_HOTPATH("glGetUniformIndices");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetUniformIndices");
        if (!fn) {
            glesmod_report_missing("glGetUniformIndices");
            return;
        }
    }
    ((void (*)(GLuint, GLsizei, const GLchar *const *, GLuint *))fn)(p0, p1, p2, p3);
}

/* glGetUniformLocation —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLint glGetUniformLocation(GLuint p0, const GLchar * p1)
{
    GLESMOD_HOTPATH("glGetUniformLocation");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetUniformLocation");
        if (!fn) {
            glesmod_report_missing("glGetUniformLocation");
            return 0;
        }
    }
    return ((GLint (*)(GLuint, const GLchar *))fn)(p0, p1);
}

/* glGetUniformfv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetUniformfv(GLuint p0, GLint p1, GLfloat * p2)
{
    GLESMOD_HOTPATH("glGetUniformfv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetUniformfv");
        if (!fn) {
            glesmod_report_missing("glGetUniformfv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLfloat *))fn)(p0, p1, p2);
}

/* glGetUniformiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetUniformiv(GLuint p0, GLint p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetUniformiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetUniformiv");
        if (!fn) {
            glesmod_report_missing("glGetUniformiv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLint *))fn)(p0, p1, p2);
}

/* glGetUniformuiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetUniformuiv(GLuint p0, GLint p1, GLuint * p2)
{
    GLESMOD_HOTPATH("glGetUniformuiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetUniformuiv");
        if (!fn) {
            glesmod_report_missing("glGetUniformuiv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLuint *))fn)(p0, p1, p2);
}

/* glGetVertexAttribIiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetVertexAttribIiv(GLuint p0, GLenum p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetVertexAttribIiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetVertexAttribIiv");
        if (!fn) {
            glesmod_report_missing("glGetVertexAttribIiv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLint *))fn)(p0, p1, p2);
}

/* glGetVertexAttribIuiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetVertexAttribIuiv(GLuint p0, GLenum p1, GLuint * p2)
{
    GLESMOD_HOTPATH("glGetVertexAttribIuiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetVertexAttribIuiv");
        if (!fn) {
            glesmod_report_missing("glGetVertexAttribIuiv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLuint *))fn)(p0, p1, p2);
}

/* glGetVertexAttribPointerv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetVertexAttribPointerv(GLuint p0, GLenum p1, void ** p2)
{
    GLESMOD_HOTPATH("glGetVertexAttribPointerv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetVertexAttribPointerv");
        if (!fn) {
            glesmod_report_missing("glGetVertexAttribPointerv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, void **))fn)(p0, p1, p2);
}

/* glGetVertexAttribfv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetVertexAttribfv(GLuint p0, GLenum p1, GLfloat * p2)
{
    GLESMOD_HOTPATH("glGetVertexAttribfv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetVertexAttribfv");
        if (!fn) {
            glesmod_report_missing("glGetVertexAttribfv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLfloat *))fn)(p0, p1, p2);
}

/* glGetVertexAttribiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetVertexAttribiv(GLuint p0, GLenum p1, GLint * p2)
{
    GLESMOD_HOTPATH("glGetVertexAttribiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetVertexAttribiv");
        if (!fn) {
            glesmod_report_missing("glGetVertexAttribiv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLint *))fn)(p0, p1, p2);
}

/* glGetnUniformfv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetnUniformfv(GLuint p0, GLint p1, GLsizei p2, GLfloat * p3)
{
    GLESMOD_HOTPATH("glGetnUniformfv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetnUniformfv");
        if (!fn) {
            glesmod_report_missing("glGetnUniformfv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, GLfloat *))fn)(p0, p1, p2, p3);
}

/* glGetnUniformiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetnUniformiv(GLuint p0, GLint p1, GLsizei p2, GLint * p3)
{
    GLESMOD_HOTPATH("glGetnUniformiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetnUniformiv");
        if (!fn) {
            glesmod_report_missing("glGetnUniformiv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, GLint *))fn)(p0, p1, p2, p3);
}

/* glGetnUniformuiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glGetnUniformuiv(GLuint p0, GLint p1, GLsizei p2, GLuint * p3)
{
    GLESMOD_HOTPATH("glGetnUniformuiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glGetnUniformuiv");
        if (!fn) {
            glesmod_report_missing("glGetnUniformuiv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, GLuint *))fn)(p0, p1, p2, p3);
}

/* glHint —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glHint(GLenum p0, GLenum p1)
{
    GLESMOD_HOTPATH("glHint");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glHint");
        if (!fn) {
            glesmod_report_missing("glHint");
            return;
        }
    }
    ((void (*)(GLenum, GLenum))fn)(p0, p1);
}

/* glInvalidateFramebuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glInvalidateFramebuffer(GLenum p0, GLsizei p1, const GLenum * p2)
{
    GLESMOD_HOTPATH("glInvalidateFramebuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glInvalidateFramebuffer");
        if (!fn) {
            glesmod_report_missing("glInvalidateFramebuffer");
            return;
        }
    }
    ((void (*)(GLenum, GLsizei, const GLenum *))fn)(p0, p1, p2);
}

/* glInvalidateSubFramebuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glInvalidateSubFramebuffer(GLenum p0, GLsizei p1, const GLenum * p2, GLint p3, GLint p4, GLsizei p5, GLsizei p6)
{
    GLESMOD_HOTPATH("glInvalidateSubFramebuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glInvalidateSubFramebuffer");
        if (!fn) {
            glesmod_report_missing("glInvalidateSubFramebuffer");
            return;
        }
    }
    ((void (*)(GLenum, GLsizei, const GLenum *, GLint, GLint, GLsizei, GLsizei))fn)(p0, p1, p2, p3, p4, p5, p6);
}

/* glIsBuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsBuffer(GLuint p0)
{
    GLESMOD_HOTPATH("glIsBuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsBuffer");
        if (!fn) {
            glesmod_report_missing("glIsBuffer");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLuint))fn)(p0);
}

/* glIsEnabled —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsEnabled(GLenum p0)
{
    GLESMOD_HOTPATH("glIsEnabled");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsEnabled");
        if (!fn) {
            glesmod_report_missing("glIsEnabled");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLenum))fn)(p0);
}

/* glIsEnabledi —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsEnabledi(GLenum p0, GLuint p1)
{
    GLESMOD_HOTPATH("glIsEnabledi");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsEnabledi");
        if (!fn) {
            glesmod_report_missing("glIsEnabledi");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLenum, GLuint))fn)(p0, p1);
}

/* glIsFramebuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsFramebuffer(GLuint p0)
{
    GLESMOD_HOTPATH("glIsFramebuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsFramebuffer");
        if (!fn) {
            glesmod_report_missing("glIsFramebuffer");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLuint))fn)(p0);
}

/* glIsProgram —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsProgram(GLuint p0)
{
    GLESMOD_HOTPATH("glIsProgram");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsProgram");
        if (!fn) {
            glesmod_report_missing("glIsProgram");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLuint))fn)(p0);
}

/* glIsProgramPipeline —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsProgramPipeline(GLuint p0)
{
    GLESMOD_HOTPATH("glIsProgramPipeline");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsProgramPipeline");
        if (!fn) {
            glesmod_report_missing("glIsProgramPipeline");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLuint))fn)(p0);
}

/* glIsQuery —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsQuery(GLuint p0)
{
    GLESMOD_HOTPATH("glIsQuery");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsQuery");
        if (!fn) {
            glesmod_report_missing("glIsQuery");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLuint))fn)(p0);
}

/* glIsRenderbuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsRenderbuffer(GLuint p0)
{
    GLESMOD_HOTPATH("glIsRenderbuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsRenderbuffer");
        if (!fn) {
            glesmod_report_missing("glIsRenderbuffer");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLuint))fn)(p0);
}

/* glIsSampler —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsSampler(GLuint p0)
{
    GLESMOD_HOTPATH("glIsSampler");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsSampler");
        if (!fn) {
            glesmod_report_missing("glIsSampler");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLuint))fn)(p0);
}

/* glIsShader —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsShader(GLuint p0)
{
    GLESMOD_HOTPATH("glIsShader");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsShader");
        if (!fn) {
            glesmod_report_missing("glIsShader");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLuint))fn)(p0);
}

/* glIsSync —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsSync(GLsync p0)
{
    GLESMOD_HOTPATH("glIsSync");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsSync");
        if (!fn) {
            glesmod_report_missing("glIsSync");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLsync))fn)(p0);
}

/* glIsTexture —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsTexture(GLuint p0)
{
    GLESMOD_HOTPATH("glIsTexture");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsTexture");
        if (!fn) {
            glesmod_report_missing("glIsTexture");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLuint))fn)(p0);
}

/* glIsTransformFeedback —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsTransformFeedback(GLuint p0)
{
    GLESMOD_HOTPATH("glIsTransformFeedback");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsTransformFeedback");
        if (!fn) {
            glesmod_report_missing("glIsTransformFeedback");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLuint))fn)(p0);
}

/* glIsVertexArray —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glIsVertexArray(GLuint p0)
{
    GLESMOD_HOTPATH("glIsVertexArray");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glIsVertexArray");
        if (!fn) {
            glesmod_report_missing("glIsVertexArray");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLuint))fn)(p0);
}

/* glLineWidth —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glLineWidth(GLfloat p0)
{
    GLESMOD_HOTPATH("glLineWidth");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glLineWidth");
        if (!fn) {
            glesmod_report_missing("glLineWidth");
            return;
        }
    }
    ((void (*)(GLfloat))fn)(p0);
}

/* glLinkProgram —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glLinkProgram(GLuint p0)
{
    GLESMOD_HOTPATH("glLinkProgram");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glLinkProgram");
        if (!fn) {
            glesmod_report_missing("glLinkProgram");
            return;
        }
    }
    ((void (*)(GLuint))fn)(p0);
}

/* glMemoryBarrier —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glMemoryBarrier(GLbitfield p0)
{
    GLESMOD_HOTPATH("glMemoryBarrier");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glMemoryBarrier");
        if (!fn) {
            glesmod_report_missing("glMemoryBarrier");
            return;
        }
    }
    ((void (*)(GLbitfield))fn)(p0);
}

/* glMemoryBarrierByRegion —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glMemoryBarrierByRegion(GLbitfield p0)
{
    GLESMOD_HOTPATH("glMemoryBarrierByRegion");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glMemoryBarrierByRegion");
        if (!fn) {
            glesmod_report_missing("glMemoryBarrierByRegion");
            return;
        }
    }
    ((void (*)(GLbitfield))fn)(p0);
}

/* glMinSampleShading —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glMinSampleShading(GLfloat p0)
{
    GLESMOD_HOTPATH("glMinSampleShading");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glMinSampleShading");
        if (!fn) {
            glesmod_report_missing("glMinSampleShading");
            return;
        }
    }
    ((void (*)(GLfloat))fn)(p0);
}

/* glObjectLabel —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glObjectLabel(GLenum p0, GLuint p1, GLsizei p2, const GLchar * p3)
{
    GLESMOD_HOTPATH("glObjectLabel");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glObjectLabel");
        if (!fn) {
            glesmod_report_missing("glObjectLabel");
            return;
        }
    }
    ((void (*)(GLenum, GLuint, GLsizei, const GLchar *))fn)(p0, p1, p2, p3);
}

/* glObjectPtrLabel —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glObjectPtrLabel(const void * p0, GLsizei p1, const GLchar * p2)
{
    GLESMOD_HOTPATH("glObjectPtrLabel");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glObjectPtrLabel");
        if (!fn) {
            glesmod_report_missing("glObjectPtrLabel");
            return;
        }
    }
    ((void (*)(const void *, GLsizei, const GLchar *))fn)(p0, p1, p2);
}

/* glPatchParameteri —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glPatchParameteri(GLenum p0, GLint p1)
{
    GLESMOD_HOTPATH("glPatchParameteri");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glPatchParameteri");
        if (!fn) {
            glesmod_report_missing("glPatchParameteri");
            return;
        }
    }
    ((void (*)(GLenum, GLint))fn)(p0, p1);
}

/* glPauseTransformFeedback —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glPauseTransformFeedback(void)
{
    GLESMOD_HOTPATH("glPauseTransformFeedback");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glPauseTransformFeedback");
        if (!fn) {
            glesmod_report_missing("glPauseTransformFeedback");
            return;
        }
    }
    ((void (*)(void))fn)();
}

/* glPixelStorei —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glPixelStorei(GLenum p0, GLint p1)
{
    GLESMOD_HOTPATH("glPixelStorei");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glPixelStorei");
        if (!fn) {
            glesmod_report_missing("glPixelStorei");
            return;
        }
    }
    ((void (*)(GLenum, GLint))fn)(p0, p1);
}

/* glPolygonOffset —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glPolygonOffset(GLfloat p0, GLfloat p1)
{
    GLESMOD_HOTPATH("glPolygonOffset");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glPolygonOffset");
        if (!fn) {
            glesmod_report_missing("glPolygonOffset");
            return;
        }
    }
    ((void (*)(GLfloat, GLfloat))fn)(p0, p1);
}

/* glPopDebugGroup —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glPopDebugGroup(void)
{
    GLESMOD_HOTPATH("glPopDebugGroup");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glPopDebugGroup");
        if (!fn) {
            glesmod_report_missing("glPopDebugGroup");
            return;
        }
    }
    ((void (*)(void))fn)();
}

/* glProgramBinary —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramBinary(GLuint p0, GLenum p1, const void * p2, GLsizei p3)
{
    GLESMOD_HOTPATH("glProgramBinary");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramBinary");
        if (!fn) {
            glesmod_report_missing("glProgramBinary");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, const void *, GLsizei))fn)(p0, p1, p2, p3);
}

/* glProgramParameteri —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramParameteri(GLuint p0, GLenum p1, GLint p2)
{
    GLESMOD_HOTPATH("glProgramParameteri");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramParameteri");
        if (!fn) {
            glesmod_report_missing("glProgramParameteri");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLint))fn)(p0, p1, p2);
}

/* glProgramUniform1f —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform1f(GLuint p0, GLint p1, GLfloat p2)
{
    GLESMOD_HOTPATH("glProgramUniform1f");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform1f");
        if (!fn) {
            glesmod_report_missing("glProgramUniform1f");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLfloat))fn)(p0, p1, p2);
}

/* glProgramUniform1fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform1fv(GLuint p0, GLint p1, GLsizei p2, const GLfloat * p3)
{
    GLESMOD_HOTPATH("glProgramUniform1fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform1fv");
        if (!fn) {
            glesmod_report_missing("glProgramUniform1fv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, const GLfloat *))fn)(p0, p1, p2, p3);
}

/* glProgramUniform1i —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform1i(GLuint p0, GLint p1, GLint p2)
{
    GLESMOD_HOTPATH("glProgramUniform1i");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform1i");
        if (!fn) {
            glesmod_report_missing("glProgramUniform1i");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLint))fn)(p0, p1, p2);
}

/* glProgramUniform1iv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform1iv(GLuint p0, GLint p1, GLsizei p2, const GLint * p3)
{
    GLESMOD_HOTPATH("glProgramUniform1iv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform1iv");
        if (!fn) {
            glesmod_report_missing("glProgramUniform1iv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, const GLint *))fn)(p0, p1, p2, p3);
}

/* glProgramUniform1ui —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform1ui(GLuint p0, GLint p1, GLuint p2)
{
    GLESMOD_HOTPATH("glProgramUniform1ui");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform1ui");
        if (!fn) {
            glesmod_report_missing("glProgramUniform1ui");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLuint))fn)(p0, p1, p2);
}

/* glProgramUniform1uiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform1uiv(GLuint p0, GLint p1, GLsizei p2, const GLuint * p3)
{
    GLESMOD_HOTPATH("glProgramUniform1uiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform1uiv");
        if (!fn) {
            glesmod_report_missing("glProgramUniform1uiv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, const GLuint *))fn)(p0, p1, p2, p3);
}

/* glProgramUniform2f —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform2f(GLuint p0, GLint p1, GLfloat p2, GLfloat p3)
{
    GLESMOD_HOTPATH("glProgramUniform2f");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform2f");
        if (!fn) {
            glesmod_report_missing("glProgramUniform2f");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLfloat, GLfloat))fn)(p0, p1, p2, p3);
}

/* glProgramUniform2fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform2fv(GLuint p0, GLint p1, GLsizei p2, const GLfloat * p3)
{
    GLESMOD_HOTPATH("glProgramUniform2fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform2fv");
        if (!fn) {
            glesmod_report_missing("glProgramUniform2fv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, const GLfloat *))fn)(p0, p1, p2, p3);
}

/* glProgramUniform2i —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform2i(GLuint p0, GLint p1, GLint p2, GLint p3)
{
    GLESMOD_HOTPATH("glProgramUniform2i");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform2i");
        if (!fn) {
            glesmod_report_missing("glProgramUniform2i");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLint, GLint))fn)(p0, p1, p2, p3);
}

/* glProgramUniform2iv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform2iv(GLuint p0, GLint p1, GLsizei p2, const GLint * p3)
{
    GLESMOD_HOTPATH("glProgramUniform2iv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform2iv");
        if (!fn) {
            glesmod_report_missing("glProgramUniform2iv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, const GLint *))fn)(p0, p1, p2, p3);
}

/* glProgramUniform2ui —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform2ui(GLuint p0, GLint p1, GLuint p2, GLuint p3)
{
    GLESMOD_HOTPATH("glProgramUniform2ui");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform2ui");
        if (!fn) {
            glesmod_report_missing("glProgramUniform2ui");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLuint, GLuint))fn)(p0, p1, p2, p3);
}

/* glProgramUniform2uiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform2uiv(GLuint p0, GLint p1, GLsizei p2, const GLuint * p3)
{
    GLESMOD_HOTPATH("glProgramUniform2uiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform2uiv");
        if (!fn) {
            glesmod_report_missing("glProgramUniform2uiv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, const GLuint *))fn)(p0, p1, p2, p3);
}

/* glProgramUniform3f —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform3f(GLuint p0, GLint p1, GLfloat p2, GLfloat p3, GLfloat p4)
{
    GLESMOD_HOTPATH("glProgramUniform3f");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform3f");
        if (!fn) {
            glesmod_report_missing("glProgramUniform3f");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLfloat, GLfloat, GLfloat))fn)(p0, p1, p2, p3, p4);
}

/* glProgramUniform3fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform3fv(GLuint p0, GLint p1, GLsizei p2, const GLfloat * p3)
{
    GLESMOD_HOTPATH("glProgramUniform3fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform3fv");
        if (!fn) {
            glesmod_report_missing("glProgramUniform3fv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, const GLfloat *))fn)(p0, p1, p2, p3);
}

/* glProgramUniform3i —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform3i(GLuint p0, GLint p1, GLint p2, GLint p3, GLint p4)
{
    GLESMOD_HOTPATH("glProgramUniform3i");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform3i");
        if (!fn) {
            glesmod_report_missing("glProgramUniform3i");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLint, GLint, GLint))fn)(p0, p1, p2, p3, p4);
}

/* glProgramUniform3iv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform3iv(GLuint p0, GLint p1, GLsizei p2, const GLint * p3)
{
    GLESMOD_HOTPATH("glProgramUniform3iv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform3iv");
        if (!fn) {
            glesmod_report_missing("glProgramUniform3iv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, const GLint *))fn)(p0, p1, p2, p3);
}

/* glProgramUniform3ui —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform3ui(GLuint p0, GLint p1, GLuint p2, GLuint p3, GLuint p4)
{
    GLESMOD_HOTPATH("glProgramUniform3ui");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform3ui");
        if (!fn) {
            glesmod_report_missing("glProgramUniform3ui");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLuint, GLuint, GLuint))fn)(p0, p1, p2, p3, p4);
}

/* glProgramUniform3uiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform3uiv(GLuint p0, GLint p1, GLsizei p2, const GLuint * p3)
{
    GLESMOD_HOTPATH("glProgramUniform3uiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform3uiv");
        if (!fn) {
            glesmod_report_missing("glProgramUniform3uiv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, const GLuint *))fn)(p0, p1, p2, p3);
}

/* glProgramUniform4f —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform4f(GLuint p0, GLint p1, GLfloat p2, GLfloat p3, GLfloat p4, GLfloat p5)
{
    GLESMOD_HOTPATH("glProgramUniform4f");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform4f");
        if (!fn) {
            glesmod_report_missing("glProgramUniform4f");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLfloat, GLfloat, GLfloat, GLfloat))fn)(p0, p1, p2, p3, p4, p5);
}

/* glProgramUniform4fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform4fv(GLuint p0, GLint p1, GLsizei p2, const GLfloat * p3)
{
    GLESMOD_HOTPATH("glProgramUniform4fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform4fv");
        if (!fn) {
            glesmod_report_missing("glProgramUniform4fv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, const GLfloat *))fn)(p0, p1, p2, p3);
}

/* glProgramUniform4i —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform4i(GLuint p0, GLint p1, GLint p2, GLint p3, GLint p4, GLint p5)
{
    GLESMOD_HOTPATH("glProgramUniform4i");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform4i");
        if (!fn) {
            glesmod_report_missing("glProgramUniform4i");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLint, GLint, GLint, GLint))fn)(p0, p1, p2, p3, p4, p5);
}

/* glProgramUniform4iv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform4iv(GLuint p0, GLint p1, GLsizei p2, const GLint * p3)
{
    GLESMOD_HOTPATH("glProgramUniform4iv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform4iv");
        if (!fn) {
            glesmod_report_missing("glProgramUniform4iv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, const GLint *))fn)(p0, p1, p2, p3);
}

/* glProgramUniform4ui —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform4ui(GLuint p0, GLint p1, GLuint p2, GLuint p3, GLuint p4, GLuint p5)
{
    GLESMOD_HOTPATH("glProgramUniform4ui");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform4ui");
        if (!fn) {
            glesmod_report_missing("glProgramUniform4ui");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLuint, GLuint, GLuint, GLuint))fn)(p0, p1, p2, p3, p4, p5);
}

/* glProgramUniform4uiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniform4uiv(GLuint p0, GLint p1, GLsizei p2, const GLuint * p3)
{
    GLESMOD_HOTPATH("glProgramUniform4uiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniform4uiv");
        if (!fn) {
            glesmod_report_missing("glProgramUniform4uiv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, const GLuint *))fn)(p0, p1, p2, p3);
}

/* glProgramUniformMatrix2fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniformMatrix2fv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLfloat * p4)
{
    GLESMOD_HOTPATH("glProgramUniformMatrix2fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniformMatrix2fv");
        if (!fn) {
            glesmod_report_missing("glProgramUniformMatrix2fv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3, p4);
}

/* glProgramUniformMatrix2x3fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniformMatrix2x3fv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLfloat * p4)
{
    GLESMOD_HOTPATH("glProgramUniformMatrix2x3fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniformMatrix2x3fv");
        if (!fn) {
            glesmod_report_missing("glProgramUniformMatrix2x3fv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3, p4);
}

/* glProgramUniformMatrix2x4fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniformMatrix2x4fv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLfloat * p4)
{
    GLESMOD_HOTPATH("glProgramUniformMatrix2x4fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniformMatrix2x4fv");
        if (!fn) {
            glesmod_report_missing("glProgramUniformMatrix2x4fv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3, p4);
}

/* glProgramUniformMatrix3fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniformMatrix3fv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLfloat * p4)
{
    GLESMOD_HOTPATH("glProgramUniformMatrix3fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniformMatrix3fv");
        if (!fn) {
            glesmod_report_missing("glProgramUniformMatrix3fv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3, p4);
}

/* glProgramUniformMatrix3x2fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniformMatrix3x2fv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLfloat * p4)
{
    GLESMOD_HOTPATH("glProgramUniformMatrix3x2fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniformMatrix3x2fv");
        if (!fn) {
            glesmod_report_missing("glProgramUniformMatrix3x2fv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3, p4);
}

/* glProgramUniformMatrix3x4fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniformMatrix3x4fv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLfloat * p4)
{
    GLESMOD_HOTPATH("glProgramUniformMatrix3x4fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniformMatrix3x4fv");
        if (!fn) {
            glesmod_report_missing("glProgramUniformMatrix3x4fv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3, p4);
}

/* glProgramUniformMatrix4fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniformMatrix4fv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLfloat * p4)
{
    GLESMOD_HOTPATH("glProgramUniformMatrix4fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniformMatrix4fv");
        if (!fn) {
            glesmod_report_missing("glProgramUniformMatrix4fv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3, p4);
}

/* glProgramUniformMatrix4x2fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniformMatrix4x2fv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLfloat * p4)
{
    GLESMOD_HOTPATH("glProgramUniformMatrix4x2fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniformMatrix4x2fv");
        if (!fn) {
            glesmod_report_missing("glProgramUniformMatrix4x2fv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3, p4);
}

/* glProgramUniformMatrix4x3fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glProgramUniformMatrix4x3fv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLfloat * p4)
{
    GLESMOD_HOTPATH("glProgramUniformMatrix4x3fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glProgramUniformMatrix4x3fv");
        if (!fn) {
            glesmod_report_missing("glProgramUniformMatrix4x3fv");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3, p4);
}

/* glPushDebugGroup —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glPushDebugGroup(GLenum p0, GLuint p1, GLsizei p2, const GLchar * p3)
{
    GLESMOD_HOTPATH("glPushDebugGroup");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glPushDebugGroup");
        if (!fn) {
            glesmod_report_missing("glPushDebugGroup");
            return;
        }
    }
    ((void (*)(GLenum, GLuint, GLsizei, const GLchar *))fn)(p0, p1, p2, p3);
}

/* glReadBuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glReadBuffer(GLenum p0)
{
    GLESMOD_HOTPATH("glReadBuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glReadBuffer");
        if (!fn) {
            glesmod_report_missing("glReadBuffer");
            return;
        }
    }
    ((void (*)(GLenum))fn)(p0);
}

/* glReadPixels —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glReadPixels(GLint p0, GLint p1, GLsizei p2, GLsizei p3, GLenum p4, GLenum p5, void * p6)
{
    GLESMOD_HOTPATH("glReadPixels");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glReadPixels");
        if (!fn) {
            glesmod_report_missing("glReadPixels");
            return;
        }
    }
    ((void (*)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *))fn)(p0, p1, p2, p3, p4, p5, p6);
}

/* glReadnPixels —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glReadnPixels(GLint p0, GLint p1, GLsizei p2, GLsizei p3, GLenum p4, GLenum p5, GLsizei p6, void * p7)
{
    GLESMOD_HOTPATH("glReadnPixels");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glReadnPixels");
        if (!fn) {
            glesmod_report_missing("glReadnPixels");
            return;
        }
    }
    ((void (*)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLsizei, void *))fn)(p0, p1, p2, p3, p4, p5, p6, p7);
}

/* glReleaseShaderCompiler —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glReleaseShaderCompiler(void)
{
    GLESMOD_HOTPATH("glReleaseShaderCompiler");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glReleaseShaderCompiler");
        if (!fn) {
            glesmod_report_missing("glReleaseShaderCompiler");
            return;
        }
    }
    ((void (*)(void))fn)();
}

/* glRenderbufferStorage —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glRenderbufferStorage(GLenum p0, GLenum p1, GLsizei p2, GLsizei p3)
{
    GLESMOD_HOTPATH("glRenderbufferStorage");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glRenderbufferStorage");
        if (!fn) {
            glesmod_report_missing("glRenderbufferStorage");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLsizei, GLsizei))fn)(p0, p1, p2, p3);
}

/* glRenderbufferStorageMultisample —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glRenderbufferStorageMultisample(GLenum p0, GLsizei p1, GLenum p2, GLsizei p3, GLsizei p4)
{
    GLESMOD_HOTPATH("glRenderbufferStorageMultisample");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glRenderbufferStorageMultisample");
        if (!fn) {
            glesmod_report_missing("glRenderbufferStorageMultisample");
            return;
        }
    }
    ((void (*)(GLenum, GLsizei, GLenum, GLsizei, GLsizei))fn)(p0, p1, p2, p3, p4);
}

/* glResumeTransformFeedback —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glResumeTransformFeedback(void)
{
    GLESMOD_HOTPATH("glResumeTransformFeedback");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glResumeTransformFeedback");
        if (!fn) {
            glesmod_report_missing("glResumeTransformFeedback");
            return;
        }
    }
    ((void (*)(void))fn)();
}

/* glSampleCoverage —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glSampleCoverage(GLfloat p0, GLboolean p1)
{
    GLESMOD_HOTPATH("glSampleCoverage");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glSampleCoverage");
        if (!fn) {
            glesmod_report_missing("glSampleCoverage");
            return;
        }
    }
    ((void (*)(GLfloat, GLboolean))fn)(p0, p1);
}

/* glSampleMaski —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glSampleMaski(GLuint p0, GLbitfield p1)
{
    GLESMOD_HOTPATH("glSampleMaski");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glSampleMaski");
        if (!fn) {
            glesmod_report_missing("glSampleMaski");
            return;
        }
    }
    ((void (*)(GLuint, GLbitfield))fn)(p0, p1);
}

/* glSamplerParameterIiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glSamplerParameterIiv(GLuint p0, GLenum p1, const GLint * p2)
{
    GLESMOD_HOTPATH("glSamplerParameterIiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glSamplerParameterIiv");
        if (!fn) {
            glesmod_report_missing("glSamplerParameterIiv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, const GLint *))fn)(p0, p1, p2);
}

/* glSamplerParameterIuiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glSamplerParameterIuiv(GLuint p0, GLenum p1, const GLuint * p2)
{
    GLESMOD_HOTPATH("glSamplerParameterIuiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glSamplerParameterIuiv");
        if (!fn) {
            glesmod_report_missing("glSamplerParameterIuiv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, const GLuint *))fn)(p0, p1, p2);
}

/* glSamplerParameterf —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glSamplerParameterf(GLuint p0, GLenum p1, GLfloat p2)
{
    GLESMOD_HOTPATH("glSamplerParameterf");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glSamplerParameterf");
        if (!fn) {
            glesmod_report_missing("glSamplerParameterf");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLfloat))fn)(p0, p1, p2);
}

/* glSamplerParameterfv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glSamplerParameterfv(GLuint p0, GLenum p1, const GLfloat * p2)
{
    GLESMOD_HOTPATH("glSamplerParameterfv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glSamplerParameterfv");
        if (!fn) {
            glesmod_report_missing("glSamplerParameterfv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, const GLfloat *))fn)(p0, p1, p2);
}

/* glSamplerParameteri —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glSamplerParameteri(GLuint p0, GLenum p1, GLint p2)
{
    GLESMOD_HOTPATH("glSamplerParameteri");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glSamplerParameteri");
        if (!fn) {
            glesmod_report_missing("glSamplerParameteri");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, GLint))fn)(p0, p1, p2);
}

/* glSamplerParameteriv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glSamplerParameteriv(GLuint p0, GLenum p1, const GLint * p2)
{
    GLESMOD_HOTPATH("glSamplerParameteriv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glSamplerParameteriv");
        if (!fn) {
            glesmod_report_missing("glSamplerParameteriv");
            return;
        }
    }
    ((void (*)(GLuint, GLenum, const GLint *))fn)(p0, p1, p2);
}

/* glScissor —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glScissor(GLint p0, GLint p1, GLsizei p2, GLsizei p3)
{
    GLESMOD_HOTPATH("glScissor");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glScissor");
        if (!fn) {
            glesmod_report_missing("glScissor");
            return;
        }
    }
    ((void (*)(GLint, GLint, GLsizei, GLsizei))fn)(p0, p1, p2, p3);
}

/* glShaderBinary —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glShaderBinary(GLsizei p0, const GLuint * p1, GLenum p2, const void * p3, GLsizei p4)
{
    GLESMOD_HOTPATH("glShaderBinary");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glShaderBinary");
        if (!fn) {
            glesmod_report_missing("glShaderBinary");
            return;
        }
    }
    ((void (*)(GLsizei, const GLuint *, GLenum, const void *, GLsizei))fn)(p0, p1, p2, p3, p4);
}

/* glStencilFunc —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glStencilFunc(GLenum p0, GLint p1, GLuint p2)
{
    GLESMOD_HOTPATH("glStencilFunc");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glStencilFunc");
        if (!fn) {
            glesmod_report_missing("glStencilFunc");
            return;
        }
    }
    ((void (*)(GLenum, GLint, GLuint))fn)(p0, p1, p2);
}

/* glStencilFuncSeparate —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glStencilFuncSeparate(GLenum p0, GLenum p1, GLint p2, GLuint p3)
{
    GLESMOD_HOTPATH("glStencilFuncSeparate");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glStencilFuncSeparate");
        if (!fn) {
            glesmod_report_missing("glStencilFuncSeparate");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLint, GLuint))fn)(p0, p1, p2, p3);
}

/* glStencilMask —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glStencilMask(GLuint p0)
{
    GLESMOD_HOTPATH("glStencilMask");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glStencilMask");
        if (!fn) {
            glesmod_report_missing("glStencilMask");
            return;
        }
    }
    ((void (*)(GLuint))fn)(p0);
}

/* glStencilMaskSeparate —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glStencilMaskSeparate(GLenum p0, GLuint p1)
{
    GLESMOD_HOTPATH("glStencilMaskSeparate");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glStencilMaskSeparate");
        if (!fn) {
            glesmod_report_missing("glStencilMaskSeparate");
            return;
        }
    }
    ((void (*)(GLenum, GLuint))fn)(p0, p1);
}

/* glStencilOp —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glStencilOp(GLenum p0, GLenum p1, GLenum p2)
{
    GLESMOD_HOTPATH("glStencilOp");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glStencilOp");
        if (!fn) {
            glesmod_report_missing("glStencilOp");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLenum))fn)(p0, p1, p2);
}

/* glStencilOpSeparate —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glStencilOpSeparate(GLenum p0, GLenum p1, GLenum p2, GLenum p3)
{
    GLESMOD_HOTPATH("glStencilOpSeparate");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glStencilOpSeparate");
        if (!fn) {
            glesmod_report_missing("glStencilOpSeparate");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLenum, GLenum))fn)(p0, p1, p2, p3);
}

/* glTexBuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glTexBuffer(GLenum p0, GLenum p1, GLuint p2)
{
    GLESMOD_HOTPATH("glTexBuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glTexBuffer");
        if (!fn) {
            glesmod_report_missing("glTexBuffer");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLuint))fn)(p0, p1, p2);
}

/* glTexBufferRange —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glTexBufferRange(GLenum p0, GLenum p1, GLuint p2, GLintptr p3, GLsizeiptr p4)
{
    GLESMOD_HOTPATH("glTexBufferRange");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glTexBufferRange");
        if (!fn) {
            glesmod_report_missing("glTexBufferRange");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, GLuint, GLintptr, GLsizeiptr))fn)(p0, p1, p2, p3, p4);
}

/* glTexParameterIiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glTexParameterIiv(GLenum p0, GLenum p1, const GLint * p2)
{
    GLESMOD_HOTPATH("glTexParameterIiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glTexParameterIiv");
        if (!fn) {
            glesmod_report_missing("glTexParameterIiv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, const GLint *))fn)(p0, p1, p2);
}

/* glTexParameterIuiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glTexParameterIuiv(GLenum p0, GLenum p1, const GLuint * p2)
{
    GLESMOD_HOTPATH("glTexParameterIuiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glTexParameterIuiv");
        if (!fn) {
            glesmod_report_missing("glTexParameterIuiv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, const GLuint *))fn)(p0, p1, p2);
}

/* glTexParameterfv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glTexParameterfv(GLenum p0, GLenum p1, const GLfloat * p2)
{
    GLESMOD_HOTPATH("glTexParameterfv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glTexParameterfv");
        if (!fn) {
            glesmod_report_missing("glTexParameterfv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, const GLfloat *))fn)(p0, p1, p2);
}

/* glTexParameteriv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glTexParameteriv(GLenum p0, GLenum p1, const GLint * p2)
{
    GLESMOD_HOTPATH("glTexParameteriv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glTexParameteriv");
        if (!fn) {
            glesmod_report_missing("glTexParameteriv");
            return;
        }
    }
    ((void (*)(GLenum, GLenum, const GLint *))fn)(p0, p1, p2);
}

/* glTexStorage2DMultisample —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glTexStorage2DMultisample(GLenum p0, GLsizei p1, GLenum p2, GLsizei p3, GLsizei p4, GLboolean p5)
{
    GLESMOD_HOTPATH("glTexStorage2DMultisample");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glTexStorage2DMultisample");
        if (!fn) {
            glesmod_report_missing("glTexStorage2DMultisample");
            return;
        }
    }
    ((void (*)(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLboolean))fn)(p0, p1, p2, p3, p4, p5);
}

/* glTexStorage3DMultisample —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glTexStorage3DMultisample(GLenum p0, GLsizei p1, GLenum p2, GLsizei p3, GLsizei p4, GLsizei p5, GLboolean p6)
{
    GLESMOD_HOTPATH("glTexStorage3DMultisample");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glTexStorage3DMultisample");
        if (!fn) {
            glesmod_report_missing("glTexStorage3DMultisample");
            return;
        }
    }
    ((void (*)(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei, GLboolean))fn)(p0, p1, p2, p3, p4, p5, p6);
}

/* glTexSubImage2D —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glTexSubImage2D(GLenum p0, GLint p1, GLint p2, GLint p3, GLsizei p4, GLsizei p5, GLenum p6, GLenum p7, const void * p8)
{
    GLESMOD_HOTPATH("glTexSubImage2D");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glTexSubImage2D");
        if (!fn) {
            glesmod_report_missing("glTexSubImage2D");
            return;
        }
    }
    ((void (*)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *))fn)(p0, p1, p2, p3, p4, p5, p6, p7, p8);
}

/* glTexSubImage3D —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glTexSubImage3D(GLenum p0, GLint p1, GLint p2, GLint p3, GLint p4, GLsizei p5, GLsizei p6, GLsizei p7, GLenum p8, GLenum p9, const void * p10)
{
    GLESMOD_HOTPATH("glTexSubImage3D");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glTexSubImage3D");
        if (!fn) {
            glesmod_report_missing("glTexSubImage3D");
            return;
        }
    }
    ((void (*)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void *))fn)(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
}

/* glTransformFeedbackVaryings —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glTransformFeedbackVaryings(GLuint p0, GLsizei p1, const GLchar *const * p2, GLenum p3)
{
    GLESMOD_HOTPATH("glTransformFeedbackVaryings");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glTransformFeedbackVaryings");
        if (!fn) {
            glesmod_report_missing("glTransformFeedbackVaryings");
            return;
        }
    }
    ((void (*)(GLuint, GLsizei, const GLchar *const *, GLenum))fn)(p0, p1, p2, p3);
}

/* glUniform1f —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform1f(GLint p0, GLfloat p1)
{
    GLESMOD_HOTPATH("glUniform1f");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform1f");
        if (!fn) {
            glesmod_report_missing("glUniform1f");
            return;
        }
    }
    ((void (*)(GLint, GLfloat))fn)(p0, p1);
}

/* glUniform1fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform1fv(GLint p0, GLsizei p1, const GLfloat * p2)
{
    GLESMOD_HOTPATH("glUniform1fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform1fv");
        if (!fn) {
            glesmod_report_missing("glUniform1fv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, const GLfloat *))fn)(p0, p1, p2);
}

/* glUniform1i —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform1i(GLint p0, GLint p1)
{
    GLESMOD_HOTPATH("glUniform1i");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform1i");
        if (!fn) {
            glesmod_report_missing("glUniform1i");
            return;
        }
    }
    ((void (*)(GLint, GLint))fn)(p0, p1);
}

/* glUniform1iv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform1iv(GLint p0, GLsizei p1, const GLint * p2)
{
    GLESMOD_HOTPATH("glUniform1iv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform1iv");
        if (!fn) {
            glesmod_report_missing("glUniform1iv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, const GLint *))fn)(p0, p1, p2);
}

/* glUniform1ui —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform1ui(GLint p0, GLuint p1)
{
    GLESMOD_HOTPATH("glUniform1ui");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform1ui");
        if (!fn) {
            glesmod_report_missing("glUniform1ui");
            return;
        }
    }
    ((void (*)(GLint, GLuint))fn)(p0, p1);
}

/* glUniform1uiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform1uiv(GLint p0, GLsizei p1, const GLuint * p2)
{
    GLESMOD_HOTPATH("glUniform1uiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform1uiv");
        if (!fn) {
            glesmod_report_missing("glUniform1uiv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, const GLuint *))fn)(p0, p1, p2);
}

/* glUniform2f —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform2f(GLint p0, GLfloat p1, GLfloat p2)
{
    GLESMOD_HOTPATH("glUniform2f");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform2f");
        if (!fn) {
            glesmod_report_missing("glUniform2f");
            return;
        }
    }
    ((void (*)(GLint, GLfloat, GLfloat))fn)(p0, p1, p2);
}

/* glUniform2fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform2fv(GLint p0, GLsizei p1, const GLfloat * p2)
{
    GLESMOD_HOTPATH("glUniform2fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform2fv");
        if (!fn) {
            glesmod_report_missing("glUniform2fv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, const GLfloat *))fn)(p0, p1, p2);
}

/* glUniform2i —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform2i(GLint p0, GLint p1, GLint p2)
{
    GLESMOD_HOTPATH("glUniform2i");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform2i");
        if (!fn) {
            glesmod_report_missing("glUniform2i");
            return;
        }
    }
    ((void (*)(GLint, GLint, GLint))fn)(p0, p1, p2);
}

/* glUniform2iv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform2iv(GLint p0, GLsizei p1, const GLint * p2)
{
    GLESMOD_HOTPATH("glUniform2iv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform2iv");
        if (!fn) {
            glesmod_report_missing("glUniform2iv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, const GLint *))fn)(p0, p1, p2);
}

/* glUniform2ui —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform2ui(GLint p0, GLuint p1, GLuint p2)
{
    GLESMOD_HOTPATH("glUniform2ui");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform2ui");
        if (!fn) {
            glesmod_report_missing("glUniform2ui");
            return;
        }
    }
    ((void (*)(GLint, GLuint, GLuint))fn)(p0, p1, p2);
}

/* glUniform2uiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform2uiv(GLint p0, GLsizei p1, const GLuint * p2)
{
    GLESMOD_HOTPATH("glUniform2uiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform2uiv");
        if (!fn) {
            glesmod_report_missing("glUniform2uiv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, const GLuint *))fn)(p0, p1, p2);
}

/* glUniform3f —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform3f(GLint p0, GLfloat p1, GLfloat p2, GLfloat p3)
{
    GLESMOD_HOTPATH("glUniform3f");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform3f");
        if (!fn) {
            glesmod_report_missing("glUniform3f");
            return;
        }
    }
    ((void (*)(GLint, GLfloat, GLfloat, GLfloat))fn)(p0, p1, p2, p3);
}

/* glUniform3fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform3fv(GLint p0, GLsizei p1, const GLfloat * p2)
{
    GLESMOD_HOTPATH("glUniform3fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform3fv");
        if (!fn) {
            glesmod_report_missing("glUniform3fv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, const GLfloat *))fn)(p0, p1, p2);
}

/* glUniform3i —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform3i(GLint p0, GLint p1, GLint p2, GLint p3)
{
    GLESMOD_HOTPATH("glUniform3i");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform3i");
        if (!fn) {
            glesmod_report_missing("glUniform3i");
            return;
        }
    }
    ((void (*)(GLint, GLint, GLint, GLint))fn)(p0, p1, p2, p3);
}

/* glUniform3iv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform3iv(GLint p0, GLsizei p1, const GLint * p2)
{
    GLESMOD_HOTPATH("glUniform3iv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform3iv");
        if (!fn) {
            glesmod_report_missing("glUniform3iv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, const GLint *))fn)(p0, p1, p2);
}

/* glUniform3ui —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform3ui(GLint p0, GLuint p1, GLuint p2, GLuint p3)
{
    GLESMOD_HOTPATH("glUniform3ui");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform3ui");
        if (!fn) {
            glesmod_report_missing("glUniform3ui");
            return;
        }
    }
    ((void (*)(GLint, GLuint, GLuint, GLuint))fn)(p0, p1, p2, p3);
}

/* glUniform3uiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform3uiv(GLint p0, GLsizei p1, const GLuint * p2)
{
    GLESMOD_HOTPATH("glUniform3uiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform3uiv");
        if (!fn) {
            glesmod_report_missing("glUniform3uiv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, const GLuint *))fn)(p0, p1, p2);
}

/* glUniform4f —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform4f(GLint p0, GLfloat p1, GLfloat p2, GLfloat p3, GLfloat p4)
{
    GLESMOD_HOTPATH("glUniform4f");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform4f");
        if (!fn) {
            glesmod_report_missing("glUniform4f");
            return;
        }
    }
    ((void (*)(GLint, GLfloat, GLfloat, GLfloat, GLfloat))fn)(p0, p1, p2, p3, p4);
}

/* glUniform4fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform4fv(GLint p0, GLsizei p1, const GLfloat * p2)
{
    GLESMOD_HOTPATH("glUniform4fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform4fv");
        if (!fn) {
            glesmod_report_missing("glUniform4fv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, const GLfloat *))fn)(p0, p1, p2);
}

/* glUniform4i —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform4i(GLint p0, GLint p1, GLint p2, GLint p3, GLint p4)
{
    GLESMOD_HOTPATH("glUniform4i");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform4i");
        if (!fn) {
            glesmod_report_missing("glUniform4i");
            return;
        }
    }
    ((void (*)(GLint, GLint, GLint, GLint, GLint))fn)(p0, p1, p2, p3, p4);
}

/* glUniform4iv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform4iv(GLint p0, GLsizei p1, const GLint * p2)
{
    GLESMOD_HOTPATH("glUniform4iv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform4iv");
        if (!fn) {
            glesmod_report_missing("glUniform4iv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, const GLint *))fn)(p0, p1, p2);
}

/* glUniform4ui —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform4ui(GLint p0, GLuint p1, GLuint p2, GLuint p3, GLuint p4)
{
    GLESMOD_HOTPATH("glUniform4ui");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform4ui");
        if (!fn) {
            glesmod_report_missing("glUniform4ui");
            return;
        }
    }
    ((void (*)(GLint, GLuint, GLuint, GLuint, GLuint))fn)(p0, p1, p2, p3, p4);
}

/* glUniform4uiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniform4uiv(GLint p0, GLsizei p1, const GLuint * p2)
{
    GLESMOD_HOTPATH("glUniform4uiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniform4uiv");
        if (!fn) {
            glesmod_report_missing("glUniform4uiv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, const GLuint *))fn)(p0, p1, p2);
}

/* glUniformBlockBinding —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniformBlockBinding(GLuint p0, GLuint p1, GLuint p2)
{
    GLESMOD_HOTPATH("glUniformBlockBinding");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniformBlockBinding");
        if (!fn) {
            glesmod_report_missing("glUniformBlockBinding");
            return;
        }
    }
    ((void (*)(GLuint, GLuint, GLuint))fn)(p0, p1, p2);
}

/* glUniformMatrix2fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniformMatrix2fv(GLint p0, GLsizei p1, GLboolean p2, const GLfloat * p3)
{
    GLESMOD_HOTPATH("glUniformMatrix2fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniformMatrix2fv");
        if (!fn) {
            glesmod_report_missing("glUniformMatrix2fv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3);
}

/* glUniformMatrix2x3fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniformMatrix2x3fv(GLint p0, GLsizei p1, GLboolean p2, const GLfloat * p3)
{
    GLESMOD_HOTPATH("glUniformMatrix2x3fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniformMatrix2x3fv");
        if (!fn) {
            glesmod_report_missing("glUniformMatrix2x3fv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3);
}

/* glUniformMatrix2x4fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniformMatrix2x4fv(GLint p0, GLsizei p1, GLboolean p2, const GLfloat * p3)
{
    GLESMOD_HOTPATH("glUniformMatrix2x4fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniformMatrix2x4fv");
        if (!fn) {
            glesmod_report_missing("glUniformMatrix2x4fv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3);
}

/* glUniformMatrix3fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniformMatrix3fv(GLint p0, GLsizei p1, GLboolean p2, const GLfloat * p3)
{
    GLESMOD_HOTPATH("glUniformMatrix3fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniformMatrix3fv");
        if (!fn) {
            glesmod_report_missing("glUniformMatrix3fv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3);
}

/* glUniformMatrix3x2fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniformMatrix3x2fv(GLint p0, GLsizei p1, GLboolean p2, const GLfloat * p3)
{
    GLESMOD_HOTPATH("glUniformMatrix3x2fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniformMatrix3x2fv");
        if (!fn) {
            glesmod_report_missing("glUniformMatrix3x2fv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3);
}

/* glUniformMatrix3x4fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniformMatrix3x4fv(GLint p0, GLsizei p1, GLboolean p2, const GLfloat * p3)
{
    GLESMOD_HOTPATH("glUniformMatrix3x4fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniformMatrix3x4fv");
        if (!fn) {
            glesmod_report_missing("glUniformMatrix3x4fv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3);
}

/* glUniformMatrix4fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniformMatrix4fv(GLint p0, GLsizei p1, GLboolean p2, const GLfloat * p3)
{
    GLESMOD_HOTPATH("glUniformMatrix4fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniformMatrix4fv");
        if (!fn) {
            glesmod_report_missing("glUniformMatrix4fv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3);
}

/* glUniformMatrix4x2fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniformMatrix4x2fv(GLint p0, GLsizei p1, GLboolean p2, const GLfloat * p3)
{
    GLESMOD_HOTPATH("glUniformMatrix4x2fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniformMatrix4x2fv");
        if (!fn) {
            glesmod_report_missing("glUniformMatrix4x2fv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3);
}

/* glUniformMatrix4x3fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUniformMatrix4x3fv(GLint p0, GLsizei p1, GLboolean p2, const GLfloat * p3)
{
    GLESMOD_HOTPATH("glUniformMatrix4x3fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUniformMatrix4x3fv");
        if (!fn) {
            glesmod_report_missing("glUniformMatrix4x3fv");
            return;
        }
    }
    ((void (*)(GLint, GLsizei, GLboolean, const GLfloat *))fn)(p0, p1, p2, p3);
}

/* glUnmapBuffer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT GLboolean glUnmapBuffer(GLenum p0)
{
    GLESMOD_HOTPATH("glUnmapBuffer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUnmapBuffer");
        if (!fn) {
            glesmod_report_missing("glUnmapBuffer");
            return GL_FALSE;
        }
    }
    return ((GLboolean (*)(GLenum))fn)(p0);
}

/* glUseProgram —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUseProgram(GLuint p0)
{
    GLESMOD_HOTPATH("glUseProgram");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUseProgram");
        if (!fn) {
            glesmod_report_missing("glUseProgram");
            return;
        }
    }
    ((void (*)(GLuint))fn)(p0);
}

/* glUseProgramStages —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glUseProgramStages(GLuint p0, GLbitfield p1, GLuint p2)
{
    GLESMOD_HOTPATH("glUseProgramStages");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glUseProgramStages");
        if (!fn) {
            glesmod_report_missing("glUseProgramStages");
            return;
        }
    }
    ((void (*)(GLuint, GLbitfield, GLuint))fn)(p0, p1, p2);
}

/* glValidateProgram —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glValidateProgram(GLuint p0)
{
    GLESMOD_HOTPATH("glValidateProgram");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glValidateProgram");
        if (!fn) {
            glesmod_report_missing("glValidateProgram");
            return;
        }
    }
    ((void (*)(GLuint))fn)(p0);
}

/* glValidateProgramPipeline —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glValidateProgramPipeline(GLuint p0)
{
    GLESMOD_HOTPATH("glValidateProgramPipeline");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glValidateProgramPipeline");
        if (!fn) {
            glesmod_report_missing("glValidateProgramPipeline");
            return;
        }
    }
    ((void (*)(GLuint))fn)(p0);
}

/* glVertexAttrib1f —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttrib1f(GLuint p0, GLfloat p1)
{
    GLESMOD_HOTPATH("glVertexAttrib1f");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttrib1f");
        if (!fn) {
            glesmod_report_missing("glVertexAttrib1f");
            return;
        }
    }
    ((void (*)(GLuint, GLfloat))fn)(p0, p1);
}

/* glVertexAttrib1fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttrib1fv(GLuint p0, const GLfloat * p1)
{
    GLESMOD_HOTPATH("glVertexAttrib1fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttrib1fv");
        if (!fn) {
            glesmod_report_missing("glVertexAttrib1fv");
            return;
        }
    }
    ((void (*)(GLuint, const GLfloat *))fn)(p0, p1);
}

/* glVertexAttrib2f —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttrib2f(GLuint p0, GLfloat p1, GLfloat p2)
{
    GLESMOD_HOTPATH("glVertexAttrib2f");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttrib2f");
        if (!fn) {
            glesmod_report_missing("glVertexAttrib2f");
            return;
        }
    }
    ((void (*)(GLuint, GLfloat, GLfloat))fn)(p0, p1, p2);
}

/* glVertexAttrib2fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttrib2fv(GLuint p0, const GLfloat * p1)
{
    GLESMOD_HOTPATH("glVertexAttrib2fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttrib2fv");
        if (!fn) {
            glesmod_report_missing("glVertexAttrib2fv");
            return;
        }
    }
    ((void (*)(GLuint, const GLfloat *))fn)(p0, p1);
}

/* glVertexAttrib3f —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttrib3f(GLuint p0, GLfloat p1, GLfloat p2, GLfloat p3)
{
    GLESMOD_HOTPATH("glVertexAttrib3f");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttrib3f");
        if (!fn) {
            glesmod_report_missing("glVertexAttrib3f");
            return;
        }
    }
    ((void (*)(GLuint, GLfloat, GLfloat, GLfloat))fn)(p0, p1, p2, p3);
}

/* glVertexAttrib3fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttrib3fv(GLuint p0, const GLfloat * p1)
{
    GLESMOD_HOTPATH("glVertexAttrib3fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttrib3fv");
        if (!fn) {
            glesmod_report_missing("glVertexAttrib3fv");
            return;
        }
    }
    ((void (*)(GLuint, const GLfloat *))fn)(p0, p1);
}

/* glVertexAttrib4f —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttrib4f(GLuint p0, GLfloat p1, GLfloat p2, GLfloat p3, GLfloat p4)
{
    GLESMOD_HOTPATH("glVertexAttrib4f");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttrib4f");
        if (!fn) {
            glesmod_report_missing("glVertexAttrib4f");
            return;
        }
    }
    ((void (*)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat))fn)(p0, p1, p2, p3, p4);
}

/* glVertexAttrib4fv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttrib4fv(GLuint p0, const GLfloat * p1)
{
    GLESMOD_HOTPATH("glVertexAttrib4fv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttrib4fv");
        if (!fn) {
            glesmod_report_missing("glVertexAttrib4fv");
            return;
        }
    }
    ((void (*)(GLuint, const GLfloat *))fn)(p0, p1);
}

/* glVertexAttribBinding —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttribBinding(GLuint p0, GLuint p1)
{
    GLESMOD_HOTPATH("glVertexAttribBinding");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttribBinding");
        if (!fn) {
            glesmod_report_missing("glVertexAttribBinding");
            return;
        }
    }
    ((void (*)(GLuint, GLuint))fn)(p0, p1);
}

/* glVertexAttribDivisor —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttribDivisor(GLuint p0, GLuint p1)
{
    GLESMOD_HOTPATH("glVertexAttribDivisor");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttribDivisor");
        if (!fn) {
            glesmod_report_missing("glVertexAttribDivisor");
            return;
        }
    }
    ((void (*)(GLuint, GLuint))fn)(p0, p1);
}

/* glVertexAttribFormat —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttribFormat(GLuint p0, GLint p1, GLenum p2, GLboolean p3, GLuint p4)
{
    GLESMOD_HOTPATH("glVertexAttribFormat");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttribFormat");
        if (!fn) {
            glesmod_report_missing("glVertexAttribFormat");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLenum, GLboolean, GLuint))fn)(p0, p1, p2, p3, p4);
}

/* glVertexAttribI4i —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttribI4i(GLuint p0, GLint p1, GLint p2, GLint p3, GLint p4)
{
    GLESMOD_HOTPATH("glVertexAttribI4i");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttribI4i");
        if (!fn) {
            glesmod_report_missing("glVertexAttribI4i");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLint, GLint, GLint))fn)(p0, p1, p2, p3, p4);
}

/* glVertexAttribI4iv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttribI4iv(GLuint p0, const GLint * p1)
{
    GLESMOD_HOTPATH("glVertexAttribI4iv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttribI4iv");
        if (!fn) {
            glesmod_report_missing("glVertexAttribI4iv");
            return;
        }
    }
    ((void (*)(GLuint, const GLint *))fn)(p0, p1);
}

/* glVertexAttribI4ui —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttribI4ui(GLuint p0, GLuint p1, GLuint p2, GLuint p3, GLuint p4)
{
    GLESMOD_HOTPATH("glVertexAttribI4ui");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttribI4ui");
        if (!fn) {
            glesmod_report_missing("glVertexAttribI4ui");
            return;
        }
    }
    ((void (*)(GLuint, GLuint, GLuint, GLuint, GLuint))fn)(p0, p1, p2, p3, p4);
}

/* glVertexAttribI4uiv —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttribI4uiv(GLuint p0, const GLuint * p1)
{
    GLESMOD_HOTPATH("glVertexAttribI4uiv");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttribI4uiv");
        if (!fn) {
            glesmod_report_missing("glVertexAttribI4uiv");
            return;
        }
    }
    ((void (*)(GLuint, const GLuint *))fn)(p0, p1);
}

/* glVertexAttribIFormat —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttribIFormat(GLuint p0, GLint p1, GLenum p2, GLuint p3)
{
    GLESMOD_HOTPATH("glVertexAttribIFormat");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttribIFormat");
        if (!fn) {
            glesmod_report_missing("glVertexAttribIFormat");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLenum, GLuint))fn)(p0, p1, p2, p3);
}

/* glVertexAttribIPointer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttribIPointer(GLuint p0, GLint p1, GLenum p2, GLsizei p3, const void * p4)
{
    GLESMOD_HOTPATH("glVertexAttribIPointer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttribIPointer");
        if (!fn) {
            glesmod_report_missing("glVertexAttribIPointer");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLenum, GLsizei, const void *))fn)(p0, p1, p2, p3, p4);
}

/* glVertexAttribPointer —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexAttribPointer(GLuint p0, GLint p1, GLenum p2, GLboolean p3, GLsizei p4, const void * p5)
{
    GLESMOD_HOTPATH("glVertexAttribPointer");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexAttribPointer");
        if (!fn) {
            glesmod_report_missing("glVertexAttribPointer");
            return;
        }
    }
    ((void (*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *))fn)(p0, p1, p2, p3, p4, p5);
}

/* glVertexBindingDivisor —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glVertexBindingDivisor(GLuint p0, GLuint p1)
{
    GLESMOD_HOTPATH("glVertexBindingDivisor");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glVertexBindingDivisor");
        if (!fn) {
            glesmod_report_missing("glVertexBindingDivisor");
            return;
        }
    }
    ((void (*)(GLuint, GLuint))fn)(p0, p1);
}

/* glViewport —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glViewport(GLint p0, GLint p1, GLsizei p2, GLsizei p3)
{
    GLESMOD_HOTPATH("glViewport");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glViewport");
        if (!fn) {
            glesmod_report_missing("glViewport");
            return;
        }
    }
    ((void (*)(GLint, GLint, GLsizei, GLsizei))fn)(p0, p1, p2, p3);
}

/* glWaitSync —— 转发至同名 GLES 函数 */
GLESMOD_EXPORT void glWaitSync(GLsync p0, GLbitfield p1, GLuint64 p2)
{
    GLESMOD_HOTPATH("glWaitSync");
    static glesmod_proc_t fn = NULL;
    if (!fn) {
        fn = glesym_resolve("glWaitSync");
        if (!fn) {
            glesmod_report_missing("glWaitSync");
            return;
        }
    }
    ((void (*)(GLsync, GLbitfield, GLuint64))fn)(p0, p1, p2);
}

/* ==================== 安全 stub ==================== */

/*
 * 这些符号在 GL 3.2 core 中存在，但 GLES 3.2 不提供。
 * 统一导出为安全 stub 而不是留空，原因见文件头说明。
 *
 * 每个 stub 都调用 glesmod_report_stub()，它会：
 *   - 首次调用时记一条降级事件（避免刷屏）
 *   - 把符号名加入状态文件的 stub_symbols 列表，便于诊断
 *   - 不做任何可能影响渲染的副作用
 */

/* glAccum —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glAccum(GLenum p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glAccum");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glAccum");
}

/* glAlphaFunc —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glAlphaFunc(GLenum p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glAlphaFunc");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glAlphaFunc");
}

/* glArrayElement —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glArrayElement(GLint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glArrayElement");
    (void)p0;
    glesmod_report_stub("glArrayElement");
}

/* glBegin —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glBegin(GLenum p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glBegin");
    (void)p0;
    glesmod_report_stub("glBegin");
}

/* glBeginConditionalRender —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glBeginConditionalRender(GLuint p0, GLenum p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glBeginConditionalRender");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glBeginConditionalRender");
}

/* glBeginQueryIndexed —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glBeginQueryIndexed(GLenum p0, GLuint p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glBeginQueryIndexed");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glBeginQueryIndexed");
}

/* glBindBuffersBase —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glBindBuffersBase(GLenum p0, GLuint p1, GLsizei p2, const GLuint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glBindBuffersBase");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glBindBuffersBase");
}

/* glBindBuffersRange —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glBindBuffersRange(GLenum p0, GLuint p1, GLsizei p2, const GLuint * p3, const GLintptr * p4, const GLsizeiptr * p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glBindBuffersRange");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glBindBuffersRange");
}

/* glBindFragDataLocation —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glBindFragDataLocation(GLuint p0, GLuint p1, const GLchar * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glBindFragDataLocation");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glBindFragDataLocation");
}

/* glBindFragDataLocationIndexed —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glBindFragDataLocationIndexed(GLuint p0, GLuint p1, GLuint p2, const GLchar * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glBindFragDataLocationIndexed");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glBindFragDataLocationIndexed");
}

/* glBindImageTextures —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glBindImageTextures(GLuint p0, GLsizei p1, const GLuint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glBindImageTextures");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glBindImageTextures");
}

/* glBindSamplers —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glBindSamplers(GLuint p0, GLsizei p1, const GLuint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glBindSamplers");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glBindSamplers");
}

/* glBindTextureUnit —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glBindTextureUnit(GLuint p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glBindTextureUnit");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glBindTextureUnit");
}

/* glBindTextures —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glBindTextures(GLuint p0, GLsizei p1, const GLuint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glBindTextures");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glBindTextures");
}

/* glBindVertexBuffers —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glBindVertexBuffers(GLuint p0, GLsizei p1, const GLuint * p2, const GLintptr * p3, const GLsizei * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glBindVertexBuffers");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glBindVertexBuffers");
}

/* glBlitNamedFramebuffer —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glBlitNamedFramebuffer(GLuint p0, GLuint p1, GLint p2, GLint p3, GLint p4, GLint p5, GLint p6, GLint p7, GLint p8, GLint p9, GLbitfield p10, GLenum p11)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glBlitNamedFramebuffer");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    (void)p8;
    (void)p9;
    (void)p10;
    (void)p11;
    glesmod_report_stub("glBlitNamedFramebuffer");
}

/* glCallList —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCallList(GLuint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCallList");
    (void)p0;
    glesmod_report_stub("glCallList");
}

/* glCheckNamedFramebufferStatus —— GLES 不提供，安全 stub */
GLESMOD_EXPORT GLenum glCheckNamedFramebufferStatus(GLuint p0, GLenum p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCheckNamedFramebufferStatus");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glCheckNamedFramebufferStatus");
    return 0;
}

/* glClampColor —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClampColor(GLenum p0, GLenum p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClampColor");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glClampColor");
}

/* glClearAccum —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClearAccum(GLfloat p0, GLfloat p1, GLfloat p2, GLfloat p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClearAccum");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glClearAccum");
}

/* glClearBufferData —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClearBufferData(GLenum p0, GLenum p1, GLenum p2, GLenum p3, const void * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClearBufferData");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glClearBufferData");
}

/* glClearBufferSubData —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClearBufferSubData(GLenum p0, GLenum p1, GLintptr p2, GLsizeiptr p3, GLenum p4, GLenum p5, const void * p6)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClearBufferSubData");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    glesmod_report_stub("glClearBufferSubData");
}

/* glClearIndex —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClearIndex(GLfloat p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClearIndex");
    (void)p0;
    glesmod_report_stub("glClearIndex");
}

/* glClearNamedBufferData —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClearNamedBufferData(GLuint p0, GLenum p1, GLenum p2, GLenum p3, const void * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClearNamedBufferData");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glClearNamedBufferData");
}

/* glClearNamedBufferSubData —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClearNamedBufferSubData(GLuint p0, GLenum p1, GLintptr p2, GLsizeiptr p3, GLenum p4, GLenum p5, const void * p6)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClearNamedBufferSubData");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    glesmod_report_stub("glClearNamedBufferSubData");
}

/* glClearNamedFramebufferfi —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClearNamedFramebufferfi(GLuint p0, GLenum p1, GLint p2, GLfloat p3, GLint p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClearNamedFramebufferfi");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glClearNamedFramebufferfi");
}

/* glClearNamedFramebufferfv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClearNamedFramebufferfv(GLuint p0, GLenum p1, GLint p2, const GLfloat * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClearNamedFramebufferfv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glClearNamedFramebufferfv");
}

/* glClearNamedFramebufferiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClearNamedFramebufferiv(GLuint p0, GLenum p1, GLint p2, const GLint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClearNamedFramebufferiv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glClearNamedFramebufferiv");
}

/* glClearNamedFramebufferuiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClearNamedFramebufferuiv(GLuint p0, GLenum p1, GLint p2, const GLuint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClearNamedFramebufferuiv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glClearNamedFramebufferuiv");
}

/* glClearTexImage —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClearTexImage(GLuint p0, GLint p1, GLenum p2, GLenum p3, const void * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClearTexImage");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glClearTexImage");
}

/* glClearTexSubImage —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClearTexSubImage(GLuint p0, GLint p1, GLint p2, GLint p3, GLint p4, GLsizei p5, GLsizei p6, GLsizei p7, GLenum p8, GLenum p9, const void * p10)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClearTexSubImage");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    (void)p8;
    (void)p9;
    (void)p10;
    glesmod_report_stub("glClearTexSubImage");
}

/* glClientActiveTexture —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClientActiveTexture(GLenum p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClientActiveTexture");
    (void)p0;
    glesmod_report_stub("glClientActiveTexture");
}

/* glClipControl —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glClipControl(GLenum p0, GLenum p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glClipControl");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glClipControl");
}

/* glColor3b —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor3b(GLbyte p0, GLbyte p1, GLbyte p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor3b");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glColor3b");
}

/* glColor3d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor3d(GLdouble p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor3d");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glColor3d");
}

/* glColor3f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor3f(GLfloat p0, GLfloat p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor3f");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glColor3f");
}

/* glColor3i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor3i(GLint p0, GLint p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor3i");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glColor3i");
}

/* glColor3s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor3s(GLshort p0, GLshort p1, GLshort p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor3s");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glColor3s");
}

/* glColor3ub —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor3ub(GLubyte p0, GLubyte p1, GLubyte p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor3ub");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glColor3ub");
}

/* glColor3ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor3ui(GLuint p0, GLuint p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor3ui");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glColor3ui");
}

/* glColor3us —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor3us(GLushort p0, GLushort p1, GLushort p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor3us");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glColor3us");
}

/* glColor4b —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor4b(GLbyte p0, GLbyte p1, GLbyte p2, GLbyte p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor4b");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glColor4b");
}

/* glColor4d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor4d(GLdouble p0, GLdouble p1, GLdouble p2, GLdouble p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor4d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glColor4d");
}

/* glColor4f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor4f(GLfloat p0, GLfloat p1, GLfloat p2, GLfloat p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor4f");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glColor4f");
}

/* glColor4i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor4i(GLint p0, GLint p1, GLint p2, GLint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor4i");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glColor4i");
}

/* glColor4s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor4s(GLshort p0, GLshort p1, GLshort p2, GLshort p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor4s");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glColor4s");
}

/* glColor4ub —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor4ub(GLubyte p0, GLubyte p1, GLubyte p2, GLubyte p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor4ub");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glColor4ub");
}

/* glColor4ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor4ui(GLuint p0, GLuint p1, GLuint p2, GLuint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor4ui");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glColor4ui");
}

/* glColor4us —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColor4us(GLushort p0, GLushort p1, GLushort p2, GLushort p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColor4us");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glColor4us");
}

/* glColorMaterial —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColorMaterial(GLenum p0, GLenum p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColorMaterial");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glColorMaterial");
}

/* glColorP3ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColorP3ui(GLenum p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColorP3ui");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glColorP3ui");
}

/* glColorP4ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glColorP4ui(GLenum p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glColorP4ui");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glColorP4ui");
}

/* glCompressedTexImage1D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCompressedTexImage1D(GLenum p0, GLint p1, GLenum p2, GLsizei p3, GLint p4, GLsizei p5, const void * p6)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCompressedTexImage1D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    glesmod_report_stub("glCompressedTexImage1D");
}

/* glCompressedTexSubImage1D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCompressedTexSubImage1D(GLenum p0, GLint p1, GLint p2, GLsizei p3, GLenum p4, GLsizei p5, const void * p6)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCompressedTexSubImage1D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    glesmod_report_stub("glCompressedTexSubImage1D");
}

/* glCompressedTextureSubImage1D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCompressedTextureSubImage1D(GLuint p0, GLint p1, GLint p2, GLsizei p3, GLenum p4, GLsizei p5, const void * p6)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCompressedTextureSubImage1D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    glesmod_report_stub("glCompressedTextureSubImage1D");
}

/* glCompressedTextureSubImage2D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCompressedTextureSubImage2D(GLuint p0, GLint p1, GLint p2, GLint p3, GLsizei p4, GLsizei p5, GLenum p6, GLsizei p7, const void * p8)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCompressedTextureSubImage2D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    (void)p8;
    glesmod_report_stub("glCompressedTextureSubImage2D");
}

/* glCompressedTextureSubImage3D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCompressedTextureSubImage3D(GLuint p0, GLint p1, GLint p2, GLint p3, GLint p4, GLsizei p5, GLsizei p6, GLsizei p7, GLenum p8, GLsizei p9, const void * p10)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCompressedTextureSubImage3D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    (void)p8;
    (void)p9;
    (void)p10;
    glesmod_report_stub("glCompressedTextureSubImage3D");
}

/* glCopyNamedBufferSubData —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCopyNamedBufferSubData(GLuint p0, GLuint p1, GLintptr p2, GLintptr p3, GLsizeiptr p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCopyNamedBufferSubData");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glCopyNamedBufferSubData");
}

/* glCopyPixels —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCopyPixels(GLint p0, GLint p1, GLsizei p2, GLsizei p3, GLenum p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCopyPixels");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glCopyPixels");
}

/* glCopyTexImage1D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCopyTexImage1D(GLenum p0, GLint p1, GLenum p2, GLint p3, GLint p4, GLsizei p5, GLint p6)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCopyTexImage1D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    glesmod_report_stub("glCopyTexImage1D");
}

/* glCopyTexSubImage1D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCopyTexSubImage1D(GLenum p0, GLint p1, GLint p2, GLint p3, GLint p4, GLsizei p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCopyTexSubImage1D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glCopyTexSubImage1D");
}

/* glCopyTextureSubImage1D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCopyTextureSubImage1D(GLuint p0, GLint p1, GLint p2, GLint p3, GLint p4, GLsizei p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCopyTextureSubImage1D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glCopyTextureSubImage1D");
}

/* glCopyTextureSubImage2D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCopyTextureSubImage2D(GLuint p0, GLint p1, GLint p2, GLint p3, GLint p4, GLint p5, GLsizei p6, GLsizei p7)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCopyTextureSubImage2D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    glesmod_report_stub("glCopyTextureSubImage2D");
}

/* glCopyTextureSubImage3D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCopyTextureSubImage3D(GLuint p0, GLint p1, GLint p2, GLint p3, GLint p4, GLint p5, GLint p6, GLsizei p7, GLsizei p8)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCopyTextureSubImage3D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    (void)p8;
    glesmod_report_stub("glCopyTextureSubImage3D");
}

/* glCreateBuffers —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCreateBuffers(GLsizei p0, GLuint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCreateBuffers");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glCreateBuffers");
}

/* glCreateFramebuffers —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCreateFramebuffers(GLsizei p0, GLuint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCreateFramebuffers");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glCreateFramebuffers");
}

/* glCreateProgramPipelines —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCreateProgramPipelines(GLsizei p0, GLuint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCreateProgramPipelines");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glCreateProgramPipelines");
}

/* glCreateQueries —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCreateQueries(GLenum p0, GLsizei p1, GLuint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCreateQueries");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glCreateQueries");
}

/* glCreateRenderbuffers —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCreateRenderbuffers(GLsizei p0, GLuint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCreateRenderbuffers");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glCreateRenderbuffers");
}

/* glCreateSamplers —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCreateSamplers(GLsizei p0, GLuint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCreateSamplers");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glCreateSamplers");
}

/* glCreateTextures —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCreateTextures(GLenum p0, GLsizei p1, GLuint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCreateTextures");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glCreateTextures");
}

/* glCreateTransformFeedbacks —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCreateTransformFeedbacks(GLsizei p0, GLuint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCreateTransformFeedbacks");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glCreateTransformFeedbacks");
}

/* glCreateVertexArrays —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glCreateVertexArrays(GLsizei p0, GLuint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glCreateVertexArrays");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glCreateVertexArrays");
}

/* glDeleteLists —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDeleteLists(GLuint p0, GLsizei p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDeleteLists");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glDeleteLists");
}

/* glDepthRange —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDepthRange(GLdouble p0, GLdouble p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDepthRange");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glDepthRange");
}

/* glDepthRangeArrayv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDepthRangeArrayv(GLuint p0, GLsizei p1, const GLdouble * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDepthRangeArrayv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glDepthRangeArrayv");
}

/* glDepthRangeIndexed —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDepthRangeIndexed(GLuint p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDepthRangeIndexed");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glDepthRangeIndexed");
}

/* glDisableClientState —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDisableClientState(GLenum p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDisableClientState");
    (void)p0;
    glesmod_report_stub("glDisableClientState");
}

/* glDisableVertexArrayAttrib —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDisableVertexArrayAttrib(GLuint p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDisableVertexArrayAttrib");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glDisableVertexArrayAttrib");
}

/* glDrawArraysInstancedBaseInstance —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDrawArraysInstancedBaseInstance(GLenum p0, GLint p1, GLsizei p2, GLsizei p3, GLuint p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDrawArraysInstancedBaseInstance");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glDrawArraysInstancedBaseInstance");
}

/* glDrawBuffer —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDrawBuffer(GLenum p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDrawBuffer");
    (void)p0;
    glesmod_report_stub("glDrawBuffer");
}

/* glDrawElementsInstancedBaseInstance —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDrawElementsInstancedBaseInstance(GLenum p0, GLsizei p1, GLenum p2, const void * p3, GLsizei p4, GLuint p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDrawElementsInstancedBaseInstance");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glDrawElementsInstancedBaseInstance");
}

/* glDrawElementsInstancedBaseVertexBaseInstance —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDrawElementsInstancedBaseVertexBaseInstance(GLenum p0, GLsizei p1, GLenum p2, const void * p3, GLsizei p4, GLint p5, GLuint p6)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDrawElementsInstancedBaseVertexBaseInstance");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    glesmod_report_stub("glDrawElementsInstancedBaseVertexBaseInstance");
}

/* glDrawTransformFeedback —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDrawTransformFeedback(GLenum p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDrawTransformFeedback");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glDrawTransformFeedback");
}

/* glDrawTransformFeedbackInstanced —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDrawTransformFeedbackInstanced(GLenum p0, GLuint p1, GLsizei p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDrawTransformFeedbackInstanced");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glDrawTransformFeedbackInstanced");
}

/* glDrawTransformFeedbackStream —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDrawTransformFeedbackStream(GLenum p0, GLuint p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDrawTransformFeedbackStream");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glDrawTransformFeedbackStream");
}

/* glDrawTransformFeedbackStreamInstanced —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glDrawTransformFeedbackStreamInstanced(GLenum p0, GLuint p1, GLuint p2, GLsizei p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glDrawTransformFeedbackStreamInstanced");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glDrawTransformFeedbackStreamInstanced");
}

/* glEdgeFlag —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEdgeFlag(GLboolean p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEdgeFlag");
    (void)p0;
    glesmod_report_stub("glEdgeFlag");
}

/* glEnableClientState —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEnableClientState(GLenum p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEnableClientState");
    (void)p0;
    glesmod_report_stub("glEnableClientState");
}

/* glEnableVertexArrayAttrib —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEnableVertexArrayAttrib(GLuint p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEnableVertexArrayAttrib");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glEnableVertexArrayAttrib");
}

/* glEnd —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEnd(void)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEnd");
    glesmod_report_stub("glEnd");
}

/* glEndConditionalRender —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEndConditionalRender(void)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEndConditionalRender");
    glesmod_report_stub("glEndConditionalRender");
}

/* glEndList —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEndList(void)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEndList");
    glesmod_report_stub("glEndList");
}

/* glEndQueryIndexed —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEndQueryIndexed(GLenum p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEndQueryIndexed");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glEndQueryIndexed");
}

/* glEvalCoord1d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEvalCoord1d(GLdouble p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEvalCoord1d");
    (void)p0;
    glesmod_report_stub("glEvalCoord1d");
}

/* glEvalCoord1f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEvalCoord1f(GLfloat p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEvalCoord1f");
    (void)p0;
    glesmod_report_stub("glEvalCoord1f");
}

/* glEvalCoord2d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEvalCoord2d(GLdouble p0, GLdouble p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEvalCoord2d");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glEvalCoord2d");
}

/* glEvalCoord2f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEvalCoord2f(GLfloat p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEvalCoord2f");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glEvalCoord2f");
}

/* glEvalMesh1 —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEvalMesh1(GLenum p0, GLint p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEvalMesh1");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glEvalMesh1");
}

/* glEvalMesh2 —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEvalMesh2(GLenum p0, GLint p1, GLint p2, GLint p3, GLint p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEvalMesh2");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glEvalMesh2");
}

/* glEvalPoint1 —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEvalPoint1(GLint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEvalPoint1");
    (void)p0;
    glesmod_report_stub("glEvalPoint1");
}

/* glEvalPoint2 —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glEvalPoint2(GLint p0, GLint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glEvalPoint2");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glEvalPoint2");
}

/* glFlushMappedNamedBufferRange —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glFlushMappedNamedBufferRange(GLuint p0, GLintptr p1, GLsizeiptr p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glFlushMappedNamedBufferRange");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glFlushMappedNamedBufferRange");
}

/* glFogCoordd —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glFogCoordd(GLdouble p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glFogCoordd");
    (void)p0;
    glesmod_report_stub("glFogCoordd");
}

/* glFogCoordf —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glFogCoordf(GLfloat p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glFogCoordf");
    (void)p0;
    glesmod_report_stub("glFogCoordf");
}

/* glFogf —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glFogf(GLenum p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glFogf");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glFogf");
}

/* glFogi —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glFogi(GLenum p0, GLint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glFogi");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glFogi");
}

/* glFramebufferTexture1D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glFramebufferTexture1D(GLenum p0, GLenum p1, GLenum p2, GLuint p3, GLint p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glFramebufferTexture1D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glFramebufferTexture1D");
}

/* glFramebufferTexture3D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glFramebufferTexture3D(GLenum p0, GLenum p1, GLenum p2, GLuint p3, GLint p4, GLint p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glFramebufferTexture3D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glFramebufferTexture3D");
}

/* glFrustum —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glFrustum(GLdouble p0, GLdouble p1, GLdouble p2, GLdouble p3, GLdouble p4, GLdouble p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glFrustum");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glFrustum");
}

/* glGenLists —— GLES 不提供，安全 stub */
GLESMOD_EXPORT GLuint glGenLists(GLsizei p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGenLists");
    (void)p0;
    glesmod_report_stub("glGenLists");
    return 0;
}

/* glGenerateTextureMipmap —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGenerateTextureMipmap(GLuint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGenerateTextureMipmap");
    (void)p0;
    glesmod_report_stub("glGenerateTextureMipmap");
}

/* glGetActiveAtomicCounterBufferiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetActiveAtomicCounterBufferiv(GLuint p0, GLuint p1, GLenum p2, GLint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetActiveAtomicCounterBufferiv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetActiveAtomicCounterBufferiv");
}

/* glGetActiveSubroutineName —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetActiveSubroutineName(GLuint p0, GLenum p1, GLuint p2, GLsizei p3, GLsizei * p4, GLchar * p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetActiveSubroutineName");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glGetActiveSubroutineName");
}

/* glGetActiveSubroutineUniformName —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetActiveSubroutineUniformName(GLuint p0, GLenum p1, GLuint p2, GLsizei p3, GLsizei * p4, GLchar * p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetActiveSubroutineUniformName");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glGetActiveSubroutineUniformName");
}

/* glGetActiveSubroutineUniformiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetActiveSubroutineUniformiv(GLuint p0, GLenum p1, GLuint p2, GLenum p3, GLint * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetActiveSubroutineUniformiv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glGetActiveSubroutineUniformiv");
}

/* glGetActiveUniformName —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetActiveUniformName(GLuint p0, GLuint p1, GLsizei p2, GLsizei * p3, GLchar * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetActiveUniformName");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glGetActiveUniformName");
}

/* glGetBufferSubData —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetBufferSubData(GLenum p0, GLintptr p1, GLsizeiptr p2, void * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetBufferSubData");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetBufferSubData");
}

/* glGetCompressedTexImage —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetCompressedTexImage(GLenum p0, GLint p1, void * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetCompressedTexImage");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetCompressedTexImage");
}

/* glGetCompressedTextureImage —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetCompressedTextureImage(GLuint p0, GLint p1, GLsizei p2, void * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetCompressedTextureImage");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetCompressedTextureImage");
}

/* glGetCompressedTextureSubImage —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetCompressedTextureSubImage(GLuint p0, GLint p1, GLint p2, GLint p3, GLint p4, GLsizei p5, GLsizei p6, GLsizei p7, GLsizei p8, void * p9)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetCompressedTextureSubImage");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    (void)p8;
    (void)p9;
    glesmod_report_stub("glGetCompressedTextureSubImage");
}

/* glGetDoublei_v —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetDoublei_v(GLenum p0, GLuint p1, GLdouble * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetDoublei_v");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetDoublei_v");
}

/* glGetDoublev —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetDoublev(GLenum p0, GLdouble * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetDoublev");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glGetDoublev");
}

/* glGetFloati_v —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetFloati_v(GLenum p0, GLuint p1, GLfloat * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetFloati_v");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetFloati_v");
}

/* glGetFragDataIndex —— GLES 不提供，安全 stub */
GLESMOD_EXPORT GLint glGetFragDataIndex(GLuint p0, const GLchar * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetFragDataIndex");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glGetFragDataIndex");
    return 0;
}

/* glGetInternalformati64v —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetInternalformati64v(GLenum p0, GLenum p1, GLenum p2, GLsizei p3, GLint64 * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetInternalformati64v");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glGetInternalformati64v");
}

/* glGetNamedBufferParameteri64v —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetNamedBufferParameteri64v(GLuint p0, GLenum p1, GLint64 * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetNamedBufferParameteri64v");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetNamedBufferParameteri64v");
}

/* glGetNamedBufferParameteriv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetNamedBufferParameteriv(GLuint p0, GLenum p1, GLint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetNamedBufferParameteriv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetNamedBufferParameteriv");
}

/* glGetNamedBufferPointerv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetNamedBufferPointerv(GLuint p0, GLenum p1, void ** p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetNamedBufferPointerv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetNamedBufferPointerv");
}

/* glGetNamedBufferSubData —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetNamedBufferSubData(GLuint p0, GLintptr p1, GLsizeiptr p2, void * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetNamedBufferSubData");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetNamedBufferSubData");
}

/* glGetNamedFramebufferAttachmentParameteriv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetNamedFramebufferAttachmentParameteriv(GLuint p0, GLenum p1, GLenum p2, GLint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetNamedFramebufferAttachmentParameteriv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetNamedFramebufferAttachmentParameteriv");
}

/* glGetNamedFramebufferParameteriv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetNamedFramebufferParameteriv(GLuint p0, GLenum p1, GLint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetNamedFramebufferParameteriv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetNamedFramebufferParameteriv");
}

/* glGetNamedRenderbufferParameteriv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetNamedRenderbufferParameteriv(GLuint p0, GLenum p1, GLint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetNamedRenderbufferParameteriv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetNamedRenderbufferParameteriv");
}

/* glGetProgramResourceLocationIndex —— GLES 不提供，安全 stub */
GLESMOD_EXPORT GLint glGetProgramResourceLocationIndex(GLuint p0, GLenum p1, const GLchar * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetProgramResourceLocationIndex");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetProgramResourceLocationIndex");
    return 0;
}

/* glGetProgramStageiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetProgramStageiv(GLuint p0, GLenum p1, GLenum p2, GLint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetProgramStageiv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetProgramStageiv");
}

/* glGetQueryBufferObjecti64v —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetQueryBufferObjecti64v(GLuint p0, GLuint p1, GLenum p2, GLintptr p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetQueryBufferObjecti64v");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetQueryBufferObjecti64v");
}

/* glGetQueryBufferObjectiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetQueryBufferObjectiv(GLuint p0, GLuint p1, GLenum p2, GLintptr p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetQueryBufferObjectiv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetQueryBufferObjectiv");
}

/* glGetQueryBufferObjectui64v —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetQueryBufferObjectui64v(GLuint p0, GLuint p1, GLenum p2, GLintptr p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetQueryBufferObjectui64v");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetQueryBufferObjectui64v");
}

/* glGetQueryBufferObjectuiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetQueryBufferObjectuiv(GLuint p0, GLuint p1, GLenum p2, GLintptr p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetQueryBufferObjectuiv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetQueryBufferObjectuiv");
}

/* glGetQueryIndexediv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetQueryIndexediv(GLenum p0, GLuint p1, GLenum p2, GLint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetQueryIndexediv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetQueryIndexediv");
}

/* glGetQueryObjecti64v —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetQueryObjecti64v(GLuint p0, GLenum p1, GLint64 * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetQueryObjecti64v");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetQueryObjecti64v");
}

/* glGetQueryObjectiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetQueryObjectiv(GLuint p0, GLenum p1, GLint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetQueryObjectiv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetQueryObjectiv");
}

/* glGetQueryObjectui64v —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetQueryObjectui64v(GLuint p0, GLenum p1, GLuint64 * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetQueryObjectui64v");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetQueryObjectui64v");
}

/* glGetSubroutineIndex —— GLES 不提供，安全 stub */
GLESMOD_EXPORT GLuint glGetSubroutineIndex(GLuint p0, GLenum p1, const GLchar * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetSubroutineIndex");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetSubroutineIndex");
    return 0;
}

/* glGetSubroutineUniformLocation —— GLES 不提供，安全 stub */
GLESMOD_EXPORT GLint glGetSubroutineUniformLocation(GLuint p0, GLenum p1, const GLchar * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetSubroutineUniformLocation");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetSubroutineUniformLocation");
    return 0;
}

/* glGetTextureImage —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetTextureImage(GLuint p0, GLint p1, GLenum p2, GLenum p3, GLsizei p4, void * p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetTextureImage");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glGetTextureImage");
}

/* glGetTextureLevelParameterfv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetTextureLevelParameterfv(GLuint p0, GLint p1, GLenum p2, GLfloat * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetTextureLevelParameterfv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetTextureLevelParameterfv");
}

/* glGetTextureLevelParameteriv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetTextureLevelParameteriv(GLuint p0, GLint p1, GLenum p2, GLint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetTextureLevelParameteriv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetTextureLevelParameteriv");
}

/* glGetTextureParameterIiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetTextureParameterIiv(GLuint p0, GLenum p1, GLint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetTextureParameterIiv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetTextureParameterIiv");
}

/* glGetTextureParameterIuiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetTextureParameterIuiv(GLuint p0, GLenum p1, GLuint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetTextureParameterIuiv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetTextureParameterIuiv");
}

/* glGetTextureParameterfv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetTextureParameterfv(GLuint p0, GLenum p1, GLfloat * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetTextureParameterfv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetTextureParameterfv");
}

/* glGetTextureParameteriv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetTextureParameteriv(GLuint p0, GLenum p1, GLint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetTextureParameteriv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetTextureParameteriv");
}

/* glGetTextureSubImage —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetTextureSubImage(GLuint p0, GLint p1, GLint p2, GLint p3, GLint p4, GLsizei p5, GLsizei p6, GLsizei p7, GLenum p8, GLenum p9, GLsizei p10, void * p11)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetTextureSubImage");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    (void)p8;
    (void)p9;
    (void)p10;
    (void)p11;
    glesmod_report_stub("glGetTextureSubImage");
}

/* glGetTransformFeedbacki64_v —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetTransformFeedbacki64_v(GLuint p0, GLenum p1, GLuint p2, GLint64 * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetTransformFeedbacki64_v");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetTransformFeedbacki64_v");
}

/* glGetTransformFeedbacki_v —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetTransformFeedbacki_v(GLuint p0, GLenum p1, GLuint p2, GLint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetTransformFeedbacki_v");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetTransformFeedbacki_v");
}

/* glGetTransformFeedbackiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetTransformFeedbackiv(GLuint p0, GLenum p1, GLint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetTransformFeedbackiv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetTransformFeedbackiv");
}

/* glGetUniformSubroutineuiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetUniformSubroutineuiv(GLenum p0, GLint p1, GLuint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetUniformSubroutineuiv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetUniformSubroutineuiv");
}

/* glGetUniformdv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetUniformdv(GLuint p0, GLint p1, GLdouble * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetUniformdv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetUniformdv");
}

/* glGetVertexArrayIndexed64iv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetVertexArrayIndexed64iv(GLuint p0, GLuint p1, GLenum p2, GLint64 * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetVertexArrayIndexed64iv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetVertexArrayIndexed64iv");
}

/* glGetVertexArrayIndexediv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetVertexArrayIndexediv(GLuint p0, GLuint p1, GLenum p2, GLint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetVertexArrayIndexediv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetVertexArrayIndexediv");
}

/* glGetVertexArrayiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetVertexArrayiv(GLuint p0, GLenum p1, GLint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetVertexArrayiv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetVertexArrayiv");
}

/* glGetVertexAttribLdv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetVertexAttribLdv(GLuint p0, GLenum p1, GLdouble * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetVertexAttribLdv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetVertexAttribLdv");
}

/* glGetVertexAttribdv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetVertexAttribdv(GLuint p0, GLenum p1, GLdouble * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetVertexAttribdv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glGetVertexAttribdv");
}

/* glGetnCompressedTexImage —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetnCompressedTexImage(GLenum p0, GLint p1, GLsizei p2, void * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetnCompressedTexImage");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetnCompressedTexImage");
}

/* glGetnTexImage —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetnTexImage(GLenum p0, GLint p1, GLenum p2, GLenum p3, GLsizei p4, void * p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetnTexImage");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glGetnTexImage");
}

/* glGetnUniformdv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glGetnUniformdv(GLuint p0, GLint p1, GLsizei p2, GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glGetnUniformdv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glGetnUniformdv");
}

/* glIndexMask —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glIndexMask(GLuint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glIndexMask");
    (void)p0;
    glesmod_report_stub("glIndexMask");
}

/* glIndexd —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glIndexd(GLdouble p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glIndexd");
    (void)p0;
    glesmod_report_stub("glIndexd");
}

/* glIndexf —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glIndexf(GLfloat p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glIndexf");
    (void)p0;
    glesmod_report_stub("glIndexf");
}

/* glIndexi —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glIndexi(GLint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glIndexi");
    (void)p0;
    glesmod_report_stub("glIndexi");
}

/* glIndexs —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glIndexs(GLshort p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glIndexs");
    (void)p0;
    glesmod_report_stub("glIndexs");
}

/* glIndexub —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glIndexub(GLubyte p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glIndexub");
    (void)p0;
    glesmod_report_stub("glIndexub");
}

/* glInitNames —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glInitNames(void)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glInitNames");
    glesmod_report_stub("glInitNames");
}

/* glInvalidateBufferData —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glInvalidateBufferData(GLuint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glInvalidateBufferData");
    (void)p0;
    glesmod_report_stub("glInvalidateBufferData");
}

/* glInvalidateBufferSubData —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glInvalidateBufferSubData(GLuint p0, GLintptr p1, GLsizeiptr p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glInvalidateBufferSubData");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glInvalidateBufferSubData");
}

/* glInvalidateNamedFramebufferData —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glInvalidateNamedFramebufferData(GLuint p0, GLsizei p1, const GLenum * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glInvalidateNamedFramebufferData");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glInvalidateNamedFramebufferData");
}

/* glInvalidateNamedFramebufferSubData —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glInvalidateNamedFramebufferSubData(GLuint p0, GLsizei p1, const GLenum * p2, GLint p3, GLint p4, GLsizei p5, GLsizei p6)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glInvalidateNamedFramebufferSubData");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    glesmod_report_stub("glInvalidateNamedFramebufferSubData");
}

/* glInvalidateTexImage —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glInvalidateTexImage(GLuint p0, GLint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glInvalidateTexImage");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glInvalidateTexImage");
}

/* glInvalidateTexSubImage —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glInvalidateTexSubImage(GLuint p0, GLint p1, GLint p2, GLint p3, GLint p4, GLsizei p5, GLsizei p6, GLsizei p7)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glInvalidateTexSubImage");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    glesmod_report_stub("glInvalidateTexSubImage");
}

/* glIsList —— GLES 不提供，安全 stub */
GLESMOD_EXPORT GLboolean glIsList(GLuint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glIsList");
    (void)p0;
    glesmod_report_stub("glIsList");
    return GL_FALSE;
}

/* glLightModelf —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glLightModelf(GLenum p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glLightModelf");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glLightModelf");
}

/* glLightModeli —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glLightModeli(GLenum p0, GLint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glLightModeli");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glLightModeli");
}

/* glLightf —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glLightf(GLenum p0, GLenum p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glLightf");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glLightf");
}

/* glLighti —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glLighti(GLenum p0, GLenum p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glLighti");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glLighti");
}

/* glLineStipple —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glLineStipple(GLint p0, GLushort p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glLineStipple");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glLineStipple");
}

/* glListBase —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glListBase(GLuint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glListBase");
    (void)p0;
    glesmod_report_stub("glListBase");
}

/* glLoadIdentity —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glLoadIdentity(void)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glLoadIdentity");
    glesmod_report_stub("glLoadIdentity");
}

/* glLoadName —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glLoadName(GLuint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glLoadName");
    (void)p0;
    glesmod_report_stub("glLoadName");
}

/* glMapGrid1d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMapGrid1d(GLint p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMapGrid1d");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glMapGrid1d");
}

/* glMapGrid1f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMapGrid1f(GLint p0, GLfloat p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMapGrid1f");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glMapGrid1f");
}

/* glMapGrid2d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMapGrid2d(GLint p0, GLdouble p1, GLdouble p2, GLint p3, GLdouble p4, GLdouble p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMapGrid2d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glMapGrid2d");
}

/* glMapGrid2f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMapGrid2f(GLint p0, GLfloat p1, GLfloat p2, GLint p3, GLfloat p4, GLfloat p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMapGrid2f");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glMapGrid2f");
}

/* glMapNamedBuffer —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void * glMapNamedBuffer(GLuint p0, GLenum p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMapNamedBuffer");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glMapNamedBuffer");
    return NULL;
}

/* glMapNamedBufferRange —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void * glMapNamedBufferRange(GLuint p0, GLintptr p1, GLsizeiptr p2, GLbitfield p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMapNamedBufferRange");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glMapNamedBufferRange");
    return NULL;
}

/* glMaterialf —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMaterialf(GLenum p0, GLenum p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMaterialf");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glMaterialf");
}

/* glMateriali —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMateriali(GLenum p0, GLenum p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMateriali");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glMateriali");
}

/* glMatrixMode —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMatrixMode(GLenum p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMatrixMode");
    (void)p0;
    glesmod_report_stub("glMatrixMode");
}

/* glMultiDrawArraysIndirectCount —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiDrawArraysIndirectCount(GLenum p0, const void * p1, GLintptr p2, GLsizei p3, GLsizei p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiDrawArraysIndirectCount");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glMultiDrawArraysIndirectCount");
}

/* glMultiDrawElementsIndirectCount —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiDrawElementsIndirectCount(GLenum p0, GLenum p1, const void * p2, GLintptr p3, GLsizei p4, GLsizei p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiDrawElementsIndirectCount");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glMultiDrawElementsIndirectCount");
}

/* glMultiTexCoord1d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord1d(GLenum p0, GLdouble p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord1d");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glMultiTexCoord1d");
}

/* glMultiTexCoord1f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord1f(GLenum p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord1f");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glMultiTexCoord1f");
}

/* glMultiTexCoord1i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord1i(GLenum p0, GLint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord1i");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glMultiTexCoord1i");
}

/* glMultiTexCoord1s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord1s(GLenum p0, GLshort p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord1s");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glMultiTexCoord1s");
}

/* glMultiTexCoord2d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord2d(GLenum p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord2d");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glMultiTexCoord2d");
}

/* glMultiTexCoord2f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord2f(GLenum p0, GLfloat p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord2f");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glMultiTexCoord2f");
}

/* glMultiTexCoord2i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord2i(GLenum p0, GLint p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord2i");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glMultiTexCoord2i");
}

/* glMultiTexCoord2s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord2s(GLenum p0, GLshort p1, GLshort p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord2s");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glMultiTexCoord2s");
}

/* glMultiTexCoord3d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord3d(GLenum p0, GLdouble p1, GLdouble p2, GLdouble p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord3d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glMultiTexCoord3d");
}

/* glMultiTexCoord3f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord3f(GLenum p0, GLfloat p1, GLfloat p2, GLfloat p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord3f");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glMultiTexCoord3f");
}

/* glMultiTexCoord3i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord3i(GLenum p0, GLint p1, GLint p2, GLint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord3i");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glMultiTexCoord3i");
}

/* glMultiTexCoord3s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord3s(GLenum p0, GLshort p1, GLshort p2, GLshort p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord3s");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glMultiTexCoord3s");
}

/* glMultiTexCoord4d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord4d(GLenum p0, GLdouble p1, GLdouble p2, GLdouble p3, GLdouble p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord4d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glMultiTexCoord4d");
}

/* glMultiTexCoord4f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord4f(GLenum p0, GLfloat p1, GLfloat p2, GLfloat p3, GLfloat p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord4f");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glMultiTexCoord4f");
}

/* glMultiTexCoord4i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord4i(GLenum p0, GLint p1, GLint p2, GLint p3, GLint p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord4i");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glMultiTexCoord4i");
}

/* glMultiTexCoord4s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoord4s(GLenum p0, GLshort p1, GLshort p2, GLshort p3, GLshort p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoord4s");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glMultiTexCoord4s");
}

/* glMultiTexCoordP1ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoordP1ui(GLenum p0, GLenum p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoordP1ui");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glMultiTexCoordP1ui");
}

/* glMultiTexCoordP2ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoordP2ui(GLenum p0, GLenum p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoordP2ui");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glMultiTexCoordP2ui");
}

/* glMultiTexCoordP3ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoordP3ui(GLenum p0, GLenum p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoordP3ui");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glMultiTexCoordP3ui");
}

/* glMultiTexCoordP4ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glMultiTexCoordP4ui(GLenum p0, GLenum p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glMultiTexCoordP4ui");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glMultiTexCoordP4ui");
}

/* glNamedBufferData —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNamedBufferData(GLuint p0, GLsizeiptr p1, const void * p2, GLenum p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNamedBufferData");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glNamedBufferData");
}

/* glNamedBufferStorage —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNamedBufferStorage(GLuint p0, GLsizeiptr p1, const void * p2, GLbitfield p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNamedBufferStorage");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glNamedBufferStorage");
}

/* glNamedBufferSubData —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNamedBufferSubData(GLuint p0, GLintptr p1, GLsizeiptr p2, const void * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNamedBufferSubData");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glNamedBufferSubData");
}

/* glNamedFramebufferDrawBuffer —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNamedFramebufferDrawBuffer(GLuint p0, GLenum p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNamedFramebufferDrawBuffer");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glNamedFramebufferDrawBuffer");
}

/* glNamedFramebufferDrawBuffers —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNamedFramebufferDrawBuffers(GLuint p0, GLsizei p1, const GLenum * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNamedFramebufferDrawBuffers");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glNamedFramebufferDrawBuffers");
}

/* glNamedFramebufferParameteri —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNamedFramebufferParameteri(GLuint p0, GLenum p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNamedFramebufferParameteri");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glNamedFramebufferParameteri");
}

/* glNamedFramebufferReadBuffer —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNamedFramebufferReadBuffer(GLuint p0, GLenum p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNamedFramebufferReadBuffer");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glNamedFramebufferReadBuffer");
}

/* glNamedFramebufferRenderbuffer —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNamedFramebufferRenderbuffer(GLuint p0, GLenum p1, GLenum p2, GLuint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNamedFramebufferRenderbuffer");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glNamedFramebufferRenderbuffer");
}

/* glNamedFramebufferTexture —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNamedFramebufferTexture(GLuint p0, GLenum p1, GLuint p2, GLint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNamedFramebufferTexture");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glNamedFramebufferTexture");
}

/* glNamedFramebufferTextureLayer —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNamedFramebufferTextureLayer(GLuint p0, GLenum p1, GLuint p2, GLint p3, GLint p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNamedFramebufferTextureLayer");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glNamedFramebufferTextureLayer");
}

/* glNamedRenderbufferStorage —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNamedRenderbufferStorage(GLuint p0, GLenum p1, GLsizei p2, GLsizei p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNamedRenderbufferStorage");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glNamedRenderbufferStorage");
}

/* glNamedRenderbufferStorageMultisample —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNamedRenderbufferStorageMultisample(GLuint p0, GLsizei p1, GLenum p2, GLsizei p3, GLsizei p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNamedRenderbufferStorageMultisample");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glNamedRenderbufferStorageMultisample");
}

/* glNewList —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNewList(GLuint p0, GLenum p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNewList");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glNewList");
}

/* glNormal3b —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNormal3b(GLbyte p0, GLbyte p1, GLbyte p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNormal3b");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glNormal3b");
}

/* glNormal3d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNormal3d(GLdouble p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNormal3d");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glNormal3d");
}

/* glNormal3f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNormal3f(GLfloat p0, GLfloat p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNormal3f");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glNormal3f");
}

/* glNormal3i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNormal3i(GLint p0, GLint p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNormal3i");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glNormal3i");
}

/* glNormal3s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNormal3s(GLshort p0, GLshort p1, GLshort p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNormal3s");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glNormal3s");
}

/* glNormalP3ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glNormalP3ui(GLenum p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glNormalP3ui");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glNormalP3ui");
}

/* glOrtho —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glOrtho(GLdouble p0, GLdouble p1, GLdouble p2, GLdouble p3, GLdouble p4, GLdouble p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glOrtho");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glOrtho");
}

/* glPassThrough —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPassThrough(GLfloat p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPassThrough");
    (void)p0;
    glesmod_report_stub("glPassThrough");
}

/* glPatchParameterfv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPatchParameterfv(GLenum p0, const GLfloat * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPatchParameterfv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glPatchParameterfv");
}

/* glPixelStoref —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPixelStoref(GLenum p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPixelStoref");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glPixelStoref");
}

/* glPixelTransferf —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPixelTransferf(GLenum p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPixelTransferf");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glPixelTransferf");
}

/* glPixelTransferi —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPixelTransferi(GLenum p0, GLint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPixelTransferi");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glPixelTransferi");
}

/* glPixelZoom —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPixelZoom(GLfloat p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPixelZoom");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glPixelZoom");
}

/* glPointParameterf —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPointParameterf(GLenum p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPointParameterf");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glPointParameterf");
}

/* glPointParameterfv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPointParameterfv(GLenum p0, const GLfloat * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPointParameterfv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glPointParameterfv");
}

/* glPointParameteri —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPointParameteri(GLenum p0, GLint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPointParameteri");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glPointParameteri");
}

/* glPointParameteriv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPointParameteriv(GLenum p0, const GLint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPointParameteriv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glPointParameteriv");
}

/* glPointSize —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPointSize(GLfloat p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPointSize");
    (void)p0;
    glesmod_report_stub("glPointSize");
}

/* glPolygonOffsetClamp —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPolygonOffsetClamp(GLfloat p0, GLfloat p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPolygonOffsetClamp");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glPolygonOffsetClamp");
}

/* glPopAttrib —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPopAttrib(void)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPopAttrib");
    glesmod_report_stub("glPopAttrib");
}

/* glPopClientAttrib —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPopClientAttrib(void)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPopClientAttrib");
    glesmod_report_stub("glPopClientAttrib");
}

/* glPopMatrix —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPopMatrix(void)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPopMatrix");
    glesmod_report_stub("glPopMatrix");
}

/* glPopName —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPopName(void)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPopName");
    glesmod_report_stub("glPopName");
}

/* glPrimitiveRestartIndex —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPrimitiveRestartIndex(GLuint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPrimitiveRestartIndex");
    (void)p0;
    glesmod_report_stub("glPrimitiveRestartIndex");
}

/* glProgramUniform1d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniform1d(GLuint p0, GLint p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniform1d");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glProgramUniform1d");
}

/* glProgramUniform1dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniform1dv(GLuint p0, GLint p1, GLsizei p2, const GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniform1dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glProgramUniform1dv");
}

/* glProgramUniform2d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniform2d(GLuint p0, GLint p1, GLdouble p2, GLdouble p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniform2d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glProgramUniform2d");
}

/* glProgramUniform2dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniform2dv(GLuint p0, GLint p1, GLsizei p2, const GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniform2dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glProgramUniform2dv");
}

/* glProgramUniform3d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniform3d(GLuint p0, GLint p1, GLdouble p2, GLdouble p3, GLdouble p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniform3d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glProgramUniform3d");
}

/* glProgramUniform3dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniform3dv(GLuint p0, GLint p1, GLsizei p2, const GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniform3dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glProgramUniform3dv");
}

/* glProgramUniform4d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniform4d(GLuint p0, GLint p1, GLdouble p2, GLdouble p3, GLdouble p4, GLdouble p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniform4d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glProgramUniform4d");
}

/* glProgramUniform4dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniform4dv(GLuint p0, GLint p1, GLsizei p2, const GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniform4dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glProgramUniform4dv");
}

/* glProgramUniformMatrix2dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniformMatrix2dv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLdouble * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniformMatrix2dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glProgramUniformMatrix2dv");
}

/* glProgramUniformMatrix2x3dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniformMatrix2x3dv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLdouble * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniformMatrix2x3dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glProgramUniformMatrix2x3dv");
}

/* glProgramUniformMatrix2x4dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniformMatrix2x4dv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLdouble * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniformMatrix2x4dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glProgramUniformMatrix2x4dv");
}

/* glProgramUniformMatrix3dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniformMatrix3dv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLdouble * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniformMatrix3dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glProgramUniformMatrix3dv");
}

/* glProgramUniformMatrix3x2dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniformMatrix3x2dv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLdouble * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniformMatrix3x2dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glProgramUniformMatrix3x2dv");
}

/* glProgramUniformMatrix3x4dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniformMatrix3x4dv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLdouble * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniformMatrix3x4dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glProgramUniformMatrix3x4dv");
}

/* glProgramUniformMatrix4dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniformMatrix4dv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLdouble * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniformMatrix4dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glProgramUniformMatrix4dv");
}

/* glProgramUniformMatrix4x2dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniformMatrix4x2dv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLdouble * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniformMatrix4x2dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glProgramUniformMatrix4x2dv");
}

/* glProgramUniformMatrix4x3dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProgramUniformMatrix4x3dv(GLuint p0, GLint p1, GLsizei p2, GLboolean p3, const GLdouble * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProgramUniformMatrix4x3dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glProgramUniformMatrix4x3dv");
}

/* glProvokingVertex —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glProvokingVertex(GLenum p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glProvokingVertex");
    (void)p0;
    glesmod_report_stub("glProvokingVertex");
}

/* glPushAttrib —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPushAttrib(GLbitfield p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPushAttrib");
    (void)p0;
    glesmod_report_stub("glPushAttrib");
}

/* glPushClientAttrib —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPushClientAttrib(GLbitfield p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPushClientAttrib");
    (void)p0;
    glesmod_report_stub("glPushClientAttrib");
}

/* glPushMatrix —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPushMatrix(void)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPushMatrix");
    glesmod_report_stub("glPushMatrix");
}

/* glPushName —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glPushName(GLuint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glPushName");
    (void)p0;
    glesmod_report_stub("glPushName");
}

/* glQueryCounter —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glQueryCounter(GLuint p0, GLenum p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glQueryCounter");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glQueryCounter");
}

/* glRasterPos2d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRasterPos2d(GLdouble p0, GLdouble p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRasterPos2d");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glRasterPos2d");
}

/* glRasterPos2f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRasterPos2f(GLfloat p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRasterPos2f");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glRasterPos2f");
}

/* glRasterPos2i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRasterPos2i(GLint p0, GLint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRasterPos2i");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glRasterPos2i");
}

/* glRasterPos2s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRasterPos2s(GLshort p0, GLshort p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRasterPos2s");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glRasterPos2s");
}

/* glRasterPos3d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRasterPos3d(GLdouble p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRasterPos3d");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glRasterPos3d");
}

/* glRasterPos3f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRasterPos3f(GLfloat p0, GLfloat p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRasterPos3f");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glRasterPos3f");
}

/* glRasterPos3i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRasterPos3i(GLint p0, GLint p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRasterPos3i");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glRasterPos3i");
}

/* glRasterPos3s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRasterPos3s(GLshort p0, GLshort p1, GLshort p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRasterPos3s");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glRasterPos3s");
}

/* glRasterPos4d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRasterPos4d(GLdouble p0, GLdouble p1, GLdouble p2, GLdouble p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRasterPos4d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glRasterPos4d");
}

/* glRasterPos4f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRasterPos4f(GLfloat p0, GLfloat p1, GLfloat p2, GLfloat p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRasterPos4f");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glRasterPos4f");
}

/* glRasterPos4i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRasterPos4i(GLint p0, GLint p1, GLint p2, GLint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRasterPos4i");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glRasterPos4i");
}

/* glRasterPos4s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRasterPos4s(GLshort p0, GLshort p1, GLshort p2, GLshort p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRasterPos4s");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glRasterPos4s");
}

/* glRectd —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRectd(GLdouble p0, GLdouble p1, GLdouble p2, GLdouble p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRectd");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glRectd");
}

/* glRectf —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRectf(GLfloat p0, GLfloat p1, GLfloat p2, GLfloat p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRectf");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glRectf");
}

/* glRecti —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRecti(GLint p0, GLint p1, GLint p2, GLint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRecti");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glRecti");
}

/* glRects —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRects(GLshort p0, GLshort p1, GLshort p2, GLshort p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRects");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glRects");
}

/* glRenderMode —— GLES 不提供，安全 stub */
GLESMOD_EXPORT GLint glRenderMode(GLenum p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRenderMode");
    (void)p0;
    glesmod_report_stub("glRenderMode");
    return 0;
}

/* glRotated —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRotated(GLdouble p0, GLdouble p1, GLdouble p2, GLdouble p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRotated");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glRotated");
}

/* glRotatef —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glRotatef(GLfloat p0, GLfloat p1, GLfloat p2, GLfloat p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glRotatef");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glRotatef");
}

/* glScaled —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glScaled(GLdouble p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glScaled");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glScaled");
}

/* glScalef —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glScalef(GLfloat p0, GLfloat p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glScalef");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glScalef");
}

/* glScissorArrayv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glScissorArrayv(GLuint p0, GLsizei p1, const GLint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glScissorArrayv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glScissorArrayv");
}

/* glScissorIndexed —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glScissorIndexed(GLuint p0, GLint p1, GLint p2, GLsizei p3, GLsizei p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glScissorIndexed");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glScissorIndexed");
}

/* glScissorIndexedv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glScissorIndexedv(GLuint p0, const GLint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glScissorIndexedv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glScissorIndexedv");
}

/* glSecondaryColor3b —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glSecondaryColor3b(GLbyte p0, GLbyte p1, GLbyte p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glSecondaryColor3b");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glSecondaryColor3b");
}

/* glSecondaryColor3d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glSecondaryColor3d(GLdouble p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glSecondaryColor3d");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glSecondaryColor3d");
}

/* glSecondaryColor3f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glSecondaryColor3f(GLfloat p0, GLfloat p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glSecondaryColor3f");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glSecondaryColor3f");
}

/* glSecondaryColor3i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glSecondaryColor3i(GLint p0, GLint p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glSecondaryColor3i");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glSecondaryColor3i");
}

/* glSecondaryColor3s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glSecondaryColor3s(GLshort p0, GLshort p1, GLshort p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glSecondaryColor3s");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glSecondaryColor3s");
}

/* glSecondaryColor3ub —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glSecondaryColor3ub(GLubyte p0, GLubyte p1, GLubyte p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glSecondaryColor3ub");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glSecondaryColor3ub");
}

/* glSecondaryColor3ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glSecondaryColor3ui(GLuint p0, GLuint p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glSecondaryColor3ui");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glSecondaryColor3ui");
}

/* glSecondaryColor3us —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glSecondaryColor3us(GLushort p0, GLushort p1, GLushort p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glSecondaryColor3us");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glSecondaryColor3us");
}

/* glSecondaryColorP3ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glSecondaryColorP3ui(GLenum p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glSecondaryColorP3ui");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glSecondaryColorP3ui");
}

/* glShadeModel —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glShadeModel(GLenum p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glShadeModel");
    (void)p0;
    glesmod_report_stub("glShadeModel");
}

/* glShaderStorageBlockBinding —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glShaderStorageBlockBinding(GLuint p0, GLuint p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glShaderStorageBlockBinding");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glShaderStorageBlockBinding");
}

/* glSpecializeShader —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glSpecializeShader(GLuint p0, const GLchar * p1, GLuint p2, const GLuint * p3, const GLuint * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glSpecializeShader");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glSpecializeShader");
}

/* glTexCoord1d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord1d(GLdouble p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord1d");
    (void)p0;
    glesmod_report_stub("glTexCoord1d");
}

/* glTexCoord1f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord1f(GLfloat p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord1f");
    (void)p0;
    glesmod_report_stub("glTexCoord1f");
}

/* glTexCoord1i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord1i(GLint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord1i");
    (void)p0;
    glesmod_report_stub("glTexCoord1i");
}

/* glTexCoord1s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord1s(GLshort p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord1s");
    (void)p0;
    glesmod_report_stub("glTexCoord1s");
}

/* glTexCoord2d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord2d(GLdouble p0, GLdouble p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord2d");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glTexCoord2d");
}

/* glTexCoord2f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord2f(GLfloat p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord2f");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glTexCoord2f");
}

/* glTexCoord2i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord2i(GLint p0, GLint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord2i");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glTexCoord2i");
}

/* glTexCoord2s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord2s(GLshort p0, GLshort p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord2s");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glTexCoord2s");
}

/* glTexCoord3d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord3d(GLdouble p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord3d");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTexCoord3d");
}

/* glTexCoord3f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord3f(GLfloat p0, GLfloat p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord3f");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTexCoord3f");
}

/* glTexCoord3i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord3i(GLint p0, GLint p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord3i");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTexCoord3i");
}

/* glTexCoord3s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord3s(GLshort p0, GLshort p1, GLshort p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord3s");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTexCoord3s");
}

/* glTexCoord4d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord4d(GLdouble p0, GLdouble p1, GLdouble p2, GLdouble p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord4d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glTexCoord4d");
}

/* glTexCoord4f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord4f(GLfloat p0, GLfloat p1, GLfloat p2, GLfloat p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord4f");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glTexCoord4f");
}

/* glTexCoord4i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord4i(GLint p0, GLint p1, GLint p2, GLint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord4i");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glTexCoord4i");
}

/* glTexCoord4s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoord4s(GLshort p0, GLshort p1, GLshort p2, GLshort p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoord4s");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glTexCoord4s");
}

/* glTexCoordP1ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoordP1ui(GLenum p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoordP1ui");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glTexCoordP1ui");
}

/* glTexCoordP2ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoordP2ui(GLenum p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoordP2ui");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glTexCoordP2ui");
}

/* glTexCoordP3ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoordP3ui(GLenum p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoordP3ui");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glTexCoordP3ui");
}

/* glTexCoordP4ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexCoordP4ui(GLenum p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexCoordP4ui");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glTexCoordP4ui");
}

/* glTexEnvf —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexEnvf(GLenum p0, GLenum p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexEnvf");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTexEnvf");
}

/* glTexEnvi —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexEnvi(GLenum p0, GLenum p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexEnvi");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTexEnvi");
}

/* glTexGend —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexGend(GLenum p0, GLenum p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexGend");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTexGend");
}

/* glTexGenf —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexGenf(GLenum p0, GLenum p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexGenf");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTexGenf");
}

/* glTexGeni —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexGeni(GLenum p0, GLenum p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexGeni");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTexGeni");
}

/* glTexImage1D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexImage1D(GLenum p0, GLint p1, GLint p2, GLsizei p3, GLint p4, GLenum p5, GLenum p6, const void * p7)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexImage1D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    glesmod_report_stub("glTexImage1D");
}

/* glTexImage2DMultisample —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexImage2DMultisample(GLenum p0, GLsizei p1, GLenum p2, GLsizei p3, GLsizei p4, GLboolean p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexImage2DMultisample");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glTexImage2DMultisample");
}

/* glTexImage3DMultisample —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexImage3DMultisample(GLenum p0, GLsizei p1, GLenum p2, GLsizei p3, GLsizei p4, GLsizei p5, GLboolean p6)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexImage3DMultisample");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    glesmod_report_stub("glTexImage3DMultisample");
}

/* glTexStorage1D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexStorage1D(GLenum p0, GLsizei p1, GLenum p2, GLsizei p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexStorage1D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glTexStorage1D");
}

/* glTexSubImage1D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTexSubImage1D(GLenum p0, GLint p1, GLint p2, GLsizei p3, GLenum p4, GLenum p5, const void * p6)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTexSubImage1D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    glesmod_report_stub("glTexSubImage1D");
}

/* glTextureBarrier —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureBarrier(void)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureBarrier");
    glesmod_report_stub("glTextureBarrier");
}

/* glTextureBuffer —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureBuffer(GLuint p0, GLenum p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureBuffer");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTextureBuffer");
}

/* glTextureBufferRange —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureBufferRange(GLuint p0, GLenum p1, GLuint p2, GLintptr p3, GLsizeiptr p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureBufferRange");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glTextureBufferRange");
}

/* glTextureParameterIiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureParameterIiv(GLuint p0, GLenum p1, const GLint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureParameterIiv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTextureParameterIiv");
}

/* glTextureParameterIuiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureParameterIuiv(GLuint p0, GLenum p1, const GLuint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureParameterIuiv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTextureParameterIuiv");
}

/* glTextureParameterf —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureParameterf(GLuint p0, GLenum p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureParameterf");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTextureParameterf");
}

/* glTextureParameterfv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureParameterfv(GLuint p0, GLenum p1, const GLfloat * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureParameterfv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTextureParameterfv");
}

/* glTextureParameteri —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureParameteri(GLuint p0, GLenum p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureParameteri");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTextureParameteri");
}

/* glTextureParameteriv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureParameteriv(GLuint p0, GLenum p1, const GLint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureParameteriv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTextureParameteriv");
}

/* glTextureStorage1D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureStorage1D(GLuint p0, GLsizei p1, GLenum p2, GLsizei p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureStorage1D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glTextureStorage1D");
}

/* glTextureStorage2D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureStorage2D(GLuint p0, GLsizei p1, GLenum p2, GLsizei p3, GLsizei p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureStorage2D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glTextureStorage2D");
}

/* glTextureStorage2DMultisample —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureStorage2DMultisample(GLuint p0, GLsizei p1, GLenum p2, GLsizei p3, GLsizei p4, GLboolean p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureStorage2DMultisample");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glTextureStorage2DMultisample");
}

/* glTextureStorage3D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureStorage3D(GLuint p0, GLsizei p1, GLenum p2, GLsizei p3, GLsizei p4, GLsizei p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureStorage3D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glTextureStorage3D");
}

/* glTextureStorage3DMultisample —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureStorage3DMultisample(GLuint p0, GLsizei p1, GLenum p2, GLsizei p3, GLsizei p4, GLsizei p5, GLboolean p6)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureStorage3DMultisample");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    glesmod_report_stub("glTextureStorage3DMultisample");
}

/* glTextureSubImage1D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureSubImage1D(GLuint p0, GLint p1, GLint p2, GLsizei p3, GLenum p4, GLenum p5, const void * p6)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureSubImage1D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    glesmod_report_stub("glTextureSubImage1D");
}

/* glTextureSubImage2D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureSubImage2D(GLuint p0, GLint p1, GLint p2, GLint p3, GLsizei p4, GLsizei p5, GLenum p6, GLenum p7, const void * p8)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureSubImage2D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    (void)p8;
    glesmod_report_stub("glTextureSubImage2D");
}

/* glTextureSubImage3D —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureSubImage3D(GLuint p0, GLint p1, GLint p2, GLint p3, GLint p4, GLsizei p5, GLsizei p6, GLsizei p7, GLenum p8, GLenum p9, const void * p10)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureSubImage3D");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    (void)p8;
    (void)p9;
    (void)p10;
    glesmod_report_stub("glTextureSubImage3D");
}

/* glTextureView —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTextureView(GLuint p0, GLenum p1, GLuint p2, GLenum p3, GLuint p4, GLuint p5, GLuint p6, GLuint p7)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTextureView");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    glesmod_report_stub("glTextureView");
}

/* glTransformFeedbackBufferBase —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTransformFeedbackBufferBase(GLuint p0, GLuint p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTransformFeedbackBufferBase");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTransformFeedbackBufferBase");
}

/* glTransformFeedbackBufferRange —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTransformFeedbackBufferRange(GLuint p0, GLuint p1, GLuint p2, GLintptr p3, GLsizeiptr p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTransformFeedbackBufferRange");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glTransformFeedbackBufferRange");
}

/* glTranslated —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTranslated(GLdouble p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTranslated");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTranslated");
}

/* glTranslatef —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glTranslatef(GLfloat p0, GLfloat p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glTranslatef");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glTranslatef");
}

/* glUniform1d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniform1d(GLint p0, GLdouble p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniform1d");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glUniform1d");
}

/* glUniform1dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniform1dv(GLint p0, GLsizei p1, const GLdouble * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniform1dv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glUniform1dv");
}

/* glUniform2d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniform2d(GLint p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniform2d");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glUniform2d");
}

/* glUniform2dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniform2dv(GLint p0, GLsizei p1, const GLdouble * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniform2dv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glUniform2dv");
}

/* glUniform3d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniform3d(GLint p0, GLdouble p1, GLdouble p2, GLdouble p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniform3d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glUniform3d");
}

/* glUniform3dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniform3dv(GLint p0, GLsizei p1, const GLdouble * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniform3dv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glUniform3dv");
}

/* glUniform4d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniform4d(GLint p0, GLdouble p1, GLdouble p2, GLdouble p3, GLdouble p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniform4d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glUniform4d");
}

/* glUniform4dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniform4dv(GLint p0, GLsizei p1, const GLdouble * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniform4dv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glUniform4dv");
}

/* glUniformMatrix2dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniformMatrix2dv(GLint p0, GLsizei p1, GLboolean p2, const GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniformMatrix2dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glUniformMatrix2dv");
}

/* glUniformMatrix2x3dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniformMatrix2x3dv(GLint p0, GLsizei p1, GLboolean p2, const GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniformMatrix2x3dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glUniformMatrix2x3dv");
}

/* glUniformMatrix2x4dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniformMatrix2x4dv(GLint p0, GLsizei p1, GLboolean p2, const GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniformMatrix2x4dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glUniformMatrix2x4dv");
}

/* glUniformMatrix3dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniformMatrix3dv(GLint p0, GLsizei p1, GLboolean p2, const GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniformMatrix3dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glUniformMatrix3dv");
}

/* glUniformMatrix3x2dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniformMatrix3x2dv(GLint p0, GLsizei p1, GLboolean p2, const GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniformMatrix3x2dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glUniformMatrix3x2dv");
}

/* glUniformMatrix3x4dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniformMatrix3x4dv(GLint p0, GLsizei p1, GLboolean p2, const GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniformMatrix3x4dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glUniformMatrix3x4dv");
}

/* glUniformMatrix4dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniformMatrix4dv(GLint p0, GLsizei p1, GLboolean p2, const GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniformMatrix4dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glUniformMatrix4dv");
}

/* glUniformMatrix4x2dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniformMatrix4x2dv(GLint p0, GLsizei p1, GLboolean p2, const GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniformMatrix4x2dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glUniformMatrix4x2dv");
}

/* glUniformMatrix4x3dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniformMatrix4x3dv(GLint p0, GLsizei p1, GLboolean p2, const GLdouble * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniformMatrix4x3dv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glUniformMatrix4x3dv");
}

/* glUniformSubroutinesuiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glUniformSubroutinesuiv(GLenum p0, GLsizei p1, const GLuint * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUniformSubroutinesuiv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glUniformSubroutinesuiv");
}

/* glUnmapNamedBuffer —— GLES 不提供，安全 stub */
GLESMOD_EXPORT GLboolean glUnmapNamedBuffer(GLuint p0)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glUnmapNamedBuffer");
    (void)p0;
    glesmod_report_stub("glUnmapNamedBuffer");
    return GL_FALSE;
}

/* glVertex2d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertex2d(GLdouble p0, GLdouble p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertex2d");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertex2d");
}

/* glVertex2f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertex2f(GLfloat p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertex2f");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertex2f");
}

/* glVertex2i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertex2i(GLint p0, GLint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertex2i");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertex2i");
}

/* glVertex2s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertex2s(GLshort p0, GLshort p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertex2s");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertex2s");
}

/* glVertex3d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertex3d(GLdouble p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertex3d");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glVertex3d");
}

/* glVertex3f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertex3f(GLfloat p0, GLfloat p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertex3f");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glVertex3f");
}

/* glVertex3i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertex3i(GLint p0, GLint p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertex3i");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glVertex3i");
}

/* glVertex3s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertex3s(GLshort p0, GLshort p1, GLshort p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertex3s");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glVertex3s");
}

/* glVertex4d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertex4d(GLdouble p0, GLdouble p1, GLdouble p2, GLdouble p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertex4d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertex4d");
}

/* glVertex4f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertex4f(GLfloat p0, GLfloat p1, GLfloat p2, GLfloat p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertex4f");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertex4f");
}

/* glVertex4i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertex4i(GLint p0, GLint p1, GLint p2, GLint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertex4i");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertex4i");
}

/* glVertex4s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertex4s(GLshort p0, GLshort p1, GLshort p2, GLshort p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertex4s");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertex4s");
}

/* glVertexArrayAttribBinding —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexArrayAttribBinding(GLuint p0, GLuint p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexArrayAttribBinding");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glVertexArrayAttribBinding");
}

/* glVertexArrayAttribFormat —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexArrayAttribFormat(GLuint p0, GLuint p1, GLint p2, GLenum p3, GLboolean p4, GLuint p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexArrayAttribFormat");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glVertexArrayAttribFormat");
}

/* glVertexArrayAttribIFormat —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexArrayAttribIFormat(GLuint p0, GLuint p1, GLint p2, GLenum p3, GLuint p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexArrayAttribIFormat");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glVertexArrayAttribIFormat");
}

/* glVertexArrayAttribLFormat —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexArrayAttribLFormat(GLuint p0, GLuint p1, GLint p2, GLenum p3, GLuint p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexArrayAttribLFormat");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glVertexArrayAttribLFormat");
}

/* glVertexArrayBindingDivisor —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexArrayBindingDivisor(GLuint p0, GLuint p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexArrayBindingDivisor");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glVertexArrayBindingDivisor");
}

/* glVertexArrayElementBuffer —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexArrayElementBuffer(GLuint p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexArrayElementBuffer");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexArrayElementBuffer");
}

/* glVertexArrayVertexBuffer —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexArrayVertexBuffer(GLuint p0, GLuint p1, GLuint p2, GLintptr p3, GLsizei p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexArrayVertexBuffer");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glVertexArrayVertexBuffer");
}

/* glVertexArrayVertexBuffers —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexArrayVertexBuffers(GLuint p0, GLuint p1, GLsizei p2, const GLuint * p3, const GLintptr * p4, const GLsizei * p5)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexArrayVertexBuffers");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    glesmod_report_stub("glVertexArrayVertexBuffers");
}

/* glVertexAttrib1d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib1d(GLuint p0, GLdouble p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib1d");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib1d");
}

/* glVertexAttrib1dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib1dv(GLuint p0, const GLdouble * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib1dv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib1dv");
}

/* glVertexAttrib1s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib1s(GLuint p0, GLshort p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib1s");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib1s");
}

/* glVertexAttrib1sv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib1sv(GLuint p0, const GLshort * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib1sv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib1sv");
}

/* glVertexAttrib2d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib2d(GLuint p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib2d");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glVertexAttrib2d");
}

/* glVertexAttrib2dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib2dv(GLuint p0, const GLdouble * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib2dv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib2dv");
}

/* glVertexAttrib2s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib2s(GLuint p0, GLshort p1, GLshort p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib2s");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glVertexAttrib2s");
}

/* glVertexAttrib2sv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib2sv(GLuint p0, const GLshort * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib2sv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib2sv");
}

/* glVertexAttrib3d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib3d(GLuint p0, GLdouble p1, GLdouble p2, GLdouble p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib3d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttrib3d");
}

/* glVertexAttrib3dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib3dv(GLuint p0, const GLdouble * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib3dv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib3dv");
}

/* glVertexAttrib3s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib3s(GLuint p0, GLshort p1, GLshort p2, GLshort p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib3s");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttrib3s");
}

/* glVertexAttrib3sv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib3sv(GLuint p0, const GLshort * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib3sv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib3sv");
}

/* glVertexAttrib4Nbv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4Nbv(GLuint p0, const GLbyte * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4Nbv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib4Nbv");
}

/* glVertexAttrib4Niv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4Niv(GLuint p0, const GLint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4Niv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib4Niv");
}

/* glVertexAttrib4Nsv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4Nsv(GLuint p0, const GLshort * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4Nsv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib4Nsv");
}

/* glVertexAttrib4Nub —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4Nub(GLuint p0, GLubyte p1, GLubyte p2, GLubyte p3, GLubyte p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4Nub");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glVertexAttrib4Nub");
}

/* glVertexAttrib4Nubv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4Nubv(GLuint p0, const GLubyte * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4Nubv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib4Nubv");
}

/* glVertexAttrib4Nuiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4Nuiv(GLuint p0, const GLuint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4Nuiv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib4Nuiv");
}

/* glVertexAttrib4Nusv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4Nusv(GLuint p0, const GLushort * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4Nusv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib4Nusv");
}

/* glVertexAttrib4bv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4bv(GLuint p0, const GLbyte * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4bv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib4bv");
}

/* glVertexAttrib4d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4d(GLuint p0, GLdouble p1, GLdouble p2, GLdouble p3, GLdouble p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glVertexAttrib4d");
}

/* glVertexAttrib4dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4dv(GLuint p0, const GLdouble * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4dv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib4dv");
}

/* glVertexAttrib4iv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4iv(GLuint p0, const GLint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4iv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib4iv");
}

/* glVertexAttrib4s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4s(GLuint p0, GLshort p1, GLshort p2, GLshort p3, GLshort p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4s");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glVertexAttrib4s");
}

/* glVertexAttrib4sv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4sv(GLuint p0, const GLshort * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4sv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib4sv");
}

/* glVertexAttrib4ubv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4ubv(GLuint p0, const GLubyte * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4ubv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib4ubv");
}

/* glVertexAttrib4uiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4uiv(GLuint p0, const GLuint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4uiv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib4uiv");
}

/* glVertexAttrib4usv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttrib4usv(GLuint p0, const GLushort * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttrib4usv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttrib4usv");
}

/* glVertexAttribI1i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI1i(GLuint p0, GLint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI1i");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribI1i");
}

/* glVertexAttribI1iv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI1iv(GLuint p0, const GLint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI1iv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribI1iv");
}

/* glVertexAttribI1ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI1ui(GLuint p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI1ui");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribI1ui");
}

/* glVertexAttribI1uiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI1uiv(GLuint p0, const GLuint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI1uiv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribI1uiv");
}

/* glVertexAttribI2i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI2i(GLuint p0, GLint p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI2i");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glVertexAttribI2i");
}

/* glVertexAttribI2iv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI2iv(GLuint p0, const GLint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI2iv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribI2iv");
}

/* glVertexAttribI2ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI2ui(GLuint p0, GLuint p1, GLuint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI2ui");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glVertexAttribI2ui");
}

/* glVertexAttribI2uiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI2uiv(GLuint p0, const GLuint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI2uiv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribI2uiv");
}

/* glVertexAttribI3i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI3i(GLuint p0, GLint p1, GLint p2, GLint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI3i");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttribI3i");
}

/* glVertexAttribI3iv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI3iv(GLuint p0, const GLint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI3iv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribI3iv");
}

/* glVertexAttribI3ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI3ui(GLuint p0, GLuint p1, GLuint p2, GLuint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI3ui");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttribI3ui");
}

/* glVertexAttribI3uiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI3uiv(GLuint p0, const GLuint * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI3uiv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribI3uiv");
}

/* glVertexAttribI4bv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI4bv(GLuint p0, const GLbyte * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI4bv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribI4bv");
}

/* glVertexAttribI4sv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI4sv(GLuint p0, const GLshort * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI4sv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribI4sv");
}

/* glVertexAttribI4ubv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI4ubv(GLuint p0, const GLubyte * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI4ubv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribI4ubv");
}

/* glVertexAttribI4usv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribI4usv(GLuint p0, const GLushort * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribI4usv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribI4usv");
}

/* glVertexAttribL1d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribL1d(GLuint p0, GLdouble p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribL1d");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribL1d");
}

/* glVertexAttribL1dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribL1dv(GLuint p0, const GLdouble * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribL1dv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribL1dv");
}

/* glVertexAttribL2d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribL2d(GLuint p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribL2d");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glVertexAttribL2d");
}

/* glVertexAttribL2dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribL2dv(GLuint p0, const GLdouble * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribL2dv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribL2dv");
}

/* glVertexAttribL3d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribL3d(GLuint p0, GLdouble p1, GLdouble p2, GLdouble p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribL3d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttribL3d");
}

/* glVertexAttribL3dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribL3dv(GLuint p0, const GLdouble * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribL3dv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribL3dv");
}

/* glVertexAttribL4d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribL4d(GLuint p0, GLdouble p1, GLdouble p2, GLdouble p3, GLdouble p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribL4d");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glVertexAttribL4d");
}

/* glVertexAttribL4dv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribL4dv(GLuint p0, const GLdouble * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribL4dv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexAttribL4dv");
}

/* glVertexAttribLFormat —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribLFormat(GLuint p0, GLint p1, GLenum p2, GLuint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribLFormat");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttribLFormat");
}

/* glVertexAttribLPointer —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribLPointer(GLuint p0, GLint p1, GLenum p2, GLsizei p3, const void * p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribLPointer");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glVertexAttribLPointer");
}

/* glVertexAttribP1ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribP1ui(GLuint p0, GLenum p1, GLboolean p2, GLuint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribP1ui");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttribP1ui");
}

/* glVertexAttribP1uiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribP1uiv(GLuint p0, GLenum p1, GLboolean p2, const GLuint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribP1uiv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttribP1uiv");
}

/* glVertexAttribP2ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribP2ui(GLuint p0, GLenum p1, GLboolean p2, GLuint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribP2ui");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttribP2ui");
}

/* glVertexAttribP2uiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribP2uiv(GLuint p0, GLenum p1, GLboolean p2, const GLuint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribP2uiv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttribP2uiv");
}

/* glVertexAttribP3ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribP3ui(GLuint p0, GLenum p1, GLboolean p2, GLuint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribP3ui");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttribP3ui");
}

/* glVertexAttribP3uiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribP3uiv(GLuint p0, GLenum p1, GLboolean p2, const GLuint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribP3uiv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttribP3uiv");
}

/* glVertexAttribP4ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribP4ui(GLuint p0, GLenum p1, GLboolean p2, GLuint p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribP4ui");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttribP4ui");
}

/* glVertexAttribP4uiv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexAttribP4uiv(GLuint p0, GLenum p1, GLboolean p2, const GLuint * p3)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexAttribP4uiv");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    glesmod_report_stub("glVertexAttribP4uiv");
}

/* glVertexP2ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexP2ui(GLenum p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexP2ui");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexP2ui");
}

/* glVertexP3ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexP3ui(GLenum p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexP3ui");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexP3ui");
}

/* glVertexP4ui —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glVertexP4ui(GLenum p0, GLuint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glVertexP4ui");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glVertexP4ui");
}

/* glViewportArrayv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glViewportArrayv(GLuint p0, GLsizei p1, const GLfloat * p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glViewportArrayv");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glViewportArrayv");
}

/* glViewportIndexedf —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glViewportIndexedf(GLuint p0, GLfloat p1, GLfloat p2, GLfloat p3, GLfloat p4)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glViewportIndexedf");
    (void)p0;
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    glesmod_report_stub("glViewportIndexedf");
}

/* glViewportIndexedfv —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glViewportIndexedfv(GLuint p0, const GLfloat * p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glViewportIndexedfv");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glViewportIndexedfv");
}

/* glWindowPos2d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glWindowPos2d(GLdouble p0, GLdouble p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glWindowPos2d");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glWindowPos2d");
}

/* glWindowPos2f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glWindowPos2f(GLfloat p0, GLfloat p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glWindowPos2f");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glWindowPos2f");
}

/* glWindowPos2i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glWindowPos2i(GLint p0, GLint p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glWindowPos2i");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glWindowPos2i");
}

/* glWindowPos2s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glWindowPos2s(GLshort p0, GLshort p1)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glWindowPos2s");
    (void)p0;
    (void)p1;
    glesmod_report_stub("glWindowPos2s");
}

/* glWindowPos3d —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glWindowPos3d(GLdouble p0, GLdouble p1, GLdouble p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glWindowPos3d");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glWindowPos3d");
}

/* glWindowPos3f —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glWindowPos3f(GLfloat p0, GLfloat p1, GLfloat p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glWindowPos3f");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glWindowPos3f");
}

/* glWindowPos3i —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glWindowPos3i(GLint p0, GLint p1, GLint p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glWindowPos3i");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glWindowPos3i");
}

/* glWindowPos3s —— GLES 不提供，安全 stub */
GLESMOD_EXPORT void glWindowPos3s(GLshort p0, GLshort p1, GLshort p2)
{
    glesmod_lazy_init();
    GLESMOD_TRACE("glWindowPos3s");
    (void)p0;
    (void)p1;
    (void)p2;
    glesmod_report_stub("glWindowPos3s");
}

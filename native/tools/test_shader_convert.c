/*
 * test_shader_convert.c -- 主机端测试：验证 GLSL -> GLSL ES 转换器
 *
 * 用途：本机跑通转换逻辑，避免「改完只能上真机看结果」的漫长循环。
 *
 * 测试的输入不是编造的，而是直接取自真实的第三方代码：
 *   net.neoforged.fancymodloader:earlydisplay:4.0.43
 *   net/neoforged/fml/earlydisplay/ElementShader.java
 * 真机上的启动失败正是这个着色器导致的。
 *
 * 编译运行（主机，非 Android）：
 *   gcc -std=c11 -DGLESMOD_HOST_TEST -o test_shader.exe \
 *       native/tools/test_shader_convert.c native/src/shader.c \
 *       native/src/core.c native/src/custom.c native/src/enum.c -lm
 *
 * LGPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 转换器入口（定义在 shader.c） */
char *glesmod_convert_shader_source(const char *src, int *out_len, int stage);

/* 阶段常量（与 gl_internal.h 保持一致，此处独立定义以免测试依赖内部头） */
#define STAGE_UNKNOWN   0
#define STAGE_VERTEX    0x8B31  /* GL_VERTEX_SHADER */
#define STAGE_FRAGMENT  0x8B30  /* GL_FRAGMENT_SHADER */

/*
 * 下面这些符号在 shader.c 中被引用，但主机测试不链接完整后端，
 * 因此在这里给出最小实现，避免链接失败。
 */
int glesmod_trace_enabled = 0;

void glesmod_degrade(int code, const char *detail) {
    (void)code;
    (void)detail;
}

/*
 * shader.c 现在会调用 glesmod_log 输出「规则 R 已建立用户函数签名表」
 * 这条一次性诊断（它同时是构建产物的内容标记，见 required_strings.txt）。
 * 主机测试不链接完整后端，所以必须在这里补一个空实现 ——
 * 否则链接期报 undefined reference to `glesmod_log`，
 * run_shader_tests.ps1 会抛 "shader test build failed" 并中止整个构建。
 * 空实现是合适的：单元测试断言的是转换结果，不是日志输出。
 */
void glesmod_log(const char *message) {
    (void)message;
}

void glesmod_trace_dump(const char *label, const char *text) {
    (void)label;
    (void)text;
}

/*
 * FML 早期显示窗口的顶点着色器原文。
 *
 * 逐字节取自 ElementShader.java 的文本块，因此保留了 Java 文本块
 * 的固有特征：以一个换行开头，每行带 25 个空格缩进。
 *
 * 报错 "ERROR: 0:12" 对应的正是第 12 行：
 *     gl_Position = vec4((position/screenSize) * 2 - 1, 0.0, 1.0);
 * 该行的 '*' 与 '-' 即 ES 1.00 不接受 vec2 * int 所致。
 */
static const char *FML_VERTEX =
    "\n"
    "                         #version 150 core\n"
    "                         in vec2 position;\n"
    "                         in vec2 tex;\n"
    "                         in vec4 colour;\n"
    "                         uniform vec2 screenSize;\n"
    "                         out vec2 fTex;\n"
    "                         out vec4 fColour;\n"
    "                         void main() {\n"
    "                             fTex = tex;\n"
    "                             fColour = colour;\n"
    "                             gl_Position = vec4((position/screenSize) * 2 - 1, 0.0, 1.0);\n"
    "                         }\n";

/* FML 的片元着色器原文（同一个类） */
static const char *FML_FRAGMENT =
    "\n"
    "                         #version 150 core\n"
    "                         uniform sampler2D tex;\n"
    "                         uniform int rendertype;\n"
    "                         in vec2 fTex;\n"
    "                         in vec4 fColour;\n"
    "                         out vec4 fragColor;\n"
    "\n"
    "                         void main() {\n"
    "                             if (rendertype == 0)\n"
    "                                    fragColor = vec4(1,1,1,texture(tex, fTex).r) * fColour;\n"
    "                             if (rendertype == 1)\n"
    "                                    fragColor = texture(tex, fTex) * fColour;\n"
    "                             if (rendertype == 2)\n"
    "                                    fragColor = fColour;\n"
    "                         }\n";

/* 简单的旧式桌面 GLSL 2D 纹理着色器（本项目必须支持的常见形态） */
static const char *LEGACY_DESKTOP =
    "#version 120\n"
    "varying vec2 vTexCoord;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(tex0, vTexCoord);\n"
    "}\n";

/*
 * 旧式顶点着色器：使用 attribute / varying。
 * ES 3.x 已移除这两个关键字，必须分别转换为 in / out。
 */
static const char *LEGACY_VERTEX =
    "#version 120\n"
    "attribute vec3 aPos;\n"
    "varying vec2 vTexCoord;\n"
    "void main() {\n"
    "    vTexCoord = aPos.xy;\n"
    "    gl_Position = vec4(aPos, 1.0);\n"
    "}\n";

static int failures = 0;

static void check(int cond, const char *what) {
    printf("  [%s] %s\n", cond ? "PASS" : "FAIL", what);
    if (!cond) failures++;
}

static void show(const char *title, const char *s) {
    printf("  ----- %s -----\n", title);
    const char *p = s;
    int n = 0;
    while (*p && n < 30) {
        const char *e = strchr(p, '\n');
        size_t len = e ? (size_t)(e - p) : strlen(p);
        printf("    %2d| %.*s\n", n, (int)len, p);
        n++;
        if (!e) break;
        p = e + 1;
    }
    printf("\n");
}

int main(void) {
    printf("=== GLSL ES 转换器主机端测试 ===\n\n");

    /* ---------------- 用例 1：FML 顶点着色器 ---------------- */
    printf("[1] FML 顶点着色器（真机启动失败的元凶）\n");
    {
        int len = 0;
        char *out = glesmod_convert_shader_source(FML_VERTEX, &len, STAGE_VERTEX);
        check(out != NULL, "转换返回非 NULL");
        if (out != NULL) {
            show("转换结果", out);

            /*
             * 关键断言：必须产生 #version 320 es。
             *
             * 【注意】这一条【不是】充分的。曾误以为 vec2 * int 在
             * ES 3.00+ 就合法，只要提升版本即可，结果真机仍然失败。
             * 实际上 ES（含 3.20）从不允许二元运算里的隐式 int->float
             * 转换 —— 这是语言规则，与版本无关。因此下面的字面量浮点化
             * 断言才是真正决定启动成败的那一条。
             */
            check(strstr(out, "#version 320 es") != NULL,
                  "输出含 '#version 320 es'");

            /* 原 #version 150 core 必须已被移除，否则两条指令冲突 */
            check(strstr(out, "150 core") == NULL,
                  "原有 '#version 150 core' 已移除");

            /* 顶点着色器无需 precision，但 gl_Position 赋值必须保留 */
            check(strstr(out, "gl_Position") != NULL,
                  "gl_Position 赋值保留");
            check(strstr(out, "screenSize") != NULL,
                  "uniform screenSize 保留");

            /*
             * 【本测试最关键的两项断言】
             *
             * 桌面 GLSL 允许 vec2 * 2（隐式 int->float），GLSL ES 不允许。
             * 真机上该表达式正是启动失败的直接原因：
             *   '*' : wrong operand types  no operation '*' exists that takes
             *         a left-hand operand of type '2-component vector of float'
             *         and a right operand of type 'const int'
             *
             * 因此转换器必须把 '2' 与 '1' 改写成 '2.0' / '1.0'。
             */
            check(strstr(out, "(position/screenSize) * 2.0 - 1.0") != NULL,
                  "整数字面量已浮点化: (position/screenSize) * 2.0 - 1.0");
            check(strstr(out, "* 2 -") == NULL,
                  "不再残留 '* 2 -'（该形式在 GLSL ES 中非法）");

            free(out);
        }
    }

    /* ---------------- 用例 2：FML 片元着色器 ---------------- */
    printf("\n[2] FML 片元着色器\n");
    {
        int len = 0;
        char *out = glesmod_convert_shader_source(FML_FRAGMENT, &len, STAGE_FRAGMENT);
        check(out != NULL, "转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "#version 320 es") != NULL,
                  "输出含 '#version 320 es'");
            check(strstr(out, "precision highp float;") != NULL,
                  "片元着色器含 precision 声明（ES 强制要求）");
            check(strstr(out, "texture(tex, fTex)") != NULL,
                  "texture() 调用保留");

            /*
             * 该着色器使用 in/out 与 texture()（ES 3.00 风格），
             * 不含 gl_FragColor，因此不应被注入 glesmod_FragColor，
             * 也不应被改写为 out 变量。
             */
            check(strstr(out, "glesmod_FragColor") == NULL,
                  "未误注入 glesmod_FragColor（该着色器本就用 out）");

            free(out);
        }
    }

    /* ---------------- 用例 3：旧式桌面 GLSL ---------------- */
    printf("\n[3] 旧式桌面 GLSL（texture2D + gl_FragColor）\n");
    {
        int len = 0;
        char *out = glesmod_convert_shader_source(LEGACY_DESKTOP, &len, STAGE_FRAGMENT);
        check(out != NULL, "转换返回非 NULL");
        if (out != NULL) {
            show("转换结果", out);
            check(strstr(out, "#version 320 es") != NULL,
                  "输出含 '#version 320 es'");
            check(strstr(out, "texture2D") == NULL,
                  "texture2D 已替换为 texture");
            check(strstr(out, "texture(tex0, vTexCoord)") != NULL,
                  "替换结果正确: texture(tex0, vTexCoord)");
            check(strstr(out, "gl_FragColor") == NULL,
                  "gl_FragColor 已替换");
            check(strstr(out, "glesmod_FragColor") != NULL,
                  "已注入并使用 glesmod_FragColor");
            check(strstr(out, "out vec4 glesmod_FragColor") != NULL,
                  "已声明 out vec4 glesmod_FragColor");
            free(out);
        }
    }

    /* ---------------- 用例 4：已是 ES 源码则不动 ---------------- */
    printf("\n[4] 已是 ES 源码（应原样返回）\n");
    {
        const char *es =
            "#version 300 es\n"
            "precision mediump float;\n"
            "out vec4 c;\n"
            "void main() { c = vec4(1.0); }\n";
        int len = 0;
        char *out = glesmod_convert_shader_source(es, &len, STAGE_VERTEX);
        check(out != NULL, "转换返回非 NULL");
        if (out != NULL) {
            check(strcmp(out, es) == 0, "内容与输入完全一致（未被改写）");
            check(strstr(out, "#version 300 es") != NULL,
                  "原 #version 300 es 保留");
            free(out);
        }
    }

    /* ---------------- 用例 5：阶段相关的限定符转换 ---------------- */
    printf("\n[5] 旧式限定符转换（attribute / varying，按阶段）\n");
    {
        /* 顶点着色器：attribute -> in, varying -> out */
        int len = 0;
        char *out = glesmod_convert_shader_source(LEGACY_VERTEX, &len, STAGE_VERTEX);
        check(out != NULL, "顶点：转换返回非 NULL");
        if (out != NULL) {
            show("顶点转换结果", out);
            check(strstr(out, "attribute") == NULL, "顶点：attribute 已替换");
            check(strstr(out, "in vec3 aPos") != NULL, "顶点：attribute -> in");
            check(strstr(out, "varying") == NULL, "顶点：varying 已替换");
            check(strstr(out, "out vec2 vTexCoord") != NULL,
                  "顶点：varying -> out（顶点里是输出）");
            free(out);
        }

        /* 片元着色器：varying -> in（方向与顶点相反） */
        out = glesmod_convert_shader_source(LEGACY_DESKTOP, &len, STAGE_FRAGMENT);
        check(out != NULL, "片元：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "varying") == NULL, "片元：varying 已替换");
            check(strstr(out, "in vec2 vTexCoord") != NULL,
                  "片元：varying -> in（片元里是输入）");
            free(out);
        }
    }

    /* ---------------- 用例 6：整数字面量浮点化（安全性） ---------------- */
    printf("\n[6] 整数字面量浮点化 —— 该改的改，不该改的绝不能碰\n");
    {
        /*
         * 混合场景：同时包含
         *   - 需要改写：vec2 * 2
         *   - 绝不能改写：int 声明、数组下标、ivec 构造、纹理单元参数、
         *                 预处理器行、标识符内的数字
         * 误改任何一项都会引入新的编译错误，比不改更糟。
         */
        static const char *MIXED =
            "#version 120\n"
            "uniform sampler2D tex;\n"
            "void main() {\n"
            "    int i = 0;\n"
            "    float arr[4];\n"
            "    vec2 uv = vec2(0.5, 0.5);\n"          /* 0.5 本已是浮点 */
            "    vec2 scaled = uv * 3;\n"              /* 需改写 -> 3.0 */
            "    ivec2 iv = ivec2(1, 2);\n"            /* 绝不能改写 */
            "    int k = i + 1;\n"                     /* 绝不能改写 */
            "    float f = arr[2];\n"                  /* 下标绝不能改写 */
            "    vec4 c = vec4(1, 0, 0, 1);\n"         /* vec4 构造内应改写 */
            "    vec4 t = texture(tex, uv);\n"
            "    gl_FragColor = t * f + c * 0.5;\n"
            "}\n";

        int len = 0;
        char *out = glesmod_convert_shader_source(MIXED, &len, STAGE_FRAGMENT);
        check(out != NULL, "转换返回非 NULL");
        if (out != NULL) {
            show("转换结果", out);

            /* --- 应改写的 --- */
            check(strstr(out, "uv * 3.0") != NULL,
                  "浮点上下文 vec2 运算: uv * 3 -> uv * 3.0");
            check(strstr(out, "vec4(1.0, 0.0, 0.0, 1.0)") != NULL,
                  "vec4 构造实参: vec4(1,0,0,1) -> vec4(1.0,0.0,0.0,1.0)");

            /* --- 绝不能改写的 --- */
            check(strstr(out, "int i = 0;") != NULL,
                  "int 声明保持 int i = 0（未变成 0.0）");
            check(strstr(out, "int k = i + 1;") != NULL,
                  "整型语句保持 int k = i + 1");
            check(strstr(out, "ivec2 iv = ivec2(1, 2);") != NULL,
                  "ivec2 构造内部保持整数");
            check(strstr(out, "arr[2]") != NULL,
                  "数组下标保持 arr[2]（未变成 arr[2.0]）");
            check(strstr(out, "sampler2D") != NULL,
                  "标识符 sampler2D 未被破坏（其 '2' 不得改写）");
            check(strstr(out, "#version 320 es") != NULL,
                  "预处理器行的 #version 正确生成");
            check(strstr(out, "320.0 es") == NULL,
                  "预处理器行内的 320 未被误改写");

            /*
             * vec2(0.5, 0.5) 里没有整数，应保持原样；
             * 若被改成 0.50.0 之类即为严重 bug。
             */
            check(strstr(out, "vec2(0.5, 0.5)") != NULL,
                  "已是浮点的字面量不被二次改写");

            free(out);
        }
    }

    /* ---------------- 用例 7：整数比较不得被误改写 ---------------- */
    printf("\n[7] 整数比较（FML 片元着色器的真实陷阱）\n");
    {
        /*
         * 这是 FML 片元着色器里的真实写法：
         *     uniform int rendertype;
         *     if (rendertype == 0) fragColor = vec4(1,1,1,...) * fColour;
         *
         * 该语句里既有 int 变量，又有 vec4 构造，因此整条语句会被判为
         * 「浮点上下文」。若不加区分地把所有整数都浮点化，会得到
         *     rendertype == 0.0
         * 即 int 与 float 比较 —— 在 GLSL ES 中同样非法。
         * 那就只是把编译失败的原因换了一个，问题并未解决。
         *
         * 正确行为：
         *   - `rendertype == 0` 中的 0 保持不变（整数比较）
         *   - `vec4(1,1,1,...)` 中的 1 —— 语句含 int 变量（rendertype），
         *     按「整数标识符」规则保持整数。
         *
         * 【关于 vec4(1,1,1,x)：这是刻意的行为变更】
         *   此处曾经断言必须浮点化为 vec4(1.0,1.0,1.0,...)。后来发现该
         *   「浮点化」逻辑会把 `float(i + 1)` 改坏成 `float(i + 1.0)`，
         *   因此改为：只要语句里出现 int 变量，整数字面量一律保持整数。
         *
         *   这不会破坏本行代码 —— 已用 glslang（glslc）实测确认
         *   `vec4(1, 1, 1, texture(...).r)` 在 #version 320 es 下合法
         *   （vecN 构造允许混合标量类型）。
         *   反之，若继续浮点化，则 `float(i + 1)` 这类真实代码会被改坏。
         *   两害相权，取保持整数。
         */
        static const char *INT_CMP =
            "#version 150 core\n"
            "uniform sampler2D tex;\n"
            "uniform int rendertype;\n"
            "in vec2 fTex;\n"
            "in vec4 fColour;\n"
            "out vec4 fragColor;\n"
            "void main() {\n"
            "    if (rendertype == 0)\n"
            "        fragColor = vec4(1,1,1,texture(tex, fTex).r) * fColour;\n"
            "    if (rendertype == 1)\n"
            "        fragColor = texelFetch(tex, ivec2(0, 0), 0) * fColour;\n"
            "}\n";

        int len = 0;
        char *out = glesmod_convert_shader_source(INT_CMP, &len, STAGE_FRAGMENT);
        check(out != NULL, "转换返回非 NULL");
        if (out != NULL) {
            show("转换结果", out);

            check(strstr(out, "rendertype == 0)") != NULL,
                  "整数比较 rendertype == 0 保持不变（不得变成 0.0）");
            check(strstr(out, "rendertype == 0.0") == NULL,
                  "未产生非法比较 rendertype == 0.0");
            check(strstr(out, "rendertype == 1)") != NULL,
                  "整数比较 rendertype == 1 保持不变");
            check(strstr(out, "vec4(1,1,1,") != NULL,
                  "含 int 变量的语句中 vec4(1,1,1,...) 保持整数"
                  "（glslang 实测该写法合法）");
            check(strstr(out, "ivec2(0, 0)") != NULL,
                  "ivec2 构造内部保持整数（未被误改为 0.0）");

            free(out);
        }
    }

    /* ---------------- 用例 8：残留 #version（MC 的 fog.glsl 陷阱） ---------- */
    printf("\n[8] 残留 #version —— Minecraft #moj_import 展开后的真实形态\n");
    {
        /*
         * 【这是真机 "could not preload shader position" 的根因】
         *
         * Minecraft 在调用 glShaderSource 之前会展开 #moj_import，
         * 把 include 文件的内容原样内联。而 MC 的 include 文件（fog.glsl）
         * 自己也以 `#version 150` 开头，于是实际交给 GL 的源码中，
         * 第二个 #version 出现在文件中部：
         *
         *     #version 150
         *
         *     #version 150        <- 从 fog.glsl 内联进来，位置非法
         *
         *     vec4 linear_fog(...) { ... }
         *
         * GLSL 规定 #version 只能出现在程序的第一个非注释位置，
         * 出现第二处即编译失败。
         *
         * 因此转换器必须移除【全部】#version 行，只保留自己生成的那一条。
         */
        static const char *INLINED =
            "#version 150\n"
            "\n"
            "#version 150\n"          /* fog.glsl 被内联后带来的第二处 */
            "\n"
            "vec4 linear_fog(vec4 c, float d, float s, float e, vec4 fc) {\n"
            "    return c;\n"
            "}\n"
            "\n"
            "void main() {\n"
            "    gl_Position = vec4(0.0);\n"
            "}\n";

        int len = 0;
        char *out = glesmod_convert_shader_source(INLINED, &len, STAGE_VERTEX);
        check(out != NULL, "转换返回非 NULL");
        if (out != NULL) {
            show("转换结果", out);

            /* 统计 #version 出现次数：必须恰好 1 次 */
            int count = 0;
            for (const char *p = out; (p = strstr(p, "#version")) != NULL; p++) {
                count++;
            }
            check(count == 1,
                  "输出中 #version 恰好出现 1 次（残留会导致编译失败）");
            check(strstr(out, "#version 320 es") != NULL,
                  "保留的是我们生成的 '#version 320 es'");
            check(strstr(out, "150") == NULL,
                  "残留的 '#version 150' 已被移除");
            check(strstr(out, "linear_fog") != NULL,
                  "include 内联进来的函数定义仍然保留");

            free(out);
        }
    }

    /* ---------------- 用例 9：#moj_import 展开（真机根因） ---------------- */
    printf("\n[9] #moj_import 展开 —— 真机 \"malformed preprocessor directive\" 的根因\n");
    {
        /*
         * 【这是决定性的真机证据】
         *
         * 驱动返回的错误是：
         *     ERROR: 0:4: '' : GLSL compile error: malformed preprocessor directive
         *
         * 而 position.vsh 原文（MC 核心着色器，1-based）为：
         *     1| #version 150
         *     2| (空)
         *     3| #moj_import <fog.glsl>
         *     4| (空)
         *
         * 我们的转换器去掉 #version、前面加 "#version 320 es"+空行后，
         * #moj_import 正好落到第 4 行 —— 与驱动报的行号完全吻合。
         *
         * 结论：#moj_import 是 Minecraft 自己的指令，GLSL 不支持，
         *       而它被【原样】送进了驱动。转换器必须自己展开它。
         */
        static const char *MOJ =
            "#version 150\n"
            "\n"
            "#moj_import <fog.glsl>\n"
            "\n"
            "in vec3 Position;\n"
            "uniform mat4 ProjMat;\n"
            "uniform mat4 ModelViewMat;\n"
            "uniform int FogShape;\n"
            "out float vertexDistance;\n"
            "void main() {\n"
            "    gl_Position = ProjMat * ModelViewMat * vec4(Position, 1.0);\n"
            "    vertexDistance = fog_distance(Position, FogShape);\n"
            "}\n";

        int len = 0;
        char *out = glesmod_convert_shader_source(MOJ, &len, STAGE_VERTEX);
        check(out != NULL, "转换返回非 NULL");
        if (out != NULL) {
            show("转换结果（前 20 行）", out);

            /* 核心断言：绝不能残留 #moj_import */
            check(strstr(out, "#moj_import") == NULL,
                  "不残留 #moj_import（残留即触发 malformed preprocessor directive）");
            check(strstr(out, "moj_import") == NULL,
                  "完全不出现 moj_import 字样");

            /* include 的函数必须真正被内联进来 */
            check(strstr(out, "float fog_distance(vec3 pos, int shape)") != NULL,
                  "fog.glsl 的 fog_distance 已内联");
            check(strstr(out, "vec4 linear_fog(") != NULL,
                  "fog.glsl 的 linear_fog 已内联");

            /* 内联内容里的 #version 也必须被清掉，只留一条 */
            int count = 0;
            for (const char *p = out; (p = strstr(p, "#version")) != NULL; p++) {
                count++;
            }
            check(count == 1, "输出中 #version 恰好 1 次（内联内容的 #version 已清除）");

            /* 主文件的代码不受影响 */
            check(strstr(out, "vertexDistance = fog_distance(Position, FogShape)") != NULL,
                  "主文件中对 fog_distance 的调用保留");

            free(out);
        }

        /* 引号写法与未知 include 的处理 */
        static const char *QUOTED_AND_UNKNOWN =
            "#version 150\n"
            "#moj_import \"matrix.glsl\"\n"
            "#moj_import <does_not_exist.glsl>\n"
            "void main() { gl_Position = vec4(0.0); }\n";
        out = glesmod_convert_shader_source(QUOTED_AND_UNKNOWN, &len, STAGE_VERTEX);
        check(out != NULL, "引号写法：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "mat2 mat2_rotate_z(float radians)") != NULL,
                  "引号写法 #moj_import \"matrix.glsl\" 也能展开");
            check(strstr(out, "#moj_import") == NULL,
                  "未知 include 不残留 #moj_import（改为注释占位）");
            free(out);
        }
    }

    /* ---------------- 用例 10：#line 指令不得被改写（真机根因） ------------- */
    printf("\n[10] #line 指令 —— 真机 \"malformed preprocessor directive\" 的真正原因\n");
    {
        /*
         * 【这是最终定位到的真机故障】
         *
         * 日志里拿到的原始源码（MC 交给 glShaderSource 的）：
         *      0| #version 150
         *      1| (空)
         *      2| #line 0 1
         *      3| (一条块注释，内容形如 version 150)
         *      4| (空)
         *      5| vec4 linear_fog(...) { ... }
         *      ...
         *     33| #line 3 0
         *
         * 我们送出的源码中变成了：
         *      #line 0 1.0        <- 非法！
         *
         * 即 `#line 0 1` 里的那个 `1`（行号偏移）被当作浮点上下文里的
         * 整数字面量改写成了 1.0。
         *
         * 两个缺陷叠加造成：
         *   1. 预处理器行保护只在扫描指针恰好停在 '#' 上时生效；
         *      而 #line 前面有换行，指针停在换行符上，保护被跳过。
         *   2. 判断「算术运算符」时，`1` 右侧跨行看到下一行的块注释开头，
         *      把那个斜杠当成了除法运算符。
         *
         * 修好后：所有以 # 开头的行整行原样保留。
         */
        static const char *WITH_LINE_DIRECTIVE =
            "#version 150\n"
            "\n"
            "#line 0 1\n"
            "/*#version 150*/\n"
            "\n"
            "vec4 linear_fog(vec4 inColor, float d, float s, float e, vec4 fc) {\n"
            "    return inColor;\n"
            "}\n"
            "#line 3 0\n"
            "\n"
            "in vec3 Position;\n"
            "out float vertexDistance;\n"
            "void main() {\n"
            "    gl_Position = vec4(Position, 1.0);\n"
            "    vertexDistance = 2.0 * 3.0;\n"
            "}\n";

        int len = 0;
        char *out = glesmod_convert_shader_source(WITH_LINE_DIRECTIVE, &len,
                                                  STAGE_VERTEX);
        check(out != NULL, "转换返回非 NULL");
        if (out != NULL) {
            show("转换结果", out);

            /* 核心断言：#line 的数字必须原样保留 */
            check(strstr(out, "#line 0 1\n") != NULL,
                  "#line 0 1 原样保留（行号数字不得被浮点化）");
            check(strstr(out, "#line 0 1.0") == NULL,
                  "未产生非法的 '#line 0 1.0'");
            check(strstr(out, "#line 3 0\n") != NULL,
                  "#line 3 0 原样保留");
            check(strstr(out, "#line 3 0.0") == NULL,
                  "未产生非法的 '#line 3 0.0'");

            /* 普通代码仍应正常处理 */
            check(strstr(out, "vertexDistance = 2.0 * 3.0;") != NULL,
                  "普通语句里的浮点字面量保持原样");
            check(strstr(out, "gl_Position = vec4(Position, 1.0);") != NULL,
                  "普通语句未被破坏");

            free(out);
        }

        /* 注释紧跟在数字后跨行出现，不得被当作除法 */
        static const char *COMMENT_AFTER_INT =
            "#version 150\n"
            "void main() {\n"
            "    float a = 2;\n"
            "    /* 这里的注释紧跟在上一行数字之后 */\n"
            "    float b = a * 3;\n"
            "    gl_Position = vec4(b);\n"
            "}\n";
        out = glesmod_convert_shader_source(COMMENT_AFTER_INT, &len, STAGE_VERTEX);
        check(out != NULL, "注释场景：转换返回非 NULL");
        if (out != NULL) {
            /*
             * 【重要纠正：这里原先的期望值是错的】
             *
             * 原断言是 `float a = 2;` 必须**保持整数**，理由写作
             * 「GLSL ES 允许初始化处的隐式 int->float」。
             * 用 glslang 实测，这句**直接报错**：
             *     '=' : cannot convert from 'const int' to 'temp highp float'
             * 真机 Adreno 亦然（BSL 的 `float cloudThickness = 5;`
             * 在真机上就是这一条错）。桌面 GLSL 才允许，ES 不允许。
             *
             * 正确行为：初始化处的整数字面量也要浮点化。
             * 相关证据见 native/tools/.cache/probe_int_float_init.vert。
             */
            check(strstr(out, "float a = 2.0;") != NULL,
                  "初始化中的整数字面量已浮点化（ES 不允许隐式 int->float）");

            /*
             * 而 `a * 3` 是【二元运算】，同样必须浮点化。
             * 下一行的注释以块注释开头，绝不能被误认为除法运算符
             * （那正是 #line 被改坏的成因）。
             */
            check(strstr(out, "float b = a * 3.0;") != NULL,
                  "二元运算 a * 3 仍被正确浮点化为 a * 3.0");

            check(strstr(out, "2.0.0") == NULL,
                  "未出现 2.0.0 之类的重复改写");
            free(out);
        }
    }

    /* ---------------- 用例 11：整数向量与浮点混合运算（渲染卡死根因） -------- */
    printf("\n[11] ivec 与 float 混合运算 —— 渲染管线整体失效的根因\n");
    {
        /*
         * 【这是导致「画面卡在 Mojang 徽标页」的根因】
         *
         * 驱动报：
         *   ERROR: 1:14: '/' : wrong operand types  no operation '/' exists
         *   that takes a left-hand operand of type 'in 2-component vector of int'
         *   and a right operand of type 'const float'
         *
         * 源头是 Minecraft 的 light.glsl（被 #moj_import 内联进各核心着色器）：
         *     vec4 minecraft_sample_lightmap(sampler2D lightMap, ivec2 uv) {
         *         return texture(lightMap, clamp(uv / 256.0, ...));
         *     }
         * `uv` 是 ivec2，`uv / 256.0` 在桌面 GLSL 合法、在 GLSL ES 非法。
         *
         * 因为核心着色器几乎都引用 light.glsl，它们会全部编译失败，
         * reloadShaders 抛异常，渲染管线失效 —— 逻辑照常运行
         * （背景音乐正常、按钮可点）但画面停止更新。
         */
        static const char *IVEC_FLOAT =
            "#version 150\n"
            "\n"
            "vec4 minecraft_sample_lightmap(sampler2D lightMap, ivec2 uv) {\n"
            "    return texture(lightMap, clamp(uv / 256.0, vec2(0.5 / 16.0),"
            " vec2(15.5 / 16.0)));\n"
            "}\n"
            "\n"
            "in ivec2 UV2;\n"
            "out vec4 vertexColor;\n"
            "void main() {\n"
            "    vertexColor = minecraft_sample_lightmap(Sampler2, UV2);\n"
            "}\n";

        int len = 0;
        char *out = glesmod_convert_shader_source(IVEC_FLOAT, &len, STAGE_VERTEX);
        check(out != NULL, "转换返回非 NULL");
        if (out != NULL) {
            show("转换结果", out);

            /*
             * 核心断言：ivec2 与浮点字面量做除法时必须被显式转成 vec2。
             * clamp 的第一个实参必须是浮点向量，否则 ES 报类型不匹配。
             */
            check(strstr(out, "clamp(vec2(uv) / 256.0") != NULL,
                  "ivec2 除法已改写为 vec2(uv) / 256.0");
            check(strstr(out, "clamp(uv / 256.0") == NULL,
                  "不再残留非法的 ivec2 / float");

            /* 参数声明本身不得被破坏 */
            check(strstr(out, "ivec2 uv") != NULL,
                  "参数声明 ivec2 uv 保持原样");

            /* 其它语句不受影响 */
            check(strstr(out, "in ivec2 UV2;") != NULL,
                  "in ivec2 UV2 声明保持原样");
            check(strstr(out, "minecraft_sample_lightmap(Sampler2, UV2)") != NULL,
                  "对函数的调用保持原样（此处 UV2 未参与浮点运算）");

            free(out);
        }

        /*
         * 反向断言：合法的整数运算绝不能被改写。
         * `ivec2 / 2` 是整数除法，语义与 `vec2(ivec2)/2.0` 完全不同。
         */
        static const char *INT_DIV_OK =
            "#version 150\n"
            "in ivec2 UV2;\n"
            "void main() {\n"
            "    ivec2 half = UV2 / 2;\n"
            "    gl_Position = vec4(vec2(half), 0.0, 1.0);\n"
            "}\n";
        out = glesmod_convert_shader_source(INT_DIV_OK, &len, STAGE_VERTEX);
        check(out != NULL, "整数除法场景：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "UV2 / 2;") != NULL,
                  "整数除法 UV2 / 2 未被改写（保持整数语义）");
            check(strstr(out, "vec2(UV2) / 2") == NULL,
                  "未把整数除法误改为浮点运算");
            free(out);
        }
    }

    /* ---------------- 用例 12：整数向量赋值给浮点向量 ---------------- */
    printf("\n[12] ivec 赋值给 vec —— \"cannot convert attribute int to varying float\"\n");
    {
        /*
         * 【真机驱动原话】
         *   ERROR: 0:34: 'assign' : cannot convert from
         *   'attribute 2-component vector of int' to
         *   'varying 2-component vector of float'
         *
         * 出自 rendertype_* 系列顶点着色器的末尾：
         *     out vec2 texCoord2;   // 浮点向量
         *     in  ivec2 UV2;        // 整数向量
         *     ...
         *     texCoord2 = UV2;      // 类型不匹配
         *
         * 桌面 GLSL 允许这种隐式转换，GLSL ES 不允许。
         */
        static const char *ASSIGN_IVEC_TO_VEC =
            "#version 150\n"
            "in ivec2 UV2;\n"
            "out vec2 texCoord2;\n"
            "void main() {\n"
            "    texCoord2 = UV2;\n"
            "}\n";

        int len = 0;
        char *out = glesmod_convert_shader_source(ASSIGN_IVEC_TO_VEC, &len,
                                                  STAGE_VERTEX);
        check(out != NULL, "转换返回非 NULL");
        if (out != NULL) {
            show("转换结果", out);
            check(strstr(out, "texCoord2 = vec2(UV2);") != NULL,
                  "texCoord2 = UV2 已改写为 texCoord2 = vec2(UV2)");
            check(strstr(out, "= UV2;") == NULL,
                  "不再残留无转换的直接赋值");
            free(out);
        }

        /*
         * 【反向断言 1】整数除法必须保持不变。
         * texelFetch 需要整数坐标，`UV2 / 16` 的整数语义不可改成浮点。
         */
        static const char *KEEP_INT_DIV =
            "#version 150\n"
            "in ivec2 UV2;\n"
            "uniform sampler2D Sampler2;\n"
            "out vec4 vertexColor;\n"
            "void main() {\n"
            "    vertexColor = texelFetch(Sampler2, UV2 / 16, 0);\n"
            "}\n";
        out = glesmod_convert_shader_source(KEEP_INT_DIV, &len, STAGE_VERTEX);
        check(out != NULL, "整数除法场景：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "texelFetch(Sampler2, UV2 / 16, 0)") != NULL,
                  "texelFetch 的整数坐标 UV2 / 16 未被改写");
            free(out);
        }

        /*
         * 【反向断言 2】左值本身是 ivec 时不得加 vecN()。
         * `ivec2 iv = UV2;` 是合法的整数赋值，加了 vec2() 反而出错。
         */
        static const char *IVEC_TO_IVEC =
            "#version 150\n"
            "in ivec2 UV2;\n"
            "void main() {\n"
            "    ivec2 iv = UV2;\n"
            "    gl_Position = vec4(vec2(iv), 0.0, 1.0);\n"
            "}\n";
        out = glesmod_convert_shader_source(IVEC_TO_IVEC, &len, STAGE_VERTEX);
        check(out != NULL, "整数左值场景：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "ivec2 iv = UV2;") != NULL,
                  "ivec2 左值的赋值保持不变（不误加 vec2）");
            free(out);
        }

        /*
         * 【反向断言 3】比较运算不得被误判为赋值。
         */
        static const char *COMPARE_NOT_ASSIGN =
            "#version 150\n"
            "in ivec2 UV2;\n"
            "out vec2 texCoord2;\n"
            "void main() {\n"
            "    if (UV2 == ivec2(0)) texCoord2 = vec2(0.0);\n"
            "}\n";
        out = glesmod_convert_shader_source(COMPARE_NOT_ASSIGN, &len,
                                            STAGE_VERTEX);
        check(out != NULL, "比较场景：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "vec2(UV2) == ivec2(0)") == NULL,
                  "比较运算 UV2 == ivec2(0) 未被误判为赋值");
            free(out);
        }
    }

    /* ---------------- 用例 13：整数变量参与时字面量保持整数 ---------------- */
    printf("\n[13] 整数表达式里的字面量不得浮点化 —— end_portal 着色器被改坏的真因\n");
    {
        /*
         * 【本用例来自 MC 真实的 rendertype_end_portal.fsh】
         *
         * 原文：
         *     for (int i = 0; i < EndPortalLayers; i++) {
         *         color += textureProj(Sampler1, texProj0 *
         *                  end_portal_layer(float(i + 1))).rgb * COLORS[i];
         *     }
         *
         * `i` 是 int，`float(i + 1)` 完全合法（float() 是转换构造函数）。
         *
         * 但此前的转换器只看到「1 左边是 '+'、语句里又有 float」，
         * 于是把 `1` 浮点化成 `1.0`，产出 `float(i + 1.0)` ——
         * 这是 int + float，GLSL ES 直接拒绝（已用 glslang 实测确认）。
         *
         * 也就是说：转换器把本来合法的着色器改坏了。
         */
        static const char *FLOAT_OF_INT_EXPR =
            "#version 150\n"
            "uniform int EndPortalLayers;\n"
            "vec4 end_portal_layer(float layer) { return vec4(layer); }\n"
            "out vec4 fragColor;\n"
            "void main() {\n"
            "    for (int i = 0; i < EndPortalLayers; i++) {\n"
            "        fragColor += end_portal_layer(float(i + 1));\n"
            "    }\n"
            "}\n";

        int len = 0;
        char *out = glesmod_convert_shader_source(FLOAT_OF_INT_EXPR, &len,
                                                  STAGE_FRAGMENT);
        check(out != NULL, "转换返回非 NULL");
        if (out != NULL) {
            show("转换结果", out);
            check(strstr(out, "float(i + 1)") != NULL,
                  "float(i + 1) 保持原样（未被改成 float(i + 1.0)）");
            check(strstr(out, "i + 1.0") == NULL,
                  "未产生非法的 int + float 表达式");
            check(strstr(out, "for (int i = 0;") != NULL,
                  "for 循环头的 int i = 0 未被浮点化");
            free(out);
        }

        /*
         * 【反向断言】没有整数变量参与时，浮点上下文里的整数字面量
         * 仍然必须浮点化 —— 否则会回归到 GLSL ES 的
         * "int + float"（桌面允许、ES 禁止），即最初 FML 着色器的故障。
         */
        static const char *PURE_FLOAT_EXPR =
            "#version 150\n"
            "in vec2 position;\n"
            "uniform vec2 screenSize;\n"
            "void main() {\n"
            "    gl_Position = vec4((position/screenSize) * 2 - 1, 0.0, 1.0);\n"
            "}\n";
        out = glesmod_convert_shader_source(PURE_FLOAT_EXPR, &len, STAGE_VERTEX);
        check(out != NULL, "纯浮点场景：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "* 2.0 - 1.0") != NULL,
                  "无 int 变量时仍正确浮点化：* 2 -> * 2.0");
            free(out);
        }

        /*
         * 【反向断言】浮点构造实参里的整数字面量仍应浮点化。
         * glslang 实测确认 vec2(0,0)/vec4(1,1,1,1) 本身合法，
         * 但浮点化后语义一致且更安全，保持既有行为。
         */
        static const char *CTOR_ARGS =
            "#version 150\n"
            "out vec4 c;\n"
            "void main() { c = vec4(1, 1, 1, 1); }\n";
        out = glesmod_convert_shader_source(CTOR_ARGS, &len, STAGE_FRAGMENT);
        check(out != NULL, "构造实参场景：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "vec4(1.0, 1.0, 1.0, 1.0)") != NULL,
                  "浮点构造实参内的整数仍然浮点化");
            free(out);
        }
    }

    /* ---------------- 用例 14：NULL 安全性 ---------------- */
    printf("\n[14] 健壮性\n");
    {
        check(glesmod_convert_shader_source(NULL, NULL, STAGE_VERTEX) == NULL,
              "输入 NULL 返回 NULL 而非崩溃");
    }

    /*
     * ---------------- 用例 15：ES 采样器精度限定符 ----------------
     *
     * GLSL ES 里除 sampler2D / samplerCube 之外的采样器类型**都没有默认精度**，
     * 而桌面 GLSL 全部有默认精度。所以「桌面能编译」的着色器一旦被我们改标成
     * #version 320 es 就必然失败。
     *
     * 【真实故障】Sodium 0.8.13 的区块顶点着色器含
     *     uniform isamplerBuffer u_SectionTimeInfo;
     * 驱动报 'sampler/image' : type requires declaration of default precision
     * qualifier -> Sodium 抛 RuntimeException -> 进入世界直接崩溃。
     */
    printf("\n[15] ES 采样器精度注入（Sodium 区块着色器崩溃根因）\n");
    {
        static const char *SODIUM_VSH =
            "#version 330\n"
            "#extension GL_ARB_separate_shader_objects : require\n"
            "layout(std140) uniform u_Globals {\n"
            "    mat4 u_ProjectionMatrix;\n"
            "    mat4 u_ModelViewMatrix;\n"
            "    bool u_UseRGSS;\n"
            "};\n"
            "uniform isamplerBuffer u_SectionTimeInfo;\n"
            "uniform sampler2D u_LightTex;\n"
            "layout(location = 0) out vec4 v_Color;\n"
            "void main() {\n"
            "    int chunkFade = texelFetch(u_SectionTimeInfo, 0).r;\n"
            "    v_Color = texture(u_LightTex, vec2(0.5));\n"
            "    gl_Position = u_ProjectionMatrix * u_ModelViewMatrix "
            "* vec4(0.0);\n"
            "}\n";

        int len = 0;
        char *out = glesmod_convert_shader_source(SODIUM_VSH, &len,
                                                  STAGE_VERTEX);
        check(out != NULL, "Sodium 顶点着色器：转换返回非 NULL");
        if (out != NULL) {
            show("转换结果", out);
            check(strstr(out, "precision highp isamplerBuffer;") != NULL,
                  "顶点阶段注入了 isamplerBuffer 的精度声明（崩溃修复）");
            check(strstr(out, "precision highp sampler2D;") != NULL,
                  "顶点阶段注入了 sampler2D 的精度声明");
            check(strstr(out, "GL_ARB_separate_shader_objects") == NULL,
                  "桌面专属 #extension 已被移除（ES 里 require 会报错）");
            check(strstr(out, "u_SectionTimeInfo") != NULL,
                  "着色器主体未被破坏");
            check(strstr(out, "texelFetch") != NULL,
                  "texelFetch 调用保留");
            check(strstr(out, "#version 320 es") != NULL,
                  "版本已改为 320 es");
            free(out);
        }

        /* 【反向断言】未使用的采样器类型不应被声明，避免输出膨胀 */
        static const char *PLAIN_VSH =
            "#version 150\n"
            "in vec3 Position;\n"
            "void main() { gl_Position = vec4(Position, 1.0); }\n";
        out = glesmod_convert_shader_source(PLAIN_VSH, &len, STAGE_VERTEX);
        check(out != NULL, "无采样器场景：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "precision highp isamplerBuffer;") == NULL,
                  "未用到 isamplerBuffer 时不注入其精度声明");
            check(strstr(out, "precision highp sampler2D;") == NULL,
                  "未用到 sampler2D 时不注入其精度声明");
            free(out);
        }

        /* 【边界】标识符内的子串不应被误判为「用到了该采样器类型」 */
        static const char *SUBSTR_VSH =
            "#version 150\n"
            "// a comment mentioning mysampler2Dx\n"
            "in vec3 Position;\n"
            "void main() { gl_Position = vec4(Position, 1.0); }\n";
        out = glesmod_convert_shader_source(SUBSTR_VSH, &len, STAGE_VERTEX);
        check(out != NULL, "子串场景：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "precision highp sampler2D;") == NULL,
                  "标识符子串不触发精度注入（按标识符边界匹配）");
            free(out);
        }

        /* 片元阶段同样需要采样器精度（此前已注入 float/int，补采样器） */
        static const char *FRAG_SAMPLER =
            "#version 150\n"
            "in vec2 UV0;\n"
            "uniform sampler2D Sampler0;\n"
            "out vec4 c;\n"
            "void main() { c = texture(Sampler0, UV0); }\n";
        out = glesmod_convert_shader_source(FRAG_SAMPLER, &len, STAGE_FRAGMENT);
        check(out != NULL, "片元采样器场景：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "precision highp sampler2D;") != NULL,
                  "片元阶段也注入采样器精度");
            check(strstr(out, "precision highp float;") != NULL,
                  "片元阶段仍保留 float 精度声明");
            free(out);
        }
    }

    /*
     * ---------------- 用例 16：Sodium 区块着色器的整数/浮点混算 ----------------
     *
     * 【本用例来自真机崩溃】Sodium 0.8.13 的 block_layer_opaque.vsh +
     * chunk_vertex.glsl 里有这样的代码：
     *
     *     uvec3 _get_relative_chunk_coord(uint pos) {
     *         return uvec3(pos) >> uvec3(5u, 0u, 2u) & uvec3(7u, 3u, 7u);
     *     }
     *     vec3 _get_draw_translation(uint pos) {
     *         return _get_relative_chunk_coord(pos) * vec3(16.0);
     *     }
     *     vec3 translation = u_RegionOffset + _get_draw_translation(_draw_id);
     *
     * 三处「整数域操作数 × 浮点向量」—— 桌面 GLSL 合法，**GLSL ES 非法**
     * （uint 与 float 之间没有隐式转换）。glslang 实测原话：
     *     '*' : no operation '*' exists that takes a left-hand operand of type
     *           '3-component vector of uint' and a right operand of type
     *           'const 3-component vector of float'
     * 先前实现只匹配「声明为 ivecN/uvecN 的变量」，覆盖不到：
     *   (1) 构造函数 uvec3(pos)      (2) 函数返回值      (3) uniform uint 标量
     */
    printf("\n[16] Sodium 区块着色器的整数/浮点混算（真机崩溃根因）\n");
    {
        static const char *SODIUM_CHUNK_VSH =
            "#version 330\n"
            "uniform mat4 u_ProjectionMatrix;\n"
            "uniform mat4 u_ModelViewMatrix;\n"
            "uniform vec3 u_RegionOffset;\n"
            "uniform uint u_RegionID;\n"
            "uniform int u_CurrentTime;\n"
            "uvec3 _get_relative_chunk_coord(uint pos) {\n"
            "    return uvec3(pos) >> uvec3(5u, 0u, 2u) & uvec3(7u, 3u, 7u);\n"
            "}\n"
            "vec3 _get_draw_translation(uint pos) {\n"
            "    return _get_relative_chunk_coord(pos) * vec3(16.0);\n"
            "}\n"
            "void main() {\n"
            "    uint _draw_id = 3u;\n"
            "    vec3 translation = u_RegionOffset + "
            "_get_draw_translation(_draw_id);\n"
            "    vec3 position = vec3(1.0) + translation;\n"
            "    gl_Position = u_ProjectionMatrix * u_ModelViewMatrix "
            "* vec4(position, 1.0);\n"
            "}\n";

        int len = 0;
        char *out = glesmod_convert_shader_source(SODIUM_CHUNK_VSH, &len,
                                                  STAGE_VERTEX);
        check(out != NULL, "Sodium 区块顶点着色器：转换返回非 NULL");
        if (out != NULL) {
            show("转换结果", out);
            check(strstr(out, "vec3(_get_relative_chunk_coord(pos)) * "
                              "vec3(16.0)") != NULL,
                  "函数返回值 uvec3 -> vec3(...) 包裹（情形 b）");
            check(strstr(out, "_get_relative_chunk_coord(pos) * vec3(16.0)")
                      == NULL ||
                  strstr(out, "vec3(_get_relative_chunk_coord(pos))")
                      != NULL,
                  "未留下裸的 uvec3 返回值参与乘法");
            check(strstr(out, "uvec3(pos) >> uvec3(5u, 0u, 2u)") != NULL,
                  "合法的 uvec 移位/按位与保持原样（不得改写）");
            check(strstr(out, "5u") != NULL && strstr(out, "2u") != NULL,
                  "uint 字面量后缀 5u/2u 保持原样");
            free(out);
        }

        /* 单独验证构造函数与 uint 标量两种形态 */
        static const char *CTOR_CASE =
            "#version 330\n"
            "void main() {\n"
            "    uint d = 3u;\n"
            "    vec3 t = uvec3(d) * vec3(16.0);\n"
            "    gl_Position = vec4(t, 1.0);\n"
            "}\n";
        out = glesmod_convert_shader_source(CTOR_CASE, &len, STAGE_VERTEX);
        check(out != NULL, "构造函数形态：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "vec3(uvec3(d)) * vec3(16.0)") != NULL,
                  "uvec3(d) 构造函数 -> vec3(uvec3(d)) 包裹（情形 a）");
            free(out);
        }

        static const char *SCALAR_CASE =
            "#version 330\n"
            "uniform uint u_RegionID;\n"
            "void main() {\n"
            "    vec3 t = u_RegionID * vec3(16.0);\n"
            "    gl_Position = vec4(t, 1.0);\n"
            "}\n";
        out = glesmod_convert_shader_source(SCALAR_CASE, &len, STAGE_VERTEX);
        check(out != NULL, "uint 标量形态：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "float(u_RegionID) * vec3(16.0)") != NULL,
                  "uniform uint 标量 -> float(...) 包裹（情形 c）");
            free(out);
        }

        /*
         * 【真机夹具回归】Sodium chunk_vertex.glsl 里的
         *     _vert_position = (_deinterleave_u20x3(a_Position) * VERTEX_SCALE)
         *                      + VERTEX_OFFSET;
         * `_deinterleave_u20x3` 返回 uvec3，VERTEX_SCALE 是 float。
         * => uvec3 * float 标量，GLSL ES 非法。
         *
         * 这一处曾被漏掉：右操作数判定只覆盖「浮点字面量 / vecN(...) /
         * 浮点向量变量」，唯独漏了最简单的「浮点标量变量」。
         * 结果是 2026-09-28 22:50 那轮真机仍然崩溃在同一处。
         */
        static const char *FLOAT_SCALAR_CASE =
            "#version 330 core\n"
            "const float VERTEX_SCALE = 0.5;\n"
            "const float VERTEX_OFFSET = -8.0;\n"
            "in uvec2 a_Position;\n"
            "vec3 _vert_position;\n"
            "uvec3 _deinterleave_u20x3(uvec2 data) {\n"
            "    return uvec3(data.x) >> uvec3(0u, 10u, 20u);\n"
            "}\n"
            "void _vert_init() {\n"
            "    _vert_position = (_deinterleave_u20x3(a_Position) "
            "* VERTEX_SCALE) + VERTEX_OFFSET;\n"
            "}\n"
            "void main() { _vert_init(); "
            "gl_Position = vec4(_vert_position, 1.0); }\n";
        out = glesmod_convert_shader_source(FLOAT_SCALAR_CASE, &len,
                                            STAGE_VERTEX);
        check(out != NULL, "uvec3 * float标量：转换返回非 NULL");
        if (out != NULL) {
            show("转换结果", out);
            check(strstr(out, "(vec3(_deinterleave_u20x3(a_Position)) "
                              "* VERTEX_SCALE)") != NULL,
                  "uvec3返回值 * float标量 -> vec3(...) 包裹（真机崩溃根因）");
            check(strstr(out, "_deinterleave_u20x3(a_Position) "
                              "* VERTEX_SCALE") == NULL ||
                  strstr(out, "vec3(_deinterleave_u20x3(a_Position))")
                      != NULL,
                  "未留下裸的 uvec3 与 float 相乘");
            free(out);
        }

        /*
         * 【反向断言】纯整数运算绝不能被加包裹 ——
         * 否则会改变语义（例如 texelFetch 需要整数坐标）。
         */
        static const char *PURE_INT_CASE =
            "#version 330\n"
            "uniform isamplerBuffer u_T;\n"
            "uniform ivec2 UV2;\n"
            "uniform ivec2 Size;\n"
            "void main() {\n"
            "    ivec2 uv = UV2 / 16;\n"
            "    ivec2 q = UV2 + Size;\n"
            "    int fade = texelFetch(u_T, 0).r;\n"
            "    gl_Position = vec4(vec2(uv + q) + vec2(float(fade)), 1.0);\n"
            "}\n";
        out = glesmod_convert_shader_source(PURE_INT_CASE, &len, STAGE_VERTEX);
        check(out != NULL, "纯整数运算：转换返回非 NULL");
        if (out != NULL) {
            check(strstr(out, "UV2 / 16") != NULL,
                  "ivec2 / 整数 保持整数除法（texelFetch 依赖）");
            check(strstr(out, "vec2(UV2) / 16") == NULL,
                  "未把整数除法误改成浮点");
            free(out);
        }
    }

    /*
     * ---------------- 用例 17：规则 N（uint 与整数字面量） ----------------
     *
     * 【真机证据】Flywheel 1.0.6 的 packed_material.glsl：
     *     void _flw_unpackMaterialProperties(uint p, out FlwMaterial m) {
     *         m.ambientOcclusion = (p & _FLW_AMBIENT_OCCLUSION_MASK) != 0;
     *     }
     * Adreno 驱动报：
     *     error: wrong operand types  no operation '!=' exists that takes a
     *            left-hand operand of type 'uint' and a right operand of type
     *            'const int' (or there is no acceptable conversion)
     *
     * `p` 与 mask 都是 uint，`0` 是 int。桌面 GLSL 允许隐式整型转换，
     * GLSL ES 明确禁止，因此必须在字面量上补 `u` 后缀。
     *
     * 下列每一行的「合法 / 非法」判定均由 glslc（ES 3.20）逐条实测得出，
     * 见 native/tools/es-uint-op-probe.ps1 与 es-uint-probe.ps1。
     */
    printf("\n[17] 规则 N：uint 标量旁的整数字面量必须补 u 后缀\n");
    {
        static const char *const N_TEMPLATE =
            "#version 330 core\n"
            "const uint MASK = 3u;\n"
            "uniform uint p;\n"
            "void main() {\n"
            "    float ao = 0.0;\n"
            "    %s\n"
            "}\n";

        /* 每项：语句 -> 必须出现的改写结果 */
        static const struct { const char *stmt; const char *want; } N_POS[] = {
            /* 这一类就是真机 Flywheel 报错的那一行 */
            { "ao = (p & MASK) != 0 ? 1.0 : 0.0;", "!= 0u" },
            { "ao = (p & MASK) == 0 ? 1.0 : 0.0;", "== 0u" },
            { "ao = p > 0 ? 1.0 : 0.0;",           "> 0u"  },
            { "ao = p < 1 ? 1.0 : 0.0;",           "< 1u"  },
            { "ao = p >= 0 ? 1.0 : 0.0;",          ">= 0u" },
            { "ao = p <= 1 ? 1.0 : 0.0;",          "<= 1u" },
            { "ao = (p & 0) != 0 ? 1.0 : 0.0;",    "& 0u"  },
            { "ao = (p | 0) != 0 ? 1.0 : 0.0;",    "| 0u"  },
            { "ao = (p ^ 0) != 0 ? 1.0 : 0.0;",    "^ 0u"  },
            { "ao = p + 0 == 0u ? 1.0 : 0.0;",     "+ 0u"  },
            { "ao = p - 1 == 0u ? 1.0 : 0.0;",     "- 1u"  },
            { "ao = p * 0 == 0u ? 1.0 : 0.0;",     "* 0u"  },
            { "ao = p % 1 == 0u ? 1.0 : 0.0;",     "% 1u"  },
            /* 带 u 后缀的字面量作为左操作数：类型确定是 uint */
            { "ao = (0xFFu & p) != 0 ? 1.0 : 0.0;", "!= 0u" },
            /* 声明初始化 / 普通赋值 / 复合赋值 */
            { "uint r = 0; ao = r != 0.0 ? 1.0 : 0.0;", "uint r = 0u" },
            { "uint r; r = 0; ao = float(r);",          "r = 0u" },
            { "uint r = 0u; r += 0; ao = float(r);",    "+= 0u" },
        };
        static const size_t N_POS_COUNT = sizeof(N_POS) / sizeof(N_POS[0]);

        char src[768];
        for (size_t i = 0; i < N_POS_COUNT; i++) {
            snprintf(src, sizeof src, N_TEMPLATE, N_POS[i].stmt);
            int l2 = 0;
            char *o2 = glesmod_convert_shader_source(src, &l2, STAGE_FRAGMENT);
            char what[256];
            snprintf(what, sizeof what, "%s  ->  %s", N_POS[i].stmt,
                     N_POS[i].want);
            check(o2 != NULL && strstr(o2, N_POS[i].want) != NULL, what);
            if (o2 != NULL) free(o2);
        }

        /*
         * 【反向断言 —— 比正向更重要】
         * 下列情形**绝不能被补 u**，否则会把本来合法的代码改坏。
         * 这是规则 N 最危险的地方：同一份 Flywheel 源码里，
         * `fogShape == 0` 是 int 比较，`p != 0` 是 uint 比较，
         * 两者字形单调而语义相反。
         */
        static const struct { const char *stmt; const char *forbid; } N_NEG[] = {
            /* int 变量与 int 比较：合法，不得补 u（真机 Flywheel 同源代码） */
            { "int fs; ao = fs == 0 ? 1.0 : 0.0;", "0u" },
            /* for 循环里的 int 与 0：合法 */
            { "int i; for (i = 0; i < 2; i++) { ao += 1.0; }", "0u" },
            /* 移位右操作数允许 int：实测合法，不得动 */
            { "ao = float(p >> 0);", "0u" },
            { "ao = float(p << 1);", "1u" },
            /* float 与字面量：走浮点规则（变成 0.0），绝不能变 0u */
            { "float f = 0; ao = f;", "0u" },
            /* ivec2 / 整数：合法整数除法，不得动 */
            { "uniform ivec2 UV2; ivec2 uv = UV2 / 16; ao = float(uv.x);", "16u" },
            /* 已是 uint 字面量：不得重复加后缀 */
            { "ao = float(p & 0xFFu);", "0xFFuu" },
        };
        static const size_t N_NEG_COUNT = sizeof(N_NEG) / sizeof(N_NEG[0]);

        for (size_t i = 0; i < N_NEG_COUNT; i++) {
            snprintf(src, sizeof src, N_TEMPLATE, N_NEG[i].stmt);
            int l2 = 0;
            char *o2 = glesmod_convert_shader_source(src, &l2,
                                                     STAGE_FRAGMENT);
            char what[256];
            snprintf(what, sizeof what, "[不得改写] %s", N_NEG[i].stmt);
            check(o2 != NULL && strstr(o2, N_NEG[i].forbid) == NULL, what);
            if (o2 != NULL) free(o2);
        }

        /*
         * 完整复刻真机失败的那一行，确认端到端产出正确。
         */
        static const char *RULE_N_REAL =
            "#version 330 core\n"
            "const uint _FLW_AMBIENT_OCCLUSION_LENGTH = 1u;\n"
            "struct FlwMaterial { float ambientOcclusion; };\n"
            "void _flw_unpackMaterialProperties(uint p, out FlwMaterial m) {\n"
            "    m.ambientOcclusion = (p & _FLW_AMBIENT_OCCLUSION_LENGTH) "
            "!= 0 ? 1.0 : 0.0;\n"
            "}\n";
        int l3 = 0;
        char *o3 = glesmod_convert_shader_source(RULE_N_REAL, &l3,
                                                 STAGE_FRAGMENT);
        check(o3 != NULL, "真机 Flywheel packed_material 片段：转换返回非 NULL");
        if (o3 != NULL) {
            check(strstr(o3, "!= 0u") != NULL,
                  "真机失败行已修复为 `!= 0u`");
            check(strstr(o3, "!= 0 ?") == NULL,
                  "不再残留 `!= 0 ?`（ES 下非法）");
            free(o3);
        }
    }

    /*
     * ---------------- 用例 18：规则 O（标量不可能带成员访问） ----------------
     *
     * 【真机证据】Flywheel 1.0.6 的 instancing 顶点着色器（shader=220）：
     *     struct FlwInstance { vec4 color; ivec2 overlay; vec2 light;
     *                          mat4x4 pose; };
     *     void flw_instanceVertex(in FlwInstance i) {
     *         flw_vertexPos = i.pose * flw_vertexPos;
     *     }
     * 同一份着色器另一处有 `int i = coord - start;`，
     * 于是整数标量表按名字收录了 `i`，`i.pose` 被误包成 `float(i).pose`，
     * 驱动报：
     *     ERROR: 10:2: 'constructor' : can't convert
     *     ERROR: 10:2: 'pose' : field selection requires structure, vector,
     *                          or matrix on left hand side
     *
     * 判据：GLSL 里标量绝不可能有成员/分量访问，
     *       所以「标量表命中 + 紧跟 '.'」必定是同名冲突，放弃改写。
     */
    printf("\n[18] 规则 O：标量命中后紧跟 '.' 时不得包装（Flywheel 第四例）\n");
    {
        /*
         * 完整复刻真机形态：结构体 + 同名 int 局部变量 + 成员访问。
         * 关键点是 `int i = coord - start;` 与 `in FlwInstance i` 同名。
         */
        static const char *STRUCT_SHADOW_CASE =
            "#version 330 core\n"
            "in vec4 flw_vertexPos;\n"
            "struct FlwInstance {\n"
            "    vec4 color;\n"
            "    ivec2 overlay;\n"
            "    vec2 light;\n"
            "    mat4x4 pose;\n"
            "};\n"
            "void flw_instanceVertex(in FlwInstance i) {\n"
            "    flw_vertexPos = i.pose * flw_vertexPos;\n"
            "}\n"
            "int helper(int coord, int start) {\n"
            "    int i = coord - start;\n"
            "    return i;\n"
            "}\n";
        int l4 = 0;
        char *o4 = glesmod_convert_shader_source(STRUCT_SHADOW_CASE, &l4,
                                                 STAGE_VERTEX);
        check(o4 != NULL, "结构体同名冲突：转换返回非 NULL");
        if (o4 != NULL) {
            show("转换结果", o4);
            /* 核心断言：绝不能出现 float(i).pose */
            check(strstr(o4, "float(i).pose") == NULL,
                  "未把 i.pose 误包成 float(i).pose");
            check(strstr(o4, "= i.pose * flw_vertexPos") != NULL,
                  "i.pose 保持原样（结构体成员访问）");
            /* 结构体定义本身不得被改动 */
            check(strstr(o4, "mat4x4 pose;") != NULL,
                  "结构体成员 mat4x4 pose 保留");
            free(o4);
        }

        /*
         * 【反向断言】真正的整数标量与浮点向量运算**必须照旧被包装**，
         * 不能因为规则 O 而漏改。
         */
        static const char *SCALAR_STILL_WRAPPED =
            "#version 330 core\n"
            "uniform uint u_RegionID;\n"
            "out vec3 v;\n"
            "void main() {\n"
            "    v = u_RegionID * vec3(16.0);\n"
            "}\n";
        o4 = glesmod_convert_shader_source(SCALAR_STILL_WRAPPED, &l4,
                                           STAGE_VERTEX);
        check(o4 != NULL, "纯整数标量乘法：转换返回非 NULL");
        if (o4 != NULL) {
            check(strstr(o4, "float(u_RegionID) * vec3(16.0)") != NULL,
                  "规则 O 未影响真正的整数标量包装");
            free(o4);
        }

        /*
         * 【反向断言】结构体成员访问在**没有**同名整数标量时也不得被包装，
         * 且 uvecN 向量仍要有分量参与浮点运算时的既有能力。
         */
        static const char *NO_SHADOW =
            "#version 330 core\n"
            "struct S { mat4 pose; };\n"
            "in vec4 p;\n"
            "out vec4 o;\n"
            "void f(in S i) { o = i.pose * p; }\n";
        o4 = glesmod_convert_shader_source(NO_SHADOW, &l4, STAGE_VERTEX);
        check(o4 != NULL, "无同名冲突：转换返回非 NULL");
        if (o4 != NULL) {
            check(strstr(o4, "i.pose * p") != NULL,
                  "无同名冲突时 i.pose 同样保持原样");
            free(o4);
        }
    }

    /*
     * ---------------- 用例 19：规则 P（混合 int/uint 二元运算） ----------------
     *
     * 【真机证据】Flywheel 1.0.6 的 instancing 顶点着色器（shader=220）：
     *     void _flw_main(in FlwInstance instance, in uint stableInstanceID,
     *                    in uint baseVertex) {
     *         flw_vertexId = gl_VertexID - baseVertex;
     *     }
     * `gl_VertexID` 是 int（内建），`baseVertex` 是 `in uint`。
     * Adreno 驱动报：
     *     error: no operation '-' exists that takes a left-hand operand of
     *            type 'gl_VertexID int' and a right operand of type 'in uint'
     *     error: cannot convert from 'gl_VertexID int' to 'uint'
     *
     * 【语义等价性】已用 SPIR-V 逐指令证明（probe_mixed_int_uint.ps1）：
     *   桌面隐式转换与显式 uint() 都生成 `OpBitcast %uint`，完全相同。
     *
     * 【ES 合法组合（glslc 实测）】
     *   + - * / % & | ^ 与 < > <= >= == != 都需要转换
     *   << >> 合法（右操作数允许 int），**绝不能动**
     */
    printf("\n[19] 规则 P：混合 int/uint 运算补 uint() 转换（Flywheel 最后一处阻塞）\n");
    {
        static const char *const M_TEMPLATE =
            "#version 330 core\n"
            "layout(std140) uniform B { uint baseVertex; };\n"
            "out float v;\n"
            "void main() {\n"
            "    uint acc = baseVertex;\n"
            "    %s\n"
            "    gl_Position = vec4(float(acc));\n"
            "}\n";

        /* 每项：语句 -> 必须出现的改写结果 */
        static const struct { const char *stmt; const char *want; } M_POS[] = {
            /* 真机那一行：int 内建 - uint 形参（不透明 uniform block 里的 uint） */
            { "uint vid = gl_VertexID - baseVertex;",
              "uint(gl_VertexID) - baseVertex" },
            /* 顺序反过来：uint - int —— 同样需要转换 */
            { "uint vid = baseVertex - gl_VertexID;",
              "baseVertex - uint(gl_VertexID)" },
            /* 比较运算 */
            { "bool b = gl_VertexID < baseVertex; v = b ? 1.0 : 0.0;",
              "uint(gl_VertexID) < baseVertex" },
            { "bool b = baseVertex == gl_VertexID; v = b ? 1.0 : 0.0;",
              "baseVertex == uint(gl_VertexID)" },
            /*
             * 整数字面量在 uint 一侧：由**规则 N** 先补 u 后缀处理
             * （`+ 0` -> `+ 0u`），因此规则 P 不应再包 uint()。
             * 两者都能编译，但补后缀更简洁，且规则 N 的运行顺序在前。
             * 这里断言的就是这个正确分工 —— 同时断言**不得**出现
             * 双重转换 `uint(0u)`。
             */
            { "uint r = baseVertex + 0; acc = r; v = float(acc);",
              "baseVertex + 0u" },
            /* 按位运算 */
            { "uint r = gl_VertexID | baseVertex; acc = r; v = float(acc);",
              "uint(gl_VertexID) | baseVertex" },
            /* 复合赋值：uintVar += intExpr */
            { "acc += gl_VertexID; v = float(acc);",
              "acc += uint(gl_VertexID)" },
        };
        static const size_t M_POS_COUNT = sizeof(M_POS) / sizeof(M_POS[0]);

        char msrc[512];
        for (size_t i = 0; i < M_POS_COUNT; i++) {
            snprintf(msrc, sizeof msrc, M_TEMPLATE, M_POS[i].stmt);
            int l5 = 0;
            char *o5 = glesmod_convert_shader_source(msrc, &l5, STAGE_VERTEX);
            char what[256];
            snprintf(what, sizeof what, "%s  ->  %s", M_POS[i].stmt,
                     M_POS[i].want);
            check(o5 != NULL && strstr(o5, M_POS[i].want) != NULL, what);
            if (o5 != NULL) free(o5);
        }

        /*
         * 【反向断言 —— 比正向更重要】
         * 下列情形**绝不能**被改写，否则会把本来合法的代码改坏，
         * 或把语义从「按位等价」变成「按有符号解释」。
         */
        static const struct { const char *stmt; const char *forbid; } M_NEG[] = {
            /* 移位：右操作数允许 int，实测合法，必须保持原样 */
            { "uint r = baseVertex << gl_VertexID; acc = r; v = float(acc);",
              "uint(gl_VertexID)" },
            { "uint r = baseVertex >> 3; acc = r; v = float(acc);", "uint(3)" },
            /* 纯 int 与 int 比较：合法，不得加 uint() */
            { "v = gl_VertexID < 0 ? 1.0 : 0.0;", "uint(" },
            /* uint 与 uint：本来就合法 */
            { "uint r = baseVertex + acc; acc = r; v = float(acc);",
              "uint(baseVertex)" },
            /* 不得对已带 u 后缀的字面量再做 uint() 包装（双重转换） */
            { "uint r = baseVertex + 0; acc = r; v = float(acc);",
              "uint(0" },
        };
        static const size_t M_NEG_COUNT = sizeof(M_NEG) / sizeof(M_NEG[0]);

        for (size_t i = 0; i < M_NEG_COUNT; i++) {
            snprintf(msrc, sizeof msrc, M_TEMPLATE, M_NEG[i].stmt);
            int l5 = 0;
            char *o5 = glesmod_convert_shader_source(msrc, &l5, STAGE_VERTEX);
            char what[256];
            snprintf(what, sizeof what, "[不得改写] %s", M_NEG[i].stmt);
            check(o5 != NULL && strstr(o5, M_NEG[i].forbid) == NULL, what);
            if (o5 != NULL) free(o5);
        }

        /*
         * 【防重复写出的回归断言】
         * 这里曾出过一个真实 bug：扫描到运算符时，左操作数已经写入输出，
         * 回写分支又写了一遍，产出 `gl_VertexID uint(gl_VertexID) - ...`。
         * 断言「不得出现 X uint(X) 这种同一 token 相邻重复」。
         */
        {
            static const char *DUP_CASE =
                "#version 330 core\n"
                "uniform uint baseVertex;\n"
                "out float v;\n"
                "void main() {\n"
                "    uint vid = gl_VertexID - baseVertex;\n"
                "    v = float(vid);\n"
                "    gl_Position = vec4(v);\n"
                "}\n";
            int l6 = 0;
            char *o6 = glesmod_convert_shader_source(DUP_CASE, &l6,
                                                     STAGE_VERTEX);
            check(o6 != NULL, "防重复写出：转换返回非 NULL");
            if (o6 != NULL) {
                show("转换结果", o6);
                check(strstr(o6, "uint(gl_VertexID) - baseVertex") != NULL,
                      "产出正确的 uint(gl_VertexID) - baseVertex");
                /*
                 * 重复写出的特征：`gl_VertexID` 紧跟着 `uint(gl_VertexID)`
                 * 中间只有一个空格或空白。
                 */
                check(strstr(o6, "gl_VertexIDuint(") == NULL &&
                      strstr(o6, "gl_VertexID uint(") == NULL,
                      "未出现左操作数被重复写出（gl_VertexID uint(...)）");
                free(o6);
            }
        }
    }

    /*
     * ------------- 用例 20：规则 P 绝不能碰整数向量（真机材质错乱根因） -------------
     *
     * 【真机症状】
     *   2026-10-05 真机测试：静态方块材质全错，生物与动态结构正常。
     *   日志里**没有任何编译错误** —— 这是最危险的一类缺陷。
     *
     * 【根因】
     *   Sodium 0.8.13 的区块顶点着色器（负责静态方块）里有：
     *       in uvec2 a_TexCoord;
     *       return vec2(a_TexCoord & TEXTURE_MAX_VALUE) / float(TEXTURE_MAX_COORD);
     *   `scan_int_idents` 把 `uvec2 a_TexCoord` 也收进了整数名表
     *   （规则 B' 需要「任何整数类型」都算数，见 texelFetch 的教训），
     *   而规则 P 把该表当成「确定是标量 int」使用，于是判定
     *   「int 标量 & uint」→ 包装成：
     *       return vec2(uint(a_TexCoord) & TEXTURE_MAX_VALUE) / ...
     *
     * 【为什么没报错却出错】
     *   GLSL ES 里 `uint(uvec2)` **合法**，等价于取第 0 分量。
     *   实测 SPIR-V（probe_uvec_ctor.ps1）：
     *       原始 : OpIAnd %v2uint        -> OpConvertUToF %v2float   = (u, v)
     *       改写 : OpCompositeExtract 0  -> OpCompositeConstruct %u %u = (u, u)
     *   贴图坐标从 (u,v) 塌成 (u,u)，纹理沿对角线采样 —— 材质全错。
     *
     * 【修法】规则 P 改用只含标量的名表（scan_scalar_int_idents），
     *   并给「带分量选择的操作数」加了一道保险。
     *
     * 【回归门禁的教训】
     *   当时那两个语料快照都早于本次改动，所以「CHANGED=0」比较的是
     *   同一份旧代码，等于没验证。快照必须在改动**之后**重建才有意义。
     */
    printf("\n[20] 规则 P：整数向量不得被包 uint()（真机静态方块材质错乱）\n");
    {
        /*
         * 结构与 Sodium 完全一致：uvec2 输入 + uint 常量 + 按位与。
         * `TEXTURE_MAX_VALUE` 是真 uint，这是诱导规则 P 误判的关键。
         */
        static const char *VEC_CASE =
            "#version 330 core\n"
            "in uvec2 a_TexCoord;\n"
            "const uint TEXTURE_MAX_VALUE = 32767u;\n"
            "const uint TEXTURE_MAX_COORD = 32768u;\n"
            "out vec2 v;\n"
            "vec2 getTexCoord() {\n"
            "    return vec2(a_TexCoord & TEXTURE_MAX_VALUE)\n"
            "           / float(TEXTURE_MAX_COORD);\n"
            "}\n"
            "void main() {\n"
            "    v = getTexCoord();\n"
            "    gl_Position = vec4(v, 0.0, 1.0);\n"
            "}\n";

        int l7 = 0;
        char *o7 = glesmod_convert_shader_source(VEC_CASE, &l7, STAGE_VERTEX);
        check(o7 != NULL, "整数向量场景：转换返回非 NULL");
        if (o7 != NULL) {
            show("uvec2 按位与（Sodium 区块顶点着色器结构）", o7);

            /* 必须保持原样：这是本次真机材质错乱的直接守卫 */
            check(strstr(o7, "a_TexCoord & TEXTURE_MAX_VALUE") != NULL,
                  "uvec2 & uint 保持原样（不得改写）");
            check(strstr(o7, "uint(a_TexCoord)") == NULL,
                  "[致命] 未把 uvec2 包成 uint(...)（会静默塌成 .x，贴图坐标全错）");
            check(strstr(o7, "vec2(a_TexCoord)") == NULL,
                  "未对 uvec2 做多余的 vec2(...) 包装");

            /*
             * 反向确认：真 int 标量与 uint 的混合运算**仍然**要被修
             * （否则本次修复就是把规则 P 整个关掉了）。
             */
            check(strstr(o7, "float(TEXTURE_MAX_COORD)") != NULL,
                  "规则 P 未误伤同文件内的其它表达式");

            free(o7);
        }

        /*
         * 第二个守卫：带分量选择的操作数一律放弃。
         * `a_LightAndData.z` 里 `.z` 只可能来自向量，而规则 P 只会给
         * 标量包 uint()；继续猜下去迟早踩到向量分量。
         */
        static const char *COMP_CASE =
            "#version 330 core\n"
            "in uvec4 a_LightAndData;\n"
            "uniform uint bias;\n"
            "out float v;\n"
            "void main() {\n"
            "    bool b = (a_LightAndData.z & 1u) != 0u;\n"
            "    bool c = (a_LightAndData.w > bias);\n"
            "    v = (b ? 1.0 : 0.0) + (c ? 1.0 : 0.0);\n"
            "    gl_Position = vec4(v);\n"
            "}\n";

        int l8 = 0;
        char *o8 = glesmod_convert_shader_source(COMP_CASE, &l8, STAGE_VERTEX);
        check(o8 != NULL, "分量选择场景：转换返回非 NULL");
        if (o8 != NULL) {
            show("uvec4 分量参与运算", o8);
            check(strstr(o8, "uint(a_LightAndData.z)") == NULL,
                  "[致命] 未把 uvec4 的分量包成 uint(...)");
            check(strstr(o8, "a_LightAndData.z & 1u") != NULL,
                  "uvec4 分量的按位与保持原样");
            free(o8);
        }
    }

    /*
     * ------------- 用例 21：宏展开 + 括号整数域（Flywheel 装配着色器） -------------
     *
     * 【真机证据】Flywheel 1.0.6 的装配片元着色器
     * （pipeline/instancing/frag/flywheel_material_default/
     *   flywheel_light_smooth_when_embedded_embedded.frag）
     * 原先报 49 个错误；本轮修完后只剩 3 个，且那 3 个是 **Flywheel 自身**
     * 的 `float` 函数里写 `return 0;`（上游问题，本项目不改）。
     *
     * 本用例守住的是本轮修掉的三个**我们自己的**缺陷：
     *
     *  (1) `#define` 整型常量宏未展开
     *        #define COUNT 16
     *        float(COUNT-1) / COUNT        -> 曾是 float(COUNT-1.0)
     *        clamp(int(x), 0, COUNT - 1)   -> 曾是 clamp(..., 0.0, COUNT - 1.0)
     *      同一个宏在 float 域与 int 域需要**相反**的改写，
     *      因此不能把宏名一刀切当 int（试过，只把错误从 38 降到 22）。
     *
     *  (2) 纯常量表达式被误改（规则 B/B'）
     *        add_to_index(coefficients, COUNT - 1, addend)   // 形参是 int index
     *      展开后是 `16 - 1`，无 int 变量也无浮点证据，
     *      却不该被浮点化（常量折叠无需改写）。
     *
     *  (3) 括号表达式整体是整数域时的 `int * float`
     *        ((index + 1) & 1) * exp2(-power)
     *      按位与只能作用于整数，右侧是浮点 -> 前者必须包 float(...)。
     *
     *  (4) `float >= int`（复合比较运算符）
     *        coefficient_depth >= COUNT     -> 曾是 coefficient_depth >= 16
     *
     *  (5) `exp2(-power)`：实参前带**一元负号**，而 power 是 int
     *       -> 必须产出 exp2(-float(power))，不能产出 exp2(-power)。
     */
    printf("\n[21] 宏展开 + 括号整数域（Flywheel 装配着色器）\n");
    {
        static const char *INT_MACRO_CASE =
            "#version 460\n"
            "#define TRANSPARENCY_WAVELET_COEFFICIENT_COUNT 16\n"
            "#define TRANSPARENCY_WAVELET_RANK 3\n"
            "in float depthIn;\n"
            "in sampler2DArray coefficients;\n"
            "out vec4 fragColor;\n"
            "void add_to_index(inout vec4 co[4], int index, float addend) {\n"
            "    co[index >> 2] = addend;\n"
            "}\n"
            "float get_coefficients(in sampler2DArray c, int index) {\n"
            "    return float(index);\n"
            "}\n"
            "void main() {\n"
            "    int index = 3;\n"
            "    float depth = depthIn;\n"
            "    vec4 co[4];\n"
            "    depth *= float(TRANSPARENCY_WAVELET_COEFFICIENT_COUNT-1)\n"
            "             / TRANSPARENCY_WAVELET_COEFFICIENT_COUNT;\n"
            "    int idx = clamp(int(floor(depth"
            " * TRANSPARENCY_WAVELET_COEFFICIENT_COUNT)), 0,"
            " TRANSPARENCY_WAVELET_COEFFICIENT_COUNT - 1);\n"
            "    add_to_index(co, TRANSPARENCY_WAVELET_COEFFICIENT_COUNT - 1,"
            " depth);\n"
            "    int power = TRANSPARENCY_WAVELET_RANK - index;\n"
            "    float phase = ((index + 1) & 1) * exp2(-power);\n"
            "    float t = depth >= TRANSPARENCY_WAVELET_COEFFICIENT_COUNT"
            " ? 1.0 : depth;\n"
            "    float sc = get_coefficients(coefficients,"
            " TRANSPARENCY_WAVELET_COEFFICIENT_COUNT - 1);\n"
            "    fragColor = vec4(phase, t, sc, float(idx));\n"
            "}\n";

        int l9 = 0;
        char *o9 = glesmod_convert_shader_source(INT_MACRO_CASE, &l9,
                                                 STAGE_FRAGMENT);
        check(o9 != NULL, "宏展开场景：转换返回非 NULL");
        if (o9 != NULL) {
            show("Flywheel wavelet 结构（宏 + int/float 双域）", o9);

            /* (1) #define 必须**保留**（Sodium 有 #ifndef ... #error 守卫） */
            check(strstr(o9, "#define TRANSPARENCY_WAVELET_COEFFICIENT_COUNT 16")
                      != NULL,
                  "整型常量宏的 #define 必须保留（删除会触发 #ifndef/#error）");

            /* (1) int 域：clamp 的第 2/3 实参必须保持整数 */
            check(strstr(o9, "), 0, 16 - 1)") != NULL,
                  "clamp 的整数重载实参保持整数（0 与 16 - 1）");
            check(strstr(o9, "), 0.0, 16 - 1.0)") == NULL,
                  "[致命] clamp 实参被浮点化（会选不到 int 重载）");

            /* (2) 纯常量表达式不得浮点化（形参是 int index） */
            check(strstr(o9, "add_to_index(co, 16 - 1, depth)") != NULL,
                  "纯常量表达式 16 - 1 未被浮点化（add_to_index 形参是 int）");

            /* (3) 括号整数域 -> 必须包 float(...) */
            check(strstr(o9, "float(((index + 1) & 1))") != NULL,
                  "括号整数域 ((index+1)&1) 被包 float(...)（int * float）");

            /* (4) float >= int 必须修 */
            check(strstr(o9, "depth >= 16.0") != NULL,
                  "float >= int 已改为 float >= float");

            /* (5) 一元负号 + int 变量实参 */
            check(strstr(o9, "exp2(-float(power))") != NULL,
                  "exp2(-power) 已改为 exp2(-float(power))");

            free(o9);
        }

        /*
         * 反向断言：括号里**没有**按位运算时不得包装。
         *   `(a + b) * 2.0` 里的括号是纯算术，是否为整数域取决于 a/b，
         *   由其它规则按其自身语义处理；本规则不得越权插入 float()。
         */
        static const char *NO_BITWISE_CASE =
            "#version 330 core\n"
            "uniform float u;\n"
            "out float v;\n"
            "void main() {\n"
            "    float a = 1.0;\n"
            "    float b = 2.0;\n"
            "    v = (a + b) * 2.0 + u;\n"
            "}\n";
        int l10 = 0;
        char *o10 = glesmod_convert_shader_source(NO_BITWISE_CASE, &l10,
                                                 STAGE_FRAGMENT);
        check(o10 != NULL, "纯算术括号场景：转换返回非 NULL");
        if (o10 != NULL) {
            check(strstr(o10, "float((a + b))") == NULL,
                  "[不得改写] 无按位运算的括号不被包 float(...)");
            check(strstr(o10, "(a + b) * 2.0") != NULL,
                  "纯浮点算术的括号保持原样");
            free(o10);
        }
    }

    /*
     * ---------- 用例 22：浮点函数内 `return <整数>;`（Flywheel 最后一处） ----------
     *
     * 【真机证据】Flywheel 的 flywheel:internal/wavelet.glsl：
     *     float total_absorbance(in sampler2DArray coefficients) {
     *         if (scale_coefficient == 0) {
     *             return 0;        // <-- int 字面量，函数返回 float
     *         }
     * 真机 Adreno 与 glslang 都报：
     *     'return' : type does not match, or is not convertible to the
     *                function's return type
     *
     * 【这算谁的责任 —— 已实测判定】
     *   native/tools/probe_return_int_float.ps1 用真 glslang 对照：
     *       #version 460 桌面 : ACCEPTED
     *       #version 320 es   : REJECTED
     *   桌面接受、ES 拒绝 => 与 `float f = 0;`、`vec2 * 2`、`float < int`、
     *   `pow(f, 3)` 完全同类，**属于本转换器要适配的桌面专属写法**，
     *   不是上游 bug。（先前误判为"上游 bug"，此用例即为纠正。）
     */
    printf("\n[22] 浮点函数内 `return <整数>;`（Flywheel 最后一处阻塞）\n");
    {
        static const char *RET_CASE =
            "#version 460\n"
            "in sampler2DArray coefficients;\n"
            "out vec4 fragColor;\n"
            "float get_coefficients(in sampler2DArray c, int index) {\n"
            "    return float(index);\n"
            "}\n"
            "float total_absorbance(in sampler2DArray c) {\n"
            "    float sc = get_coefficients(c, 15);\n"
            "    if (sc == 0) {\n"
            "        return 0;\n"
            "    }\n"
            "    return sc * 2;\n"
            "}\n"
            "vec3 vecReturn(int i) {\n"
            "    if (i == 0) { return 0; }\n"
            "    return vec3(1.0);\n"
            "}\n"
            "void main() {\n"
            "    fragColor = vec4(vec3(total_absorbance(coefficients))\n"
            "                     + vecReturn(1), 1.0);\n"
            "}\n";

        int l11 = 0;
        char *o11 = glesmod_convert_shader_source(RET_CASE, &l11,
                                                 STAGE_FRAGMENT);
        check(o11 != NULL, "浮点 return 场景：转换返回非 NULL");
        if (o11 != NULL) {
            show("float 函数内的 return 整数", o11);

            /* 必须补 .0（否则 ES 报 return 类型不匹配） */
            check(strstr(o11, "return 0.0;") != NULL,
                  "float 函数内 `return 0;` 已补 `.0`");
            check(strstr(o11, "return 0;") == NULL,
                  "[致命] 仍残留 `return 0;`（ES 下类型不匹配）");

            free(o11);
        }

        /*
         * 【反向断言 —— 比正向更重要】
         * 整型返回类型的函数里 `return 0;` 是**完全合法**的，
         * 绝不能被补成 `return 0.0;`，否则会把正确代码改坏
         * （而且必然报 return 类型不匹配）。
         */
        static const char *INT_RET_CASE =
            "#version 460\n"
            "out vec4 fragColor;\n"
            "int counter() {\n"
            "    return 0;\n"
            "}\n"
            "void main() {\n"
            "    fragColor = vec4(float(counter()));\n"
            "}\n";
        int l12 = 0;
        char *o12 = glesmod_convert_shader_source(INT_RET_CASE, &l12,
                                                  STAGE_FRAGMENT);
        check(o12 != NULL, "整型 return 场景：转换返回非 NULL");
        if (o12 != NULL) {
            check(strstr(o12, "return 0;") != NULL,
                  "[不得改写] int 函数内 `return 0;` 保持整数");
            check(strstr(o12, "return 0.0;") == NULL,
                  "[不得改写] 未把 int 函数的 return 改成浮点");
            free(o12);
        }

        /*
         * 【反向断言 2】非字面量的整数返回值不处理。
         *   `return someInt;`（float 函数里返回 int 变量）需要的是
         *   `float(someInt)` 而不是加 `.0`，形态完全不同；
         *   本规则保守地只管「整个返回值就是一个整数字面量」，
         *   以免误改。
         */
        static const char *VAR_RET_CASE =
            "#version 460\n"
            "out vec4 fragColor;\n"
            "float f(int k) {\n"
            "    return k;\n"
            "}\n"
            "void main() {\n"
            "    fragColor = vec4(f(1));\n"
            "}\n";
        int l13 = 0;
        char *o13 = glesmod_convert_shader_source(VAR_RET_CASE, &l13,
                                                  STAGE_FRAGMENT);
        check(o13 != NULL, "变量 return 场景：转换返回非 NULL");
        if (o13 != NULL) {
            check(strstr(o13, "return k.0;") == NULL,
                  "[不得改写] 未对标识符 return 误加 `.0`");
            free(o13);
        }
    }

    /* ---------------- 用例：预处理器 #error 消息 ---------------- */
    printf("\n[23] 预处理器：#error 的桌面字符串消息\n");
    {
        /*
         * 真机证据（Mali-G57，Sodium 0.8.13 + Iris 1.8.14）：
         *     Shader compilation log for sodium:blocks/block_layer_opaque.vsh:
         *     2:65: L0001: Unknown character '"'(34)
         *
         * 引号来自 chunk_vertex.glsl 的非活动 #else 分支：
         *     #error "Vertex compression must be enabled"
         * Mali 的词法器仍会扫描它。GLSL ES 没有字符串字面量，
         * 因此转换器必须把消息规范化为合法 token。
         */
        static const char *ERROR_DIRECTIVE_CASE =
            "#version 330 core\n"
            "#define USE_VERTEX_COMPRESSION\n"
            "#ifdef USE_VERTEX_COMPRESSION\n"
            "void main() { gl_Position = vec4(0.0); }\n"
            "#else\n"
            "#error \"Vertex compression must be enabled\"\n"
            "#endif\n";
        int l23 = 0;
        char *o23 = glesmod_convert_shader_source(ERROR_DIRECTIVE_CASE, &l23,
                                                  STAGE_VERTEX);
        check(o23 != NULL, "#error 场景：转换返回非 NULL");
        if (o23 != NULL) {
            check(strstr(o23, "#error \"") == NULL,
                  "#error 消息不再包含双引号（Mali L0001 的根因）");
            check(strstr(o23, "#error Vertex compression must be enabled")
                      != NULL,
                  "#error 消息被规范化为合法 token 序列");
            check(strstr(o23, "#error Vertex compression must be enabled\n#endif")
                      != NULL,
                  "#error 规范化保持单行，未改变 #endif 的行位置");
            free(o23);
        }

        /* 活动分支也必须保留 #error，不能被静默删除 */
        static const char *ERROR_ACTIVE_CASE =
            "#version 330 core\n"
            "#error \"unsupported path\"\n"
            "void main() { gl_Position = vec4(0.0); }\n";
        int l24 = 0;
        char *o24 = glesmod_convert_shader_source(ERROR_ACTIVE_CASE, &l24,
                                                  STAGE_VERTEX);
        check(o24 != NULL, "#error 活动分支：转换返回非 NULL");
        if (o24 != NULL) {
            check(strstr(o24, "#error unsupported path") != NULL,
                  "#error 活动分支仍保留指令（不会被静默删除）");
            check(strstr(o24, "#error \"") == NULL,
                  "活动分支的 #error 消息同样不含引号");
            free(o24);
        }
    }

    printf("\n=== 结果: %s（失败 %d 项）===\n",
           failures == 0 ? "全部通过" : "存在失败", failures);
    return failures == 0 ? 0 : 1;
}

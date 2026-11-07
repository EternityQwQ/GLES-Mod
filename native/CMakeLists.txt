cmake_minimum_required(VERSION 3.22)
project(glesmod_native C)

# The native core is deliberately one shared library: FCL dlopen()s it by path,
# and LWJGL resolves every GL symbol from this image. Keep the source list
# explicit so a new .c file cannot be silently picked up by a glob.
add_library(gl_gles SHARED
    src/core.c
    src/custom.c
    src/enum.c
    src/generated_forwarders.c
    src/probe.c
    src/shader.c
)

target_include_directories(gl_gles PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(gl_gles PRIVATE c_std_11)
target_compile_definitions(gl_gles PRIVATE _GNU_SOURCE)

# Only GLESMOD_EXPORT symbols are visible. This keeps the 849 exported GL
# entry points from being buried under helper/state symbols, and mirrors the
# visibility model the forwarder generator assumes.
target_compile_options(gl_gles PRIVATE
    -O2
    -fvisibility=hidden
    -Wall
)

# liblog: __android_log_print in core.c
# libdl:  dlopen/dlsym/dladdr
# libm:   math helpers used by the shader converter
target_link_libraries(gl_gles PRIVATE
    log
    dl
    m
)

set_target_properties(gl_gles PROPERTIES
    OUTPUT_NAME gl_gles
    PREFIX "lib"
)

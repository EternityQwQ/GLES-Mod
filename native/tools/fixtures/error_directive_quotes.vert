#version 330 core

// Minimal reproduction of the guard in Sodium's chunk_vertex.glsl.
// The branch is inactive because USE_VERTEX_COMPRESSION is defined, but
// Mali's lexer still scans the quoted #error message and fails with
//     2:65: L0001: Unknown character '"'(34)
// The converter must normalise the message to GLSL ES tokens.
#define USE_VERTEX_COMPRESSION

#ifdef USE_VERTEX_COMPRESSION
in vec3 a_Position;

void main() {
    gl_Position = vec4(a_Position, 1.0);
}
#else
#error "Vertex compression must be enabled"
#endif

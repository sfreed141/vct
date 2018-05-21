#version 430 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

#define warpDim 32
layout (invocations = warpDim) in;

in VS_OUT {
    vec2 tc;
} gs_in[];

out VS_OUT {
    vec2 tc;
} gs_out;


void main() {
    for (int v = 0; v < 3; ++v) {
        gl_Position = gl_in[v].gl_Position;
        gl_Layer = gl_InvocationID;
        gs_out.tc = gs_in[v].tc;
        EmitVertex();
    }
    EndPrimitive();
}

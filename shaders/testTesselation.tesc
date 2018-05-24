#version 430 core

layout(vertices = 3) out;

out vec4 tcPosition[];

void main() {
    // if (gl_InvocationID == 0) {
        gl_TessLevelInner[0] = 10.0;
        gl_TessLevelOuter[0] = 10.0;
        gl_TessLevelOuter[1] = 10.0;
        gl_TessLevelOuter[2] = 10.0;
    // }

    // gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    tcPosition[gl_InvocationID] = gl_in[gl_InvocationID].gl_Position;
}
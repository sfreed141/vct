#version 430 core

layout(vertices = 3) out;

in vec3 fragPosition[];
in vec3 fragNormal[];
in vec2 fragTexcoord[];

out vec4 tcPosition[];
out vec3 tcNormal[];
out vec2 tcTexcoord[];

uniform float voxelDim = 256;
uniform vec3 voxelMin = vec3(-20), voxelMax = vec3(20), voxelCenter = vec3(0);

void main() {
    if (gl_InvocationID == 0) {
        // World positions of vertices (ccw)
        vec3 a = fragPosition[0], b = fragPosition[1], c = fragPosition[2];
        // Triangle edges (corresponds to edge opposite of respective vertex)
        vec3 A = c - b, B = c - a, C = b - a;

        // gl_TessLevelInner[0] = 1.0;
        // gl_TessLevelOuter[0] = 1.0;
        // gl_TessLevelOuter[1] = 1.0;
        // gl_TessLevelOuter[2] = 1.0;

        gl_TessLevelInner[0] = 1.0;
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = 1.0;
        gl_TessLevelOuter[2] = 1.0;
    }

    // gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    // tcPosition[gl_InvocationID] = gl_in[gl_InvocationID].gl_Position;

    tcPosition[gl_InvocationID] = vec4(fragPosition[gl_InvocationID], 1);
    tcNormal[gl_InvocationID] = fragNormal[gl_InvocationID];
    tcTexcoord[gl_InvocationID] = fragTexcoord[gl_InvocationID];
}
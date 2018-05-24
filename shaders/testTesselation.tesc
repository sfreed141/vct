#version 430 core

layout(vertices = 3) out;

in vec3 fragPosition[];
in vec3 fragNormal[];
in vec2 fragTexcoord[];

out vec4 tcPosition[];
out vec3 tcNormal[];
out vec2 tcTexcoord[];

void main() {
    if (gl_InvocationID == 0) {
        gl_TessLevelInner[0] = 4.0;
        gl_TessLevelOuter[0] = 4.0;
        gl_TessLevelOuter[1] = 4.0;
        gl_TessLevelOuter[2] = 4.0;
    }

    // gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    // tcPosition[gl_InvocationID] = gl_in[gl_InvocationID].gl_Position;

    tcPosition[gl_InvocationID] = vec4(fragPosition[gl_InvocationID], 1);
    tcNormal[gl_InvocationID] = fragNormal[gl_InvocationID];
    tcTexcoord[gl_InvocationID] = fragTexcoord[gl_InvocationID];
}
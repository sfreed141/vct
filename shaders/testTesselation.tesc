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
        gl_TessLevelInner[0] = 1.0;
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = 1.0;
        gl_TessLevelOuter[2] = 1.0;


        // World positions of vertices (ccw)
        vec3 a = fragPosition[0], b = fragPosition[1], c = fragPosition[2];
        // Triangle edges (corresponds to edge opposite of respective vertex)
        vec3 A = c - b, B = c - a, C = b - a;

        vec3 lengths = vec3(length(A), length(B), length(C));
        float s = (lengths.x + lengths.y + lengths.z) * 0.5;    // semiperimeter
        float area = sqrt(s * (s - lengths.x) * (s - lengths.y) * (s - lengths.z)); // Heron's formula
        vec3 altitudes = 2 * area / lengths;
        float maxAltitude = max(altitudes.x, max(altitudes.y, altitudes.z));

        vec3 voxelSize = (voxelMax - voxelMin) / voxelDim;
        // vec3 divfactors = abs(vec3(
        //     dot(normalize(A), voxelSize),
        //     dot(normalize(B), voxelSize),
        //     dot(normalize(C), voxelSize)
        // ));
        vec3 divfactors = abs(vec3(
            length(normalize(A) * voxelSize),
            length(normalize(B) * voxelSize),
            length(normalize(C) * voxelSize)
        ));
        // vec3 divfactors = vec3(length(voxelSize));

        vec3 outer = max(vec3(1.0), lengths / divfactors);        // what if bigger than max tess level?
        gl_TessLevelInner[0] = max(1.0, maxAltitude / voxelSize.x);
        gl_TessLevelOuter[0] = outer.z;
        gl_TessLevelOuter[1] = outer.x;
        gl_TessLevelOuter[2] = outer.y;
    }

    // gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    // tcPosition[gl_InvocationID] = gl_in[gl_InvocationID].gl_Position;

    tcPosition[gl_InvocationID] = vec4(fragPosition[gl_InvocationID], 1);
    tcNormal[gl_InvocationID] = fragNormal[gl_InvocationID];
    tcTexcoord[gl_InvocationID] = fragTexcoord[gl_InvocationID];
}
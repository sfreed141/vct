#version 430 core

layout(vertices = 3) out;

in vec3 fragPosition[];
in vec3 fragNormal[];
in vec2 fragTexcoord[];

out vec4 tcPosition[];
out vec3 tcNormal[];
out vec2 tcTexcoord[];
out vec3 tcVoxelPosition[];

uniform float voxelDim = 256;
uniform vec3 voxelMin = vec3(-20), voxelMax = vec3(20), voxelCenter = vec3(0);

vec3 getVoxelPosition(vec3 p) {
    return (p - voxelCenter - voxelMin) / (voxelMax - voxelMin);
}

ivec3 getVoxelIndex(vec3 p) {
    return ivec3(getVoxelPosition(p));
}

void main() {
    tcPosition[gl_InvocationID] = vec4(fragPosition[gl_InvocationID], 1);
    tcNormal[gl_InvocationID] = fragNormal[gl_InvocationID];
    tcTexcoord[gl_InvocationID] = fragTexcoord[gl_InvocationID];

    tcVoxelPosition[gl_InvocationID] = getVoxelPosition(tcPosition[gl_InvocationID].xyz);

    if (gl_InvocationID == 0) {
        gl_TessLevelInner[0] = 1.0;
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = 1.0;
        gl_TessLevelOuter[2] = 1.0;


        // World positions of vertices (ccw)
        vec3 a = tcPosition[0].xyz, b = tcPosition[1].xyz, c = tcPosition[2].xyz;

        ivec3 voxelA = ivec3(voxelDim * tcVoxelPosition[0]), voxelB = ivec3(voxelDim * tcVoxelPosition[1]), voxelC = ivec3(voxelDim * tcVoxelPosition[2]);
        if (all(equal(voxelA, voxelB)) && all(equal(voxelA, voxelC))) {
            // TODO should do one atomicaverage
            gl_TessLevelInner[0] = 0.0;
            gl_TessLevelOuter[0] = 0.0;
            gl_TessLevelOuter[1] = 0.0;
            gl_TessLevelOuter[2] = 0.0;
        }
        else {
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
    }
}